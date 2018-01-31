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

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <lsr/ls_str.h>

/**
 * Define the module name, MUST BE the same as .so file name;
 * i.e., if MNAME is testmodule, then the .so must be testmodule.so.
 */

#define MNAME       mtsendfile
lsi_module_t MNAME;

static int begin_process(const lsi_session_t *session)
{
    struct stat sb;
    off_t off = 0;
    off_t sz = -1; //-1 set to send all data
    int cnt;
    ls_strpair_t args;
    char buf[256];

    g_api->parse_req_args(session, 0, 0, NULL, 0);
    cnt = g_api->get_qs_args_count(session);
    LSM_DBG((&MNAME), session, "qs arg cnt: %d\n", cnt);
    if (1 != cnt
        || LS_FAIL == g_api->get_qs_arg_by_idx(session, 0, &args)
        || strncmp(ls_str_cstr(&args.key), "name", strlen("name"))
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

    const char * root = g_api->get_server_root();
    const size_t len = strlen(root);
    memcpy(buf, root, len);
    buf[len] = '/';
    memcpy(buf+len+1, ls_str_cstr(&args.val), ls_str_len(&args.val));
    buf[len+1+ls_str_len(&args.val)] = '\0';

    LSM_DBG((&MNAME), session, "full path name: %s\n", buf);

    if (stat(buf, &sb) == 0)
    {
        sz = sb.st_size;
    }
    else {
        g_api->set_status_code(session, 500);
        char br[] = "\nMT FILE NOT FOUND\n";
        g_api->append_resp_body(session, br, sizeof(br));
        g_api->end_resp(session);
        return LS_FAIL;
    }

    if (g_api->send_file(session, buf, off, sz) < 0)
    {
        g_api->set_status_code(session, 500);
        char br[] = "\nMT FILE SEND FAILED\n";
        g_api->append_resp_body(session, br, sizeof(br));
        g_api->end_resp(session);
        return LS_FAIL;
    }

    // g_api->append_resp_body(session, txtlast, sizeof(txtlast) - 1);
    g_api->end_resp(session);
    return 0;
}


static lsi_reqhdlr_t myhandler = { begin_process, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, NULL, &myhandler, NULL, NULL, NULL, {0} };
