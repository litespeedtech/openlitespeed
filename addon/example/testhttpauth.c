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
#include <string.h>
#include <stdint.h>
#include "stdlib.h"
#include <unistd.h>

#define     MNAME       testhttpauth
lsi_module_t MNAME;

int httpAuth(lsi_cb_param_t *rec)
{
    //test if the IP is 127.0.0.1 pass through, otherwise, reply 403
    char ip[16] = {0};
    g_api->get_req_var_by_id(rec->_session, LSI_REQ_GEOIP_ADDR, ip, 16);
    if (strcmp(ip, "127.0.0.1") == 0)
        return LSI_HK_RET_OK;
    else
    {
        g_api->set_status_code(rec->_session, 403);
        g_api->log(rec->_session, LSI_LOG_INFO, "Access denied since ip = %s.\n",
                   ip);
        return LSI_HK_RET_ERROR;
    }
}

static lsi_serverhook_t serverHooks[] =
{
    {LSI_HKPT_HTTP_AUTH, httpAuth, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init(lsi_module_t *pModule)
{
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "", serverHooks};
