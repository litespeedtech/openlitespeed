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

#include "edluastream.h"
#include <edio/multiplexer.h>
#include <../addon/include/ls.h>
#include <socket/coresocket.h>
#include <socket/gsockaddr.h>

#include <util/loopbuf.h>
#include <util/datetime.h>

#include <modules/lua/lsluaengine.h>
#include <modules/lua/lsluaapi.h>

#include "ls_lua.h"
#include "lsluasession.h"

#include <fcntl.h>


EdLuaStream::EdLuaStream()
    : m_pRecvState( NULL )
    , m_pSendState( NULL )
    , m_bufOut( 4096 )
    , m_bufIn( 4096 )
    , m_iFlag( LUA_STREAM_NONE )
    , m_iCurInPos( 0 )
    , m_iToRead( EDLUA_STREAM_READ_LINE )
    , m_iToSend( 0 )
    , m_iTimeoutMs( 10000 )
    , m_iRecvTimeout( 0 )
    , m_iSendTimeout( 0 )
{

}

EdLuaStream::~EdLuaStream()
{
}

inline int64_t getCurTimeMs()
{
    int32_t usec;
    time_t t;
    t = g_api->get_cur_time(&usec);

    return ( int64_t ) t * 1000 + usec / 1000;
}

static int build_lua_socket_error_ret( lua_State * pState, int error )
{
    char achError[1024] = "socket error: ";
    strerror_r( error, &achError[14], 1000 );
    LsLuaEngine::api()->pushnil( pState );
    LsLuaEngine::api()->pushstring( pState, achError );
    return 2;
}

int EdLuaStream::resume( lua_State * &pState, int nArg )
{
    lua_State * p = pState;
    pState = NULL;

    LsLuaEngine::api()->resumek( p, 0, nArg );
    return nArg;
}

int EdLuaStream::resumeWithError( lua_State * &pState, int flag, int errcode )
{
    int ret;
    m_iFlag &= ~flag;
    ret = build_lua_socket_error_ret( pState, errcode );
    resume( pState, ret );
    return ret;
}


int EdLuaStream::connectTo( lua_State * pState, const char * pAddr, uint16_t port )
{
    int fd;
    int ret;
    GSockAddr sockAddr;
    Multiplexer * pMplx = (Multiplexer *)g_api->get_multiplexer();
    if ( sockAddr.parseAddr( pAddr ) == -1 )
    {
        LsLuaEngine::api()->pushnil( pState );
        LsLuaEngine::api()->pushstring( pState, "Bad address" );
        return 2;
    }
    sockAddr.setPort( port );

    ret = CoreSocket::connect( sockAddr, pMplx->getFLTag(), &fd, 1 );
    if ( fd != -1 )
    {
        LsLua_log( pState, LSI_LOG_DEBUG, 0, "[EDLuaStream][%p] connecting to [%s]...",
                     this, pAddr );
        ::fcntl( fd, F_SETFD, FD_CLOEXEC );
        if ( ret == 0 )
        {
            init( fd, pMplx, POLLHUP | POLLERR );
            m_iFlag |= LUA_STREAM_CONNECTED;
            // pop LUA stack, return "connect finished successful"
            LsLuaEngine::api()->pushinteger( m_pSendState, 1 );
            return 1;
        }
        else
        {
            init( fd, pMplx, POLLIN | POLLOUT | POLLHUP | POLLERR );
            m_iFlag |= LUA_STREAM_CONNECTING;
            // suspend LUA coroutine
            // m_iSendTimeout = ( int64_t )DateTime::s_curTime * 1000 + DateTime::s_curTimeUs / 1000 + m_iTimeoutMs;
            m_iSendTimeout = getCurTimeMs() + m_iTimeoutMs;
            m_pSendState = pState;

            return LsLuaEngine::api()->yieldk( m_pSendState, 0, 0, NULL );
        }
    }
    return build_lua_socket_error_ret( pState, errno );

}

