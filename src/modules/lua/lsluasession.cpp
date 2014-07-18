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

#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <openssl/sha.h>
#include "edluastream.h"
#include "lsluasession.h"
#include "lsluaengine.h"
#include "lsluaapi.h"
#include <../addon/include/ls.h>
#include <log4cxx/logger.h>
#include <http/httpsession.h>

// local stuff for TIMER
#define SET_TIMER_NORMAL        0
#define SET_TIMER_ENDDELAY      1
#define SET_TIMER_MAXRUNTIME    2

static int  LsLua_sess__end_helper( LsLuaSession *  );
static void LsLua_createMiscTable( lua_State * );
static void LsLua_createGlobal( lua_State * );

int LsLuaSession::s_key = 0;    // unique session key

LsLuaSession::LsLuaSession()
    : m_wait4RespBuf_L( NULL )
    , m_pHttpSession( NULL )
    , m_pVM( NULL )
    , m_pVM_mom( NULL )
    , m_top(0)
    , m_pEndTimer(NULL)
    , m_pMaxTimer(NULL)
    , m_pStream( NULL )
    , m_pTimerList( NULL )
{
    m_key = ++s_key;    // noted 0 is not used
    m_ref = LUA_REFNIL;
    clearLuaStatus();
    // add();
}

LsLuaSession::~LsLuaSession()
{
    m_key = 0;          // no more timer callback
    if (m_ref != LUA_REFNIL)
    {
        LsLuaEngine::unrefX(this);
        m_ref = LUA_REFNIL;
    }
    // remove();
}

void LsLuaSession::clearLuaStatus()
{
    m_exitCode = 0;
    m_doneFlag = 0;
    m_runFlag = 0;
    m_waitReqBody = 0;
    m_waitLineExec = 0;
    m_waitRespBuf = 0;
    m_luaLineCount = 0;
    m_UrlRedirected = 0;
    m_endFlag = 0;
}

void LsLuaSession::closeAllStream()
{
    register LsLuaStreamData * p_data;
    // LsLua_log( getLuaState(), LSI_LOG_NOTICE, 0, "closeAllStream HTTP %p session <%p>", getHttpSession(), this);
    for (p_data = m_pStream; p_data; p_data = p_data->next())
    {
        p_data->close(m_pVM);
    }
}

//
//  this is slow linear search... 
//  If we need performance then we need to change EdLuaStream to set reverse link.
//
void LsLuaSession::markCloseStream(lua_State * L, EdLuaStream * sock)
{
    register int            n;
    register LsLuaStreamData * p_data;
    
    // LsLua_log( L, LSI_LOG_NOTICE, 0, "markCloseStream HTTP %p session <%p>", getHttpSession(), this);
    n = 0;
    for (p_data = m_pStream; p_data; p_data = p_data->next())
    {
        // LsLua_log( L, LSI_LOG_NOTICE, 0, "markCloseStream item %d ", n);
        if (p_data->isMatch(sock))
        {
            LsLua_log( L, LSI_LOG_NOTICE, 0, "markCloseStream HTTP %p session <%p> %d", getHttpSession(), this, n);
            p_data->close(L);
            return;
        }
        n++;
    }
}

EdLuaStream* LsLuaSession::newEdLuaStream( lua_State * L )
{
    LsLuaSession * pSession = LsLua_getSession( L );
    EdLuaStream ** p;
    EdLuaStream * p_sock;
    if (!pSession)
        return NULL;
    if (!(p_sock = new EdLuaStream()))
        return  NULL;
    if (!(p = ( EdLuaStream ** )( LsLuaEngine::api()->newuserdata( L, sizeof( EdLuaStream * )))))
    {
        delete p_sock;
        return NULL;
    }
    *p = p_sock;
    
    LsLuaStreamData * p_data = new LsLuaStreamData(p_sock, pSession->m_pStream);
    pSession->m_pStream = p_data;
    
    return(p_sock);
}

void LsLuaStreamData::close(lua_State * L)
{
    if (isActive())
    {
        setNotActive();
        m_sock->closex(L);
    }
}
//
// @brief onWrite - call by modlua.c to indicate the resp buffer available to write
// @param[in] httpSession
// @return -1 on failure, 0 no more write, 1 more write
//
int LsLuaSession::onWrite( lsi_session_t * httpSession )
{
    //
    // if I am waiting for it and there is buffer then process it.
    //
    if (isWaitRespBuf() && (g_api->is_resp_buffer_available(httpSession) == 1))
    {
        clearWaitRespBuf(); // not waiting for it anymore!
        g_api->set_handler_write_state(httpSession, 0); // dont call me anymore!
        // Wake up
        lua_State * L  = m_wait4RespBuf_L;
        m_wait4RespBuf_L = NULL;
        LsLuaEngine::api()->resumek( L, NULL, 0 );
        return 1;
    }
    else
    {
        return 1;
    }
}

int LsLuaSession::wait4RespBuffer( lua_State* L )
{
    setWaitRespBuf();               // enable callback
    g_api->set_handler_write_state(getHttpSession(), 1); // start to call me
    m_wait4RespBuf_L = L;
    return LsLuaEngine::api()->yieldk( L, 0, 0, NULL );
}

//
//  static callback for general purpose LsLuaSession Timer 
//
void LsLuaSession::timerCb( void* data)
{
    LsLuaTimerData  * _data = (LsLuaTimerData *)data;
    
    LsLua_log( _data->session()->getLuaState(), LSI_LOG_DEBUG, 0
                    , "SESSION timerCb [%p] HTTP %p session %p key %d id %d"
                    , _data->session()->getLuaState()
                    , _data->session()->getHttpSession()
                    , _data->session()
                    , _data->key()
                    , _data->id()
             );
    
    // remove from the list...
    _data->session()->rmTimerFromList( _data );
    
    // checking the key is to make sure we are not attemping to use recycling data
    if ((!_data->flag()) && (_data->key() == _data->session()->key()) && _data->session()->getLuaState())
    {
        _data->timerCallBack();
    }
    delete _data;
}

//
//  reach User specified endtime - forcely abort here
//
void LsLuaSession::maxRunTimeCb( LsLuaSession* pSession, lua_State* L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0,
                "SESSION maxRunTimeCb [%p] HTTP %p session <%p>",
                pSession->getLuaState(), pSession->getHttpSession(),
                pSession );
 
    // LsLua_sess__end( L );
    // LsLua_sess__end( pSession->getLuaState() );
    LsLua_sess__end_helper( pSession );
    pSession->m_pMaxTimer = NULL; // remove myself from check
}

void LsLuaSession::luaLineLooper( LsLuaSession* pSession, lua_State* L )
{
    if (LsLuaEngine::debug() & 0x10)
        LsLuaSession::trace("LINELOOPER", pSession, L);
    pSession->clearWaitLineExec();        // enable further again
    LsLuaEngine::api()->resumek( L, NULL, 0 );
}

void LsLuaSession::luaLineHookCb( lua_State* L, lua_Debug* ar )
{
    // NOTE - since I don't use multiple hookflag point - dont need to check
    // if (ar->event == LUA_HOOKLINE)
    // if (ar->event == LUA_HOOKCOUNT)
    {
        LsLuaSession * pSession;
        if ( (pSession = LsLua_getSession( L )) )
        {
            if ((!pSession->getLuaCounter()) && LsLuaEngine::api()->jitMode)
            {
                // JIT ignore first hook...
                pSession->upLuaCounter();
                return;
            }
            pSession->upLuaCounter();
            if (!pSession->isWaitLineExec())
            {
                LsLua_log( L, LSI_LOG_DEBUG, 0,
                    "SESSION linehook [%p] HTTP %p session <%p> %d",
                    pSession->getLuaState(), pSession->getHttpSession(),
                    pSession, pSession->getLuaCounter());
                pSession->setWaitLineExec();        // disable further
                pSession->setTimer(LsLuaEngine::getPauseTime(), LsLuaSession::luaLineLooper, L, SET_TIMER_NORMAL);
                
                LsLuaEngine::api()->yieldk( L, 0, 0, NULL );
            }
#if 0 // FOR JIT it is possible to have thing running... so don't brother to send information out
            else
            {
                LsLua_log( L, LSI_LOG_NOTICE, 0,
                    "ERROR: SESSION IGNORE luahook callback [%p] HTTP %p session <%p> %d",
                    pSession->getLuaState(), pSession->getHttpSession(),
                    pSession, pSession->getLuaCounter() );
            }
#endif
        }
    }
}

