/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
*                                                                            *
*    This program is free software: you can redistribute it and/or modify    *
*    it under the terms of the GNU General Public License as published by    *
*    the Free Software Foundation, either version 3 of the License, or       *
*    (at your option) any later version.                                     *
*                                                                            *
*    This program is distributed in the hope that it will be useful,         *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
*    GNU General Public License for more details.                            *
*                                                                            *
*    You should have received a copy of the GNU General Public License       *
*    along with this program. If not, see http://www.gnu.org/licenses/.      *
*****************************************************************************/
#include "staticfilehandler.h"
#include <http/datetime.h>
#include <http/expiresctrl.h>
#include <http/httpconnection.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/handlertype.h>
#include <http/httpmime.h>
#include <http/httpreq.h>
#include <http/httpresourcemanager.h>
#include <http/httpresp.h>
#include <http/httprange.h>
#include <http/httpserverconfig.h>
#include <http/staticfilecache.h>
#include <http/staticfilecachedata.h>
#include <http/staticfiledata.h>
#include <util/gzipbuf.h>
#include <util/iovec.h>
#include <http/moov.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <util/ssnprintf.h>


RedirectHandler::RedirectHandler()
{
    setHandlerType( HandlerType::HT_REDIRECT );
}
RedirectHandler::~RedirectHandler()
{}
const char * RedirectHandler::getName() const
{
    return "redirect";
}


StaticFileHandler::StaticFileHandler()
{
    setType( 0 );
}

const char * StaticFileHandler::getName() const
{
    return "static";
}

StaticFileHandler::~StaticFileHandler()
{}


inline int buildStaticFileHeaders( HttpResp * pResp, StaticFileData * pData )
{
    pData->setCurPos( 0 );
    pData->setCurEnd( pData->getECache()->getFileSize() );
    pResp->setContentLen( pData->getECache()->getFileSize() );
    pResp->parseAdd( pData->getCache()->getHeaderBuf(),
                      pData->getCache()->getHeaderLen() );
    pResp->parseAdd( pData->getECache()->getCLHeader().c_str(),
                      pData->getECache()->getCLHeader().len() );
    return 0;
}



#include <http/httpglobals.h>
#define READ_BUF_SIZE 16384

static int cacheSend( HttpConnection* pConn, StaticFileData * pData, int remain )
{
    const char * pBuf;
    off_t written;
    long len;
#if !defined( NO_SENDFILE )
    if ( HttpServerConfig::getInstance().getUseSendfile() &&
         pData->getECache()->getfd() != -1 && !pConn->isSSL() )
    {
        len = pConn->writeRespBodySendFile( pData->getECache()->getfd(),
                                            pData->getCurPos(), remain );
        if ( len > 0 )
        {
            pData->incCurPos( len );
        }
        return (pData->getRemain() > 0 );
    }
#endif
    while( remain > 0 )
    {
        len = (remain < READ_BUF_SIZE)? remain : READ_BUF_SIZE ;
        written = remain;
        pBuf = pData->getECache()->getCacheData(
                pData->getCurPos(), written, HttpGlobals::g_achBuf, len );
        if ( written <= 0 )
        {
            return -1;
        }
        len = pConn->writeRespBody( pBuf, written );
        if ( len > 0 )
        {
            pData->incCurPos( len );
            if ( len < written )
                return 1;
        }
        else
            return 1;
        remain = pData->getRemain();
    }
    return ( remain > 0 );
}

// static int cacheSend( HttpConnection* pConn, int written,
//             const char * pPrefixBuf, int &prefixLen )
// {
//     StaticFileData* pData = pConn->getReq()->getStaticFileData();
//     long len = (written < READ_BUF_SIZE)? written : READ_BUF_SIZE ;
//     const char * pBuf;
//     pBuf = pData->getECache()->getCacheData(
//             pData->getCurPos(), written, HttpGlobals::g_achBuf, len );
//     if ( written <= 0 )
//     {
//         return -1;
//     }
//     IOVec iov;
//     iov.append( pPrefixBuf, prefixLen );
//     iov.append( pBuf, written );
//     int total = prefixLen + written;
//     written = pConn->writeRespBodyv( iov, total );
//     if ( written - prefixLen > 0 )
//         pData->incCurPos( written - prefixLen );
//     else
//         prefixLen = written;
//     return ( pData->getRemain() > 0 );
// }

