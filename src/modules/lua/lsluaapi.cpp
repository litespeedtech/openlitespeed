/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <modules/lua/lsluasession.h>
#include <modules/lua/edluastream.h>
#include <modules/lua/lsluaengine.h>
#include <log4cxx/logger.h>
#include <log4cxx/layout.h>

/* Generic LUA headers
*/
#include <math.h>
#include "ls_lua.h"
#include "lsluaapi.h"

void LsLua_create_shared(lua_State *);
void LsLua_create_shared_meta(lua_State *);


//
//  Static LsLuaApi
//
const char * LsLuaApi::m_badmsg[] = {
    "BAD NO MEMORY",
    "BAD LUA CALL",
    "BAD LUA USER",
    "BAD PARAMETER(s)",
    "BAD SOCKET",
    "BAD REQ",
    "BAD LUA",
    "BAD LUA"   // future use
};
unsigned short  LsLuaApi::m_bcount = 0;


//
//  Logging stuff
//
static LOG4CXX_NS::Logger * s_pLogger       = NULL;
static char s_logPattern[40]    = "%d [%p] [LUA] %m";

static LOG4CXX_NS::Logger * getLogger()
{
    if ( s_pLogger )
        return s_pLogger;
    s_pLogger = LOG4CXX_NS::Logger::getLogger( "LUA" );
    LOG4CXX_NS::Layout* layout = LOG4CXX_NS::Layout::getLayout( "lua_log_pattern", "layout.pattern" );
    layout->setUData( s_logPattern );
    s_pLogger->setLayout( layout );
    s_pLogger->setLevel( LSI_LOG_INFO ); // NOTICE - use member value instead!
    s_pLogger->setParent( LOG4CXX_NS::Logger::getRootLogger() );
    return s_pLogger;
}

void LsLua_log( lua_State * pState, int level, int no_linefeed, const char * fmt, ...)
{
    if ( level >= LsLuaEngine::debugLevel() )
    {
        char achNewFmt[1024];
        va_list arglist;
        va_start(arglist, fmt);
        snprintf( achNewFmt, 1023, "[%p] %s", pState, fmt );
        getLogger()->vlog(level, achNewFmt, arglist, no_linefeed);
        va_end( arglist );
    }
}


void LsLua_lograw_buf( const char *pStr, int len )
{
    getLogger()->lograw( pStr, len );
}

static int	LsLua_print(lua_State * L)
{
    LsLua_log_ex( L, LSI_LOG_INFO );
    return 0;
}

static int	LsLua_dummy(lua_State * L)
{
    LsLua_log( L, LSI_LOG_INFO, 0, "IN DUMMY");
    return 0;
}

int LsLuaApi::doString(lua_State * L, const char * str)
{
    return LsLuaEngine::api()->loadstring(L, str) || 
                LsLuaEngine::api()->pcallk(L, 0, LUA_MULTRET, 0, 0, NULL);
}

/*
*	Internal LiteSpeedTech "C" function
*/
extern void LsLua_create_header(lua_State *);
extern void LsLua_create_header_meta(lua_State *);

int	ls_lua_cppFuncSetup(lua_State * L)
{
    /*
    *	(1) overwritten the print
    *	(2)	setup LiteSpeed space
    *	(3)	load LiteSpeed functions
    */
    LsLuaEngine::api()->pushcclosure(L, LsLua_print, 0);
    LsLuaEngine::api()->setglobal(L, "print");
    LsLuaEngine::api()->pushcclosure(L, LsLua_dummy, 0);
    LsLuaEngine::api()->setglobal(L, "dummy");

    LsLua_create_session(L);
    LsLua_create_tcp_sock_meta(L);
    LsLua_create_req_meta(L);
    LsLua_create_resp_meta(L);

    /* put in header */
    LsLua_create_header( L );
    LsLua_create_header_meta( L );
    
    /* put in shared */
    LsLua_create_shared_meta(L);
    LsLua_create_shared(L);
    
    LsLua_create_session_meta( L );

    // LsLuaApi::dumpStack(L, "DONE cppFuncSetup", 10);
    return 0;
}