void LsLuaSession::setTimer( int msec, LsLuaSesionCallBackFunc_t f, lua_State * L, int flag)
{
    LsLuaTimerData  * data = new LsLuaTimerData(this, f, L);
    
    data->setId(g_api->set_timer(msec, timerCb, data));
    LsLua_log( L, LSI_LOG_DEBUG, 0
            , "setTimer %p session <%p> <%d msec> id %d"
            , getHttpSession(), this, msec, data->id());
    
    switch (flag)
    {
    case SET_TIMER_ENDDELAY:
        m_pEndTimer = data;
        break;
    case SET_TIMER_MAXRUNTIME:
        m_pMaxTimer = data;
        break;
    case SET_TIMER_NORMAL:
        addTimerToList(data);
    }
}

void LsLuaSession::releaseTimer()
{
    releaseTimerList();
    
    if (m_pMaxTimer) {
        LsLua_log( getLuaState(), LSI_LOG_DEBUG, 0
                    , "REMOVE maxTimer %p %d"
                    , this
                    , m_pMaxTimer->id());
            
        m_pMaxTimer->setFlag(1);
        g_api->remove_timer (m_pMaxTimer->id());
        delete m_pMaxTimer;
        m_pMaxTimer = NULL;
    }
    
    if (m_pEndTimer) {
        LsLua_log( getLuaState(), LSI_LOG_DEBUG, 0
                    , "REMOVE endTimer %p %d"
                    , this
                    , m_pEndTimer->id());
            
        m_pEndTimer->setFlag(1);
        g_api->remove_timer (m_pEndTimer->id());
        delete m_pEndTimer;
        m_pEndTimer = NULL;
    }
    
}

//
//  Add normal LUA timer to session list
//
void LsLuaSession::addTimerToList( LsLuaTimerData* pTimerData )
{
    pTimerData->setNext( m_pTimerList );
    m_pTimerList = pTimerData;
    
#if 0
    char buf[0x100];
    snprintf(buf, 0x100, "addTimerToList ADD %3d next %p", pTimerData->id(), pTimerData->next());
    dumpTimerList( buf );
#endif
}

//
//  remove normal LUA timer from session list
//
void LsLuaSession::rmTimerFromList( LsLuaTimerData* pTimerData )
{
    LsLuaTimerData * pTimerList;
    LsLuaTimerData * pNext;
    
#if 0
    char buf[0x100];
    snprintf(buf, 0x100, "rmTimerFromList REMOVE %3d next %p", pTimerData->id(), pTimerData->next());
#endif
    if ( (pTimerList = m_pTimerList) )
    {
        if ( pTimerData == pTimerList )
        {
            m_pTimerList = pTimerData->next();
            pTimerData->setNext(NULL);
#if 0            
            dumpTimerList( buf );
#endif
            return ;
        }
        while (  (pNext = pTimerList->next()) )
        {
            if (pNext == pTimerData)
            {
                pTimerList->setNext(pTimerData->next());
                pTimerData->setNext(NULL);
#if 0            
                dumpTimerList( buf );
#endif
                return;
            }
            pTimerList = pNext;
        }
    }
}

void LsLuaSession::releaseTimerList()
{
    register LsLuaTimerData * pList;
    register LsLuaTimerData * pNext;

#if 0
    LsLua_log( getLuaState(), LSI_LOG_NOTICE, 0 
                , "%s"
                , "RELEASE TIMERLIST");
#endif
    for (pList = m_pTimerList; pList; pList = pNext)
    {
#if 0
        LsLua_log( getLuaState(), LSI_LOG_NOTICE, 0
                , "REMOVE TIMER [%3d] next %3d flag %d"
                , pList->id(), pList->key(), pList->flag());
#endif
        
        pNext = pList->next();
        g_api->remove_timer (pList->id());
        delete pList;
        pList = pNext;
    }
    m_pTimerList = NULL;
}


void LsLuaSession::dumpTimerList( const char * tag )
{
    register LsLuaTimerData * pList;
    
    LsLua_log( getLuaState(), LSI_LOG_NOTICE, 0 
                , "DUMPTIMERLIST %s"
                , tag);
    for (pList = m_pTimerList; pList; pList = pList->next())
    {
        LsLua_log( getLuaState(), LSI_LOG_NOTICE, 0
                , "TIMER-ITEM [%3d] next %3d flag %d"
                , pList->id(), pList->key(), pList->flag());
    }
}

//
//  RESUME LUA SESSION
//
void LsLuaSession::resume( lua_State * L, int nargs )
{
    // LsLuaApi::dumpStack( getLuaState(), "SESSION RESUME", 10 );
    if (isLuaWaitReqBody())
    {
        clearLuaWaitReqBody();
        LsLuaEngine::api()->resumek( getLuaState(), NULL, nargs );
        return;
    }
}

int LsLuaSession::setupLuaEnv(lua_State * L, LsLuaUserParam * pUser)
{
    // it has been initialized
    if ( m_pVM ) 
        return 0;
    // 1. create coroutine for this session,
    if (!(m_pVM = LsLuaEngine::api()->newthread( L )))
    {
        return -1;
    }
    
    // 2. inherit system level injected APIs. 
    LsLuaEngine::injectLsiapi(m_pVM);

    // 3. create global varaible instance for "ls"
    //    this refers to this LsLuaSession object
    if (LsLua_setSession( m_pVM, this ))
        return -1;
    
    // 4. create "ls.ctx" table. 
    LsLua_createMiscTable( m_pVM );

    // track my mom state
    m_pVM_mom = L;
    if (LsLuaEngine::debug() & 0x10)
        LsLuaSession::trace("SETUP", this, m_pVM);
    
    // maxruntime
    if (pUser->getMaxRunTime() > 0)
    {
        setTimer(pUser->getMaxRunTime(), LsLuaSession::maxRunTimeCb, m_pVM_mom, SET_TIMER_MAXRUNTIME);
        LsLua_log( L, LSI_LOG_DEBUG, 0
                , "HTTP %p session <%p> MAX RUNTIME SET TO <%d msec>"
                , getHttpSession(), this, pUser->getMaxRunTime());
    }
    
    //
    // setup maxlinecount - this will the code slower but the server will be a lot happier
    //
    if (pUser->getMaxLineCount() > 0)
    {
        int ret;
        //
        // NOTE - the behavior is different between 5.2 and JIT 2
        //          The JIT doesn't use MASKLINE too well!

        //  Hack - jit mode will divid the counter by 1000000
        //
        int linecount = 0;
        linecount = pUser->getMaxLineCount();
        if ( (LsLuaEngine::api()->jitMode ) && ( LsLuaEngine::getJitLineMod() > 1 ) )
            linecount = pUser->getMaxLineCount() / LsLuaEngine::getJitLineMod();
            
        ret = LsLuaEngine::api()->sethook( getLuaState(),
                 LsLuaSession::luaLineHookCb,
                 // LUA_MASKCOUNT | LUA_MASKLINE, 
                 LUA_MASKCOUNT,
                 linecount
                 );
        
                                     // LUA_MASKLINE, LsLuaEngine::getMaxLineCount() );
        if (ret != 1)
        {
            LsLua_log( L, LSI_LOG_NOTICE, 0
                    , "PROBLEM SETHOOK %d HTTP %p <%p> MAX RUNTIME TO <%d msec>"
                    , ret, getHttpSession(), this, pUser->getMaxLineCount() );
        }
    }
    return 0;
}

