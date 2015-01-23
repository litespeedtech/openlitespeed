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
#ifndef NO_LUA_TEST
#include "lsluaengine.h"
#include "lsluaapi.h"
#include "ls_lua.h"
#endif

#include <shm/lsshmhash.h>
#include <shm/lsi_shm.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#define LSLUA_SHMVALUE_MAGIC   0x20140523

//
// LiteSpeed LUA Shared memory interface
//

//
//  Data structure to keep all LUA Shared memory stuff
//  In general all we use are long and double.
//  and small strings buffer...
//  If you need more data than a double link will be used.
//  I would need a timer check function very soon...
//
typedef enum {
    nil_type = 0,
    long_type,
    double_type,
    string_type,
    boolean_type
} lsLuaShmValueType_t;

struct lsLuaShmValue_s
{
    uint32_t             m_magic;        
    time_t               m_expireTime;   /* time to expire in sec    */
    int32_t              m_expireTimeUs; /* time to expire in sec    */
    uint32_t             m_flags;        /* 32bits flags             */
    uint32_t             m_valueLen;     /* len of the Value         */
    lsLuaShmValueType_t  m_type;         /* type of the Value        */
    union {
        double           m_double;
        uint32_t         m_offset;       /* offset to the SHM        */
        uint32_t         m_ulong;
        uint32_t         m_ulongArray [2];
        uint16_t         m_ushortArray[4];
        uint16_t         m_ushort;
#define LSSHM_VALUE_MINLEN       8 /* a structure or memory  */
        uint8_t          m_ucharArray [LSSHM_VALUE_MINLEN];
        uint8_t          m_uchar;
        uint8_t          m_boolean;
    };
};
typedef struct lsLuaShmValue_s  lsLuaShmValue_t;

//
//  Helper function to create/open share memory for Lua
//
static inline lsi_shmhash_t * lsLuaOpenShm(const char * name)
{
    return lsi_shmhash_open(NULL, name,
                                    LSSHM_HASHINITSIZE,
                                    LsShmHash::hash_string,
                                    LsShmHash::comp_string);
}

static inline int lsLuaCloseShm(lsi_shmhash_t *phash)
{
    return lsi_shmhash_close(phash);
}

//
//  @brief lsLuaFindShmValue
//  @brief (1) search the name from hash and return the previously defined value 
//
//  @brief phash - LiteSpeed SHM Hash table handle
//  @brief name - name string to search for
//  @brief return 0 if failed to find name
//
static lsLuaShmValue_t * lsLuaFindShmValue(lsi_shmhash_t *phash
                                           , const char * name)
{
    int retsize;
    lsi_shmhash_datakey_t       key;
    int len = strlen(name)+1;
    if ( (key = lsi_shmhash_find(phash, (const uint8_t *)name, len, &retsize)) )
    {
        return (lsLuaShmValue_t *)lsi_shmhash_datakey2ptr(phash, (lsi_shm_off_t)key);
    }
    return NULL;
}