static int addExpiresHeader( HttpResp * pResp, StaticFileCacheData * pData,
            const ExpiresCtrl *pExpires )
{
    time_t expire;
    int    age;
    switch( pExpires->getBase() )
    {
    case EXPIRES_ACCESS:
        age = pExpires->getAge();
        expire = DateTime::s_curTime + age;
        break;
    case EXPIRES_MODIFY:
        expire = pData->getLastMod() + pExpires->getAge();
        age = (int)expire - (int)DateTime::s_curTime;
        break;
    default:
        return 0;
    }
    
    char sTemp[RFC_1123_TIME_LEN + 1] = {0};
    HttpRespHeaders & buf = pResp->getRespHeaders();
    buf.add(HttpRespHeaders::H_CACHE_CTRL, "Cache-Control", 13, "max-age=", 8);
    int n = safe_snprintf(sTemp, RFC_1123_TIME_LEN, "%d", age);
    buf.appendLastVal("Cache-Control", 13, sTemp, n);

    DateTime::getRFCTime( expire, sTemp );
    buf.add(HttpRespHeaders::H_EXPIRES, "Expires", 7, sTemp, RFC_1123_TIME_LEN);
    return 0;
}

#define FLV_MIME "video/x-flv" 
#define FLV_HEADER "FLV\x1\x1\0\0\0\x9\0\0\0\x9"
#define FLV_HEADER_LEN (sizeof(FLV_HEADER)-1)

#define MP4_MIME "video/mp4"

static int processFlvStream( HttpConnection * pConn, off_t start )
{
    HttpReq * pReq = pConn->getReq();
    StaticFileData * pData = pReq->getStaticFileData();
    
    FileCacheDataEx * &pECache = pData->getECache();
    int ret = pData->getCache()->readyCacheData( pECache, 0 );
    if ( !ret )
    {
        HttpResp * pResp = pConn->getResp();
        pECache->incRef();
        pConn->resetResp();
        pResp->prepareHeaders( pReq, 0 );

        pResp->parseAdd( pData->getCache()->getHeaderBuf(),
                          pData->getCache()->getHeaderLen() );
        
        pData->setCurPos( start );
        pData->setCurEnd( pData->getECache()->getFileSize() );
        pResp->setContentLen( pData->getECache()->getFileSize() - 
                            start + FLV_HEADER_LEN );
        pResp->appendContentLenHeader();
        pResp->finalizeHeader( pReq->getVersion(), pReq->getStatusCode(), pReq->getVHost() );
        pResp->appendExtra( FLV_HEADER, FLV_HEADER_LEN );
        pResp->written( FLV_HEADER_LEN );
        ret = pConn->beginWrite();
        if ( ret > 0 )
        {
            pConn->continueWrite();
            return 0;
        }
    }
    return ret;
}

#include <arpa/inet.h>

int calcMoovContentLen( HttpConnection * pConn, off_t &contentLen )
{
    int ret = 0;
    HttpReq * pReq = pConn->getReq();
    StaticFileData * pData = pReq->getStaticFileData();
    
    FileCacheDataEx * &pECache = pData->getECache();

    unsigned char * mini_moov = NULL;
    uint32_t mini_moov_size;
    mini_moov = pData->getCache()->getMiniMoov();
    mini_moov_size = pData->getCache()->getMiniMoovSize();

    contentLen = 0;

    moov_data_t * moov_data = (moov_data_t *)pData->getParam();
    if ( !moov_data )
        return -1;

    while( moov_data->remaining_bytes > 0 )
    {
        ret = get_moov(
                    pECache->getfd(),
                    moov_data->start_time,
                    0.0,
                    moov_data,
                    mini_moov,
                    mini_moov_size);
        if( ret == -1 )
        {
            return -1;
        }
        if( moov_data->is_mem == 1 )
        {
            contentLen += moov_data->mem.buf_size;
            free(moov_data->mem.buffer);
            moov_data->mem.buffer = NULL;
        }else{
            contentLen += moov_data->file.data_size;
        }
    }

    //static char mdat_header64[16] = { 0, 0, 0, 1, 'm', 'd', 'a', 't' };
    uint64_t mdat_start;
    uint64_t mdat_size;
    int      mdat_64bit;

    ret= get_mdat(
                pECache->getfd(),
                moov_data->start_time,
                0.0,
                & mdat_start,
                & mdat_size,
                & mdat_64bit,
                mini_moov,
                mini_moov_size);
    if ( ret == -1 )
        return -1;
    if ( mdat_64bit )
    {
        contentLen += 16;
    }
    else
    {
        contentLen += 8;
    }
    contentLen += mdat_size;
    return 1;

}


