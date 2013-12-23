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
#include "httpreq.h"
#include <http/accesscache.h>
#include <http/clientinfo.h>
#include <http/datetime.h>
#include <http/denieddir.h>
#include <http/handlertype.h>
#include <http/handlerfactory.h>
#include <http/hotlinkctrl.h>
#include <http/htauth.h>
#include <http/httpcontext.h>
#include <http/httpdefs.h>
#include <http/httplog.h>
#include <http/httpglobals.h>
#include <http/httpmime.h>
#include <http/httprange.h>
#include <http/httpresourcemanager.h>
#include <http/httpserverconfig.h>
#include <http/httpstatuscode.h>
#include <http/httputil.h>
#include <http/httpvhost.h>
#include <http/platforms.h>
#include <http/smartsettings.h>
#include <http/vhostmap.h>
#include <util/gpath.h>
#include <util/ni_fio.h>
#include <util/pool.h>
#include <util/stringtool.h>
#include <util/vmembuf.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <new>
#include <util/ssnprintf.h>

#define HEADER_BUF_INIT_SIZE 2048
#define REQ_BUF_INIT_SIZE    1024
static const char s_hex[17] = "0123456789ABCDEF";
static char * escape_uri( char * p, char * pEnd, const char * pURI, int uriLen )
{
    const char * pURIEnd = pURI + uriLen;
    register char ch;
    while( (pURI < pURIEnd) && (p < pEnd - 3 ) )
    {
        ch = *pURI++;
        if ( isalnum(ch) )
            *p++ = ch;
        else
        {
            switch( ch )
            {
            case '_': case '-': case '!': case '~': case '*': case '\\':
            case '(': case ')': case '/': case '\'': case ';': case '$':
            case '.':
                *p++ = ch;
                break;
            default:
                *p++ = '%';
                *p++ = s_hex[ ((unsigned char )ch) >> 4 ];
                *p++ = s_hex[ ch & 0xf ];
                break;
            }
        }
    }
    return p;
}
HttpReq::HttpReq( )
    : m_reqBuf( REQ_BUF_INIT_SIZE )
    , m_headerBuf( HEADER_BUF_INIT_SIZE )
{
    m_urls = (key_value_pair *)g_pool.allocate( sizeof( key_value_pair ) * ( MAX_REDIRECTS + 1 ));
    if ( !m_urls )
        throw std::bad_alloc();
    m_reqBuf.resize(HEADER_BUF_PAD);
    m_headerBuf.resize(HEADER_BUF_PAD);
    *((int*)m_reqBuf.begin()) = 0;
    *((int*)m_headerBuf.begin() ) = 0;
    m_iReqHeaderBufFinished = HEADER_BUF_PAD;
    //m_pHTAContext = NULL;
    m_pContext = NULL;
    m_pSSIRuntime = NULL;
    ::memset( m_commonHeaderLen, 0,
              (char *)(&m_iLocationOff + 1) - (char *)m_commonHeaderLen );
    m_upgradeProto = UPD_PROTO_NONE;
}

HttpReq::~HttpReq()
{
    if ( m_urls )
        g_pool.deallocate( m_urls, sizeof( key_value_pair ) * ( MAX_REDIRECTS + 1 ) );
}


void HttpReq::reset()
{
    if ( m_pReqBodyBuf )
    {
        if ( !m_pReqBodyBuf->isMmaped() )
            delete m_pReqBodyBuf;
        else
        {
            HttpGlobals::getResManager()->recycle( m_pReqBodyBuf );
        }
    }
    if ( m_pRange )
    {
        delete m_pRange;
    }
    ::memset( m_commonHeaderOffset, 0,
              (char *)(&m_iLocationOff + 1) - (char *)m_commonHeaderOffset );
    m_reqBuf.resize(HEADER_BUF_PAD);
}

void HttpReq::resetHeaderBuf()
{
    m_iReqHeaderBufFinished = HEADER_BUF_PAD;
    m_headerBuf.resize(HEADER_BUF_PAD);
}

void HttpReq::reset2()
{
    reset();
    if ( m_headerBuf.size() - m_iReqHeaderBufFinished > 0 )
    {
        memmove( m_headerBuf.begin() + HEADER_BUF_PAD,
                 m_headerBuf.begin() + m_iReqHeaderBufFinished,
                 m_headerBuf.size() - m_iReqHeaderBufFinished );
        m_headerBuf.resize( HEADER_BUF_PAD + m_headerBuf.size() - m_iReqHeaderBufFinished );
        m_iReqHeaderBufFinished = HEADER_BUF_PAD;
    }
    else
        resetHeaderBuf();
}

int HttpReq::appendPendingHeaderData( const char * pBuf, int len )
{
    if ( m_headerBuf.available() < len )
        if ( m_headerBuf.grow( len ) )
            return -1;
    memmove( m_headerBuf.end(), pBuf, len );
    m_headerBuf.used( len );
    return 0;
}

static inline int growBuf( AutoBuf &buf, int len )
{
    int g = 1024;
    if ( g < len )
        g = len;
    if ( buf.grow( g ) == -1 )
        return SC_500;
    return 0;
}

int HttpReq::processHeader()
{
    if ( m_iHeaderStatus == HEADER_REQUEST_LINE )
    {
        int ret = processRequestLine();
        if ( ret )
        {
            if (( m_iHeaderStatus == HEADER_REQUEST_LINE)&&
                    ( m_iReqHeaderBufFinished >=
                      HttpServerConfig::getInstance().getMaxURLLen() ))
            {
                m_reqLineLen = m_iReqHeaderBufFinished - m_reqLineOff;
                return SC_414;
            }
        }

        return ret;
    }
    else
        return processHeaderLines();
}

#define MAX_HEADER_TAG_LEN  20


const HttpVHost * HttpReq::matchVHost()
{
    if ( !m_pVHostMap )
        return NULL;
    m_pVHost = m_pVHostMap->getDedicated();
    if ( !m_pVHost )
    {
        register char * pHost = m_headerBuf.begin() + m_iHostOff;
        register char * pHostEnd = pHost + m_iHostLen;
        register char ch = *pHostEnd;
        if ( m_iLeadingWWW )
            pHost += 4;
        *pHostEnd = 0;
        m_pVHost = m_pVHostMap->matchVHost( pHost, pHostEnd );
        *pHostEnd = ch;
    }
    if ( m_pVHost )
        ((HttpVHost *)m_pVHost)->incRef();
    return m_pVHost;
}

int HttpReq::RemoveSpace(const char **pCur, const char *pBEnd)
{
    while( 1 )
    {
        if ( *pCur >= pBEnd)
        {
            m_iReqHeaderBufFinished = *pCur - m_headerBuf.begin();
            return 1;
        }
        if ( isspace( **pCur ))
            ++*pCur;
        else
        {
            return 0;
        }
    }
}
int HttpReq::processRequestLine()
{
    int result =0;
    register const char *pCur = m_headerBuf.begin() + m_iReqHeaderBufFinished;
    register const char *pBEnd = m_headerBuf.end();
    int iBufLen = pBEnd - pCur;
    if ( pBEnd > HttpServerConfig::getInstance().getMaxURLLen() + m_headerBuf.begin() )
        pBEnd = HttpServerConfig::getInstance().getMaxURLLen() + m_headerBuf.begin();
    const char* pLineEnd = ( const char *)memchr( pCur, '\n', pBEnd - pCur );
    while(( pLineEnd = ( const char *)memchr( pCur, '\n', pBEnd - pCur )) != NULL )
    {
        while( pCur < pLineEnd )
        {
            if ( !isspace( *pCur ) )
                break;
            ++pCur;
        }
        if ( pCur != pLineEnd )
            break;
        ++pCur;
        m_iReqHeaderBufFinished = pCur - m_headerBuf.begin();
    }
    if(pLineEnd == NULL)
    {
       if(iBufLen < MAX_BUF_SIZE + 20)
           return 1;
       else
        return SC_400;
    }
    m_reqLineOff = pCur - m_headerBuf.begin();

    const char * p = pCur;
    while(( p < pLineEnd )&&(( *p != ' ' )&&( *p != '\t' )))
        ++p;
    result = GetReqMethod( pCur, p );
    if ( result )
        return result;

    pCur = p+1;
    while(( p < pLineEnd )&&(( *p == ' ' )||( *p == '\t' )))
        ++p;
    pCur = p;
    result = GetReqHost(pCur, pLineEnd);
    if (result )
        return result;
    p += m_iHostLen;
   pCur = (char*)memchr(p, '/', (pLineEnd -p));
    while(( p < pLineEnd )&&(( *p != ' ' )&&( *p != '\t' )))
        ++p;
    if ( p == pLineEnd )
        return SC_400;
    m_curURL.keyOff = m_reqBuf.size();
    m_reqURLOff = pCur - m_headerBuf.begin();
    result = GetReqURL(pCur, p );
    if ( result )
        return result;

    pCur = p+1;
    while(( p < pLineEnd )&&(( *p == ' ' )||( *p == '\t' )))
        ++p;
    pCur = p;

    result = GetReqVersion(pCur, pLineEnd );
    if( result )
        return result;
    m_reqLineLen = pLineEnd - m_headerBuf.begin() - m_reqLineOff;
    if ( *(pLineEnd - 1) == '\r')
        m_reqLineLen --;

    m_iHeaderStatus = HEADER_HEADER;
    m_iHS1 = 0;
    m_iHS2 = HttpHeader::H_HEADER_END;
    m_iHS3 = 0;
    m_iReqHeaderBufFinished = pLineEnd + 1 - m_headerBuf.begin();
    result = processHeaderLines();

    return result;
}
int HttpReq::GetReqVersion(const char *pCur, const char *pBEnd)
{
    if ( strncasecmp( pCur, "http/1.", 7 ) != 0 )
    {
        return SC_400;
    }
    pCur += 7;
    {
        if ( *pCur == '1' )
        {
            m_ver = HTTP_1_1;
            keepAlive( 1 );
            ++pCur;
        }
        else if ( *pCur == '0' )
        {
            keepAlive( 0 );
            m_ver = HTTP_1_0;
            *(char *)pCur++ = '1';  //for mod_proxy, always use HTTP/1.1
        }
        else
        {
            m_ver = HTTP_1_1;
            return SC_505;
        }
    }
    return 0;
}

