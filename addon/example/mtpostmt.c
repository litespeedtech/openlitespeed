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
*                                                                            *
*    This is just like mtpost, but it only has a global for the total count. *
*    All data is done from independent buffers to we can be massively        *
*    parallel if the system is willint to do that.                           *
*****************************************************************************/
#include "../include/ls.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MNAME       mtpostmt
lsi_module_t MNAME;

volatile int s_lock = 0;
#define LOCK_TEST_AND_SET   while (__sync_lock_test_and_set(&s_lock,1)) while (s_lock);
#define UNLOCK_TEST_AND_SET __sync_lock_release(&s_lock);
typedef struct session_list {
    lsi_session_t       *   m_plsi_session;    
    int                     m_TotalCount;
    int                     m_RecvSize;
    char                *   m_pmtPostBuffer;
    struct session_list *   m_psession_listNext;
} session_list_t_;
session_list_t_ *s_psession_list = NULL;

/** You must lock the session list before calling **/
static session_list_t_ *session_listFind(const lsi_session_t *plsi_session) {
    // Assume locked!
    session_list_t_ *psession_list = s_psession_list;
    while (psession_list) {
        if (psession_list->m_plsi_session == plsi_session) {
            // Found!
            return psession_list;
        }
        psession_list = psession_list->m_psession_listNext;
    }
    g_api->set_status_code(plsi_session, 500);
    LSM_ERR((&MNAME), plsi_session,"ERROR! Session %p not found in list!\n", plsi_session);
    g_api->end_resp(plsi_session);
    return NULL;
}


/** You must NOT lock the sessioni list before calling **/
static void session_listFree(const lsi_session_t    *plsi_session,
                             session_list_t_        *psession_list) {
    LOCK_TEST_AND_SET
    LSM_DBG((&MNAME), plsi_session,"Freeing session: %p, %p (in struct)\n", plsi_session, psession_list->m_plsi_session);
    if (!s_psession_list) {
        g_api->set_status_code(plsi_session, 500);
        LSM_ERR((&MNAME), plsi_session,"ERROR!  Attempt to free %p from an empty psession list!\n", plsi_session);
        g_api->end_resp(plsi_session);
        UNLOCK_TEST_AND_SET
        return;
    }
    if (s_psession_list == psession_list) {
        // Found!
        s_psession_list = psession_list->m_psession_listNext;
        LSM_DBG((&MNAME), plsi_session,"Freeing root session: %p\n", plsi_session);
    }
    else {
        session_list_t_ *psession_listLast = s_psession_list;
        int bFound = 0;
        while (psession_listLast->m_psession_listNext) {
            if (psession_listLast->m_psession_listNext == psession_list) {
                // Found - link around it.
                psession_listLast->m_psession_listNext = psession_list->m_psession_listNext;
                bFound = 1;
                break;
            }
            else {
                psession_listLast = psession_listLast->m_psession_listNext;
            }
        }
        if (!bFound) {
            LSM_ERR((&MNAME), plsi_session,"ERROR!  Attempt to free a session that wasn't found: %p!\n", plsi_session);
            UNLOCK_TEST_AND_SET
            return;
        }
        else {
            LSM_DBG((&MNAME), plsi_session, "Freeing non-root session: %p\n", plsi_session);
        }
    }
    if (psession_list->m_pmtPostBuffer) {
        free(psession_list->m_pmtPostBuffer);
        psession_list->m_pmtPostBuffer = NULL;
    }
    free(psession_list);
    UNLOCK_TEST_AND_SET
}

        
static int sendResponse(const lsi_session_t *plsi_session, 
                        session_list_t_     *psession_list,
                        char                *pExplanation) {
    char response[256];
    g_api->set_status_code(plsi_session, 200);
    g_api->set_resp_header(plsi_session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);
    sprintf(response,"Response: Received: %d bytes user data", psession_list->m_TotalCount);
    LSM_DBG((&MNAME), plsi_session,"Sending response: %s for session %p during %s\n", response, plsi_session, pExplanation);
    g_api->append_resp_body(plsi_session, response, strlen(response));
    g_api->end_resp(plsi_session);
    session_listFree(psession_list->m_plsi_session, psession_list);
    return(0);
}

