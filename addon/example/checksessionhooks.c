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

#define     MNAME       checksessionhooks
lsi_module_t MNAME;

/**
 * This module tests to make sure every session hook is reached.
 */

const char *s_pCheckHookNames[LSI_HKPT_TOTAL_COUNT] =
{
    "L4_BEGINSESSION",
    "L4_ENDSESSION",
    "L4_RECVING",
    "L4_SENDING",
    "HTTP_BEGIN",
    "RECV_REQ_HEADER",
    "URI_MAP",
    "HTTP_AUTH",
    "RECV_REQ_BODY",
    "RCVD_REQ_BODY",
    "RECV_RESP_HEADER",
    "RECV_RESP_BODY",
    "RCVD_RESP_BODY",
    "HANDLER_RESTART",
    "SEND_RESP_HEADER",
    "SEND_RESP_BODY",
    "HTTP_END",
    "MAIN_INITED",
    "MAIN_PREFORK",
    "MAIN_POSTFORK",
    "WORKER_POSTFORK",
    "WORKER_ATEXIT",
    "MAIN_ATEXIT"
};


int enableNextL4Hook(lsi_param_t *rec)
{
    static int s_iL4Level = LSI_HKPT_L4_RECVING;
    g_api->log(NULL, LSI_LOG_DEBUG,
               "[Module:checkhttphooks] Try to enable level %s\n",
               s_pCheckHookNames[s_iL4Level]);
    g_api->enable_hook(rec->session, &MNAME, 1, &s_iL4Level, 1);
    if (s_iL4Level == LSI_HKPT_L4_ENDSESSION)
    {
        int disableHooks[] = { LSI_HKPT_L4_RECVING, LSI_HKPT_L4_SENDING };
        g_api->log(NULL, LSI_LOG_DEBUG,
                    "[Module:checkhttphooks] All hooks enabled, "
                    "disable L4 recv/send to prevent infinite loop.\n");
        g_api->enable_hook(rec->session, &MNAME, 0, disableHooks, 2);
        return 0;

    }
    s_iL4Level++;
    if (s_iL4Level == LSI_HKPT_HTTP_BEGIN)
        s_iL4Level = LSI_HKPT_L4_ENDSESSION;
    return 0;
}


int enableNextHttpHook(lsi_param_t *rec)
{
    static int s_iHttpLevel = LSI_HKPT_RCVD_REQ_HEADER;
    g_api->log(NULL, LSI_LOG_DEBUG,
               "[Module:checkhttphooks] Try to enable level %s\n",
               s_pCheckHookNames[s_iHttpLevel]);
    g_api->enable_hook(rec->session, &MNAME, 1, &s_iHttpLevel, 1);
    s_iHttpLevel++;
    // make sure to enable send resp header too, ignore req body.
    if (s_iHttpLevel == LSI_HKPT_RCVD_REQ_BODY
        || s_iHttpLevel == LSI_HKPT_RCVD_RESP_HEADER
        || s_iHttpLevel == LSI_HKPT_SEND_RESP_HEADER)
    {
        g_api->log(NULL, LSI_LOG_DEBUG,
                "[Module:checkhttphooks] Enable next level because "
                "%s may be skipped.\n", s_pCheckHookNames[s_iHttpLevel - 1]);
        enableNextHttpHook(rec);
    }
    return 0;
}


int skippedHook(lsi_param_t *rec)
{
    g_api->log(NULL, LSI_LOG_DEBUG,
            "[Module:checkhttphooks] This level is generally skipped because "
            "it requires something else (e.g. a request body). "
            "This function shows that this level was enabled.\n");
    return 0;
}


int reachEnd(const char *pLevel)
{
    g_api->log(NULL, LSI_LOG_DEBUG, "[Module:checkhttphooks] Reached the %s end!\n", pLevel);
    return 0;
}


int reachHttpEnd(lsi_param_t *rec)  {   return reachEnd("Http");    }
int reachL4End(lsi_param_t *rec)    {   return reachEnd("L4");      }

static lsi_serverhook_t serverHooks[] =
{
    { LSI_HKPT_L4_BEGINSESSION,     enableNextL4Hook,   LSI_HOOK_NORMAL, LSI_FLAG_ENABLED},
    { LSI_HKPT_L4_ENDSESSION,       reachL4End,         LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_L4_RECVING,          enableNextL4Hook,   LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_L4_SENDING,          enableNextL4Hook,   LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_HTTP_BEGIN,          enableNextHttpHook, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED},
    { LSI_HKPT_RCVD_REQ_HEADER,     enableNextHttpHook, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_URI_MAP,             enableNextHttpHook, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_HTTP_AUTH,           enableNextHttpHook, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_RECV_REQ_BODY,       skippedHook,        LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_RCVD_REQ_BODY,       skippedHook,        LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_RCVD_RESP_HEADER,    enableNextHttpHook, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_RECV_RESP_BODY,      enableNextHttpHook, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_RCVD_RESP_BODY,      enableNextHttpHook, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_HANDLER_RESTART,     skippedHook,        LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_SEND_RESP_HEADER,    enableNextHttpHook, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_SEND_RESP_BODY,      enableNextHttpHook, LSI_HOOK_NORMAL, 0},
    { LSI_HKPT_HTTP_END,            reachHttpEnd,       LSI_HOOK_NORMAL, 0},
    LSI_HOOK_END   //Must put this at the end position
};

static int _init(lsi_module_t *pModule)
{
    return 0;
}

lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, _init, NULL, NULL,
                        "checksessionhooks v1.0", serverHooks, {0}};

