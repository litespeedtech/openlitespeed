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
#include "lsluasession.h"
#include "lsluaengine.h"
#include "lsluascript.h"
#include "lsluaapi.h"
#include "ls_lua.h"
#include "../addon/include/ls.h"
#include <http/httplog.h>
#include <log4cxx/logger.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

// misc error strings...
#define LUA_ERRSTR_SANDBOX "\r\nERROR: LUA SANDBOX SETUP\r\n"
#define LUA_ERRSTR_SCRIPT  "\r\nERROR: FAILED TO LOAD LUA SCRIPT\r\n"
#define LUA_ERRSTR_ERROR  "\r\nERROR: LUA ERROR\r\n"

#define LUA_RESUME_ERROR    "LUA RESUME SCRIPT ERROR"

#define LS_LUA_BEGINSTR "return function() ngx=ls "
#define LS_LUA_ENDSTR "\nls._end(0) end"

#define LS_LUA_THREADTABLE "_lsThread"


int             LsLuaEngine::s_firstTime = 1;           // firstTime parse param
int             LsLuaEngine::s_debugLevel = 0;          // debugLevel from server
int             LsLuaEngine::s_ready = 0;               // not ready
ls_lua_t *      LsLuaEngine::s_luaSys = NULL;
ls_lua_api_t *  LsLuaEngine::s_luaApi = NULL;
lua_State *     LsLuaEngine::s_stateSystem = NULL;

int             LsLuaEngine::s_maxRunTime = 0;           // 0 no limit
int             LsLuaEngine::s_maxLineCount = 0;         // default is 0 (10000000 instruction per yield ~ 1000000 lines)
int             LsLuaEngine::s_jitLineMod = 10000;       // JIT hookcount is very fast... we need to slow it down
                                                         // JIT hookcount is different
                                                         // It based on abnormal LUA code ("C")
                                                         
int             LsLuaEngine::s_debug = 0;                // internal LUA debug trace
int             LsLuaEngine::s_pauseTime = 500;          // pause time 500 msec per yield
char *          LsLuaEngine::s_lib = NULL;               // user specified
char            LsLuaEngine::s_luaName[0x10] = "LUA";    // default module name
const char *    LsLuaEngine::s_syslib = "/usr/local/lib/libluajit-5.1.so"; // default
char *          LsLuaEngine::s_luaPath = NULL;           // user specified LUA_PATH
const char *    LsLuaEngine::s_sysLuaPath = "/usr/local/lib/lua"; // default LUA_PATH
char            LsLuaEngine::s_version[0x20] = "LUA-NOT-READY"; // default LUA_VERSION

LsLuaEngine::TYPE LsLuaEngine::s_type = LSLUA_ENGINE_REGULAR;
LsLuaEngine::LsLuaEngine()
{
}

LsLuaEngine::~LsLuaEngine()
{
}

