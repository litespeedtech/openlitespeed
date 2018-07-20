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

#include <lsr/ls_fileio.h>
#include <lsr/ls_str.h>
#include <lsr/ls_strtool.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/uio.h>

#define MNAME       mtsetters
lsi_module_t MNAME;



static int release_data(void *data)
{
    ls_strpair_t *pair = (ls_strpair_t *)data;
    ls_str_d(&pair->key);
    ls_str_d(&pair->val);
    free(pair);
    return 0;
}

static int init_module(lsi_module_t *module)
{
    g_api->init_module_data(module, release_data, LSI_DATA_HTTP);
    g_api->init_module_data(module, release_data, LSI_DATA_IP);
    g_api->init_module_data(module, release_data, LSI_DATA_VHOST);
    g_api->init_module_data(module, release_data, LSI_DATA_L4);

    return 0;
}


static int set_data(const lsi_session_t *session)
{
    const lsi_module_t *module = &MNAME;
    ls_strpair_t *pair = NULL;

    if (NULL == g_api->get_module_data(session, module, LSI_DATA_HTTP))
    {
        pair = malloc(sizeof(*pair));
        ls_str(&pair->key, "mtsettershttp", 13);
        ls_str(&pair->val, "httpdata", 8);
        g_api->set_module_data(session, module, LSI_DATA_HTTP, pair);
    }

    if (NULL == g_api->get_module_data(session, module, LSI_DATA_IP))
    {
        pair = malloc(sizeof(*pair));
        ls_str(&pair->key, "mtsettersip", 11);
        ls_str(&pair->val, "ipdata", 6);
    }

    if (NULL == g_api->get_module_data(session, module, LSI_DATA_VHOST))
    {
        pair = malloc(sizeof(*pair));
        ls_str(&pair->key, "mtsettersvhost", 14);
        ls_str(&pair->val, "vhostdata", 9);
    }

    if (NULL == g_api->get_module_data(session, module, LSI_DATA_L4))
    {
        pair = malloc(sizeof(*pair));
        ls_str(&pair->key, "mtsettersl4", 11);
        ls_str(&pair->val, "l4data", 6);
    }

    return 0;
}


static void ontimer(const void *param)
{
    const lsi_session_t *session = (const lsi_session_t *)param;
    LSM_DBGH((&MNAME), session, "ontimer callback\n");
}


static int setresphdrshelper(const lsi_session_t *session, ls_strpair_t *data,
    int headerop)
{

    g_api->set_resp_header(session, LSI_RSPHDR_SERVER, NULL, 0,
            ls_str_cstr(&data->val), ls_str_len(&data->val), headerop);

    g_api->set_resp_header(session, LSI_RSPHDR_END,
            ls_str_cstr(&data->key), ls_str_len(&data->key),
            ls_str_cstr(&data->val), ls_str_len(&data->val), headerop);
    return LS_OK;
}


static int setresphdrs(const lsi_session_t *session)
{
    int multihdrslen;
    ls_strpair_t *httpdata, *ipdata, *vhostdata, *l4data;
    char multihdrs[128];

    set_data(session);

    httpdata = g_api->get_module_data(session, &MNAME, LSI_DATA_HTTP);
    ipdata = g_api->get_module_data(session, &MNAME, LSI_DATA_IP);
    vhostdata = g_api->get_module_data(session, &MNAME, LSI_DATA_VHOST);
    l4data = g_api->get_module_data(session, &MNAME, LSI_DATA_L4);

    if ((!httpdata) || (!ipdata) || (!vhostdata) || (!l4data))
    {
        LSM_ERR((&MNAME), session, "missing module data\n");
        return LS_FAIL;
    }

    setresphdrshelper(session, httpdata, LSI_HEADEROP_SET);
    setresphdrshelper(session, ipdata, LSI_HEADEROP_APPEND);
    setresphdrshelper(session, vhostdata, LSI_HEADEROP_MERGE);
    setresphdrshelper(session, l4data, LSI_HEADEROP_ADD);

    multihdrslen = snprintf(multihdrs, 128, "%s: %s\n%s: %s\n%s: %s\n%s: %s\n",
        ls_str_cstr(&httpdata->key), ls_str_cstr(&ipdata->val),
        ls_str_cstr(&ipdata->key), ls_str_cstr(&vhostdata->val),
        ls_str_cstr(&vhostdata->key), ls_str_cstr(&l4data->val),
        ls_str_cstr(&l4data->key), ls_str_cstr(&httpdata->val));

    if (g_api->set_resp_header2(session, multihdrs, multihdrslen, LSI_HEADEROP_SET))
    {
        LSM_ERR((&MNAME), session, "failed to set multi headers\n");
    }

    g_api->remove_resp_header(session, -1, ls_str_cstr(&httpdata->key), ls_str_len(&httpdata->key));
    g_api->remove_resp_header(session, -1, ls_str_cstr(&ipdata->key), ls_str_len(&ipdata->key));
    g_api->remove_resp_header(session, -1, ls_str_cstr(&vhostdata->key), ls_str_len(&vhostdata->key));
    g_api->remove_resp_header(session, -1, ls_str_cstr(&l4data->key), ls_str_len(&l4data->key));

    return LS_OK;
}


