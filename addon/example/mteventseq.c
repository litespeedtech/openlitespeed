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
#include <sys/types.h>
#include <lsr/ls_str.h>
#include <lsr/ls_lock.h>

#define MNAME       mteventseq
lsi_module_t MNAME;

typedef struct myData_s
{
    long seq;
    int done;
} myData_t;

static char resp_buf[] = "MT event sequence module handler.<br>\r\n";

int dataRelease(void *p)
{
    free(p);
    return 0;
}


static int init(lsi_module_t *pModule)
{
    g_api->init_module_data(pModule, dataRelease, LSI_DATA_HTTP);
    return 0;
}


int process_event(lsi_session_t *session, long lParam, void *pParam)
{
    myData_t * data = (myData_t *) g_api->get_module_data(session, &MNAME,
                                                 LSI_DATA_HTTP);

    char buf[256] = "process_event seq ";


    size_t len = strlen(buf);

    len += snprintf(buf+len, 255-len, "want %ld got %ld", data->seq, lParam);

    if (data->seq != lParam)
    {
        // bad, report problem
        len += snprintf(buf+len, 255-len, " FAIL<br>\n");
        if (LS_FAIL == g_api->append_resp_body(session, buf, strlen(buf)))
        {
            LSM_ERR((&MNAME), session, "failed to append resp body\n");
        }
        g_api->flush(session);
        return LS_FAIL;
    }

    len += snprintf(buf+len, 255-len, " OK<br>\n");
    if (LS_FAIL == g_api->append_resp_body(session, buf, strlen(buf)))
    {
        LSM_ERR((&MNAME), session, "failed to append resp body\n");
        return LS_FAIL;
    }
    g_api->flush(session);

    (data->seq)++;

    if ((lParam + 1) >= (long) pParam)
    {
        //g_api->end_resp(session);
        ls_atomic_setint(&data->done, 1);
        ls_futex_wake(&data->done);
    }
    return LS_OK;
}


static int process_req(const lsi_session_t *session)
{
    long l;
    int cnt;
    ls_strpair_t args;
    char buf[256];

    g_api->set_status_code(session, 200);
    g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);

    g_api->parse_req_args(session, 0, 0, NULL, 0);
    cnt = g_api->get_qs_args_count(session);
    LSM_DBG((&MNAME), session, "qs arg cnt: %d\n", cnt);
    if (1 != cnt
        || LS_FAIL == g_api->get_qs_arg_by_idx(session, 0, &args)
        || strncmp(ls_str_cstr(&args.key), "num", strlen("num"))
        )
    {
        g_api->set_status_code(session, 400);
        g_api->send_resp_headers(session);
        LSM_ERR((&MNAME), session, "failed to get req arg by index, key %.*s val %.*s\n",
                (int)ls_str_len(&args.key),
                ls_str_cstr(&args.key),
                (int)ls_str_len(&args.val),
                ls_str_cstr(&args.val)
                );
        char br[] = "BAD REQUEST\n";
        g_api->append_resp_body(session, br, sizeof(br));
        g_api->end_resp(session);
        return LS_FAIL;
    }

    memcpy(buf, ls_str_cstr(&args.val), ls_str_len(&args.val));
    buf[ls_str_len(&args.val)] = '\0';

    long num = atol(buf);

    g_api->send_resp_headers(session);

    if (g_api->append_resp_body(session, resp_buf, sizeof(resp_buf) - 1) == LS_FAIL)
    {
        return LS_FAIL;
    }
    snprintf(buf, sizeof(buf), "num %ld<br>\n", num);
    if (g_api->append_resp_body(session, buf, strlen(buf)) == LS_FAIL)
    {
        return LS_FAIL;
    }
    if (g_api->flush(session) == LS_FAIL)
    {
        return LS_FAIL;
    }

    myData_t * data = (myData_t *) malloc(sizeof(myData_t));
    data->seq = 0;
    data->done = 0;
    ls_futex_setup(&data->done);

    g_api->set_module_data(session, &MNAME, LSI_DATA_HTTP,
                           (void *)data);

    for(l = 0; l < num; ++l)
    {
        g_api->create_event(process_event, session, l, (void *) num, 0);
    }

    struct timespec timeout = { 0, 1000000 };

    for (l = 0; l < num * 10; l++)
    {
        if (0 == ls_futex_wait(&data->done, 0, &timeout))
        {
            break;
        }
    }

    if (ls_atomic_fetch_add(&data->done,0))
    {
        // all cbs finished, we didn't time out
        free(data);
        data = NULL;
        g_api->set_module_data(session, &MNAME, LSI_DATA_HTTP, NULL);
    }

    return 0;
}


/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_reqhdlr_t myhandler = { process_req, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init, &myhandler, NULL, NULL, NULL, {0} };