int LsLuaEngine::init()
{
    const char *  pLibPath;
    s_ready = 0;     // set no ready
    
    pLibPath = s_lib ? s_lib : s_syslib;

    char    errbuf[MAX_ERRBUF_SIZE];
    if (!(s_luaSys = ls_lua_alloc(LS_LUA_SYS, pLibPath, errbuf)))
    {
        g_api->log(NULL, LSI_LOG_ERROR, "[LUA] %s\n", errbuf);
        return -1;
    }
    s_luaApi = s_luaSys->dl;
    if (s_luaApi->jitMode)
    {
        s_type = LSLUA_ENGINE_JIT;
        strcpy(s_luaName, "JIT");
    }
    else
    {
        s_type = LSLUA_ENGINE_REGULAR;
        strcpy(s_luaName, "LUA");
    }
    // check if the number is correctly set
    g_api->log(NULL, LSI_LOG_DEBUG
                    , "%s REGISTRYINDEX[%d] GLOBALSINDEX[%d]\n"
                    , s_luaName, s_luaApi->LS_LUA_REGISTRYINDEX, s_luaApi->LS_LUA_GLOBALSINDEX
              );
    g_api->log(NULL, LSI_LOG_DEBUG
                    , "%s lib[%s] luapath[%s]\n"
                    , s_luaName, s_lib?s_lib:"" , s_luaPath?s_luaPath:""
              );
    g_api->log(NULL, LSI_LOG_DEBUG
                    , "%s maxruntime[%d] maxlinecount[%d]\n"
                    , s_luaName, s_maxRunTime , s_maxLineCount
              );
    g_api->log(NULL, LSI_LOG_DEBUG
                    , "%s pause[%dmsec] jitlinemod[%d]\n"
                    , s_luaName, s_pauseTime , s_jitLineMod 
              );
    
    if (s_type == LSLUA_ENGINE_JIT)
    {
        if (s_luaApi->LS_LUA_REGISTRYINDEX != -10000) {
            g_api->log(NULL, LSI_LOG_WARN,
                         "JIT PATCH REGISTRYINDEX IS NOT -10000\n");
            // may be I should return error here here!
            return -1;
        }
    }

    // create a system LUA stack
    s_stateSystem = newLuaConnection();
    if (!s_stateSystem)
        return -1;

    injectLsiapi(s_stateSystem);
    // NOTE - "ls" left in the STACK of s_stateSystem GC happier
    
    // Setup info 
    LsLuaApi::execLuaCmd(getSystemState(), "ls.set_version(_VERSION)");
// LsLuaApi::dumpStack(s_stateSystem, "engineinit", 10);
    s_ready = 1;     // ready rock n roll
    return 0;
}

int	LsLuaEngine::isReady(lsi_session_t *session)
{
    if (!s_ready)
        return 0;
    return 1;
}

int LsLuaEngine::resumeNcheck( LsLuaSession* pSession )
{
    register int    ret;
    ret = s_luaApi->resumek(pSession->getLuaState(), 0, 0);
    switch(ret)
    {
    case 0:
        // NOTE - HTTP END already happened.. nothing for me to do here.
        return 0;
    case LUA_YIELD:
        // g_api->log(pSession->getHttpSession(), LSI_LOG_DEBUG, "RESUMEK YIELD %d\n", ret);
        ret = 0;
        break;
    case LUA_ERRRUN:
        g_api->log(pSession->getHttpSession(), LSI_LOG_NOTICE, "RESUMEK ERRRUN %d\n", ret);
        ret = -1;
        break;
    case LUA_ERRMEM:
        g_api->log(pSession->getHttpSession(), LSI_LOG_NOTICE, "RESUMEK ERRMEM %d\n", ret);
        ret = -1;
        break;
    case LUA_ERRERR:
        g_api->log(pSession->getHttpSession(), LSI_LOG_NOTICE, "RESUMEK ERRERR %d\n", ret);
        ret = -1;
        break;
    default:
        g_api->log(pSession->getHttpSession(), LSI_LOG_NOTICE, "RESUMEK ERROR %d\n", ret);
        ret = -1;
    }
    if (ret)
    {
        // send error informaton to error log
        LsLuaApi::dumpStack(pSession->getLuaState(), LUA_RESUME_ERROR, 10);
        // send error information back to http
        LsLuaApi::dumpStack2Http(pSession->getHttpSession(), pSession->getLuaState(), LUA_RESUME_ERROR, 10);
        g_api->append_resp_body(pSession->getHttpSession(), "\r\n", 2);
        g_api->end_resp(pSession->getHttpSession());
    }
    return ret;
}

