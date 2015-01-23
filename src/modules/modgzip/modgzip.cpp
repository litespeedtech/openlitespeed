/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include <ls.h>
#include <lsr/lsr_loopbuf.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define     MODULE_VERSION      "1.1"
#define     INIT_LOOPBUF_SIZE   8192

#define     COMPRESS_FLAG    "---"
#define     DECOMPRESS_FLAG  "+++"
#define     SENDING_FLAG     "-->"
#define     RECVING_FLAG     "<--"

enum
{
    Z_UNINITED = 0,
    Z_INITED,
    Z_EOF,
    Z_END,
};

typedef struct _ZBufInfo
{
    int8_t          inited;
    uint8_t         compressLevel; //0: Decompress, 1-9:compress
    int16_t         iZState;
    z_stream      * pZStream;
    lsr_loopbuf_t * pLoopbuf;
} ZBufInfo;


typedef struct _ModgzipMData
{
    ZBufInfo    recvZBufInfo;
    ZBufInfo    sendZBufInfo;
} ModgzipMData;

void releaseZBufInfo( ZBufInfo& zBufInfo )
{
    if ( zBufInfo.inited == 0 )
        return ;
    
    if ( zBufInfo.pLoopbuf )
    {
        lsr_loopbuf_xdelete( zBufInfo.pLoopbuf, (lsr_xpool_t *)zBufInfo.pZStream->opaque );
        zBufInfo.pLoopbuf = NULL;
    }
    
    if ( zBufInfo.pZStream )
    {
        if ( zBufInfo.iZState == Z_INITED || zBufInfo.iZState == Z_EOF )
        {
            if ( zBufInfo.compressLevel == 0 )
                inflateEnd( zBufInfo.pZStream );
            else
                deflateEnd( zBufInfo.pZStream );
        }
        //delete zBufInfo.pZStream;
        zBufInfo.pZStream = NULL;
    }

    
    zBufInfo.inited = 0;
}

static int releaseData( void *data )
{
    ModgzipMData *myData = ( ModgzipMData * )data;
    if ( myData )
    {
        releaseZBufInfo( myData->recvZBufInfo );
        releaseZBufInfo( myData->sendZBufInfo );
        //delete myData;
    }
    return 0;
}

static int releaseDataPartR( lsi_cb_param_t *rec, lsi_module_t *pModule )
{
    ModgzipMData *myData = ( ModgzipMData * ) g_api->get_module_data( rec->_session, pModule, LSI_MODULE_DATA_HTTP );
    if ( myData )
    {
        releaseZBufInfo( myData->recvZBufInfo );
        if ( myData->sendZBufInfo.inited == 0 )
        {
            //delete myData;
            g_api->set_module_data( rec->_session, pModule, LSI_MODULE_DATA_HTTP, NULL );
        }
    }
    return 0;
}

static z_stream *init_Z_Stream( lsr_xpool_t *pPool )
{
    z_stream *pStream = (z_stream *)lsr_xpool_alloc( pPool, sizeof( z_stream ));
    if ( !pStream )
        return NULL;
    memset( pStream, 0, sizeof( z_stream ));
    pStream->opaque = pPool;
    pStream->zalloc = (alloc_func)lsr_xpool_calloc;
    pStream->zfree = (free_func)lsr_xpool_free;
    return pStream;
}

static int init_Z_State_Buf( ZBufInfo& zBufInfo, lsr_xpool_t *pPool )
{
    int ret = 0;
    //assert(zBufInfo.pZStream == NULL);
    memset( &zBufInfo, 0, sizeof( ZBufInfo ) );
    //zBufInfo.pZStream = new z_stream;
    zBufInfo.pZStream = init_Z_Stream( pPool );
    if ( zBufInfo.pZStream == NULL )
        ret = -1;
    else
    {
        zBufInfo.pLoopbuf = lsr_loopbuf_xnew( INIT_LOOPBUF_SIZE, pPool );
        if ( zBufInfo.pLoopbuf == NULL )
        {
            //delete zBufInfo.pZStream;
            lsr_xpool_free( pPool, zBufInfo.pZStream );
            ret = -1;
        }
    }
    zBufInfo.inited = ( ret == 0 );
    return ret;
}

