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
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <dlfcn.h>

#include <math.h>
#include "ls_lua.h"

/* LUA 5.2 don't use GLOBALSINDEX
*   Since the set and get global should be used.
*/

static void    ls_lua_version_patches( ls_lua_api_t * );
typedef struct
{
    ls_lua_api_t *  dl_link;
    ls_lua_t *      user_link;
    ls_lua_t *      sys;
} ls_lua_base_t;

static ls_lua_base_t    _base = {NULL, NULL, NULL};
static ls_lua_api_t *    _base_Api = NULL;

static void freeModuleItem( ls_lua_api_t * item )
{
    if ( item )
    {
        if ( item->dl )
        {
            dlclose( item->dl );
            item->dl = NULL;
        }

        if ( item->moduleName )
        {
            free( item->moduleName );
            item->moduleName = NULL;
        }
        free( item );
    }
}

#if 0
static void freeModule( ls_lua_api_t * item )
{
    register ls_lua_api_t   *dlp;

    if ( !item )
        return;

    /* disconnect myself */
    if ( item == _base.dl_link )
    {
        _base.dl_link = item->link;
    }
    else
    {
        for ( dlp = _base.dl_link; dlp; dlp = dlp->link )
        {
            if ( dlp->link == item )
            {
                dlp->link = item->link;
            }
        }
    }

    /* free the item here */
    freeModuleItem( item );
}
#endif

static ls_lua_api_t * findModuleByName( const char * name )
{
    register ls_lua_api_t   *dlp;

    for ( dlp = _base.dl_link; dlp; dlp = dlp->link )
    {
        if ( !strcmp( dlp->moduleName, name ) )
        {
            return ( dlp );
        }
    }
    return NULL;
}

