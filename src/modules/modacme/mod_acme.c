/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ls.h"

#define MNAME       mod_acme
lsi_module_t MNAME;

enum acme_key {
    AK_ENABLE,
    AK_THUMBPRINT,
    N_AKS
};


static lsi_config_key_t acme_config_array[] = {
    { "acmeEnable",     AK_ENABLE,       0, },
    { "acmeThumbPrint", AK_THUMBPRINT,   0, },
    { NULL,             0,               0, },   // last element number be all zeroes
};


struct acme_config {
    size_t      thumb_len;
    char        thumb_str[0];   /* NUL-terminated */
};


static void * acme_parse_config(module_param_info_t *params, int param_count,
                    void *unused_initial_config, int level, const char *name)
{
    struct acme_config *config;
    const module_param_info_t *p, *param[N_AKS] = {};
    size_t cfg_size;

    g_api->log(NULL, LSI_LOG_DEBUG,
        "[ACME] Enter acme_parse_config\n");
     for (p = params; p < params + param_count; ++p)
        param[p->key_index] = p;

    if (!(param[AK_ENABLE] && param[AK_THUMBPRINT]
                                        && atoi(param[AK_ENABLE]->val)))
    {
        g_api->log(NULL, LSI_LOG_DEBUG,
            "[ACME] Parameter error, don't use mod_acme\n");
        return NULL;
    }

    cfg_size = sizeof(*config) + param[AK_THUMBPRINT]->val_len + 1;
    config = malloc(cfg_size);
    if (!config)
        return NULL;

    config->thumb_len = param[AK_THUMBPRINT]->val_len;
    memcpy(config->thumb_str, param[AK_THUMBPRINT]->val,
                                        param[AK_THUMBPRINT]->val_len);
    config->thumb_str[config->thumb_len] = '\0';
    return config;
}


/* RFC 8555, Section 8.3 */
#define CHALLENGE_PREFIX "/.well-known/acme-challenge/"

/* RFC 4648, Section 5, Table 2: The "URL and Filename safe" Base 64 Alphabet */
static const signed char valid_base64url[0x100] = {
    [(int)'A'] = 1, [(int)'B'] = 1, [(int)'C'] = 1, [(int)'D'] = 1,
    [(int)'E'] = 1, [(int)'F'] = 1, [(int)'G'] = 1, [(int)'H'] = 1,
    [(int)'I'] = 1, [(int)'J'] = 1, [(int)'K'] = 1, [(int)'L'] = 1,
    [(int)'M'] = 1, [(int)'N'] = 1, [(int)'O'] = 1, [(int)'P'] = 1,
    [(int)'Q'] = 1, [(int)'R'] = 1, [(int)'S'] = 1, [(int)'T'] = 1,
    [(int)'U'] = 1, [(int)'V'] = 1, [(int)'W'] = 1, [(int)'X'] = 1,
    [(int)'Y'] = 1, [(int)'Z'] = 1, [(int)'a'] = 1, [(int)'b'] = 1,
    [(int)'c'] = 1, [(int)'d'] = 1, [(int)'e'] = 1, [(int)'f'] = 1,
    [(int)'g'] = 1, [(int)'h'] = 1, [(int)'i'] = 1, [(int)'j'] = 1,
    [(int)'k'] = 1, [(int)'l'] = 1, [(int)'m'] = 1, [(int)'n'] = 1,
    [(int)'o'] = 1, [(int)'p'] = 1, [(int)'q'] = 1, [(int)'r'] = 1,
    [(int)'s'] = 1, [(int)'t'] = 1, [(int)'u'] = 1, [(int)'v'] = 1,
    [(int)'w'] = 1, [(int)'x'] = 1, [(int)'y'] = 1, [(int)'z'] = 1,
    [(int)'0'] = 1, [(int)'1'] = 1, [(int)'2'] = 1, [(int)'3'] = 1,
    [(int)'4'] = 1, [(int)'5'] = 1, [(int)'6'] = 1, [(int)'7'] = 1,
    [(int)'8'] = 1, [(int)'9'] = 1, [(int)'_'] = 1, [(int)'-'] = 1,
};