static int trysendfile(const lsi_session_t *session)
{
    const int bufsize = 1024;
    int fd, filestrlen, ret, i;
    uint64_t file_size = 0;
    char buf[bufsize];
    const char *filepath = "$SERVER_ROOT/docs/config.html";
    struct stat st;
    void *bodybuf;

    filestrlen = g_api->expand_current_server_variable(LSI_CFG_SERVER,
                    filepath, buf, bufsize);
    if (filestrlen <= 0)
        LSM_ERR((&MNAME), session, "Failed to expand server variable\n");
    else if (LS_FAIL == g_api->get_file_stat(session, buf,
            filestrlen, &st))
    {
        LSM_DBGH((&MNAME), session, "Server conf %.*s\n", filestrlen, buf);
    }
    else
    {
        buf[filestrlen] = '\0';
        file_size = st.st_size;
    }

    // NOTICE: O_NONBLOCK not set. Is that required?
    fd = ls_fio_open(buf, O_RDONLY, 0);
    if (-1 == fd)
    {
        if (ENOENT == errno)
            LSM_NOT((&MNAME), session, "File does not exist.\n");
        else
            LSM_ERR((&MNAME), session, "Failed to open file %d\n", errno);
        return LS_FAIL;
    }
    bodybuf = g_api->get_new_body_buf(file_size);

    do
    {
        i = ls_fio_read(fd, buf, bufsize);
        if (i <= 0)
            break;
        if (LS_FAIL == (ret = g_api->append_body_buf(bodybuf, buf, i)))
        {
            LSM_ERR((&MNAME), session, "Failed to append to body buf %d\n", errno);
            break;
        }
    } while (bufsize == i);

    ls_fio_lseek(fd, 0, SEEK_SET);
    g_api->reset_body_buf(bodybuf, 1);
    g_api->set_resp_content_length(session, file_size);
    ret = g_api->send_file2(session, fd, 0, file_size);
    if (ret)
    {
        LSM_ERR((&MNAME), session, "Failed to send file, received ret %d\n", ret);
    }
    ls_fio_close(fd);
    return LS_OK;
}


static int process_req(const lsi_session_t *session)
{
    const char *qs, *uri;
    int qs_len, uri_len, timer_id_remove, timer_id_leave;
    if (NULL == session)
    {
        LSM_ERR((&MNAME), session, "Session is empty\n");
        return LS_FAIL;
    }
    timer_id_remove = g_api->set_timer(10, 1, ontimer, session);
    timer_id_leave = g_api->set_timer(10, 1, ontimer, session);
    if ((LS_FAIL != timer_id_remove) || (LS_FAIL != timer_id_leave))
    {
        LSM_ERR((&MNAME), session, "Was able to set up timer(s) %d %d\n",
                timer_id_remove, timer_id_leave);
    }

    if (LS_FAIL == g_api->set_req_wait_full_body(session))
        LSM_ERR((&MNAME), session, "Failed to set req wait full body\n");

    if (LS_FAIL == g_api->set_resp_wait_full_body(session))
        LSM_ERR((&MNAME), session, "Failed to set resp wait full body\n");

    if (LS_FAIL == g_api->set_resp_buffer_compress_method(session, 0))
        LSM_ERR((&MNAME), session, "Failed to set no gzip\n");

    if (LS_FAIL == g_api->set_force_mime_type(session, NULL))
        LSM_ERR((&MNAME), session, "Failed to set force mime type to null\n");



    qs = g_api->get_req_query_string(session, &qs_len);
    uri = g_api->get_req_uri(session, &uri_len);

    const char *redir_int_qs = "new=qs";
    if ((strlen(redir_int_qs) != qs_len) || (strncmp(redir_int_qs, qs, qs_len)))
    {
        if (LS_OK == g_api->set_uri_qs(session, LSI_URL_REDIRECT_INTERNAL | LSI_URL_QS_SET,
            uri, uri_len, redir_int_qs, strlen(redir_int_qs)))
        {
            return 0;
        }
    }
    else
    {
        if (LS_FAIL == g_api->set_uri_qs(session, LSI_URL_REWRITE | LSI_URL_QS_APPEND,
            "newuri", 6, redir_int_qs, strlen(redir_int_qs)))
        {
            LSM_ERR((&MNAME), session, "failed to rewrite uri\n");
        }
    }

    g_api->set_status_code(session, 200);
    g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);
    setresphdrs(session);

    if (!g_api->is_resp_headers_sent(session))
        g_api->send_resp_headers(session);

    trysendfile(session);

    if (timer_id_remove != LS_FAIL)
        g_api->remove_timer(timer_id_remove);
    g_api->end_resp(session);
    return 0;
}


/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_reqhdlr_t myhandler = { process_req, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init_module, &myhandler, NULL, NULL, NULL, {0} };