int LsLuaSession::setupLuaEnvX(lua_State * L, LsLuaUserParam * pUser)
{
    // it has been initialized
    if ( m_pVM ) 
        return 0;
    // 1. create coroutine for this session,
    if (!(m_pVM = LsLuaEngine::api()->newthread( L )))
    {
        return -1;
    }
    
    // 2. inherit system level injected APIs. 
    // LsLuaEngine::injectLsiapi(m_pVM);
    // LsLuaApi::dumpStack(m_pVM, "after injectLsiapi", 0);
    LsLua_createGlobal( m_pVM);

    // 3. create global varaible instance for "ls"
    //    this refers to this LsLuaSession object
    LsLua_setSession( m_pVM, this );
    // LsLuaApi::dumpStack(m_pVM, "after setsession", 10);
    
    // 4. create "ls.ctx" table. 
    LsLua_createMiscTable( m_pVM );
    // LsLuaApi::dumpStack(m_pVM, "after misctable", 10);

    // track my mom state
    m_pVM_mom = L;
    if (LsLuaEngine::debug() & 0x10)
        LsLuaSession::trace("SETUP", this, m_pVM);
    
    // maxruntime
    if (pUser->getMaxRunTime() > 0)
    {
        setTimer( pUser->getMaxRunTime(), LsLuaSession::maxRunTimeCb, m_pVM_mom, SET_TIMER_MAXRUNTIME);
        LsLua_log( L, LSI_LOG_DEBUG, 0, "HTTP %p session <%p> MAX RUNTIME SET TO <%d msec>",
            getHttpSession(), this , pUser->getMaxRunTime() );
    }
    
    //
    // setup maxlinecount - this will the code slower but the server will be a lot happier
    //
    if (pUser->getMaxLineCount() > 0)
    {
        int ret;
        //
        // NOTE - the behavior is different between 5.2 and JIT 2
        //          The JIT doesn't use MASKLINE too well!
        //  Hack - jit mode will divid the counter by 1000000
        //
        int linecount = 0;
        linecount = pUser->getMaxLineCount();
        if ( (LsLuaEngine::api()->jitMode ) && ( LsLuaEngine::getJitLineMod() > 1 ) )
                linecount = pUser->getMaxLineCount() / LsLuaEngine::getJitLineMod();
            
        ret = LsLuaEngine::api()->sethook( getLuaState(),
                 LsLuaSession::luaLineHookCb,
                 // LUA_MASKCOUNT | LUA_MASKLINE, 
                 LUA_MASKCOUNT,
                 linecount
                 );
        
                                     // LUA_MASKLINE, LsLuaEngine::getMaxLineCount() );
        if (ret != 1)
        {
            LsLua_log( L, LSI_LOG_NOTICE, 0,
                 "PROBLEM SETHOOK %d HTTP %p <%p> MAX LINELOOPER TO <%d msec>",
                 ret, getHttpSession(), this, pUser->getMaxLineCount() );
        }
    }
    return 0;
}



//
//  Meta to track LsLuaSession
//
static int LsLua_session_tostring( lua_State * L )
{
    char    buf[0x100];

    snprintf( buf, 0x100, "<ls %p>", L );
    LsLuaEngine::api()->pushstring( L, buf );
    return 1;
}

typedef struct sessionMeta_t
{
    LsLuaSession*   pSession;
    int             active;
    int             key;
} sessionMeta_t;

static int LsLua_session_gc( lua_State * L )
{
    //LsLuaApi::dumpStack(L, "LsLua_session_gc", 10);
    if (LsLuaEngine::debug() & 0x10)
    {
        sessionMeta_t * p;
        if ( (p = ( sessionMeta_t *)LsLuaEngine::api()->touserdata(L, -1)) )
        {
            LsLua_log( L, LSI_LOG_NOTICE, 0
                            , "<LsLua_session_gc %p [%d %d]>"
                            , p->pSession, p->active, p->key);

            LsLuaSession * pSession = p->pSession;
            if (p->active && (p->key == pSession->key()))
            {
                LsLua_log( L, LSI_LOG_NOTICE, 0
                                , "<LsLua_session_gc RELEASE ACTIVE %p [%d]>"
                                , pSession, p->key);
            }
        }
        else
        {
            LsLua_log( L, LSI_LOG_NOTICE, 0, "<ls.session GC>");
        }
    }
    return 0;
}

#define LSLUA_SESSION_META "LS_SESSMETA"
void LsLua_create_session_meta( lua_State * L )
{
    // LsLuaApi::dumpStack( L, "BEGIN LsLua_create_session_meta", 10 );
    LsLuaEngine::api()->newmetatable( L, LSLUA_SESSION_META);       // meta
    LsLuaEngine::api()->pushcclosure( L, LsLua_session_gc, 0);      // meta func
    LsLuaEngine::api()->setfield( L, -2, "__gc");                   // meta
    LsLuaEngine::api()->pushcclosure( L, LsLua_session_tostring, 0);// meta func
    LsLuaEngine::api()->setfield( L, -2, "__tostring");             // meta
    LsLuaApi::pop(L, 1);
    // LsLuaApi::dumpStack( L, "AFTER LsLua_create_session_meta", 10 );
}

LsLuaSession *  LsLua_getSession(lua_State * L)
{
    sessionMeta_t * p;
    LsLuaSession * pSession = NULL;

    // pSession = LsLuaSession::findByLuaState(L);
    // if (!pSession)
    {
        LsLuaEngine::api()->getglobal(L, "__ls_session");
        // pSession = ( LsLuaSession *)LsLuaEngine::api()->touserdata(L, -1);
        if ( (p = ( sessionMeta_t *)LsLuaEngine::api()->touserdata(L, -1)) )
        {
            pSession = p->pSession;
        }
        else
        {
            LsLua_log( L, LSI_LOG_NOTICE, 0, "LsLua_getSession FAILED %p n <%p>", L);
        }
        LsLuaApi::pop(L, 1);
    }
    return pSession;
}

static void LsLua_clearSession(lua_State *L )
{
    sessionMeta_t * p;
    LsLuaEngine::api()->getglobal(L, "__ls_session");
    if ( (p = ( sessionMeta_t *)LsLuaEngine::api()->touserdata(L, -1)) )
    {
        p->active = 0;
        p->pSession = NULL;
        LsLuaApi::pop(L, 1);
    }
}

int LsLua_setSession(lua_State *L, LsLuaSession * pSession)
{
    sessionMeta_t * p;
    if (!(p = ( sessionMeta_t * )( LsLuaEngine::api()->newuserdata( L, sizeof( sessionMeta_t )))))
        return -1;
    p->pSession = pSession;
    p->active = pSession->isEndFlag() ? 0 : 1;
    p->key = pSession->key();
    // LsLuaEngine::api()->pushlightuserdata(L, pSession);
    LsLuaEngine::api()->getfield( L, LsLuaEngine::api()->LS_LUA_REGISTRYINDEX, LSLUA_SESSION_META );
    LsLuaEngine::api()->setmetatable( L, -2 );
    LsLuaEngine::api()->setglobal(L, "__ls_session");
    // LsLuaApi::dumpStack(L, "after setglobal setsession", 10);
    return 0;
}

#define LSLUA_REQ       "LS_REQ"
#define LSLUA_RESP      "LS_RESP"
#define LSLUA_SESSION   "LS_SESSION"
#define LS_HEADER       LS_LUA ".header"

//
//  tmpBuf ???
//
#define TMPBUFSIZE  0x2000
static int      _tmpsize = TMPBUFSIZE;
static char     _tmpbuf[TMPBUFSIZE];