//
//  @brief lsLuaSetShmValue
//  @brief (1) search the name from hash and return the previously defined value 
//  @brief (2) if not defined then a new one will be created according to the given
//  @brief              parameter
//  @brief (3) if defined then the new parameter will over written the hashvalue
//  @brief (4) if pvalue is null then the SHM hash object will be removed
//
//  @brief phash - LiteSpeed SHM Hash table handle
//  @brief name - name string to search for
//  @brief type - long, double, non-null, terminated string and boolean
//  @brief pvalue - the given value pointer
//  @brief size - sizeof the object
//
//  @brief return 0 if set is good... else -1
//
static lsLuaShmValue_t * lsLuaSetShmValue(lsi_shmhash_t *phash
                                           , const char * name
                                           , lsLuaShmValueType_t type
                                           , const void * pvalue,
                                           int size)
{
    lsi_shmhash_datakey_t       key;
    int                         retsize;
    
    int len = strlen(name)+1;
    
    if (!pvalue)
    {
        /* remove the name from SHM HASH OBJECT */
        lsi_shmhash_remove(phash, (const uint8_t *)name, len);
        return NULL;
    }
    if (!size)
        return NULL; /* bad parameter */
    
    if (!(key = lsi_shmhash_find(phash, (const uint8_t *)name, len, &retsize)))
    {
        /* create a new SHM HASH OBJECT */
        lsLuaShmValue_t shmvalue;
        
        shmvalue.m_magic = LSLUA_SHMVALUE_MAGIC;
        shmvalue.m_expireTime = 0;
        shmvalue.m_expireTimeUs = 0;
        shmvalue.m_flags = 0;
        shmvalue.m_type = type;
        shmvalue.m_valueLen = size;
        if ( size > (int)sizeof(double) )
        {
            lsi_shm_key_t offset;
            // allocated pool memory if size if larger than standard...
            offset = lsi_shmhash_alloc2(phash, size);
            if (!offset)
                return NULL; 
            
            shmvalue.m_offset = offset;
            memcpy( lsi_shmhash_key2ptr(phash, offset), pvalue, size);
        }
        else
        {
            memcpy(shmvalue.m_ucharArray, pvalue, size);
        }
        
        key = lsi_shmhash_insert(phash, (const uint8_t *)name, len, (const uint8_t *)&shmvalue, sizeof(shmvalue));
        if (!key)
        {
            if (size > (int)sizeof(double))
                lsi_shmhash_release2(phash, shmvalue.m_offset, shmvalue.m_valueLen);
            return NULL;
        }
        return (lsLuaShmValue_t*)lsi_shmhash_datakey2ptr(phash, key);
    }

    // replace to new value
    lsLuaShmValue_t * pShmValue;
    pShmValue = (lsLuaShmValue_t *)lsi_shmhash_datakey2ptr(phash, key);
    
    /* if ( (type == string_type) && (size > sizeof(double)) ) */
    if ( (size > (int)sizeof(double)) )
    {
        // if newsize bigger need double reference
        if ((int)(pShmValue->m_valueLen) != size)
        {
            lsi_shm_key_t offset ;
            offset = lsi_shmhash_alloc2(phash, size) ;
            if (!offset)
                return NULL; // no memory
            
            if (pShmValue->m_valueLen > sizeof(double))
                lsi_shmhash_release2(phash, pShmValue->m_offset, pShmValue->m_valueLen );
            
            pShmValue->m_offset = offset;
            pShmValue->m_valueLen = size;
        }
        pShmValue->m_type = type;
        memcpy( lsi_shmhash_key2ptr(phash, pShmValue->m_offset), pvalue, size);
        return pShmValue;
    }
   
    if (pShmValue->m_valueLen > sizeof(double))
    {
        lsi_shmhash_release2(phash, pShmValue->m_offset, pShmValue->m_valueLen );
    }
   
    pShmValue->m_type = type;
    pShmValue->m_valueLen = size;
    switch(type)
    {
    case long_type:
        pShmValue->m_ulong = *((unsigned long*)pvalue);
        break;
    case double_type:
        pShmValue->m_double = *((double*)pvalue);
        break;
    case string_type:
        memcpy( pShmValue->m_ucharArray, pvalue, size);
        break;
    case boolean_type:
        pShmValue->m_boolean = *((unsigned char *)pvalue);
        break;
    default:
        return NULL;
    }
    return pShmValue;
}

//
//  Check if time all expired...
//
inline static int isTimeExpired( lsLuaShmValue_t * pShmValue )
{
    if ( ! pShmValue->m_expireTime )
        return 0; // never expire
        
    time_t t;
    int32_t usec;
    t = g_api->get_cur_time(&usec);
    
    time_t dt;
    dt = t - pShmValue->m_expireTime;
    if ( (dt > 0)
            || ( (!dt)&&(usec > pShmValue->m_expireTimeUs)) )
        return 1;
    else
        return 0;
}

