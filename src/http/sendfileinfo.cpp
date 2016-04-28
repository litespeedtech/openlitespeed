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
#include "sendfileinfo.h"
#include <http/staticfilecachedata.h>

#include <assert.h>


SendFileInfo::SendFileInfo()
    : m_pFileData(NULL)
    , m_pECache(NULL)
    , m_pAioBuf(NULL)
    , m_lCurPos(0)
    , m_lCurEnd(0)
    , m_lAioLen(0)
{
}

SendFileInfo::~SendFileInfo()
{}

int SendFileInfo::getfd()
{
    if (!m_pECache)
        return (long)m_pParam;
    else
        return m_pECache->getfd();
}