int HttpReq::GetReqURL(const char *pCur, const char *pBEnd)
{
    const char *pFound = pCur,*pMark = ( const char *)memchr( pCur, '?', pBEnd - pCur );
    int result = 0;
    if(pMark)
    {
        pCur = pMark + 1;
        m_curURL.valOff = pCur - m_headerBuf.begin();
        m_curURL.valLen = pBEnd - pCur;
    }
    else
    {
        m_curURL.valOff = 0;
        m_curURL.valLen = 0;
        pMark = pBEnd;
    }
    result = GetReqURI(pFound, pMark);
    if(result != 0)
    {
        return result;
    }
    m_reqURLLen = pBEnd - m_headerBuf.begin() - m_reqURLOff ;
    //m_reqURLLen = pCur - m_headerBuf.begin() - m_reqURLOff;
    return 0;
}

int HttpReq::GetReqURI(const char *pCur, const char *pBEnd )
{
    int len = pBEnd - pCur;
    int m = m_reqBuf.available();
    if(m < len )
    {
        if (m_reqBuf.grow( len - m ) == -1)
        {
            return SC_500;
        }
    }
    int n = HttpUtil::unescape((char*) m_reqBuf.end(), len, pCur );
    --n;
    n = GPath::clean( m_reqBuf.end(), n );
    m_curURL.keyOff = m_reqBuf.size();
    if(n > 0)
        m_reqBuf.used(n);
    else
        return SC_400;
    m_reqBuf.append( '\0' );
    m_curURL.keyLen = n;
    return 0;
}

int HttpReq::GetReqHost(const char* pCur, const char *pBEnd)
{
    bool bNoHost = false;
    if (( *pCur == '/' )||( *pCur == '%' ))
        bNoHost = true;
    if(!bNoHost)
    {
        const char * pHost;
        pHost = ( const char *)memchr( pCur, ':', pBEnd - pCur );
        if (( pHost )&&(pHost +2 < pBEnd))
        {
            if((*(pHost + 1)=='/' )&&(*(pHost + 2)=='/' ))
                pCur = pHost + 2;
            else
                return SC_400;
        }
        else
        {
            m_iReqHeaderBufFinished = pCur - m_headerBuf.begin();
            return 1;
        }
        if(memchr(pCur + 1, '/', (pBEnd -pCur -1))!= NULL)
               HostProcess(pCur + 1, pBEnd);
        else
        {
            pCur = pBEnd;
            m_iReqHeaderBufFinished = m_headerBuf.size();
            return 1;
        }
        if ( !m_iHostLen )
            m_iHostOff = 0;
    }
    return 0;
}
int HttpReq::GetReqMethod(const char *pCur, const char *pBEnd )
{

    if ( (m_method = HttpMethod::parse2( pCur )) == 0 )
    {
        return SC_400;
    }

    if ( m_method == HttpMethod::HTTP_HEAD )
        m_iNoRespBody = 1;

    if (( m_method < HttpMethod::HTTP_OPTIONS )||
            ( m_method > HttpMethod::DAV_SEARCH ))
    {
        return SC_505;
    }
    pCur += HttpMethod::getLen( m_method );
    if ((*pCur != ' ')&&(*pCur != '\t' ))
    {
        return SC_400;
    }
    return 0;
}

int HttpReq::processHeaderLines()
{
    const char *pCur = m_headerBuf.begin() + m_iReqHeaderBufFinished;
    const char *pBEnd = m_headerBuf.end();
    key_value_pair * pCurHeader = NULL;
    const char *pMark = NULL;
    const char *pLineEnd= NULL;
    const char* pLineBegin  = pCur;
    const char* pTemp = NULL;
    const char* pTemp1 = NULL;
    bool headerfinished = false;
    int index;
    while((pLineEnd  = ( const char *)memchr(pLineBegin, '\n', pBEnd - pLineBegin )) != NULL)
    {
        pMark = ( const char *)memchr(pLineBegin, ':', pLineEnd - pLineBegin);
        if(pMark != NULL)
        {
            while(1)
            {
                if(pLineEnd + 1 >= pBEnd)
                    return 1;
                if((*(pLineEnd+1)==' ')||(*(pLineEnd+1)=='\t'))
                {
                    *((char*)pLineEnd) = ' ';
                    if(*(pLineEnd-1)=='\r')
                        *((char*)pLineEnd -1) = ' ';
                }
                else
                    break;
                pLineEnd  = ( const char *)memchr(pLineEnd, '\n', pBEnd - pLineEnd );
                if( pLineEnd == NULL)
                    return 1;
                else
                    continue;
            }
            pTemp = pMark + 1;
            pTemp1 = pLineEnd;
            SkipSpaceBothSide(pTemp, pTemp1);
            index = HttpHeader::getIndex( pLineBegin );
            if(index != HttpHeader::H_HEADER_END)
            {
                m_commonHeaderLen[ index ] = pTemp1 - pTemp;
                m_commonHeaderOffset[index] = pTemp - m_headerBuf.begin();
                int Result = ProcessHeader(index);
                if(Result != 0)
                    return Result;
            }
            else
            {
                pCurHeader = newUnknownHeader();
                pCurHeader->keyOff = pLineBegin - m_headerBuf.begin();
                pCurHeader->keyLen = SkipSpace( pMark, pLineBegin) - pLineBegin;
                pCurHeader->valOff = pTemp - m_headerBuf.begin();
                pCurHeader->valLen = pTemp1 - pTemp;
                
                if ( pCurHeader->keyLen == 7 && strncasecmp(pLineBegin, "Upgrade", 7) == 0 &&
                    pCurHeader->valLen == 9 && strncasecmp(pTemp, "websocket", 9) == 0 )
                {
                    m_upgradeProto = UPD_PROTO_WEBSOCKET;
                }
            }
        }
        pLineBegin = pLineEnd + 1;
        if((*(pLineEnd-1)=='\r')&&(*(pLineEnd-2)=='\n'))
        {
            headerfinished = true;
            break;
        }
    }
    m_iReqHeaderBufFinished = pLineBegin - m_headerBuf.begin();
    if(headerfinished)
    {
        m_iHttpHeaderEnd = m_iReqHeaderBufFinished;
        m_iHeaderStatus = HEADER_OK;
        return 0;
    }
    return 1;
}

int HttpReq::ProcessHeader(int index)
{
    const char *pCur = m_headerBuf.begin() + m_commonHeaderOffset[index];
    const char *pBEnd = pCur + m_commonHeaderLen[ index ];
    switch (index)
    {
    case HttpHeader::H_HOST:
        m_iHostOff = m_commonHeaderOffset[index];
        HostProcess(pCur, pBEnd);
        break;
    case HttpHeader::H_CONTENT_LENGTH:
        m_lEntityLength = strtol(pCur, NULL, 0);
        break;
    case HttpHeader::H_TRANSFER_ENCODING:
        if (strncasecmp( pCur, "chunked", 7 ) == 0 )
        {
            if ( getMethod() <= HttpMethod::HTTP_HEAD )
                return SC_400;
            m_lEntityLength = CHUNKED;
        }
        break;
    case HttpHeader::H_ACC_ENCODING:
        if(m_commonHeaderLen[ index ] >= 4)
        {
            char ch = *pBEnd;
            *((char*)pBEnd) = 0;
            if(strstr(pCur, "gzip") != NULL)
                m_iAcceptGzip = REQ_GZIP_ACCEPT |
                        HttpServerConfig::getInstance().getGzipCompress();
            *((char*)pBEnd) = ch;
        }
        break;
    case HttpHeader::H_CONNECTION:
        if ( m_ver == HTTP_1_1 )
            keepAlive( strncasecmp( pCur, "clos", 4 ) != 0 );
        else
            keepAlive( strncasecmp( pCur, "keep", 4 ) == 0 );
        break;
    default:
        break;
    }
    return 0;
}

int HttpReq::HostProcess(const char* pCur, const char *pBEnd)
{
    const char *pstrfound;
    if((pCur+4) < pBEnd)
    {
        register char ch1;
        int iNum = pBEnd - pCur;
        for(int i = 0; i < iNum; i++)
        {
            ch1 = pCur[i];
            if (( ch1 >= 'A' )&&( ch1 <= 'Z' ))
            {
                ch1 |= 0x20;
                ((char*)pCur)[i] = ch1;
            }
        }
        if(strncmp((char *)pCur, "www.", 4) == 0)
            m_iLeadingWWW = 1;
    }
    pstrfound = (char*)memchr(pCur, ':', (pBEnd -pCur));
    if(pstrfound)
        m_iHostLen = pstrfound - pCur;
    else
        m_iHostLen = pBEnd - pCur;
    if ( !m_iHostLen )
        m_iHostOff = 0;

    return 0;
}

const char* HttpReq::SkipSpace(const char *pOrg, const char *pDest)
{
    char ch;
    if(pOrg > pDest)
    {
        //pTemp = pOrg - 1;
        for(; pOrg > pDest; pOrg--)
        {
            ch = *(pOrg -1);
            if((ch == ' ')||(ch == '\t')||(ch == '\r'))
                continue;
            else
                break;
        }
    }
    else
    {
        for(; pOrg < pDest; pOrg++)
        {
            if((*pOrg == ' ')||(*pOrg == '\t')||(*pOrg == '\r'))
                continue;
            else
                break;
        }
    }
    return pOrg;
}