//
//  handy define to extract buf and push onto LUA STACK
//  PUSH_LSI_BUF
//  (1) check session && call the ret = lsi_api(session, buf, len)
//  (2) if (ret)  push buf <to LUA stack>
//  (3) else      push nil <to LUA stack>
//
//  PUSH_LSI_ENV
//  (1) check session && call the ret = lsi_api(session, key, buf, len)
//  (2) if (ret)  push buf <to LUA stack>
//  (3) else      push nil <to LUA stack>
//
#define PUSH_LSI_BUF( session, lsi_api ) {\
    if ( (_tmpsize = session ? lsi_api (session, _tmpbuf, TMPBUFSIZE) : 0) ) \
            LsLuaEngine::api()->pushlstring( L, _tmpbuf, _tmpsize );\
    else \
            LsLuaEngine::api()->pushnil( L ); \
    }
#define PUSH_LSI_ENV( session, lsi_api, key ) {\
    if ( (_tmpsize = session ? lsi_api (session, key, _tmpbuf, TMPBUFSIZE) : 0) ) \
            LsLuaEngine::api()->pushlstring( L, _tmpbuf, _tmpsize );\
    else \
            LsLuaEngine::api()->pushnil( L ); \
    }
//
//  USED by C routines...
// 
#define GET_LSI_BUF( session, lsi_api ) {\
    _tmpsize = session ? lsi_api (session, _tmpbuf, TMPBUFSIZE) : 0 ; \
    }

typedef struct lslua_log_param_t
{
    LsLuaSession * _pLuaSession;
    lua_State    * _L;
    int            _level;
} lslua_log_param_t;

int LsLua_log_flush( void * param, const char * pBuf, int len, int *flag )
{
    lslua_log_param_t * pLog = (lslua_log_param_t *)param;
    if ( pLog->_pLuaSession && pLog->_pLuaSession->getHttpSession() )
    {
        lsi_session_t * pSess = pLog->_pLuaSession->getHttpSession();
        if ( !(*flag & LSLUA_PRINT_FLAG_ADDITION) )
            g_api->log( pSess, pLog->_level, "[%p] [LUA] ", pSess );
        g_api->lograw( pSess, pBuf, len );
    }
    else
    {
        if ( !(*flag & LSLUA_PRINT_FLAG_ADDITION) )
            LsLua_log( pLog->_L, pLog->_level, 1, "" );
        LsLua_lograw_buf( pBuf, len );
    }
    return 0;
}

int  LsLua_log_ex( lua_State * L, int level )           
{
    LsLuaSession * p_req = LsLua_getSession( L );
    lslua_log_param_t log_param = { p_req, L, level };
    lslua_print_stream_t stream = { &log_param, LsLua_log_flush, LSLUA_PRINT_FLAG_LF, {0, 0, 0} };
    LsLua_generic_print( L, &stream );
    return 0;
}

int LsLua_resp_body_flush( void * param, const char * pBuf, int len, int *flag )
{
    LsLuaSession * pSession = (LsLuaSession *)param;
    if ( pSession && pSession->getHttpSession() )
    {
        lsi_session_t * pHttpSess = pSession->getHttpSession();
        if ( pHttpSess )
            if ( g_api->append_resp_body( pHttpSess, pBuf, len ) == -1 )
                return -1;
    }
    else
        return -1;

    return 0;
}

static int  LsLua_sess_log( lua_State * L )           
{
    int level;
    level = LsLuaEngine::api()->tointegerx( L, 1, NULL );
    LsLuaEngine::api()->remove( L, 1 );
    return LsLua_log_ex( L, level );
}

//
//  For Internal debugging purpose
//
static int  LsLua_sess_debug( lua_State * L )
{
    size_t n;
    // LsLuaApi::dumpStack(L, "debug", 10);
    LsLuaSession * pSession = LsLua_getSession( L );
    const char * cp = LsLuaEngine::api()->tolstring( L, 1, &n);
    if (cp && n)
    {
        if (!strncmp(cp, "hookcount", 9))
        {
            LsLuaEngine::api()->pushinteger( L, pSession->getLuaCounter());
            return 1;
        }
        if (!strncmp(cp, "lua", 3))
        {
            const char * cmd = LsLuaEngine::api()->tolstring( L, 2, &n);
            if (cmd)
            {
                if (LsLuaApi::execLuaCmd(L, cmd))
                {
                    return LsLuaApi::return_badparameter(L, LSLUA_BAD);
                }
                else
                {
                    LsLuaEngine::api()->pushinteger( L, 0);
                    return 1;
                }
            }
            else 
            {
                return LsLuaApi::return_badparameter(L, LSLUA_BAD_PARAMETERS);
            }
        }
        else
        {
            return LsLuaApi::return_badparameter(L, LSLUA_BAD_PARAMETERS);
        }
    }
    //
    // default show hookcount
    //
    LsLuaEngine::api()->pushinteger( L, pSession->getLuaCounter());
    return 1;
}

static int  LsLua_sess_exec( lua_State * L )          
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "sess_exec  not supported yet" );
    return 0;
}

//
// @belief LsLua_sess_redirect
// @param[in] uri - redirected uri
// @param[in] status - 301, 302, 307, 0 - internal redirect
//
static int  LsLua_sess_redirect( lua_State * L )      
{
    LsLuaSession * pSession = LsLua_getSession( L );
    size_t len;
    const char * cp = LsLuaEngine::api()->tolstring( L, 1, &len);
    int status = 1 * LsLuaEngine::api()->tonumberx(L, 2, NULL);
    
    switch (status)
    {
        case LSI_URI_NOCHANGE:          // 0
        case LSI_URI_REWRITE:           // 1
        case LSI_URL_REDIRECT_INTERNAL: // 2
        case LSI_URL_REDIRECT_301:      
        case LSI_URL_REDIRECT_302:
        case LSI_URL_REDIRECT_307:
            // don't modify it.
            break;
        case 301:
            status = LSI_URL_REDIRECT_301;
            break;
        case 302:
            status = LSI_URL_REDIRECT_302;
            break;
        case 307:
            status = LSI_URL_REDIRECT_307;
            break;
        default:
            status = LSI_URI_REWRITE;
            break;
    }
    if (g_api->set_uri_qs(pSession->getHttpSession(), status, cp, len, "", 0 ))
    {
        // redirect - failed
        LsLuaEngine::api()->pushinteger( L, 0 );
        // LsLua_log( L, LSI_LOG_NOTICE, 0, "sess_redirect [%.*s] status %d FAILED", len, cp, status);
        return 1;
    }
    pSession->setUrlRedirected(); // disable to flush and end_resp
    LsLuaEngine::api()->pushinteger( L, 1 );
    // LsLua_log( L, LSI_LOG_NOTICE, 0, "sess_redirect [%.*s] status %d", len, cp, status);
    return 1;
}

static int  LsLua_sess_send_headers( lua_State * L )  
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "sess_send_headers  not supported yet" );
    return 0;
}

static int  LsLua_sess_headers_sent( lua_State * L )  
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "sess_headers_sent  not supported yet" );
    return 0;
}

static int  LsLua_sess_printHelper( lua_State * L, lslua_print_stream_t &s, LsLuaSession * pSession )         
{
    if (g_api->is_resp_buffer_available(pSession->getHttpSession()) == 1)
    {
        if ( LsLua_generic_print( L, &s ) == -1 )
        {
        }
    }
    else
    {
        return pSession->wait4RespBuffer(L);
    }
    return 0;
}

static int  LsLua_sess_print( lua_State * L )         
{
    LsLuaSession * pSession = LsLua_getSession( L );
    lslua_print_stream_t stream = { pSession, LsLua_resp_body_flush, 0, {0, 0, 0} };
    return(LsLua_sess_printHelper( L, stream, pSession ));
}

static int  LsLua_sess_say( lua_State * L )           
{
    // LsLuaApi::dumpStack(L, "say", 10);
    LsLuaSession * pSession = LsLua_getSession( L );
    lslua_print_stream_t stream = { pSession, LsLua_resp_body_flush, LSLUA_PRINT_FLAG_LF, {0, 0, 0} };
    return(LsLua_sess_printHelper( L, stream, pSession ));
}

