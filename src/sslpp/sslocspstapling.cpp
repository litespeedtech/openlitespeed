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
#include <sslpp/sslocspstapling.h>
#include <sslpp/sslerror.h>
#include "main/httpserverbuilder.h"
#include <http/httpglobals.h>

#include <util/base64.h>
#include <util/httpfetch.h>
#include <util/vmembuf.h>
#include <util/stringtool.h>
#include <util/xmlnode.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static AutoStr2 s_ErrMsg = "";
const char* getStaplingErrMsg(){ return s_ErrMsg.c_str(); }

static void setLastErrMsg(const char *format, ...)
{
    const unsigned int MAX_LINE_LENGTH = 1024;
    char s[MAX_LINE_LENGTH] = {0};
    va_list ap;
    va_start( ap, format );
    int ret = vsnprintf( s, MAX_LINE_LENGTH, format, ap );
    va_end( ap );

    if ( ( unsigned int )ret > MAX_LINE_LENGTH )
        ret = MAX_LINE_LENGTH;
    s_ErrMsg.setLen(0);
    s_ErrMsg.setStr(s,ret);
}


static unsigned int escapeBase64Uri(unsigned char *s, size_t size, unsigned char *d)
{
    unsigned char *begin = d;
    while (size--)
    {
        if ( isalnum(*s) )
            *d++ = *s++;
        else
        {   
            *d++ = '%';
            *d++ = StringTool::s_hex[*s >> 4];
            *d++ = StringTool::s_hex[*s & 0x0f];
            s++;
        }
    }
    return (unsigned int) (d - begin);
}

static X509 * load_cert( const char * pPath )
{
    X509 * pCert; 
    BIO * bio = BIO_new_file(pPath, "r");
    if ( bio == NULL )
        return NULL;
    pCert = PEM_read_bio_X509_AUX(bio, NULL, NULL, NULL);
    BIO_free( bio );
    return pCert;
}

static int OcspRespCb(void *pArg, HttpFetch *pHttpFetch)
{
    SslOcspStapling* pSslOcspStapling = (SslOcspStapling*)pArg;
    pSslOcspStapling->processResponse(pHttpFetch);
    return 0;   
}

int SslOcspStapling::callback(SSL *ssl)
{
    int     iResult;
    unsigned char* pbuff;
    iResult = SSL_TLSEXT_ERR_NOACK;
    if ( m_iDataLen > 0 )
    {
        /*OpenSSL will free pbuff by itself */
        pbuff = (unsigned char*)malloc(m_iDataLen);
        if ( pbuff == NULL )
            return SSL_TLSEXT_ERR_NOACK;
        memcpy( pbuff, m_pRespData, m_iDataLen );
        SSL_set_tlsext_status_ocsp_resp(ssl, pbuff, m_iDataLen);
        iResult = SSL_TLSEXT_ERR_OK;
    }
    update();
    return iResult;
}

int SslOcspStapling::processResponse(HttpFetch *pHttpFetch)
{
    struct stat        st;
    const char          *pRespContentType;
    //assert( pHttpFetch == m_pHttpFetch );
    int istatusCode = m_pHttpFetch->getStatusCode() ;
    pRespContentType = m_pHttpFetch->getRespContentType();
    if( ( istatusCode == 200) && (strcasecmp(pRespContentType, "application/ocsp-response") ==0))
        verifyRespFile();
    else
    {
        setLastErrMsg("Received bad OCSP response. ReponderUrl=%s, StatusCode=%d, ContentType=%s\n", 
                      m_sOcspResponder.c_str(), istatusCode, ((pRespContentType)?(pRespContentType):("")));
        //printf("%s\n", s_ErrMsg.c_str());
        m_pHttpFetch->writeLog(s_ErrMsg.c_str());
    }
    
    if ( ::stat( m_sRespfileTmp.c_str(), &st ) == 0 )   
        unlink( m_sRespfileTmp.c_str() );
    return 0;
}

SslOcspStapling::SslOcspStapling()
        : m_pHttpFetch( NULL )
        , m_iDataLen( 0 )
        , m_pRespData( NULL )
        , m_RespTime( 0 )
        , m_pCertId( NULL )
{
}

SslOcspStapling::~SslOcspStapling()
{
    if ( m_pRespData != NULL )
        delete []m_pRespData;     
    if  ( m_pHttpFetch != NULL)
        delete m_pHttpFetch;
    if ( m_pCertId != NULL)
        OCSP_CERTID_free(m_pCertId);
}