int HttpReq::SkipSpaceBothSide(const char* &pHBegin, const char* &pHEnd)
{
    char ch;
    for(; pHBegin <=pHEnd; pHEnd--)
    {
        if(pHBegin ==pHEnd)
            break;
        ch = *(pHEnd-1);
        if((ch == ' ')||(ch == '\t')||(ch == '\r'))
            continue;
        else
            break;
    }
    if(pHBegin+1 >= pHEnd)
        return (pHEnd - pHBegin);
    for(; pHBegin <=pHEnd; pHBegin++)
    {
        if(pHBegin ==pHEnd)
            break;
        if((*pHBegin == ' ')||(*pHBegin == '\t')||(*pHBegin == '\r'))
            continue;
        else
            break;
    }
    return (pHEnd - pHBegin);
}


key_value_pair * HttpReq::newKeyValueBuf( int &idxOff)
{
    char * p = NULL;
    int orgSize;
    int newSize;
    int used;
    if ( idxOff == 0 )
    {
        orgSize = 0;
        used = 0;
    }
    else
    {
        p = m_reqBuf.getPointer( idxOff );
        orgSize = *((int *)p );
        used = *( ((int *)p) + 1 );
    }
    if ( used == orgSize )
    {
        newSize = orgSize + 10;
        int total = sizeof( int ) * 2 + sizeof( key_value_pair ) * newSize;
        //pad buffer to begin with 4 bytes alignment
        int pad = m_reqBuf.size() % 4;
        if ( pad )
        {
            m_reqBuf.used( 4 - pad );
        }
        idxOff = m_reqBuf.size();
        if ( m_reqBuf.available() < total )
            if ( m_reqBuf.grow( total ) )
                return NULL;
        char * pNewBuf = m_reqBuf.end();
        m_reqBuf.used( total );
        if ( orgSize > 0 )
            memmove( pNewBuf, p, sizeof( int ) * 2 + sizeof( key_value_pair ) * orgSize );
        else
            *( ((int *)pNewBuf) + 1 ) = 0;
        p = pNewBuf;
        *((int *)p ) = newSize;
    }
    ++*( ((int *)p) + 1 );
    return ( key_value_pair *)( p + sizeof( int ) * 2 ) + used;
}

key_value_pair * HttpReq::getCurHeaderIdx()
{
    char * p = m_reqBuf.getPointer( m_headerIdxOff );
    int used = *( ((int *)p) + 1 );
    return ( key_value_pair *)( p + sizeof( int ) * 2 ) + (used-1);
}


key_value_pair * HttpReq::getValueByKey( const AutoBuf & buf, int idxOff, const char * pName,
                                  int namelen ) const
{
    if ( !idxOff )
        return NULL;
    const char * p = m_reqBuf.getPointer( idxOff );
    key_value_pair * pIndex = ( key_value_pair *)( p + sizeof( int ) * 2 );
    key_value_pair * pEnd = pIndex + *( ((int *)p) + 1 );
    while( pIndex < pEnd )
    {
        if ( namelen == pIndex->keyLen )
        {
            p = buf.getPointer( pIndex->keyOff );
            if ( strncasecmp( p, pName, namelen ) == 0 )
            {
                return pIndex;
            }
        }
        ++pIndex;
    }
    return NULL;
}


int HttpReq::processNewReqData(const struct sockaddr * pAddr)
{
    if ( !m_pVHost->isEnabled() )
    {
        LOG_INFO(( getLogger(),
                   "[%s] VHost disabled [%s], access denied.",
                   getLogId(),getVHost()->getName() ));
        return SC_403;
    }
    if ( !m_pVHost->enableGzip() )
        andGzip( ~GZIP_ENABLED );
    AccessCache * pAccessCache = m_pVHost->getAccessCache();
    if ( pAccessCache )
    {
        if (//!m_accessGranted &&
            !pAccessCache->isAllowed( pAddr ))
        {
            LOG_INFO(( getLogger(),
                       "[%s] [ACL] Access to virtual host [%s] is denied",
                       getLogId(),getVHost()->getName() ));
            return SC_403;
        }
    }

    m_iScriptNameLen = m_curURL.keyLen;
    if ( D_ENABLED( DL_LESS ) )
    {
        const char * pQS = getQueryString();
        char * pQSEnd = (char *)pQS + getQueryStringLen();
        char ch = 0;
        if ( *pQS )
        {
            ch = *pQSEnd;
            *pQSEnd = 0;
        }
        LOG_D(( getLogger(),
                "[%s] New request: \n\tMethod=[%s], URI=[%s],\n"
                "\tQueryString=[%s]\n\tContent Length=%d\n",
                getLogId(), HttpMethod::get( getMethod() ), getURI(),
                pQS, getContentLength() ));
        if ( *pQS )
            *pQSEnd = ch;

    }
    //access control of virtual host
    return 0;
}


int HttpReq::getPort() const
{
    return m_pVHostMap->getPort();
}

const AutoStr2& HttpReq::getPortStr() const
{
    return m_pVHostMap->getPortStr();
}

const AutoStr2* HttpReq::getLocalAddrStr() const
{
    return m_pVHostMap->getAddrStr();
}


void HttpReq::getAAAData( struct AAAData &aaa, int &satisfyAny )
{
    m_pContext->getAAAData( aaa );
    if ( aaa.m_pHTAuth )
    {
        m_pHTAuth = aaa.m_pHTAuth;
        m_pAuthRequired = m_pContext->getAuthRequired();
    }
    satisfyAny = m_pContext->isSatisfyAny();
    if ( m_pFMContext )
    {
        if ( m_pFMContext->getConfigBits() & BIT_ACCESS )
            aaa.m_pAccessCtrl = m_pFMContext->getAccessControl();
        if ( m_pFMContext->getConfigBits() & BIT_AUTH )
            m_pHTAuth = aaa.m_pHTAuth = m_pFMContext->getHTAuth();
        if ( m_pFMContext->getConfigBits() & BIT_AUTH_REQ )
            m_pAuthRequired = aaa.m_pRequired = m_pFMContext->getAuthRequired();
        if ( m_pFMContext->getConfigBits() & BIT_SATISFY )
            satisfyAny      = m_pFMContext->isSatisfyAny();
    }
}





int HttpReq::translatePath( const char * pURI, int uriLen, char * pReal, int len ) const
{

    const HttpContext * pContext = m_pContext;
    *pReal = 0;
    if (( !pContext )||
            (strncmp( pContext->getURI(), pURI, pContext->getURILen() ) != 0 ) )
    {
        pContext = m_pVHost->bestMatch( pURI );
    }
    if ( pContext->getHandlerType() > HandlerType::HT_STATIC )
        pContext = NULL;
    return translate( pURI, uriLen, pContext, pReal, len );
}

int HttpReq::translate( const char * pURI, int uriLen,
                        const HttpContext* pContext, char * pReal, int len ) const
{
    char * pHead = pReal;
    int l;
    *pReal = 0;
    //offset context URI from request URI
    if ( pContext != NULL )
    {
        //assert( pContext->match( pURI ) );
        l = pContext->getLocationLen();
        if ( l >= len )
            return -1;
        ::memmove( pReal, pContext->getLocation(), l );
        len -= l;
        pReal += l;
        if ( '/' == *pURI )
        {
            uriLen -= pContext->getURILen();
            pURI += pContext->getURILen();
        }
    }
    else
    {
        const AutoStr2 * pDocRoot = m_pVHost->getDocRoot();
        l = pDocRoot->len();
        if ( l >= len )
            return -1;
        ::memmove( pReal , pDocRoot->c_str(), l );
        len -= l;
        pReal += l;
        ++pURI;
        --uriLen;
    }
    if ( uriLen > 0 )
    {
        if ( uriLen > len )
            return -1;
        ::memmove( pReal, pURI, uriLen );
        pReal += uriLen;
    }
    *pReal = 0;
    return pReal - pHead;
}

int HttpReq::setCurrentURL( const char * pURL, int len, int alloc )
{
    assert( pURL );
    if ( *pURL != '/' )
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] URL is invalid - not start with '/'.", getLogId() ));
        return SC_400;
    }
    if ( getLocation() == pURL )
    {
        //share the buffer of location
        m_curURL.keyOff = m_iLocationOff;
        m_iLocationOff = 0;
    }
    else
    {
        if ( m_reqBuf.available() < len )
        {
            int ret;
            if (( ret = growBuf( m_reqBuf, len )) )
                return ret;
        }
        m_curURL.keyOff = m_reqBuf.size();
    }
    char * pURI = m_reqBuf.getPointer( m_curURL.keyOff );
    const char * pArgs = pURL ;
    //decode URI
    m_curURL.keyLen = len;
    int n = HttpUtil::unescape_inplace( pURI, m_curURL.keyLen, pArgs );
    if ( n == -1 )
    {
        if ( D_ENABLED( DL_LESS ) )
            LOG_D(( getLogger(), "[%s] Unable to decode request URI: %s.",
                    getLogId(), pURI ));
            return SC_400;    //invalid url format
    }
    m_curURL.valLen = n - 1 - ( pArgs -  pURI);
    if ((alloc)||( m_curURL.valLen > 0 )||( !m_pSSIRuntime ))
    {
        if ( m_iLocationOff != m_curURL.keyOff )
        {
            m_curURL.valOff = -(m_curURL.keyOff + ( pArgs - pURI ));
            m_reqBuf.used( (n + 3) & (~3) );
        }
        else
            m_curURL.valOff = m_curURL.keyOff + ( pArgs - pURI );
    }
    else
    {
        m_curURL.valOff = m_urls[m_iRedirects-1].valOff;
        m_curURL.valLen = m_urls[m_iRedirects-1].valLen;
    }
    m_iScriptNameLen = m_curURL.keyLen;
    m_iPathInfoLen = 0;
    return 0;
}


