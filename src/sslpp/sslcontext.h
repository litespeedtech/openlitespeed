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
#ifndef SSLCONTEXT_H
#define SSLCONTEXT_H


#include <sys/stat.h>
#include <util/autostr.h>
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

class SslOcspStapling;
class ConfigCtx;
class XmlNode;

class SSLContext
{
private:
    SSL_CTX*    m_pCtx;
    short       m_iMethod;
    char        m_iRenegProtect;
    char        m_iEnableSpdy;
    struct stat m_stKey;
    struct stat m_stCert;
    
    SslOcspStapling * m_pStapling;
    
    SSLContext( const SSLContext& rhs ) {}
    void operator=( const SSLContext& rhs ) {}

    void release();
    int init( int method = SSL_ALL);
    static int seedRand(int len);
    void updateProtocol( int method );

public:
    enum
    {
        SSL_v3      = 1,
        SSL_TLSv1   = 2,
        SSL_TLSv11  = 4,
        SSL_TLSv12  = 8,
        SSL_ALL     = 15
    };
    enum
    {
        FILETYPE_PEM,
        FILETYPE_ASN1
    };

    explicit SSLContext( int method = SSL_ALL );
    ~SSLContext();
    SSL_CTX* get() const    {   return m_pCtx;  }
    SSL* newSSL();
    int setKeyCertificateFile( const char * pKeyCertFile, int iType,
                               int chained );
    int setKeyCertificateFile( const char * pKeyFile, int iKeyType,
                               const char * pCertFile, int iCertType,
                               int chained );
    int setCertificateFile( const char * pFile, int type, int chained );
    int setCertificateChainFile( const char * pFile );
    int setPrivateKeyFile( const char * pFile, int type );
    int checkPrivateKey();
    long setOptions( long options );
    long setSessionCacheMode( long mode );
    long setSessionCacheSize( long size );
    long setSessionTimeout( long timeout );
    void setProtocol( int method );
    void setRenegProtect( int p )   {   m_iRenegProtect = p;    }
    int  setCipherList( const char * pList );
    int  setCALocation( const char * pCAFile, const char * pCAPath, int cv );

    int  isKeyFileChanged( const char * pKeyFile ) const;
    int  isCertFileChanged( const char * pCertFile ) const;

    int initSNI( void * param );

    static int  initSSL();
    
    static int  publickey_encrypt( const unsigned char * pPubKey, int keylen,
                const char * content,
                 int len, char * encrypted, int bufLen );
    static int  publickey_decrypt( const unsigned char * pPubKey, int keylen,
                const char * encrypted,
                    int len, char * decrypted, int bufLen );
    void setClientVerify( int mode, int depth);
    int addCRL( const char * pCRLFile, const char * pCRLPath);
    int enableSpdy( int level );
    int getEnableSpdy() const   {   return m_iEnableSpdy;   } 
    SslOcspStapling * getStapling () {  return m_pStapling; }
    void setStapling (SslOcspStapling * pSslOcspStapling) {  m_pStapling = pSslOcspStapling;}
    int configStapling(const XmlNode *pNode,  
                       const char *pCAFile, char *pachCert, ConfigCtx* pcurrentCtx);
    int  initECDH();
    int  initDH( const char * pFile );
};
#endif