static int initZstream( lsi_cb_param_t *rec, lsi_module_t *pModule, z_stream *pStream, uint8_t compressLevel )
{
    int ret = 0;
    pStream->avail_in = 0;
    pStream->next_in = Z_NULL;
    if ( compressLevel == 0 )
    {
        //To automatically detect gzip and deflate
        if ( inflateInit2( pStream, 32 + MAX_WBITS ) != Z_OK )
            ret = -1;
    }
    else
    {
        if ( deflateInit2( pStream, compressLevel, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY ) != Z_OK )
            ret = -1;
    }

    g_api->log( rec->_session, ( ( ret == 0 ) ? LSI_LOG_DEBUG : LSI_LOG_ERROR ),
                        "[%s%s] initZstream init method [%s], %s.\n", g_api->get_module_name( pModule ),
                        ( (compressLevel == 0) ? DECOMPRESS_FLAG : COMPRESS_FLAG ),
                        ( (compressLevel == 0) ? "un-Gzip" : "Gzip"), (( ret == 0 ) ? "succeed" : "failed" ) );
    return ret;
}

int flushLoopBuf( lsi_cb_param_t *rec, int iZState, lsr_loopbuf_t *pBuf )
{
    int written = 0;
    int sz = 0;
    while( !lsr_loopbuf_empty( pBuf ) )
    {
        sz = g_api->stream_write_next( rec, lsr_loopbuf_begin( pBuf ), lsr_loopbuf_block_size( pBuf ) );
        if ( sz < 0 )
            return -1;
        else if (sz > 0)
        {
            lsr_loopbuf_pop_front( pBuf, sz );
            written += sz;
        }
        else
            break;
    }
    
    if ( lsr_loopbuf_empty( pBuf ) && iZState == Z_END )
    {
        rec->_flag_in |= LSI_CB_FLAG_IN_EOF;
        g_api->stream_write_next( rec, NULL, 0);
    }
    
    return written;
}

static int clearData( lsi_cb_param_t *rec )
{
    g_api->free_module_data( rec->_session, g_api->get_module( rec ), LSI_MODULE_DATA_HTTP, releaseData );
    return 0;
}

static int clearDataPartR( lsi_cb_param_t *rec )
{
    return releaseDataPartR( rec, (lsi_module_t *)g_api->get_module(rec) );
}

