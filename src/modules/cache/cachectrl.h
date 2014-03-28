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
#ifndef CACHECTRL_H
#define CACHECTRL_H

/**
	@author Gang Wang <gwang@litespeedtech.com>
*/
class CacheCtrl
{
public:
    CacheCtrl();

    ~CacheCtrl();

    enum 
    {
        no_cache = (1<<0),
        no_store = (1<<1),
        max_age = (1<<2),
        max_stale = (1<<3),
        min_fresh = (1<<4),
        no_transform = (1<<5),
        only_if_cached = (1<<6),
        cache_public = (1<<7),
        cache_private = (1<<8),
        must_revalidate = (1<<9),
        proxy_revalidate = (1<<10),
        s_maxage = (1<<11),
        has_cookie = ( 1<<12 )
    };

    int isCacheOff() const  {   return (m_flags & (no_cache | no_store )) || (m_iMaxAge == 0);    }
    int isPublicCacheable() const {   return !( m_flags & (no_cache | no_store | cache_private |has_cookie ) );   }
    int isPrivateCacheable() const  {   return m_flags & cache_private;   }
    void setHasCookie()     {   m_flags |= has_cookie;  }
    int getFlags() const    {   return m_flags;     }
    int getMaxAge() const   {   return m_iMaxAge;   }
    void init(int flags, int iMaxAge, int iMaxStale);
    int parse(const char * pHeader, int len);
    int isMaxAgeSet() const {   return m_flags & (max_age | s_maxage);  }
    void setMaxAge(int age) {   m_iMaxAge = age;    }
    int isMaxStaleSet() const   {   return m_flags & max_stale;     }
    int getMaxStale() const     {   return m_iMaxStale;     }
    void setMaxStale( int age ) {   m_iMaxStale = age;      }

private:
    int m_flags;
    int m_iMaxAge;
    int m_iMaxStale;


};

#endif