int buildMoov( HttpConnection * pConn )
{
    int ret = 0;
    HttpReq * pReq = pConn->getReq();
    StaticFileData * pData = pReq->getStaticFileData();
    
    FileCacheDataEx * &pECache = pData->getECache();

    unsigned char * mini_moov = NULL;
    uint32_t mini_moov_size;
    mini_moov = pData->getCache()->getMiniMoov();
    mini_moov_size = pData->getCache()->getMiniMoovSize();

    moov_data_t * moov_data = (moov_data_t *)pData->getParam();
    if ( !moov_data )
        return -1;

    while( moov_data->remaining_bytes > 0 )
    {
        ret = get_moov(
                    pECache->getfd(),
                    moov_data->start_time,
                    0.0,
                    moov_data,
                    mini_moov,
                    mini_moov_size);
        if( ret == -1 )
        {
            return -1;
        }
        if( moov_data->is_mem == 1 )
        {
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( pReq->getLogger(), "[%s] is_mem, buf_size=%u, remaining=%d", 
                    pReq->getLogId(), moov_data->mem.buf_size, moov_data->remaining_bytes));
            pConn->appendDynBody(0, (char*)moov_data->mem.buffer, moov_data->mem.buf_size);
            free(moov_data->mem.buffer);
            moov_data->mem.buffer = NULL;
        }else{
            //pConn->flushDynBodyChunk();
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( pReq->getLogger(), "[%s] send from file, start=%u, buf_size=%u, remaining=%d", 
                    pReq->getLogId(), (uint32_t)moov_data->file.start_offset, moov_data->file.data_size, 
                    moov_data->remaining_bytes));
            pData->setCurPos( moov_data->file.start_offset );
            pData->setCurEnd( moov_data->file.start_offset + moov_data->file.data_size );
            return 1;
            //sendfile( pECache->getfd(), moov_data.file.start_offset, 
            //            moov_data.file.data_size );
            //fseek(infile, moov_data.file.start_offset, SEEK_SET);
            //copy_data(infile, outfile, moov_data.file.data_size);
        }
    }

    pConn->getReq()->clearContextState( MP4_SEEK );

    static char mdat_header64[16] = { 0, 0, 0, 1, 'm', 'd', 'a', 't' };
    uint64_t mdat_start;
    uint64_t mdat_size;
    int      mdat_64bit;
    uint32_t * pLen32;

    ret= get_mdat(
                pECache->getfd(),
                moov_data->start_time,
                0.0,
                & mdat_start,
                & mdat_size,
                & mdat_64bit,
                mini_moov,
                mini_moov_size);
    free( moov_data );
    pData->setParam( NULL );
    if ( ret == -1 )
        return -1;
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( pReq->getLogger(), "[%s] mdat_start=%u, mdat_size=%u, mdat_64bit=%d\n",
                pReq->getLogId(), (uint32_t)mdat_start, (uint32_t)mdat_size, mdat_64bit ));
    pLen32 = (uint32_t *)(&mdat_header64[8]);
    if ( mdat_64bit )
    {
        *pLen32++ = htonl((uint32_t)( (mdat_size + 16)>> 32));
        *pLen32 = htonl( (uint32_t)( (mdat_size + 16) & 0xffffffff));
        pConn->appendDynBody( 0, mdat_header64, 16 );
    }
    else
    {
        *pLen32 = htonl( (uint32_t)( (mdat_size + 8) & 0xffffffff));
        memmove( &mdat_header64[12], "mdat", 4 );
        pConn->appendDynBody( 0, &mdat_header64[8], 8 );
    }
    //pConn->flushDynBodyChunk();
    pData->setCurPos( mdat_start );
    pData->setCurEnd( mdat_start + mdat_size  );
    return 1;

}

