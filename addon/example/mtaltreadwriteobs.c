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

#define MNAME       mtaltreadwriteobs
lsi_module_t MNAME;
DECL_COMPONENT_LOG("mtaltreadwriteobs")


/**
 * To use:
 * Set up a context using this module as the handler.
 * One or two query string parameters are required (case insensitive):
 * - readsize (required)
 * - writesize (optional)
 * If only readsize is passed in, writesize will be set to the same value.
 * M and K are valid for use (e.g. 1k to represent size of 1024.)
 *
 */

static int extractValue(const lsi_session_t *session, const char *pValue, int iValueLen)
{
    int iValue;
    char *pEnd;
    char buf[11]; // max len - INTMAX is 10 characters long + 1.

    if (iValueLen >= sizeof(buf))
    {
        LSC_ERR(session, "Size value is too long.\n");
        return LS_FAIL;
    }

    memcpy(buf, pValue, iValueLen);
    buf[iValueLen] = '\0'; // ensure null terminated for strtol.
    pValue = buf;
    iValue = strtol(pValue, &pEnd, 10);

    if (pEnd == pValue)
    {
        LSC_ERR(session, "Read size must be a number (optionally can add trailing K or M)\n");
        return LS_FAIL;
    }
    else if (pEnd - pValue + 1 == iValueLen) // allow for K or M
    {
        switch(*pEnd)
        {
        case 'm':
        case 'M':
            if (iValue >= 1UL << (sizeof(iValue) * 8 - 1 - 20))
            {
                LSC_ERR(session, "Value is too high to be combined with m.\n");
                return LS_FAIL;
            }
            iValue <<= 20;
            break;
        case 'k':
        case 'K':
            if (iValue >= 1UL << (sizeof(iValue) * 8 - 1 - 10))
            {
                LSC_ERR(session, "Value is too high to be combined with k.\n");
                return LS_FAIL;
            }
            iValue <<= 10;
            break;
        default:
            LSC_ERR(session, "Only K and M are allowed after numbers.\n");
            return LS_FAIL;
        }
    }
    else if (pEnd - pValue != iValueLen)
    {
        LSC_ERR(session, "Have some garbage values after numbers.\n");
        return LS_FAIL;
    }
    return iValue;
}


static int parseInput(const lsi_session_t *session, int64_t *iReadSize, int64_t *iWriteSize)
{


    int i, iQsCnt;
    ls_strpair_t args;

    ls_str(&args.key, NULL, 0);
    ls_str(&args.val, NULL, 0);
    *iReadSize = -1;
    *iWriteSize = -1;

    if (LS_FAIL == g_api->parse_req_args(session, 0, 0, NULL, 0))
    {
        LSC_ERR(session, "Failed to parse args.\n");
        return LS_FAIL;
    }

    iQsCnt = g_api->get_qs_args_count(session);
    if ((iQsCnt != 1) && (iQsCnt != 2))
    {
        LSC_ERR(session, "Incorrect number of query string arguments. %d\n", iQsCnt);
        return LS_FAIL;
    }

    for (i = 0; i < iQsCnt; ++i)
    {
        g_api->get_qs_arg_by_idx(session, i, &args);
        if ((8 == ls_str_len(&args.key))
            && (0 == strncasecmp("readsize", ls_str_cstr(&args.key), 8)))
        {
            *iReadSize = extractValue(session, ls_str_cstr(&args.val),
                    ls_str_len(&args.val));

            if (LS_FAIL == *iReadSize)
            {
                LSC_ERR(session, "Bad read size value.\n");
                return LS_FAIL;
            }
        }
        else if ((9 == ls_str_len(&args.key))
            && (0 == strncasecmp("writesize", ls_str_cstr(&args.key), 9)))
        {
            *iWriteSize = extractValue(session, ls_str_cstr(&args.val),
                    ls_str_len(&args.val));

            if (LS_FAIL == *iWriteSize)
            {
                LSC_ERR(session, "Bad write size value.\n");
                return LS_FAIL;
            }
        }
    }

    if (-1 == *iReadSize)
    {
        LSC_ERR(session, "Must have at least a read size.\n");
        return LS_FAIL;
    }
    else if (-1 == *iWriteSize)
        *iWriteSize = *iReadSize;

    return LS_OK;
}