#define LUA_DLSYM( name ) {\
    dlp->name = ( pf_ ## name )dlsym( dlp->dl, "lua_" #name ); \
    if ( !dlp->name ) \
    { \
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_" #name ); \
        return LS_LUA_FAILED; \
    } }

#define LUA_DLSYM2( name, lua_func ) {\
    dlp->name = ( pf_ ## name )dlsym( dlp->dl, lua_func ); \
    if ( !dlp->name ) \
    { \
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC %s", lua_func  ); \
        return LS_LUA_FAILED; \
    } }
    

static int  loadModule( ls_lua_api_t * dlp, char * errbuf )
{
    dlp->dl = dlopen( dlp->moduleName, RTLD_LOCAL | RTLD_LAZY );
    if ( !dlp->dl )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "FAILED TO LOAD %s",
                 dlerror() );
        return LS_LUA_FAILED;   /* failed to load */
    }
    /* check for JIT lua */
    /* if (dlsym( dlp->dl, "luaopen_jit" )) */
    if (dlsym( dlp->dl, "luaJIT_setmode" ))
        dlp->jitMode = 1;
    else
        dlp->jitMode = 0;
    
    snprintf( errbuf, MAX_ERRBUF_SIZE, "loading LUA%s MODULE [%s]",
             dlp->jitMode ? " JIT" : "",
             dlp->moduleName
           );
    
    /* bind the function here */
    LUA_DLSYM( close );
    
    LUA_DLSYM2( newstate, "luaL_newstate" );
    LUA_DLSYM2( openlibs, "luaL_openlibs" );
    LUA_DLSYM2( openlib, "luaL_openlib" );
    LUA_DLSYM2( loadstring, "luaL_loadstring" );

    LUA_DLSYM( load );
    
    LUA_DLSYM( setfield );
    LUA_DLSYM( getfield );
    LUA_DLSYM( gettable );

    /* c interfaces */
    LUA_DLSYM( type );
    LUA_DLSYM( gettop );
    LUA_DLSYM( toboolean );
    LUA_DLSYM( tocfunction );

    LUA_DLSYM( topointer );

    LUA_DLSYM( tolstring );
    LUA_DLSYM( tothread );
    LUA_DLSYM( touserdata );

    LUA_DLSYM2( checkudata, "luaL_checkudata" );

    LUA_DLSYM2( newmetatable, "luaL_newmetatable" );
    LUA_DLSYM( newuserdata );
    LUA_DLSYM( remove );
    LUA_DLSYM( xmove );
    LUA_DLSYM( replace );
    LUA_DLSYM( gc );
    
    LUA_DLSYM2( ref, "luaL_ref" );
    LUA_DLSYM2( unref, "luaL_unref" );
    
    /* LUA_DLSYM( setfenv ); */
    dlp->setfenv = ( pf_setfenv )dlsym( dlp->dl, "lua_setfenv" );
    if ( dlp->jitMode && (!dlp->setfenv))
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_setfenv in JITMODE" );
        return LS_LUA_FAILED;
    }

    dlp->objlen = ( pf_objlen )dlsym( dlp->dl, "lua_objlen" );
    dlp->rawlen = ( pf_rawlen )dlsym( dlp->dl, "lua_rawlen" );
    if ( !( dlp->objlen || dlp->rawlen ) )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_objlen and lua_rawlen" );
        return LS_LUA_FAILED;
    }

    LUA_DLSYM( rawgeti );
    LUA_DLSYM( rawseti );
    LUA_DLSYM( rawget );
    LUA_DLSYM( rawset );
    LUA_DLSYM( createtable );
    LUA_DLSYM( settable );

    LUA_DLSYM( setmetatable );
    LUA_DLSYM( settop );
    LUA_DLSYM( pushboolean  );
    LUA_DLSYM( pushcclosure  );
    LUA_DLSYM( pushfstring  );
    LUA_DLSYM( pushlightuserdata );
    LUA_DLSYM( pushinteger  ); 
    LUA_DLSYM( pushnumber  ); 
    LUA_DLSYM( pushlstring  ); 
    LUA_DLSYM( pushstring  );  
    LUA_DLSYM( pushthread   );  
    LUA_DLSYM( pushvalue   );   
    LUA_DLSYM( pushvfstring );
    LUA_DLSYM( pushnil );     
    LUA_DLSYM( next );        
    LUA_DLSYM( insert );        
    LUA_DLSYM( newthread );
    
    LUA_DLSYM( sethook );
    

    /* do all the incompatibility here
    *   hopefully not too many
    */
    dlp->loadbufferx = ( pf_loadbufferx )dlsym( dlp->dl, "luaL_loadbufferx" );
    if ( !dlp->loadbufferx )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC luaL_loadbufferx" );
        return LS_LUA_FAILED;
    }

    dlp->pcall = ( pf_pcall )dlsym( dlp->dl, "lua_pcall" );
    dlp->pcallk = ( pf_pcallk )dlsym( dlp->dl, "lua_pcallk" );
    if ( !( dlp->pcall || dlp->pcallk ) )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_pcall and lua_pcallk" );
        return LS_LUA_FAILED;
    }

    dlp->loadfile = ( pf_loadfile )dlsym( dlp->dl, "luaL_loadfile" );
    dlp->loadfilex = ( pf_loadfilex )dlsym( dlp->dl, "luaL_loadfilex" );
    if ( !( dlp->loadfile || dlp->loadfilex ) )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC luaL_loadfile and luaL_loadfilex" );
        return LS_LUA_FAILED;
    }

    dlp->tonumberx = ( pf_tonumberx )dlsym( dlp->dl, "lua_tonumberx" );
    dlp->tonumber = ( pf_tonumber )dlsym( dlp->dl, "lua_tonumber" );
    if ( !( dlp->tonumber || dlp->tonumberx ) )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_tonumber and lua_tonumberx" );
        return LS_LUA_FAILED;
    }

    dlp->tointegerx = ( pf_tointegerx )dlsym( dlp->dl, "lua_tointegerx" );
    dlp->tointeger = ( pf_tointeger )dlsym( dlp->dl, "lua_tointeger" );
    if ( !( dlp->tointeger || dlp->tointegerx ) )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_tointeger and lua_tointegerx" );
        return LS_LUA_FAILED;
    }

    dlp->yield = ( pf_yield )dlsym( dlp->dl, "lua_yield" );
    dlp->yieldk = ( pf_yieldk )dlsym( dlp->dl, "lua_yieldk" );
    if ( !( dlp->yield || dlp->yieldk ) )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_yield and lua_yieldk" );
        return LS_LUA_FAILED;
    }

    dlp->resume = ( pf_resume )dlsym( dlp->dl, "lua_resume" );
    dlp->resumek = ( pf_resumek )dlsym( dlp->dl, "lua_resume" ); /* hack for 5.2 */
    if ( !( dlp->resume || dlp->resumek ) )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_resume and lua_resumek" );
        return LS_LUA_FAILED;
    }

    /* only supported above 5.1
    */
    dlp->setglobal = ( pf_setglobal )dlsym( dlp->dl, "lua_setglobal" );
    if ( !dlp->setglobal )
    {
        /* 5.2 and above only */
        ;
        /* JIT use setfield instead
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_setglobal");
        return LS_LUA_FAILED;
        */
    }

    dlp->getglobal = ( pf_getglobal )dlsym( dlp->dl, "lua_getglobal" );
    if ( !dlp->setglobal )
    {
        /* 5.2 and above only */
        ;
        /* JIT use getfield instead
        snprintf( errbuf, MAX_ERRBUF_SIZE, "MISSING LUA FUNC lua_getglobal");
        return LS_LUA_FAILED;
        */
    }

    /* patch it */
    ls_lua_version_patches( dlp );

    dlp->remoteEnable = 0;  // disable by default

    return LS_LUA_OK;
}

