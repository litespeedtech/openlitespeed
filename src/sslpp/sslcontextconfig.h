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

#ifndef SSLCONTEXTCONFIG_H
#define SSLCONTEXTCONFIG_H
#include <lsdef.h>
#include <util/autostr.h>


#define MULTI_CERT_ALL   1
#define MULTI_CERT_ECC   2

class SslContextConfig
{
public:
    SslContextConfig();
    ~SslContextConfig();

public:
    AutoStr     m_sName;
    AutoStr     m_sCertFile[3];
    AutoStr     m_sKeyFile[3];
    AutoStr     m_sCiphers;
    AutoStr     m_sCAPath;
    AutoStr     m_sCAFile;
    AutoStr     m_sDHParam;
    AutoStr     m_sCaChainFile;
    AutoStr     m_sOcspResponder;
    //AutoStr     m_sOcspCa;
    char        m_iKeyCerts;
    char        m_iEnableMultiCerts;
    char        m_iCertChain;
    char        m_iClientVerify;
    char        m_iProtocol;
    char        m_iEnableECDHE;
    char        m_iEnableDHE;
    char        m_iEngine;
    char        m_iInsecReneg;
    char        m_iEnableSpdy;
    char        m_iEnableCache;
    char        m_iEnableTicket;
    char        m_iEnableStapling;
    int         m_iVerifyDepth;
    int32_t     m_iOcspMaxAge;

    LS_NO_COPY_ASSIGN(SslContextConfig);
};

#endif // SSLCONTEXTCONFIG_H
