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
#include "sslconnection.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include "sslerror.h"
#include <openssl/err.h>

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <config.h>

static const char * s_pErrInvldSSL = "Invalid Parameter, SSL* ssl is null\n";

SSLConnection::SSLConnection()
    : m_ssl( NULL )
    , m_iStatus( DISCONNECTED )
    , m_iWant( 0)
{
}

SSLConnection::SSLConnection( SSL* ssl)
    : m_ssl( ssl )
    , m_iStatus( DISCONNECTED )
    , m_iWant( 0)
{
    if ( !m_ssl )
        throw SSLError(s_pErrInvldSSL);
}

SSLConnection::SSLConnection( SSL* ssl, int fd )
    : m_ssl( ssl )
    , m_iStatus( DISCONNECTED )
    , m_iWant( 0)
{
    if ( !m_ssl )
        throw SSLError(s_pErrInvldSSL);
    if ( SSL_set_fd( m_ssl, fd ) == 0 )
        throw SSLError();
}

SSLConnection::SSLConnection( SSL* ssl, int rfd, int wfd )
    : m_ssl( ssl )
    , m_iStatus( DISCONNECTED )
    , m_iWant( 0)
{
    if ( !m_ssl )
        throw SSLError(s_pErrInvldSSL);
    if (( SSL_set_rfd( m_ssl, rfd ) == 0 )||
        ( SSL_set_wfd( m_ssl, wfd ) == 0 ))
        throw SSLError();
}

SSLConnection::~SSLConnection()
{
    if ( m_ssl )
        release();
}

void SSLConnection::setSSL( SSL* ssl )
{
    assert( !m_ssl );
    //m_iWant = 0;
    m_ssl = ssl;
}
void SSLConnection::release()
{
    assert( m_ssl );
    if ( m_iStatus != DISCONNECTED )
        shutdown( 0 );
    SSL_free( m_ssl);
    m_ssl = NULL;
}

int SSLConnection::setfd( int fd )
{
    m_iWant = 0;
    int ret = SSL_set_fd( m_ssl, fd );
    if ( ret )
        m_iStatus = DISCONNECTED;
    return ret == 0;
}

int SSLConnection::setfd( int rfd, int wfd )
{
    m_iWant = 0;
    int ret = SSL_set_rfd( m_ssl, rfd );
    if ( !ret )
        return ret == 0;
    m_iStatus = DISCONNECTED;

    return SSL_set_wfd( m_ssl, wfd ) == 0;
}


int SSLConnection::read( char * pBuf, int len )
{
    assert( m_ssl );
    m_iWant = 0;
    int ret = SSL_read( m_ssl, pBuf, len );
    if ( ret > 0 )
    {
        return ret;
    }
    else
    {
        m_iWant = LAST_READ;
        return checkError( ret );
    }
}

int SSLConnection::write( const char * pBuf, int len )
{
    assert( m_ssl );
    m_iWant = 0;
    int ret = SSL_write( m_ssl, pBuf, len );
    if ( ret > 0 )
    {
        m_iWant &= ~LAST_WRITE;
        return ret;
    }
    else
    {
        m_iWant = LAST_WRITE;
        return checkError( ret );
    }
}

int SSLConnection::flush()
{
    BIO * pBIO = SSL_get_wbio( m_ssl );
    if ( !pBIO )
        return 0;
    m_iWant = 0;
    int ret = BIO_flush( pBIO );
    if ( ret != 1 )   //1 means BIO_flush succeed.
    {
        return checkError( ret );
    }
    return ret;
}

int SSLConnection::shutdown( int bidirectional)
{
    assert( m_ssl );
    if ( m_iStatus == ACCEPTING )
    {
        ERR_clear_error();
        m_iStatus = DISCONNECTED;
        return 0;
    }
    if ( m_iStatus != DISCONNECTED )
    {
        m_iWant = 0;
        SSL_set_shutdown( m_ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN );
        //SSL_set_quiet_shutdown( m_ssl, !bidirectional );
        int ret = SSL_shutdown( m_ssl );
        if (( ret )||
            (( ret == 0 )&&( !bidirectional )))
        {
            m_iStatus = DISCONNECTED;
            return 0;
        }
        else
            m_iStatus = SHUTDOWN;
        return checkError( ret );
    }
    return 0;
}

