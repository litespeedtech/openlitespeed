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

#ifndef LSLUAAPI_H
#define LSLUAAPI_H

#include <modules/lua/lsluaengine.h>
#include <ls.h>
#include <string.h>

enum LSLUA_BAD
{
    LSLUA_NO_MEMORY         = 0,
    LSLUA_BAD_LUA           = 1,
    LSLUA_BAD_USER          = 2,
    LSLUA_BAD_PARAMETERS    = 3,
    LSLUA_BAD_SOCKET        = 4,
    LSLUA_BAD_REQ           = 5,
    LSLUA_BAD               = 6,
    LSLUA_BAD_XXX           = 7     // making sure in power of 2
};



#define LSLUA_PRINT_MAX_FRAG_SIZE 256

#define LSLUA_PRINT_FLAG_DEBUG      1
#define LSLUA_PRINT_FLAG_VALUE_ONLY 2
#define LSLUA_PRINT_FLAG_KEY_ONLY   4
#define LSLUA_PRINT_FLAG_ADDITION   8
#define LSLUA_PRINT_FLAG_CR         16
#define LSLUA_PRINT_FLAG_LF         32

class LsLuaSession;
#ifdef __cplusplus
extern "C" {
#endif

typedef struct lslua_static_buf_t
{
    char * _pBegin;
    char * _pEnd;
    char * _pCur;
    
} lslua_static_buf_t ;

typedef int (*print_flush)( void * output_stream, const char * pBuf, int len, int *flag );

typedef struct lslua_print_stream_t
{
    void *              _param;
    print_flush         _fp_flush;
    int                 _flag;
    lslua_static_buf_t  _buffer;
} lslua_print_stream_t;

int LsLua_generic_print( lua_State * L, lslua_print_stream_t * pStream );

void LsLua_log( lua_State * pState, int level, int no_linefeed, const char * fmt, ...);

void LsLua_lograw_buf( const char *pStr, int len );

static inline void LsLua_lograw_string( const char *pStr )
{    LsLua_lograw_buf( pStr, strlen( pStr ));    }

int  LsLua_log_ex( lua_State * L, int level );

LsLuaSession *  LsLua_getSession(lua_State * L);
int LsLua_setSession(lua_State *L, LsLuaSession * pSession);


#ifdef __cplusplus
}
#endif

//
//  Use this for all LiteSpeed LUA API
//
//      hopefully most of the inline function should be here
//      LUA system interface class
//
class LsLuaApi
{
public:
    LsLuaApi();
    ~LsLuaApi();
    
    // inline interface
    inline static int  return_badparameter(lua_State * L, enum LSLUA_BAD key)
    {
        LsLuaEngine::api()->pushinteger(L, -1);
        LsLuaEngine::api()->pushstring(L, m_badmsg[key&0x7]);
        return 2;
    }
    inline static int  return_badparameterx(lua_State * L, enum LSLUA_BAD key)
    {
        LsLuaEngine::api()->pushnil(L);
        LsLuaEngine::api()->pushstring(L, m_badmsg[key&0x7]);
        return 2;
    }


    inline static int  tonumberByIdx(lua_State * L, int idx)
    {
        return LsLuaEngine::api()->tonumberx(L, idx, NULL) ;
    }

    inline static void pop(lua_State * L, int n) { LsLuaEngine::api()->settop(L, -(n)-1); }

    // inline 
    inline static int compileLuaCmd(lua_State * L, const char * cmd)
    { return LsLuaEngine::api()->loadstring(L, cmd); }

    inline static int   execLuaCmd(lua_State * L, const char * cmd)
    {
        int ret = compileLuaCmd(L, cmd);
        if (ret)
            return ret;
        return(LsLuaEngine::api()->pcallk(L, 0, LUA_MULTRET, 0, 0, NULL));
#if 0        
        if (compileExecLuaCmd(L, cmd) || LsLuaEngine::api()->pcallk(L, 0, LUA_MULTRET, 0, 0, NULL))
            return -1 ;
        else
            return 0 ;
#endif        
    }

    inline static void pushTableItem(lua_State * L, const char * tableName, const char * itemName, int remove = 1)
    {
        if (tableName) LsLuaEngine::api()->getglobal(L, tableName);  // tableName
        LsLuaEngine::api()->pushstring(L, itemName);                 // table itemName
        LsLuaEngine::api()->gettable(L, -2);                         // table item
        if (remove) LsLuaEngine::api()->remove(L, -2);               // item note-if tableName was pushed before will be removed.
    }
    
    inline static void createTable(lua_State * L, const char * tableName, const char * itemName)
    {
        if (tableName) LsLuaEngine::api()->getglobal(L, tableName); // tableName
        LsLuaEngine::api()->pushstring(L, itemName);                // table itemName
        LsLuaEngine::api()->createtable(L, 0, 0);                   // table itemName newtable
        LsLuaEngine::api()->settable(L, -3);                        // table
        if (tableName) LsLuaApi::pop(L, 1);                         //
    }
    
    static int myputstring(const char *);

    //
    //  dummy - future ...
    // 
    static int checkLua(lua_State * L);
    
    //
    //  Helpfull debug stuff
    //
    inline static void dumpStackCount(lua_State * L, const char * tag)
    {
        LsLua_log(L, LSI_LOG_INFO, 0, "%s STACK[%d]", tag, LsLuaEngine::api()->gettop(L));
    }
    
    // format debug message for stack[idx] to buf[bufLen]
    static int dumpStackIdx2Buf(lua_State * L, int idx, char * buf, int bufLen);
    
    // send information to error log
    static void dumpStack(lua_State * L, const char * tag, int num);
    // static void dumpStackIdx(lua_State * L, int idx);
    
    // send information to calling http
    static void dumpStack2Http(lsi_session_t * session, lua_State * L, const char * tag, int num);
    // static void dumpStackIdx2Http(lsi_session_t * session, lua_State * L, int idx);

    // execute LUA string as command
    static int doString(lua_State * L, const char * str);
    
    

private:
    LsLuaApi( const LsLuaApi& other );
    LsLuaApi& operator=( const LsLuaApi& other );
    bool operator==( const LsLuaApi& other );

    static const char  *     m_badmsg[];
    static unsigned short    m_bcount;
};

#endif // LSLUAAPI_H