int SslOcspStapling::init(SSL_CTX* pSslCtx)
{
    int             iResult;
    X509*           pCert;
    struct timeval  CurTime;
    struct stat     st;
    m_pCtx = pSslCtx;
    pCert = NULL;
    iResult = -1;
    
    if ( ::stat( m_sRespfileTmp.c_str(), &st ) == 0 )  
    {
        gettimeofday( &CurTime, NULL );
        if ( (st.st_mtime + 30) <= CurTime.tv_sec )
            unlink( m_sRespfileTmp.c_str() );   
    }
    //SSL_CTX_set_default_verify_paths( m_pCtx );

    pCert = load_cert( m_sCertfile.c_str() );
    if ( pCert == NULL )
    {
        setLastErrMsg("Failed to load file: %s!\n", m_sCertfile.c_str());
        return -1;
    }

    if ( ( getCertId( pCert ) == 0 ) && ( getResponder( pCert ) == 0 ))
    {
        m_addrResponder.setHttpUrl(m_sOcspResponder.c_str(), m_sOcspResponder.len());
        iResult = 0;
        //update();
        
    }
    X509_free( pCert );
    return iResult;
}

int SslOcspStapling::update()
{

    struct timeval CurTime;
    struct stat st;

    gettimeofday( &CurTime, NULL );
    if ( ::stat( m_sRespfile.c_str(), &st ) == 0 )
    {
        if ( (st.st_mtime + m_iocspRespMaxAge) > CurTime.tv_sec )
        {
            if ( m_RespTime != st.st_mtime )
            {
                verifyRespFile( 0 );
                m_RespTime = st.st_mtime;
            }
            return 0;
        }
    }
    createRequest();
    return 0;
}


int SslOcspStapling::getResponder(X509* pCert)
{
    char                    *pUrl;
    X509                    *pCAt;
    STACK_OF(X509)          *pXchain; 
    int                     i;
    int                     n;
    
#if OPENSSL_VERSION_NUMBER >= 0x10000003L
    STACK_OF(OPENSSL_STRING)  *strResp;
#else
    STACK                    *strResp;
#endif
    if ( m_sOcspResponder.c_str() )
    {
        return 0;
    }
    strResp = X509_get1_ocsp( pCert );
    if ( strResp == NULL )
    {
        pXchain = m_pCtx->extra_certs;
        n = sk_X509_num(pXchain);
        for ( i = 0; i < n; i++ )
        {
            pCert = sk_X509_value(pXchain, i);
            strResp = X509_get1_ocsp( pCert );
            if ( strResp )
                break;
        }
    }
    if (strResp == NULL)
    {
        if ( m_sCAfile.c_str() == NULL)
            return -1;
        pCAt = load_cert( m_sCAfile.c_str() );
        if ( pCAt == NULL )
        {
            setLastErrMsg("Failed to load file: %s!\n", m_sCAfile.c_str());
            return -1;
        }
            
        strResp = X509_get1_ocsp(pCAt);
        X509_free( pCAt );
        if ( strResp == NULL )
        {
            setLastErrMsg("Failed to get responder!\n");
            return -1;
        }
    }
#if OPENSSL_VERSION_NUMBER >= 0x1000004fL
    pUrl = sk_OPENSSL_STRING_value(strResp, 0);
#elif OPENSSL_VERSION_NUMBER >= 0x10000003L
    pUrl = (char *)sk_value((const _STACK*) strResp, 0);
#else
    pUrl = (char *)sk_value((const STACK*) strResp, 0);
#endif
    if (pUrl )
    {
        m_sOcspResponder.setStr( pUrl );
        return 0;
    }
    X509_email_free( strResp );
    setLastErrMsg("Failed to get responder Url!\n");
    return -1;
}

int SslOcspStapling::getRequestData(unsigned char *pReqData)
{
    int             len = -1;
    OCSP_REQUEST    *ocsp;
    OCSP_CERTID     *id;
    
    if ( m_pCertId == NULL )
        return -1;
    ocsp = OCSP_REQUEST_new();
    if ( ocsp == NULL )
        return -1;
    
    id = OCSP_CERTID_dup(m_pCertId);
    if ( OCSP_request_add0_id(ocsp, id) != NULL )
    {
        len = i2d_OCSP_REQUEST(ocsp, &pReqData);
        if ( len > 0 )
        {
            *pReqData = 0;  
        }
    }
    OCSP_REQUEST_free(ocsp);
    return  len;
}