/* From RFC 8555, Section 8:

 " token (required, string):  A random value that uniquely identifies
 "    the challenge.  This value MUST have at least 128 bits of entropy.
 "    It MUST NOT contain any characters outside the base64url alphabet
 "    and MUST NOT include base64 padding characters ("=").  See
 "    [RFC4086] for additional information on randomness requirements.

 * Return true if file is valid, false otherwise.
 */
static int acme_filename_is_ok(const char *p, size_t len)
{
    const char *end;
    unsigned count;

    count = 0;
    for (end = p + len; p < end; ++p)
        count += valid_base64url[(unsigned char)*p];

    return count == len;
}


static int acme_req_header_cb(lsi_param_t *param)
{
    const char *uri;
    int len;

    uri = g_api->get_req_uri(param->session, &len);
    g_api->log(param->session, LSI_LOG_DEBUG,
        "[ACME] Enter acme_req_header_cb, I am %d, uri: %s\n", getuid(), uri);
    int count = g_api->get_req_headers_count(param->session);
    struct iovec keys[count + 1], vals[count + 1];
    g_api->get_req_headers(param->session, keys, vals, count);
    if (len > (int) sizeof(CHALLENGE_PREFIX)
        && 0 == memcmp(uri, CHALLENGE_PREFIX, sizeof(CHALLENGE_PREFIX) - 1)
        && acme_filename_is_ok(uri + sizeof(CHALLENGE_PREFIX) - 1,
                            (size_t) len - sizeof(CHALLENGE_PREFIX) + 1))
    {
        g_api->register_req_handler(param->session, &MNAME, 0);
        g_api->log(param->session, LSI_LOG_DEBUG,
               "[ACME] %s: register_req_handler for URI: %.*s\n", __func__, len, uri);
    }
    return LSI_OK;
}


static int acme_respond(const lsi_session_t *session)
{
    const struct acme_config *config;
    const char *uri;
    int len;

    uri = g_api->get_req_uri(session, &len);
    g_api->log(session, LSI_LOG_DEBUG,
        "[ACME] Enter acme_respond, I am %d\n", getuid());
    if (!(len > (int) sizeof(CHALLENGE_PREFIX)
          && 0 == memcmp(uri, CHALLENGE_PREFIX, sizeof(CHALLENGE_PREFIX) - 1)))
    {
        g_api->log(session, LSI_LOG_DEBUG,
            "[ACME] ot a challenge prefix, it's: %s\n", uri);
        return -1;
    }

    config = (struct acme_config *) g_api->get_config(session, &MNAME);
    if (!config)
    {
        g_api->log(session, LSI_LOG_DEBUG,
            "[ACME] No acme config configured\n");
        return -1;
    }

    g_api->append_resp_body(session, uri + sizeof(CHALLENGE_PREFIX) - 1,
        len - sizeof(CHALLENGE_PREFIX) + 1);
    g_api->append_resp_body(session, ".", 1);
    g_api->append_resp_body(session, config->thumb_str, config->thumb_len);
    g_api->end_resp(session);
    g_api->log(session, LSI_LOG_DEBUG,
        "[ACME] %s: replied with `%.*s.%.*s'\n", __func__,
        len - (int) sizeof(CHALLENGE_PREFIX) + 1, uri + sizeof(CHALLENGE_PREFIX) - 1,
        (int) config->thumb_len, config->thumb_str);

    return 0;
}


static lsi_serverhook_t acme_server_hooks[] =
{
    { LSI_HKPT_RCVD_REQ_HEADER, acme_req_header_cb, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED, },
    LSI_HOOK_END
};

static lsi_confparser_t acme_confparser = {
    acme_parse_config, free, acme_config_array,
};

static lsi_reqhdlr_t acme_handler = { acme_respond, NULL, NULL, NULL, NULL, NULL, NULL };
LSMODULE_EXPORT lsi_module_t MNAME =
{
    LSI_MODULE_SIGNATURE, NULL, &acme_handler, &acme_confparser, "v1.0", acme_server_hooks, {0}
};
