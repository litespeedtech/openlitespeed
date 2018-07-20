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

#include <lsr/ls_str.h>
#include <lsr/ls_strtool.h>

#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/uio.h>

#define MNAME       mtgetters
lsi_module_t MNAME;
DECL_COMPONENT_LOG("mtgetters")

static int check_req_hdrs(const lsi_session_t *session)
{
    struct iovec *iov_keys, *iov_vals;
    char *req_raw_hdrs;
    int hdr_cnt, req_raw_hdr_len, ret;

    ret = g_api->get_req_content_length(session);
    hdr_cnt = g_api->get_req_headers_count(session);
    req_raw_hdr_len = g_api->get_req_raw_headers_length(session);

    if ((LS_FAIL == hdr_cnt) || (LS_FAIL == req_raw_hdr_len))
        return LS_FAIL;

    req_raw_hdrs = malloc(sizeof(char) * req_raw_hdr_len + 1);
    iov_keys = malloc(sizeof(struct iovec) * hdr_cnt);
    iov_vals = malloc(sizeof(struct iovec) * hdr_cnt);

    if ((NULL == req_raw_hdrs) || (NULL == iov_keys) || (NULL == iov_vals))
    {
        LSM_ERR((&MNAME), session, "Failed to allocate space for req_raw_hdrs\n");
        free(req_raw_hdrs);
        free(iov_keys);
        free(iov_vals);
        return LS_FAIL;
    }

    ret = g_api->get_req_raw_headers(session, req_raw_hdrs, req_raw_hdr_len + 1);

    if (ret != req_raw_hdr_len)
    {
        LSM_ERR((&MNAME), session, "getting raw req headers returned incorrect len %d, expected %d\n",
            ret, req_raw_hdr_len);
    }
    ret = g_api->get_req_headers(session, iov_keys, iov_vals, hdr_cnt);
    if (ret != hdr_cnt)
    {
        LSM_ERR((&MNAME), session, "getting iov req headers returned incorrect count %d, expected %d\n",
            ret, hdr_cnt);
    }
    else
    {
        int i, val_len, iovk_len, iovv_len;
        const char *val, *iovk, *iovv;
        for (i = 0; i < hdr_cnt; ++i)
        {
            iovk = (const char *)iov_keys[i].iov_base;
            iovk_len = (int)iov_keys[i].iov_len;
            iovv = (const char *)iov_vals[i].iov_base;
            iovv_len = (int)iov_vals[i].iov_len;
            val = g_api->get_req_header(session, iovk, iovk_len, &val_len);
            if (NULL == val)
            {
                LSM_ERR((&MNAME), session, "get_req_header did not find header %.*s",
                   iovk_len, iovk);
            }
            else if ((val_len != iovv_len)
                || (0 != strncmp(iovv, val, val_len)))
            {
                LSM_ERR((&MNAME), session, "get_req_header val did not match. iov: %d %.*s, header %d %.*s",
                    iovv_len, iovv_len, iovv, val_len, val_len, val);
            }
        }

    }
    free(req_raw_hdrs);
    free(iov_keys);
    free(iov_vals);
    return LS_OK;
}


static int check_req_uri(const lsi_session_t *session)
{
    int len, uri_len;
    char buf[1024];
    const char *uri;

    len = g_api->get_req_org_uri(session, buf, 1024);
    if (LS_FAIL == len)
    {
        LSM_ERR((&MNAME), session, "failed to get org uri\n");
    }

    uri = g_api->get_mapped_context_uri(session, &uri_len);

    // This check doesn't really do anything, just testing using the buffer.
    if (((uri_len <= len) && (0 != strncmp(uri, buf, uri_len)))
        || ((uri_len > len) && (0 != strncmp(uri, buf, len))))
    {
        LSM_DBGH((&MNAME), session, "context mapped uri is different\n");
    }

    // Test this one second so I can use it for path.
    uri = g_api->get_req_uri(session, &uri_len);

    // This check doesn't really do anything, just testing using the buffer.
    if (((uri_len <= len) && (0 != strncmp(uri, buf, uri_len)))
        || ((uri_len > len) && (0 != strncmp(uri, buf, len))))
    {
        LSM_DBGH((&MNAME), session, "uri was changed during processing.\n");
    }

    len = g_api->get_uri_file_path(session, uri, uri_len, buf, 1024);

    if (LS_FAIL == len)
        LSM_ERR((&MNAME), session, "failed to get uri path\n");
    else
        LSM_DBGH((&MNAME), session, "path returned was %.*s", len, buf);

    len = g_api->get_file_path_by_uri(session, uri, uri_len, buf, 1024);

    if (LS_FAIL == len)
        LSM_ERR((&MNAME), session, "failed to get path by uri\n");
    else
        LSM_DBGH((&MNAME), session, "path by uri returned was %.*s", len, buf);

    return LS_OK;
}


