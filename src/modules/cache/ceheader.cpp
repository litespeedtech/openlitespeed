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
#include "ceheader.h"

CeHeader::CeHeader()
    : m_tmCreated(0)
    , m_tmExpire(0)
    , m_iFlag(CEH_IN_CONSTRUCT)
    , m_iKeyLen(0)
    , m_iStatusCode(0)
    , m_iValPart1Len(0)
    , m_iValPart2Len(0)
    , m_tmLastMod(0)
    , m_iOffETag(0)
    , m_iLenETag(0)
{
}


CeHeader::~CeHeader()
{
}


