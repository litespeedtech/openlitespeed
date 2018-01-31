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
*    This module is intented to be used in conjunction with the corresponding*
*    program POSTtest2 to pound George's multi-threading code to death and it*
*    does it by running 2 threads, a send thread and a receive thread.  And  *
*    they both have a buffer size and expected size.  So they run in parallel*
*    and the receive thread checks the data against what it expects.  And    *
*    fails ugly if it doesn't match.                                         *
*****************************************************************************/
#include "../include/ls.h"
#include "../include/lsr/ls_str.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <sys/syscall.h>
 
#define MNAME       mtpostmt2
lsi_module_t MNAME;

volatile int s_lock = 0;
#define LOCK_TEST_AND_SET   while (__sync_lock_test_and_set(&s_lock,1)) while (s_lock);
#define UNLOCK_TEST_AND_SET __sync_lock_release(&s_lock);
typedef struct session_list {
    lsi_session_t       *   m_plsi_session;    
    int                     m_RecvCount;
    int                     m_SendCount;
    int                     m_SendSize;
    int                     m_RecvSize;
    int                     m_RecvContentLength;
    char                *   m_pmtRecvBuffer;
    char                *   m_pmtSendBuffer;
    struct session_list *   m_psession_listNext;
} session_list_t_;
static session_list_t_ *s_psession_list = NULL;

static char *tid_string(char chTid[])
{
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    if (syscall(SYS_gettid) == getpid()) {
        sprintf(chTid, "Main thread");
    }
    else {
        sprintf(chTid, "Secondary thread: %d", (int)syscall(SYS_gettid));
    }
    return(chTid);
#else
    return "";
#endif
}


static int init_module(lsi_module_t *module) 
{
    char chTid[80];
    LSM_DBG((&MNAME),
            NULL,
            "Module initialization, %s\n",
            tid_string(chTid));
    module->about = LSIAPI_VERSION_STRING;
    return(0);
}

static int l4_begin_session(lsi_param_t *pLSI_Param) 
{
    char chTid[80];
    LSM_DBG((&MNAME),
             pLSI_Param->session,
             "L4_BEGINSESSION hook %s\n",
             tid_string(chTid)
           );
    return(0);
}


static int httpinit(lsi_param_t *pLSI_Param)
{
    int aEnableHkpts[] = {LSI_HKPT_HTTP_AUTH, LSI_HKPT_HTTP_END};
    int iRc;
    char chTid[80];
    iRc = g_api->enable_hook(pLSI_Param->session, &MNAME, 1,
                             aEnableHkpts, 2);
    LSM_DBG((&MNAME),
            pLSI_Param->session,
            "HTTP_BEGIN hook and enable AUTH and END hooks as well (rc: %d) %s\n", 
            iRc,
            tid_string(chTid)
           );
    return 0;
}


static int httpauth(lsi_param_t *pLSI_Param)
{
    char chTid[80];
    LSM_DBG((&MNAME),
            pLSI_Param->session,
            "HTTP_AUTH hook %s\n",
            tid_string(chTid)
           );
    return 0;
}


static int httpend(lsi_param_t *pLSI_Param)
{
    char chTid[80];
    LSM_DBG((&MNAME),
            pLSI_Param->session,
            "HTTP_END hook %s\n",
            tid_string(chTid)
           );
    return 0;
}


static int maininit(lsi_param_t *pLSI_Param)
{
    char chTid[80];
    LSM_DBG((&MNAME),
            pLSI_Param->session,
            "MAIN_INIT hook %s\n",
            tid_string(chTid)
           );
    return 0;
}


static int mainprefork(lsi_param_t *pLSI_Param)
{
    char chTid[80];
    LSM_DBG((&MNAME),
            pLSI_Param->session,
            "MAIN_PREFORK hook %s\n",
            tid_string(chTid)
           );
    return 0;
}


static int mainpostfork(lsi_param_t *pLSI_Param)
{
    char chTid[80];
    LSM_DBG((&MNAME),
            pLSI_Param->session,
            "MAIN_POSTFORK hook %s\n",
            tid_string(chTid)
           );
    return 0;
}


