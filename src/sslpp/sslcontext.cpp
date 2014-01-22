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
#include "sslcontext.h"
#include "sslconnection.h"
#include "sslerror.h"

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <config.h>
#include "sslocspstapling.h"
//#include "http/vhostmap.h"


long SSLContext::setOptions( long options )
{
    return SSL_CTX_set_options( m_pCtx, options );
}

long SSLContext::setSessionCacheMode( long mode )
{
    return SSL_CTX_set_session_cache_mode( m_pCtx, mode );
}

/* default to 1024*20 = 20K */
long SSLContext::setSessionCacheSize( long size )
{
    return SSL_CTX_sess_set_cache_size( m_pCtx, size );
}

/* default to 300 */
long SSLContext::setSessionTimeout( long timeout )
{
    return SSL_CTX_set_timeout( m_pCtx, timeout );
}

int SSLContext::seedRand(int len)
{
    static int fd = open( "/dev/urandom", O_RDONLY|O_NONBLOCK );
    char achBuf[2048];
    int ret;
    if ( fd >= 0 )
    {
        int toRead;
        do
        {
            toRead = sizeof( achBuf );
            if ( len < toRead )
                toRead = len;
            ret = read( fd, achBuf, toRead );
            if ( ret > 0 )
            {
                RAND_seed( achBuf, ret );
                len -= ret;
            }
        }while((len > 0 ) &&( ret == toRead ));
        fcntl( fd, F_SETFD, FD_CLOEXEC );
        //close( fd );
    }
    else
    {
 #ifdef DEVRANDOM_EGD
        /* Use an EGD socket to read entropy from an EGD or PRNGD entropy
         * collecting daemon. */
        static const char * egdsockets[] = { "/var/run/egd-pool", "/dev/egd-pool", "/etc/egd-pool" };
        for (egdsocket = egdsockets; *egdsocket && n < len; egdsocket++)
        {

            ret = RAND_egd_bytes(*egdsocket, len);
            if (ret > 0)
                len -= ret;
        }
#endif
    }
    if ( len == 0 )
        return 0;
    if ( len > (int)sizeof( achBuf ) )
        len = (int)sizeof( achBuf );
    gettimeofday( (timeval *)achBuf, NULL);
    ret = sizeof( timeval );
    *(pid_t *)(achBuf + ret ) = getpid();   
    ret += sizeof( pid_t );
    *(uid_t *)( achBuf + ret ) = getuid();
    ret += sizeof( uid_t );
    if ( len > ret )
        memmove( achBuf + ret, achBuf + sizeof( achBuf ) - len + ret, len - ret );
    RAND_seed( achBuf, len );
    return 0;
}

static void SSLConnection_ssl_info_cb( const SSL *pSSL, int where, int ret)
{
    //if ((where & SSL_CB_HANDSHAKE_START) != 0) 
    //{
    //    //close connection, may not needed for 0.9.8m and later
    //}
    if ((where & SSL_CB_HANDSHAKE_DONE) != 0) 
    {
         pSSL->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
    }
}

void SSLContext::setProtocol( int method )
{
    if ( method == m_iMethod )
        return;
    if ( ( method & SSL_ALL ) == 0 )
        return;
    m_iMethod = method;
    updateProtocol( method );
}


void SSLContext::updateProtocol( int method )
{
    setOptions( SSL_OP_NO_SSLv2 );
    if ( !(method & SSL_v3) )
        setOptions( SSL_OP_NO_SSLv3 );
    if ( !(method & SSL_TLSv1 ) ) 
        setOptions( SSL_OP_NO_TLSv1 );
#ifdef SSL_OP_NO_TLSv1_1
    if ( !(method & SSL_TLSv11 ) )
        setOptions( SSL_OP_NO_TLSv1_1 );
#endif
#ifdef SSL_OP_NO_TLSv1_2
    if ( !(method & SSL_TLSv12 ) )
        setOptions( SSL_OP_NO_TLSv1_2 );
#endif
}

static DH * s_pDH1024 = NULL;