//
//  runScript - working code.
//
int    LsLuaEngine::runScript(lsi_session_t *session, const char * scriptpath, LsLuaUserParam * pUser, LsLuaSession ** ppSession)
{
    g_api->log(session, LSI_LOG_DEBUG, "maxruntime %d maxlinecount %d\n"
                , pUser->getMaxRunTime() 
                , pUser->getMaxLineCount());
               
    char * buf;
    const char * xbuf;
    
    xbuf = scriptpath;
    int num = (strlen(xbuf) << 1) + 1000;
    if (!(buf = (char *)malloc(num)))
    {
        if (xbuf != scriptpath)
            free((void *)xbuf);
        return -1;
    }
    snprintf(buf, num,
                 " local __ls_f, __ls_err=loadfile(\"%s\") "
                 " if __ls_f ~= nil then "
                 "  ngx = ls "
                 "  __ls_f() "
                 "  ls._end(0) "
                 " else "
                 "  ls.say('Syntax Error[', __ls_err, ']') "
                 "  ls._end(-1) "
                 " end " 
                , xbuf
           );
    if (xbuf != scriptpath)
    {
        g_api->log(session, LSI_LOG_DEBUG, "RUN FROM CACHE %s", xbuf);
        free((void *)xbuf);
    }
    lua_State * pState = newLuaConnection();
    if (!pState)
        return -1;
    LsLuaSession * p_sess = new LsLuaSession;
    if (ppSession)
        *ppSession = p_sess;
    p_sess->init(session);
    p_sess->setupLuaEnv(pState, pUser);
    // LsLuaApi::dumpStack(p_sess->getLuaState(), "runScript after setup", 10);
    // NOTE may want to remove the ls TABLE from STACK

    int ret = LsLuaApi::compileLuaCmd(p_sess->getLuaState(), buf);
    free(buf);
    if (ret)
    {
        s_luaApi->close(p_sess->getLuaState());
        return -1;
    }
    
    return resumeNcheck( p_sess );
}

void LsLuaEngine::refX( LsLuaSession * pSession)
{
    int * r;
    
    pSession->setTop(LsLuaEngine::api()->gettop(getSystemState()));
    LsLuaEngine::api()->pushvalue(getSystemState(), -1);
    
// LsLuaApi::dumpStack(getSystemState(), "refX", 10);
    
    r = pSession->getRefPtr();
    *r = LsLuaEngine::api()->ref(getSystemState(), LsLuaEngine::api()->LS_LUA_REGISTRYINDEX) ;
    
//   LsLuaApi::dumpStack(getSystemState(), "refX", 10);
}

void LsLuaEngine::unrefX( LsLuaSession * pSession)
{
    int * r;
    
    r = pSession->getRefPtr();
    if ( (*r) == LUA_REFNIL )
        return; // do nothing if refX() never installed

    int from = LsLuaEngine::api()->gettop(getSystemState());
    if (from > pSession->getTop())
        from = pSession->getTop();
    while (from > 0)
    {
        lua_State * thread;
        if ( (thread = LsLuaEngine::api()->tothread(getSystemState(), from)) )
        {
            if (thread == pSession->getLuaState())
            {
                LsLuaEngine::api()->remove(getSystemState(), from);
                break;
            }
        }
        from--;
    }
    
    // r = pSession->getRefPtr();
    LsLuaEngine::api()->unref(getSystemState(), LsLuaEngine::api()->LS_LUA_REGISTRYINDEX, *r) ;
    *r = LUA_REFNIL;
    
//  LsLuaApi::dumpStack(getSystemState(), "unrefX", 10);
}

int LsLuaEngine::loadRefX( LsLuaSession * pSession, lua_State * L)
{
    int * r;
    lua_State * y;
    r = pSession->getRefPtr();
    if ( (*r) == LUA_REFNIL )
        return 0; // do nothing if refX() never installed
        
    LsLuaEngine::api()->rawgeti(getSystemState(), LsLuaEngine::api()->LS_LUA_REGISTRYINDEX, *r) ;

// LsLuaApi::dumpStack(getSystemState(), "loadRefX", 10);
    y = LsLuaEngine::api()->tothread ( getSystemState(), -1) ;
    if (y != L)
    {
        g_api->log(pSession->getHttpSession(), LSI_LOG_ERROR, "Session thread %p != %p\n", L, y);
        LsLuaApi::pop(getSystemState(), 1);
        return -1;
    }
    else
    {
        LsLuaApi::pop(getSystemState(), 1);
        return 0;
    }
    // LsLuaEngine::api()->xmove(getSystemState(), p_sess->getLuaState(), 2);
}