int HttpReq::setRewriteURI( const char * pURL, int len, int no_escape )
{
    int totalLen = len + ((!no_escape)? len * 2: 0);
    if ( totalLen > MAX_URL_LEN )
        totalLen = MAX_URL_LEN;
    if ( m_reqBuf.available() < totalLen + 4)
        if ( m_reqBuf.grow( totalLen ) == -1 )
            return SC_500;
    m_curURL.keyOff = m_reqBuf.size();
    char * p = m_reqBuf.end();
    char * pEnd = p + m_reqBuf.available() - 1;
    if ( !no_escape )
        p = escape_uri( p, pEnd, pURL, len );
    else
    {
        memcpy( p, pURL, len );
        p += len;
    }
    *p = 0;
    m_curURL.keyLen = p - m_reqBuf.end();
    m_iScriptNameLen = m_curURL.keyLen;
    m_reqBuf.used( ((m_iScriptNameLen + 1) + 3)& ~3  );
    return 0;
}
int HttpReq::appendIndexToUri( const char * pIndex, int indexLen )
{
    if ( m_curURL.keyOff + m_curURL.keyLen + 1 >= m_reqBuf.size() )
    {
        if ( m_reqBuf.available() < indexLen )
        {
            int ret;
            if (( ret = growBuf( m_reqBuf, indexLen )) )
                return ret;
        }
        memmove( m_reqBuf.end() - 1, pIndex, indexLen + 1 );
        m_curURL.keyLen += indexLen;
        m_iScriptNameLen = m_curURL.keyLen;
        m_reqBuf.used( indexLen );
        return 0;

    }
    int len = m_curURL.keyLen + indexLen + 1;
    if ( m_reqBuf.available() < len )
    {
        int ret;
        if (( ret = growBuf( m_reqBuf, len )) )
            return ret;
    }
    memmove( m_reqBuf.end(), getURI(), m_curURL.keyLen );
    memmove( m_reqBuf.end() + m_curURL.keyLen, pIndex, indexLen + 1 );
    m_curURL.keyOff = m_reqBuf.size();
    m_curURL.keyLen = len - 1;
    m_iScriptNameLen = m_curURL.keyLen;
    m_iPathInfoLen = 0;
    m_reqBuf.used( len );
    return 0;
}


int HttpReq::setRewriteQueryString(  const char * pQS, int len )
{
    ++len;
    if ( m_reqBuf.available() < len )
    {
        int ret;
        if (( ret = growBuf( m_reqBuf, len )) )
            return ret;
    }
    m_curURL.valOff = -m_reqBuf.size();
    memmove( m_reqBuf.end(), pQS, len );
    m_reqBuf.used( (len +3 ) & ~3 );
    m_curURL.valLen = len - 1;
    return 0;
}

int HttpReq::setRewriteLocation( char * pURI, int uriLen,
                                 const char * pQS, int qsLen, int escape )
{
    int totalLen = uriLen + qsLen + 2 + (escape)? uriLen: 0;
    if ( totalLen > MAX_URL_LEN )
        totalLen = MAX_URL_LEN;
    if ( m_reqBuf.available() < totalLen )
        if ( m_reqBuf.grow( totalLen ) == -1 )
            return SC_500;
    char * p = m_reqBuf.end();
    char * pEnd = p + m_reqBuf.available();
    if ( escape )
        p += HttpUtil::escape( pURI, p, pEnd - p - 4 );
    else
    {
        memcpy( p, pURI, uriLen );
        p += uriLen;
    }
    if ( *pQS )
    {
        *p++ = '?';
        if ( qsLen > pEnd - p )
            qsLen = pEnd - p;
        memmove( p, pQS, qsLen );
        p += qsLen;
    }
    *p = 0;
    m_iLocationOff = m_reqBuf.size();
    m_iLocationLen = p - m_reqBuf.end();
    m_reqBuf.used( ((m_iLocationLen + 1) + 3)& ~3  );
    return 0;
}

int HttpReq::saveCurURL()
{
    if ( m_iRedirects >= MAX_REDIRECTS )
    {
        LOG_ERR(( getLogger(), "[%s] Maximum number of redirect reached.", getLogId() ));
        return SC_500;
    }
    m_urls[m_iRedirects++] = m_curURL;
    return 0;
}


int HttpReq::internalRedirect( const char * pURL, int len, int alloc )
{
    assert( pURL );
    int ret = saveCurURL();
    if ( ret )
        return ret;
    setCurrentURL( pURL, len, alloc );
    return detectLoopRedirect();
}



int HttpReq::internalRedirectURI( const char * pURI, int len, int resetPathInfo, int no_escape )
{
    assert( pURI );
    int ret = saveCurURL();
    if ( ret )
        return ret;
    setRewriteURI( pURI, len, no_escape );
    if ( resetPathInfo )
    {
        m_iPathInfoLen = 0;
        return detectLoopRedirect();
    }
    return 0;
}

int HttpReq::detectLoopRedirect()
{
    const char * pURI = getURI();
    const char * pArg = getQueryString();
    //detect loop
    int i;
    for( i = 0; i < m_iRedirects; ++i )
    {
        if (( getURILen() == m_urls[i].keyLen)
                &&( strncmp( pURI, m_reqBuf.getPointer( m_urls[i].keyOff ),
                             m_urls[i].keyLen) == 0 )
                &&( getQueryStringLen() == m_urls[i].valLen )
                &&( strncmp( pArg, m_reqBuf.getPointer( m_urls[i].valOff ),
                             m_urls[i].valLen) == 0 ))
            break;
    }
    if ( i < m_iRedirects ) //loop detected
    {
        m_reqBuf.pop_end( m_reqBuf.size() - m_curURL.keyOff );
        m_curURL = m_urls[ --m_iRedirects ];
        LOG_ERR(( getLogger(), "[%s] detect loop redirection.", getLogId() ));
        //FIXME:
        // print the redirect url stack.
        return SC_500;
    }
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] redirect to: \n\tURI=[%s],\n"
                "\tQueryString=[%s]", getLogId(), getURI(),
                getQueryString() ));
    if ( m_pReqBodyBuf )
        m_pReqBodyBuf->rewindReadBuf();

    return 0;
}

int HttpReq::redirect( const char * pURL, int len, int alloc )
{
    //int method = getMethod();
    if (//( HttpMethod::HTTP_PUT < method )||
        (*pURL != '/'))
    {
        if ( pURL != getLocation() )
            setLocation( pURL, len );
        //Do not remove this line, otherwise, external custom error page breaks
        setStatusCode( SC_302 );
        return SC_302;
    }
    return internalRedirect( pURL, len, alloc );
}

int HttpReq::contextRedirect(const HttpContext *pContext, int destLen )
{
    int code = pContext->redirectCode();
    if (( code >= SC_400 )||(( code != -1)&&( code < SC_300 )))
        return code;
    char * pDestURI;
    if ( destLen )
    {
        pDestURI = HttpGlobals::g_achBuf;
    }
    else
    {
        int contextURILen = pContext->getURILen();
        int uriLen = getURILen();
        pDestURI = (char *)pContext->getLocation();
        destLen = pContext->getLocationLen();
        if ( contextURILen < uriLen )
        {
            char * p = HttpGlobals::g_achBuf;
            memmove( p, pDestURI, destLen );
            memmove( p + destLen, getURI() + contextURILen,
                     uriLen - contextURILen + 1 );
            pDestURI = p;
            destLen += uriLen - contextURILen;
        }
    }

    if (( code != -1 )||( *pDestURI != '/'))
    {
        if ( code == -1 )
            code = SC_302;
        int qslen = getQueryStringLen();
        if ( qslen > 0 )
        {
            memmove( pDestURI + destLen + 1, getQueryString(), qslen );
            *(pDestURI + destLen++) = '?';
            destLen += qslen;
        }
        setLocation( pDestURI, destLen );
        return code;
    }
    else
    {
        return internalRedirect( pDestURI, destLen );
    }
}

static const char * findSuffix( const char * pBegin, const char * pEnd )
{
    register const char * pCur = pEnd;
    if ( pBegin < pEnd - 16 )
        pBegin = pEnd - 16;
    while( pCur > pBegin )
    {
        register char ch = *( pCur - 1 );
        if (ch == '/' )
            break;
        if (ch == '.' )
        {
            return pCur;
        }
        --pCur;
    }
    return NULL;
}



int HttpReq::processMatchList( const HttpContext * pContext, const char * pURI, int iURILen )
{
    m_iMatchedLen = 2048;
    const HttpContext * pMatchContext = pContext->match( pURI, iURILen,
                                        HttpGlobals::g_achBuf, m_iMatchedLen );
    if ( pMatchContext )
    {
        m_pContext = pMatchContext;
        m_pHttpHandler = m_pContext->getHandler();
        if ( pMatchContext->getHandlerType() != HandlerType::HT_REDIRECT )
        {
            char * p = strchr( HttpGlobals::g_achBuf, '?' );
            if ( p )
            {
                *p = 0;
                int newLen = p - HttpGlobals::g_achBuf;
                ++p;
                m_curURL.valLen = m_iMatchedLen - newLen;
                m_curURL.valOff = -dupString( p,  m_curURL.valLen );
                m_iMatchedLen = newLen;
            }
        }
        if ( m_iMatchedLen == 0 )
        {
            m_iMatchedLen = iURILen;
            memmove( HttpGlobals::g_achBuf, pURI, iURILen + 1 );
        }
    }
    else
        m_iMatchedLen = 0;
    return m_iMatchedLen;
}

int HttpReq::checkSuffixHandler( const char * pURI, int len, int &cacheable )
{
    if ( !m_iMatchedLen )
    {
        const char * pURIEnd = pURI + len - 1;
        int ret = processSuffix( pURI, pURIEnd, cacheable );
        if ( ret )
            return ret;
    }
    if ( m_pHttpHandler->getHandlerType() == HandlerType::HT_PROXY )
        return -2;
    if ( D_ENABLED( DL_LESS ))
        LOG_D(( getLogger(), "[%s] Cannot found appropriate handler for [%s]",
                getLogId(), pURI ));
    return SC_404;
}



