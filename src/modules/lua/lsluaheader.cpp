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
#include "lsluasession.h"
#include "lsluaengine.h"
#include "lsluaapi.h"
#include <../addon/include/ls.h>
#include <log4cxx/logger.h>
#include <http/httpsession.h>

static int LsLua_header_dummy(lua_State * L)
{
    LsLuaApi::dumpStack( L, "ls.header dummy ", 10);
    return 0;
}

static int LsLua_header_tostring( lua_State * L )
{
    char    buf[0x100];

    snprintf( buf, 0x100, "<ls.header %p>", L );
    LsLuaEngine::api()->pushstring( L, buf );
    return 1;
}

static int LsLua_header_gc( lua_State * L )
{
    LsLua_log( L, LSI_LOG_INFO, 0, "<ls.header GC>");
    return 0;
}

static const luaL_Reg lslua_header_funcs[] =
{
    {"dummy",       LsLua_header_dummy             },
    {NULL, NULL}
};

static const luaL_Reg lslua_header_meta_sub[] =
{
    {"__gc",        LsLua_header_gc},
    {"__tostring",  LsLua_header_tostring},
    {NULL, NULL}
};


int LsLua_header_get(lua_State * L)
{
#define _NUMIOV 0x100
    struct iovec  iovpool[_NUMIOV];

    size_t          len;
    const char *    cp;
    // LsLuaApi::dumpStack( L, "ls.header GET", 10);

    LsLuaSession * pSession = LsLua_getSession( L );
    cp = LsLuaEngine::api()->tolstring( L, 2, &len);
    // dont touch stack anymore!
    // LsLuaApi::pop(L, 2);
    if (cp && len)
    {
        // only support status for now!
        int num;
        if ( (num = g_api->get_resp_header(pSession->getHttpSession(),
                LSI_RESP_HEADER_CONTENT_TYPE, cp, len, iovpool, _NUMIOV)) ) {
            // only care the last one!
            num--;
            if (((char *)iovpool[num].iov_base)[0] == ' ')
                LsLuaEngine::api()->pushnil(L);
            else
                LsLuaEngine::api()->pushlstring( L,
                        (const char *)iovpool[num].iov_base, iovpool[num].iov_len );
            return 1;
        }
        LsLua_log( L, LSI_LOG_INFO, 0, "ls.header GET %s", cp);
    }
    else
    {
        LsLua_log( L, LSI_LOG_INFO, 0, "ls.header GET BADSTACK");
    }
    LsLuaEngine::api()->pushnil(L);
    LsLuaApi::dumpStackCount( L, "END ls.header GET");
    return 1;
}

int LsLua_header_set(lua_State * L)
{
    size_t          len, tolen;
    const char *    cp; 
    const char *    p_to;
    // LsLuaApi::dumpStack( L, "ls.header SET", 10);

    LsLuaSession * pSession = LsLua_getSession( L );
    cp = LsLuaEngine::api()->tolstring( L, 2, &len);
    if (cp && len)
    {
        p_to = LsLuaEngine::api()->tolstring( L, 3, &tolen);
        if (p_to && tolen)
        {
            LsLua_log( L, LSI_LOG_DEBUG, 0, "ls.header SET %s to %s", cp, p_to);
            g_api->set_resp_header(pSession->getHttpSession(),
                LSI_RESP_HEADER_CONTENT_TYPE, cp, len, p_to, tolen, LSI_HEADER_SET);
        }
        else
        {
            g_api->set_resp_header(pSession->getHttpSession(),
                LSI_RESP_HEADER_CONTENT_TYPE, cp, len, "", 0, LSI_HEADER_SET);
            LsLua_log( L, LSI_LOG_DEBUG, 0, "ls.header DEL %s", cp);
        }
    }
    else
    {
        LsLua_log( L, LSI_LOG_INFO, 0, "ls.header SET BADSTACK");
    }
    return 0;
}

void LsLua_create_header( lua_State * L )
{
    /* should have the table on the stack already */
    // LsLuaApi::dumpStack( L, "BEGIN create_header", 10);
    
    LsLuaEngine::api()->createtable(L, 0, 0); /* create ls.header table         */
    LsLuaEngine::api()->createtable(L, 0, 2); /* create metatable for ls.header */
    LsLuaEngine::api()->pushcclosure(L, LsLua_header_set, 0);
    LsLuaEngine::api()->setfield(L, -2, "__newindex");
    LsLuaEngine::api()->pushcclosure(L, LsLua_header_get, 0);
    LsLuaEngine::api()->setfield(L, -2, "__index");
    LsLuaEngine::api()->setmetatable(L, -2);
    LsLuaEngine::api()->setfield(L, -2, "header"); 
    
    // LsLuaApi::dumpStack( L, "END create_header", 10);
}

void LsLua_create_header_meta( lua_State * L )
{
    ;
}