int EdLuaStream::onInitialConnected()
{
    int n = 0;
    int error;
    int ret = getSockError( &error );
    m_iFlag &= ~LUA_STREAM_CONNECTING;
    if ( ( ret == -1 ) || ( error != 0 ) )
    {
        if ( ret != -1 )
            errno = error;
        // pop LUA stack, return "connect failure"
        n = build_lua_socket_error_ret( m_pSendState, errno );
    }
    else
    {
        m_iFlag |= LUA_STREAM_CONNECTED;
        /*
        if ( D_ENABLED( DL_LESS ) )
        {
            char        achSockAddr[128];
            char        achAddr[128]    = "";
            int         port            = 0;
            socklen_t   len             = 128;

            if ( getsockname( getfd(), (struct sockaddr *)achSockAddr, &len ) == 0 )
            {
                GSockAddr::ntop( (struct sockaddr *)achSockAddr, achAddr, 128 );
                port = GSockAddr::getPort( (struct sockaddr *)achSockAddr );
            }

            LOG_D(( getLogger(), "connected to [%s] on local addres [%s:%d]!",
                    m_pWorker->getURL(), achAddr, port ));
        }
        */
        // pop LUA stack, return "connect finished successful"
        LsLuaEngine::api()->pushinteger( m_pSendState, 1 );
        n = 1;
    }
    //resume LUA coroutine
    return resume( m_pSendState, n );

}


int EdLuaStream::send( lua_State * pState, const char * pBuf, int32_t iLen )
{
    int ret;
    if ( !( m_iFlag & LUA_STREAM_CONNECTED ) )
    {
        return build_lua_socket_error_ret( pState, ENOTCONN );
    }

    if ( m_iFlag & LUA_STREAM_SEND )
    {
        LsLuaEngine::api()->pushnil( pState );
        LsLuaEngine::api()->pushstring( pState, "socket send in progress" );
        return 2;
    }

    m_iToSend = iLen;
    if ( m_bufOut.empty() )
    {
        ret = write( pBuf, iLen );
        if ( ret > 0 )
        {
            pBuf += ret;
            iLen -= ret;
        }
        else
            if ( ret < 0 )
            {
                // pop LUA stack, return error
                return build_lua_socket_error_ret( pState, errno );

            }
    }
    if ( iLen > 0 )
    {
        m_bufOut.append( pBuf, iLen );
        continueWrite();
        // suspend LUA coroutine
        m_iFlag |= LUA_STREAM_SEND;
        // m_iSendTimeout = ( int64_t )DateTime::s_curTime * 1000 + DateTime::s_curTimeUs / 1000 + m_iTimeoutMs;
        m_iSendTimeout = getCurTimeMs() + m_iTimeoutMs;
        m_pSendState = pState;
        return LsLuaEngine::api()->yieldk( m_pSendState, 0, 0, NULL );
    }
    else
    {
        // pop LUA stack, return success
        LsLuaEngine::api()->pushinteger( pState, m_iToSend );
        return 1;
    }
    return 0;
}

int EdLuaStream::doWrite( lua_State * pState )
{
    int ret = 0; 
    int len;
    while ( m_bufOut.size() > 0 )
    {
        len = m_bufOut.blockSize();
        ret = write( m_bufOut.begin(), len );
        if ( ret >= 0 )
        {
            if ( ret > 0 )
                m_bufOut.pop_front( ret );
            if ( ret < len )
                return 0;
        }
        else
            if ( ret < 0 )
            {
                // pop LUA stack, return error
                ret = build_lua_socket_error_ret( pState, errno );
                break;
            }
    }
    m_iFlag &= ~LUA_STREAM_SEND;
    if ( m_bufOut.empty() )
    {
        //pop LUA stack, return success
        LsLuaEngine::api()->pushinteger( m_pSendState, m_iToSend );
        ret = 1;
    }
    suspendWrite();
    //resume LUA coroutine
    return resume( m_pSendState, ret );

}

int EdLuaStream::onWrite()
{
    if ( m_iFlag & LUA_STREAM_CONNECTING )
    {
        suspendWrite();
        return onInitialConnected();
    }
    if ( !( m_iFlag & LUA_STREAM_SEND ) )
    {
        suspendWrite();
        return 0;
    }
    return doWrite( m_pSendState );
}

