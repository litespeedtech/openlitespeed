/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include "lsimoduledata.h"
#include <lsiapi/lsiapi.h>
#include <log4cxx/logger.h>

#include <string.h>

typedef void *void_pointer;

bool LsiModuleData::initData(int count)
{
    //For a reuse case, it was already init-ed, needn't to do again
    //Because m_iCount should always >= 2, we can check it here
    if (m_pData)
    {
        if (m_iCount > 0)
            return false;
        else
            delete m_pData;
    }

    m_pData = new void_pointer[count];
    if (!m_pData)
    {
        LS_ERROR("LsiModuleData::initData() error, seems out of memory.");
        return false;
    }

    memset(m_pData, 0, sizeof(void_pointer) * count);
    m_iCount = count;
    return true;
}