int	LsLuaEngine::runScriptX(lsi_session_t * session, const char * scriptpath, LsLuaUserParam * pUser, LsLuaSession ** ppSession)
{
    int ret;
    
    g_api->log(session, LSI_LOG_DEBUG, "maxruntime %d maxlinecount %d\n"
                , pUser->getMaxRunTime() 
                , pUser->getMaxLineCount());
               
    if ( (ret = LsLuaFuncMap::loadLuaScript(session, getSystemState(), scriptpath)) )
    {
        g_api->end_resp(session);
        return (0);
    }
    // I should have the function on top of stack from here
    LsLuaSession * p_sess = new LsLuaSession;
    *ppSession = p_sess;
    p_sess->init(session);
    p_sess->setupLuaEnvX(getSystemState(), pUser);
    // LsLuaEngine::api()->remove(p_sess->getLuaState(), -1);  // SESSION -> table

    LsLuaEngine::api()->pushvalue(getSystemState(), -2);           // func thread func
    // SEESION just pushed a thread to system stack 
    LsLuaEngine::api()->remove(getSystemState(), -3);              // thread func
    // NOTE - the func and thread are moved to thread stack
    // LsLuaEngine::api()->xmove(getSystemState(), p_sess->getLuaState(), 2);
    
    LsLuaEngine::api()->xmove(getSystemState(), p_sess->getLuaState(), 1); // thread in main and func in new
    LsLuaEngine::refX( p_sess );
    if (p_sess->getRef() == LUA_REFNIL)
    {
        g_api->append_resp_body(session, LUA_ERRSTR_ERROR, strlen(LUA_ERRSTR_ERROR) );
        g_api->end_resp(session);
        return (0);
    }
    
    // Create sandbox for user function
    LsLuaEngine::api()->pushvalue(p_sess->getLuaState(),  LsLuaEngine::api()->LS_LUA_GLOBALSINDEX);
    // NOTE- the current setfenv implementation will only only with JIT
    //      In the future, we may port this for 5.2 engine.
    if ((ret = LsLuaEngine::api()->setfenv(p_sess->getLuaState(), -2)) != 1)
    {
        g_api->log(session, LSI_LOG_ERROR, "%s %d\n", LUA_ERRSTR_SANDBOX, ret);
        g_api->append_resp_body(session, LUA_ERRSTR_SANDBOX, strlen(LUA_ERRSTR_SANDBOX) );
        g_api->end_resp(session);
        return (0);
    }
    if ( (ret = LsLuaEngine::api()->pcallk(p_sess->getLuaState(), 0, 1, 0, 0, NULL)) )
    {
        g_api->log(session, LSI_LOG_ERROR, "%s %d\n", LUA_ERRSTR_SCRIPT, ret);
        g_api->append_resp_body(session, LUA_ERRSTR_SCRIPT, strlen(LUA_ERRSTR_SCRIPT) );
        g_api->end_resp(session);
        return 0;
    }
    
    return resumeNcheck( p_sess );
}

lua_State * LsLuaEngine::newLuaConnection()
{
    return (api()->newstate()) ;
}

lua_State * LsLuaEngine::newLuaThread(lua_State * L)
{
    return (api()->newthread(L)) ;
}

lua_State * LsLuaEngine::injectLsiapi(lua_State * L)
{
    extern int          ls_lua_cppFuncSetup(lua_State *);
    register lua_State *pState;
    
    pState = L ? L : api()->newstate();
    if (pState)
    {
        api()->openlibs(pState);
        ls_lua_cppFuncSetup(pState);
    }
    return pState;
}