int EdLuaStream::processBufIn( lua_State * pState )
{
    int ret;
    int res;
    if ( m_iToRead == EDLUA_STREAM_READ_LINE )
    {
        const char * p;
        const char * pBuf = m_bufIn.getPointer( m_iCurInPos );
        if ( m_iCurInPos < m_bufIn.blockSize() )
        {
            p = ( const char * )memchr( pBuf, '\n', m_bufIn.blockSize() - m_iCurInPos );
            if ( ( !p ) && ( m_bufIn.size() > m_bufIn.blockSize() ) )
            {
                m_iCurInPos = m_bufIn.blockSize();
                pBuf = m_bufIn.getPointer( m_iCurInPos );
                p = ( const char * )memchr( pBuf, '\n', m_bufIn.size() - m_iCurInPos );
            }
        }
        else
            p = ( const char * )memchr( pBuf, '\n', m_bufIn.size() - m_iCurInPos );
        if ( p )
        {
            res = p - pBuf + m_iCurInPos;
            ret = res + 1;
            if ( ( res > 0 ) && ( *m_bufIn.getPointer( res - 1 ) == '\r' ) )
            {
                --res;
            }
        }
        else
            return 0;
    }
    else
        if ( m_iToRead > 0 )
        {
            if ( m_bufIn.size() < m_iToRead )
                return 0;
            ret = res = m_iToRead;
        }
        else
            return 0;
    //pop LUA stack , return bytes read
    if ( m_bufIn.blockSize() != m_bufIn.size() && res > m_bufIn.blockSize() )
    {
        m_bufIn.straight();
    }
    LsLuaEngine::api()->pushlstring( pState, m_bufIn.begin(), res );
    m_bufIn.pop_front( ret );
    LsLua_log( pState, LSI_LOG_DEBUG, 0, "[%p] return %d bytes, pop buffer: %d, left: %d  ", 
               this, res, ret, m_bufIn.size() );
    return 1;

}

int EdLuaStream::doRead( lua_State *pState )
{
    int ret = 0;
    int toRead = 0;
    int res = 0;
    while ( 1 )
    {
        if ( m_iCurInPos >= m_bufIn.size() )
        {
            if ( m_bufIn.available() < 2048 )
            {
                m_bufIn.guarantee( 4096 );
            }

            toRead = m_bufIn.contiguous();
            ret = read( m_bufIn.end(), toRead );
            if ( ret > 0 )
            {
                LsLua_log( pState, LSI_LOG_DEBUG, 0, "[%p] read %d bytes. ", this, ret );
                m_bufIn.used( ret );
            }
            else
                if ( ret == 0 )
                {
                    LsLua_log( pState, LSI_LOG_DEBUG, 0, "[%p] read nothing. ", this );
                    break;
                }
                else
                    if ( ret < 0 )
                    {
                        LsLua_log( pState, LSI_LOG_DEBUG, 0, "[%p] socket error: %d:%s ", this, errno, strerror( errno ) );
                        if ( errno == ECONNRESET )
                        {
                            LsLua_log( pState, LSI_LOG_DEBUG, 0, "[%p] connection closed by peer. ", this );
                        }
                        if ( ( errno == ECONNRESET ) && ( m_iToRead == EDLUA_STREAM_READ_ALL ) )
                            res = 0;
                        else
                            res = build_lua_socket_error_ret( pState, errno );
                        //add data received so far
                        if ( m_bufIn.blockSize() != m_bufIn.size() )
                        {
                            LsLua_log( pState, LSI_LOG_DEBUG, 0, "[%p] buffer straight ", this ); 
                            m_bufIn.straight();
                        }
                        LsLua_log( pState, LSI_LOG_DEBUG, 0, "[%p] return %d bytes ", this, m_bufIn.size() );
                        LsLuaEngine::api()->pushlstring( pState, m_bufIn.begin(), m_bufIn.size() );
                        m_bufIn.clear();
                        res++;
                    }
        }
        if ( !res )
        {
            res = processBufIn( pState );
            if ( !res )
            {
                m_iCurInPos = m_bufIn.size();
                continue;
            }
        }
        if ( m_iFlag & LUA_STREAM_RECV )
        {
            suspendRead();
            m_iFlag &= ~LUA_STREAM_RECV;
            resume( m_pRecvState, res );
        }
        return res;

    }
    if ( !( m_iFlag & LUA_STREAM_RECV ) )
    {
        continueRead();
        m_iFlag |= LUA_STREAM_RECV;
        // m_iRecvTimeout = ( int64_t )DateTime::s_curTime * 1000 + DateTime::s_curTimeUs / 1000 + m_iTimeoutMs;
        m_iRecvTimeout = getCurTimeMs() + m_iTimeoutMs;
        // suspend LUA coroutine
        m_pRecvState = pState;
        return LsLuaEngine::api()->yieldk( m_pRecvState, 0, 0, NULL ) ;
    }
    return 0;

}