int HttpReq::processSuffix( const char * pURI, const char * pURIEnd, int &cacheable )
{
    const char * pSuffix = findSuffix( pURI, pURIEnd);
    if ( pSuffix )
    {
        const HotlinkCtrl * pHLC= m_pVHost->getHotlinkCtrl();
        if ( pHLC )
        {
            int ret = checkHotlink( pHLC, pSuffix );
            if ( ret )
            {
                if ( cacheable )
                    cacheable = ret;
                else
                    return ret;
            }
        }
    }
    if ( m_pHttpHandler->getHandlerType() == HandlerType::HT_NULL )
    {
        return processMime( pSuffix );
    }
    return 0;
}

int HttpReq::processMime( const char * pSuffix )
{
    if ( !m_pMimeType )
        m_pMimeType = m_pContext->determineMime( pSuffix, (char *)m_pForcedType );
    m_pHttpHandler = m_pMimeType->getHandler();
    if ( !m_pHttpHandler )
    {
        LOG_INFO(( getLogger(), "[%s] Missing handler for suffix [.%s], access denied.",
                   getLogId(), pSuffix?pSuffix:"" ));
        return SC_403;
    }
    if ( m_pHttpHandler->getHandlerType() != HandlerType::HT_NULL )
    {
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] Find handler [%s] for [.%s]",
                    getLogId(), m_pHttpHandler->getName(), pSuffix?pSuffix:"" ));
        if ( !m_pContext->isScriptEnabled() )
        {
            LOG_INFO(( getLogger(), "[%s] Scripting is disabled for "
                       "VHost [%s], context [%s] access denied.", getLogId(),
                       m_pVHost->getName(), m_pContext->getURI() ));
            return SC_403;
        }
    }
    else
    {
        if ( m_iPathInfoLen > 0 ) //Path info is not allowed for static file
        {
            LOG_INFO(( getLogger(), "[%s] URI '%s' refers to a static file with PATH_INFO [%s].",
                       getLogId(), getURI(), getPathInfo() ));
            return SC_404;
        }
        if (( m_pMimeType->getMIME()->len() > 20 )&&
                ( strncmp( "x-httpd-", m_pMimeType->getMIME()->c_str() + 12, 8 ) == 0 ))
        {
            LOG_ERR(( getLogger(), "[%s] MIME type [%s] for suffix '.%s' does not "
                      "allow serving as static file, access denied!",
                      getLogId(), m_pMimeType->getMIME(), pSuffix?pSuffix:"" ));
            dumpHeader();
            return SC_403;
        }
    }
    return 0;
}

int HttpReq::locationToUrl( const char * pLocation, int len )
{
    char achURI[8192];
    const HttpContext * pNewCtx;
    pNewCtx = m_pVHost->matchLocation( pLocation );
    if ( pNewCtx )
    {
        int n = snprintf( achURI, 8192, "%s%s", pNewCtx->getURI(),
                        pLocation + pNewCtx->getLocationLen() );
        pLocation = achURI;
        len = n;
        if ( D_ENABLED( DL_MORE ))
             LOG_D(( getLogger(), "[%s] File [%s] has been mapped to URI: [%s] , "
                                  " via context: [%s]", getLogId(), pLocation,
                                  achURI, pNewCtx->getURI() ));
    }
    setLocation( pLocation, len );
    return 0;
}


int HttpReq::postRewriteProcess( const char * pURI, int len )
{
    //if is rewriten to file path, reverse lookup context from file path
    const HttpContext * pNewCtx;
    pNewCtx = m_pVHost->matchLocation( pURI );
    if ( pNewCtx )
    {
        // should skip context processing;
        m_iContextState &= ~PROCESS_CONTEXT;
        m_pContext = pNewCtx;
        m_pHttpHandler = pNewCtx->getHandler();
        m_pMimeType = NULL;
        // if new context is same as previous one skip authentication
        if ( pNewCtx != m_pContext )
        {
            m_iContextState &= ~CONTEXT_AUTH_CHECKED;
            m_pContext = pNewCtx;
        }
        m_iMatchedLen = len;
        memmove( HttpGlobals::g_achBuf, pURI, len + 1 );
    }
    else
    {
        setRewriteURI( pURI, len );
        if ( m_pContext && len > m_pContext->getURILen() )
        {
            const HttpContext * pContext = m_pVHost->bestMatch( pURI );
            if ( pContext != m_pContext )
            {
                m_pContext = pContext;
                return 1;
            }
        }
        if ( m_pContext && (strncmp( m_pContext->getURI(), pURI,
                                        m_pContext->getURILen() ) != 0) )
        {
            if ( m_pContext->shouldMatchFiles() )
                filesMatch( getURI() + getURILen() );
            return 1;
        }
    }
    return 0;
}


int HttpReq::processContext()
{
    const char * pURI;
    int   iURILen;
    const HttpContext * pOldCtx = m_pContext;
    pURI = getURI();
    iURILen = getURILen();
    m_pMimeType = NULL;
    m_iMatchedLen = 0;

    if ( m_pVHost->getRootContext().getMatchList() )
    {
        processMatchList( &m_pVHost->getRootContext(), pURI, iURILen);
    }

    if ( !m_iMatchedLen )
    {
        m_pContext = m_pVHost->bestMatch( pURI );
        if ( !m_pContext )
        {
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] No context found for URI: [%s]",
                        getLogId(), pURI ));
            return SC_404;
        }
        if ( D_ENABLED( DL_MORE ))
            LOG_D(( getLogger(), "[%s] Find context with URI: [%s], "
                    "location: [%s]", getLogId(),
                    m_pContext->getURI(),
                    m_pContext->getLocation()?m_pContext->getLocation():"" ));

        if ((*( m_pContext->getURI() + m_pContext->getURILen() - 1 ) == '/' )&&
                (*( pURI + m_pContext->getURILen() - 1 ) != '/' ))
            return redirectDir( pURI );
        m_pHttpHandler = m_pContext->getHandler();

        if ( m_pContext->getMatchList() ) //regular expression match
        {
            processMatchList(m_pContext, pURI, iURILen);
        }
    }
    if ( m_pHttpHandler->getHandlerType() == HandlerType::HT_PROXY )
        return -2;
    if ( m_pContext != pOldCtx )
        m_iContextState &= ~CONTEXT_AUTH_CHECKED;
    return 0;
}

int HttpReq::processContextPath()
{
    int ret;
    const char * pURI;
    int   iURILen;
    pURI = getURI();
    iURILen = getURILen();
    if ( m_pContext->getHandlerType() == HandlerType::HT_REDIRECT )
    {
        ret = contextRedirect( m_pContext, m_iMatchedLen );
        if ( ret )
            return ret;
        return -1;
    }

    if ( m_iMatchedLen )
    {
        return processMatchedURI( pURI, iURILen, HttpGlobals::g_achBuf, m_iMatchedLen );
    }
    
    int cacheable = 0;
    ret = processURIEx( pURI, iURILen, cacheable );


    if ( cacheable > 1 )       //hotlinking status
        return cacheable;
    return ret;
}

void HttpReq::saveMatchedResult()
{
    if ( m_iMatchedLen )
    {
        m_iMatchedOff = dupString( HttpGlobals::g_achBuf, m_iMatchedLen );
    }
}
void HttpReq::restoreMatchedResult()
{
    if ( m_iMatchedLen )
    {
        memmove( HttpGlobals::g_achBuf, m_reqBuf.getPointer( m_iMatchedOff ),
                 m_iMatchedLen + 1 );
    }
}

char * HttpReq::allocateAuthUser()
{
    if ( m_reqBuf.available() < AUTH_USER_SIZE )
    {
        if ( m_reqBuf.grow( AUTH_USER_SIZE ) == -1 )
            return NULL;
    }
    m_iAuthUserOff = m_reqBuf.size();
    m_reqBuf.used( AUTH_USER_SIZE );
    return m_reqBuf.begin() + m_iAuthUserOff;

}


int HttpReq::redirectDir( const char * pURI )
{
    char newLoc[MAX_URL_LEN];
    char * p = newLoc;
    char * pEnd = newLoc + sizeof( newLoc );
    p += HttpUtil::escape( pURI, p, MAX_URL_LEN - 4 );
    *p++ = '/';
    const char * pQueryString = getQueryString();
    if ( *pQueryString )
    {
        int len = getQueryStringLen();
        *p++ = '?';
        if ( len > pEnd - p )
            len = pEnd - p;
        memmove( p, pQueryString, len );
        p+= len;
    }
    setLocation( newLoc, p - newLoc );
    return SC_301;
}

int HttpReq::processMatchedURI( const char * pURI, int uriLen, char * pMatched, int len )
{
    if ( !m_pContext->allowBrowse())
    {
        LOG_INFO(( getLogger(), "[%s] Context [%s] is not accessible: access denied.",
                   getLogId(), m_pContext->getURI() ));
        return SC_403; //access denied
    }
    if ( m_pHttpHandler->getHandlerType() >= HandlerType::HT_FASTCGI )
    {
        m_pRealPath = NULL;
        return 0;
    }
    char * pBegin = pMatched + 1;
    char * pEnd = pMatched + len;
    int cacheable = 0;
    return processPath( pURI, uriLen, pMatched, pBegin, pEnd, cacheable );

}

int HttpReq::processURIEx( const char * pURI, int uriLen, int &cacheable )
{
    if ( m_pContext )
    {
        if ( !m_pContext->allowBrowse())
        {
            LOG_INFO(( getLogger(), "[%s] Context [%s] is not accessible: "
                       "access denied.",
                       getLogId(), m_pContext->getURI() ));
            return SC_403; //access denied
        }
        if ( m_pContext->getHandlerType() >= HandlerType::HT_FASTCGI )
        {
            int contextLen = m_pContext->getURILen();
            cacheable = ( contextLen >= uriLen );
            m_iScriptNameLen = contextLen;
            if ((contextLen != uriLen)
                    &&( *( m_pContext->getURI() + contextLen - 1 ) == '/' ))
                --contextLen;
            if ( contextLen < uriLen )
            {
                m_iPathInfoLen = uriLen - contextLen;
                m_iPathInfoOff = dupString( pURI + contextLen, m_iPathInfoLen );
            }
            m_pRealPath = NULL;
            return 0;
        }
    }
    char * pBuf = HttpGlobals::g_achBuf;
    int pathLen = translate( pURI, uriLen, m_pContext,
                             pBuf, G_BUF_SIZE - 1 );
    if ( pathLen == -1 )
    {
        LOG_ERR(( getLogger(), "[%s] translate() failed,"
                  " translation buffer is too small.", getLogId()  ));
        return SC_500;
    }
    char * pEnd = pBuf + pathLen;
    char * pBegin = pEnd - uriLen;
    if ( m_pContext )
    {
        pBegin += m_pContext->getURILen();
        if ( *(pBegin-1) == '/' )
            --pBegin;
    }
    return processPath( pURI, uriLen, pBuf, pBegin, pEnd, cacheable );
}





