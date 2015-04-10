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

#include <lsr/ls_xpool.h>

#include <string.h>

/**
 * This module tests setting the enable flag during the session.
 * It is recommended to use this along with testenableflag 2, 3, and 4.
 *
 * This one tests: Disabling statically enabled callback functions.
 *
 * To test: After compiling and installing this module, access a dynamically
 * processed page.
 *
 * i.e. curl -i http://localhost:8088/phpinfo.php
 */

#define     MNAME       testenableflag1
lsi_module_t MNAME;
/////////////////////////////////////////////////////////////////////////////
#define     VERSION         "V1.0"

static int addoutput(lsi_cb_param_t *rec, const char *level)
{
    int len = 0, lenNew;
    char *pBuf;
    if (rec->_param_len <= 0)
        return g_api->stream_write_next(rec, rec->_param, rec->_param_len);
    pBuf = ls_xpool_alloc(g_api->get_session_pool(rec->_session), rec->_param_len + strlen(level) + 1);
    snprintf(pBuf, rec->_param_len + strlen(level) + 1, "%.*s%.*s",
                rec->_param_len, (const char *)rec->_param, strlen(level) + 1, level);
    lenNew = rec->_param_len + strlen(level);
    len = g_api->stream_write_next(rec, pBuf, lenNew);
    if (len < lenNew)
        *rec->_flag_out = LSI_CB_FLAG_OUT_BUFFERED_DATA;
    if (len < rec->_param_len)
        return len;
    return rec->_param_len;
}

static int addrecvresp(lsi_cb_param_t *rec)
{   return addoutput(rec, "RECV1: If this appears, something is wrong.\n");  }


static int addsendresp(lsi_cb_param_t *rec)
{   return addoutput(rec, "SEND1: If this appears, something is wrong.\n");  }

static int beginSession(lsi_cb_param_t *rec)
{
    int aEnableHkpts[] = {LSI_HKPT_RECV_RESP_BODY, LSI_HKPT_SEND_RESP_BODY};
    g_api->set_session_hook_enable_flag(rec->_session, &MNAME, 0,
                                        aEnableHkpts, 2);
    return 0;
}

static lsi_serverhook_t serverHooks[] =
{
    {LSI_HKPT_HTTP_BEGIN, beginSession, LSI_HOOK_NORMAL, LSI_HOOK_FLAG_ENABLED},
    {LSI_HKPT_RECV_RESP_BODY, addrecvresp, LSI_HOOK_NORMAL,
        LSI_HOOK_FLAG_ENABLED | LSI_HOOK_FLAG_TRANSFORM},
    {LSI_HKPT_SEND_RESP_BODY, addsendresp, LSI_HOOK_NORMAL,
        LSI_HOOK_FLAG_ENABLED | LSI_HOOK_FLAG_TRANSFORM},
    lsi_serverhook_t_END   //Must put this at the end position
};

static int _init()
{
    MNAME._info = VERSION;  //set version string
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL, "", serverHooks};