static int compressbuf( lsi_cb_param_t *rec, lsi_module_t *pModule, int isSend )
{
    ModgzipMData *myData = ( ModgzipMData * ) g_api->get_module_data( rec->_session, pModule, LSI_MODULE_DATA_HTTP );
    if ( !myData )
        return LSI_RET_ERROR;

    const char *pSendingFlag = ( isSend ? SENDING_FLAG : RECVING_FLAG );
    ZBufInfo *pZBufInfo = ( isSend ? & ( myData->sendZBufInfo ) : & ( myData->recvZBufInfo ) );
    const char *pCompressFlag = ( (pZBufInfo->compressLevel == 0) ? DECOMPRESS_FLAG : COMPRESS_FLAG );
    
    //Needn't call here since falg set in main program
    //g_api->set_resp_buffer_gzip_flag(rec->_session,  ((pZBufInfo->compressLevel == 0) ? 0 : 1 ) );

    z_stream *pStream = pZBufInfo->pZStream;
    lsr_loopbuf_t *pBuf = pZBufInfo->pLoopbuf;
    const char *pModuleName = g_api->get_module_name( pModule );
    int written = 0;

    if ( pZBufInfo->inited == 0 )
        return LSI_RET_ERROR;
    if ( pZBufInfo->iZState == Z_UNINITED )
    {
        if ( initZstream( rec, pModule, pStream, pZBufInfo->compressLevel ) != 0 )
        {
            //assert(isSend == 0);
            g_api->set_session_hook_enable_flag( rec->_session, LSI_HKPT_RECV_RESP_BODY, pModule, 0 ); //To disable
            return g_api->stream_write_next( rec, ( const char * )rec->_param, rec->_param_len );
        }
        else
            pZBufInfo->iZState = Z_INITED;
    }
    
    int sz, len, ret;
    int consumed = 0;
    int finish = Z_NO_FLUSH;
    if ( pZBufInfo->iZState < Z_EOF )
    {
        pStream->avail_in = rec->_param_len;
        pStream->next_in = ( Byte * )rec->_param;
        if ( rec->_flag_in & LSI_CB_FLAG_IN_EOF )
        {
            pZBufInfo->iZState = Z_EOF;
            rec->_flag_in |= LSI_CB_FLAG_IN_FLUSH;
        }
        else if ( rec->_flag_in & LSI_CB_FLAG_IN_FLUSH )
        {
            finish = Z_PARTIAL_FLUSH;
        }
    }
    rec->_flag_in &= ~LSI_CB_FLAG_IN_EOF;
    
    if (pZBufInfo->iZState == Z_EOF) 
        finish = Z_FINISH;
    if ( !lsr_loopbuf_empty( pBuf ))
    {
        written = flushLoopBuf( rec, pZBufInfo->iZState, pBuf );
        if ( written == -1 )
            return -1;
        if ( !lsr_loopbuf_empty( pBuf ))
        {
            if ( rec->_flag_out )
                *rec->_flag_out |= LSI_CB_FLAG_OUT_BUFFERED_DATA;
            return 0;
        }
    }
    
    if ( pZBufInfo->iZState == Z_END ) 
        //return LSI_RET_OK;   cannot return LSI_RET_OK here , will be called repeatedly in endless loop.
        // either return error or return the full length of the input data, to consume it. 
        // we wont be in a endless loop.
        return rec->_param_len;         

    do
    {
        lsr_loopbuf_xguarantee( pBuf, 1024, g_api->get_session_pool( rec->_session ) );
        len = lsr_loopbuf_contiguous( pBuf );
        pStream->avail_out = len;
        pStream->next_out = (unsigned char *)lsr_loopbuf_end( pBuf );

        if ( pZBufInfo->compressLevel == 0 )
            ret = inflate( pStream, finish );
        else
            ret = deflate( pStream, finish );

        if ( ret >= Z_OK )
        {
            consumed = rec->_param_len - pStream->avail_in;
            lsr_loopbuf_used( pBuf, len - pStream->avail_out );
            
            if ( pZBufInfo->iZState ==Z_EOF && ret == Z_STREAM_END )
            {
                if ( pZBufInfo->compressLevel == 0 )
                    inflateEnd( pStream );
                else
                    deflateEnd( pStream );
//                 pStream = NULL;
//                 pZBufInfo->pZStream = NULL;
                pZBufInfo->iZState = Z_END;
                g_api->log( rec->_session, LSI_LOG_DEBUG, "[%s%s] compressbuf end of stream set.\n",
                                    pModuleName, pCompressFlag );
            }
            
            sz = flushLoopBuf( rec, pZBufInfo->iZState, pBuf );
            if ( sz > 0 )
                written += sz;
            else if ( sz < 0 )
            {
                g_api->log( rec->_session, LSI_LOG_ERROR, "[%s%s] compressbuf in %d, return %d (written %d, flag in %d)\n",
                                    pModuleName, pCompressFlag, rec->_param_len, sz, written, rec->_flag_in );
                return LSI_RET_ERROR;
            }
            if ( !lsr_loopbuf_empty( pBuf ))
                break;
        }
        else
        {
            g_api->log( rec->_session, LSI_LOG_ERROR, "[%s%s] compressbuf in %d, compress function return %d\n",
                                pModuleName, pCompressFlag, rec->_param_len, ret );
            if (ret != Z_BUF_ERROR) //value is -5
                return LSI_RET_ERROR;
        }
    }
    while ( pZBufInfo->iZState != Z_END && pStream->avail_out == 0 );

    //NOTICE: Changed loopbuf to use lsr_xloopbuf, pBuff no longer exists.
    //written += sendoutLoopBuf(rec, pBuff);
    if ( !(lsr_loopbuf_empty( pBuf )) || (pZBufInfo->iZState != Z_END && pStream->avail_out == 0))
    {
        if ( rec->_flag_out )
            *rec->_flag_out |= LSI_CB_FLAG_OUT_BUFFERED_DATA;
    }

    g_api->log( rec->_session, LSI_LOG_DEBUG, "[%s%s] compressbuf [%s] in %d, consumed: %d, written %d, flag in %d, buffer has %d.\n",
                        pModuleName, pCompressFlag, pSendingFlag, rec->_param_len, consumed, written, rec->_flag_in, lsr_loopbuf_size( pBuf ) );
    return consumed;
}

static int sendingHook( lsi_cb_param_t *rec )
{
    return compressbuf( rec, (lsi_module_t *)g_api->get_module(rec), 1 );
}
static int recvingHook( lsi_cb_param_t *rec )
{
    return compressbuf( rec, (lsi_module_t *)g_api->get_module(rec), 0 );
}

static int init( lsi_module_t * pModule )
{
    return g_api->init_module_data( pModule, releaseData, LSI_MODULE_DATA_HTTP );
}