//
//  Configuration from LiteSpeed
//  simple scanf parse - 
//
void* LsLuaEngine::parseParam( const char* param, void* initial_config, int level, const char *name )
{
    char    key[0x1000];
    char    value[0x1000];
    int     n, nb;
    register char    * p;
    int     sec, line;
    char *  cp;
    
    LsLuaUserParam * pParent = (LsLuaUserParam * )initial_config;
    LsLuaUserParam * pUser = new LsLuaUserParam(level);
    
    g_api->log(NULL, LSI_LOG_DEBUG, "%s: LUA PARSEPARAM %d [%.20s]\n"
                    , name
                    , level
                    , param ? param : "");
    
    if ((!pUser) || (!pUser->isReady()))
    {
        g_api->log(NULL, LSI_LOG_ERROR, "LUA PARSEPARAM NO MEMORY");
        return NULL;
    }
    if (pParent)
    {
        *pUser = *pParent;
    }

    if (!param)
    {
        // david said that I should check param...
        s_firstTime = 0;
        return pUser;
    }
    p = (char *)param;
    g_api->log(NULL, LSI_LOG_DEBUG, "%s: LUA PARSEPARAM %d %s Parent %2d [%.20s] %d\n"
                    , name
                    , level
                    , (level == LSI_MODULE_DATA_HTTP) ? "HTTP" : ""
                    , pParent ? pParent->getLevel() : -1
                    , param
                    , pParent ? pParent->getMaxRunTime() : -1
                    );
    
    do
    {
        n = sscanf(p, "%s%s%n", key, value, &nb);
        if (n == 2)
        {
            // Parameters on the httpServerConfig level - the very firstTime
            if (s_firstTime)
            {
                if ( (!strcasecmp("luapath", key)) )
                {
                    if ( (cp = strdup(value)) )
                    {
                        if (s_luaPath)
                            free(s_luaPath);
                        s_luaPath = cp;
                    }
                    g_api->log(NULL, LSI_LOG_NOTICE
                            , "%s LUA SET %s = %s [%s]\n"
                            , name, key, value, s_luaPath ? s_luaPath : s_sysLuaPath);
                    p += nb;
                    continue;
                }
                else if (  (!strcasecmp("lib", key)) || (!strcasecmp("lua.so", key)) ) 
                {
                    if ( (cp = strdup(value)) )
                    {
                        if (s_lib)
                            free(s_lib);
                        s_lib = cp;
                    }
                    g_api->log(NULL, LSI_LOG_NOTICE
                            , "%s LUA SET %s = %s [%s]\n"
                            , name, key, value, s_lib ? s_lib : "NULL");
                    p += nb;
                    continue;
                }
            }
            
            // runtime parameters
            if (!strcasecmp("maxruntime", key))
            {
                // allow hex and decimal
                if ((sscanf(value, "%i", &sec) == 1) && (sec > 0))
                {
                    if (s_firstTime)
                        s_maxRunTime = sec;
                    pUser->setMaxRunTime(sec);
                }
                g_api->log(NULL, LSI_LOG_NOTICE
                            , "%s LUA SET %s = %s msec [%d %s]\n"
                            , name , key, value, pUser->getMaxRunTime()
                            , pUser->getMaxRunTime() ? "ENABLED" : "DISABLED");
            }
            else if (!strcasecmp("maxlinecount", key))
            {
                // allow hex and decimal
                if ((sscanf(value, "%i", &line) == 1) && (line >= 0))
                {
                    if (s_firstTime)
                        s_maxLineCount = line;
                    pUser->setMaxLineCount(line);
                }
                g_api->log(NULL, LSI_LOG_NOTICE
                            , "%s LUA SET %s = %s [%d %s]\n"
                            , name , key, value, pUser->getMaxLineCount()
                            , pUser->getMaxLineCount() ? "ENABLED" : "DISABLED");
            }
            
            // extemely differcult parameters for fine tuning
            else if (!strcasecmp("jitlinemod", key))
            {
                // allow hex and decimal
                if ((sscanf(value, "%i", &line) == 1) && (line > 0))
                    s_jitLineMod = line;
                g_api->log(NULL, LSI_LOG_NOTICE
                            , "%s LUA SET %s = %s [%d]\n"
                            , name, key, value, s_jitLineMod );
            }
            else if (!strcasecmp("pause", key))
            {
                // allow hex and decimal
                if ((sscanf(value, "%i", &sec) == 1) && (sec > 0))
                    s_pauseTime = sec;
                g_api->log(NULL, LSI_LOG_NOTICE
                            , "%s LUA SET %s = %s [%d]\n"
                            , name, key, value, s_pauseTime);
            }
#if 0
//
// DEBUG STUFF
//
            else if (!strcasecmp("debug", key))
            {
                // allow hex and decimal
                if ((sscanf(value, "%i", &s_debug) == 1) && (s_debug > 0))
                    ;
                else
                    s_debug = 0;
                g_api->log(NULL, LSI_LOG_NOTICE,
                             "SET %s = %s [%d]\n", key, value, s_debug);;
            }
#endif
            else
            {
                // ignore this pair values
                g_api->log(NULL, LSI_LOG_NOTICE,
                            "%s IGNORE MODULE PARAMETERS [%s] [%s]\n", name, key, value);
            }
        }
        p += nb;
    } while (n == 2);
    
    s_firstTime = 0;
    return (void *)pUser;
}