int processH264Stream( HttpConnection * pConn, double start )
{
    HttpReq * pReq = pConn->getReq();
    StaticFileData * pData = pReq->getStaticFileData();
    
    FileCacheDataEx * &pECache = pData->getECache();
    int ret = pData->getCache()->readyCacheData( pECache, 0 );
    if ( ret )
        return ret;
    pECache->incRef();
    unsigned char * mini_moov = NULL;
    uint32_t mini_moov_size;
    mini_moov = pData->getCache()->getMiniMoov();
    if ( !mini_moov )
    {   
        if ( get_mini_moov( pECache->getfd(),   //in - video file descriptor
                &mini_moov,                     //out
                &mini_moov_size                 //out
                ) == 1 )
        {
            pData->getCache()->setMiniMoov( mini_moov, mini_moov_size );
        }
        else
        {
            LOG_NOTICE(( pReq->getLogger(), "[%s] Failed to parse moov header from MP4/H.264 video file [%s].",
                        pReq->getLogId(), pReq->getRealPath()->c_str() ));
            return SC_500;
        }
    }
        
    moov_data_t * moov_data;
    moov_data = (moov_data_t *)malloc( sizeof( moov_data_t ) );
    if ( !moov_data )
        return SC_500;
    pConn->getReq()->orContextState( MP4_SEEK );
    pData->setParam( moov_data );
    moov_data->remaining_bytes = 1;  //first call
    moov_data->start_time = start;

    pConn->resetResp();

    off_t contentLen = 0;
    if ( calcMoovContentLen( pConn, contentLen ) == -1 )
    {
        LOG_NOTICE(( pReq->getLogger(), "[%s] Failed to calculate content length for seek request for MP4/H.264 video file [%s].",
                        pReq->getLogId(), pReq->getRealPath()->c_str() ));
        return SC_500;
    }
    pConn->getResp()->setContentLen( contentLen );

    pConn->setupRespCache();
    //pConn->getReq()->setVersion( HTTP_1_0 );
    pConn->getReq()->keepAlive( 0 );
    pConn->getResp()->getRespHeaders().add(HttpRespHeaders::H_CONTENT_TYPE,  "Content-Type", 12, "video/mp4", 9 );
    pConn->prepareDynRespHeader(0, 2 );


    moov_data->remaining_bytes = 1;  //first call
    ret = buildMoov( pConn );
    if ( ret <= 1 )
    {
        ret = pConn->beginWrite();
        if ( ret > 0 )
        {
            pConn->continueWrite();
            return 0;
        }
    }
    return ret;
}

static int processRange( HttpConnection * pConn, HttpReq * pReq, const char *pRange );


////////////////////////////////////////////////////////////
//      Internal functions end
////////////////////////////////////////////////////////////

