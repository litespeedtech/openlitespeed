#ifndef SSLCONTEXTCONFIG_H
#define SSLCONTEXTCONFIG_H
#include <lsdef.h>
#include <util/autostr.h>

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