int EdLuaStream::recv( lua_State * pState, int32_t toRead )
{
    if ( !( m_iFlag & LUA_STREAM_CONNECTED ) )
    {
        return build_lua_socket_error_ret( pState, ENOTCONN );
    }
    if ( m_iFlag & LUA_STREAM_RECV )
    {
        LsLuaEngine::api()->pushnil( pState );
        LsLuaEngine::api()->pushstring( pState, "socket read in progress" );
        return 2;
    }
    m_iToRead = toRead;
    m_iCurInPos = 0;
    return doRead( pState );
}

int EdLuaStream::onRead()
{
    if ( m_iFlag & LUA_STREAM_RECV )
    {
        return doRead( m_pRecvState );
    }
    else
        if ( m_iFlag & LUA_STREAM_CONNECTING )
        {
            suspendRead();
            return onInitialConnected();
        }
        else
        {
            suspendRead();
        }
    return 0;
}


int EdLuaStream::onEventDone()
{
    //do nothing
    if ( m_iFlag & LUA_STREAM_RECYCLE )
        delete this;
    return 0;
}

void EdLuaStream::onTimer()
{
    // int64_t curTime = ( int64_t )DateTime::s_curTime * 1000 + DateTime::s_curTimeUs / 1000;
    int64_t curTime = getCurTimeMs();
    if ( m_iFlag & LUA_STREAM_RECV )
    {
        if ( m_iRecvTimeout < curTime )
        {
            LsLua_log( m_pRecvState, LSI_LOG_DEBUG, 0, "[%p] receive timed out.", this );
            resumeWithError( m_pRecvState, LUA_STREAM_RECV, ETIMEDOUT );
        }
    }
    if ( m_iFlag & ( LUA_STREAM_SEND | LUA_STREAM_CONNECTING ) )
    {
        if ( m_iSendTimeout < curTime )
        {
            if ( m_iFlag & LUA_STREAM_CONNECTING )
                LsLua_log( m_pSendState, LSI_LOG_DEBUG, 0, "[%p] connect timed out.", this );
            else
                LsLua_log( m_pSendState, LSI_LOG_DEBUG, 0, "[%p] send timed out.", this );
            resumeWithError( m_pSendState, LUA_STREAM_SEND | LUA_STREAM_CONNECTING, ETIMEDOUT );
        }
    }

}

int EdLuaStream::onError()
{
    int error = errno;
    int ret = getSockError( &error );
    if ( ( ret == -1 ) || ( error != 0 ) )
    {
        if ( ret != -1 )
            errno = error;
    }
    LsLua_log( NULL, LSI_LOG_DEBUG, 0, " [%p] EdLuaStream::onError()", this );
    // mark connection error state

    EdStream::close();
    // pop LUA stack, return error
    errno = ENOTCONN;
    m_iFlag &= ~( LUA_STREAM_CONNECTING | LUA_STREAM_CONNECTED );
    if ( m_iFlag & LUA_STREAM_RECV )
    {
        resumeWithError( m_pRecvState, LUA_STREAM_RECV, ENOTCONN );
    }
    if ( m_iFlag & LUA_STREAM_SEND )
    {
        resumeWithError( m_pSendState, LUA_STREAM_SEND, ENOTCONN );
    }
    return ret;
}

//
// close the socket forcely
//
int EdLuaStream::closex( lua_State * pState )
{
    if ( m_iFlag & LUA_STREAM_CONNECTED )
    {
        LsLua_log( pState, LSI_LOG_DEBUG, 0, "closex %d", EdLuaStream::getfd() );
        EdStream::close();
        m_iFlag &= ~LUA_STREAM_CONNECTED;
        // return(EdLuaStream::close(pState));
    }
    return (0);
}