ls_lua_api_t * ls_lua_dl_loadModule( const char * name, char * errbuf )
{
    register ls_lua_api_t   *dlp;

    errbuf[0] = 0;
    if ( (dlp = findModuleByName( name )) )
    {
        return dlp;
    }
    dlp = ( ls_lua_api_t * )malloc( sizeof( ls_lua_api_t ) + strlen( name ) + 1 );
    if ( !dlp )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "NO MEMORY" );
        return NULL;
    }
    if ( !( dlp->moduleName = strdup( name ) ) )
    {
        snprintf( errbuf, MAX_ERRBUF_SIZE, "NO MEMORY" );
        free( dlp );
        return NULL;
    }

    if ( loadModule( dlp, errbuf ) )
    {
        /* problem in load module */
        free( dlp->moduleName );
        free( dlp );
        return NULL;
    }
    return dlp;
}

/*  Unload all the module here
*/
void    ls_lua_dl_freeAllModule()
{
    register ls_lua_api_t   *dlp;

    for ( dlp = _base.dl_link; dlp; dlp = dlp->link )
    {
        freeModuleItem( dlp );
    }
}


/*
*   NOTE - we may need to re-visit here...  just get this done first.
*   Original I am thinking a general purpose NON-block stuff; which will
*       require using LUA upvalues for book keeping, now George think that
*       it would be only one socket per LUA stack. This will make the
*       implementation very simple.
*   Instead of LUA lookup for ls_lua_t - I use a dumb linear lookup... :(
*
*   now I want std::map... too bad not in the C library.
*
*/

static void addUserToBase( ls_lua_t * user )
{
    user->link = _base.user_link;
    _base.user_link = user;
    if ( !_base.sys )
    {
        _base.sys = _base.user_link;
    }
}

static void removeUserFromBase( ls_lua_t * user )
{
    register ls_lua_t   *up;

    if ( !user )
        return;

    /* disconnect myself */
    if ( user == _base.user_link )
    {
        _base.user_link = user->link;
    }
    else
    {
        for ( up = _base.user_link; up; up = up->link )
        {
            if ( up->link == user )
            {
                up->link = user->link;
                break;
            }
        }
    }
    if ( _base.sys == user )
    {
        _base.sys = _base.user_link;
    }
}

ls_lua_t * ls_lua_findByState( lua_State * state )
{
    register ls_lua_t   *up;

    for ( up = _base.user_link; up; up = up->link )
    {
        if ( up->L == state )
        {
            return ( up );
        }
    }
    return NULL;
}

void    ls_lua_setSystem( ls_lua_t * sys )
{
    _base.sys = sys;
}

ls_lua_t * ls_lua_getSystem()
{
    return _base.sys;
}

/*===============================================================*/
/*
*   load user and binded with the corresponding loadable
*/
ls_lua_t *  ls_lua_alloc( const char * name, const char * moduleName, char * errbuf )
{
    register ls_lua_t   *lsp;

    if ( (lsp = (ls_lua_t *)malloc( sizeof( ls_lua_t ))) )
    {
        if ( (lsp->dl = ls_lua_dl_loadModule( moduleName, errbuf )) )
        {
            if ( (lsp->name = strdup( name )) )
            {
                lsp->L = NULL;
                lsp->link = NULL;
                lsp->udata = NULL;
                lsp->active = 1;
                lsp->mom = NULL;    /* Not a clone */
                addUserToBase( lsp );
                return lsp;
            }
            free( lsp->dl );
        }
        free( lsp );
    }
    return NULL;
}