void LsLuaEngine::removeParam( void* config )
{
    g_api->log(NULL, LSI_LOG_DEBUG,
                            "REMOVE PARAMETERS [%p]\n", config);
    if (s_lib)
    {
        free(s_lib);
        s_lib = NULL;
    }
}

//
// invoke LUA command under coroutine
//
int LsLuaEngine::execLuaCmd(const char * cmd)
{
    register lua_State * p_newL;

    if (!(p_newL = s_luaApi->newthread(s_stateSystem)))
    {
        return -1;
    }
    // my current session!

    if (LsLuaApi::compileLuaCmd(p_newL, cmd))
    {
        s_luaApi->close(p_newL);
        return -1;
    }
    s_luaApi->resumek(p_newL, 0, 0);
    return 0;
}

//
//  Test driver
//
int LsLuaEngine::testCmd()
{
    static int  loopCount = 0;

    ++loopCount;
    switch ( loopCount )
    {
    case 1: // Testing dual socket
        execLuaCmd(
            "print('ls=', ls) "
            "local sock,err =ls.socket.tcp() print('sock.tcp=', sock) "
            "local code,err = sock:connect('61.135.169.125', 80) "
            "if code == 1 then "
            "  print ('FIRST  sock.connect = ', sock) "
            "    local ysock, err = ls.socket.tcp() print('sock.tcp=', ysock) "
            "    local code,err = ysock:connect('61.135.169.125', 80)"
            "    if code == 1 then "
            "      print ('SECOND sock.connect = ', ysock) "
            "      sock:send('GET /index.php HTTP/1.0\\r\\n\\r\\n') "
            "      ysock:send('GET /index.php HTTP/1.0\\r\\n\\r\\n') "
            "      y = '' "
            "      while y ~= nil do "
            "        y, err = ysock:receive() ls.puts(y) "
            "      end "
            "      code,err = ysock:close() print('close ', code, str) "
            "    else "
            "      ls.puts('SECOND CONNECTION FAILED') "
            "      ls.puts(err) "
            "    end "
            "  y = '' "
            "  while y ~= nil do "
            "    y = sock:receive() ls.puts(y) "
            "  end "
            "  ls.puts('BYE LiteSpeed') "
            "  sock:close() "
            "  code,err = sock:close() print('close ', code, str) "
            "else "
            "  ls.puts('BYE CONNECTION FAILED') "
            "  ls.puts(err) "
            "end "
            "print('collectgarbage=', collectgarbage('count')*1024) "
            "ls.exit(0)"
        );
        break;
    case 4:
        execLuaCmd(
            "print('ls=', ls) "
            "function sockproc() "
            "  local sock "
            "    sock = ls.socket.tcp() print('sock.tcp=', sock) "
            "  local code, err "
            "    code, err = sock:connect('61.135.169.125', 80) "
            "  if code == 1 then "
            "    sock:send('GET /index.php HTTP/1.0\\r\\n\\r\\n') "
            "    y = '' "
            "    while y ~= nil do "
            "        y = sock:receive() ls.puts(y) "
            "    end "
            "  code,err = sock:close() print('close ', code, str) "
            "  ls.puts('BYE LiteSpeed') "
            "  else "
            "     ls.puts('ERROR CONNECTION FAILED') "
            "     ls.puts(err) "
            "  end "
            "end "
            "sockproc()"
            "print('collectgarbage=', collectgarbage('count')*1024)"
            " collectgarbage() "
            "print('collectgarbage=', collectgarbage('count')*1024)"
            "ls.exit(0) "
        );
        break;
    default:
        break;
    }
    return 0;
}