static lsi_serverhook_t compressHooks[] = {
    { LSI_HKPT_SEND_RESP_BODY, sendingHook, 3000, LSI_HOOK_FLAG_PROCESS_STATIC | LSI_HOOK_FLAG_TRANSFORM },
    { LSI_HKPT_RECV_RESP_BODY, recvingHook, 3000, 1 },
    { LSI_HKPT_RCVD_RESP_BODY, clearDataPartR, 3000, 0 },
    { LSI_HKPT_HTTP_END, clearData, LSI_HOOK_NORMAL, 0 },
    { LSI_HKPT_HANDLER_RESTART, clearData, LSI_HOOK_NORMAL, 0 },
    lsi_serverhook_t_END  //Must put this at the end position
};


static lsi_serverhook_t decompressHooks[] = {
    { LSI_HKPT_SEND_RESP_BODY, sendingHook, -3000, LSI_HOOK_FLAG_PROCESS_STATIC | LSI_HOOK_FLAG_TRANSFORM },
    { LSI_HKPT_RECV_RESP_BODY, recvingHook, -3000, 1 },
    { LSI_HKPT_RCVD_RESP_BODY, clearDataPartR, -3000, 0 },
    { LSI_HKPT_HTTP_END, clearData, LSI_HOOK_NORMAL, 0 },
    { LSI_HKPT_HANDLER_RESTART, clearData, LSI_HOOK_NORMAL, 0 },
    lsi_serverhook_t_END  //Must put this at the end position
};




lsi_module_t modcompress = { LSI_MODULE_SIGNATURE, init, NULL, NULL, MODULE_VERSION, compressHooks, {0} };
lsi_module_t moddecompress = { LSI_MODULE_SIGNATURE, init, NULL, NULL, MODULE_VERSION, decompressHooks, {0} };

static void set_Module_data( lsi_session_t *session, lsi_module_t * pModule, ModgzipMData *myData )
{
    g_api->set_module_data( session, pModule, LSI_MODULE_DATA_HTTP, ( void * )myData );
    g_api->set_session_hook_enable_flag( session, LSI_HKPT_HTTP_END, pModule, 1 );
    g_api->set_session_hook_enable_flag( session, LSI_HKPT_HANDLER_RESTART, pModule, 1 );
}

static int enableHooks( lsi_session_t *session, lsi_module_t *pModule, int isSend, uint8_t compressLevel )
{
    ModgzipMData *myData = ( ModgzipMData * ) g_api->get_module_data( session, pModule, LSI_MODULE_DATA_HTTP );
    lsr_xpool_t *pPool = g_api->get_session_pool( session );
    if ( !myData )
    {
        //myData = new ModgzipMData;
        myData = (ModgzipMData *)lsr_xpool_alloc( pPool, sizeof( ModgzipMData ));
        if ( !myData )
        {
            g_api->log( session, LSI_LOG_ERROR, "[%s] AddHooks failed: no enough memory [Error 1].\n", g_api->get_module_name( pModule ) );
            return -1;
        }
        else
        {
            memset( myData, 0, sizeof( *myData ) );
        }
    }

    int err = 0;
    if ( isSend )
    {
        if ( init_Z_State_Buf( myData->sendZBufInfo, pPool ) == 0 )
        {
            err = g_api->set_session_hook_enable_flag( session, LSI_HKPT_SEND_RESP_BODY, pModule, 1 );
            if (err == 0)
                myData->sendZBufInfo.compressLevel = compressLevel;
        }
        else
            err = 1;
    }
    else
    {
        if ( init_Z_State_Buf( myData->recvZBufInfo, pPool ) == 0 )
        {
            err = g_api->set_session_hook_enable_flag( session, LSI_HKPT_RECV_RESP_BODY, pModule, 1 );
            if (err == 0)
            {
                g_api->set_session_hook_enable_flag( session, LSI_HKPT_RCVD_RESP_BODY, pModule, 1 );
                myData->recvZBufInfo.compressLevel = compressLevel;
            }
        }
        else
            err = 1;
    }

    if ( err == 0 )
    {
        set_Module_data( session, pModule, myData );
        return 0;
    }
    else
    {
        //If no prevoius inited ZBufInfo, so this is the first time try, no module data set before.
        if ( myData->recvZBufInfo.inited == 0 && myData->sendZBufInfo.inited == 0 )
            lsr_xpool_free( pPool, myData );
            //delete myData;
        g_api->log( session, LSI_LOG_ERROR, "[%s] AddHooks failed: no enough memory or enable hooks failed [Error 2].\n", g_api->get_module_name( pModule ) );
        return -1;
    }
}

//The below is the only function exported
int addModgzipFilter( lsi_session_t *session, int isSend, uint8_t compressLevel )
{
    if ( compressLevel == 0 )
        return enableHooks( session, &moddecompress, isSend, 0 );
    else
        return enableHooks( session, &modcompress, isSend, compressLevel );
}