int EdLuaStream::close( lua_State * pState )
{
    int ret;
        LsLua_log( pState, LSI_LOG_DEBUG, 0, "close %d", EdLuaStream::getfd() );
    ret = EdStream::close();
    m_iFlag &= ~LUA_STREAM_CONNECTED;

    if ( m_iFlag & LUA_STREAM_CONNECTING )
    {
        resumeWithError( m_pSendState, LUA_STREAM_CONNECTING, EBADF );
    }

    if ( m_iFlag & LUA_STREAM_RECV )
        doRead( m_pRecvState );   //this should resume the read coroutine with error
    if ( m_iFlag & LUA_STREAM_SEND )
        doWrite( m_pSendState );  //this should resume the write coroutine with error
    if ( ret == -1 )
        ret = build_lua_socket_error_ret( pState, errno );
    else
    {
        LsLuaEngine::api()->pushinteger( pState, 1 );
        ret = 1;
    }
    return ret;
}


//
//  TCP SOCK C CODES - should be in interface class in the future
//
#define LSLUA_TCPSOCKDATA "LS_TCP"

static int LsLua_sock_tostring( lua_State * L )
{
    char    buf[0x100];

    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( !p )
        return 0;
    EdLuaStream * p_sock = *p;
    if ( p_sock )
    {
        snprintf( buf, 0x100, "<ls.socket %p>", p_sock );
    }
    else
    {
        strcpy( buf, "<ls.socket DATA-INVALID>" );
    }
    LsLuaEngine::api()->pushstring( L, buf );
    return 1;
}

static int LsLua_sock_gc( lua_State * L )
{
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( !p )
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "GC <ls.socket INVALID LUA UDATA>" );
        return 0;
    }
    
    // DO NOTHING ... EdStream will manage this
#if 0    
    EdLuaStream * p_sock = *p;
    LsLua_log( L, LSI_LOG_NOTICE, 0, "GC <ls.socket %p fd %d>", p_sock, p_sock->getfd() );
    // if ( p_sock && (p_sock->getfd() != -1))
    if ( p_sock )
    {
        // remove myself from session pool
        
        // LsLuaSession * pSession = LsLua_getSession( L );
        // if (pSession)
        //    pSession->markCloseStream(L, p_sock);
        
        // Don't delete this since EdStream will manage itself.
        // delete p_sock;
    }
    else
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "GC <ls.socket INVALID> EdLuaStream" );
    }
#endif
    return 0;
}

static int  LsLua_sock_create( lua_State * L )
{
    EdLuaStream *   p_sock = LsLuaSession::newEdLuaStream(L);
    if (!p_sock)
    {
        LsLuaEngine::api()->pushnil( L );
    }
    else
    {
        LsLuaEngine::api()->getfield( L, LsLuaEngine::api()->LS_LUA_REGISTRYINDEX, LSLUA_TCPSOCKDATA );
        LsLuaEngine::api()->setmetatable( L, -2 );
    }
    return 1;
}

static int  LsLua_sock_connect( lua_State * L )
{
    EdLuaStream * p_sock = NULL;
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( p )
        p_sock = *p;
    if ( !p_sock )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );

    size_t  size;
    const char * cp;
    if ( ( cp = LsLuaEngine::api()->tolstring(L, 2, &size) ) && size )
        return p_sock->connectTo( L, cp, LsLuaApi::tonumberByIdx( L, 3 ) );
    else
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );
}

static int  LsLua_sock_close( lua_State * L )
{
    EdLuaStream * p_sock = NULL;
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( p )
        p_sock = *p;
    if ( !p_sock )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );

    return p_sock->close( L );
}

static int  LsLua_sock_read( lua_State * L )
{
    EdLuaStream * p_sock = NULL;
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( p )
        p_sock = *p;
    if ( !p_sock )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );

    // support no parameter read
    if ( ( LsLuaEngine::api()->gettop( L ) < 2 ) )
        return p_sock->recv( L, EDLUA_STREAM_READ_LINE );

    size_t  size;
    const char * cp;
    if ( ( !( cp = LsLuaEngine::api()->tolstring( L, 2, &size ) ) ) || ( !size ) )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_PARAMETERS );

    // default each line
    int key = EDLUA_STREAM_READ_LINE;
    if ( !memcmp( cp, "*l", 2 ) )
        key = EDLUA_STREAM_READ_ALL;
    // SIMON HACK
    else if (!strcmp(cp, "*a")) key = EDLUA_STREAM_READ_LINE;
    else key = atoi(cp);

    return p_sock->recv( L, key );
}