//
//  My c overload put string
//
int	LsLuaApi::myputstring(const char * str)
{
    LsLua_lograw_string(str);
    return 1;
}

//
//  Check all the LUA state and try to return the dead resource
//
int LsLuaApi::checkLua(lua_State * L)
{
    // ls_lua_checkuser();
    // LsLuaApi::dumpStack(L, "DONE checkLua", 10);
    return 0;
}



static int LsLua_print_one( lua_State * L, int idx, lslua_print_stream_t * pStream ); 


static int LsLua_print_table( lua_State * L, lslua_print_stream_t * pStream )
{
    if ( pStream->_flag & LSLUA_PRINT_FLAG_DEBUG )
        *(pStream->_buffer._pCur++) = '{';
    /* Push another reference to the table on top of the stack (so we know
    *  where it is, and this function can work for negative, positive and
    *  pseudo indices
    */
    LsLuaEngine::api()->pushvalue(L, -1);
    /* stack now contains: -1 => table */
    LsLuaEngine::api()->pushnil(L);
    /* stack now contains: -1 => nil; -2 => table */
    while (LsLuaEngine::api()->next(L, -2))
    {
        /* stack now contains: -1 => value; -2 => key; -3 => table */
        /* copy the key so that lua_tostring does not modify the original */
        LsLuaEngine::api()->pushvalue(L, -2);

        if ( !(pStream->_flag & LSLUA_PRINT_FLAG_VALUE_ONLY) )
        {
            LsLua_print_one(L, -1,  pStream );
            *(pStream->_buffer._pCur++) = '=';
        }
        if ( !(pStream->_flag & LSLUA_PRINT_FLAG_KEY_ONLY) )
            LsLua_print_one(L, -2, pStream);
        *(pStream->_buffer._pCur++) = ' ';

        /* pop value + copy of key, leaving original key */
        LsLuaApi::pop(L, 2 );
        //LsLuaEngine::api()->settop(L, -1);
        /* stack now contains: -1 => key; -2 => table */
    }
    /* stack now contains: -1 => table (when lua_next returns 0 it pops the key */
    /* but does not push anything.) */
    /* Pop table */
    LsLuaApi::pop(L, 1 );
    /* Stack is now the same as it was on entry to this function */
    if ( pStream->_flag & LSLUA_PRINT_FLAG_DEBUG )
        *(pStream->_buffer._pCur++) = '{';
    return 0;
}