static int check_req_cookies(const lsi_session_t *session)
{
    int i, all_cookies_len, cookies_cnt, cookie_val_len;
    const char *all_cookies, *cookie_val;
    ls_strpair_t cookie;
    ls_str(&cookie.key, NULL, 0);
    ls_str(&cookie.val, NULL, 0);

    if (NULL == (all_cookies = g_api->get_req_cookies(session, &all_cookies_len)))
    {
        LSM_ERR((&MNAME), session, "failed to get all req cookies\n");
    }

    if (LS_FAIL == (cookies_cnt = g_api->get_req_cookie_count(session)))
    {
        LSM_ERR((&MNAME), session, "failed to get req cookie count\n");
        return LS_FAIL;
    }
    for (i = 0; i < cookies_cnt; ++i)
    {
        if (0 == g_api->get_cookie_by_index(session, i, &cookie))
        {
            LSM_ERR((&MNAME), session, "failed to get cookie by index\n");
            continue;
        }
        cookie_val = g_api->get_cookie_value(session, ls_str_cstr(&cookie.key),
                            (int)ls_str_len(&cookie.key), &cookie_val_len);
        if (NULL == cookie_val)
        {
            LSM_ERR((&MNAME), session, "cookie val was null for key %.*s\n",
                (int)ls_str_len(&cookie.key), ls_str_cstr(&cookie.key));
            continue;
        }
        if ((cookie_val_len != ls_str_len(&cookie.val))
            || (0 != strncmp(ls_str_cstr(&cookie.val), cookie_val, cookie_val_len)))
        {
            LSM_ERR((&MNAME), session, "cookie val did not match. pair %lu %.*s, api %d %.*s\n",
                ls_str_len(&cookie.val), (int)ls_str_len(&cookie.val), ls_str_cstr(&cookie.val),
                cookie_val_len, cookie_val_len, cookie_val);
            continue;
        }
    }
    return LS_OK;
}


static void log_strpair(const lsi_session_t *session, const char *prefix,
    ls_strpair_t *pair)
{
    const char *keyptr = ls_str_cstr(&pair->key);
    const char *valptr = ls_str_cstr(&pair->val);
    int keylen = (int)ls_str_len(&pair->key);
    int vallen = (int)ls_str_len(&pair->val);
    LSM_DBGH((&MNAME), session, "%s key %d %.*s val %d %.*s\n",
             prefix, keylen, keylen, keyptr, vallen, vallen, valptr);
}