//
//  @brief setExpirationTime - set the expiration time for lsLuaShmValue_t
//  @brief return 0 - expiration saved
//  @brief return 1 - won't expire
//
inline static int setExpirationTime( lsLuaShmValue_t * pShmValue,
                                            double expTime)
{
    // check resolution
#define MIN_RESOLUTION 0.001
    if (expTime < MIN_RESOLUTION)
    {
        pShmValue->m_expireTime = 0;
        return 1;
    }
    int sec = (int)expTime; // chop!
    int usec = (int)((expTime - sec) * 1000000.0) % 1000000;
    
    // potential race between sec and usec loading...
    int32_t now_usec;
    time_t t = g_api->get_cur_time(&now_usec);
   
    time_t curTime = t + sec;
    int curTimeUs = now_usec + usec;
    if (curTimeUs > 1000000)
    {
        curTime++;
        curTimeUs -= 1000000;
    }
    pShmValue->m_expireTime = curTime;
    pShmValue->m_expireTimeUs = curTimeUs;
    return 0;
} 

int LsLua_shared_cbfunc_check_expired_n_release(lsi_shmhash_t *pHash
                            , uint8_t * p
                            , void * )
{
    lsLuaShmValue_t * pShmValue = (lsLuaShmValue_t*)p;
    
    if ( isTimeExpired(pShmValue) )
    {
        pShmValue->m_magic = 0;
        if (pShmValue->m_valueLen > sizeof(double))
        {
            lsi_shmhash_release2(pHash, pShmValue->m_offset, pShmValue->m_valueLen);
        }
        return 1;
    }
    return 0;
}

int LsLua_shared_cbfunc(lsi_shmhash_t *pHash
                            , uint8_t * p
                            , void * pUdata)
{
    const char * keyValue = (const char *)pUdata;
    lsLuaShmValue_t * pShmValue = (lsLuaShmValue_t*)p;
    
    if ( (!strcmp(keyValue, "flush_all")) )
    {
        pShmValue->m_expireTime = 1; // expire it
    }
    else
    {
        pShmValue->m_expireTime = 2; // expire it
    }
    return 0;
}

#ifndef NO_LUA_TEST
//
//  LUA Interface 
//

#define LSLUA_SHARED_DATA "LS_SHARED"
//
//  Useful inline for lsi_shmhash_t * udata
//
static inline lsi_shmhash_t * LUA_CHECKU_SHARED( lua_State * L, const char * tag )
{
    register lsi_shmhash_t * * pp;
    pp = ( lsi_shmhash_t * * )LsLuaEngine::api()->checkudata( L, 1, LSLUA_SHARED_DATA );
    if ( !pp )
    {
        LsLua_log( L, LSI_LOG_NOTICE, 0, "%s <INVALID LUA UDATA>" , tag );
        return 0;
    }
    return (*pp);
}

//
//  @brief lsLuaSharedObjectHelper
//  @brief Helper should be called in each of the sharedObject
//  @brief (1) check actual number of elements against the given numElem
//  @brief (2) extract the keyName to keyName buffer
//  @brief (3) if keyName is too long... it will truncated to keyLen.
//  @brief return lsi_shmhash_t * if success
//  @brief return NULL on any error
//
static inline lsi_shmhash_t * lsLuaSharedObjectHelper( lua_State * L, const char * tag
                                        , int & numElem, char * keyName, int keyLen )
{
    // LsLuaApi::dumpStack( L, "BEGIN lsLuaShareObjectHelper", 10);
    register lsi_shmhash_t * pShared = LUA_CHECKU_SHARED(L, tag);
    register int num = LsLuaEngine::api()->gettop(L);
    
    /*  
     *NOTE: 1 = USERDATA T, 2 = KEY, ... 
     */
    if (( num >= numElem ) && pShared )
    {
        numElem = num;
        size_t  size;
        const char * cp;
        if ( ( cp = LsLuaEngine::api()->tolstring( L, 2, &size ) )  &&  size )
        {
            keyName[0] = 0;
            if (size >= (size_t)keyLen)
            {
                // the string buffer is too big
                LsLua_log( L, LSI_LOG_NOTICE, 0
                            , "%s LUA SHARE NAME [%s] LEN %d too big"
                            , tag, keyName, size);
                return NULL;
            }
            // snprinf - guaranteed the null will be added - given keyName is big enough
            snprintf(keyName, keyLen, "%.*s", (int)size, cp);
            if (keyName[0])
                return pShared;
        }
    }
    return NULL;
}
            
