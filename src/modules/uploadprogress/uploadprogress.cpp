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
#include <string.h>
#include <stdlib.h>

#define MNAME               uploadprogress
#define ModuleNameStr       "mod-uploadprogress"
#define MOD_QS              "X-Progress-ID="
#define MOD_QS_LEN          (sizeof(MOD_QS) -1)
#define MAX_BUF_LENG        20
#define EXPIRE_TIME         (30 * 1000)

lsi_shmhash_t *pShmHash = NULL;
extern lsi_module_t MNAME;

enum
{
    UPLOAD_START = 0,
    UPLOAD_ERROR,
    UPLOAD_UPLOADING,
    UPLOAD_DONE,
};

enum HTTPMETHOD
{
    METHOD_UNKNOWN = 0,
    METHOD_GET,
    METHOD_POST,
};

typedef struct _MyMData
{
    char      * pBuffer;
    char      * pProgressID;  //This need to be malloc when POST, and free in timerCB
    int         iWholeLength;
    int         iFinishedLength;
} MyMData;


static int releaseMData(void *data)
{
    MyMData *myData = (MyMData *)data;
    if (myData)
        delete myData;
    return 0;
}


static void removeShmEntry(void *progressID)
{
    g_api->shm_htable_delete(pShmHash, (const uint8_t *)progressID,
                             strlen((char *)progressID));
    free((char *)progressID);
}


static int releaseModuleData(lsi_cb_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->_session, &MNAME,
                      LSI_MODULE_DATA_HTTP);
    if (myData)
    {
        /**
         * Set a timer to clean the shm data in 30 seconds
         */
        g_api->set_timer(EXPIRE_TIME, 0, removeShmEntry, myData->pProgressID);
        g_api->free_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP,
                                releaseMData);
        g_api->log(rec->_session, LSI_LOG_DEBUG, "[%s]releaseModuleData.\n",
                   ModuleNameStr);
    }
    return 0;
}


static int setProgress(MyMData *pData)
{
    snprintf(pData->pBuffer, MAX_BUF_LENG, "%X:%X", 
             pData->iWholeLength, pData->iFinishedLength);
    return 0;
}

static const char *getProgressId(lsi_session_t *session, int &idLen)
{
    const char *pQS = g_api->get_req_query_string(session, &idLen);
    if (!pQS || strncasecmp(MOD_QS, pQS, MOD_QS_LEN) != 0 || (size_t)idLen <= MOD_QS_LEN)
        return NULL;

    idLen -= MOD_QS_LEN;
    return (pQS + MOD_QS_LEN);
}

static lsi_hash_key_t hashBuf(const void *__s, int len)
{
    lsi_hash_key_t __h = 0;
    const uint8_t *p = (const uint8_t *)__s;
    while (--len >= 0)
    {
        __h = __h * 37 + (const uint8_t )*p;
        ++p;
    }
    return __h;
}

static int  compBuf(const void *pVal1, const void *pVal2, int len)
{
    return memcmp((const char *)pVal1, (const char *)pVal2, len);
}

static int _init(lsi_module_t *module)
{
    lsi_shmpool_t *pShmPool = g_api->shm_pool_init("moduploadp", 0);
    if ( pShmPool == NULL)
    {
        g_api->log(NULL, LSI_LOG_ERROR, "shm_pool_init return NULL, quit.\n");
        return LS_FAIL;
    }
    pShmHash = g_api->shm_htable_init(pShmPool, NULL, 0, hashBuf, compBuf);
    if ( pShmHash == NULL )
    {
        g_api->log(NULL, LSI_LOG_ERROR, "shm_htable_init return NULL, quit.\n");
        return LS_FAIL;
    }

    g_api->init_module_data(module, releaseMData, LSI_MODULE_DATA_HTTP);
    return LS_OK;
}


static int reqBodyRead(lsi_cb_param_t *rec)
{
    MyMData *myData = (MyMData *)g_api->get_module_data(rec->_session, &MNAME,
                      LSI_MODULE_DATA_HTTP);
    int len = g_api->stream_read_next(rec, (char *)rec->_param, rec->_param_len);
    myData->iFinishedLength += len;
    setProgress(myData);
    return len;
}


/**
 * Check if is a uploading, we will handle the below case:
 * POST(GET) /xxxxxxxx?X-Progress-ID=xxxxxxxxxxxxxxxxxxxxxxxxxxxx
 */