static int check_req_args(const lsi_session_t *session)
{
    int i, cnt;
    ls_strpair_t args;
    char *filepath;
    ls_str(&args.key, NULL, 0);
    ls_str(&args.val, NULL, 0);

    g_api->is_req_body_finished(session);
    if (LS_FAIL == g_api->parse_req_args(session, 1, 0, NULL, 0))
    {
        LSM_ERR((&MNAME), session, "failed to parse args\n");
        return LS_FAIL;
    }

    cnt = g_api->get_req_args_count(session);
    for (i = 0; i < cnt; ++i)
    {
        filepath = NULL;
        if (LS_FAIL == g_api->get_req_arg_by_idx(session, i, &args, &filepath))
        {
            LSM_ERR((&MNAME), session, "failed to get req arg by index\n");
            break;
        }
        log_strpair(session, "req args", &args);
        LSM_DBGH((&MNAME), session, "filepath %s\n", filepath);
    }

    cnt = g_api->get_qs_args_count(session);
    for (i = 0; i < cnt; ++i)
    {
        if (LS_FAIL == g_api->get_qs_arg_by_idx(session, i, &args))
        {
            LSM_ERR((&MNAME), session, "failed to get qs arg by index\n");
            break;
        }
        log_strpair(session, "qs args", &args);
    }

    cnt = g_api->get_post_args_count(session);
    for (i = 0; i < cnt; ++i)
    {
        filepath = NULL;
        if (LS_FAIL == g_api->get_post_arg_by_idx(session, i, &args, &filepath))
        {
            LSM_ERR((&MNAME), session, "failed to get post arg by index\n");
            break;
        }
        log_strpair(session, "post args", &args);
        LSM_DBGH((&MNAME), session, "filepath %s file %d\n", filepath,
            g_api->is_post_file_upload(session, i));
    }

    return LS_OK;
}


static int check_misc(const lsi_session_t *session)
{
    int len;
    const char *ptr;
    char buf[1024];
    time_t sec;
    int32_t usec;
    ls_xpool_t *pool;
    if (!g_api->is_req_handler_registered(session))
    {
        LSM_ERR((&MNAME), session, "Req handler not registered\n");
    }

    if (NULL == (ptr = g_api->get_req_handler_type(session)))
    {
        LSM_ERR((&MNAME), session, "Handler type was null\n");
    }

    if (NULL == (ptr = g_api->get_client_ip(session, &len)))
    {
        LSM_ERR((&MNAME), session, "cannot get client ip\n");
    }

    if (1 == g_api->get_client_access_level(session))
    {
        LSM_ERR((&MNAME), session, "client access denied\n");
    }

    ptr = g_api->get_mime_type_by_suffix(session, "php");
    LSM_ERR((&MNAME), session, "got mime type for php %s\n", ptr);

    if (NULL == (ptr = g_api->get_req_file_path(session, &len)))
    {
        LSM_ERR((&MNAME), session, "req file path returned null\n");
    }

    if (!g_api->is_access_log_on(session))
    {
        LSM_ERR((&MNAME), session, "Access log is not on\n");
    }

    if (LS_FAIL == (len = g_api->get_access_log_string(session,
         "%v %h %l %u %t \"%r\" %>s %b", buf, 1024)))
    {
        LSM_ERR((&MNAME), session, "Failed to get access log string\n");
    }

    struct stat st;
    const char *statpath = "/tmp/lshttpd/openlitespeed.pid";
    if (LS_FAIL == g_api->get_file_stat(session, statpath, strlen(statpath), &st))
    {
        LSM_ERR((&MNAME), session, "get file stat failed\n");
    }

    sec = g_api->get_cur_time(&usec);
    LSM_DBGH((&MNAME), session, "cur time %lu %u\n", sec, usec);

    if (NULL == (pool = g_api->get_session_pool(session)))
        LSM_DBGH((&MNAME), session, "did not get pool\n");

    len = g_api->get_local_sockaddr(session, buf, 1024);
    if (len <= 0)
    {
        LSM_ERR((&MNAME), session, "failed to get local sockaddr %d\n", len);
    }
    else
        LSM_DBGH((&MNAME), session, "local sockaddr %.*s\n", len, buf);

    ls_str_t confvar;
    ls_str_x(&confvar, "$SERVER_ROOT/conf/httpd_config.conf", 35, pool);

    len = g_api->expand_current_server_variable(LSI_CFG_SERVER,
        ls_str_cstr(&confvar), buf, 1024);
    if (len <= 0)
        LSM_ERR((&MNAME), session, "Failed to expand server variable\n");
    else
        LSM_DBGH((&MNAME), session, "Server conf %.*s\n", len, buf);

    ls_str_xsetstr(&confvar, "$VH_ROOT/conf/httpd_config.conf", 31, pool);
    len = g_api->expand_current_server_variable(LSI_CFG_VHOST,
        ls_str_cstr(&confvar), buf, 1024);
    if (len <= 0)
        LSM_ERR((&MNAME), session, "Failed to expand vh_root variable\n");
    else
        LSM_DBGH((&MNAME), session, "vhroot conf %.*s\n", len, buf);

    ls_str_xsetstr(&confvar, "$VH_DOMAIN/test", 15, pool);
    len = g_api->expand_current_server_variable(LSI_CFG_LISTENER,
        ls_str_cstr(&confvar), buf, 1024);
    if (len <= 0)
        LSM_ERR((&MNAME), session, "Failed to expand vh_domain variable\n");
    else
        LSM_DBGH((&MNAME), session, "vhdomain test %.*s\n", len, buf);

    ls_str_xsetstr(&confvar, "$DOC_ROOT/conf/httpd_config.conf", 32, pool);
    len = g_api->expand_current_server_variable(LSI_CFG_VHOST,
        ls_str_cstr(&confvar), buf, 1024);
    if (len <= 0)
        LSM_ERR((&MNAME), session, "Failed to expand doc_root variable\n");
    else
        LSM_DBGH((&MNAME), session, "docroot conf %.*s\n", len, buf);

    LSM_DBGH((&MNAME), session, "server root %s", g_api->get_server_root());


    return LS_OK;
}


