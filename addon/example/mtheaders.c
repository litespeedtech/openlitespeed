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
#include <pthread.h>

#define MNAME       mtheaders
lsi_module_t MNAME;


static char resp_buf[] = "MT Headers module handler resp_buf.<br>\r\n";

typedef struct args_s {
    int v;
    const lsi_session_t * session;
} args_t;

void * make_cookie(void * arg)
{
    int v = ((args_t *)arg)->v;
    const lsi_session_t * session = ((args_t *)arg)->session;
    free(arg);
    char buf[128];
    // for clarity only as same value as index
    int ops[] = {
        LSI_HEADEROP_SET,
        LSI_HEADEROP_APPEND,
        LSI_HEADEROP_MERGE,
        LSI_HEADEROP_ADD
    };

    char * opstrs[] = {
        "LSI_HEADEROP_SET",
        "LSI_HEADEROP_APPEND",
        "LSI_HEADEROP_MERGE",
        "LSI_HEADEROP_ADD"
    };

    int res;
    int thread = ((int)pthread_self()) & 0xffff;

    size_t len = snprintf(buf, 128, "thread idx=%d:thread=%d", v, thread);
    res = g_api->set_resp_header(session, LSI_RSPHDR_SET_COOKIE, NULL, 0,
            buf, len, LSI_HEADEROP_ADD);

    len = snprintf(buf, 128, "%dcubed=%d,thread=%d", v, v*v*v, thread);
    do {
        int op = ops[1 + (random() % ((sizeof(ops) / sizeof(int) - 1)))];
        //int op = ops[random() % (sizeof(ops) / sizeof(int))];
        snprintf(buf+len, 128 - len, "op=%s", opstrs[op]);

        sched_yield();
        usleep(random() % 100);
        res = g_api->set_resp_header(session, LSI_RSPHDR_SET_COOKIE, NULL, 0,
                buf, len, op);
    } while(LS_FAIL != res);
    return NULL;
}

void reqhdrcb(int key_id, const char * key, int key_len, 
              const char * value, int val_len, void * arg)
{
    LSM_DBGH((&MNAME), ((lsi_cb_sess_param_t *)arg)->session, "enter %s: key_id %d, key %.*s, key_len %d, val %.*s, val_len %d, arg %p, session %p\n",
             __func__, key_id, key_len, key, key_len, val_len, value, val_len, arg, ((lsi_cb_sess_param_t *)arg)->session);

}

static int process_req(const lsi_session_t *session)
{
    int i;
    g_api->set_status_code(session, 200);
    g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
            "text/html", 9, LSI_HEADEROP_SET);

    const int num = 20;
    pthread_t threads[num];

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (i = 0; i < num; i++)
    {
        args_t * pArgs = malloc(sizeof(args_t));
        pArgs->session = session;
        pArgs->v = i;
        (void) pthread_create(&threads[i], &attr, make_cookie, (void *) pArgs);
    }

    const char * filt = NULL;
    lsi_cb_sess_param_t arg = { (lsi_session_t *)session, NULL };
    g_api->foreach(session, LSI_DATA_REQ_HEADER, filt, reqhdrcb, &arg);

    filt = "r(?i)accep(?-i)";
    g_api->foreach(session, LSI_DATA_REQ_HEADER, filt, reqhdrcb, &arg);

    filt = "!r(?i)accep(?-i)";
    g_api->foreach(session, LSI_DATA_REQ_HEADER, filt, reqhdrcb, &arg);

    filt = NULL;
    g_api->foreach(session, LSI_DATA_REQ_VAR, filt, reqhdrcb, &arg);

    filt = "r7";
    g_api->foreach(session, LSI_DATA_REQ_VAR, filt, reqhdrcb, &arg);

    filt = NULL;
    g_api->foreach(session, LSI_DATA_REQ_ENV, filt, reqhdrcb, &arg);

    filt = "r7";
    g_api->foreach(session, LSI_DATA_REQ_ENV, filt, reqhdrcb, &arg);

    filt = NULL;
    g_api->foreach(session, LSI_DATA_REQ_SPECIAL_ENV, filt, reqhdrcb, &arg);

    filt = "r7";
    g_api->foreach(session, LSI_DATA_REQ_SPECIAL_ENV, filt, reqhdrcb, &arg);

    filt = NULL;
    g_api->foreach(session, LSI_DATA_REQ_CGI_HEADER, filt, reqhdrcb, &arg);

    filt = "r7";
    g_api->foreach(session, LSI_DATA_REQ_CGI_HEADER, filt, reqhdrcb, &arg);

    filt = NULL;
    g_api->foreach(session, LSI_DATA_REQ_COOKIE, filt, reqhdrcb, &arg);

    filt = "r7";
    g_api->foreach(session, LSI_DATA_REQ_COOKIE, filt, reqhdrcb, &arg);

    sched_yield();
    sleep(4);

    filt = NULL;
    g_api->foreach(session, LSI_DATA_RESP_HEADER, filt, reqhdrcb, &arg);

    filt = "r7";
    g_api->foreach(session, LSI_DATA_RESP_HEADER, filt, reqhdrcb, &arg);

    g_api->send_resp_headers(session);

    pthread_attr_destroy(&attr);

    for(i = 0; i < 10; ++i)
    {
        if (g_api->append_resp_body(session, resp_buf, 
                    sizeof(resp_buf) - 1) == LS_FAIL)
            break;
        if (g_api->flush(session) == LS_FAIL)
            break;
        sleep(1);
    }

    filt = NULL;
    g_api->foreach(session, LSI_DATA_RESP_HEADER, filt, reqhdrcb, &arg);

    filt = "!r(?i)cook";
    g_api->foreach(session, LSI_DATA_RESP_HEADER, filt, reqhdrcb, &arg);

    filt = "r(?i)cook";
    g_api->foreach(session, LSI_DATA_RESP_HEADER, filt, reqhdrcb, &arg);

    filt = "g0*cook*";
    g_api->foreach(session, LSI_DATA_RESP_HEADER, filt, reqhdrcb, &arg);

    g_api->end_resp(session);

    for (i = 0; i < num; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}


/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_reqhdlr_t myhandler = { process_req, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, NULL, &myhandler, NULL, NULL, NULL, {0} };