/*
-----BEGIN DH PARAMETERS-----
MIGHAoGBALgPB5cuGaCX/AxfsOApWEZ+8PTkY5aeRImkDvq6XNjG/slJfPxREEyD
IN/caV2MzgE24AirvYKAzij24hSZmRIfWxcHn3NLfpD5LOdOk1t+GcaTivCILxmh
1pkfHj0949REDcZFxVYMxIpxlwvgYOxYpDfLzsA8mnkJqKmWpNqzAgEC
-----END DH PARAMETERS-----
*/
static unsigned char dh1024_p[] = 
{
    0xB8,0x0F,0x07,0x97,0x2E,0x19,0xA0,0x97,0xFC,0x0C,0x5F,0xB0,
    0xE0,0x29,0x58,0x46,0x7E,0xF0,0xF4,0xE4,0x63,0x96,0x9E,0x44,
    0x89,0xA4,0x0E,0xFA,0xBA,0x5C,0xD8,0xC6,0xFE,0xC9,0x49,0x7C,
    0xFC,0x51,0x10,0x4C,0x83,0x20,0xDF,0xDC,0x69,0x5D,0x8C,0xCE,
    0x01,0x36,0xE0,0x08,0xAB,0xBD,0x82,0x80,0xCE,0x28,0xF6,0xE2,
    0x14,0x99,0x99,0x12,0x1F,0x5B,0x17,0x07,0x9F,0x73,0x4B,0x7E,
    0x90,0xF9,0x2C,0xE7,0x4E,0x93,0x5B,0x7E,0x19,0xC6,0x93,0x8A,
    0xF0,0x88,0x2F,0x19,0xA1,0xD6,0x99,0x1F,0x1E,0x3D,0x3D,0xE3,
    0xD4,0x44,0x0D,0xC6,0x45,0xC5,0x56,0x0C,0xC4,0x8A,0x71,0x97,
    0x0B,0xE0,0x60,0xEC,0x58,0xA4,0x37,0xCB,0xCE,0xC0,0x3C,0x9A,
    0x79,0x09,0xA8,0xA9,0x96,0xA4,0xDA,0xB3,
};

static DH * getTmpDhParam()
{
    unsigned char dh1024_g[] = { 0x02 };
    if ( s_pDH1024 )
        return s_pDH1024;

    if ((s_pDH1024=DH_new()) != NULL)
    {
        s_pDH1024->p=BN_bin2bn(dh1024_p,sizeof(dh1024_p),NULL);
        s_pDH1024->g=BN_bin2bn(dh1024_g,sizeof(dh1024_g),NULL);
        if ((s_pDH1024->p == NULL) || (s_pDH1024->g == NULL))
        {
            DH_free(s_pDH1024);
            s_pDH1024 = NULL;
        }
    }
    return s_pDH1024;
}

int SSLContext::initDH( const char * pFile )
{
    DH *pDH = NULL;
    if ( pFile )
    {
        BIO *bio;
        if ((bio = BIO_new_file(pFile, "r")) != NULL)
        {
            pDH = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
            BIO_free(bio);
        }
    }
    if ( !pDH )
    {
        pDH = getTmpDhParam();
        if ( !pDH )
            return -1;
    }
    SSL_CTX_set_tmp_dh( m_pCtx, pDH );
    if ( pDH != s_pDH1024 )
        DH_free( pDH ); 
    SSL_CTX_set_options( m_pCtx, SSL_OP_SINGLE_DH_USE);
    return 0;
}

int SSLContext::initECDH()
{
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
#ifndef OPENSSL_NO_ECDH
    EC_KEY *ecdh; 
    ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1); 
    if (ecdh == NULL)
        return -1;
    SSL_CTX_set_tmp_ecdh(m_pCtx,ecdh); 
    EC_KEY_free(ecdh);

    SSL_CTX_set_options(m_pCtx, SSL_OP_SINGLE_ECDH_USE);

#endif
#endif
    return 0;
}