void SSLConnection::toAccept()
{
    m_iStatus = ACCEPTING;
    m_iWant = READ;
}

int SSLConnection::accept()
{
    assert( m_ssl );
    m_iWant = 0;
    int ret = SSL_accept( m_ssl );
    if ( ret == 1 )
    {
        //printf("%d\n", getSpdyVersion());
        m_iStatus = CONNECTED;
        return ret;
    }
    else
        return checkError( ret );
}

int SSLConnection::checkError( int ret)
{
    int err = SSL_get_error( m_ssl, ret );
    //printf( "SSLError:%s\n", SSLError(err).what() );
    switch( err )
    {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        if ( err == SSL_ERROR_WANT_READ )
            m_iWant |= READ;
        else
            m_iWant |= WRITE;
        ERR_clear_error();
        return 0;
    default:
        errno = EIO;
    }
    return -1;
}

int SSLConnection::connect()
{
    assert( m_ssl );
    m_iStatus = CONNECTING;
    m_iWant = 0;
    int ret = SSL_connect( m_ssl );
    if ( ret == 1 )
    {
        m_iStatus = CONNECTED;
        return 1;
    }
    else
        return checkError( ret );
}

int SSLConnection::tryagain()
{
    assert( m_ssl );
    switch( m_iStatus )
    {
    case CONNECTING:
        return connect();
    case ACCEPTING:
        return accept();
    case SHUTDOWN:
        return shutdown( 1 );
    }
    return 0;
}

X509 * SSLConnection::getPeerCertificate() const
{   return SSL_get_peer_certificate( m_ssl );   }

long SSLConnection::getVerifyResult() const
{   return SSL_get_verify_result( m_ssl );      }

int SSLConnection::getVerifyMode() const
{   return SSL_get_verify_mode( m_ssl );        }



const char * SSLConnection::getCipherName() const
{   return SSL_get_cipher_name( m_ssl );    }

const SSL_CIPHER * SSLConnection::getCurrentCipher() const
{   return SSL_get_current_cipher( m_ssl ); }

SSL_SESSION * SSLConnection::getSession() const
{   return SSL_get_session( m_ssl );        }

const char * SSLConnection::getVersion() const
{   return SSL_get_version( m_ssl );        }

static const char NPN_SPDY_PREFIX[] = { 's', 'p', 'd', 'y', '/' };
int SSLConnection::getSpdyVersion()
{
    int v = 0;
    
#ifdef LS_ENABLE_SPDY
#ifdef TLSEXT_TYPE_next_proto_neg
    unsigned int             len;
    const unsigned char     *data;
    SSL_get0_next_proto_negotiated(m_ssl, &data, &len);
    if (len > sizeof( NPN_SPDY_PREFIX ) && 
        strncasecmp((const char *)data, NPN_SPDY_PREFIX, sizeof( NPN_SPDY_PREFIX ) ) == 0) 
    {
        v = data[ sizeof( NPN_SPDY_PREFIX ) ] - '1';
        if (( v == 2 )&&( len >= 8 )&&( data[6] == '.')&&( data[7] == '1')) 
             v = 3;
        return v;
    }
#endif
#endif
    return v;
}

int SSLConnection::getSessionIdLen( SSL_SESSION * s )
{   return s->session_id_length;     }

const unsigned char * SSLConnection::getSessionId( SSL_SESSION * s )
{   return s->session_id;           }

int SSLConnection::getCipherBits( const SSL_CIPHER * pCipher, int *algkeysize )
{
    return SSL_CIPHER_get_bits( (SSL_CIPHER *)pCipher, algkeysize );
}

int SSLConnection::isClientVerifyOptional( int i )
{   return i == SSL_VERIFY_PEER;    }

int SSLConnection::isVerifyOk() const
{   return SSL_get_verify_result( m_ssl ) == X509_V_OK;     }

int SSLConnection::buildVerifyErrorString( char * pBuf, int len ) const
{
    return snprintf( pBuf, len, "FAILED: %s", X509_verify_cert_error_string(
                        SSL_get_verify_result( m_ssl )) );
}