static int LsLua_shared_tostring( lua_State * L )
{
    const char * tag = "ls.shared.tostring";
    char    buf[0x100];
    
    register lsi_shmhash_t * pShared = LUA_CHECKU_SHARED(L, tag);
    if ( pShared )
        snprintf( buf, 0x100, "%s <%p>", tag, pShared );
    
    LsLuaEngine::api()->pushstring( L, buf );
    return 1;
}

static int LsLua_shared_gc( lua_State * L )
{
    const char * tag = "ls.shared.gc";
    register lsi_shmhash_t * pShared = LUA_CHECKU_SHARED(L, tag);
    
    if (pShared)
    {
        lsLuaCloseShm(pShared);
        LsLua_log( L, LSI_LOG_DEBUG, 0, "LsLua_shared_gc %s <%p>", tag, pShared);
    }
    return 0;
}

//
//  Helper to create a LUA Meta Object for ls.shared.DICT
//
static int  LsLua_shared_create ( lua_State * L , lsi_shmhash_t * pHash)
{
    // LsLuaApi::dumpStack( L, "BEGIN LsLua_shared_create", 10);
    if (pHash)
    {    
        register lsi_shmhash_t * * pp;
        pp = ( lsi_shmhash_t * * )( LsLuaEngine::api()->newuserdata( L, sizeof( lsi_shmhash_t * * )));
        if (pp)
        {
            *pp = pHash;
            LsLuaEngine::api()->getfield( L, LsLuaEngine::api()->LS_LUA_REGISTRYINDEX, LSLUA_SHARED_DATA );
            LsLuaEngine::api()->setmetatable( L, -2 );
            // LsLuaApi::dumpStack( L, "LsLua_shared_create", 10);
            return 1;
        }
    }
    // LsLuaApi::dumpStack( L, "END LsLua_shared_create", 10);
    return LsLuaApi::return_badparameterx( L, LSLUA_BAD_PARAMETERS );
}

//
//  @brief LsLua_shared_get_helper - support get and get_stale
//
inline static int  LsLua_shared_get_helper( lua_State * L, int checkExpired )
{
    const char * tag = "ls.shared.get_helper";
    char namebuf[0x100];
    int numElem = 2;
    register lsi_shmhash_t * pShared ;

    // LsLuaApi::dumpStack( L, "BEGIN LsLua_shared_get_helper", 10);
    pShared = lsLuaSharedObjectHelper( L, tag, numElem, namebuf, 0x100 );
    if (pShared)
    {
        register lsLuaShmValue_t * pVal;
        pVal = lsLuaFindShmValue(pShared, namebuf);
        if (!pVal)
        {
            // LsLua_log( L, LSI_LOG_NOTICE, 0, "HASH-NOT-FOUND %s <%p> [%s]", tag, pShared, namebuf);
                
            LsLuaEngine::api()->pushnil( L );
            LsLuaEngine::api()->pushstring( L, "not found");
            return 2;
        }
        
        if ( checkExpired && (isTimeExpired( pVal )) )
        {
            LsLuaEngine::api()->pushnil( L );
            LsLuaEngine::api()->pushstring( L, "expired");
            return 2;
        }
        
        switch(pVal->m_type)
        {
        case long_type:
            LsLuaEngine::api()->pushinteger( L, pVal->m_ulong );
            break;
        case double_type:
            LsLuaEngine::api()->pushnumber( L, (lua_Number)pVal->m_double );
            break;
        case string_type:
            if ( pVal->m_valueLen > sizeof(double) )
            {
                LsLuaEngine::api()->pushlstring( L
                                , (const char *)lsi_shmhash_key2ptr( pShared, pVal->m_offset )
                                , pVal->m_valueLen);
            }
            else
            {
                LsLuaEngine::api()->pushlstring( L
                                , (const char *)pVal->m_ucharArray
                                , pVal->m_valueLen);
            }
            break;
                
        case boolean_type:
            if (pVal->m_boolean)
                LsLuaEngine::api()->pushboolean( L, 1 );
            else
                LsLuaEngine::api()->pushboolean( L, 0 );
            break;
                
        case nil_type:
        default:
            /* problem unknow type */
            LsLuaEngine::api()->pushnil( L );
            LsLuaEngine::api()->pushstring( L, "not a shared value type");
            return 2;
        }
            
        // LsLuaApi::dumpStack( L, "END LsLua_shared_get", 10);
        if ( !checkExpired )
        {
            LsLuaEngine::api()->pushinteger( L, pVal->m_flags );
            
            if ( isTimeExpired(pVal) )
                LsLuaEngine::api()->pushboolean( L, 1);
            return 3;
        }
        if (pVal->m_flags) // tricky... doc...
        {
            LsLuaEngine::api()->pushinteger( L, pVal->m_flags );
            return 2;
        }
        else
            return 1;
    }
    // something is not right about LUA - may be new version!
    LsLuaEngine::api()->pushnil( L );
    LsLuaEngine::api()->pushstring( L, "not a shared OBJECT");
    return 2;
}