int SSLContext::init( int iMethod )
{
    if ( m_pCtx != NULL )
        return 0;
    SSL_METHOD * meth;
    if ( initSSL() )
        return -1;
    m_iMethod = iMethod;
    m_iEnableSpdy = 0;
    meth = (SSL_METHOD *)SSLv23_method();
    m_pCtx = SSL_CTX_new (meth);
    if ( m_pCtx )
    {
#ifdef SSL_OP_NO_COMPRESSION
        /* OpenSSL >= 1.0 only */
        SSL_CTX_set_options(m_pCtx, SSL_OP_NO_COMPRESSION);
#endif
        setOptions( SSL_OP_SINGLE_DH_USE|SSL_OP_ALL );
        //setOptions( SSL_OP_NO_SSLv2 );
        updateProtocol( iMethod );

        setOptions( SSL_OP_CIPHER_SERVER_PREFERENCE);

        //increase defaults
        setSessionTimeout( 100800 ); 
        setSessionCacheSize ( 1024 * 40 ); 

        
        SSL_CTX_set_mode( m_pCtx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER
#ifdef SSL_MODE_RELEASE_BUFFERS
            |SSL_MODE_RELEASE_BUFFERS
#endif
        );
        if ( m_iRenegProtect )
        {
            setOptions( SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION );
            SSL_CTX_set_info_callback( m_pCtx, SSLConnection_ssl_info_cb );
        }
        //initECDH();

        return 0;
    }
    else
    {
        //FIXME: log ssl error
        return -1;
    }
}

SSLContext::SSLContext( int iMethod )
    : m_pCtx( NULL )
    , m_iMethod( iMethod )
    , m_iRenegProtect( 1 )
    , m_iEnableSpdy( 0 )
    , m_pStapling( NULL )
{
}
SSLContext::~SSLContext()
{
    release();
}


void SSLContext::release()
{
    if ( m_pCtx != NULL )
    {
        SSL_CTX* pCtx = m_pCtx;
        m_pCtx = NULL;
        SSL_CTX_free( pCtx );
    }
    if ( m_pStapling )
        delete m_pStapling;
}


SSL* SSLContext::newSSL()
{
    init( m_iMethod );
    seedRand( 128 );
    return SSL_new(m_pCtx);
}

static int translateType( int type )
{
    switch( type )
    {
    case SSLContext::FILETYPE_PEM:
        return SSL_FILETYPE_PEM;
    case SSLContext::FILETYPE_ASN1:
        return SSL_FILETYPE_ASN1;
    default:
        return -1;
    }
}

static int isFileChanged( const char * pFile, const struct stat &stOld )
{
    struct stat st;
    if ( ::stat( pFile, &st ) == -1 )
        return 0;
    return ((st.st_size != stOld.st_size)||
            (st.st_ino != stOld.st_ino )||
            (st.st_mtime != stOld.st_mtime ));
}

int SSLContext::isKeyFileChanged( const char * pKeyFile ) const
{
    return isFileChanged( pKeyFile, m_stKey );
}

int SSLContext::isCertFileChanged( const char * pCertFile ) const
{
    return isFileChanged( pCertFile, m_stCert );
}

int SSLContext::setKeyCertificateFile( const char * pFile, int iType, int chained )
{
    return setKeyCertificateFile( pFile, iType, pFile, iType, chained );
}

int SSLContext::setKeyCertificateFile( const char * pKeyFile, int iKeyType,
                                        const char * pCertFile, int iCertType,
                                        int chained )
{
    if ( !setCertificateFile( pCertFile, iCertType, chained ) )
        return 0;
    if ( !setPrivateKeyFile( pKeyFile, iKeyType ) )
        return 0;
    return  SSL_CTX_check_private_key( m_pCtx );
}

int SSLContext::setCertificateFile( const char * pFile, int type, int chained )
{
    if ( !pFile )
        return 0;
    ::stat( pFile, &m_stCert );
    if ( init( m_iMethod ) )
        return 0;

   // m_sCertfile.setStr( pFile );
    if ( chained )
        return SSL_CTX_use_certificate_chain_file( m_pCtx, pFile );
    else
        return SSL_CTX_use_certificate_file( m_pCtx, pFile,
            translateType( type ) );
}


