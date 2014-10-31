/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2014  LiteSpeed Technologies, Inc.                        *
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
#ifndef LSI_BASE_FETCH_H_
#define LSI_BASE_FETCH_H_

#include <pthread.h>
#include "pagespeed.h"
#include "ls_server_context.h"
#include "net/instaweb/http/public/async_fetch.h"
#include "net/instaweb/http/public/headers.h"
#include "net/instaweb/util/public/string.h"

class LsiBaseFetch : public AsyncFetch
{
public:
    LsiBaseFetch( lsi_session_t* session, int pipe_fd,
                  LsiServerContext* server_context,
                  const RequestContextPtr& request_ctx,
                  PreserveCachingHeaders preserve_caching_headers );
    virtual ~LsiBaseFetch();

    int CollectAccumulatedWrites( lsi_session_t* session );

    int CollectHeaders( lsi_session_t* session );

    void Release();
    void set_ipro_lookup( bool x )
    {
        ipro_lookup_ = x;
    }
    
    void set_pipe_fd( int pipe_fd )
    {
        pipe_fd_ = pipe_fd;
        g_api->log( session_, LSI_LOG_DEBUG, "[Module:modpagespeed]set_pipe_fd pipe_fd_ = %d.\n", pipe_fd_ );
    }
//     bool isLastBufSent()
//     {
//         return last_buf_sent_;
//     }
//     bool isDoneCalled( bool& success )
//     {
//         success = m_success;
//         return done_called_;
//     }

    bool isDoneAndSuccess()    { return done_called_ && m_success;  }
private:
    virtual bool HandleWrite( const StringPiece& sp, MessageHandler* handler );
    virtual bool HandleFlush( MessageHandler* handler );
    virtual void HandleHeadersComplete();
    virtual void HandleDone( bool success );

    void RequestCollection();
    int CopyBufferToLs( lsi_session_t* session );

    void Lock();
    void Unlock();

    // Called by Done() and Release().  Decrements our reference count, and if
    // it's zero we delete ourself.
    void DecrefAndDeleteIfUnreferenced();

    lsi_session_t* session_;
    GoogleString buffer_;
    LsiServerContext* server_context_;
    bool done_called_;
    bool last_buf_sent_;
    int pipe_fd_;
    int references_;
    pthread_mutex_t mutex_;
    bool ipro_lookup_;
    bool m_success;
    PreserveCachingHeaders preserve_caching_headers_;
};

#endif  // LSI_BASE_FETCH_H_