static int  LsLua_shared_get( lua_State * L )
{ 
    return LsLua_shared_get_helper(L, 1);
}

static int  LsLua_shared_get_stale( lua_State * L )
{ 
    return LsLua_shared_get_helper(L, 0);
}

//
//  @brief LsLua_shared_set_helper
//  @brief load value from lua_State STACK element 
//  @brief elements:      3 = value, 4 = expiration, 5 flags
//
static int LsLua_shared_set_helper(lua_State * L
                        , lsi_shmhash_t * pShared
                        , int numElem
                        , const char * keyName)
{
    // LsLuaApi::dumpStack( L, "BEGIN LsLua_shared_set_helper", 10);
    
    /* NOTE: elements 3 = value, 4 = expTime, 5 = flags */
    register lsLuaShmValue_t * p_Val = NULL;
    const char * p_string;
    size_t      len;
            
    int valueType = LsLuaEngine::api()->type( L, 3 );
    switch (valueType)
    {
    case LUA_TNIL:
        // remove the share key
        p_Val = lsLuaSetShmValue(pShared, keyName, nil_type, NULL, 0);
        LsLuaEngine::api()->pushboolean( L, 1 );
        LsLuaEngine::api()->pushnil( L );
        LsLuaEngine::api()->pushboolean( L, 0 ); // always...
        // DONE! 
        return 3;
                
    case LUA_TNUMBER:
        // using double
        double myDouble;
        long myLong;
        p_string = LsLuaEngine::api()->tolstring( L, 3, &len ) ;
        if (memchr(p_string, '.', len))
        {
            // double
            myDouble = LsLuaEngine::api()->tonumberx( L, 3, NULL );
            p_Val = lsLuaSetShmValue(pShared, keyName, double_type, &myDouble, sizeof(myDouble));
        }
        else
        {
            // regular long
            myLong = LsLuaEngine::api()->tonumberx( L, 3, NULL );
            p_Val = lsLuaSetShmValue(pShared, keyName, long_type, &myLong, sizeof(myLong));
        }
        break;
                
    case LUA_TBOOLEAN:
        // using 1 or 0
        char myBool;
        if ( LsLuaEngine::api()->toboolean( L, 3) )
        {
            // true
            myBool = 1;
        }
        else
        {
            // false
            myBool = 0;
        }
        p_Val = lsLuaSetShmValue(pShared, keyName, boolean_type, &myBool, sizeof(myBool));
        break;
        
    case LUA_TSTRING:
        p_string = LsLuaEngine::api()->tolstring( L, 3, &len ) ;
        p_Val = lsLuaSetShmValue(pShared, keyName, string_type, p_string, len);
        break;
                
    case LUA_TNONE:
    case LUA_TTABLE:
    case LUA_TFUNCTION:
    case LUA_TUSERDATA:
    case LUA_TTHREAD:
    case LUA_TLIGHTUSERDATA:
    default:
        LsLuaEngine::api()->pushboolean( L, 0 );
        LsLuaEngine::api()->pushstring( L, "bad value type" );
        LsLuaEngine::api()->pushboolean( L, 0 );
        return 3;
    }

    if (p_Val)
    {
        if (numElem > 3)
        {
            lua_Number tValue = LsLuaEngine::api()->tonumberx( L, 4, NULL);
            setExpirationTime( p_Val, tValue );
                
            if (numElem > 4)
            {
                p_Val->m_flags = LsLuaEngine::api()->tointegerx( L, 5, NULL );
            } 
        }
        LsLuaEngine::api()->pushboolean( L, 1 );
        LsLuaEngine::api()->pushnil( L );
        LsLuaEngine::api()->pushboolean( L, 0 );
        // LsLuaApi::dumpStack( L, "END LsLua_shared_set", 10);
        return 3;
    }
    LsLuaEngine::api()->pushboolean( L, 0 );
    LsLuaEngine::api()->pushstring( L, "bad hashkey" );
    LsLuaEngine::api()->pushboolean( L, 0 );
    return 3;
}