static int  LsLua_sess_flush( lua_State * L )         
{
    LsLuaSession * pSession = LsLua_getSession( L );

    if (pSession) g_api->flush( pSession->getHttpSession());
    return 0;
}

static int  LsLua_sess_exit( lua_State * L )          
{
    // may be we should introduce a variable for LUA for exit already indication...
    LsLuaSession * pSession = LsLua_getSession( L );
    int exitCode = LsLuaEngine::api()->tointegerx( L, 1, NULL );
    
    if ((!pSession) || pSession->isLuaDoneFlag())
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "ignore EXIT session <%p> value = %d", pSession, exitCode);
    }
    else
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "EXIT session <%p> value = %d", pSession, exitCode);
        pSession->setLuaExitCode(exitCode);
        //
        //  need to send the exit level back to user...
        //
        // LsLua_sess__end( L );
        // LsLuaEngine::api()->yieldk( pSession->getLuaState(), 0, 0, NULL );
        LsLua_sess__end_helper( pSession );
        LsLuaEngine::api()->yieldk( L, 0, 0, NULL );
    }
    return 0;
}

//
//  The last place when I remove all the LUA and LsLuaSession
//
static inline void killThisSession(LsLuaSession * pSession)
{
    if (LsLuaEngine::debug() & 0x10)
        LsLuaSession::trace("killThisSession", pSession, NULL);
    if (pSession->getLuaStateMom())
    {
        //
        // should try to remove all socket before close
        //
        pSession->closeAllStream();
        
        if (pSession->isUrlRedirected())
        {
            g_api->flush( pSession->getHttpSession());
            g_api->end_resp( pSession->getHttpSession());
        }

        // Killing/disable the LUA State
        if (LsLuaEngine::api()->jitMode)
        {
            if (pSession->getLuaState() 
                    && (!LsLuaEngine::loadRefX(pSession, pSession->getLuaState())))
            {
                LsLua_clearSession( pSession->getLuaState() );
                LsLuaEngine::unrefX( pSession );
            }
        }
        else
        {
//            LsLuaEngine::api()->close( pSession->getLuaState() );
            LsLuaEngine::api()->close( pSession->getLuaStateMom() );
        }

        pSession->clearState();
        pSession->releaseTimer();
        
        delete pSession;
    }
}

//
//  Detected http end condition - abort current operation
//  This called by module handler 
//
void http_end_callback(void *pHttpSession, LsLuaSession * pSession)
{
    // LsLuaSession *  pSession = LsLuaSession::findByLsiSession( (lsi_session_t *)pHttpSession );
    
    if (LsLuaEngine::debug() & 0x10)
        LsLuaSession::trace("http_end_callback", pSession, NULL);
    
    if (pSession)
    {
        if (pSession->endTimer())
            pSession->endTimer()->setFlag(1);
        if (pSession->maxRunTimer())
            pSession->maxRunTimer()->setFlag(1);
        
        killThisSession(pSession);
    }
    else
    {
        // Can't find corresponding session... ignore!
        ;
    }
}

//
//  sleep_timer_callback - called from lsi_api timerCb
//  This will resume the LUA sleep call 
//
static void sleep_timer_callback(LsLuaSession *pSession, lua_State * L)
{
    if (LsLuaEngine::debug() & 0x10)
        LsLuaSession::trace("sleep_timer_callback", pSession, L);
    if (pSession->isEndFlag())
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "RACE sleep_timer_callback %p <%p>", pSession->getLuaState(), pSession );
        return;
    }

#if 0
    if (LsLuaEngine::loadRefX(pSession, L))
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, 
                   "PROBLEM: RESUME sleep_timer_callback %p <%p> ",
                   pSession->getLuaState(), pSession );
        
        g_api->append_resp_body(pSession->getHttpSession(), "ABORT LUA ERROR", 15) ;
        g_api->end_resp(pSession->getHttpSession());
        return;
    }
#endif

    //
    // 0 - the process finished...
    // LUA_YIELD - it yielded again
    //
    int ret = LsLuaEngine::api()->resumek( L, NULL, 0 );
    
#if 0
    LsLua_log( L, LSI_LOG_NOTICE, 0,
                    "RESUME sleep_timer_callback %p <%p> %d",
                    pSession->getLuaState(), pSession, ret );
#endif
    if (ret == 0) 
    {
#if 0
        // the co-routine finished.
        LsLua_log( L, LSI_LOG_NOTICE, 0,
                    "RESUME sleep_timer_callback %p <%p> %d DONE",
                    pSession->getLuaState(), pSession, ret );
#endif
        ;
    } else if (ret == LUA_YIELD)
    {
#if 0
        // the co-routine yielded again.
        LsLua_log( L, LSI_LOG_NOTICE, 0,
                    "RESUME sleep_timer_callback %p <%p> %d YIELDED",
                    pSession->getLuaState(), pSession, ret );
#endif
        ;
    }
    else
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0,
                    "RESUME sleep_timer_callback %p <%p> %d ERROR",
                    pSession->getLuaState(), pSession, ret );
    }
}

//
//
//
static int  LsLua_sess__end_helper(  LsLuaSession * pSession )
{
    if (!pSession) 
        return 0;   //  avoid base session and multiple call
    if ( pSession->isLuaDoneFlag() )
        return 0;   //  avoid base session and multiple call

    // int level = LsLuaEngine::api()->tointegerx( L, 1, NULL );
    // NOTE: level is not used at the moment...
    pSession->setEndFlag();
    pSession->setLuaDoneFlag();
    //
    // I am done - flush everything out
    //
    if (!pSession->isUrlRedirected())
    {
        // If not redirected then I should be done!
        // g_api->flush( pSession->getHttpSession());
        g_api->end_resp( pSession->getHttpSession());
    }
    return 0;
}

//
// Special end inject by LiteSpeed
//
static int  LsLua_sess__end( lua_State * L )          
{
    LsLuaSession * pSession = LsLua_getSession( L );
    
    return LsLua_sess__end_helper(  pSession );
}
    
static int  LsLua_sess_eof( lua_State * L )           
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "sess_eof  not supported yet" );
    return 0;
}

static int  LsLua_sess_time( lua_State * L )         
{
    time_t  t;
    int32_t us;
    t = g_api->get_cur_time(&us);
    
    LsLuaEngine::api()->pushinteger( L, (int)t );
    LsLuaEngine::api()->pushinteger( L, (int)us);
    return 2;
}

static int  LsLua_sess_set_version( lua_State * L )         
{
    const char * cp;
    size_t len;
    cp = LsLuaEngine::api()->tolstring( L, 1, &len );
    if (cp && len)
        LsLuaEngine::set_version(cp, len);
    return 0;
}

static int  LsLua_sess_logtime( lua_State * L )         
{
    char buf[0x100];
    
    time_t  t;
    int32_t us;
    t = g_api->get_cur_time(&us);
    
    struct tm * p_tm;
    if ( (p_tm = localtime(&t)) )
    {
        strftime(buf, sizeof(buf), "%a %d %b %Y %T %z", p_tm);
        LsLuaEngine::api()->pushstring( L, buf );
    }
    else
    {
        LsLuaEngine::api()->pushnil( L );
    }
    return 1;
}

static int  LsLua_sess_sleep( lua_State * L )         
{
    LsLuaSession * pSession = LsLua_getSession( L );
    
    if (!pSession)
    {
        LsLuaEngine::api()->pushinteger( L, -1);
        LsLuaEngine::api()->pushstring( L, "NO LUASESSION" );
        return 0;
    }

    register int avail = LsLuaEngine::api()->gettop(L);
    if (avail < 1)
    {
        LsLuaEngine::api()->pushinteger( L, -1);
        LsLuaEngine::api()->pushstring( L, "missing sleep timevalue" );
        return 2;
    }
    int msec = 1000 * LsLuaEngine::api()->tonumberx(L, 1, NULL);
    if (msec < 1)
    {
        LsLuaEngine::api()->pushinteger( L, -1);
        LsLuaEngine::api()->pushstring( L, "bad sleep timevalue" );
        return 2;
    }
    LsLuaEngine::api()->pushinteger( L, 0);
    LsLuaEngine::api()->pushnil( L );
    // LsLua_log( L, LSI_LOG_NOTICE, 0, "sleep %d msec" , msec);

    if (LsLuaEngine::debug() & 0x10)
        LsLuaSession::trace("SETTIMER", pSession, L);
    pSession->setTimer(msec, sleep_timer_callback, L, SET_TIMER_NORMAL);
    return LsLuaEngine::api()->yieldk( L, 2, 0, NULL );
}