int SSLContext::setCertificateChainFile( const char * pFile )
{
    BIO *bio;
    X509 *x509;
    STACK_OF(X509) * pExtraCerts;
    unsigned long err;
    int n;

    if ((bio = BIO_new(BIO_s_file_internal())) == NULL)
        return 0;
    if (BIO_read_filename(bio, pFile) <= 0) {
        BIO_free(bio);
        return 0;
    }
    pExtraCerts=m_pCtx->extra_certs;
    if ( pExtraCerts != NULL) 
    {
        sk_X509_pop_free((STACK_OF(X509) *)pExtraCerts, X509_free);
        m_pCtx->extra_certs = NULL;
    }
    n = 0;
    while ((x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL)) != NULL) 
    {
        if (!SSL_CTX_add_extra_chain_cert(m_pCtx, x509)) 
        {
            X509_free(x509);
            BIO_free(bio);
            return 0;
        }
        n++;
    }
    if ((err = ERR_peek_error()) > 0) 
    {
        if (!(   ERR_GET_LIB(err) == ERR_LIB_PEM
              && ERR_GET_REASON(err) == PEM_R_NO_START_LINE)) 
        {
            BIO_free(bio);
            return 0;
        }
        while (ERR_get_error() > 0) ;
    }
    //m_sCertfile.setStr( pFile );
    BIO_free(bio);
    return n > 0;
}



int SSLContext::setCALocation( const char * pCAFile, const char * pCAPath, int cv )
{
    int ret; 
    if ( init( m_iMethod ) )
        return -1;
    ret = SSL_CTX_load_verify_locations(m_pCtx, pCAFile, pCAPath);
    if ( (ret != 0) && cv )
    {

        //m_sCAfile.setStr( pCAFile );
        
        ret = SSL_CTX_set_default_verify_paths( m_pCtx );
        STACK_OF(X509_NAME) *pCAList = NULL;
        if ( pCAFile ) 
        {
            pCAList = SSL_load_client_CA_file( pCAFile );
        }
        if ( pCAPath )
        {
            if ( !pCAList ) 
            {
                pCAList = sk_X509_NAME_new_null();
            }
            if ( !SSL_add_dir_cert_subjects_to_stack( pCAList, pCAPath ) )
            {
                sk_X509_NAME_free( pCAList );
                pCAList = NULL;
            }
        }
        if ( pCAList )
            SSL_CTX_set_client_CA_list( m_pCtx, pCAList );
    }

    return ret;
}

int SSLContext::setPrivateKeyFile( const char * pFile, int type )
{
    if ( !pFile )
        return 0;
    ::stat( pFile, &m_stKey );
    if ( init( m_iMethod ) )
        return 0;
    return SSL_CTX_use_PrivateKey_file( m_pCtx, pFile,
            translateType( type ) );
}

int SSLContext::checkPrivateKey()
{
    if ( m_pCtx )
        return SSL_CTX_check_private_key( m_pCtx );
    else
        return 0;
}

int SSLContext::setCipherList( const char * pList )
{
    if ( m_pCtx )
    {
        char cipher[4096];

        if ( (strncasecmp( pList, "ALL:", 4 ) == 0 )
            ||(strncasecmp( pList, "SSLv3:", 6 ) == 0 )
            ||(strncasecmp( pList, "TLSv1:", 6 ) == 0 ))
        {
            //snprintf( cipher, 4095, "RC4:%s", pList );
            //strcpy( cipher, "ALL:HIGH:!aNULL:!SSLV2:!eNULL" );
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
            strcpy( cipher, "ECDHE-RSA-AES128-SHA256:AES128-GCM-SHA256:RC4:HIGH:!MD5:!aNULL:!EDH" );

#else
            strcpy( cipher, "RC4:HIGH:!aNULL:!MD5:!EDH" );
#endif
            //strcpy( cipher, "RC4:-EXP:-SSLv2:-ADH" );
            pList = cipher;
        }

        return SSL_CTX_set_cipher_list( m_pCtx, pList );
    }
    else
        return 0;
}