static int LsLua_print_one( lua_State * L, int idx, lslua_print_stream_t * pStream )
{
    void *          p_udata;
    size_t          size;
    register int    t;
    const char *    p_string;
    lslua_static_buf_t * pBuf = &pStream->_buffer;

    if ( pBuf->_pEnd - pBuf->_pCur <= LSLUA_PRINT_MAX_FRAG_SIZE )
    {
        if ( pStream->_fp_flush( pStream->_param, pBuf->_pBegin, pBuf->_pCur - pBuf->_pBegin, &pStream->_flag ) == -1 )
            return -1;
        pBuf->_pCur = pBuf->_pBegin;
    }
    
    t = LsLuaEngine::api()->type(L, idx);
    switch(t)
    {
    case LUA_TNONE:
        *pBuf->_pCur++ = 'n';
        *pBuf->_pCur++ = 'o';
        *pBuf->_pCur++ = 'n';
        *pBuf->_pCur++ = 'e';
        break;
    case LUA_TNIL:
        *pBuf->_pCur++ = 'n';
        *pBuf->_pCur++ = 'i';
        *pBuf->_pCur++ = 'l';
        break;
    case LUA_TSTRING:
        p_string = LsLuaEngine::api()->tolstring(L, idx, &size);
        if ( size < LSLUA_PRINT_MAX_FRAG_SIZE )
        {
            memcpy( pBuf->_pCur, p_string, size );
            pBuf->_pCur += size; 
        }
        else
        {
            if ( pBuf->_pCur - pBuf->_pBegin > 0 )
            {
                if ( pStream->_fp_flush( pStream->_param, pBuf->_pBegin, pBuf->_pCur - pBuf->_pBegin, &pStream->_flag ) == -1 )
                    return -1;
                pBuf->_pCur = pBuf->_pBegin;
            }
            if ( pStream->_fp_flush( pStream->_param, p_string, size, &pStream->_flag ) == -1 )
                return -1;
            
        }
        break;
    case LUA_TBOOLEAN:
        if ( LsLuaEngine::api()->toboolean(L, idx) )
        {
            *pBuf->_pCur++ = 't';
            *pBuf->_pCur++ = 'r';
            *pBuf->_pCur++ = 'u';
            *pBuf->_pCur++ = 'e';
        }
        else
        {
            *pBuf->_pCur++ = 'f';
            *pBuf->_pCur++ = 'a';
            *pBuf->_pCur++ = 'l';
            *pBuf->_pCur++ = 's';
            *pBuf->_pCur++ = 'e';
        }
        break;
    case LUA_TNUMBER:
        pBuf->_pCur += snprintf( pBuf->_pCur, pBuf->_pEnd - pBuf->_pCur, 
                                 "%g", LsLuaEngine::api()->tonumberx(L, idx, NULL));
        break;
    case LUA_TTABLE:
        LsLuaEngine::api()->pushvalue(L, idx);
        LsLua_print_table(L, pStream);
        LsLuaApi::pop(L, 1 );

        break;
    case LUA_TFUNCTION:
        *pBuf->_pCur++ = '(';
        *pBuf->_pCur++ = ')';
        break;
    case LUA_TUSERDATA:
        /* ls_lua_printf("TUSERDATA"); */
        p_udata = LsLuaEngine::api()->touserdata(L, idx);
        pBuf->_pCur += snprintf( pBuf->_pCur, pBuf->_pEnd - pBuf->_pCur, 
                                 "<0x%X>", (int)(long)p_udata);
        break;
    case LUA_TLIGHTUSERDATA:
        /* ls_lua_printf("TUSERDATA"); */
        p_udata = LsLuaEngine::api()->touserdata(L, idx);
        pBuf->_pCur += snprintf( pBuf->_pCur, pBuf->_pEnd - pBuf->_pCur, 
                                 "[0x%X]", (int)(long)p_udata);
        break;
    case LUA_TTHREAD:
        *pBuf->_pCur++ = 'T';
        *pBuf->_pCur++ = 'T';
        *pBuf->_pCur++ = 'H';
        *pBuf->_pCur++ = 'R';
        *pBuf->_pCur++ = 'E';
        *pBuf->_pCur++ = 'A';
        *pBuf->_pCur++ = 'D';
        break;
    default:
        pBuf->_pCur += snprintf( pBuf->_pCur, pBuf->_pEnd - pBuf->_pCur, "TYPE[%d]", t);
        break;
    }
    /* ls_lua_printf(" "); */
    return t;
}

/*
*   overload all the print LUA
*/
int LsLua_generic_print( lua_State * L, lslua_print_stream_t * pStream )
{
    char            buf[0x1000];
    int             nArgs;
    register int    i;
    
    pStream->_buffer._pBegin = buf;
    pStream->_buffer._pCur = buf;
    pStream->_buffer._pEnd = &buf[ sizeof(buf) ];
    
    nArgs = LsLuaEngine::api()->gettop(L);
    for (i = 1; i <= nArgs; i++)
    {
        
        if ( LsLua_print_one(L, i, pStream ) == -1 )
            return -1;
        if (i < nArgs)
        {
            * pStream->_buffer._pCur++ = ' ';
        }
    }
    if ( pStream->_flag & LSLUA_PRINT_FLAG_CR )
        * pStream->_buffer._pCur = '\r';
    if ( pStream->_flag & LSLUA_PRINT_FLAG_LF )
        * pStream->_buffer._pCur++ = '\n';
    if (  pStream->_buffer._pCur !=  pStream->_buffer._pBegin )
    {
        return pStream->_fp_flush( 
                    pStream->_param, buf, 
                    pStream->_buffer._pCur - pStream->_buffer._pBegin, &pStream->_flag );
    }
    return 0;
}