static int  LsLua_sess_is_subrequest( lua_State * L ) 
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "sess_is_subrequest  not supported yet" );
    return 0;
}

static int  LsLua_sess_on_abort( lua_State * L )      
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "sess_on_abort  not supported yet" );
    return 0;
}

//
//  Using OPENSSL SHA1 implementation 
//
static int LsLua_sess_sha1_bin( lua_State * L)
{
    const uint8_t *         src;
    SHA_CTX                 shaCtx;
    uint8_t                 shaBuf[SHA_DIGEST_LENGTH];
    size_t                  len;

    // LsLuaApi::dumpStack( L, "Enter LsLua_sess_sha1_bin", 10);
    if ( ( LsLuaEngine::api()->gettop( L ) != 1 ) )
    {
        LsLuaEngine::api()->pushnil( L );
        LsLuaEngine::api()->pushstring( L, "expecting one argument");
        return 2;
    }
    if ( ( !( src = (uint8_t *)LsLuaEngine::api()->tolstring(L, 1, &len) ) ) || ( !len ) )
    {
        src     = (uint8_t *) "";
        len    = 0;
    }
    SHA1_Init(&shaCtx);
    SHA1_Update(&shaCtx, src, len);
    SHA1_Final(shaBuf, &shaCtx);

    LsLuaApi::pop(L, 1);    // pop the ls
    LsLuaEngine::api()->pushlstring( L, (const char *)shaBuf, sizeof(shaBuf));
    // LsLuaApi::dumpStack( L, "Exit LsLua_sess_sha1_bin", 10);
    return 1;
}

static int LsLua_sess_tostring( lua_State * L )
{
    char    buf[0x100];

    snprintf( buf, 0x100, "<Lslua_sess %p>", L );
    LsLuaEngine::api()->pushstring( L, buf );
    return 1;
}

static int LsLua_sess_gc( lua_State * L )
{
    // LsLua_log( L, LSI_LOG_NOTICE, 0, "<ls.session GC>");
    if (LsLuaEngine::debug() & 0x10)
    {
        LsLuaSession * pSession = LsLua_getSession( L );
        LsLuaSession::trace("<LsLua_sess _gc>", pSession, L);
    }
    return 0;
}

static const luaL_Reg lslua_session_funcs[] =
{
//  { "status",         LsLua_sess_status           },
    { "debug",          LsLua_sess_debug            }, // special function to dump info
    { "exec",           LsLua_sess_exec             },
    { "redirect",       LsLua_sess_redirect         },
    { "send_headers",   LsLua_sess_send_headers     },
    { "headers_sent",   LsLua_sess_headers_sent     },
    { "puts",           LsLua_sess_print            },
    { "print",          LsLua_sess_print            },
    { "say",            LsLua_sess_say              },
    { "log",            LsLua_sess_log              },
    { "flush",          LsLua_sess_flush            },
    { "exit",           LsLua_sess_exit             },
    { "_end",           LsLua_sess__end             },  // internal ls special end
    { "eof",            LsLua_sess_eof              },
    { "sleep",          LsLua_sess_sleep            },
    
    { "time",           LsLua_sess_time             },
    { "logtime",        LsLua_sess_logtime          },
    { "set_version",    LsLua_sess_set_version      },
    { "is_subrequest",  LsLua_sess_is_subrequest    },
    { "on_abort",       LsLua_sess_on_abort         },
    
    { "sha1_bin",       LsLua_sess_sha1_bin         },
    {NULL, NULL}
};

//
//  ls.status SESSION
//
static int LsLua_get(lua_State * L)
{
    size_t          len;
    const char *    cp;
    // const char * tag = "LsLua_get";
    // LsLuaApi::dumpStack( L, tag, 10);
    LsLuaSession * pSession = LsLua_getSession( L );
    cp = LsLuaEngine::api()->tolstring( L, 2, &len);
    if (cp && len)
    {
        // only support status for now!
        if (!strncmp(cp, "status", 6)) 
        {
            if ( pSession && pSession->getHttpSession() )
            {
                int status = g_api->get_status_code( pSession->getHttpSession());
                LsLuaEngine::api()->pushinteger( L, status);
                return 1;
            }
        }
        else
        {
            LsLua_log( L, LSI_LOG_NOTICE, 0, "ls GET %s notready", cp);
        }
    }
    else
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "ls GET BADSTACK");
    }
    LsLuaEngine::api()->pushinteger(L, -1);
    return 1;
}

static int LsLua_set(lua_State * L)
{
    size_t          len;
    const char *    cp;
    // LsLuaApi::dumpStack( L, "ls SET", 10);
    LsLuaSession * pSession = LsLua_getSession( L );
    cp = LsLuaEngine::api()->tolstring( L, 2, &len);
    if (cp && len)
    {
        // only support status for now!
        if (!strncmp(cp, "status", 6)) 
        {
            if ( pSession && pSession->getHttpSession() )
            {
                int status = LsLuaEngine::api()->tointegerx( L, 3, NULL);
                
                g_api->set_status_code( pSession->getHttpSession(), status);
                LsLuaEngine::api()->pushinteger( L, status);
                return 1;
            }
        }
        else
        {
            LsLua_log( L, LSI_LOG_NOTICE, 0, "ls SET %s notready", cp);
        }
    }
    else
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "ls SET BADSTACK");
    }
    return 1;
}

void LsLua_create_session( lua_State * L )
{
    // LsLuaApi::dumpStack( L, "after create_session", 10 );
    LsLuaEngine::api()->openlib( L, LS_LUA, lslua_session_funcs, 0 );
    
    // push constant here
    LsLuaEngine::api()->pushlightuserdata(L, NULL);
    LsLuaEngine::api()->setfield(L, -2, "null");
    // LsLuaApi::dumpStack( L, "after create_session constant", 10 );
}

void LsLua_createGlobal( lua_State * L )
{
    LsLuaEngine::api()->createtable(L, 0, 1);
    LsLuaEngine::api()->pushvalue(L, -1);
    LsLuaEngine::api()->setfield(L, -2, "_G");
    
    LsLuaEngine::api()->createtable(L, 0, 1);
    
    if (LsLuaEngine::api()->jitMode)
    {
        LsLuaEngine::api()->pushvalue(L, LsLuaEngine::api()->LS_LUA_GLOBALSINDEX);
        LsLuaEngine::api()->setfield(L, -2, "__index");
        LsLuaEngine::api()->setmetatable(L, -2);
        LsLuaEngine::api()->replace(L, LsLuaEngine::api()->LS_LUA_GLOBALSINDEX);
    }
    else
    {
        LsLuaEngine::api()->getglobal(L, "_G");
        LsLuaEngine::api()->setfield(L, -2, "__index");
        LsLuaEngine::api()->setmetatable(L, -2);
        LsLuaEngine::api()->rawseti(L, LsLuaEngine::api()->LS_LUA_REGISTRYINDEX,
                                    LsLuaEngine::api()->LS_LUA_RIDX_GLOBALS);
    }
}