LsLuaFuncMap * LsLuaFuncMap::s_map = {NULL};
int LsLuaFuncMap::s_num = 0;

//
//  loadScript the system stack
//
int LsLuaFuncMap::loadLuaScript( lsi_session_t *session, lua_State * L, const char* scriptName )
{
    register LsLuaFuncMap * p;
    
    for (p = s_map; p; p = p->m_next)
    {
        if (!strcmp(scriptName, p->scriptNname()))
        {
            struct stat scriptStat;
            if (!stat(scriptName, &scriptStat))
            {
                // same time -> same file --- a resonable assumption
                if ((scriptStat.st_mtime == p->m_stat.st_mtime)
                        && (scriptStat.st_ino == p->m_stat.st_ino)
                        && (scriptStat.st_size == p->m_stat.st_size)
                   )
                    
                {
                    p->loadLua(L);
                    return (0);
                }
                //
                // oh! script file is newer - need to recompile
                //
                p->unloadLua(L);    // unload the LUA table
                p->remove();        // remove myself from base
                delete p;           // killed loaded entry
            
                // NOTE never/ever continue the loop.
                return loadLuaScript(session, L, scriptName);
            }
            // Can't stat the new file... uhmmm use the cache
            p->loadLua(L);
            return(0);
        }
    }
    p = new LsLuaFuncMap(session, L, scriptName);
    if (p->isReady())
    {
        g_api->log(session, LSI_LOG_NOTICE,
                     "LUA LOAD FROM SRC SAVED TO CACHE %s\n", scriptName);
        return 0;
    }
    else
    {
        // Failed to load script
        int ret = p->m_status ;
        g_api->log(session, LSI_LOG_NOTICE,
                     "LUA FAILED TO LOAD %s %d\n", scriptName, ret);
        delete p;
        return ret;
    }
}

//
//  local Datablock for file loading... dont put in header
//
#define F_PAGESIZE    0x2000    
typedef struct {
    FILE *  fp;
    char    buf[F_PAGESIZE];    // 8k page!
    size_t  size;
    int     state;          // 0 - not ready, 1 - begin, 2 - data, 3 - end
} luaFile_t;