int HttpReq::processPath( const char * pURI, int uriLen, char * pBuf,
                          char * pBegin, char * pEnd,  int &cacheable )
{
    int ret;
    char * p;
    ret = SC_404;
    //find the first valid file or directory
    p = pEnd;
    do
    {
        while(( p > pBegin )&&( *(p - 1) == '/' ))
            --p;
        *p = 0;
        ret = nio_stat( pBuf, &m_fileStat );

        if ( p!= pEnd )
            *p = '/';
        if ( ret != -1 )
        {
            if (( S_ISDIR( m_fileStat.st_mode)))
            {
                if ( p == pEnd )
                    return redirectDir( pURI );
                else
                {
                    if ( ++p != pEnd )
                    {
                        if ( D_ENABLED( DL_LESS ))
                            LOG_D(( getLogger(), "[%s] File not found [%s] ",
                                    getLogId(), pBuf ));
                        return SC_404;
                    }
                }
            }
            break;
        }
        while(( --p > pBegin )&&( *p != '/' ))
            ;
    } while( p >= pBegin );
    if (ret == -1)
    {
        //if ( strcmp( p, "/favicon.ico" ) != 0 )
        if ( D_ENABLED( DL_LESS ))
            LOG_D(( getLogger(), "[%s] File not found [%s]",
                    getLogId(), pBuf ));
        return checkSuffixHandler( pURI, uriLen, cacheable );
    }
    if ( p != pEnd )
    {
        // set PATH_INFO and SCRIPT_NAME
        m_iPathInfoLen = (pEnd - p);
        int orgURILen = getOrgURILen();
        if ( m_iPathInfoLen < orgURILen )
        {
            if ( strncmp( getOrgURI() + orgURILen - m_iPathInfoLen, p, m_iPathInfoLen ) == 0 )
            {
                m_iScriptNameLen = uriLen - m_iPathInfoLen;
            }
        }
        m_iPathInfoOff = dupString( p, m_iPathInfoLen );
        *p = 0;
    }
    else
        cacheable = 1;
    // if it is a directory then try to append index file
    if ( S_ISDIR( m_fileStat.st_mode))
    {
        if ( p != pEnd )
        {
            return checkSuffixHandler( pURI, uriLen, cacheable );
        }
        //FIXME: why need to check this?
        //if (m_pHttpHandler->getHandlerType() != HandlerType::HT_NULL )
        //    return SC_404;
        int l = pBuf + G_BUF_SIZE - p;
        const StringList * pIndexList = m_pContext->getIndexFileList();
        if ( pIndexList )
        {
            StringList::const_iterator iter;
            StringList::const_iterator iterEnd = pIndexList->end();
            for( iter = pIndexList->begin();
                    iter < iterEnd; ++iter )
            {
                int n = (*iter)->len() + 1;
                if ( n > l )
                    continue;
                memcpy( p, (*iter)->c_str(), n );
                if ( nio_stat( pBuf, &m_fileStat ) != -1 )
                {
                    p += n - 1;
                    l = 0;
                    appendIndexToUri( (*iter)->c_str(), n - 1 );
                    break;
                }
            }
        }
        cacheable = 0;
        if ( l )
        {
            *p = 0;
            if ( m_pContext->isAutoIndexOn() )
            {
                addEnv( "LS_AI_PATH", 10, pBuf, p - pBuf );
                const AutoStr2 * pURI = m_pVHost->getAutoIndexURI();
                ret = internalRedirectURI( pURI->c_str(), pURI->len() );
                if ( ret )
                    return ret;
                return -1;  //internal redirect
            }
            if ( D_ENABLED( DL_LESS ))
                LOG_D(( getLogger(), "[%s] Index file is not available in [%s]",
                        getLogId(), pBuf ));
            return SC_404; //can't find a index file
        }
    }
    *p = 0;
    {
        int follow = m_pVHost->followSymLink();
        if (( follow != LS_ALWAYS_FOLLOW )||
                ( HttpServerConfig::getInstance().checkDeniedSymLink() ))
        {
            ret = checkSymLink( follow, pBuf, p, pBegin );
            if ( ret )
                return ret;
        }

        int forcedType = 0;
        if ( m_pContext->shouldMatchFiles() )
            forcedType = filesMatch( p );
        if ((!forcedType)&&
                ( m_pHttpHandler->getHandlerType() == HandlerType::HT_NULL ))
        {
            //m_pMimeType = NULL;
            ret = processSuffix( pBuf, p, cacheable );
            if ( ret )
                return ret;
        }

    }
    m_sRealPathStore.setStr(pBuf, p - pBuf );
    m_pRealPath = &m_sRealPathStore;
    return ret;
}

int HttpReq::filesMatch( const char * pEnd )
{
    int len;
    const char * p = pEnd;

    while( *(p - 1 ) != '/' )
        --p;
    len = pEnd - p;
    if ( len )
    {
        m_pFMContext = m_pContext->matchFilesContext( p, len );
        if (( m_pFMContext )&&( m_pFMContext->getConfigBits()& BIT_FORCE_TYPE ))
        {
            if ( m_pFMContext->getForceType() )
            {
                m_pMimeType = m_pFMContext->getForceType();
                m_pHttpHandler = m_pMimeType->getHandler();
                return 1;
            }
        }
    }
    return 0;
}

int HttpReq::checkSymLink( int follow, char * pBuf, char * &p, char * pBegin )
{
    int rpathLen = GPath::checkSymLinks( pBuf, p, pBuf + MAX_BUF_SIZE, pBegin, follow );
    if ( rpathLen > 0 )
    {
        if ( HttpServerConfig::getInstance().checkDeniedSymLink() )
        {
            if ( HttpGlobals::getDeniedDir()->isDenied( pBuf ) )
            {
                LOG_INFO(( getLogger(), "[%s] Path [%s] is in access denied list, access denied",
                           getLogId(), pBuf ));
                return SC_403;
            }
        }
        p = pBuf + rpathLen;
        return 0;
    }
    else
    {
        LOG_INFO(( getLogger(), "[%s] Found symbolic link, or owner of symbolic link and link "
                   "target does not match for path [%s], access denied.",
                   getLogId(), pBuf ));
        return SC_403;
    }

}


int HttpReq::checkPathInfo( const char * pURI, int iURILen, int &pathLen,
                            short &scriptLen, short &pathInfoLen,
                            const HttpContext * pContext )
{
    if ( !pContext )
    {
        pContext = m_pVHost->bestMatch( pURI );
        if ( !pContext )
            pContext = &m_pVHost->getRootContext();
    }
    int contextLen = pContext->getURILen();
    char * pBuf = HttpGlobals::g_achBuf;
    if ( pContext->getHandlerType() >= HandlerType::HT_FASTCGI )
    {
        scriptLen = contextLen;
        if ( *( pContext->getURI() + contextLen - 1 ) == '/' )
            --contextLen;
        if ( contextLen < iURILen )
        {
            pathInfoLen = iURILen - contextLen;
        }
        memccpy( pBuf, getDocRoot()->c_str(), 0, G_BUF_SIZE - 1 );
        pathLen = getDocRoot()->len();
        memccpy( pBuf + pathLen, pURI, 0, G_BUF_SIZE - pathLen );
        pathLen += contextLen;
        return 0;
    }
    if ( m_iMatchedLen > 0 )
        pathLen = m_iMatchedLen;
    else
        pathLen = translate( pURI, iURILen, pContext,
                    pBuf, G_BUF_SIZE - 1 );
    if ( pathLen == -1 )
    {
        return -1;
    }
    char * pEnd = pBuf + pathLen;
    char * pBegin = pEnd - iURILen;
    if ( *pURI == '/' )
        pBegin += pContext->getURILen() - 1;
    //find the first valid file or directory
    char * p = NULL;
    char * p1 = pEnd;
    char * pTest;
    int ret;
    
    do
    {
        pTest = p1;
        while(( pTest > pBegin )&&( *(pTest - 1) == '/' ))
            --pTest;
        *pTest = 0;
        ret = nio_stat( pBuf, &m_fileStat );

        if ( pTest != pEnd )
            *pTest = '/';
        if ( ret != -1 )
        {
            if ( S_ISREG( m_fileStat.st_mode ) )
                p = pTest;
            break;
        }
        p1 = p = pTest;
        while(( --p1 > pBegin )&&( *p1 != '/' ))
            ;
    }while( p1 > pBegin );
    if ( !p )
        p = pTest;
    // set PATH_INFO and SCRIPT_NAME
    pathInfoLen = (pEnd - p);
    int orgURILen = getOrgURILen();
    if ( pathInfoLen < orgURILen )
    {
        if ( strncmp( getOrgURI() + orgURILen - pathInfoLen, p, pathInfoLen ) == 0 )
        {
            scriptLen = getURILen() - pathInfoLen;
        }
    }
    pathLen = p - pBuf;
    return 0;
}


int HttpReq::addWWWAuthHeader( HttpRespHeaders &buf ) const
{
    if ( m_pHTAuth )
        return m_pHTAuth->addWWWAuthHeader( buf );
    else
        return 0;
}



int HttpReq::setLocation( const char * pLoc, int len )
{
    assert( pLoc );
    m_iLocationOff = dupString( pLoc, len );
    m_iLocationLen = len;
    return m_iLocationOff;
}

