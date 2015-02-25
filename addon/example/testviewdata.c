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
#include <stdlib.h>
#include <string.h>

#define     MNAME       testviewdata
lsi_module_t MNAME;
/////////////////////////////////////////////////////////////////////////////
#define     VERSION         "V1.0"

static int viewData0(lsi_cb_param_t *rec, const char *level)
{
    if (rec->_param_len < 100)
        g_api->log(rec->_session, LSI_LOG_INFO,
                   "[testautocompress] viewData [%s] %s\n",
                   level, (const char *)rec->_param);
    else
    {
        g_api->log(rec->_session, LSI_LOG_INFO,
                   "[testautocompress] viewData [%s] ",  level);
        g_api->lograw(rec->_session, rec->_param, 40);
        g_api->lograw(rec->_session, "(...)", 5);
        g_api->lograw(rec->_session, rec->_param + rec->_param_len - 40, 40);
        g_api->lograw(rec->_session, "\n", 1);
    }
    return g_api->stream_write_next(rec, rec->_param, rec->_param_len);
}

static int viewData1(lsi_cb_param_t *rec) {   return viewData0(rec, "RECV");  }
static int viewData2(lsi_cb_param_t *rec) {   return viewData0(rec, "SEND");  }

static int beginSession(lsi_cb_param_t *rec)
{
    g_api->set_session_hook_enable_flag(rec->_session, LSI_HKPT_RECV_RESP_BODY,
                                        &MNAME, 1);
    g_api->set_session_hook_enable_flag(rec->_session, LSI_HKPT_SEND_RESP_BODY,
                                        &MNAME, 1);
    return 0;
}

static lsi_serverhook_t serverHooks[] =
{
    {LSI_HKPT_HTTP_BEGIN, beginSession, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_RECV_RESP_BODY, viewData1, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_DECOMPRESS_REQUIRED },
    {LSI_HKPT_SEND_RESP_BODY, viewData2, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_DECOMPRESS_REQUIRED},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init()
{
    MNAME._info = VERSION;  //set version string
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "", serverHooks};
