/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2014  LiteSpeed Technologies, Inc.                        *
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
#ifndef CACHECONFIG_H
#define CACHECONFIG_H

#include "util/autostr.h"

#define CACHE_ENABLED                       (1<<0)
//#define CACHE_POST_NOCACHE                  (1<<1)
#define CACHE_QS_CACHE                      (1<<2)
#define CACHE_REQ_COOKIE_CACHE              (1<<3)
#define CACHE_IGNORE_REQ_CACHE_CTRL_HEADER  (1<<4)
#define CACHE_IGNORE_RESP_CACHE_CTRL_HEADER (1<<5)
//#define CACHE_IGNORE_SURROGATE_HEADER       (1<<6)
#define CACHE_RESP_COOKIE_CACHE             (1<<7)

#define CACHE_MAX_AGE_SET                   (1<<8)
#define CACHE_PRIVATE_ENABLED               (1<<9)
#define CACHE_PRIVATE_AGE_SET               (1<<10)
#define CACHE_STALE_AGE_SET                 (1<<11)

class CacheConfig
{
public:

    CacheConfig();

    ~CacheConfig();
    
    void inherit( const CacheConfig * pParent );
    void setConfigBit( int bit, int enable )
    {   m_iCacheConfigBits |= bit;
        m_iCacheFlag = (m_iCacheFlag & ~bit) | ((enable)?bit:0); }
    
    void setDefaultAge( int age )       {   m_defaultAge = age;     }
    int  getDefaultAge() const          {   return m_defaultAge;    }
    
    void setPrivateAge( int age )       {   m_privateAge = age;     }
    int  getPrivateAge() const          {   return m_privateAge;    }

    void setMaxStale( int age )         {   m_iMaxStale = age;       }
    int  getMaxStale() const            {   return m_iMaxStale;      }
    
    void setStoragePath( const char * s, int len){   m_sStoragePath.setStr(s, len);    }
    const char* getStoragePath() const  {   return m_sStoragePath.c_str(); }

    int  isPrivateEnabled() const   {   return m_iCacheFlag & CACHE_PRIVATE_ENABLED;    }
    int  isPublicPrivateEnabled() const 
    {   return m_iCacheFlag & ( CACHE_PRIVATE_ENABLED | CACHE_ENABLED ); }
    int  isEnabled() const          {   return m_iCacheFlag & CACHE_ENABLED; }
    int  isSet( int bit ) const     {   return m_iCacheFlag & bit;     }
    int  getConfigBits( int bit) const  {   return m_iCacheConfigBits & bit; }
    void apply( const CacheConfig * pParent );

private:

    short   m_iCacheConfigBits;
    short   m_iCacheFlag;
    int     m_defaultAge;
    int     m_privateAge;
    int     m_iMaxStale;
    AutoStr2 m_sStoragePath;
};

#endif