void LsLua_create_sess_meta( lua_State * L )
{
    // LsLuaApi::dumpStack( L, "BEGIN create_session_meta", 10 );
    LsLuaEngine::api()->createtable( L, 0, 2 );
    LsLuaEngine::api()->pushcclosure( L, LsLua_get, 0);
    LsLuaEngine::api()->setfield( L, -2, "__index");
    LsLuaEngine::api()->pushcclosure( L, LsLua_set, 0);
    LsLuaEngine::api()->setfield( L, -2, "__newindex");
    LsLuaEngine::api()->pushcclosure( L, LsLua_sess_gc, 0);
    LsLuaEngine::api()->setfield( L, -2, "__gc");
    LsLuaEngine::api()->pushcclosure( L, LsLua_sess_tostring, 0);
    LsLuaEngine::api()->setfield( L, -2, "__tostring");
    LsLuaEngine::api()->setmetatable( L, -2);
    // LsLuaApi::dumpStack( L, "END create_session_meta", 10 );
}
//
//  ls.status END SESSION
//

//
//  set/get elements from header
//
extern int LsLua_header_set(lua_State *);
extern int LsLua_header_get(lua_State *);

#define LSLUA_RESP_HEADER "LS_RESP_HEADER"
#define LSLUA_RESP "LS_RESP"
#define LSLUA_REQ "LS_REQ"

static int  LsLua_req_start_time( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_start_time  not supported yet" );
    return 0;
}

static int LsLua_req_http_version( lua_State * L )
{
    LsLuaSession * pSession = LsLua_getSession( L );
    PUSH_LSI_ENV(pSession->getHttpSession(), g_api->get_req_var_by_id, LSI_REQ_VAR_SERVER_PROTO);

    return 1;
}

static int LsLua_req_raw_header( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_raw_header  not supported yet" );
    return 0;
}

static int LsLua_req_clear_header( lua_State * L )
{
    register int    avail; 
    avail = LsLuaEngine::api()->gettop(L);
    if (avail < 1)
    {
        // nothing to do
        return 0;
    }
    // pop the rest of junk from stack
    if (avail > 1)
        LsLuaApi::pop(L, avail - 1);
    // LsLuaEngine::api()->getglobal(L, "ls.header"); // This wont work use nil instead
    LsLuaEngine::api()->pushnil( L );
    LsLuaEngine::api()->insert(L, -2);
    LsLuaEngine::api()->pushnil( L );
    return LsLua_header_set(L);
}

static int LsLua_req_set_header( lua_State * L )
{
    register int    avail; 
    avail = LsLuaEngine::api()->gettop(L);
    if (avail < 2)
    {
        // do nothing
        return 0;
    }
    // pop the rest of junk from stack
    if (avail > 2)
        LsLuaApi::pop(L, avail - 2);
    // LsLuaEngine::api()->getglobal(L, "ls.header"); // This wont work use nil instead
    LsLuaEngine::api()->pushnil( L );
    LsLuaEngine::api()->insert(L, -3);
    return LsLua_header_set(L);
}

static int LsLua_req_get_headers( lua_State * L )
{
    LsLuaSession * pSession = LsLua_getSession( L );

    PUSH_LSI_BUF(pSession->getHttpSession(), g_api->get_req_raw_headers);

    return 1;
}

static int LsLua_req_get_method( lua_State * L )
{
    LsLuaSession * pSession = LsLua_getSession( L );
    PUSH_LSI_ENV(pSession->getHttpSession(), g_api->get_req_var_by_id, LSI_REQ_VAR_REQ_METHOD);
    return 1;
}

static int LsLua_req_set_method( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_set_method  not supported yet" );
    return 0;
}

static int LsLua_req_set_uri( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_set_uri  not supported yet" );
    return 0;
}

static int LsLua_req_set_uri_args( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_set_uri_args  not supported yet" );
    return 0;
}

static int LsLua_req_get_uri_args( lua_State * L )
{
    LsLuaApi::pushTableItem(L, LS_LUA, LS_LUA_URIARGS, 1);
    return 1;
}

static int LsLua_req_get_post_args( lua_State * L )
{
    LsLuaApi::pushTableItem(L, LS_LUA, LS_LUA_POSTARGS, 1);
    return 1;
}

//
// simple key=value parser - 
//  no check of length of string since the LiteSpeed Server 
//      already making sure the valid string for me
//
static int searchFor(char * from, char key)
{
    register char   c;
    register int    num = 0;
    while ((c = *from++) && (c != key) && (!isspace(c)))
        num++;
    return num;
}

static int LsLua_req_read_body( lua_State * L )
{
    LsLuaSession *  pSession = LsLua_getSession( L );
    
    if (!pSession)
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "LsLua_req_read_body: FAILED TO ACQUIRE LUASESSION" );
        return 0;
    }

    //
    //  check if we have the whole req_body!
    //
    if (pSession->isLuaWaitReqBody())
    {
        // I was in wait state...
        LsLua_log( L, LSI_LOG_NOTICE, 0, "req_read_body: ERROR SHOULD BE IN YIELDING STATE." );
        return 0;
    }
    if (!g_api->is_req_body_finished( pSession->getHttpSession()))
    {
        // LsLua_log( L, LSI_LOG_NOTICE, 0, "req_body_read YIELD" );
        pSession->setLuaWaitReqBody();
        return LsLuaEngine::api()->yieldk( pSession->getLuaState(), 0, 0, NULL );
    }
    return 0;
}

//
//  process the req_read_body
//
int  LsLua_req_read_body_parser(lua_State * L)
{
    char            tmp[0x2000];
    register int    nb, nv;
    register char * outbuf;
    LsLuaSession *  pSession = LsLua_getSession( L );

    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_body_read  START PROCESSING" );
    
    outbuf = tmp;
    nv = 0x2000;
    nb = 0;
    if (g_api->get_req_content_length(pSession->getHttpSession()))
    {
        GET_LSI_BUF(pSession->getHttpSession(), g_api->read_req_body);
        _tmpbuf[_tmpsize] = 0; // enforce null terminated string
        LsLua_log( L, LSI_LOG_NOTICE, 0, "PARSE req_read_body [%s]", _tmpbuf );
        if (_tmpsize)
        {
            //
            // parse Req parameters... the first line 
            // foo=bar&bar=baz&bar=blah&sick=fever
            // User-Agent: curl/7.32.0
            // Host: localhost:8088
            // ...

            register int    len;
            register char * from = _tmpbuf;
            do
            {
                char * p_key, * p_val;
                if ( (len = searchFor(from, '=')) )
                {
                    p_key = from;
                    from += len;
                    *from++ = 0;
                    if ( (len = searchFor(from, '&')) )
                    {
                        // may need to figure out if it is number later
                        p_val = from;
                        from += len;
                        *from++ = 0;
                        // got a pair here
                        nb = snprintf(outbuf, nv, "ls.%s.%s=\"%s\" ", LS_LUA_POSTARGS, p_key, p_val);
                        outbuf += nb;
                        nv -= nb;
                    }
                }
            } while (len);
            *outbuf = 0;
            if (nb)
                LsLuaApi::execLuaCmd(L, tmp);

            LsLua_log( L, LSI_LOG_NOTICE, 0, "CMD req_read_body [%s]", tmp );
        }
    }
    return 0;
}

static int LsLua_req_discard_body( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_discard_body  not supported yet" );
    return 0;
}

static int LsLua_req_get_body_data( lua_State * L )
{
    LsLuaSession * pSession = LsLua_getSession( L );

    if (g_api->get_req_content_length(pSession->getHttpSession()))
    {
        PUSH_LSI_BUF(pSession->getHttpSession(), g_api->read_req_body);
    }
    else
        LsLuaEngine::api()->pushnil( L );

    return 1;
}

static int LsLua_req_get_body_file( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_get_body_file  not supported yet" );
    return 0;
}

static int LsLua_req_set_body_data( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_set_body_data  not supported yet" );
    return 0;
}

static int LsLua_req_set_body_file( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_set_body_file  not supported yet" );
    return 0;
}

static int LsLua_req_init_body( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_init_body  not supported yet" );
    return 0;
}

static int LsLua_req_append_body( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_append_body  not supported yet" );
    return 0;
}