int HttpReq::dupString( const char * pString )
{
    int ret = m_reqBuf.size();
    if ( m_reqBuf.append( pString, strlen( pString ) + 1 ) == -1)
        return 0;
    return ret;
}

int HttpReq::dupString( const char * pString, int len )
{
    int ret = m_reqBuf.size();
    if ( m_reqBuf.append( pString, len + 1 ) == -1)
        return 0;
    *( m_reqBuf.end() - 1) = 0;
    return ret;
}


#define HTTP_PAGE_SIZE 4096
#include <util/blockbuf.h>
int HttpReq::prepareReqBodyBuf()
{
    if (( m_lEntityLength != CHUNKED )&&
            ( m_lEntityLength <= m_headerBuf.capacity() - m_iReqHeaderBufFinished ))
    {
        m_pReqBodyBuf = new VMemBuf();
        if ( !m_pReqBodyBuf )
            return SC_500;
        BlockBuf * pBlock = new BlockBuf( m_headerBuf.begin() +
                                          m_iReqHeaderBufFinished, m_lEntityLength );
        if ( !pBlock )
        {
            return SC_500;
        }
        m_pReqBodyBuf->set( pBlock );
    }
    else
    {
        m_pReqBodyBuf = HttpGlobals::getResManager()->getVMemBuf();
        if ( !m_pReqBodyBuf || m_pReqBodyBuf->reinit( m_lEntityLength ))
        {
            LOG_ERR(( getLogger(), "[%s] Failed to obtain or reinitialize VMemBuf.", getLogId() ));
            return SC_500;
        }
    }
    return 0;
}

void HttpReq::processReqBodyInReqHeaderBuf()
{
    int already = m_headerBuf.size() - m_iReqHeaderBufFinished;
    if ( !already )
        return;
    if ( already > m_lEntityLength )
        already = m_lEntityLength;
    if ( m_lEntityLength > m_headerBuf.capacity() - m_iReqHeaderBufFinished )
    {
        size_t size;
        char * pBuf = getBodyBuf()->getWriteBuffer( size );
        assert( pBuf );
        assert( (int)size >= already );
        memmove( pBuf, m_headerBuf.begin() + m_iReqHeaderBufFinished, already );
    }
    getBodyBuf()->writeUsed( already );
    m_lEntityFinished = already;
    m_iReqHeaderBufFinished += already;
}

void HttpReq::tranEncodeToContentLen()
{
    int off = m_commonHeaderOffset[HttpHeader::H_TRANSFER_ENCODING];
    if ( !off )
        return;
    int len = 8;
    char * pBegin = m_headerBuf.begin() + off - 1;
    while( *pBegin != ':' )
    {
        ++len;
        --pBegin;
    }
    if ( (pBegin[-1] | 0x20) != 'g' )
        return;
    pBegin -= 17;
    len += 17;
    int n = safe_snprintf( pBegin, len, "Content-length: %ld", m_lEntityFinished );
    memset( pBegin + n, ' ', len - n );
    m_commonHeaderOffset[HttpHeader::H_CONTENT_LENGTH] =
        pBegin + 16 - m_headerBuf.begin();
    m_commonHeaderLen[ HttpHeader::H_CONTENT_LENGTH] = n - 16;
}

const AutoStr2 * HttpReq::getDocRoot() const
{
    return m_pVHost->getDocRoot();
}

const ExpiresCtrl * HttpReq::shouldAddExpires()
{
    const ExpiresCtrl * p;
    if ( m_pContext )
    {
        p = &m_pContext->getExpires();
    }
    else if ( m_pVHost )
    {
        p = &m_pVHost->getExpires();
    }
    else
    {
        p = HttpGlobals::getMime()->getDefault()->getExpires();
    }
    if ( p->isEnabled() )
    {
        if ( m_pMimeType->getExpires()->getBase() )
            p = m_pMimeType->getExpires();
        else if ( !p->getBase() )
            p = NULL;
    }
    else
        p = NULL;
    return p;
}


int HttpReq::checkHotlink( const HotlinkCtrl * pHLC, const char * pSuffix )
{
    if (( !pHLC )||
            ( pHLC->getSuffixList()->find( pSuffix ) == NULL ))
    {
        return 0;
    }
    int ret = 0;
    char chNULL = 0;
    char * pRef = (char *)getHeader( HttpHeader::H_REFERER );
    if ( *pRef )
    {
        char * p = (char *)memchr( pRef + 4, ':', 2 );
        if ( !p )
            pRef = &chNULL;
        else
        {
            if (( *(++p) != '/' )||( *(++p) != '/' ))
                pRef = &chNULL;
            else
                pRef = p + 1;
        }
    }
    if ( !*pRef )
    {
        if ( !pHLC->allowDirectAccess() )
        {
            LOG_INFO(( "[%s] [HOTLINK] Direct access detected, access denied.",
                       getLogId() ));
            ret = SC_403;
        }
    }
    else
    {
        int len = strcspn( pRef, "/:" );
        char ch = *(pRef + len);
        *(pRef + len) = 0;
        if ( pHLC->onlySelf() )
        {
            const char * pHost = getHeader( HttpHeader::H_HOST );
            if ( memcmp( pHost, pRef, getHeaderLen( HttpHeader::H_HOST ) ) != 0 )
            {
                LOG_INFO(( "[%s] [HOTLINK] Reference from other web site,"
                           " access denied, referrer: [%s]", getLogId(), pRef  ));
                ret = SC_403;
            }
        }
        else
        {
            if ( !pHLC->allowed( pRef, len ) )
            {
                LOG_INFO(( "[%s] [HOTLINK] Disallowed referrer,"
                           " access denied, referrer: [%s]", getLogId(), pRef ));
                ret = SC_403;
            }
        }
        *(pRef + len) = ch;
    }
    if ( ret )
    {
        const char * pRedirect = pHLC->getRedirect();
        if ( pRedirect )
        {
            if ( strcmp( pRedirect, getURI() ) == 0 ) //redirect to the same url
                return 0;
            setLocation( pRedirect, pHLC->getRedirectLen() );
            return SC_302;
        }
    }
    return ret;
}

void HttpReq::dumpHeader()
{
    const char * pBegin = getOrgReqLine();
    char * pEnd = (char *)pBegin + getOrgReqLineLen();
    if ( m_iHeaderStatus == HEADER_REQUEST_LINE ) 
        return;
    if ( pEnd >= m_headerBuf.begin() + m_iReqHeaderBufFinished )
    {
        pEnd = m_headerBuf.begin() + m_iReqHeaderBufFinished;
        if ( pEnd >= m_headerBuf.begin() + m_headerBuf.capacity() )
            pEnd = m_headerBuf.begin() + m_headerBuf.capacity() - 1;
    }
    if ( pEnd < pBegin )
        pEnd = (char *)pBegin;
    *pEnd = 0;
    LOG_INFO(( getLogger(), "[%s] Content len: %d, Request line: \n%s", getLogId(),
               m_lEntityLength, pBegin ));
    if ( m_iRedirects > 0 )
    {
        LOG_INFO(( getLogger(), "[%s] Redirect: #%d, URL: %s", getLogId(),
                   m_iRedirects, getURI() ));
    }
}

int HttpReq::getUGidChroot( uid_t *pUid, gid_t *pGid,
                            const AutoStr2 **pChroot )
{
    if ( m_fileStat.st_mode & (S_ISUID|S_ISGID) )
    {
        if ( !m_pContext || !m_pContext->allowSetUID())
        {
            LOG_INFO(( getLogger(), "[%s] Context [%s] set UID/GID mode is "
                       "not allowed, access denied.", getLogId(),
                       m_pContext->getURI() ));
            return SC_403;
        }
    }
    char chMode = UID_FILE;
    if (  m_pContext )
        chMode = m_pContext->getSetUidMode();
    switch( chMode )
    {
    case UID_FILE:
        *pUid = m_fileStat.st_uid;
        *pGid = m_fileStat.st_gid;
        break;
    case UID_DOCROOT:
        if ( m_pVHost )
        {
            *pUid = m_pVHost->getUid();
            *pGid = m_pVHost->getGid();
            break;
        }
    case UID_SERVER:
    default:
        *pUid = HttpGlobals::s_uid;
        *pGid = HttpGlobals::s_gid;
    }
    if ( HttpGlobals::s_ForceGid )
    {
        *pGid = HttpGlobals::s_ForceGid;
    }
    if (( *pUid < HttpGlobals::s_uidMin )||
            ( *pGid < HttpGlobals::s_gidMin ))
    {
        LOG_INFO(( getLogger(), "[%s] CGI's UID or GID is smaller "
                   "than the minimum UID, GID configured, access denied.",
                   getLogId() ));
        return SC_403;
    }
    *pChroot = NULL;
    if (  m_pVHost )
    {
        chMode = m_pContext->getChrootMode();
        switch( chMode )
        {
        case CHROOT_VHROOT:
            *pChroot = m_pVHost->getVhRoot();
            break;
        case CHROOT_PATH:
            *pChroot = m_pVHost->getChroot();
            if ( !(*pChroot)->c_str() )
                *pChroot = NULL;
        }
    }
    if ( !*pChroot )
    {
        *pChroot = HttpGlobals::s_psChroot;
    }
    return 0;

}

const AutoStr2 * HttpReq::getDefaultCharset() const
{
    if (m_pContext )
        return m_pContext->getDefaultCharset();
    return NULL;
}

void  HttpReq::smartKeepAlive( const char * pValue )
{
    if ( m_pVHost->getSmartKA() )
        if ( !HttpMime::shouldKeepAlive( pValue ) )
            m_iKeepAlive = 0;
}

