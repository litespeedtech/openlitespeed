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

#define MNAME       mtflood
lsi_module_t MNAME;


static char buffer[1000010];

static int process_req(const lsi_session_t *session)
{
    int i;
    g_api->set_status_code(session, 200);
    g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);

    // First build this terrible buffer to send.
    for (i = 0; i < 1000000; i++) {
        buffer[i] = (i % 10) + '0';
    }
    buffer[1000000] = '\r';
    buffer[1000001] = '\n';
    buffer[1000002] = 0;
    
    for(i = 0; i < 1000; ++i)
    {
        char bufferTitle[80];
        sprintf(bufferTitle,"Line #%d",i);
        memcpy(buffer,bufferTitle,strlen(bufferTitle));
        g_api->append_resp_body(session, buffer, 1000002);
    }

    g_api->end_resp(session);
    return 0;
}


/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_reqhdlr_t myhandler = { process_req, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, NULL, &myhandler, NULL, NULL, NULL, {0} };