static int checkReqHeader(lsi_cb_param_t *rec)
{
    int idLen;
    const char *progressID = getProgressId(rec->_session, idLen);
    int contentLength = g_api->get_req_content_length(rec->_session);
    if (progressID && contentLength <= 0)
    {
        //GET, must disable cache module
        g_api->set_req_env(rec->_session, "cache-control", 13, "no-cache", 8);
        return 0;
    }
    if (!progressID || contentLength <= 0)
        return 0;

    char buf[MAX_BUF_LENG], *pBuffer;
    sprintf(buf, "%X:0", contentLength);
    lsi_shm_off_t offset = g_api->shm_htable_add(pShmHash,
        (const uint8_t *)progressID, idLen, (const uint8_t *)buf, MAX_BUF_LENG);
    pBuffer = (char *)g_api->shm_htable_off2ptr(pShmHash, offset);
    if (!offset || !pBuffer)
    {
        g_api->log(rec->_session, LSI_LOG_ERROR,
                   "[%s]checkReqHeader can't add shm entry.\n", ModuleNameStr);
        return 0;
    }
    
    MyMData *myData = (MyMData *) g_api->get_module_data(rec->_session, &MNAME,
                      LSI_MODULE_DATA_HTTP);
    if (!myData)
    {
        myData = new MyMData;
        if (!myData)
        {
            g_api->log(rec->_session, LSI_LOG_ERROR,
                   "[%s]checkReqHeader out of memory.\n", ModuleNameStr);
            return 0;
        }
        memset(myData, 0, sizeof(MyMData));
    }

    myData->pProgressID = strndup(progressID, idLen);
    myData->iWholeLength = contentLength; 
    myData->iFinishedLength = 0;
    myData->pBuffer = pBuffer;
    g_api->set_module_data(rec->_session, &MNAME, LSI_MODULE_DATA_HTTP,
                           (void *)myData);

    int aEnableHkpt[] = {LSI_HKPT_RECV_REQ_BODY, LSI_HKPT_HTTP_END };
    g_api->set_session_hook_enable_flag(rec->_session, &MNAME, 1, aEnableHkpt,
                                        sizeof(aEnableHkpt) / sizeof(int));
    return LSI_HK_RET_OK;
}


static int getState(int iWholeLength, int iFinishedLength)
{
    if (iWholeLength <= 0)
        return UPLOAD_ERROR;
    else if (iFinishedLength >= iWholeLength)
        return UPLOAD_DONE;
    else if (iFinishedLength == 0)
        return UPLOAD_START;
    else 
        return UPLOAD_UPLOADING;
}


/**
 * We will handle the below case:
 * GET /progress?X-Progress-ID=xxxxxxxxxxxxxxxxxxxxxxxxxxxx
 * Otherwise return error 40X
 */
static int begin_process(lsi_session_t *session)
{
    int idLen;
    const char *progressID = getProgressId(session, idLen);
    if (!progressID)
        return 400; //Bad Request

    int valLen;
    lsi_shm_off_t offset = g_api->shm_htable_find(pShmHash,
                                                  (const uint8_t *)progressID, 
                                                  idLen, &valLen);
    if (offset == 0 || valLen <= 2 ) //At least 3 bytes
    {
        g_api->log(session, LSI_LOG_ERROR,
                   "[%s]begin_process error, can't find shm entry .\n", ModuleNameStr);
        return 500;
    }
    
    char *p = (char *)g_api->shm_htable_off2ptr(pShmHash, offset);
    int iWholeLength, iFinishedLength;
    sscanf(p, "%X:%X", &iWholeLength, &iFinishedLength);
    int state = getState(iWholeLength, iFinishedLength);

    char buf[100] = {0}; //enough
    g_api->set_resp_header(session, LSI_RESP_HEADER_CONTENT_TYPE, NULL, 0,
                           "application/json", 16, LSI_HEADER_SET);
    if (state == UPLOAD_ERROR)
        strcpy(buf, "{ \"state\" : \"error\", \"status\" : 500 }\r\n");
    else if (state == UPLOAD_START)
        strcpy(buf, "{ \"state\" : \"starting\" }\r\n");
    else if (state == UPLOAD_DONE)
        strcpy(buf, "{ \"state\" : \"done\" }\r\n");
    else
        snprintf(buf, 100, "{ \"state\" : \"uploading\", \"size\" : %d, \"received\" : %d }\r\n",
                 iWholeLength, iFinishedLength);

    g_api->append_resp_body(session, buf, strlen(buf));
    g_api->end_resp(session);
    g_api->log(session, LSI_LOG_DEBUG,
               "[module uploadprogress:%s] processed for URI: %s\n",
               MNAME._info, g_api->get_req_uri(session, NULL));
    return 0;
}


static lsi_serverhook_t server_hooks[] =
{
    { LSI_HKPT_RECV_REQ_HEADER, checkReqHeader, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED },
    { LSI_HKPT_RECV_REQ_BODY, reqBodyRead, LSI_HOOK_NORMAL, 0 },
    { LSI_HKPT_HTTP_END, releaseModuleData, LSI_HOOK_LAST, 0},
    lsi_serverhook_t_END   //Must put this at the end position
};

static lsi_handler_t myhandler = { begin_process, NULL, NULL, NULL };
lsi_module_t MNAME =
    { LSI_MODULE_SIGNATURE, _init, &myhandler, NULL, "v1.0", server_hooks, {0} };


