/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2021  LiteSpeed Technologies, Inc.                 *
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

#include <lsr/ls_base64.h>

AuthUser::AuthUser()
    : m_pGroups(NULL)
{
}


AuthUser::~AuthUser()
{
    if (m_pGroups)
        delete m_pGroups;
}


int AuthUser::setGroups(const char *pGroups, const char *pEnd)
{
    if (!pGroups)
    {
        if (m_pGroups)
        {
            delete m_pGroups;
            m_pGroups = NULL;
        }
        return 0;
    }
    else
    {
        if (m_pGroups)
            m_pGroups->release_objects();
        else
        {
            m_pGroups = new StringList();
            if (!m_pGroups)
                return LS_FAIL;
        }
    }
    int size;
    if ((size = m_pGroups->split(pGroups, pEnd, ",")) == 0)
    {
        delete m_pGroups;
        m_pGroups = NULL;
        return 0;
    }
    m_pGroups->sort();
    return size;
}


int AuthUser::addGroup(const char *pGroup)
{
    if (!m_pGroups)
    {
        m_pGroups = new StringList();
        if (!m_pGroups)
            return LS_FAIL;
    }
    m_pGroups->add(pGroup);
    m_pGroups->sort();
    return 0;

}


void AuthUser::updatePasswdEncMethod()
{
    const char *pass = m_passwd.c_str();
    if (strncmp(pass, "$apr1$", 6) == 0)
        setEncMethod(ENCRYPT_APMD5);
    else if (strncasecmp(pass, "{sha}", 5) == 0)
    {
        char buf[128];
        const char *p = pass + 5;
        int len = ls_base64_decode(p, strlen(p), buf);
        if (len == 20)
        {
            setPasswd(buf, len);
            setEncMethod(ENCRYPT_SHA);
        }
    }
    else if (pass[0] == '$' && pass[1] == '2' && pass[3] == '$'
             && (pass[2] == 'y' || pass[2] == 'a'))
        setEncMethod(ENCRYPT_BCRYPT);

}


int AuthGroup::add(const char *pUser)
{
    if (StringList::add(pUser))
    {
        sort();
        return 0;
    }
    return LS_FAIL;
}

