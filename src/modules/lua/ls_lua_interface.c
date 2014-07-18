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

/*
*
*   Routine -
*
*   Function -
*
*   include file -
*
*   Created by -    Simon chan
*
*   Date -      02/12/2014
*
*   Remark -    LiteSpeedTech interface
*               only four routines
*               (1) ls_lua_setup - setup library name and startup script
*               (2) ls_lua_loadSys - load the lua module and bind it
*               (3) ls_lua_doScript - exec a LuaScript
*               (4) ls_lua_doCommand - exec a LuaStatement string(null)
*
*   calling sequence
*   ls_lua_setup("/usr/local/lib/libluajit-5.1.so", "/home/user/simon/code/app/lauloader/ls_startup.lua");
*   ls_lua_loadSys();
*   ...
*       ls_lua_doScript("Lua_script_file");
*   ...
*       ls_lua_doCommand("Lua_cmd_string");
*
*   ---------------------------------------------------------------------
*   02/12/2014 SC   Create this one
*/

#if 0 /* NO LONGER USEFULL */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "ls_lua.h"


/*
*   setup the system module here
*/
#define LS_LUA_SYS      "ls_lua_sys"
#define LS_LUA_STARTUP  "ls_startup.lua"
typedef struct
{
    char *              module_path;        /* LUA module path          */
    char *              script_path;        /* LiteSpeed Startup script */
    ls_lua_t *          lsp;                /* loaded module            */
} ls_lua_path_t;
static ls_lua_path_t    _lsSys = {NULL, NULL, NULL};

ls_lua_t *  ls_lua_get()
{
    return _lsSys.lsp;
}

int ls_lua_setUp( const char * modulePath, const char * scriptPath )
{
    if ( modulePath )
    {
        if ( _lsSys.module_path )
        {
            free( _lsSys.module_path );
        }
        if ( !( _lsSys.module_path = strdup( modulePath ) ) )
        {
            return LS_LUA_NO_MEM;
        }
    }

    if ( _lsSys.script_path )
    {
        free( _lsSys.script_path );
    }
    if ( !scriptPath )
    {
        /* _lsSys.script_path = strdup(LS_LUA_STARTUP; */
        /* for testing... */
        _lsSys.script_path = strdup( "/home/user/simon/code/app/lualoader/ls_startup.lua" );
    }
    else
    {
        _lsSys.script_path = strdup( scriptPath );
    }
    if ( !_lsSys.script_path )
    {
        return LS_LUA_NO_MEM;
    }
    return LS_LUA_OK;
}

int ls_lua_loadSys()
{
    char                    errbuf[MAX_ERRBUF_SIZE] = {0};
    register ls_lua_t *     lsp;

    if ( lsp = _lsSys.lsp )
    {
        return LS_LUA_OK;
    }
    if ( !_lsSys.module_path )
    {
        /* fprintf(stderr, "MISSING LS_LUA SYS MODULE\n"); */
        return LS_LUA_FAILED;
    }
    _lsSys.lsp = ls_lua_alloc( LS_LUA_SYS, _lsSys.module_path, errbuf );
    if ( !_lsSys.lsp )
    {
        /* fprintf(stderr, "FAILED TO LOAD LS_LUA SYS MODULE [%s] %s\n",
          _lsSys.module_path, errbuf);
        */
        return LS_LUA_FAILED;
    }
    return LS_LUA_OK;
}
#endif

#if 0
int ls_lua_doScript( const char * scriptName )
{
    register int        ret;
    register ls_lua_t *     lsp;
    if ( !( lsp = _lsSys.lsp ) )
    {
        return LS_LUA_FAILED;
    }
    if ( ret = ls_lua_newState( lsp ) )
    {
        return LS_LUA_FAILED;
    }
    /* exec the system script */
    if ( ret = ls_lua_loadScript( lsp, scriptName ) )
    {
        return LS_LUA_FAILED;
    }

    /* watch... we need to support coroutine...
    *           I will need to check status here
    */
    if ( lsp->wait )
    {
        /* wait state... */
        return LS_LUA_OK;
    }
    else
    {
        ls_lua_close( lsp ); /* free up the stack here */
    }
    return LS_LUA_OK;
}

int ls_lua_doCommand( const char * luaCommand )
{
    register int            ret;
    register ls_lua_t *     lsp;
    if ( !( lsp = _lsSys.lsp ) )
    {
        return LS_LUA_FAILED;
    }
    if ( ret = ls_lua_newState( _lsSys.lsp ) )
    {
        return LS_LUA_FAILED;
    }
    /* exec the system script */
    if ( ret = ls_lua_loadCmd( lsp, luaCommand ) )
    {
        return LS_LUA_FAILED;
    }

    /* watch... we need to support coroutine...
    *           I will need to check status here
    */
    if ( lsp->wait )
    {
        /* wait state... */
        return LS_LUA_OK;
    }
    else
    {
        ls_lua_close( lsp ); /* free up the stack here */
    }
    return LS_LUA_OK;
}
#endif