static int  LsLua_shared_set( lua_State * L )
{
    const char * tag = "ls.shared.set";
    char namebuf[0x100];
    int numElem = 3;
    register lsi_shmhash_t * pShared ;

    // LsLuaApi::dumpStack( L, "BEGIN LsLua_shared_set", 10);
    pShared = lsLuaSharedObjectHelper( L, tag, numElem, namebuf, 0x100 );
    if (pShared)
    {
        return LsLua_shared_set_helper( L, pShared, numElem, namebuf );
    }
    LsLuaEngine::api()->pushboolean( L, 0 );
    LsLuaEngine::api()->pushstring( L, "bad parameters" );
    LsLuaEngine::api()->pushboolean( L, 0 );
    return 3;
}

// NO different as shared_set
static int LsLua_shared_safe_set(lua_State * L)
{
    return LsLua_shared_set(L);
}

static int  LsLua_shared_add( lua_State * L )
{
    const char * tag = "ls.shared.add";
    char namebuf[0x100];
    int numElem = 3;
    register lsi_shmhash_t * pShared ;

    // LsLuaApi::dumpStack( L, "BEGIN LsLua_shared_add", 10);
    pShared = lsLuaSharedObjectHelper( L, tag, numElem, namebuf, 0x100 );
    if (pShared)
    {
        lsLuaShmValue_t * pShmValue = lsLuaFindShmValue(pShared, namebuf);
        if ( (!pShmValue) || isTimeExpired(pShmValue) )
        {
            return LsLua_shared_set_helper( L, pShared, numElem, namebuf );
        }
        else
        {
            LsLuaEngine::api()->pushboolean( L, 0 );
            LsLuaEngine::api()->pushstring( L, "exists" );
            LsLuaEngine::api()->pushboolean( L, 0 );
        }
        return 3;
    }
    else
    {
        LsLuaEngine::api()->pushboolean( L, 0 );
        LsLuaEngine::api()->pushstring( L, "bad parameters" );
        LsLuaEngine::api()->pushboolean( L, 0 );
        return 3;
    } 
}

static int  LsLua_shared_safe_add( lua_State * L )
{
    return LsLua_shared_add( L );
}

static int  LsLua_shared_replace( lua_State * L )
{
    const char * tag = "ls.shared.add";
    char namebuf[0x100];
    int numElem = 3;
    register lsi_shmhash_t * pShared ;

    // LsLuaApi::dumpStack( L, "BEGIN LsLua_shared_replace", 10);
    pShared = lsLuaSharedObjectHelper( L, tag, numElem, namebuf, 0x100 );
    if (pShared)
    {
        lsLuaShmValue_t * pShmValue = lsLuaFindShmValue(pShared, namebuf);
        if ( ( pShmValue ) && (!isTimeExpired(pShmValue)) )
        {
            return LsLua_shared_set_helper( L, pShared, numElem, namebuf );
        }
        else
        {
            LsLuaEngine::api()->pushboolean( L, 0 );
            LsLuaEngine::api()->pushstring( L, "not found" );
            LsLuaEngine::api()->pushboolean( L, 0 );
            return 3;
        }
    }
    else
    {
        LsLuaEngine::api()->pushboolean( L, 0 );
        LsLuaEngine::api()->pushstring( L, "bad parameters" );
        LsLuaEngine::api()->pushboolean( L, 0 );
        return 3;
    } 
}