void HttpReq::addEnv( const char * pKey, int keyLen, const char * pValue, int valLen )
{
    key_value_pair * pIdx = getValueByKey( m_reqBuf, m_envIdxOff, pKey, keyLen );
    int n;
    int keyOff, valOff;
    if ( pIdx )
    {
        if ( valLen == pIdx->valLen )
        {
            if ( strncmp( pValue, m_reqBuf.getPointer( pIdx->valOff ), valLen ) == 0 )
                return;
        }
        keyOff = pIdx->valOff;
        n = pIdx - getOffset( m_envIdxOff, 0 );
    }
    else
    {
        pIdx = newKeyValueBuf( m_envIdxOff );
        n = getListSize( m_envIdxOff ) - 1;
        keyOff = dupString( pKey, keyLen );
    }
    valOff = dupString( pValue, valLen );
    pIdx = getOffset( m_envIdxOff, n );
    pIdx->keyOff = keyOff;
    pIdx->keyLen = keyLen;
    pIdx->valOff = valOff;
    pIdx->valLen = valLen;
}

const char * HttpReq::getEnv( const char * pKey, int keyLen, int &valLen )
{
    key_value_pair * pIdx = getValueByKey( m_reqBuf, m_envIdxOff, pKey, keyLen );
    if ( pIdx )
    {
        valLen = pIdx->valLen;
        return m_reqBuf.getPointer( pIdx->valOff );
    }
    return NULL;
}

const char * HttpReq::getEnvByIndex( int idx, int &keyLen,
                                     const char * &pValue, int &valLen  )
{
    key_value_pair * pIdx = getOffset( m_envIdxOff, idx );
    if ( pIdx )
    {
        pValue = m_reqBuf.getPointer( pIdx->valOff );
        valLen = pIdx->valLen;
        keyLen = pIdx->keyLen;
        return m_reqBuf.getPointer( pIdx->keyOff );
    }
    return NULL;
}


const char * HttpReq::getUnknownHeaderByIndex( int idx, int &keyLen,
        const char * &pValue, int &valLen  )
{
    key_value_pair * pIdx = getOffset( m_headerIdxOff, idx );
    if ( pIdx )
    {
        pValue = m_headerBuf.getPointer( pIdx->valOff );
        valLen = pIdx->valLen;
        keyLen = pIdx->keyLen;
        return m_headerBuf.getPointer( pIdx->keyOff );
    }
    return NULL;
}


char HttpReq::getRewriteLogLevel() const
{
    return m_pVHost->getRewriteLogLevel();
}

StaticFileCacheData *& HttpReq::setStaticFileCache()
{
    return m_dataStaticFile.getCache();
}

#include <util/iovec.h>
void HttpReq::appendHeaderIndexes( IOVec * pIOV, int cntUnknown )
{
    pIOV->append( &m_commonHeaderLen, (char *)&m_headerIdxOff - (char *)&m_commonHeaderLen );
    if ( cntUnknown > 0 )
    {
        key_value_pair * pIdx = getOffset( m_headerIdxOff, 0 );
        pIOV->append( pIdx, sizeof( key_value_pair ) * cntUnknown );
    }
}

const AutoBuf * HttpReq::getExtraHeaders() const
{
    return (m_pContext)?m_pContext->getExtraHeaders():NULL;
}

char HttpReq::isGeoIpOn() const
{
    return (m_pContext)?m_pContext->isGeoIpOn():0;
}

const char * HttpReq::encodeReqLine( int &len )
{
    const char * pURI = getURI();
    int uriLen = getURILen();
    int start = m_reqBuf.size();
    int maxLen = uriLen * 3 + 16 + getQueryStringLen();
    if ( m_reqBuf.available() < maxLen )
        if ( m_reqBuf.reserve( start + maxLen + 10 ) == -1 )
            return NULL;
    char * p = m_reqBuf.getPointer( start );
    memmove( p, HttpMethod::get( m_method ), HttpMethod::getLen( m_method ));
    p += HttpMethod::getLen(m_method);
    *p++ = ' ';
    //p = escape_uri( p, pURI, uriLen );
    memmove( p, pURI, uriLen );
    p += uriLen;
    if ( getQueryStringLen() > 0 )
    {
        *p++ = '?';
        memmove( p, getQueryString(), getQueryStringLen() );
        p+= getQueryStringLen();
    }
    len = p - m_reqBuf.end();
    return m_reqBuf.end();
}


int HttpReq::getDecodedOrgReqURI( char * &pValue )
{
    if ( m_iRedirects > 0 )
    {
        pValue = m_reqBuf.getPointer( m_urls[0].keyOff );
        return m_urls[0].keyLen;
    }
    else
    {
        pValue = m_reqBuf.getPointer( m_curURL.keyOff );
        return m_curURL.keyLen;
    }
}
SSIConfig * HttpReq::getSSIConfig()
{
    if ( m_pContext )
        return m_pContext->getSSIConfig();
    return NULL;
}

int HttpReq::isXbitHackFull() const
{   return (m_pContext)? (m_pContext->isXbitHackFull() &&
                     ( S_IXGRP & m_fileStat.st_mode ) ): 0 ;    }
int HttpReq::isIncludesNoExec() const
{
    return (m_pContext)? m_pContext->isIncludesNoExec() : 1 ;
}                     
#include <ssi/ssiruntime.h>
void HttpReq::backupPathInfo()
{
    m_pSSIRuntime->savePathInfo( m_iPathInfoOff, m_iPathInfoLen, m_iRedirects );

}

void HttpReq::restorePathInfo()
{
    int oldRedirects = m_iRedirects;
    m_pSSIRuntime->restorePathInfo( m_iPathInfoOff, m_iPathInfoLen, m_iRedirects );
    if ( m_iRedirects < oldRedirects )
    {
        m_curURL =  m_urls[m_iRedirects];
    }
}        
void HttpReq::stripRewriteBase( const HttpContext * pCtx, 
            const char * &pURL, int &iURLLen )
{
    if (( pCtx->getLocationLen() <= m_iMatchedLen )&&
        ( strncmp( HttpGlobals::g_achBuf, pCtx->getLocation(), pCtx->getLocationLen() ) == 0 ))
    {
        pURL = HttpGlobals::g_achBuf + pCtx->getLocationLen();
        iURLLen = m_iMatchedLen - pCtx->getLocationLen();
    }
}
int HttpReq::detectLoopRedirect(const char * pURI, int uriLen,
                const char * pArg, int qsLen, int isSSL )
{
    //detect loop
    int i;
    const char * p = pURI;
    int relativeLen = uriLen;
    if ( strncasecmp( p, "http", 4 ) == 0)
    {
        int isHttps;
        p += 4;
        isHttps = (*p == 's');
        if ( isHttps != isSSL )
            return 0;
        if (isHttps )
        {
            ++p;
        }
        if ( strncmp( p, "://", 3 ) == 0 )
        {
            p += 3;
        }
        const char * pOldHost = getHeader( HttpHeader::H_HOST );
        int oldHostLen = getHeaderLen( HttpHeader::H_HOST );
        if (( *pOldHost )&&
            ( strncasecmp( p, pOldHost, oldHostLen ) != 0 ))
        {
            return 0;
        }
        p += oldHostLen;
        if ( *p != '/' )
            return 0;
        relativeLen -= p - pURI;
    }

    const char * pCurURI = getURI();
    const char * pCurArg = getQueryString();
    int         iCurURILen = getURILen();
    int         iCurQSLen = getQueryStringLen();
    if (( iCurURILen == relativeLen )&&( iCurQSLen == qsLen )
        && (pCurURI != p )&&
        (strncmp( p, pCurURI, iCurURILen ) == 0 )&&
        (strncmp( pArg, pCurArg, iCurQSLen ) == 0 ))
        return 1;
    for( i = 0; i < m_iRedirects; ++i )
    {
        if (((( uriLen == m_urls[i].keyLen)
            &&( strncmp( pURI, m_reqBuf.getPointer( m_urls[i].keyOff ),
                        m_urls[i].keyLen) == 0 ))||
             (( relativeLen != uriLen )&&
              ( relativeLen == m_urls[i].keyLen)&&
              ( strncmp( p, m_reqBuf.getPointer( m_urls[i].keyOff ),
                        m_urls[i].keyLen) == 0 )))
            &&( qsLen == m_urls[i].valLen )
            &&( strncmp( pArg, getQueryString( &m_urls[i] ),
                        m_urls[i].valLen) == 0 ))
            break;
    }
    if ( i < m_iRedirects ) //loop detected
    {
        return 1;
    }
    
    return 0;
}

int HttpReq::getETagFlags() const
{
    int tagMod = 0;
    int etag;
    if (m_pContext )
        etag = m_pContext->getFileEtag();
    else if ( m_pVHost )
        etag = m_pVHost->getRootContext().getFileEtag();
    else 
        etag = HttpGlobals::s_pGlobalVHost->getRootContext().getFileEtag();
    if ( tagMod )
    {
        int mask = ( tagMod & ETAG_MOD_ALL ) >> 3;
        etag = (etag & ~mask ) | (tagMod & mask );
    }
    return etag;
}

int HttpReq::checkScriptPermission()
{
    if ( !m_pRealPath )
        return 1;
    if ( m_fileStat.st_mode & HttpServerConfig::getInstance().getScriptForbiddenBits() )
    {
        LOG_INFO(( getLogger(),
                   "[%s] [%s] file permission is restricted for script.",
                   getLogId(),getVHost()->getName(), m_pRealPath->c_str() ));
        return 0;
    }
    return 1;
}
// int HttpReq::redirect( const char * pURL, int len, int alloc )
// {
//     if (//( HttpMethod::HTTP_PUT < m_method )||
//         (*pURL != '/'))
//     {
//         if ( pURL != getLocation() )
//             setLocation( pURL, len );
//         //Do not remove this line, otherwise, external custom error page breaks
//         setStatusCode( SC_302 );
//         return SC_302;
//     }
//     return internalRedirect( pURL, len, alloc );
// }
// int HttpReq::internalRedirect( const char * pURL, int len, int alloc )
// {
//     assert( pURL );
//     int ret = saveCurURL();
//     if ( ret )
//         return ret;
//     m_iContextState &= ~FP_CONTEXT;
//     setCurrentURL( pURL, len, alloc );
//     return detectLoopRedirect();
// }



