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
#ifndef ls_lua_h
#define ls_lua_h

#include <math.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
/* need lua_State * */
#include <lua.h>
#include <lauxlib.h>
#ifdef __cplusplus
};
#endif

#if 0
/* Check this on every release - This valuable according to JIT 5.1
*/
#ifndef LUA_GLOBALSINDEX
#define LUA_GLOBALSINDEX (-10002)
#endif
#endif

/* LUA Table names */
#define LS_LUA_SYS      "ls_lua_sys"    /* Module name after loaded */
#define LS_LUA          "ls"            /* LiteSpeed Name space */
#define LS_LUA_FUNCTABLE "_func"            /* LiteSpeed Name space */
#define LS_LUA_HEADER   "header"        /* header */
#define LS_LUA_CTX      "ctx"           /* ctx */
#define LS_LUA_URIARGS  "__UriArgs"     /* uri args table */
#define LS_LUA_POSTARGS "__PostArgs"    /* POST args table */

#define LS_LUA_CLEANUPMSEC 2000    /* specified cleanup time (msec) after _end() 2sec delay*/

/* loadable function */
typedef lua_State *     ( *pf_newstate )();
typedef void ( *pf_func )( lua_State * );
typedef void ( *pf_openlibs )( lua_State * );
typedef void ( *pf_openlib )( lua_State *, const char * tname, const luaL_Reg *, int nup );
typedef void ( *pf_close )( lua_State * );
typedef int ( *pf_loadstring )( lua_State *, const char * );
typedef int ( *pf_load )( lua_State *, lua_Reader reader, void * data, const char * chunkname);

/* allow to setup c interface */
typedef void ( *pf_setglobal )( lua_State *, const char * );
typedef void ( *pf_getglobal )( lua_State *, const char * );
typedef int ( *pf_setfield )( lua_State *, int, const char * );
typedef void ( *pf_getfield )( lua_State *, int, const char * );
typedef void ( *pf_gettable )( lua_State *, int idx );

/* stack */
typedef int ( *pf_gettop )( lua_State * );
typedef int ( *pf_type )( lua_State *, int );
typedef int             ( *pf_toboolean )( lua_State *, int );
typedef lua_CFunction   ( *pf_tocfunction )( lua_State *, int );
typedef const char *    ( *pf_tolstring )( lua_State *, int, size_t * );
typedef const void *    ( *pf_topointer )( lua_State *, int );
typedef lua_State *     ( *pf_tothread )( lua_State *, int );
typedef void *          ( *pf_touserdata )( lua_State *, int );
typedef void *          ( *pf_checkudata )( lua_State *, int, const char * tname );
typedef void *          ( *pf_newuserdata )( lua_State *, size_t size );
typedef int             ( *pf_newmetatable )( lua_State *, const char * tname );
typedef void            ( *pf_remove )( lua_State *, int );
typedef void            ( *pf_xmove )( lua_State * from, lua_State * to , int num);
typedef void            ( *pf_replace )( lua_State * , int idx);
typedef int            ( *pf_setfenv )( lua_State * , int idx);
typedef int            ( *pf_gc )( lua_State * , int what, int data);

/* strlen = lawlen or objlen */
typedef size_t ( *pf_objlen )( lua_State *, int );
typedef size_t ( *pf_rawlen )( lua_State *, int );
typedef void ( *pf_rawget )( lua_State *, int idx );
typedef void ( *pf_rawset )( lua_State *, int idx );
typedef void ( *pf_rawgeti )( lua_State *, int idx, int n );
typedef void ( *pf_rawseti )( lua_State *, int idx, int n );
typedef int ( *pf_ref )( lua_State *, int idx );
typedef void ( *pf_unref )( lua_State *, int idx, int ref );

/* table */
typedef void ( *pf_createtable )( lua_State *, int  narray, int nrec );
typedef void ( *pf_settable )( lua_State *, int idx );
typedef void ( *pf_setmetatable )( lua_State *, int idx );
typedef void ( *pf_settop )( lua_State *, int num );

typedef void ( *pf_pushboolean )( lua_State *, int );
typedef void ( *pf_pushcclosure )(lua_State *, lua_CFunction, int );
typedef const char * ( *pf_pushfstring )(lua_State *L, const char *fmt, ...);
typedef void ( *pf_pushinteger )( lua_State *, int );
typedef void ( *pf_pushnumber )( lua_State *, lua_Number );
typedef void ( *pf_pushlightuserdata )(lua_State *L, void *p);
typedef void ( *pf_pushlstring )( lua_State *, const char *, size_t len );
typedef void ( *pf_pushstring )( lua_State *, const char * );
typedef void ( *pf_pushthread )(lua_State *L);
typedef void ( *pf_pushvalue )( lua_State *, int idx );
typedef const char * ( *pf_pushvfstring )(lua_State *, const char *, va_list );
typedef void ( *pf_pushnil )( lua_State * );

typedef int ( *pf_next )( lua_State *, int idx );
typedef int ( *pf_insert )( lua_State *, int idx );

typedef lua_State *     ( *pf_newthread )( lua_State * );

/* NOTE on PATCHES - NOTED
    all the 5.1 and below will call the 5.2 with extract 0 or NULL
*/
/* typedef int              (*pf_pcall)(lua_State *, int, int, int); */
typedef int ( *pf_pcall )( lua_State *, int, int, int, int, lua_CFunction );
typedef int ( *pf_pcallk )( lua_State *, int, int, int, int, lua_CFunction );
/* typedef int              (*pf_loadfile)(lua_State *, const char *); */
typedef int ( *pf_loadfile )( lua_State *, const char *, const char * );
typedef int ( *pf_loadfilex )( lua_State *, const char *, const char * );
/* typedef double           (*pf_tonumber)(lua_State *, int); */
typedef double( *pf_tonumber )( lua_State *, int, int * );
typedef double( *pf_tonumberx )( lua_State *, int, int * );
/* typedef int           (*pf_tointeger)(lua_State *, int); */
typedef int ( *pf_tointegerx )( lua_State *, int, int *isnum );
typedef int ( *pf_tointeger )( lua_State *, int, int *isnum );