static int LsLua_shared_incr(lua_State * L)
{
    const char * tag = "ls.shared.incr";
    char namebuf[0x100];
    int numElem = 2;
    register lsi_shmhash_t * pShared ;

    // LsLuaApi::dumpStack( L, "BEGIN LsLua_shared_incr", 10);
    pShared = lsLuaSharedObjectHelper( L, tag, numElem, namebuf, 0x100 );
    if (pShared)
    {
        lsLuaShmValue_t * pShmValue = lsLuaFindShmValue(pShared, namebuf);
        if ( ( pShmValue ) && (!isTimeExpired(pShmValue)) )
        {
            if (pShmValue->m_type == long_type)
            {
                pShmValue->m_ulong++;
                LsLuaEngine::api()->pushinteger( L, pShmValue->m_ulong );
                LsLuaEngine::api()->pushnil( L );
            } 
            else if (pShmValue->m_type == double_type)
            {
                pShmValue->m_double += 1.0;
                LsLuaEngine::api()->pushnumber( L, pShmValue->m_double );
                LsLuaEngine::api()->pushnil( L );
            }
            else
            {
                LsLuaEngine::api()->pushnil( L );
                LsLuaEngine::api()->pushstring( L, "not a number" );
            }
            return 2;
        }
        else
        {
            LsLuaEngine::api()->pushnil( L );
            LsLuaEngine::api()->pushstring( L, "not found" );
            return 2;
        }
    }
    LsLuaEngine::api()->pushnil( L );
    LsLuaEngine::api()->pushstring( L, "bad parameters" );
    return 2;
}

static int LsLua_shared_delete(lua_State * L)
{
    // 1 = USERDATA 2 = key
    register int num = LsLuaEngine::api()->gettop(L);
    if (num > 2)
    {
        LsLuaApi::pop(L, num - 2);
    }
    LsLuaEngine::api()->pushnil( L );
    return LsLua_shared_set( L );
}

//
//  @brief LsLua_shared_flush_all 
//  @brief mark all lsi_shmhash_t * to expired status
//
static int LsLua_shared_flush_all(lua_State * L)
{
    // LsLuaApi::dumpStack( L, "BEGIN lsLua_shared_flush_all", 10);
    register lsi_shmhash_t *pShared = LUA_CHECKU_SHARED(L, "lsLua_shared_flush_all");
    
    if (!pShared)
    { 
        LsLuaEngine::api()->pushnil( L );
        LsLuaEngine::api()->pushstring( L, "bad parameters" );
        return 2;
    }
    //
    //  do the flush here
    //
    lsi_shmhash_scanraw(pShared
                    , LSLUA_SHMVALUE_MAGIC
                    , sizeof(lsLuaShmValue_t)
                    , LsLua_shared_cbfunc
                    , (void *)"flush_all");
                    
    return 0;
}

//
//  @brief LsLua_shared_flush_expired 
//  @brief delete all expired keys
//
static int LsLua_shared_flush_expired(lua_State * L)
{
    // LsLuaApi::dumpStack( L, "BEGIN lsLua_shared_flush_all", 10);
    register lsi_shmhash_t * pShared = LUA_CHECKU_SHARED(L, "lsLua_shared_flush_all");
    
    if (!pShared)
    { 
        LsLuaEngine::api()->pushinteger( L, 0 );
        return 1;
    }
    else
    {
        // NOTE 1-USERDATA 2-number
        int numOut = 0;
        numOut = LsLuaEngine::api()->tointegerx( L, 2, NULL);
#if 0
        if (LsLuaEngine::api()->type( L, 2 ) == LUA_TNUMBER)
            numOut = LsLuaEngine::api()->tointegerx( L, 2, NULL);
        else
            numOut = 0;
#endif
        if (numOut < 0)
            numOut = 0; // deleted all
            
        int n = lsi_shmhash_scanraw_checkremove(pShared
                    , LSLUA_SHMVALUE_MAGIC
                    , sizeof(lsLuaShmValue_t)
                    , LsLua_shared_cbfunc_check_expired_n_release
                    , (void *)&numOut);
                    
        //
        //  flush out all the expired keys here
        //
        // LsLua_log( L, LSI_LOG_NOTICE, 0, "%s %d ret %d", "LsLua_shared_flush_expired", numOut, n);
        LsLuaEngine::api()->pushinteger( L, n );
        return 1;
    }
}

//
//  The USERDATA Shared Object GET/SET
//

