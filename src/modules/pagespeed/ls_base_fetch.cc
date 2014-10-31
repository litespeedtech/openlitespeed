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

#include "ls_base_fetch.h"

#include "net/instaweb/http/public/response_headers.h"
#include "net/instaweb/rewriter/public/rewrite_stats.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "net/instaweb/util/public/message_handler.h"
#include <unistd.h>
#include <errno.h>

LsiBaseFetch::LsiBaseFetch( lsi_session_t* session, int pipe_fd,
                            LsiServerContext* server_context,
                            const RequestContextPtr& request_ctx,
                            PreserveCachingHeaders preserve_caching_headers )
    : AsyncFetch( request_ctx ),
      session_( session ),
      server_context_( server_context ),
      done_called_( false ),
      last_buf_sent_( false ),
      pipe_fd_( pipe_fd ),
      references_( 2 ),
      ipro_lookup_( false ),
      m_success( false ),
      preserve_caching_headers_( preserve_caching_headers )
{
    if( pthread_mutex_init( &mutex_, NULL ) )
    {
        CHECK( 0 );
    }
}

LsiBaseFetch::~LsiBaseFetch()
{
    pthread_mutex_destroy( &mutex_ );
}

void LsiBaseFetch::Lock()
{
    pthread_mutex_lock( &mutex_ );
}

void LsiBaseFetch::Unlock()
{
    pthread_mutex_unlock( &mutex_ );
}

bool LsiBaseFetch::HandleWrite( const StringPiece& sp,
                                MessageHandler* handler )
{
    Lock();
    buffer_.append( sp.data(), sp.size() );
    Unlock();
    //g_api->log(NULL, LSI_LOG_DEBUG, "*****[size %d]%s\n", sp.size(), sp.data());
    return true;
}

int LsiBaseFetch::CopyBufferToLs( lsi_session_t* session )
{
    CHECK(!(done_called_ && last_buf_sent_))
        << "CopyBufferToLs() was called after the last buffer was sent";

    if( !done_called_ && buffer_.empty() )
        return 1;

    //g_api->log( NULL, LSI_LOG_DEBUG, "* * *[size %d]\n", buffer_.size() );  //, buffer_.c_str());
    copy_response_body_to_buff( session, buffer_.c_str(), buffer_.size(), done_called_ /* send_last_buf */);

    // Done with buffer contents now.
    buffer_.clear();

    if( done_called_ )
    {
        last_buf_sent_ = true;
        return 0;
    }

    return 1;
}

int LsiBaseFetch::CollectAccumulatedWrites( lsi_session_t* session )
{
    if( last_buf_sent_ )
        return 0;

    int rc;
    Lock();
    rc = CopyBufferToLs( session );
    Unlock();
    return rc;
}

int LsiBaseFetch::CollectHeaders( lsi_session_t* session )
{
    const ResponseHeaders* pagespeed_headers = response_headers();

    if( content_length_known() )
    {
        g_api->set_resp_content_length( session, content_length() );
    }

    return copy_response_headers_to_server( session, *pagespeed_headers,
                                            preserve_caching_headers_ );
}

void LsiBaseFetch::RequestCollection()
{
    if( pipe_fd_ == -1 )
        return ;

    int rc;
    char c = 'A';
    rc = write( pipe_fd_, &c, 1 );
    g_api->log( NULL, LSI_LOG_DEBUG, "[Module:modpagespeed]RequestCollection called, errno %d, pipe_fd_=%d, rc=%d, session=%ld, last_buf_sent_=%s\n\n",
                errno, pipe_fd_, rc, ( long ) session_, ( last_buf_sent_ ? "true" : "false" ) );
}

void LsiBaseFetch::HandleHeadersComplete()
{
    int status_code = response_headers()->status_code();
    bool status_ok = ( status_code != 0 ) && ( status_code < 400 );

    if( !ipro_lookup_ || status_ok )
    {
        // If this is a 404 response we need to count it in the stats.
        if( response_headers()->status_code() == HttpStatus::kNotFound )
        {
            server_context_->rewrite_stats()->resource_404_count()->Add( 1 );
        }
    }

    // For the IPRO lookup, supress notification of the nginx side here.
    // If we send both this event and the one from done, nasty stuff will happen
    // if we loose the race with with the nginx side destructing this base fetch
    // instance (and thereby clearing the byte and its pending extraneous event.
    if( !ipro_lookup_ )
    {
       //RequestCollection();  // Headers available.
    }
}

bool LsiBaseFetch::HandleFlush( MessageHandler* handler )
{
    //RequestCollection();
    return true;
}

void LsiBaseFetch::Release()
{
    DecrefAndDeleteIfUnreferenced();
}

void LsiBaseFetch::DecrefAndDeleteIfUnreferenced()
{
    if( __sync_add_and_fetch( &references_, -1 ) == 0 )
    {
        delete this;
    }
}

void LsiBaseFetch::HandleDone( bool success )
{
    m_success = success;
    g_api->log( session_, LSI_LOG_DEBUG, "[Module:modpagespeed]HandleDone called, success=%d, done_called_=%d!\n",
                success, done_called_ );

    Lock();
    done_called_ = true;
    Unlock();
    RequestCollection();

    //Just set to -1, but not to close it because it does not belong to this class
    pipe_fd_ = -1;

    DecrefAndDeleteIfUnreferenced();
}
