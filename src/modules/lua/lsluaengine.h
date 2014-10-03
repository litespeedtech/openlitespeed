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

#ifndef LSLUAENGINE_H
#define LSLUAENGINE_H

class LsLuaFuncMap;
class LsLuaScript;
class LsLuaState;
class LsLuaSession;

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ls.h>
#include <modules/lua/ls_lua.h>
#include <socket/gsockaddr.h>

class LsLuaFunc;
class LsLuaUserParam;

class LsLuaEngine
{
public:
    enum TYPE
    {
        LSLUA_ENGINE_REGULAR = 0,
        LSLUA_ENGINE_JIT = 1,
    };
    LsLuaEngine();
    ~LsLuaEngine();
    
    static void set_version(const char * buf, size_t len)
    {
        snprintf(s_version, sizeof(s_version)-1, "%s %.*s", s_luaName, (int)len, buf);
    }
    static const char * version()
    { return s_version; }

    static inline struct ls_lua_api_t * api()
    {   return s_luaApi;        }

    // only three functions will be used to by handler
    // static int init( TYPE type, const char * pDynLibPath );
    static int init();
    static int isReady(lsi_session_t *session);                      // setup Session and get ready
    // 5.2 LUA
    static int runScript(lsi_session_t *session, const char * scriptpath, LsLuaUserParam *pUser, LsLuaSession ** pSession); // setup Session and run
    // 5.1 JIT LUA
    static int runScriptX(lsi_session_t *session, const char * scriptpath, LsLuaUserParam *pUser, LsLuaSession ** pSession); // setup Session and run
    static void refX( LsLuaSession * );
    static void unrefX( LsLuaSession * );
    static int  loadRefX( LsLuaSession *, lua_State *);

    // call by standalone testing
    static int testCmd();

    // inject api
    static lua_State * injectLsiapi(lua_State *);   // return the newly created lua_State
private:
    static lua_State * getSystemState()
    {   return s_stateSystem; }
    
    // invoke resume coroutine and check status
    static int resumeNcheck(LsLuaSession * pSession);
public:
    
    // module parameter setup
    static void * parseParam ( const char *param, int param_len, void *initial_config, int level, const char *name );
    static void removeParam( void * config);
    
    static int getMaxRunTime()
    {   return s_maxRunTime; }
    
    static int getMaxLineCount()
    {   return s_maxLineCount; }
    
    static int getPauseTime()
    {   return s_pauseTime; }
    
    static int getJitLineMod()
    {   return s_jitLineMod; }
    
    static const char * getLuaName()
    {   return s_luaName; }
    
    static int debugLevel()
    {   return s_debugLevel; }
    
    static void setDebugLevel( int level )
    {   s_debugLevel = level; }
    
    static int debug()
    { return s_debug ; }
    
private:
    static int execLuaCmd(const char * cmd);    // invoke LUA inside the coroutine
    static lua_State * newLuaConnection();
    static lua_State * newLuaThread(lua_State *);
    
private:
    LsLuaEngine( const LsLuaEngine& other );
    LsLuaEngine& operator=( const LsLuaEngine& other );
    bool operator==( const LsLuaEngine& other );
private:
    static const char * s_syslib; // the module library /usr/lib/libluajit.so
    static const char * s_sysLuaPath;  // system default LUA_PATH
    static ls_lua_t *   s_luaSys;
    static ls_lua_api_t * s_luaApi;
    static lua_State * s_stateSystem;
    static TYPE s_type;
    static int          s_ready;    // init is good and ready to load scripts
    //
    //  module parameters
    //
    static char *       s_lib;          // the module library /usr/lib/libluajit.so
    static char       * s_luaPath;      // user defined LUA_PATH
    static int          s_debug;        // note: this is internal debug for LUA
    static int          s_maxRunTime;   // max allowable runtime in msec
    static int          s_maxLineCount; // time to throddle
    static int          s_pauseTime;    // time to pause in msec
    static int          s_jitLineMod;   // modifier for JIT
    static int          s_firstTime;    // detect very first time parameter parsing
    static int          s_debugLevel;   // current web server debug level
    static char         s_luaName[0x10];// 15 bytes to save the name
    static char         s_version[0x20];// 31 bytes for LUA information
};

//
//  @Container for all loaded LUA functions
//  @loadLuaScript - load LUA script file into the container, return status (see below)
//                 
//  @ scriptName - return the name of the script
//  @ funcName - LiteSpeed internal name of the script
//  @ statue - 1 good. 0 not ready, -1 syntax error, -2 LUA error
//
class LsLuaFuncMap
{
public:
    static int  loadLuaScript(lsi_session_t *session, lua_State * L, const char * scriptName);
    
    const char * scriptNname() const
    { return m_scriptName; }
    const char * funcName() const
    { return m_funcName; }
    
    int isReady() const
    { return m_status == 1 ? 1 : 0; }
    
    int status() const
    { return m_status; }
    
private:
   LsLuaFuncMap(lsi_session_t *session, lua_State *L, const char  * scriptName);
   LsLuaFuncMap();
   ~LsLuaFuncMap();
   void add();
   void remove();
   void loadLua(lua_State *);
   void unloadLua(lua_State *); // remove from LUA_TABLE
   
   static const char * textFileReader (lua_State *, void * d, size_t * retSize);
private:
    LsLuaFuncMap( const LsLuaFuncMap& other );
    LsLuaFuncMap& operator=( const LsLuaFuncMap& other );
    bool operator==( const LsLuaFuncMap& other );
    
private:
    lua_State *         m_L;            // lua State
    char *              m_scriptName;   // lua
    char *              m_funcName;     //
    int                 m_status;       // 1 - ready, 0 - not ready, -1 - syntax error, -2 - lua error
    LsLuaFuncMap *      m_next;
    struct stat         m_stat;
    
    static LsLuaFuncMap * s_map;
    static int          s_num;
};

//
//  @brief LsLusUserParam
//
class LsLuaUserParam
{
    int         m_maxRunTime;   // max allowable runtime in msec
    int         m_maxLineCount; // time to throddle
    int         m_level;
    int         m_ready;
public:
    LsLuaUserParam(int level)
            : m_maxRunTime(LsLuaEngine::getMaxRunTime())
            , m_maxLineCount(LsLuaEngine::getMaxLineCount())
            , m_level(level)
            , m_ready(1)
    { }
    
    ~LsLuaUserParam()
    {
    };
    
    int isReady() const
    {   return m_ready; }
    
    void setReady(int flag)
    {   m_ready = flag; }
    
    void setLevel(int level)
    {   m_level = level; }
    
    int getLevel() const
    {   return m_level; }
    
    int getMaxRunTime() const
    {   return m_maxRunTime; }
    
    int getMaxLineCount() const
    {   return m_maxLineCount; }
    
    void setMaxRunTime(int maxTime)
    {   m_maxRunTime = maxTime; }
    
    void setMaxLineCount(int maxLine)
    {   m_maxLineCount = maxLine; }
    
    LsLuaUserParam& operator= ( const LsLuaUserParam& other )
    {
        m_maxRunTime = other.m_maxRunTime;
        m_maxLineCount = other.m_maxLineCount;
        m_ready = other.m_ready;
        return *this;
    }
private:
    LsLuaUserParam( const LsLuaUserParam& other );
    bool operator==( const LsLuaUserParam& other );
};

#endif // LSLUAENGINE_H