static int check_resp_misc(const lsi_session_t *session)
{
    LSM_DBGH((&MNAME), session, "status code %d\n",
        g_api->get_status_code(session));

    if (!g_api->is_resp_buffer_available(session))
        LSM_DBGH((&MNAME), session, "resp buffer not available\n");
    else if(g_api->get_resp_buffer_compress_method(session) == 0)
        LSM_DBGH((&MNAME), session, "resp buffer not compressed\n");
    LSM_DBGH((&MNAME), session, "resp buffer avail and gzipped\n");

    if (g_api->is_suspended(session) <= 0)
    {
        LSM_ERR((&MNAME), session, "session is suspended\n");
    }

    if (g_api->is_resp_handler_aborted(session))
        LSM_DBGH((&MNAME), session, "resp handler is aborted\n");

        return LS_OK;
}


static int check_resp_hdrs(const lsi_session_t *session)
{
    struct iovec *iov_keys, *iov_vals;
    int hdr_cnt, ret;

    hdr_cnt = g_api->get_resp_headers_count(session);

    LSM_DBGH((&MNAME), session, "is resp headers sent %d\n",
        g_api->is_resp_headers_sent(session));

    if ((LS_FAIL == hdr_cnt))
        return LS_FAIL;

    iov_keys = malloc(sizeof(struct iovec) * hdr_cnt);
    iov_vals = malloc(sizeof(struct iovec) * hdr_cnt);

    if ((NULL == iov_keys) || (NULL == iov_vals))
    {
        LSM_ERR((&MNAME), session, "Failed to allocate space for response headers\n");
        return LS_FAIL;
    }

    ret = g_api->get_resp_headers(session, iov_keys, iov_vals, hdr_cnt);

    if (ret != hdr_cnt)
    {
        LSM_ERR((&MNAME), session, "getting iov req headers returned incorrect count %d, expected %d\n",
            ret, hdr_cnt);
        free(iov_keys);
        free(iov_vals);
        return LS_OK;
    }
    int i, val_len, iovk_len, iovv_len;
    const char *iovk, *iovv;
    struct iovec iovals[5];
    for (i = 0; i < hdr_cnt; ++i)
    {
        iovk = (const char *)iov_keys[i].iov_base;
        iovk_len = (int)iov_keys[i].iov_len;
        iovv = (const char *)iov_vals[i].iov_base;
        iovv_len = (int)iov_vals[i].iov_len;
        val_len = g_api->get_resp_header(session, LSI_RSPHDR_UNKNOWN,
            iovk, iovk_len, iovals, 5);
        if (0 == val_len)
        {
            LSM_ERR((&MNAME), session, "get_resp_header did not find header %.*s\n",
                iovk_len, iovk);
        }
        else if (val_len > 1)
        {
            LSM_DBGH((&MNAME), session, "get_resp_header val got multiple matches.\n");

        }
        else if (((int)iovals[0].iov_len != iovv_len)
            || (0 != strncmp(iovv, iovals[0].iov_base, iovv_len)))
        {
            LSM_DBGH((&MNAME), session, "get_req_header val did not match. iov: %d %.*s, header %d %.*s",
                iovv_len, iovv_len, iovv,
                (int)iovals[0].iov_len, (int)iovals[0].iov_len,
                (char *)iovals[0].iov_base);
        }
    }
    free(iov_keys);
    free(iov_vals);
    return LS_OK;
}


