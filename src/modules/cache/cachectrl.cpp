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
#include "cachectrl.h"

#include <util/autostr.h>
#include <util/stringtool.h>

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

CacheCtrl::CacheCtrl()
    : m_flags( 0 )
    , m_iMaxAge( INT_MAX )
    , m_iMaxStale( 0 )
{
}


CacheCtrl::~CacheCtrl()
{
}

static const char * s_directives[12] = 
{
    "no-cache",
    "no-store",
    "max-age",
    "max-stale",
    "min-fresh",
    "no-transform",
    "only-if-cached",
    "public",
    "private",
    "must-revalidate",
    "proxy-revalidate",
    "s-maxage"
};

static const int s_dirLen[12] =
{   8, 8, 7, 9, 9, 12, 14, 6, 7, 15, 16, 8    };

void CacheCtrl::init(int flags, int iMaxAge, int iMaxStale)
{
    m_flags = flags;
    m_iMaxAge = iMaxAge;
    m_iMaxStale = iMaxStale;
}

int CacheCtrl::parse( const char * pHeader, int len)
{
    StrParse parser( pHeader, pHeader + len, "," );
    const char * p;
    while( !parser.isEnd() )
    {
        p = parser.trim_parse();
        if ( !p )
            break;
        if ( p != parser.getStrEnd() )
        {
            AutoStr2 s( p, parser.getStrEnd() - p );
            int i;
            for( i = 0; i < 12; ++i )
            {
                if ( strncasecmp( s.c_str(), s_directives[i], s_dirLen[i] ) == 0 )
                    break;
            }
            if ( i < 12 )
            {
                m_flags |= (1<<i);
                if ((( i == 2 )&&!( m_flags & (i << 11 )))||
                    (i == 11 )||(i == 3 ))
                {
                    p += s_dirLen[i];
                    while(( *p == ' ' )||(*p == '=' )||(*p=='"'))
                        ++p;
                    if ( isdigit( *p ) )
                    {
                        if ( i == 3 )
                            m_iMaxStale = atoi( p );
                        else
                        {
                            m_iMaxAge = atoi( p );
                            if (m_iMaxAge > 0)
                                m_flags |= cache_public;
                            else
                            {
                                m_flags &= ~cache_public;
                                m_flags &= ~cache_private;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
    
}