//
// @LsLuaFuncMap - container which use to manage all the loaded LUA script
// @status = 1 - script loaded and push onto the top of stack
// @status = 0 - not ready... abort 
// @statue = -1 - Failed to access script
// @statue = -2 - LUA script SYNTAX error
// @statue = -3 - LUA ERROR
//
LsLuaFuncMap::LsLuaFuncMap(lsi_session_t *session, lua_State * L, const char * scriptName)
{
    if (s_num == 0) {
        // create global table in the very first time
        LsLuaEngine::api()->createtable(L, 0, 0);
        LsLuaEngine::api()->setglobal(L, LS_LUA_FUNCTABLE);
        // LsLuaApi::pop(L, 1);                            // func
    }
    s_num++;
    m_L = L;
    m_scriptName = strdup(scriptName);
    char funcName[0x100];
    snprintf(funcName, 0x100, "x%07d", s_num);
    m_funcName = strdup(funcName);
    m_status = 0; // set to noready... may be useless...
    
    int ret;
    int top;
    top = LsLuaEngine::api()->gettop(L);
    // Ready for luaReader
    luaFile_t   loadData;
    // too many duplicated code... use goto onError
    if (!(loadData.fp = fopen(m_scriptName, "r")))
    {
        m_status = -1;
        goto errout;
    }
    loadData.size = sizeof(loadData.buf);
    loadData.state = 1; // ready to load
    // save file time
    stat(m_scriptName, &m_stat);
    // ret = LsLuaEngine::api()->loadfilex(L, m_scriptName, NULL)
    ret = LsLuaEngine::api()->load(L, textFileReader, (void *)&loadData, m_scriptName);
    fclose(loadData.fp);
    if (ret)
    {
        size_t          len;
        const char *    cp;
        cp = LsLuaEngine::api()->tolstring(L, top+1, &len);
        if (cp && len) g_api->append_resp_body(session, cp, len);
        if (ret == LUA_ERRSYNTAX) m_status = -2;
        else m_status = -3;
        goto errout;
    }
    if (LsLuaEngine::api()->type(L, -1) != LUA_TFUNCTION) goto errout;
  
    // into the global func table
    LsLuaEngine::api()->getglobal(L, LS_LUA_FUNCTABLE); // func __funcTable
    LsLuaEngine::api()->pushstring(L, m_funcName);  // func __funcTable x0001
    LsLuaEngine::api()->pushvalue(L, -3);           // func __funcTable x0001 func
    LsLuaEngine::api()->settable(L, -3);            // func __funcTable
    LsLuaApi::pop(L, 1);                            // func
    // LsLuaApi::dumpStack(L, "after settable", 10);
    add();
    // LsLuaEngine::api()->settop(L, top+1);     // not necessary
    m_status = 1;
    return; // done with good statue
    // too many duplicated code... use goto instead
errout:
    LsLuaApi::dumpStack(L, "ERROR: LOADSCRIPT FAILED", 10);
    LsLuaEngine::api()->settop(L, top);     // reset stack
    g_api->append_resp_body(session, LUA_ERRSTR_SCRIPT, strlen(LUA_ERRSTR_SCRIPT));
}

LsLuaFuncMap::~LsLuaFuncMap()
{
    if (m_scriptName)
        free(m_scriptName);
    if (m_funcName)
        free(m_funcName);
    m_status = 0; // mark no longer usable
}

void LsLuaFuncMap::loadLua(lua_State *L)
{
    LsLuaEngine::api()->getglobal(L, LS_LUA_FUNCTABLE);       // __funcTable
    LsLuaEngine::api()->getfield(L, -1, m_funcName);  // __funcTable func
    LsLuaEngine::api()->remove(L, -2);                // func
}

void LsLuaFuncMap::unloadLua(lua_State *L)
{
    LsLuaEngine::api()->getglobal(L, LS_LUA_FUNCTABLE);     // __funcTable
    LsLuaEngine::api()->pushnil(L);                         // __funcTable nil
    LsLuaEngine::api()->setfield(L, -2, m_funcName);        // __funcTable
    LsLuaEngine::api()->remove(L, -1);                      //
}

void LsLuaFuncMap::add()
{
    m_next = s_map;
    s_map = this;
}

void LsLuaFuncMap::remove()
{
    register LsLuaFuncMap * p;
    
    if (this == s_map)
    {
        s_map = m_next;
        return;
    }
    
    for (p = s_map; p->m_next; p = p->m_next)
    {
        if (p->m_next == this)
        {
            p->m_next = m_next;
            return;
        }
    }
}

const char* LsLuaFuncMap::textFileReader( lua_State*, void* d, size_t* retSize )
{
    luaFile_t * p_d = (luaFile_t *)d;
    switch (p_d->state)
    {
    case 1:
        *retSize = strlen(LS_LUA_BEGINSTR);
        memcpy( p_d->buf, LS_LUA_BEGINSTR, *retSize );
        p_d->state = 2;
        break;
    case 2:
        int num;
        num = fread(p_d->buf, 1, p_d->size, p_d->fp);
        if (num > 0)
            *retSize = num;
        else
        {
            *retSize = strlen(LS_LUA_ENDSTR);
            memcpy(p_d->buf, LS_LUA_ENDSTR, *retSize);
            p_d->state = 0; // done!
        }
        break;
    default:
        *retSize = 0;
    }
    return p_d->buf; // always return buf... retSize should be zero for bad buffer
}

