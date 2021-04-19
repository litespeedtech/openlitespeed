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

#ifndef SSLCERTCOMP_H
#define SSLCERTCOMP_H

#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif
#include <openssl/ssl.h>

#ifdef OPENSSL_IS_BORINGSSL
#define SSLCERTCOMP     // The determiner in ALL the code which uses this class.

#include <lsdef.h>

class SslCertComp 
{
    SslCertComp(); 
    ~SslCertComp();
    
public:
    typedef struct 
    {
        int     m_input_len;
        int     m_len;
        uint8_t m_comp[1];
    } comp_cache_t;
    
    static void activateComp(bool activate);      
    static void activateDecomp(bool activate);
    static void enableCertComp(SSL_CTX *ctx);
    static void enableCertDecomp(SSL_CTX *ctx);
    static void disableCertCompDecomp(SSL_CTX *ctx);
    static void setBrCompressLevel(int level);

    LS_NO_COPY_ASSIGN(SslCertComp);
};

#endif
#endif
