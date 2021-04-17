/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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


#ifndef SSLOCSPSTAPLING_H
#define SSLOCSPSTAPLING_H
#include <socket/gsockaddr.h>
#include <util/autostr.h>
#include <time.h>

class SslContext;
class HttpFetch;

typedef struct asn1_string_st ASN1_TIME;
typedef struct ocsp_basic_response_st OCSP_BASICRESP;
typedef struct ocsp_cert_id_st OCSP_CERTID;
typedef struct ocsp_response_st OCSP_RESPONSE;
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct x509_st X509;
typedef struct x509_store_st X509_STORE;

class SslOcspStapling
{
public:
    SslOcspStapling();
    ~SslOcspStapling();
    int init(SslContext *pSslCtx);
    int update();
    int getCertId(X509 *pCert);
    int createRequest();
    int getResponder(X509 *pCert);
    int callback(SSL *ssl);
    int processResponse(HttpFetch *pHttpFetch);
    int verifyRespFile(int is_new_resp);
    int certVerify(OCSP_RESPONSE *pResponse, OCSP_BASICRESP *pBasicResp,
                   X509_STORE *pXstore);
    void releaseRespData();
    int updateRespData(OCSP_RESPONSE *pResponse);
    int getRequestData(unsigned char **pReqData);
    void setCertName (const char *Certfile);
    void certIdToOcspRespFileName();

    void setOcspResponder(const char *url)    {   m_sOcspResponder.setStr(url);     }
    void setRespMaxAge(const int iMaxAge)     {   m_iocspRespMaxAge = iMaxAge;      }
    void setRespfile(const char *Respfile)    {   m_sRespfile.setStr(Respfile);     }
    
    static void setCachePath(const char *pPath);
    static const char *getCachePath();
    static int setProxy(const char *pProxyAddrStr);

private:
    HttpFetch      *m_pHttpFetch;

    unsigned char  *m_pReqData;
    uint32_t        m_iDataLen;
    int             m_iocspRespMaxAge;
    unsigned char  *m_pRespData;
    GSockAddr       m_addrResponder;

    AutoStr         m_sCertName;
    AutoStr2        m_sOcspResponder;
    AutoStr         m_sRespfile;
    AutoStr         m_sRespfileTmp;
    ASN1_TIME      *m_notBefore;
    SslContext     *m_pCtx;
    time_t          m_RespTime;
    time_t          m_statTime;
    time_t          m_nextUpdate;
    OCSP_CERTID    *m_pCertId;
    static const struct sockaddr *s_proxy_addr;

};

const char *getStaplingErrMsg();
#endif // SSLOCSPSTAPLING_H
