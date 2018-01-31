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
#include "../include/ls.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MNAME       mtpost
lsi_module_t MNAME;

volatile int s_lock = 0;
#define LOCK_TEST_AND_SET   while (__sync_lock_test_and_set(&s_lock,1)) while (s_lock);
#define UNLOCK_TEST_AND_SET __sync_lock_release(&s_lock);
char *mtpostBuffer = NULL;
int totalCount = 0;
int recvSize;

static int sendResponse(const lsi_session_t *session) {
    char response[256];
    g_api->set_status_code(session, 200);
    g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);
    sprintf(response,"Response: Received: %d bytes user data", totalCount);
    LSM_DBG((&MNAME), session,"Sending response: %s\n",response);
    g_api->append_resp_body(session,response,strlen(response));
    g_api->end_resp(session);
    return(0);
}

static int process_req(const lsi_session_t *session)
{
    int headerLength = 256;
    char *size;
    LSM_DBG((&MNAME), session,"Entering mtpost, request content length: %d\n", (int)g_api->get_req_content_length(session));
    LOCK_TEST_AND_SET;
    totalCount = 0;
    recvSize = 0;
    size = (char *)g_api->get_req_header_by_id(session, LSI_HDR_USERAGENT, &headerLength);
    if (headerLength != 256) {
        recvSize = atoi(size);
    }
    if (!recvSize) {
        recvSize = 30000;
    }
    if (mtpostBuffer) {
        free(mtpostBuffer);
    }
    mtpostBuffer = malloc(recvSize);
    if (!mtpostBuffer) {
        g_api->set_status_code(session, 507);
        LSM_ERR((&MNAME), session,"Insufficient memory to allocation %d bytes", recvSize);
        g_api->end_resp(session);
        UNLOCK_TEST_AND_SET
        return(507);
    }
    if (g_api->get_req_content_length(session)) {
        int result;
        do {
            result = g_api->read_req_body(session,mtpostBuffer,recvSize);
            if (result > 0) {
                LSM_DBG((&MNAME), session,"In mtpost, Read %d bytes (requested %d)\n", result, recvSize);
                totalCount += result;
            }
            if (result == -1) {
                LSM_ERR((&MNAME), session, "FATAL ERROR IN READ OF BODY!\n");
                UNLOCK_TEST_AND_SET
                return(-1);
            }
        } while (result);
    }
    if (g_api->is_req_body_finished(session)) {
        sendResponse(session);
    }
    UNLOCK_TEST_AND_SET
    return 0;
}


static int read_req_body(const lsi_session_t *session)
{
    int result;
    LSM_DBG((&MNAME), session,"Entering read_req_body, read %d of %d\n", totalCount, (int)g_api->get_req_content_length(session));
    LOCK_TEST_AND_SET
    do {
        result = g_api->read_req_body(session,mtpostBuffer,recvSize);
        if (result > 0) {
            LSM_DBG((&MNAME), session,"In mtpost, Read %d bytes (requested %d bytes)\n", result, recvSize);
            totalCount += result;
        }
        if (result == -1) {
            LSM_ERR((&MNAME), session, "FATAL ERROR IN READ OF BODY!\n");
            UNLOCK_TEST_AND_SET
            return(-1);
        }
    } while (result);
    if (totalCount >= g_api->get_req_content_length(session)) {
        sendResponse(session);
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