static int check_body_bufs(const lsi_session_t *session)
{
    int64_t off;
    void *body_buf;

    body_buf = g_api->get_req_body_buf(session);

    if (NULL == body_buf)
        LSM_ERR((&MNAME), session, "failed to get req body buf\n");
    else
    {
        off = g_api->get_body_buf_size(body_buf);
        LSM_DBGH((&MNAME), session, "req body buf size %ld\n", off);
        if (LS_FAIL != off)
        {
            LSM_DBGH((&MNAME), session, "req body buf eof %d\n",
                g_api->is_body_buf_eof(body_buf, off));
        }
        if (LS_FAIL == g_api->get_body_buf_fd(body_buf))
        {
            LSM_DBGH((&MNAME), session, "failed to get body buf fd\n");
        }

    }

    body_buf = g_api->get_resp_body_buf(session);

    if (NULL == body_buf)
        LSM_ERR((&MNAME), session, "failed to get resp body buf\n");
    else
    {
        off = g_api->get_body_buf_size(body_buf);
        LSM_DBGH((&MNAME), session, "resp body buf size %ld\n", off);
        if (LS_FAIL != off)
        {
            LSM_DBGH((&MNAME), session, "resp body buf eof %d\n",
                g_api->is_body_buf_eof(body_buf, off));
        }
        if (LS_FAIL == g_api->get_body_buf_fd(body_buf))
        {
            LSM_DBGH((&MNAME), session, "failed to get body buf fd\n");
        }

    }

    return LS_OK;
}


static int check_vhost(const lsi_session_t *session)
{
    int i, vhost_cnt;
    void *data;
    const void *vhost, *my_vhost = g_api->get_req_vhost(session);
    if (NULL == my_vhost)
    {
        LSM_ERR((&MNAME), session, "no req vhost\n");
        return LS_FAIL;
    }
    vhost_cnt = g_api->get_vhost_count();
    LSM_DBGH((&MNAME), session, "vhost count %d\n", vhost_cnt);
    for (i = 0; i < vhost_cnt; ++i)
    {
        vhost = g_api->get_vhost(i);
        if (NULL == vhost)
            LSM_ERR((&MNAME), session, "no vhost for index %d\n", i);
        else if (vhost == my_vhost)
            LSM_DBGH((&MNAME), session, "matched req vhost %d\n", i);
        else
            LSM_DBGH((&MNAME), session, "did not match req vhost %d\n", i);
    }

    if (NULL == (data = g_api->get_vhost_module_data(my_vhost, (&MNAME))))
        LSM_DBGH((&MNAME), session, "no module data\n");

    if (NULL == (data = g_api->get_vhost_module_conf(my_vhost, (&MNAME))))
        LSM_DBGH((&MNAME), session, "no module param\n");


    return LS_OK;
}

static int process_req(const lsi_session_t *session)
{
    if (NULL == session)
    {
        LSC_ERR(session, "Session is empty\n");
        return LS_FAIL;
    }


    check_req_hdrs(session);

    check_req_uri(session);

    check_req_cookies(session);

    check_req_args(session);

    // TODO: env functions.

    check_misc(session);

    g_api->set_status_code(session, 200);
    g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);

    check_resp_misc(session);

    check_resp_hdrs(session);

    check_body_bufs(session);

    check_vhost(session);

    g_api->end_resp(session);
    return 0;
}


/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_reqhdlr_t myhandler = { process_req, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, NULL, &myhandler, NULL, NULL, NULL, {0} };