ls_lua_t *  ls_lua_clone( const char * name, ls_lua_t * mom )
{
    register ls_lua_t   *lsp;

    if ( (lsp = (ls_lua_t *)malloc( sizeof( ls_lua_t ))) )
    {
        if ( (lsp->name = strdup( name )) )
        {
            lsp->dl = mom->dl;
            lsp->L = mom->L;
            lsp->link = NULL;
            lsp->udata = NULL;
            lsp->active = 1;
            lsp->mom = mom; /* from cloning */
            addUserToBase( lsp );
            return lsp;
        }
        free( lsp );
    }
    return NULL;
}

void        ls_lua_free( ls_lua_t * lsp )
{
    if ( lsp )
    {
        removeUserFromBase( lsp );
        if ( lsp->name )
        {
            free( lsp->name );
            lsp->name = NULL;
        }
        free( lsp );
    }
}

/*
====================================================================
    LiteSpeed Simulated function are here
====================================================================
*/
static int ls_setfenv( lua_State * L, int idx )
{
    int top;
    top = _base_Api->gettop( L );
    _base_Api->pushvalue( L, idx);
    _base_Api->setglobal( L, "_G");
    _base_Api->settop( L, top -1 );
    return 1;
}

static void  ls_setglobal( lua_State * L, const char * name )
{
    /* setglobal(lsp->L, name); */
    /* _base_Api->setfield( L, LUA_GLOBALSINDEX, name ); */
    _base_Api->setfield( L, _base_Api->LS_LUA_GLOBALSINDEX, name );
}

static void ls_getglobal( lua_State * L, const char * name )
{
    /* getglobal(lsp->L, name); */
    /* _base_Api->getfield( L, LUA_GLOBALSINDEX, name ); */
    _base_Api->getfield( L, _base_Api->LS_LUA_GLOBALSINDEX, name );
}

static int ls_resume( lua_State * L, lua_State * s, int narg )
{
    /* JIT */
    return _base_Api->resume( L, ( lua_State* )narg, 0 );
}

/* NOTE Check this on every release - This valuable according to JIT 5.1
*/
#ifndef LUA_GLOBALSINDEX
#define LUA_GLOBALSINDEX (-10002)
#endif

static void    ls_lua_version_patches( ls_lua_api_t * dlp )
{
    /* keep the patch data */
    _base_Api = dlp;

    if ( dlp->getglobal && dlp->setglobal )
    {
        /* if there are setglobal and getglobal
            then assume this is 5.2 and above
                NOTE JIT -10000
                NOTE 5.2 -1001000
        */
        dlp->LS_LUA_REGISTRYINDEX = LUA_REGISTRYINDEX;    /* LUA's registry */
        dlp->LS_LUA_GLOBALSINDEX = LUA_GLOBALSINDEX;      /* LUA's global */
#ifndef LUA_RIDX_GLOBALS
        /* In case the user only install the JIT - this will make it compile */
#define LUA_RIDX_GLOBALS 2
#endif
        dlp->LS_LUA_RIDX_GLOBALS = LUA_RIDX_GLOBALS;      /* LUA's registry global index */
        
        dlp->setfenv = ls_setfenv;                        /* simulate setfenv */
        
        /* should have the following undefined... */
        dlp->tointeger = dlp->tointegerx;
        dlp->tonumber = dlp->tonumberx;
        /*
        dlp->pcall
        dlp->loadfile
        dlp->yield
        dlp->objlen
        */
    }
    else
    {
        /* patchs this two */
        dlp->LS_LUA_REGISTRYINDEX = -10000;     /* LUA's registry */
        dlp->LS_LUA_GLOBALSINDEX = -10002;      /* LUA's global */
        dlp->LS_LUA_RIDX_GLOBALS = 0;           /* This is a dummy for older system */

        dlp->getglobal = ls_getglobal;
        dlp->setglobal = ls_setglobal;

        dlp->pcallk = dlp->pcall;         /* with extra 0 and NULL */
        dlp->loadfilex = dlp->loadfile;   /* with extra NULL */
        dlp->tonumberx = dlp->tonumber;   /* with extra NULL (isnum) */
        dlp->tointegerx = dlp->tointeger; /* with extra NULL (isnum) */
        dlp->yieldk = dlp->yield;         /* with extra 0 and NULL */
        dlp->resumek = ls_resume;
    }
}

/*
====================================================================
    LiteSpeed Patch DONE
====================================================================
*/