int StaticFileHandler::process( HttpConnection * pConn, const HttpHandler * pHandler )
{
    int ret;
    HttpReq* pReq = pConn->getReq();
    HttpResp * pResp = pConn->getResp();
    struct stat &st = pReq->getFileStat();
    int code = pReq->getStatusCode();
    
    int isSSI = (pReq->getSSIRuntime() != NULL );//Xuedong Add for SSI
    
//     if (pReq->getMethod() >= HttpMethod::HTTP_POST)
//         return SC_405;
    StaticFileData * pData = pReq->getStaticFileData();    
    StaticFileCacheData * &pCache = pReq->setStaticFileCache();
    if (( !pCache )||( pCache->isDirty( st,
                    pReq->getMimeType(), pReq->getDefaultCharset() )))
    {
        ret = HttpGlobals::getStaticFileCache()->getCacheElement( pReq, pCache );
        if ( ret )
        {
            if ( D_ENABLED( DL_LESS ) )
                LOG_D(( pReq->getLogger(), "[%s] getCacheElement() return %d",
                        pReq->getLogId(), ret));
            pData->setCache( NULL );
            return ret;
        }
    }
    pCache->incRef();
    register unsigned int bitReq = HttpServerConfig::getInstance().getRequiredBits();
    if (( (st.st_mode & bitReq) != bitReq )||
        ( (st.st_mode & HttpServerConfig::getInstance().getForbiddenBits() ) ))
    {
        if ( !pReq->getMimeType()->getExpires()->isImage() )
        {
            LOG_INFO(( pReq->getLogger(), "[%s] Permission of file [%s] does not "
                        "meet the requirements of 'Required bits' or "
                        "'Restricted bits', access denied.",
                        pReq->getLogId(), pReq->getRealPath()->c_str() ));
            return SC_403;
        }
    }
    
    //if ( code == SC_200 )
    if ((!isSSI)&&( code == SC_200 )) //Xuedong Add for SSI
    {
        if (( pReq->getQueryStringLen() >= 7 ) && 
            ( strcmp( pData->getCache()->getMimeType()->getMIME()->c_str(), 
                            FLV_MIME ) == 0 ))
        {
            const char * pQS = pReq->getQueryString();
            const char * p = pQS - 1;
            if (( strncmp( pQS, "start=", 6 ) == 0 )||
                ( ( p = strstr( pQS, "&start=" ) ) ) )
            {
                p += 7;
                long start = strtol( p, NULL, 10 );
                if (( start > 0 )&& (start < st.st_size) )
                {
                    return processFlvStream( pConn, start );
                }
            }
        }
        if (( pReq->getQueryStringLen() >= 7 ) && 
            ( strcmp( pData->getCache()->getMimeType()->getMIME()->c_str(), 
                            MP4_MIME ) == 0 ))
        {
            const char * pQS = pReq->getQueryString();
            const char * p = pQS - 1;
            if (( strncmp( pQS, "start=", 6 ) == 0 )||
                ( ( p = strstr( pQS, "&start=" ) ) ) )
            {
                p += 7;
                double start = strtod( p, NULL );
                if ( start > 0.0 )
                {
                    ret = processH264Stream( pConn, start );
                    if ( ret >= 0 )
                        return ret;
                }
            }
        }        
        if ( pReq->isHeaderSet( HttpHeader::H_RANGE ) )
        {
            const char * pRange = pReq->getHeader( HttpHeader::H_RANGE );
            if ( pReq->isHeaderSet( HttpHeader::H_IF_RANGE ) )
            {
                const char * pIR = pReq->getHeader( HttpHeader::H_IF_RANGE );
                ret = pData->getCache()->testIfRange( pIR
                    , pReq->getHeaderLen( HttpHeader::H_IF_RANGE ) );
            }
            else
            {
                ret = pData->getCache()->testUnMod( pReq );
                if ( ret )
                    return ret;
            }
            if ( !ret && pData->getCache()->getFileSize() > 0 )
                return processRange( pConn, pReq, pRange );
        }
        else
        {
            if ( pData->getCache()->testMod( pReq ) )
            {
                pReq->setStatusCode( SC_304 );
                pReq->setNoRespBody();
                code = SC_304;
            }
        }
    }
    FileCacheDataEx * &pECache = pData->getECache();
    ret = pData->getCache()->readyCacheData( pECache, 
             pReq->gzipAcceptable() == GZIP_REQUIRED );
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( pReq->getLogger(), "[%s] readyCacheData() return %d",
                pReq->getLogId(), ret));
    if ( !ret )
    {
        pECache->incRef();
        if ( pReq->isKeepAlive() )
            pReq->smartKeepAlive( pCache->getMimeType()->getMIME()->c_str() );
        if ( !isSSI ) //Xuedong Add for SSI
        {
            pConn->resetResp();
            pResp->prepareHeaders( pReq, RANGE_HEADER_LEN );
            switch( code )
            {
            case SC_304:
                pResp->parseAdd( pData->getCache()->getHeaderBuf(),
                        pData->getCache()->getETagHeaderLen() );
                break;
            case SC_200:
                {
                    const ExpiresCtrl *pExpireDefault = pReq->shouldAddExpires();
                    if ( pExpireDefault )
                    {
                        ret = addExpiresHeader( pResp, pCache, pExpireDefault );
                        if ( ret )
                            return ret;
                    }
                }
                //fall through
            default:
                buildStaticFileHeaders( pResp, pData );
                if ( pECache == pCache->getGziped() )
                {
                    pResp->addGzipEncodingHeader();
                }
            }
            pResp->finalizeHeader( pReq->getVersion(), pReq->getStatusCode(), pReq->getVHost());
        } //Xuedong Add for SSI Start
        else
        {
            pConn->prepareDynRespHeader(0, 1 );
            //pConn->flushDynBody( 1 );
            if (pConn->getGzipBuf()&&
                ( pECache != pCache->getGziped() ))
            {
                if ( !pConn->getGzipBuf()->isStreamStarted() )
                {
                    pConn->getGzipBuf()->reinit();
                }
            }
            else
            {
                pConn->flushDynBodyChunk();
            }

            pData->setCurPos( 0 );
            pData->setCurEnd( pECache->getFileSize() );            
        } //Xuedong Add for SSI end
        ret = pConn->beginWrite();
        if ( ret > 0 )
        {
            pConn->continueWrite();
            return 0;
        }
    }
    return ret;

}