static int process_req(const lsi_session_t *plsi_session)
{
    int headerLength = 256;
    char *size;
    session_list_t_ *psession_list = NULL;
    int recvSize = 0;
    
    LSM_DBG((&MNAME), plsi_session,"Entering mtpostmt, request content length (save session stuff): %d, session %p\n", (int)g_api->get_req_content_length(plsi_session), plsi_session);
    size = (char *)g_api->get_req_header_by_id(plsi_session, LSI_HDR_USERAGENT, &headerLength);
    if (headerLength != 256) {
        recvSize = atoi(size);
    }
    if (!recvSize) {
        recvSize = 30000;
    }
    psession_list = malloc(sizeof(session_list_t_));
    if (!psession_list) {
        g_api->set_status_code(plsi_session, 507);
        LSM_ERR((&MNAME), plsi_session,"ERROR!  Insufficient memory to allocate session size %d bytes, session: %p\n", (int)sizeof(session_list_t_), plsi_session);
        g_api->end_resp(plsi_session);
        return(507);
    }
    memset(psession_list,0,sizeof(session_list_t_));
    psession_list->m_RecvSize = recvSize;
    psession_list->m_plsi_session = (lsi_session_t *)plsi_session;
    psession_list->m_pmtPostBuffer = malloc(recvSize);
    if (!psession_list->m_pmtPostBuffer) {
        g_api->set_status_code(plsi_session, 507);
        LSM_ERR((&MNAME), plsi_session,"ERROR!  Insufficient memory to allocate %d bytes for data, session: %p\n", psession_list->m_RecvSize, plsi_session);
        g_api->end_resp(plsi_session);
        free(psession_list);
        return(507);
    }
    LOCK_TEST_AND_SET
    psession_list->m_psession_listNext = s_psession_list;
    s_psession_list = psession_list;
    UNLOCK_TEST_AND_SET
    if (g_api->get_req_content_length(plsi_session)) {
        int result;
        do {
            result = g_api->read_req_body(plsi_session,psession_list->m_pmtPostBuffer,recvSize);
            if (result > 0) {
                LSM_DBG((&MNAME), plsi_session,"In mtpostmt, Read %d bytes (requested %d)\n", result, recvSize);
                LOCK_TEST_AND_SET
                psession_list->m_TotalCount += result;
                UNLOCK_TEST_AND_SET
            }
            if (result == -1) {
                LSM_ERR((&MNAME), plsi_session, "ERROR!  FATAL ERROR IN READ OF BODY, session: %p, in process_req\n", plsi_session);
                session_listFree(plsi_session, psession_list);
                return(-1);
            }
        } while (result);
    }
    if (g_api->is_req_body_finished(plsi_session)) {
        sendResponse(plsi_session, psession_list, "process_req");
        //session_listFree(plsi_session, psession_list); Done in sendResponse
        return(0);
    }
    return 0;
}


static int read_req_body(const lsi_session_t *plsi_session)
{
    int result;
    session_list_t_ *psession_list;
    LOCK_TEST_AND_SET
    psession_list = session_listFind(plsi_session);
    UNLOCK_TEST_AND_SET
    if (!psession_list) {
        // Error already reported
        return(500);
    }
    LSM_DBG((&MNAME), plsi_session,"Entering read_req_body, read %d of %d\n", psession_list->m_TotalCount, (int)g_api->get_req_content_length(plsi_session));
    do {
        result = g_api->read_req_body(plsi_session,psession_list->m_pmtPostBuffer,psession_list->m_RecvSize);
        if (result > 0) {
            LSM_DBG((&MNAME), plsi_session,"In mtpostmt, Read %d bytes (requested %d bytes)\n", result, psession_list->m_RecvSize);
            LOCK_TEST_AND_SET
            psession_list->m_TotalCount += result;
            UNLOCK_TEST_AND_SET
        }
        if (result == -1) {
            LSM_ERR((&MNAME), plsi_session, "ERROR!  FATAL ERROR IN READ OF BODY, session: %p in read_req_body\n", plsi_session);
            session_listFree((lsi_session_t *)plsi_session, psession_list);
            return(-1);
        }
    } while (result);
    LOCK_TEST_AND_SET
    if (psession_list->m_TotalCount >= g_api->get_req_content_length(plsi_session)) {
        UNLOCK_TEST_AND_SET
        sendResponse(plsi_session, psession_list, "read_req_body");
        //session_listFree(plsi_session, psession_list); Done in sendResponse
        return(0);
    }
    UNLOCK_TEST_AND_SET
    return 0;
}


/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_reqhdlr_t myhandler = { process_req, read_req_body, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, NULL, &myhandler, NULL, NULL, NULL, {0} };


