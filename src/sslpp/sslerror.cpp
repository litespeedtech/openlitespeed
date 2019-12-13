/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */


#include "sslerror.h"
#include <openssl/err.h>
#include <string.h>
#include <lsdef.h>

#define MSG_MAX_LEN sizeof( m_achMsg ) - 1

SslError::SslError() throw()
{
    m_achMsg[0] = 0;
    m_iError = 0;
    char *p = m_achMsg;
    char *pEnd = &m_achMsg[MSG_MAX_LEN] - 1;
    const char *data;
    int flag;
    int iError;
    while ((iError = ERR_peek_error_line_data(NULL, NULL, &data, &flag)) != 0)
    {

        ERR_error_string_n(iError, p, pEnd - p);
        p += strlen(p);
        if (*data && (flag & ERR_TXT_STRING))
        {
            *p++ = ':';
            lstrncpy(p, data, pEnd - p);
        }
        p += strlen(p);
        ERR_get_error();
    }
}

SslError::SslError(int err) throw()
{
    m_achMsg[0] = 0;
    m_achMsg[MSG_MAX_LEN] = 0;
    m_iError = err;
    ERR_error_string_n(m_iError, m_achMsg, MSG_MAX_LEN);
}

SslError::SslError(const char *pErr) throw()
{
    if (pErr)
    {
        m_achMsg[MSG_MAX_LEN] = 0;
        m_iError = -1;
        memccpy(m_achMsg, pErr, 0, MSG_MAX_LEN);
    }
    else
    {
        m_achMsg[0] = 0;
        m_iError = 0;
    }
}

SslError::~SslError() throw()
{
}