int StaticFileHandler::cleanUp( HttpConnection* pConn )
{
    StaticFileCacheData *pData = pConn->getReq()->getStaticFileData()->getCache();
    if ( pData )
    {
        if ( pData->decRef() == 0 )
        {
            pData->setLastAccess( DateTime::s_curTime );
        }
        FileCacheDataEx * pECache = pConn->getReq()->getStaticFileData()->getECache();
        if ( pECache &&( pECache->decRef() == 0 ))
        {
            if ( pECache->getfd() != -1 )
                pECache->closefd();
        }
    }
    return 0;
}

static int sendMultipart( HttpConnection * pConn, HttpRange& range );

int StaticFileHandler::onWrite( HttpConnection* pConn, int aioSent )
{
    int ret = 0;
    if ( aioSent > 0 )
        pConn->getReq()->getStaticFileData()->incCurPos( aioSent );
    if ( pConn->getRespCache() )
    {
        int ret = pConn->sendDynBody();
        if ( ret != 0 )
            return ret;
    }
    HttpRange* range = pConn->getReq()->getRange();
    if (( range )&&(range->count() > 1 ))
    {
        return sendMultipart( pConn, *range );
    }
    else
    {
        StaticFileData * pData = pConn->getReq()->getStaticFileData();
        off_t remain = pData->getRemain();
        if ( remain > 0 )
        {
            ret = cacheSend( pConn, pData, remain );
        }
        if (( ret == 0 )&& ( pConn->getReq()->getContextState( MP4_SEEK ) ))
            return buildMoov( pConn );
    }
    return ret;
}

bool StaticFileHandler::notAllowed( int Method ) const
{
    return (Method >= HttpMethod::HTTP_POST);
}


static int buildRangeHeaders( HttpConnection* pConn, HttpRange& range )
{
    HttpResp * pResp = pConn->getResp();
    StaticFileData * pData1 = pConn->getReq()->getStaticFileData();
    StaticFileCacheData * pData = pData1->getCache();
    int bodyLen;
    HttpRespHeaders & buf = pResp->getRespHeaders();
    
    if ( range.count() == 1 )
    {
        pResp->parseAdd( pData->getHeaderBuf(), pData->getHeaderLen() );
        long begin, end;
        int ret = range.getContentOffset( 0, begin, end );
        if ( ret )
            return SC_500;
        
        char sTemp[8192];
        ret = range.getContentRangeString( 0, sTemp, 8191 );
        if ( ret == -1 )
            return SC_500;
        buf.add(HttpRespHeaders::H_CONTENT_RANGE, "Content-Range", 13, sTemp, ret);
        
        bodyLen = end - begin;
        pData1->setCurPos( begin );
        pData1->setCurEnd( end );
    }
    else
    {
        pResp->parseAdd( pData->getHeaderBuf(), pData->getValidateHeaderLen() );
        range.beginMultipart();
        buf.add(HttpRespHeaders::H_CONTENT_RANGE, "Content-Range", 13, "multipart/byteranges; boundary=", 31);
        buf.appendLastVal("Content-Range", 13, range.getBoundary(), strlen(range.getBoundary()) );
        bodyLen = range.getMultipartBodyLen( pData->getMimeType()->getMIME() );
    }
    pResp->setContentLen( bodyLen );
    pResp->appendContentLenHeader();
    return 0;
}