/*
SL_CTX_set_verify(ctx, nVerify,  ssl_callback_SSLVerify);
    SSL_CTX_sess_set_new_cb(ctx,      ssl_callback_NewSessionCacheEntry);
    SSL_CTX_sess_set_get_cb(ctx,      ssl_callback_GetSessionCacheEntry);
    SSL_CTX_sess_set_remove_cb(ctx,   ssl_callback_DelSessionCacheEntry);
    SSL_CTX_set_tmp_rsa_callback(ctx, ssl_callback_TmpRSA);
    SSL_CTX_set_tmp_dh_callback(ctx,  ssl_callback_TmpDH);
    SSL_CTX_set_info_callback(ctx,    ssl_callback_LogTracingState);
*/

int SSLContext::initSSL()
{
    SSL_load_error_strings();
    SSL_library_init();
#ifndef SSL_OP_NO_COMPRESSION
    /* workaround for OpenSSL 0.9.8 */
    sk_SSL_COMP_zero(SSL_COMP_get_compression_methods());
#endif
    return seedRand( 512 );
}

/*
static RSA *load_key(const char *file, char *pass, int isPrivate )
{
    BIO * bio_err = BIO_new_fp( stderr, BIO_NOCLOSE );
    BIO *bp = NULL;
    EVP_PKEY *pKey = NULL;
    RSA *pRSA = NULL;

    bp=BIO_new(BIO_s_file());
    if (bp == NULL)
    {
        return NULL;
    }
    if (BIO_read_filename(bp,file) <= 0)
    {
        BIO_free( bp );
        return NULL;
    }
    if ( !isPrivate )
        pKey = PEM_read_bio_PUBKEY( bp, NULL, NULL, pass);
    else
        pKey = PEM_read_bio_PrivateKey( bp, NULL, NULL, pass );
    if ( !pKey )
    {
        ERR_print_errors( bio_err );
    }
    else
    {
        pRSA = EVP_PKEY_get1_RSA( pKey );
        EVP_PKEY_free( pKey );
    }
    if (bp != NULL)
        BIO_free(bp);
    if ( bio_err )
        BIO_free( bio_err );
    return(pRSA);
}
*/

static RSA *load_key(const unsigned char *key, int keyLen, char *pass, int isPrivate )
{
    BIO * bio_err = BIO_new_fp( stderr, BIO_NOCLOSE );
    BIO * bp = NULL;
    EVP_PKEY *pKey = NULL;
    RSA *pRSA = NULL;

    bp = BIO_new_mem_buf( (void *)key, keyLen );
    if (bp == NULL)
    {
        return NULL;
    }
    if ( !isPrivate )
        pKey = PEM_read_bio_PUBKEY( bp, NULL, NULL, pass);
    else
        pKey = PEM_read_bio_PrivateKey( bp, NULL, NULL, pass );
    if ( !pKey )
    {
        ERR_print_errors( bio_err );
    }
    else
    {
        pRSA = EVP_PKEY_get1_RSA( pKey );
        EVP_PKEY_free( pKey );
    }
    if (bp != NULL)
        BIO_free(bp);
    if ( bio_err )
        BIO_free( bio_err );
    return(pRSA);
}


int  SSLContext::publickey_encrypt( const unsigned char * pPubKey, int keylen,
            const char * content, int len, char * encrypted, int bufLen )
{
    int ret;
    initSSL();
    RSA * pRSA = load_key( pPubKey, keylen, NULL, 0 );
    if ( pRSA )
    {
        if ( bufLen < RSA_size( pRSA) )
            return -1;
        ret = RSA_public_encrypt(len, (unsigned char *)content,
            (unsigned char *)encrypted, pRSA, RSA_PKCS1_OAEP_PADDING );
        RSA_free( pRSA );
        return ret;
    }
    else
        return -1;
    
}