static int process_req(const lsi_session_t *session)
{
    int64_t iReadSize, iWriteSize, iLen, iContentLen, iTotalRead = 0;
    char *pBuf, *pBufEnd, *pReadPtr, *pWritePtr;
    int aEnableHkpts[] = { LSI_HKPT_RECV_RESP_BODY };
    int iRc;

    if (NULL == session)
    {
        LSC_ERR(session, "Session is empty\n");
        return LS_FAIL;
    }

    if (LS_FAIL == parseInput(session, &iReadSize, &iWriteSize))
    {
        g_api->set_status_code(session, 403);
        g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);

        g_api->end_resp(session);
        return LS_FAIL;
    }

    iRc = g_api->enable_hook(session, &MNAME, 1, aEnableHkpts, 1);
    iContentLen = g_api->get_req_content_length(session);

    g_api->set_status_code(session, 200);
    g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);
    g_api->set_resp_content_length(session, iContentLen);

    if (iReadSize == iWriteSize)
    {
        pBuf = malloc(sizeof(char) * iReadSize);
        pBufEnd = pBuf + iReadSize;
    }
    else
    {
        /**
         * If read size is greater than write size, it will read once
         * then try to write (one or multiple) times until it cannot write
         * any more, then the data is moved to the beginning and
         * it can read again.
         * If write size is greater than read size, it will read multiple
         * times until it can write until it cannot do either. Then the data
         * is moved to the beginning and it can read again.
         */
        pBuf = malloc(sizeof(char) * (iReadSize + iWriteSize));
        pBufEnd = pBuf + iReadSize + iWriteSize;
    }

    if (NULL == pBuf)
    {
        LSC_ERR(session, "Failed to allocate space.\n");
        g_api->set_status_code(session, 403);
        g_api->set_resp_header(session, LSI_RSPHDR_CONTENT_TYPE, NULL, 0,
                           "text/html", 9, LSI_HEADEROP_SET);

        g_api->end_resp(session);
        return LS_FAIL;
    }

    pReadPtr = pWritePtr = pBuf;
    do
    {
        if ((pBufEnd - pReadPtr) >= iReadSize) // Enough space to read chunk
        {
            iLen = g_api->read_req_body(session, pReadPtr, iReadSize);
            if (LS_FAIL == iLen)
            {
                LSC_ERR(session, "Error while trying to read req body total read %ld contentlen %ld\n", iTotalRead, iContentLen);
                break;
            }
            pReadPtr += iLen;
            iTotalRead += iLen;
            // LSC_ERR(session, "read %ld bytes\n", iLen);
        }

        if ((pReadPtr - pWritePtr) >= iWriteSize) // Enough data to write back
        {
            iLen = g_api->append_resp_body(session, pWritePtr, iWriteSize);
            if (LS_FAIL == iLen)
            {
                LSC_ERR(session, "Error while trying to append body\n");
                break;
            }
            pWritePtr += iLen;
            // LSC_ERR(session, "wrote %d bytes\n", iLen);
        }

        if (((pReadPtr - pWritePtr) < iWriteSize)
            && ((pBufEnd - pReadPtr) < iReadSize))
        {
            memmove(pBuf, pWritePtr, pReadPtr - pWritePtr);
            pReadPtr = pBuf + (pReadPtr - pWritePtr);
            pWritePtr = pBuf;
        }
        if (iContentLen < 0)
            iContentLen = g_api->get_req_content_length(session);
    }
    while (iTotalRead < (uint64_t)iContentLen); // uint will make -1 wrap.

    if (LS_FAIL != iLen) // Only finish writing if no error.
    {
        while (pWritePtr < pReadPtr)
        {
            iLen = pReadPtr - pWritePtr;
            if (iLen > iWriteSize)
                iLen = iWriteSize;
            iLen = g_api->append_resp_body(session, pWritePtr, iLen);
            if (LS_FAIL == iLen)
            {
                LSC_ERR(session, "Error while trying to append resp body\n");
                break;
            }
            pWritePtr += iLen;

        }
    }

    free(pBuf);
    g_api->end_resp(session);
    return 0;
}

static int observer( lsi_param_t * pParam )
{
    return g_api->stream_write_next(pParam, (const char *)pParam->ptr1, pParam->len1 );
}

static int init_module(lsi_module_t * module)
{
    return 0;
}

/**
 * Define a handler, need to provide a struct _handler_st object, in which
 * the first function pointer should not be NULL
 */
static lsi_serverhook_t serverHooks[] =
{
    {
        //LSI_HKPT_RECV_RESP_BODY, observer, LSI_HOOK_NORMAL, LSI_FLAG_ENABLED
        LSI_HKPT_RECV_RESP_BODY, observer, LSI_HOOK_NORMAL, 0
    },
    LSI_HOOK_END   //Must put this at the end position
};

static lsi_reqhdlr_t myhandler = { process_req, NULL, NULL, NULL, LSI_HDLR_DEFAULT_POOL, NULL, NULL};
LSMODULE_EXPORT lsi_module_t MNAME = { LSI_MODULE_SIGNATURE, init_module, &myhandler, NULL /* parser */, "MyWonderfulModule", serverHooks, {0} };
