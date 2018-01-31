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

#define MNAME       mthello
lsi_module_t MNAME;


static char resp_buf[] = "MT Hello module handler.<br>\r\n";

static void cleaner(void * arg)
{
    g_api->log(NULL, LSI_LOG_DEBUG, "cleanup handler called, arg %s\n",
               (char *) arg);
}

static int init_func(lsi_module_t * module)
{
    g_api->register_thread_cleanup(module, cleaner, "Hi Mom!");
    return LS_OK;
}

static int process_req(const lsi_session_t *session)
{
    int i;
    g_api->set_status_code(session, 200);
    g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);
    
    for(i = 0; i < 1; ++i)
    {
        if (g_api->append_resp_body(session, resp_buf, 
                                    sizeof(resp_buf) - 1) == LS_FAIL)
            break;
        /*
        if (g_api->flush(session) == LS_FAIL)
            break;
        sleep(1);
        */
    }

    g_api->end_resp(session);
    return 0;
}


/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_reqhdlr_t myhandler = { process_req, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init_func, &myhandler, NULL, NULL, NULL, {0} };