static int workerinit(lsi_param_t *pLSI_Param)
{
    char chTid[80];
    LSM_DBG((&MNAME),
            pLSI_Param->session,
            "WORKER_INIT hook %s\n",
            tid_string(chTid)
           );
    return 0;
}


static int workeratexit(lsi_param_t *pLSI_Param)
{
    char chTid[80];
    LSM_DBG((&MNAME),
            pLSI_Param->session,
            "WORKER_ATEXIT hook %s\n",
            tid_string(chTid)
           );
    return 0;
}


static int rcvd_req_header_cbf(lsi_param_t *param)
{
    char chTid[80];
    LSM_DBG((&MNAME),
            param->session,
            "RECV_REQ_HEADER hook, length: %d %s\n", 
            param->len1,
            tid_string(chTid)
           );
    return LSI_OK;
}


static int l4recv1(lsi_param_t *param)
{
    int next;
    char chTid[80];
    next = g_api->stream_read_next(param, (void *)param->ptr1, param->len1);
    LSM_DBG((&MNAME),
            param->session,
            "L4_RECVING hook, length: %d, rc: %d, hook flag: %d, hook level: %d %s\n", 
            param->len1, 
            next,
            g_api->get_hook_flag(param->session,(int)LSI_HKPT_L4_RECVING),
            g_api->get_hook_level(param),
            tid_string(chTid)
           );
    if ((param->len1) && (param->ptr1)) {
        LSM_DBG((&MNAME),
                param->session,
                "   first byte of data: 0x%x\n", *(char *)param->ptr1);
    }      
    return next;
}


/** You must lock the session list before calling **/
/*
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
*/