static int sendMultipart( HttpConnection * pConn, HttpRange& range )
{
    long iRemain;
    int  ret = 0;
    StaticFileData* pData = pConn->getReq()->getStaticFileData();
    while( true )
    {
        iRemain = pData->getRemain();
        if ( !iRemain )
        {
            if ( range.more() )
            {
                range.next();
                range.buildPartHeader(
                    pData->getCache()->getMimeType()->getMIME()->c_str() );
            }
            if ( range.more() )
            {
                long begin, end;
                range.getContentOffset( begin, end );
                pData->setCurPos( begin );
                pData->setCurEnd( end );
                iRemain = pData->getRemain();
                assert( iRemain > 0 );
            }
 /*           else
            {
                int len = range.getPartHeaderLen();
                if ( !len )
                    return 0;
                ret = pConn->writeRespBody( range.getPartHeader(), len );
                if ( ret >= len )
                {
                    return 0;
                }
                else
                {
                    range.partHeaderSent( ret );
                    return 1;
                }
            }
*/            
        }
        int len = range.getPartHeaderLen();
        if ( len )
        {
            ret = pConn->writeRespBody( range.getPartHeader(), len );
            if ( ret > 0 )
            {
                int r = ret;
                range.partHeaderSent( ret );
                if ( r < len )
                    return 1;
            }
            else if ( ret == -1 )
                return -1;
            else if ( !ret )
                return 1;
        }
        if ( iRemain )
            ret = cacheSend(pConn, pData, iRemain);
        else
            return 0;
        if ( ret )
        {
            return ret;
        }
    }
    return 0;

}

static int processRange( HttpConnection * pConn, HttpReq * pReq, const char *pRange )
{
    StaticFileData * pData = pReq->getStaticFileData();
    HttpRange* range = new HttpRange( pData->getCache()->getFileSize() );
    int ret = range->parse( pRange );
    if ( ret )
    {
        pReq->setContentLength( pData->getCache()->getFileSize() );
        delete range;
        if ( ret == SC_416 )  //Range unsatisfiable
        {
            HttpRespHeaders &buf = pConn->getResp()->getRespHeaders();
            buf.add(HttpRespHeaders::H_CONTENT_RANGE, "Content-Range", 13, "bytes */", 8);
            
            int n;
            char sTemp[32] = {0};
            if ( sizeof( off_t ) == 8 )
                n = safe_snprintf( sTemp, 31, "%lld", (long long)pData->getCache()->getFileSize());
            else
                n = safe_snprintf( sTemp, 31, "%ld", (long)pData->getCache()->getFileSize());
            buf.appendLastVal("Content-Range", 13, sTemp, n);
        }
    }
    else
    {
        FileCacheDataEx * &pECache = pData->getECache();
        pReq->setStatusCode( SC_206 );
        pReq->setRange( range );
        ret = pData->getCache()->readyCacheData( pECache, 0 );
        if ( !ret )
        {
            HttpResp * pResp = pConn->getResp();
            pECache->incRef();
            pConn->resetResp();
            pResp->prepareHeaders( pReq, RANGE_HEADER_LEN );
            ret = buildRangeHeaders( pConn, *range);
            pResp->finalizeHeader( pReq->getVersion(), pReq->getStatusCode(), pReq->getVHost());
            if ( !ret )
            {
                ret = pConn->beginWrite();
                if ( ret > 0 )
                {
                    pConn->continueWrite();
                    return 0;
                }
            }
        }
    }
    return ret;
}