int SslOcspStapling::createRequest()
{
    int             len, len64, n1;
    unsigned char   *pReqData, *pReqData64;
    unsigned char   ReqData[4000], ReqData64[4000];
    struct stat     st;
    if ( ::stat( m_sRespfileTmp.c_str(), &st ) == 0 ) 
        return 0;
    pReqData = ReqData;
    pReqData64 = ReqData64;
    len = getRequestData(pReqData);
    if ( len <= 0 )
        return -1;
    len64 = Base64::encode( (const char*)ReqData, len, (char*)pReqData64);

    const char* pUrl = m_sOcspResponder.c_str();
    memcpy( pReqData,  pUrl, m_sOcspResponder.len());

    pReqData += m_sOcspResponder.len();
    if ( pUrl[m_sOcspResponder.len() -1] != '/' )
    {
        *pReqData++ = '/';
    }

    n1 = escapeBase64Uri( pReqData64, len64, pReqData);
    pReqData += n1;
    *pReqData = 0;
    len = pReqData - ReqData;
    
    if ( m_pHttpFetch != NULL )
        delete m_pHttpFetch;
    m_pHttpFetch = new HttpFetch();
    m_pHttpFetch->setCallBack( OcspRespCb, this);
    m_pHttpFetch->setTimeout( 30 );//Set Req timeout as 30 seconds
    m_pHttpFetch->startReq( (const char* )ReqData, 1, 1, NULL, 0,
                            m_sRespfileTmp.c_str(), NULL, m_addrResponder );
    setLastErrMsg("%lu, len = %d\n%s \n", m_pHttpFetch, len, ReqData);
    //printf("%s\n", s_ErrMsg.c_str());
    return 0;
}

void SslOcspStapling::updateRespData(OCSP_RESPONSE *pResponse)
{
    unsigned char * pbuff;
    m_iDataLen = i2d_OCSP_RESPONSE(pResponse, NULL);
    if ( m_iDataLen > 0)
    {
        if ( m_pRespData != NULL )
            delete [] m_pRespData;
        
        m_pRespData = new unsigned char[m_iDataLen];
        pbuff = m_pRespData;
        m_iDataLen = i2d_OCSP_RESPONSE(pResponse, &(pbuff));
        if ( m_iDataLen <= 0)
        {
            m_iDataLen = 0;
            delete [] m_pRespData;
            m_pRespData = NULL;
        }
    }    
}

int SslOcspStapling::certVerify(OCSP_RESPONSE *pResponse, OCSP_BASICRESP *pBasicResp, X509_STORE *pXstore)
{
    int                 n, iResult = -1;
    STACK_OF(X509)      *pXchain; 
    ASN1_GENERALIZEDTIME  *pThisupdate, *pNextupdate;
    struct stat         st;
    
    pXchain = m_pCtx->extra_certs;
    if ( OCSP_basic_verify(pBasicResp, pXchain, pXstore, OCSP_NOVERIFY) == 1 ) 
    {
        if ( (m_pCertId != NULL)  
             && (OCSP_resp_find_status(pBasicResp, m_pCertId, &n, 
                            NULL, NULL,&pThisupdate, &pNextupdate) == 1) 
             && (n == V_OCSP_CERTSTATUS_GOOD) 
             && (OCSP_check_validity(pThisupdate, pNextupdate, 300, -1) == 1) )
        {
            iResult = 0;
            updateRespData(pResponse);
            unlink( m_sRespfile.c_str());
            rename( m_sRespfileTmp.c_str(), m_sRespfile.c_str());
            if ( ::stat( m_sRespfile.c_str(), &st ) == 0 ) 
                m_RespTime = st.st_mtime;                
        }
    }
    if ( iResult )
    {
        setLastErrMsg( "%s", SSLError().what() );
        ERR_clear_error();
        if ( m_pHttpFetch )
            m_pHttpFetch->writeLog(s_ErrMsg.c_str());

    }
    return iResult;
}

