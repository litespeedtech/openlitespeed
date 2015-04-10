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

#include "ls_base_fetch.h"

#include <lsr/ls_atomic.h>

#include "net/instaweb/http/public/response_headers.h"
#include "net/instaweb/rewriter/public/rewrite_stats.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "net/instaweb/util/public/message_handler.h"
#include <unistd.h>
#include <errno.h>

LsiBaseFetch::LsiBaseFetch(lsi_session_t *session, int pipe_fd,
                           LsServerContext *server_context,
                           const RequestContextPtr &request_ctx,
                           PreserveCachingHeaders preserve_caching_headers)
    : AsyncFetch(request_ctx),
      m_session(session),
      m_pServerContext(server_context),
      m_bDoneCalled(false),
      m_bLastBufSent(false),
      m_iPipeFd(pipe_fd),
      m_iReferences(2),
      m_bIproLookup(false),
      m_bSuccess(false),
      m_preserveCachingHeaders(preserve_caching_headers)
{
    if (pthread_mutex_init(&m_mutex, NULL))
        CHECK(0);
}

LsiBaseFetch::~LsiBaseFetch()
{
    pthread_mutex_destroy(&m_mutex);
}

void LsiBaseFetch::Lock()
{
    pthread_mutex_lock(&m_mutex);
}

void LsiBaseFetch::Unlock()
{
    pthread_mutex_unlock(&m_mutex);
}

bool LsiBaseFetch::HandleWrite(const StringPiece &sp,
                               MessageHandler *handler)
{
    Lock();
    m_buffer.append(sp.data(), sp.size());
    Unlock();
    return true;
}

int LsiBaseFetch::CopyBufferToLs(lsi_session_t *session)
{
    CHECK(!(m_bDoneCalled && m_bLastBufSent))
            << "CopyBufferToLs() was called after the last buffer was sent";

    if (!m_bDoneCalled && m_buffer.empty())
        return 1;

    CopyRespBodyToBuf(session, m_buffer.c_str(), m_buffer.size(),
                               m_bDoneCalled /* send_last_buf */);

    m_buffer.clear();

    if (m_bDoneCalled)
    {
        m_bLastBufSent = true;
        return 0;
    }

    return 1;
}

int LsiBaseFetch::CollectAccumulatedWrites(lsi_session_t *session)
{
    if (m_bLastBufSent)
        return 0;

    int rc;
    Lock();
    rc = CopyBufferToLs(session);
    Unlock();
    return rc;
}

int LsiBaseFetch::CollectHeaders(lsi_session_t *session)
{
    const ResponseHeaders *pagespeed_headers = response_headers();

    if (content_length_known())
        g_api->set_resp_content_length(session, content_length());

    return CopyRespHeadersToServer(session, *pagespeed_headers,
                                           m_preserveCachingHeaders);
}

void LsiBaseFetch::RequestCollection()
{
    if (m_iPipeFd == -1)
        return ;

    int rc;
    char c = 'A';
    rc = write(m_iPipeFd, &c, 1);
//     g_api->log(NULL, LSI_LOG_DEBUG,
//                "[Module:modpagespeed]RequestCollection called, errno %d, "
//                "pipe_fd_=%d, rc=%d, session=%ld, m_bLastBufSent=%s\n\n",
//                errno, m_iPipeFd, rc, (long) m_session, 
//                (m_bLastBufSent ? "true" : "false"));
}

void LsiBaseFetch::HandleHeadersComplete()
{
    int statusCode = response_headers()->status_code();
    bool statusOk = (statusCode != 0 && statusCode < 400);

    if (!m_bIproLookup || statusOk)
    {
        // If this is a 404 response we need to count it in the stats.
        if (response_headers()->status_code() == HttpStatus::kNotFound)
            m_pServerContext->rewrite_stats()->resource_404_count()->Add(1);
    }

    if (!m_bIproLookup)
    {
        //RequestCollection();  // Headers available.
    }
}

bool LsiBaseFetch::HandleFlush(MessageHandler *handler)
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
    if (ls_atomic_add(&m_iReferences, -1) == 0)
        delete this;
}

void LsiBaseFetch::HandleDone(bool success)
{
    m_bSuccess = success;
//     g_api->log(m_session, LSI_LOG_DEBUG,
//                "[Module:modpagespeed]HandleDone called, success=%d, m_bDoneCalled=%d!\n",
//                success, m_bDoneCalled);

    Lock();
    m_bDoneCalled = true;
    Unlock();
    RequestCollection();

    //Just set to -1, but not to close it because it does not belong to this class
    m_iPipeFd = -1;

    DecrefAndDeleteIfUnreferenced();
}
