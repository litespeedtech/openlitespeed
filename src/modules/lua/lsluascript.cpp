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
#include "lsluascript.h"

#include <util/autobuf.h>

LsLuaScript::LsLuaScript()
    : m_pSource( NULL )
{

}


LsLuaScript::~LsLuaScript()
{
    if ( m_pSource )
        delete m_pSource;
}

void	LsLuaScript::setSource(const char * Source, int len)
{
	if (!m_pSource)
	{
		m_pSource = new AutoBuf(len+1);
	}
	else
	{
		m_pSource->clear();
	}
	m_pSource->append(Source, len);
	m_pSource->append("", 1);
}