static int  LsLua_sock_write( lua_State * L )
{
    EdLuaStream * p_sock = NULL;
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( p )
        p_sock = *p;
    if ( !p_sock )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );

    size_t  size = 0;
    const char * cp;
    if ( ( !( cp = LsLuaEngine::api()->tolstring(L, 2, &size) ) ) || ( !size ) )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_PARAMETERS );

    return p_sock->send( L, cp, size );
}

static int  LsLua_sock_settimeout( lua_State * L )
{
    EdLuaStream * p_sock = NULL;
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( p )
        p_sock = *p;
    if ( !p_sock )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );

    int timeout = LsLuaApi::tonumberByIdx( L, 2 );
    if ( timeout <= 0 )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_PARAMETERS );

    return p_sock->setTimout( timeout );
}

static int  LsLua_sock_setoption( lua_State * L )
{
    EdLuaStream * p_sock = NULL;
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( p )
        p_sock = *p;
    if ( !p_sock )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );

    LsLua_log( L, LSI_LOG_DEBUG, 0, "setoption not supported yet" );
    return 0;
}

static int  LsLua_sock_setkeepalive( lua_State * L )
{
    EdLuaStream * p_sock = NULL;
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( p )
        p_sock = *p;
    if ( !p_sock )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );

    LsLua_log( L, LSI_LOG_DEBUG, 0, "setkeepalive not supported yet" );
    
    // we don't support setkeepalive t
    LsLuaEngine::api()->pushinteger( L, 1 );
    LsLuaEngine::api()->pushlstring( L, "not supported", 13 );
    return 1;
}
static int  LsLua_sock_getreusedtimes( lua_State * L )
{
    EdLuaStream * p_sock = NULL;
    EdLuaStream ** p = ( EdLuaStream ** )LsLuaEngine::api()->checkudata( L, 1, LSLUA_TCPSOCKDATA );
    if ( p )
        p_sock = *p;
    if ( !p_sock )
        return LsLuaApi::return_badparameter( L, LSLUA_BAD_SOCKET );

    LsLua_log( L, LSI_LOG_DEBUG, 0, "getreusetimes not supported yet" );
    return 0;
}

//
//  TCP SOCK META
//
static const luaL_Reg sock_sub[] =
{
    {"tcp",         LsLua_sock_create},
    {"receive",     LsLua_sock_read},
    {"send",        LsLua_sock_write},
    {"connect",     LsLua_sock_connect},
    {"close",       LsLua_sock_close},
    {"settimeout",  LsLua_sock_settimeout},
// dummy for now!
    {"setoption",       LsLua_sock_setoption},
    {"setkeepalive",    LsLua_sock_setkeepalive},
    {"getreusedtimes",  LsLua_sock_getreusedtimes},
    {NULL, NULL}
};

static const luaL_Reg sock_meta_sub[] =
{
    {"__gc",        LsLua_sock_gc},
    {"__tostring",  LsLua_sock_tostring},
    {NULL, NULL}
};

void LsLua_create_tcp_sock_meta( lua_State * L )
{
    LsLuaEngine::api()->openlib( L, LS_LUA ".socket", sock_sub, 0 );
    LsLuaEngine::api()->newmetatable( L, LSLUA_TCPSOCKDATA );
    LsLuaEngine::api()->openlib( L, NULL, sock_meta_sub, 0 );

    // pushliteral
    LsLuaEngine::api()->pushlstring( L, "__index", 7 );
    LsLuaEngine::api()->pushvalue( L, -3 );
    LsLuaEngine::api()->rawset( L, -3 );      // metatable.__index = methods
    // pushliteral
    LsLuaEngine::api()->pushlstring( L, "__metatable", 11 );
    LsLuaEngine::api()->pushvalue( L, -3 );
    LsLuaEngine::api()->rawset( L, -3 );      // metatable.__metatable = methods

    LsLuaEngine::api()->settop( L, -3 );     // pop 2
}