//
//  GET - RETRUN shared Memory pool
//
static int LsLua_shared_GET(lua_State * L)
{
    size_t len = 0;
    const char * cp;
    
    cp = LsLuaEngine::api()->tolstring( L, 2, &len);
    if (cp && len && (len < LSSHM_MAXNAMELEN ) )
    {
        char buf[0x100];
        snprintf(buf, 0x100, "%.*s", (int)len, cp);
        // LsLua_log( L, LSI_LOG_NOTICE, 0, "%s [%s] %d", "LsLua_shared_GET", buf, len);
        
        lsi_shmhash_t * hash = lsLuaOpenShm(buf);
        if (!hash)
        {
            return LsLuaApi::return_badparameterx( L, LSLUA_BAD_PARAMETERS );
        }
        return LsLua_shared_create ( L , hash );
    }
    return LsLuaApi::return_badparameterx( L, LSLUA_BAD_PARAMETERS );
}

//
//  Don't support this
//
static int LsLua_shared_SET(lua_State * L)
{
    const char *    tag = "LsLua_shared_SET";
    LsLuaApi::dumpStack( L, tag, 10);
    return 0;
}

//
//  LiteSpeed SHARE META
//
static const luaL_Reg shared_sub[] =
{
    {"get",             LsLua_shared_get},
    {"get_stale",       LsLua_shared_get_stale},
    {"set",             LsLua_shared_set},
    {"safe_set",        LsLua_shared_safe_set},
    {"add",             LsLua_shared_add},
    {"safe_add",        LsLua_shared_safe_add},
    {"replace",         LsLua_shared_replace},
    
    {"incr",            LsLua_shared_incr},
    {"delete",          LsLua_shared_delete},
    {"flush_all",       LsLua_shared_flush_all},
    {"flush_expired",   LsLua_shared_flush_expired},
    {NULL, NULL}
};

static const luaL_Reg shared_meta_sub[] =
{
    {"__gc",        LsLua_shared_gc},
    {"__tostring",  LsLua_shared_tostring},
    {NULL, NULL}
};

void LsLua_create_shared_meta( lua_State * L )
{
    //LsLuaApi::dumpStack( L, "BEGIN LsLua_create_shared_meta", 10 );
    LsLuaEngine::api()->openlib( L, LS_LUA ".shared", shared_sub, 0 );
    LsLuaEngine::api()->newmetatable( L, LSLUA_SHARED_DATA );
    LsLuaEngine::api()->openlib( L, NULL, shared_meta_sub, 0 );

    // pushliteral
    LsLuaEngine::api()->pushlstring( L, "__index", 7 );
    LsLuaEngine::api()->pushvalue( L, -3 );
    LsLuaEngine::api()->rawset( L, -3 );      // metatable.__index = methods
    // pushliteral
    LsLuaEngine::api()->pushlstring( L, "__metatable", 11 );
    LsLuaEngine::api()->pushvalue( L, -3 );
    LsLuaEngine::api()->rawset( L, -3 );      // metatable.__metatable = methods

    LsLuaEngine::api()->settop( L, -3 );     // pop 2

    //LsLuaApi::dumpStack( L, "END LsLua_create_shared_meta", 10 );
}

void LsLua_create_shared ( lua_State * L)
{
    /* should have the table on the stack already */
    //LsLuaApi::dumpStack( L, "BEGIN LsLua_create_shared", 10);
    
    LsLuaEngine::api()->createtable(L, 0, 0); /* create ls.header table         */
    LsLuaEngine::api()->createtable(L, 0, 2); /* create metatable for ls.header */
    LsLuaEngine::api()->pushcclosure(L, LsLua_shared_SET, 0);
    LsLuaEngine::api()->setfield(L, -2, "__newindex");
    LsLuaEngine::api()->pushcclosure(L, LsLua_shared_GET, 0);
    LsLuaEngine::api()->setfield(L, -2, "__index");
    LsLuaEngine::api()->setmetatable(L, -2);
    LsLuaEngine::api()->setfield(L, -2, "shared"); 
    
    //LsLuaApi::dumpStack( L, "END LsLua_create_shared", 10);
}


#endif // NO_LUA_TEST