static int LsLua_req_finish_body( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_finish_body  not supported yet" );
    return 0;
}

static int LsLua_req_socket( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "req_socket  not supported yet" );
    return 0;
}

//
//  handy routines to dumb out the content of LSI data
//
static int LsLua_dump_lsi( lua_State * L )
{
    int numOut = 0;

    LsLuaSession * pSession = LsLua_getSession( L );

    LsLuaEngine::api()->pushstring( L , "get_req_body");
    if (g_api->get_req_content_length(pSession->getHttpSession()))
    {
        PUSH_LSI_BUF(pSession->getHttpSession(), g_api->read_req_body);
    }
    else
        LsLuaEngine::api()->pushnil( L );
    numOut += 2;

    LsLuaEngine::api()->pushstring( L , "get_org_req_uri");
    PUSH_LSI_BUF(pSession->getHttpSession(), g_api->get_req_org_uri);
    numOut += 2;

    LsLuaEngine::api()->pushstring( L , "get_req_uri");
    LsLuaEngine::api()->pushstring( L, g_api->get_req_uri(pSession->getHttpSession(), NULL ) );
    numOut += 2;

    return numOut;
}

static int LsLua_req_tostring( lua_State * L )
{
    char    buf[0x100];

    snprintf( buf, 0x100, "<ls.req %p>", L );
    LsLuaEngine::api()->pushstring( L, buf );
    return 1;
}

static int LsLua_req_gc( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "<ls.req GC>");
    return 0;
}

static const luaL_Reg lslua_req_funcs[] =
{
    { "start_time",     LsLua_req_start_time    },
    { "http_version",   LsLua_req_http_version  },
    { "raw_header",     LsLua_req_raw_header    },
    { "clear_header",   LsLua_req_clear_header  },
    { "set_header",     LsLua_req_set_header    },
    { "get_headers",    LsLua_req_get_headers   },
    { "get_method",     LsLua_req_get_method    },
    { "set_method",     LsLua_req_set_method    },
    { "set_uri",        LsLua_req_set_uri       },
    { "set_uri_args",   LsLua_req_set_uri_args  },
    { "get_uri_args",   LsLua_req_get_uri_args  },
    { "get_post_args",  LsLua_req_get_post_args },
    { "read_body",      LsLua_req_read_body     },
    { "discard_body",   LsLua_req_discard_body  },
    { "get_body_data",  LsLua_req_get_body_data },
    { "get_body_file",  LsLua_req_get_body_file },
    { "set_body_data",  LsLua_req_set_body_data },
    { "set_body_file",  LsLua_req_set_body_file },
    { "init_body",      LsLua_req_init_body     },
    { "append_body",    LsLua_req_append_body   },
    { "finish_body",    LsLua_req_finish_body   },
    { "socket",         LsLua_req_socket        },
    { "dump_lsi",       LsLua_dump_lsi          },
    {NULL, NULL}
};

static const luaL_Reg lslua_req_meta_sub[] =
{
    {"__gc",        LsLua_req_gc},
    {"__tostring",  LsLua_req_tostring},
    {NULL, NULL}
};

void LsLua_create_req_meta( lua_State * L )
{
    LsLuaEngine::api()->openlib( L, LS_LUA ".req", lslua_req_funcs, 0 );
    LsLuaEngine::api()->newmetatable( L, LSLUA_SESSION );
    LsLuaEngine::api()->openlib( L, NULL, lslua_req_meta_sub, 0 );

    // pushliteral
    LsLuaEngine::api()->pushlstring( L, "__index", 7 );
    LsLuaEngine::api()->pushvalue( L, -3 );
    LsLuaEngine::api()->rawset( L, -3 );      // metatable.__index = methods
    // pushliteral
    LsLuaEngine::api()->pushlstring( L, "__metatable", 11 );
    LsLuaEngine::api()->pushvalue( L, -3 );
    LsLuaEngine::api()->rawset( L, -3 );      // metatable.__metatable = methods

    LsLuaEngine::api()->settop( L, -3 );     // pop 2

    // LsLuaApi::dumpStack( L, "DONE LsLua_create_req_meta", 10 );
}

// dummy for testing purpose
static int  LsLua_resp_set_header_dummy( lua_State * L )
{
    LsLuaSession * pSession = LsLua_getSession( L );

    g_api->set_resp_header(pSession->getHttpSession(), 
                LSI_RESP_HEADER_UNKNOWN, "H1",   2, "V1",   2, LSI_HEADER_SET);
    g_api->set_resp_header(pSession->getHttpSession(), 
                LSI_RESP_HEADER_UNKNOWN, "H2",   2, "V2",   2, LSI_HEADER_SET);
    g_api->set_resp_header(pSession->getHttpSession(), 
                LSI_RESP_HEADER_UNKNOWN, "H3",   2, "V3",   2, LSI_HEADER_SET);
    return 0;
}

//
//  resp SECTION
//
static int LsLua_resp_get_headers( lua_State * L )
{    
    LsLuaSession * pSession = LsLua_getSession( L );
#define MAX_RESP_HEADERS_NUMBER     50
    struct iovec iov[MAX_RESP_HEADERS_NUMBER];
    char bigout[0x1000];
    register int size = 0, len = 0;
    register char * cp = bigout;

    //
    //  dummy code to enable our development 
    // REMOVE THIS on real
    LsLua_resp_set_header_dummy( L );
    
    int count = g_api->get_resp_headers(pSession->getHttpSession(),
                            iov, MAX_RESP_HEADERS_NUMBER);
    for(int i=0; i<count; ++i)
    {
        len = iov[i].iov_len;
        memcpy(cp, iov[i].iov_base, len);
        cp += len++;    // watch len++
        *cp++ = '+';
        size += len;    // just incr
    }
    // *cp = 0;
    if ( size )
    {        
        --cp; --size;
        *cp = 0;
        LsLuaEngine::api()->pushlstring( L, bigout, size );
    }    
    else
        LsLuaEngine::api()->pushnil( L );

    // LsLuaApi::pushTableItem(L, LS_LUA, LS_LUA_HEADER, 1);
    return 1;
}

static int LsLua_resp_tostring( lua_State * L )
{
    char    buf[0x100];

    snprintf( buf, 0x100, "<ls.resp %p>", L );
    LsLuaEngine::api()->pushstring( L, buf );
    return 1;
}

static int LsLua_resp_gc( lua_State * L )
{
    LsLua_log( L, LSI_LOG_NOTICE, 0, "<ls.resp GC>");
    return 0;
}

static const luaL_Reg lslua_resp_funcs[] =
{
    { "get_headers",    LsLua_resp_get_headers  },
    {NULL, NULL}
};

static const luaL_Reg lslua_resp_meta_sub[] =
{
    {"__gc",        LsLua_resp_gc},
    {"__tostring",  LsLua_resp_tostring},
    {NULL, NULL}
};

void LsLua_create_resp_meta( lua_State * L )
{
    LsLuaEngine::api()->openlib( L, LS_LUA ".resp", lslua_resp_funcs, 0 );
    LsLuaEngine::api()->newmetatable( L, LSLUA_RESP );
    LsLuaEngine::api()->openlib( L, NULL, lslua_resp_meta_sub, 0 );

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

//
//  Setup the runtime tables for each Session STACK
//
static void LsLua_createMiscTable( lua_State * L )
{
    // LsLuaApi::dumpStack( L, "BEGIN createMiscTable", 10 );
    LsLuaEngine::api()->getglobal(L, LS_LUA); // load ls env first

    LsLuaApi::createTable(L, NULL, LS_LUA_CTX);
    LsLuaApi::createTable(L, NULL, LS_LUA_URIARGS);
    LsLuaApi::createTable(L, NULL, LS_LUA_POSTARGS);

    LsLua_create_sess_meta(L);
    LsLuaApi::pop(L, 1);    // pop the ls

    // LsLuaApi::dumpStack( L, "DONE createMiscTable", 10 );
}