int  SSLContext::publickey_decrypt( const unsigned char * pPubKey, int keylen,
                    const char * encrypted, int len, char * decrypted, int bufLen )
{
    int ret;
    initSSL();
    RSA * pRSA = load_key( pPubKey, keylen, NULL, 0 );
    if ( pRSA )
    {
        if ( bufLen < RSA_size( pRSA) )
            return -1;
        ret = RSA_public_decrypt(len, (unsigned char *)encrypted,
            (unsigned char *)decrypted, pRSA, RSA_PKCS1_PADDING );
        RSA_free( pRSA );
        return ret;
    }
    else
        return -1;
}

/*
int  SSLContext::privatekey_encrypt( const char * pPrivateKeyFile, const char * content,
                    int len, char * encrypted, int bufLen )
{
    int ret;
    initSSL();
    RSA * pRSA = load_key( pPrivateKeyFile, NULL, 1 );
    if ( pRSA )
    {
        if ( bufLen < RSA_size( pRSA) )
            return -1;
        ret = RSA_private_encrypt(len, (unsigned char *)content,
            (unsigned char *)encrypted, pRSA, RSA_PKCS1_PADDING );
        RSA_free( pRSA );
        return ret;
    }
    else
        return -1;
}
int  SSLContext::privatekey_decrypt( const char * pPrivateKeyFile, const char * encrypted,
                    int len, char * decrypted, int bufLen )
{
    int ret;
    initSSL();
    RSA * pRSA = load_key( pPrivateKeyFile, NULL, 1 );
    if ( pRSA )
    {
        if ( bufLen < RSA_size( pRSA) )
            return -1;
        ret = RSA_private_decrypt(len, (unsigned char *)encrypted,
            (unsigned char *)decrypted, pRSA, RSA_PKCS1_OAEP_PADDING );
        RSA_free( pRSA );
        return ret;
    }
    else
        return -1;
}
*/

/*
    ASSERT (options->ca_file || options->ca_path);
    if (!SSL_CTX_load_verify_locations (ctx, options->ca_file, options->ca_path))
    msg (M_SSLERR, "Cannot load CA certificate file %s path %s (SSL_CTX_load_verify_locations)", options->ca_file, options->ca_path);

    // * Set a store for certs (CA & CRL) with a lookup on the "capath" hash directory * /
    if (options->ca_path) {
        X509_STORE *store = SSL_CTX_get_cert_store(ctx);

        if (store) 
        {
            X509_LOOKUP *lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
            if (!X509_LOOKUP_add_dir(lookup, options->ca_path, X509_FILETYPE_PEM))
                X509_LOOKUP_add_dir(lookup, NULL, X509_FILETYPE_DEFAULT);
            else
                msg(M_WARN, "WARNING: experimental option --capath %s", options->ca_path);
#if OPENSSL_VERSION_NUMBER >= 0x00907000L
            X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
#else
#warn This version of OpenSSL cannot handle CRL files in capath 
            msg(M_WARN, "WARNING: this version of OpenSSL cannot handle CRL files in capath");
#endif
        }
        else
            msg(M_SSLERR, "Cannot get certificate store (SSL_CTX_get_cert_store)");
    }
*/
extern SSLContext * VHostMapFindSSLContext( void * arg, const char * pName );
static int SSLConnection_ssl_servername_cb( SSL * pSSL, int *ad, void *arg )
{
#ifdef SSL_TLSEXT_ERR_OK
    const char * servername = SSL_get_servername( pSSL, TLSEXT_NAMETYPE_host_name );
    if ( !servername || !*servername )
        return SSL_TLSEXT_ERR_NOACK;
    SSLContext * pCtx = VHostMapFindSSLContext( arg, servername );
    if ( !pCtx )
        return SSL_TLSEXT_ERR_NOACK;
    SSL_set_SSL_CTX( pSSL, pCtx->get() );
    return SSL_TLSEXT_ERR_OK;
#else
    return -1;
#endif
}

int SSLContext::initSNI( void * param )
{
#ifdef SSL_TLSEXT_ERR_OK
    SSL_CTX_set_tlsext_servername_callback( m_pCtx, SSLConnection_ssl_servername_cb );
    SSL_CTX_set_tlsext_servername_arg( m_pCtx, param );
    
    return 0;
#else
    return -1;
#endif
}

