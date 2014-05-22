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
#include "cacheconfig.h"

CacheConfig::CacheConfig()
    : m_iCacheConfigBits(0)
    , m_iCacheFlag(0)
    , m_defaultAge(86400)
    , m_privateAge(60)
    , m_iMaxStale( 0 )
    , m_sStoragePath("")
{
}


CacheConfig::~CacheConfig()
{
}

void CacheConfig::inherit( const CacheConfig * pParent )
{
    if ( pParent )
    {
        if (!( m_iCacheConfigBits & CACHE_MAX_AGE_SET ))
            m_defaultAge = pParent->m_defaultAge;
        if (!( m_iCacheConfigBits & CACHE_PRIVATE_AGE_SET ))
            m_privateAge = pParent->m_privateAge;
        if (!( m_iCacheConfigBits & CACHE_STALE_AGE_SET ))
            m_iMaxStale = pParent->m_iMaxStale;
        m_iCacheFlag = (m_iCacheFlag & m_iCacheConfigBits)|
                  (pParent->m_iCacheFlag & ~m_iCacheConfigBits );
        m_sStoragePath.setStr(pParent->getStoragePath());
    }

}

void CacheConfig::apply( const CacheConfig * pParent )
{
    if ( pParent )
    {
        if (  pParent->m_iCacheConfigBits & CACHE_MAX_AGE_SET )
            m_defaultAge = pParent->m_defaultAge;
        if ( pParent->m_iCacheConfigBits & CACHE_PRIVATE_AGE_SET )
            m_privateAge = pParent->m_privateAge;
        if ( pParent->m_iCacheConfigBits & CACHE_STALE_AGE_SET )
            m_iMaxStale = pParent->m_iMaxStale;

        m_iCacheFlag = (pParent->m_iCacheFlag & pParent->m_iCacheConfigBits)|
                  (m_iCacheFlag & ~pParent->m_iCacheConfigBits );  
    }

}