int SslOcspStapling::verifyRespFile(int iNeedVerify)
{
    int                 iResult = -1;
    BIO                 *pBio;
    OCSP_RESPONSE       *pResponse;
    OCSP_BASICRESP      *pBasicResp;
    X509_STORE *pXstore;
    if ( iNeedVerify )
        pBio = BIO_new_file(m_sRespfileTmp.c_str(), "r");
    else
        pBio = BIO_new_file(m_sRespfile.c_str(), "r");
    if ( pBio == NULL )
        return -1;
    
    pResponse = d2i_OCSP_RESPONSE_bio(pBio, NULL);
    BIO_free( pBio );
    if ( pResponse == NULL )
        return -1;

    if ( OCSP_response_status(pResponse) == OCSP_RESPONSE_STATUS_SUCCESSFUL)
    {
        if ( iNeedVerify )
        {
            pBasicResp = OCSP_response_get1_basic(pResponse);
            if ( pBasicResp != NULL )
            {
                pXstore = SSL_CTX_get_cert_store(m_pCtx);
                if (pXstore)
                    iResult = certVerify( pResponse, pBasicResp, pXstore );
                OCSP_BASICRESP_free( pBasicResp );
            }
        }
        else
        {
            updateRespData(pResponse);
            iResult = 0;
        }
    }
    OCSP_RESPONSE_free( pResponse );
    return iResult;
}

int SslOcspStapling::getCertId(X509* pCert)
{
    int     i, n;
    X509                *pXissuer;
    X509_STORE          *pXstore;
    STACK_OF(X509)      *pXchain;
    X509_STORE_CTX      *pXstore_ctx;


    pXchain = m_pCtx->extra_certs;
    n = sk_X509_num(pXchain);
    for ( i = 0; i < n; i++ )
    {
        pXissuer = sk_X509_value(pXchain, i);
        if ( X509_check_issued(pXissuer, pCert) == X509_V_OK )
        {
            CRYPTO_add(&pXissuer->references, 1, CRYPTO_LOCK_X509);
            m_pCertId = OCSP_cert_to_id(NULL, pCert, pXissuer);
            X509_free( pXissuer );
            return 0;
        }
    }
    pXstore = SSL_CTX_get_cert_store(m_pCtx);
    if (pXstore == NULL)
    {
        setLastErrMsg("SSL_CTX_get_cert_store failed!\n");
        return -1;
    }
    pXstore_ctx = X509_STORE_CTX_new();
    if (pXstore_ctx == NULL)
    {
        setLastErrMsg("X509_STORE_CTX_new failed!\n");
        return -1;
    }
    if (X509_STORE_CTX_init(pXstore_ctx, pXstore, NULL, NULL) == 0)
    {
        setLastErrMsg("X509_STORE_CTX_init failed!\n");
        return -1;
    }
    n = X509_STORE_CTX_get1_issuer(&pXissuer, pXstore_ctx, pCert);
    X509_STORE_CTX_free(pXstore_ctx);
    if ( (n == -1) || (n == 0) )
    {
        setLastErrMsg("X509_STORE_CTX_get1_issuer failed!\n");
        return -1;
    }
    m_pCertId = OCSP_cert_to_id(NULL, pCert, pXissuer);
    X509_free( pXissuer );
    return 0;
}

void SslOcspStapling::setCertFile( const char* Certfile )
{
    char RespFile[4096];
    unsigned char md5[16];
    char md5Str[35] = {0};
    m_sCertfile.setStr( Certfile );  
    StringTool::getMd5(Certfile, strlen(Certfile), md5);
    StringTool::hexEncode((const char *)md5, 16, md5Str); 
    md5Str[32] = 0;
    RespFile[0] = 0;
    strcat(RespFile, HttpGlobals::s_pServerRoot);
    strcat(RespFile, "/tmp/ocspcache/R");
    strcat(RespFile, md5Str);
    strcat(RespFile, ".rsp");
    m_sRespfile.setStr( RespFile );
    strcat(RespFile, ".tmp");
    m_sRespfileTmp.setStr( RespFile );
}
int SslOcspStapling::config( const XmlNode *pNode, SSL_CTX *pSSL,  
                             const char *pCAFile, char *pachCert, ConfigCtx* pcurrentCtx )
{
    setCertFile( pachCert );

    if ( pCAFile )
        setCAFile( pCAFile );

    setRespMaxAge( pcurrentCtx->getLongValue( pNode, "ocspRespMaxAge", 60, 360000, 3600 ) );

    const char *pResponder = pNode->getChildValue( "ocspResponder" );
    if ( pResponder )
        setOcspResponder( pResponder );

    if ( init( pSSL ) == -1 )
    {
        pcurrentCtx->log_error( "OCSP Stapling can't be enabled [%s].", getStaplingErrMsg() );
        return -1;
    }


    return 0; 
}
