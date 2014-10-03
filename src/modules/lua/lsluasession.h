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

#ifndef LSLUAREQ_H
#define LSLUAREQ_H

#include <stddef.h>
#include <inttypes.h>
#include <socket/gsockaddr.h>

#include <ls.h>
#include <edluastream.h>
#include <lsluaapi.h>

class LsLuaStreamData;
class LsLuaTimerData;

//
//  Callback function type - reversed LsLuaSession and lua_State for purpose
//
typedef void (* LsLuaSesionCallBackFunc_t)(LsLuaSession *, lua_State *);

class LsLuaSession
{
public:
    LsLuaSession();
    ~LsLuaSession();

    void init( lsi_session_t * pSession )
    {
        m_pHttpSession = pSession;
        m_pVM = NULL;
    }
    // 5.2 LUA
    int setupLuaEnv(lua_State * L, LsLuaUserParam * pUser);
    // JIT LUA 5.1
    int setupLuaEnvX(lua_State * L, LsLuaUserParam * pUser);

    inline lsi_session_t * getHttpSession() const
    {   return m_pHttpSession;      }

    inline lua_State * getLuaState() const
    {   return m_pVM;      }

    inline lua_State * getLuaStateMom() const
    {   return m_pVM_mom;      }
    
    inline void clearState()
    {   m_pVM = NULL, m_pVM_mom = NULL; m_pHttpSession = NULL; }

    // static LsLuaSession *  findByLsiSession ( const lsi_session_t * pSession) ;
    // static LsLuaSession *  findByLuaState ( const lua_State * L) ;

    // these three function to control the req_body wait
    inline void setLuaWaitReqBody()
    {   m_waitReqBody = 1; }
    inline int isLuaWaitReqBody() const
    {   return m_waitReqBody ; }
    inline void clearLuaWaitReqBody()
    {   m_waitReqBody = 0; }
    
    // these throttle the lua execution speed 
    inline void setWaitLineExec()
    {   m_waitLineExec = 1; }
    inline int isWaitLineExec() const
    {   return m_waitLineExec ; }
    inline void clearWaitLineExec()
    {   m_waitLineExec = 0; }

    inline void setLuaExitCode(int code)
    {   m_exitCode = code;  }

    inline int getLuaExitCode() const
    {   return m_exitCode;  }

    inline void setLuaDoneFlag()
    {   m_doneFlag = 1;  }
    
    inline int isLuaDoneFlag() const
    {   return m_doneFlag; }
    
    inline int key() const
    {   return m_key; }
    
    void    resume(lua_State *, int numarg);

    // schedule a LUA state timer
    void    setTimer(int msec, LsLuaSesionCallBackFunc_t, lua_State *, int flag);

    static EdLuaStream * newEdLuaStream(lua_State *); // used for allocator
    void closeAllStream();                  // called by end of session
    void markCloseStream(lua_State *, EdLuaStream *);    // called by GC incase LUA GC is active
    
    // URL Redirected
    inline void setUrlRedirected()
    {   m_UrlRedirected = 1; }
    inline int isUrlRedirected() const
    {   return m_UrlRedirected; }
    inline void clearUrlRedirected()
    {   m_UrlRedirected = 0; }

    inline void setEndFlag()
    {   m_endFlag = 1; }
    inline int isEndFlag() const
    {   return m_endFlag; }
    inline void clearEndFlag()
    {   m_endFlag = 0; }
    
    // these control the lua resp
    inline void setWaitRespBuf()
    {   m_waitRespBuf = 1; }
    inline int isWaitRespBuf() const
    {   return m_waitRespBuf ; }
    inline void clearWaitRespBuf()
    {   m_waitRespBuf = 0; }
    
    inline LsLuaTimerData * endTimer() const
    {   return m_pEndTimer; }
    
    inline LsLuaTimerData * maxRunTimer() const
    {   return m_pMaxTimer; }
    
    static inline void trace( const char* tag, LsLuaSession* pSess, lua_State* L)
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0,
                "TRACE %s {%p, %p} [%p %p] %d %d %d",
                tag, pSess, L, 
                pSess ? pSess->getLuaState() : NULL, pSess->getHttpSession(),
                pSess->m_key,
                pSess->m_endFlag,
                pSess->m_doneFlag
             );
    }
    inline int * getRefPtr() 
    {   return &m_ref; }
    inline int  getRef() const
    {   return m_ref; }
    
    inline int getTop() const
    {   return m_top; };
    inline void setTop(int top) 
    {   m_top = top; };
    
    void releaseTimer();
    
    // resp control
    int onWrite(lsi_session_t * httpSession);    // called by modlua.c
    int wait4RespBuffer(lua_State *L); 
private:    
    lua_State    * m_wait4RespBuf_L;
    
private:
    LsLuaSession( const LsLuaSession& other );
    LsLuaSession& operator=( const LsLuaSession& other );
    bool operator==( const LsLuaSession& other );