/** You must NOT lock the sessioni list before calling **/
static void session_listFree(const lsi_session_t    *plsi_session,
                             session_list_t_        *psession_list) {
    LOCK_TEST_AND_SET
    LSM_DBG((&MNAME), 
            plsi_session,
            "Freeing session: %p, %p (in struct)\n", 
            plsi_session, 
            psession_list->m_plsi_session);
    if (!s_psession_list) {
        g_api->set_status_code(plsi_session, 500);
        LSM_ERR((&MNAME), 
                plsi_session,
                "ERROR!  Attempt to free %p from an empty psession list!\n", 
                plsi_session);
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
    if (psession_list->m_pmtRecvBuffer) {
        free(psession_list->m_pmtRecvBuffer);
        psession_list->m_pmtRecvBuffer = NULL;
    }
    if (psession_list->m_pmtSendBuffer) {
        free(psession_list->m_pmtSendBuffer);
        psession_list->m_pmtSendBuffer = NULL;
    }
    free(psession_list);
    UNLOCK_TEST_AND_SET
}

        
static void displayInfo(session_list_t_ *psession_list) 
{
    char chUriOriginal[2048] = { 0 };
    int  cookieLength = 2048;
    int  uriLength = 2048;
    int  ipLength = 2048;
    int  queryLength = 2048;
    char chUriFilePath[2048];
    int  iUriFilePath = sizeof(chUriFilePath);
    g_api->get_req_org_uri(psession_list->m_plsi_session,chUriOriginal,sizeof(chUriOriginal));
    g_api->get_uri_file_path(psession_list->m_plsi_session,chUriOriginal,strlen(chUriOriginal),chUriFilePath,iUriFilePath);
    LSM_DBG((&MNAME),
            psession_list->m_plsi_session,
            "Original uri: %s, uri: %s, context uri: %s, req handler registered: %s \n"
            "Remove unknown timer: %d, Cookies: %s, Cookie count: %d, Client IP: %s \n"
            "Query string: %s, Uri path: %s Status so far: %d, Response zipped: %d \n"
            "Response header count: %d, is_resp_headers_sent: %d\n",
            chUriOriginal,
            g_api->get_req_uri(psession_list->m_plsi_session,NULL),
            g_api->get_mapped_context_uri(psession_list->m_plsi_session,&uriLength),
            (g_api->is_req_handler_registered(psession_list->m_plsi_session) ? "Yes" : "No"),
            g_api->remove_timer(1234),
            g_api->get_req_cookies(psession_list->m_plsi_session, &cookieLength),
            g_api->get_req_cookie_count(psession_list->m_plsi_session),
            g_api->get_client_ip(psession_list->m_plsi_session,&ipLength),
            g_api->get_req_query_string(psession_list->m_plsi_session,&queryLength),
            chUriFilePath,
            g_api->get_status_code(psession_list->m_plsi_session),
            g_api->is_resp_buffer_gzippped(psession_list->m_plsi_session),
            g_api->get_resp_headers_count(psession_list->m_plsi_session),
            g_api->is_resp_headers_sent(psession_list->m_plsi_session)
           );
}



static void *sendThread(void *pvsession_list) {
    session_list_t_ *psession_list = (session_list_t_ *)pvsession_list;
    int i;
    int bOk = 1;
    int iBlock = 0;
    LSM_DBG((&MNAME), 
            psession_list->m_plsi_session, 
            "Entering mtpostmt2, send thread session %p, ServerRoot: %s, "
            "Module data: %s, req_vhost: %s, hdr len: %d, hdr count: %d\n", 
            psession_list->m_plsi_session, 
            g_api->get_server_root(), 
            g_api->get_module_data(psession_list->m_plsi_session, (&MNAME), LSI_DATA_HTTP) ? "Yes" : "No", 
            g_api->get_req_vhost(psession_list->m_plsi_session) ? "YES" : "NO",
            g_api->get_req_raw_headers_length(psession_list->m_plsi_session),
            g_api->get_req_headers_count(psession_list->m_plsi_session));
    displayInfo(psession_list);
    g_api->set_resp_content_length(psession_list->m_plsi_session, -1); // Use chunk encoding (it would anyway)
    // For sending, I own the send buffer, so just initialize it the way I want and start pounding
    for (i = 0; i < psession_list->m_SendSize; i++) {
        psession_list->m_pmtSendBuffer[i] = (char)('a' + (i % 26));
    }
    // No need to lock everything as we are the only sender (for now)
    while (psession_list->m_SendCount < psession_list->m_RecvContentLength) {
        int iSendSize;
        char chBlock[80];
        iBlock++;
        sprintf(chBlock,"%08d",iBlock);
        LSM_DBG((&MNAME), psession_list->m_plsi_session, "Send block: %s\n", chBlock);
        if (psession_list->m_SendCount + psession_list->m_SendSize <= psession_list->m_RecvContentLength) {
            iSendSize = psession_list->m_SendSize;
        }
        else {
            iSendSize = psession_list->m_RecvContentLength - psession_list->m_SendCount;
        }
        if (iSendSize >= 8) {
            memcpy(psession_list->m_pmtSendBuffer,chBlock,(iSendSize > 8) ? 8 : iSendSize);
        }
        else {
            memset(psession_list->m_pmtSendBuffer,'0',iSendSize);
        }
        if (g_api->append_resp_body(psession_list->m_plsi_session,
                                    psession_list->m_pmtSendBuffer,
                                    iSendSize) == -1) {
            LSM_ERR((&MNAME), psession_list->m_plsi_session,"Error in appending to response body session: %p\n", psession_list->m_plsi_session);
            bOk = 0;
            break;
        }
        psession_list->m_SendCount += iSendSize;
    }
    if (bOk) {
        /* Take out this requirement for now.  Doesn't seem to do what I think.  */
        //if (g_api->is_req_body_finished(psession_list->m_plsi_session)) {
        //    LSM_ERR((&MNAME), psession_list->m_plsi_session,"ERROR!  Expected Request body finished and it's not! Session: %p\n", psession_list->m_plsi_session);
        //    bOk = 0;
        //}
    }
    LSM_DBG((&MNAME), psession_list->m_plsi_session, "Ending mtpostmt2, send thread session %p %s\n", psession_list->m_plsi_session, bOk ? "OK" : "NOT OK");
    return(bOk ? psession_list : NULL); /* XXX: Return value does not get collected */
}


static void reportMismatchedReceive(char            *pchCompare, 
                                    session_list_t_ *psession_list, 
                                    int              iBlock,
                                    int              iRecvSize) {
    char *pchCompare1 = pchCompare;
    char *pchRecvBuffer1 = psession_list->m_pmtRecvBuffer;
    int  iCounter = 0;
    int  bMismatched = 0;
    
    while (iCounter < iRecvSize) {
        if ((*pchCompare1) != (*pchRecvBuffer1)) {
            bMismatched = 1;
            break;
        }
        iCounter++;
        pchCompare1++;
        pchRecvBuffer1++;
    }
    if (bMismatched) {
        LSM_ERR((&MNAME), 
                psession_list->m_plsi_session, 
                "ERROR!  FATAL COMPARE ERROR READING REQ BODY, block #%d compare started at: %d of %d, session: %p - mismatched at byte #%d in buffer '%c'!='%c'!\n", 
                iBlock, 
                psession_list->m_RecvCount, 
                psession_list->m_RecvContentLength,
                psession_list->m_plsi_session, 
                iCounter, 
                *pchRecvBuffer1, 
                *pchCompare1);
    }
    else {
        LSM_ERR((&MNAME), 
                psession_list->m_plsi_session, 
                "ERROR!  FATAL COMPARE ERROR READING REQ BODY, block #%d compare started at: %d, session: %p - can't find mismatched char!\n", 
                iBlock, 
                psession_list->m_RecvCount, 
                psession_list->m_plsi_session);
    }
}


static void *recvThread(void *pvsession_list) {
    session_list_t_ *psession_list = (session_list_t_ *)pvsession_list;
    int i;
    int bOk = 1;
    char *pchCompare = NULL;
    int iBlock = 0;
    LSM_DBG((&MNAME), 
            psession_list->m_plsi_session, 
            "Entering mtpostmt2, recv thread session %p, recvSize: %d, Count: %d, "
            "ContentLength: %d, ServerRoot: %s, module data: %s, req_vhost: %s "
            ", hdr len: %d, hdr count: %d\n", 
            psession_list->m_plsi_session, 
            psession_list->m_RecvSize, 
            psession_list->m_RecvCount, 
            psession_list->m_RecvContentLength,
            g_api->get_server_root(),
            g_api->get_module_data(psession_list->m_plsi_session, (&MNAME), LSI_DATA_HTTP) ? "Yes" : "No",
            g_api->get_req_vhost(psession_list->m_plsi_session) ? "YES" : "NO",
            g_api->get_req_raw_headers_length(psession_list->m_plsi_session),
            g_api->get_req_headers_count(psession_list->m_plsi_session));
    displayInfo(psession_list);
    // For receiving, I need a separate buffer to compare against
    pchCompare = malloc(psession_list->m_RecvSize);
    if (!pchCompare) {
        LSM_ERR((&MNAME), 
                psession_list->m_plsi_session,
                "ERROR!  FATAL ERROR CREATING COMPARE BUFFER, session: %p\n", 
                psession_list->m_plsi_session);
        bOk = 0;
    }
    if (bOk) {
        for (i = 0; i < psession_list->m_RecvSize; i++) {
            pchCompare[i] = (char)('A' + (i % 26));
        }
        while (psession_list->m_RecvCount < psession_list->m_RecvContentLength) {
            int iRecvSize;
            int bDoBreak = 0;
            int iInBlock = 0;
            if (psession_list->m_RecvCount + psession_list->m_RecvSize <= psession_list->m_RecvContentLength) {
                iRecvSize = psession_list->m_RecvSize;
            }
            else {
                iRecvSize = psession_list->m_RecvContentLength - psession_list->m_RecvCount;
            }
            while (iInBlock < iRecvSize) {
                int iResult;
                iResult = g_api->read_req_body(psession_list->m_plsi_session,
                                               &psession_list->m_pmtRecvBuffer[iInBlock],
                                               iRecvSize - iInBlock);
                if (iResult > 0) {
                    iInBlock += iResult;
                }
                else if (iResult == 0) {
                    // Keep receiving?
                    LSM_DBG((&MNAME), 
                            psession_list->m_plsi_session, 
                            "ERROR!  FATAL ERROR - In recv thread, received 0 bytes session %p\n", 
                            psession_list->m_plsi_session);
                    bDoBreak = 1;
                    break;
                }
                else {
                    LSM_ERR((&MNAME), psession_list->m_plsi_session, "ERROR!  FATAL ERROR READING REQ BODY, session: %p\n", psession_list->m_plsi_session);
                    bOk = 0;
                    bDoBreak = 1;
                    break;
                }
            }
            if (bDoBreak) {
                break;
            }
            if (bOk) {
                char chBlock[80];
                iBlock++; // Block counters start at 1!
                sprintf(chBlock,"%08d",iBlock);
                LSM_DBG((&MNAME), 
                        psession_list->m_plsi_session, 
                        "Expecting recv block %s\n", 
                        chBlock);
                
                memcpy(pchCompare,chBlock,(8 < iRecvSize) ? 8 : iRecvSize);
                if (!(memcmp(pchCompare, psession_list->m_pmtRecvBuffer, iRecvSize))) {
                    psession_list->m_RecvCount += iRecvSize;
                }
                else {
                    /* Find the actual bytes that are corrupted.  */
                    reportMismatchedReceive(pchCompare, psession_list, iBlock, iRecvSize);
                    bOk = 0;
                }                    
            }
            if (!bOk) {
                break;
            }
        } 
        free(pchCompare);
    }
    LSM_DBG((&MNAME), psession_list->m_plsi_session, "Ending mtpostmt2, recv thread session %p %s\n", psession_list->m_plsi_session, bOk ? "OK" : "NOT OK");
    return bOk ? psession_list : NULL; /* XXX: Return value does not get collected */
}

static int startThreads(session_list_t_     *psession_list) {
    pthread_attr_t pthread_attr1;
    pthread_t      pthread1;
    pthread_attr_t pthread_attr2;
    pthread_t      pthread2;
    int            bOk = 1;

    LSM_DBG((&MNAME), psession_list->m_plsi_session,"Entering startThreads session %p\n", psession_list->m_plsi_session);
    if (pthread_attr_init(&pthread_attr1)) {
        LSM_ERR((&MNAME), psession_list->m_plsi_session,"Error in creating attr init: %s, session: %p\n", strerror(errno), psession_list->m_plsi_session);
        bOk = 0;        
    }
    else {
        if (pthread_create(&pthread1,&pthread_attr1,recvThread,psession_list) < 0) {
            LSM_ERR((&MNAME), psession_list->m_plsi_session,"Error in creating recv thread %s, session: %p\n", strerror(errno), psession_list->m_plsi_session);
            bOk = 0;
        }
        else {
            if (pthread_attr_init(&pthread_attr2)) {
                LSM_ERR((&MNAME), psession_list->m_plsi_session,"Error in creating attr init2: %s, session: %p\n", strerror(errno), psession_list->m_plsi_session);
                bOk = 0;
            }
            else {
                if (pthread_create(&pthread2,&pthread_attr2,sendThread,psession_list) < 0) {
                    LSM_ERR((&MNAME), psession_list->m_plsi_session,"Error in creating send thread %s, session: %p\n", strerror(errno), psession_list->m_plsi_session);
                    bOk = 0;
                }
                else {
                    if (pthread_join(pthread2,NULL)) {
                        LSM_ERR((&MNAME), psession_list->m_plsi_session,"Error in waiting for send thread %s, session: %p\n", strerror(errno), psession_list->m_plsi_session);
                        bOk = 0;
                    }
                    //LSM_ERR((&MNAME), psession_list->m_plsi_session, "NOT AN ERROR!  Done with sendThread!  session: %p\n", psession_list->m_plsi_session);
                }
                pthread_attr_destroy(&pthread_attr2);
            }
            if (pthread_join(pthread1,NULL)) {
                LSM_ERR((&MNAME), psession_list->m_plsi_session,"Error in waiting for recv thread %s, session: %p\n", strerror(errno), psession_list->m_plsi_session);
                bOk = 0;
            }
            //LSM_ERR((&MNAME), psession_list->m_plsi_session, "NOT AN ERROR!  Done with recvThread!  session: %p\n", psession_list->m_plsi_session);
        }
        pthread_attr_destroy(&pthread_attr1);
    }
    if (!bOk) {
        LSM_DBG((&MNAME), psession_list->m_plsi_session,"Exiting startThreads with error, session %p\n", psession_list->m_plsi_session);
        g_api->set_status_code(psession_list->m_plsi_session, 500);
        g_api->end_resp(psession_list->m_plsi_session);
        return(-1);
    }
    LSM_DBG((&MNAME), psession_list->m_plsi_session,"Exiting startThreads no error, session %p\n", psession_list->m_plsi_session);
    g_api->set_status_code(psession_list->m_plsi_session, 200);
    g_api->end_resp(psession_list->m_plsi_session);
    return(0);
}


static int cleanUpReportedSessionError(session_list_t_ *psession_list,
                                       int             iReturnCode) {
    g_api->set_status_code(psession_list->m_plsi_session, iReturnCode);
    g_api->end_resp(psession_list->m_plsi_session);
    if (psession_list->m_pmtRecvBuffer) {
        free(psession_list->m_pmtRecvBuffer);
        psession_list->m_pmtRecvBuffer = NULL;
    }
    if (psession_list->m_pmtSendBuffer) {
        free(psession_list->m_pmtSendBuffer);
        psession_list->m_pmtSendBuffer = NULL;
    }
    free(psession_list);
    return(iReturnCode);
}


static int process_req(const lsi_session_t *plsi_session)
{
    session_list_t_ *psession_list = NULL;
    int sendSize = 0;
    int recvSize = 0;
    int numArgs = 0;
    int arg = 0;
    int bOk = 1;
    int bSetSend = 0;
    int bSetRecv = 0;
    int64_t receiveSize64;
    pid_t pid = 0;
    
    LSM_DBG((&MNAME), plsi_session,"Entering mtpostmt2, process_req session %p, HTTP_BEGIN_HOOK: %d\n", plsi_session, g_api->get_hook_flag(plsi_session, LSI_HKPT_L4_BEGINSESSION));
    g_api->parse_req_args(plsi_session,0,0,NULL,0);
    numArgs = g_api->get_qs_args_count(plsi_session);
    LSM_DBG((&MNAME), plsi_session,"Entering mtpostmt2, args_count = %d, session %p\n", numArgs, plsi_session);
    if (numArgs < 4) {
        LSM_ERR((&MNAME), plsi_session,"FATAL ERROR - expected 4 args, only got %d, session %p\n", numArgs, plsi_session);
        bOk = 0;
    }      
    else for (arg = 0; arg < numArgs; arg++) {
        ls_strpair_t ls_strpairArg;
        char chTitle[1024];
        char chValue[1024];
        if (g_api->get_qs_arg_by_idx(plsi_session, arg, &ls_strpairArg) < 0) {
            LSM_ERR((&MNAME), plsi_session,"ERROR!  Can't get qs args!\n");
            bOk = 0;
            break;
        }
        if ((ls_strpairArg.key.len >= sizeof(chTitle)) ||
            (ls_strpairArg.val.len >= sizeof(chValue))) {
            LSM_ERR((&MNAME), plsi_session,"ERROR!  Can't get qs long sizes!\n");
            bOk = 0;
            break;
        }
        memcpy(chTitle,ls_strpairArg.key.ptr,ls_strpairArg.key.len);
        chTitle[ls_strpairArg.key.len] = 0;
        memcpy(chValue,ls_strpairArg.val.ptr,ls_strpairArg.val.len);
        chValue[ls_strpairArg.val.len] = 0;
        LSM_DBG((&MNAME), plsi_session,"arg[%d]: %s=%s session: %p\n", arg, chTitle, chValue, plsi_session);
        if (!strcmp(chTitle,"sendBuffer")) {
            LSM_DBG((&MNAME), plsi_session,"sendSize = %s, session %p\n", chValue, plsi_session);
            sendSize = atoi(chValue);
            bSetSend = 1;
        }
        else if (!strcmp(chTitle,"recvBuffer")) {
            LSM_DBG((&MNAME), plsi_session,"recvSize = %s, session %p\n", chValue, plsi_session);
            recvSize = atoi(chValue);
            bSetRecv = 1;
        }
        else if (!strcmp(chTitle,"pid")) {
            LSM_DBG((&MNAME), plsi_session, "pid = %s, session %p\n", chValue, plsi_session);
            pid = atoi(chValue);
        }
        else if (!strcmp(chTitle,"testNumber")) {
            int iTestNumber;
            LSM_DBG((&MNAME), plsi_session,"testNumber = %s, session %p\n", chValue, plsi_session);
            iTestNumber = atoi(chValue);
            if (pid == 0) {
                LSM_ERR((&MNAME), plsi_session,"FATAL ERROR - 0 PID (or not set), session %p\n", plsi_session);
                bOk = 0;
            }
            else {
                char chPIDTestFile[80];
                char chTest[256];
                int  iTestNumberFile = -1;
                FILE *pFILE = NULL;
                sprintf(chPIDTestFile,"pidTest.%d.pid",pid);
                pFILE = fopen(chPIDTestFile,(iTestNumber == 0) ? "w" : "r+");
                LSM_DBG((&MNAME), plsi_session,"Test file: %s, session %p\n", chPIDTestFile, plsi_session);
                if (!pFILE) {
                    LSM_ERR((&MNAME), plsi_session,"FATAL ERROR - opening %s: %s\n",chPIDTestFile, strerror(errno));
                    bOk = 0;
                }
                else if (iTestNumber > 0) {
                    if (!fgets(chTest,sizeof(chTest),pFILE)) {
                        LSM_ERR((&MNAME), plsi_session,"FATAL ERROR - reading PID file %s: %s\n",chPIDTestFile, strerror(errno));
                        bOk = 0;
                    }
                    else {
                        iTestNumberFile = atoi(chTest);
                        LSM_DBG((&MNAME), plsi_session,"Read from file, test number: %s %d session %p\n", chTest, iTestNumberFile, plsi_session);
                        if (iTestNumberFile + 1 != iTestNumber) {
                            LSM_ERR((&MNAME), plsi_session,"FATAL ERROR - unexpected test number %d, last test %d, session %p\n", iTestNumber, iTestNumberFile, plsi_session);
                            bOk = 0;
                        }
                        else {
                            if (fseek(pFILE, 0, SEEK_SET) == -1) {
                                LSM_ERR((&MNAME), plsi_session,"FATAL ERROR - Reading PID file %s, session %p\n", strerror(errno), plsi_session);
                                bOk = 0;
                            }
                        }
                    }
                }
                if (bOk) {
                    fprintf(pFILE, "%d", iTestNumber);
                    if (ferror(pFILE)) {
                        LSM_ERR((&MNAME), plsi_session,"FATAL ERROR - writing test number: %s, session %p\n", strerror(errno), plsi_session);
                        bOk = 0;
                    }
                }    
                if (pFILE) {
                    fclose(pFILE);
                }
                pFILE = NULL;
            }
        }
    }
    if ((bOk) && ((!bSetSend) || (!bSetRecv))) {
        LSM_ERR((&MNAME), plsi_session,"ERROR!  sendBuffer or recvBuffer not set - fatal, session: %p!\n",plsi_session);
        bOk = 0;
    }
    if (!bOk) {
        g_api->set_status_code(plsi_session, 500);
        g_api->end_resp(plsi_session);
        return(500);
    }
    LSM_DBG((&MNAME), plsi_session,"Use recvSize = %d, session %p\n", recvSize, plsi_session);
    LSM_DBG((&MNAME), plsi_session,"Use sendSize = %d, session %p\n", sendSize, plsi_session);
    
    psession_list = malloc(sizeof(session_list_t_));
    if (!psession_list) {
        g_api->set_status_code(plsi_session, 507);
        LSM_ERR((&MNAME), plsi_session,"ERROR!  Insufficient memory to allocate session size %d bytes, session: %p\n", (int)sizeof(session_list_t_), plsi_session);
        g_api->end_resp(plsi_session);
        return(507);
    }
    memset(psession_list,0,sizeof(session_list_t_));
    psession_list->m_RecvSize = recvSize;
    psession_list->m_SendSize = sendSize;
    psession_list->m_plsi_session = (lsi_session_t *)plsi_session;
    psession_list->m_pmtRecvBuffer = malloc(recvSize);
    psession_list->m_pmtSendBuffer = malloc(sendSize);
    if ((!psession_list->m_pmtRecvBuffer) ||
        (!psession_list->m_pmtSendBuffer)) {
        LSM_ERR((&MNAME), plsi_session,"ERROR!  Insufficient memory to allocate %d, %d bytes for data, session: %p\n", psession_list->m_RecvSize, psession_list->m_SendSize, plsi_session);
        return cleanUpReportedSessionError(psession_list,507);
    }
    receiveSize64 = g_api->get_req_content_length(plsi_session);
    if (psession_list->m_RecvContentLength < 0) {
        LSM_ERR((&MNAME), plsi_session,"ERROR!  Chunked receives not supported for this implementation, session: %p\n", plsi_session);
        return cleanUpReportedSessionError(psession_list, 500);
    }
    if (receiveSize64 > ((1ll << (sizeof(int) * 8ll - 1ll)) - 1ll)) {
        LSM_ERR((&MNAME), plsi_session,"ERROR!  Receives over 2GB not supported for this implementation, session: %p\n", plsi_session);
        return cleanUpReportedSessionError(psession_list, 500);
    }
    psession_list->m_RecvContentLength = (int)receiveSize64;
    LSM_DBG((&MNAME), plsi_session,"RecvContentLength: %d, session %p\n", psession_list->m_RecvContentLength, plsi_session);
    LOCK_TEST_AND_SET
    psession_list->m_psession_listNext = s_psession_list;
    s_psession_list = psession_list;
    UNLOCK_TEST_AND_SET
    if (startThreads(psession_list) == -1) {
        // Error already reported.
        bOk = 0;
    }
    //LSM_ERR((&MNAME), plsi_session,"NOT AN ERROR!  Done with ALL threads!  session: %p\n", plsi_session);
    session_listFree(plsi_session, psession_list);
    return bOk ? 0 : -1;
}

static lsi_serverhook_t serverHooks[] =
{
    { LSI_HKPT_L4_BEGINSESSION, l4_begin_session, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    { LSI_HKPT_HTTP_BEGIN, httpinit, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    { LSI_HKPT_HTTP_AUTH, httpauth, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    { LSI_HKPT_MAIN_INITED, maininit, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    { LSI_HKPT_MAIN_PREFORK, mainprefork, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    { LSI_HKPT_MAIN_POSTFORK, mainpostfork, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    { LSI_HKPT_WORKER_INIT, workerinit, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    { LSI_HKPT_WORKER_ATEXIT, workeratexit, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    { LSI_HKPT_HTTP_END, httpend, LSI_HOOK_NORMAL, 0 },
    { LSI_HKPT_RCVD_REQ_HEADER, rcvd_req_header_cbf, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED },
    //{ LSI_HKPT_L4_RECVING, l4recv1, LSI_HOOK_EARLY, LSI_FLAG_TRANSFORM | LSI_FLAG_ENABLED },
      LSI_HOOK_END   //Must put this at the end position
};


/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_reqhdlr_t myhandler = { process_req, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init_module, &myhandler, NULL, "mtpostmt2", serverHooks, {0} };