/*!
    \fn SSLContext::setClientVerify( int mode, int depth)
 */
void SSLContext::setClientVerify( int mode, int depth)
{
    int req;
    switch( mode )
    {
    case 0:     //none
        req = SSL_VERIFY_NONE;
        break;
    case 1:     //optional
    case 3:     //no_ca
        req = SSL_VERIFY_PEER;
        break;
    case 2:     //required
    default:
        req = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE;
    }
    SSL_CTX_set_verify(m_pCtx, req, NULL );
    SSL_CTX_set_verify_depth(m_pCtx, depth);
    SSL_CTX_set_session_id_context( m_pCtx, (const unsigned char *)"litespeed", 9 );
}


/*!
    \fn SSLContext::addCRL( const char * pCRLFile, const char * pCRLPath)
 */
int SSLContext::addCRL( const char * pCRLFile, const char * pCRLPath)
{
    X509_STORE *store = SSL_CTX_get_cert_store(m_pCtx);
    X509_LOOKUP *lookup = X509_STORE_add_lookup(store, X509_LOOKUP_hash_dir());
    if ( pCRLFile )
    {
        if ( !X509_load_crl_file( lookup, pCRLFile, X509_FILETYPE_PEM ) )
        {
            return -1;
        }
    }
    if ( pCRLPath )
    {
        if (!X509_LOOKUP_add_dir( lookup, pCRLPath, X509_FILETYPE_PEM) )
            return -1;
    }
    X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    return 0;
}


#ifdef LS_ENABLE_SPDY
static const char * NEXT_PROTO_STRING[3] = 
{
    "\x06spdy/2\x08http/1.1",
    "\x08spdy/3.1\x06spdy/3\x08http/1.1",
    "\x08spdy/3.1\x06spdy/3\x06spdy/2\x08http/1.1" 
};

static int NEXT_PROTO_STRING_LEN[3] =
{
    16, 25, 32
};

//static const char NEXT_PROTO_STRING[] = "\x06spdy/2\x08http/1.1\x08http/1.0";

static int SSLConnection_ssl_npn_advertised_cb(SSL *pSSL, const unsigned char **out, 
                                 unsigned int *outlen, void *arg)
{
    SSLContext * pCtx = (SSLContext *)arg;
    *out = (const unsigned char *)NEXT_PROTO_STRING[ pCtx->getEnableSpdy() - 1 ];
    *outlen = NEXT_PROTO_STRING_LEN[ pCtx->getEnableSpdy() - 1 ];
    return SSL_TLSEXT_ERR_OK;
}


int SSLContext::enableSpdy( int level )
{
    m_iEnableSpdy = ( level & 3 );
    if ( m_iEnableSpdy == 0 )
        return 0;
#ifdef TLSEXT_TYPE_next_proto_neg
    SSL_CTX_set_next_protos_advertised_cb(m_pCtx, SSLConnection_ssl_npn_advertised_cb, this);
#else
    #error "Openssl version is too low (openssl 1.0.1 or higher is required)!!!"
#endif
    return 0;
}

#else
    int SSLContext::enableSpdy( int level )
    {
        return -1;
    }

#endif


static int sslCertificateStatus_cb(SSL *ssl, void *data)
{
    SslOcspStapling* pStapling = (SslOcspStapling*)data;
    return pStapling->callback( ssl );
}

#include <util/xmlnode.h>

int SSLContext::configStapling(const XmlNode *pNode,  
                             const char *pCAFile, char *pachCert, ConfigCtx* pcurrentCtx)
{
    SslOcspStapling *pSslOcspStapling = new SslOcspStapling;

    if (pSslOcspStapling->config(pNode, m_pCtx, pCAFile, pachCert, pcurrentCtx) == -1)
    {
        delete pSslOcspStapling;
        return -1;
    }
    setStapling( pSslOcspStapling ) ;

    SSL_CTX_set_tlsext_status_cb(m_pCtx, sslCertificateStatus_cb);
    SSL_CTX_set_tlsext_status_arg(m_pCtx, m_pStapling);    

    return 0; 
}