#if 0
//
//  Helpful debug
//
void LsLuaApi::dumpStackIdx(lua_State * L, int idx)
{
    size_t          size;
    register int    t;
    t = LsLuaEngine::api()->type(L, idx);
    switch(t)
    {
    case LUA_TNONE:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> TNONE", idx);
        break;
    case LUA_TNIL:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> TNIL", idx);
        break;
    case LUA_TSTRING:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> %s", idx,
                    LsLuaEngine::api()->tolstring( L, idx, &size ));
        break;
    case LUA_TNUMBER:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> %g", idx,
                    LsLuaEngine::api()->tonumberx( L, idx, NULL));
        break;
    case LUA_TBOOLEAN:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> %s", idx,
                    LsLuaEngine::api()->toboolean( L, idx) ? "true" : "false");
        break;
    case LUA_TTABLE:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> TTABLE", idx);
        break;
    case LUA_TFUNCTION:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> TFUNCTION", idx);
        break;
    case LUA_TUSERDATA:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> TUSERDATA", idx);
        break;
    case LUA_TTHREAD:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> TTHREAD", idx);
        break;
    default:
        LsLua_log( L, LSI_LOG_INFO, 0, "STACK <%d> TUNKNOWN %d", idx, t);
        break;
    }
}
#endif

void LsLuaApi::dumpStack(lua_State * L, const char * tag, int numarg)
{
    char            buf[0x1000];
    register int    avail;
    register int    i;
    register int    nb;
    
    avail = LsLuaEngine::api()->gettop(L);
    if (numarg > avail)
        numarg = avail;

    LsLua_log(L, LSI_LOG_INFO, 0, "[%p] %s STACK[%d]", L, tag, avail);
    for (i = avail - numarg + 1; i <= avail; i++)
    {
        // dumpStackIdx(L, i);
        
        if ( (nb = dumpStackIdx2Buf(L, i, buf, 0x1000)) )
            LsLua_log( L, LSI_LOG_INFO, 0, buf ) ;
    }
}

//
//  convert Stack[idx] string format
//
int LsLuaApi::dumpStackIdx2Buf(lua_State * L, int idx, char * buf, int bufLen)
{
    register int    nb = 0;
    size_t          size;
    register int    t;
    
    t = LsLuaEngine::api()->type(L, idx);
    switch(t)
    {
    case LUA_TNONE:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> TNONE", idx );
        break;
    case LUA_TNIL:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> TNIL", idx );
        break;
    case LUA_TSTRING:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> %s", idx,
                    LsLuaEngine::api()->tolstring( L, idx, &size ));
        break;
    case LUA_TNUMBER:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> %g", idx,
                    LsLuaEngine::api()->tonumberx( L, idx, NULL));
        break;
    case LUA_TBOOLEAN:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> %s", idx,
                    LsLuaEngine::api()->toboolean( L, idx) ? "true" : "false");
        break;
    case LUA_TTABLE:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> TTABLE", idx);
        break;
    case LUA_TFUNCTION:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> TFUNCTION", idx);
        break;
    case LUA_TUSERDATA:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> TUSERDATA", idx);
        break;
    case LUA_TTHREAD:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> TTHREAD", idx);
        break;
    default:
        nb = snprintf( buf, bufLen
                    , "STACK <%d> TUNKNOWN %d", idx, t);
        break;
    }
    return nb;
}

void LsLuaApi::dumpStack2Http(lsi_session_t * session, lua_State * L, const char * tag, int numarg)
{
    char            buf[0x1000];
    register int    avail;
    register int    nb;
    register int    i;
    
    avail = LsLuaEngine::api()->gettop(L);
    if (numarg > avail)
        numarg = avail;

    if ( (nb = snprintf(buf, 0x1000, "[%p] %s STACK[%d]\r\n", L, tag, avail)) )
        g_api->append_resp_body(session, buf, nb);
    for (i = avail - numarg + 1; i <= avail; i++)
    {
        if ( (nb = dumpStackIdx2Buf(L, i, buf, 0x1000)) )
            g_api->append_resp_body(session, buf, nb);
            g_api->append_resp_body(session, "\r\n", 2);
    }
}

