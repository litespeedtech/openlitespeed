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
#include "authuser.h"
#include <util/stringlist.h>
#include <util/stringtool.h>

AuthUser::AuthUser()
    :m_pGroups( NULL )
{
}

AuthUser::~AuthUser()
{
    if ( m_pGroups )
        delete m_pGroups;
}

int AuthUser::setGroups( const char * pGroups, const char * pEnd )
{
    if ( !pGroups )
    {
        if ( m_pGroups )
        {
            delete m_pGroups;
            m_pGroups = NULL;
        }
        return 0;
    }
    else
    {
        if ( m_pGroups )
            m_pGroups->release_objects();
        else
        {
            m_pGroups = new StringList();
            if ( !m_pGroups )
                return -1;
        }
    }
    int size;
    if ( ( size = m_pGroups->split( pGroups, pEnd, "," )) == 0 )
    {
        delete m_pGroups;
        m_pGroups = NULL;
        return 0;
    }
    m_pGroups->sort();
    return size;
}

int AuthUser::addGroup( const char * pGroup )
{
    if ( !m_pGroups )
    {
        m_pGroups = new StringList();
        if ( !m_pGroups )
            return -1;
    }
    m_pGroups->add( pGroup );
    m_pGroups->sort();
    return 0;
        
}

int AuthGroup::add( const char * pUser )
{
    if ( StringList::add( pUser ) )
    {
        sort();
        return 0;
    }
    return -1;
}