/* coroutine */
/* typedef int              (*pf_yield)(lua_State *, int nres); */
typedef int ( *pf_yield )( lua_State *, int nres, int ctx, lua_CFunction k );
typedef int ( *pf_yieldk )( lua_State *, int nres, int ctx, lua_CFunction k );
/* typedef int              (*pf_resume)(lua_State *, int narg); */
typedef int ( *pf_resume )( lua_State *, lua_State *from, int narg );
typedef int ( *pf_resumek )( lua_State *, lua_State *from, int narg );

/** END OF PATCHES */

typedef int ( *pf_loadbufferx )( lua_State *, const char * buf, size_t, const char * name, int mode );

/** LUA HOOKING **/
typedef int ( * pf_sethook )(lua_State *L, lua_Hook func, int mask, int count);

typedef struct ls_lua_api_t ls_lua_api_t;
struct ls_lua_api_t
{
    void *          dl;
    pf_newstate     newstate;
    pf_openlibs     openlibs;
    pf_openlib      openlib;
    pf_close        close;

    pf_loadstring   loadstring;
    pf_load         load;

    pf_setfield     setfield;
    pf_getfield     getfield;
    pf_gettable     gettable;

    /* c interface */
    pf_gettop       gettop;
    pf_type         type;
    pf_toboolean    toboolean;
    pf_tocfunction  tocfunction;
    pf_tolstring    tolstring;
    pf_topointer    topointer;
    pf_touserdata   touserdata;
    pf_tothread     tothread;
    pf_checkudata   checkudata;
    pf_objlen       objlen;
    pf_rawlen       rawlen;
    pf_rawget       rawget;
    pf_rawset       rawset;
    pf_rawgeti      rawgeti;
    pf_rawseti      rawseti;
    pf_newuserdata  newuserdata;
    pf_newmetatable newmetatable;
    pf_remove       remove;
    pf_xmove        xmove;
    pf_replace      replace;
    pf_setfenv      setfenv;
    pf_gc           gc;
    
    pf_ref          ref;
    pf_unref        unref;

    /* table */
    pf_createtable  createtable;
    pf_settable     settable;
    pf_setmetatable setmetatable;
    pf_settop       settop;

    pf_pushboolean  pushboolean;
    pf_pushcclosure pushcclosure;
    pf_pushfstring  pushfstring;
    pf_pushlightuserdata pushlightuserdata;
    pf_pushinteger  pushinteger;
    pf_pushnumber   pushnumber;
    pf_pushlstring  pushlstring;
    pf_pushstring   pushstring;
    pf_pushthread   pushthread;
    pf_pushvalue    pushvalue;
    pf_pushvfstring pushvfstring;
    pf_pushnil      pushnil;
    pf_next         next;

    pf_insert       insert;

    /* coroutine */
    pf_newthread    newthread;

    /*
    * 5.1 and below (include JIT)
    */
    pf_tointeger    tointeger;
    pf_tonumber     tonumber;
    pf_loadfile     loadfile;
    pf_pcall        pcall;

    pf_yield        yield;
    pf_resume       resume;

    /* 5.2 */
    pf_loadbufferx  loadbufferx;
    pf_tointegerx   tointegerx;         // needed one more NULL at the end
    pf_tonumberx    tonumberx;          // needed one more NULL at the end
    pf_loadfilex    loadfilex;          // needed one more NULL at the end
    pf_pcallk       pcallk;             // needed one more NULL at the end

    pf_yieldk       yieldk;             // needed 0 and NULL at the end
    pf_resumek      resumek;            // parameter different (extra ctx)

    /* 5.2 and above */
    pf_setglobal    setglobal;          // only supported > 5.2
    pf_getglobal    getglobal;

    /* LUA hooking */
    pf_sethook      sethook;
    
    /* JIT dummy */
    int             jitMode;
    
    /* LiteSpeed Internal
    */
    int             LS_LUA_GLOBALSINDEX;     // 5.2 and above this is a dummy
    int             LS_LUA_REGISTRYINDEX;    // important for version below 5.2
    int             LS_LUA_RIDX_GLOBALS;     // jitMode this a dummy - only good for > 5.2
    int             remoteEnable;            // enable script debug
    

    ls_lua_api_t *  link;
    char *          moduleName;
};

typedef struct ls_lua_t ls_lua_t;
struct ls_lua_t
{
    ls_lua_api_t *  dl;
    lua_State *     L;
    ls_lua_t *      link;           /* so I can keep track of them */
    int             active;         /* active                   */
    ls_lua_t *      mom;            /* parent                   */
    void *          udata;          /* user data                */
    char *          name;
};

#define LS_LUA_OK        0
#define LS_LUA_FAILED   -1          /* binding/module       */
#define LS_LUA_NO_MEM   -2
#define LS_LUA_BAD_CALL -3          /* failed executed lua  */
#define LS_LUA_ERR      -4          /* failed to setup lua  */

#define MAX_ERRBUF_SIZE     4096


/* callable */
#ifdef __cplusplus
extern "C" {
#endif
ls_lua_t *      ls_lua_alloc( const char * key, const char * moduleName, char * errbuf );

#ifdef __cplusplus
};
#endif

#endif
