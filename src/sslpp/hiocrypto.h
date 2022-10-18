/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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

#ifndef HIOCRYPTO_H
#define HIOCRYPTO_H
#include <unistd.h>
typedef struct x509_st X509;
class HioCrypto
{
public:
    enum ENV
    {
        CRYPTO_VERSION,
        SESSION_ID,
        CIPHER,
        CIPHER_USEKEYSIZE,
        CIPHER_ALGKEYSIZE,
        CLIENT_CERT,
        TRANS_PROTOCOL_VERSION
    };
    HioCrypto()             {}
    virtual ~HioCrypto()    {}
    
    virtual int getEnv(HioCrypto::ENV id, char *&val,int maxValLen) = 0;
    virtual X509 *getPeerCertificate() const
    {   return NULL;    }
    virtual int  getVerifyMode() const
    {   return 0;       }
    virtual int  isVerifyOk() const
    {   return 0;       }
    int  buildVerifyErrorString(char *pBuf, int len) const
    {   return 0;       }
    
};


#endif // HIOCRYPTO_H