private:
    lsi_session_t *  m_pHttpSession;
    lua_State    * m_pVM; 
    lua_State    * m_pVM_mom;       // save the parent state

    //
    //  LUA status 
    //
    int         m_exitCode;         // from ls.exit
    uint32_t    m_doneFlag    : 1;  // done - reached ls._end 
    uint32_t    m_runFlag     : 1;  // running - most of the time
    uint32_t    m_waitReqBody : 1;  // waiting for req_body
    uint32_t    m_waitLineExec : 1; // waiting Lua line Exec
    uint32_t    m_waitRespBuf : 1;  // waiting for resp buffer
    uint32_t    m_UrlRedirected : 1;// Url has been redirected
    uint32_t    m_endFlag       : 1;
    int         m_key;              // my key
    int         m_ref;              // my ref for LUA
    int         m_top;
    LsLuaTimerData * m_pEndTimer;   // track the last endTimer
    LsLuaTimerData * m_pMaxTimer;   // track the maxTimer
    LsLuaStreamData * m_pStream;    // my stream pool
    
    // Need to track all LUA installed timer 
    LsLuaTimerData *    m_pTimerList;
    void addTimerToList(LsLuaTimerData * pTimerData);
    void rmTimerFromList(LsLuaTimerData * pTimerData);
    void releaseTimerList( );
    void dumpTimerList( const char * tag );

    //
    // We don't allow to use std::map...
    // Then build a linear list to track the session for now.
    //
    void clearLuaStatus();   
    static int  s_key;              // System wise unique key
    
    // callback timer 
    static void timerCb(void *);

    // reach max runtime callback
    // static void maxruntimeCb(void *);
    static void maxRunTimeCb(LsLuaSession *pSession, lua_State *L);
    
    //
    // Hook point for LUA line hook + Lsi timer callback
    //
    static void luaLineLooper(LsLuaSession *pSession, lua_State *L);
    static void luaLineHookCb(lua_State * L, lua_Debug *ar);
    inline void upLuaCounter()
    { m_luaLineCount++; }
    u_int32_t m_luaLineCount;    // counter how many time this got called
public:
    u_int32_t getLuaCounter()
    { return m_luaLineCount; }
};


void LsLua_create_session( lua_State * L );
void LsLua_create_req_meta( lua_State * L );
void LsLua_create_resp_meta( lua_State * L );
void LsLua_create_session_meta( lua_State * L );

//
//  keep track of EdStream
//
class LsLuaStreamData
{
public: // no need to protect myself
    LsLuaStreamData(EdLuaStream * sock, LsLuaStreamData * _next)
    : m_sock(sock)
    , m_next(_next)
    , m_active(1)
    {
    }
    inline EdLuaStream * stream() const
    { return m_sock; }
    inline int isActive() const
    {   return m_active; }
    inline void setNotActive() 
    { m_active = 0; }
    inline LsLuaStreamData * next() const
    { return m_next; }
    inline int isMatch(EdLuaStream * sock)
    { return sock == m_sock; }
    void close(lua_State *);
private:
    LsLuaStreamData( );
    LsLuaStreamData( const LsLuaStreamData& other );
    LsLuaStreamData& operator=( const LsLuaStreamData& other );
    bool operator==( const LsLuaStreamData& other );
private:
    EdLuaStream * m_sock;
    LsLuaStreamData * m_next;
    int m_active; // the stream still active
};

//
//  for Timer callback
//
class LsLuaTimerData
{
public: // no need to protect myself
    LsLuaTimerData(LsLuaSession *pSession, LsLuaSesionCallBackFunc_t func, lua_State * L)
    {
       m_key = pSession->key();
       m_pFunc = func;
       m_pSession = pSession;
       m_L = L;
       m_flag = 0;
       m_id = 0;
       m_pNext = NULL;
    }
    ~LsLuaTimerData() { m_key = 0; }
    int key() const
    {   return m_key; }
    LsLuaSession * session() const
    {   return m_pSession; }
    lua_State * L() const
    {   return m_L; }
    void timerCallBack() const
    {   (*m_pFunc)(m_pSession, m_L); }
    int flag() const
    {   return m_flag; }
    void setFlag( int flag ) 
    {   m_flag = flag; }
    int id() const
    {   return m_id; }
    void setId( int id )
    {   m_id = id; }
    
    LsLuaTimerData * next()
    {   return m_pNext; }
    
    void setNext(LsLuaTimerData * pTimerData)
    {   m_pNext = pTimerData; }
    
private:    
    LsLuaTimerData( );
    LsLuaTimerData( const LsLuaTimerData& other );
    LsLuaTimerData& operator=( const LsLuaTimerData& other );
    bool operator==( const LsLuaTimerData& other );
private:    
    int             m_flag;         // Session deleted
    int             m_key;
    LsLuaSesionCallBackFunc_t        m_pFunc;
    LsLuaSession *  m_pSession;
    lua_State *     m_L;
    int             m_id;
    LsLuaTimerData * m_pNext;       // track the timer by each LUA Session
};

#endif // LSLUAREQ_H
