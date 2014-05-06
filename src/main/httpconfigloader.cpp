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
#include "httpconfigloader.h"
#include "mainserverconfig.h"
#include "util/configctx.h"
#include "plainconf.h"

HttpConfigLoader::~HttpConfigLoader()
{
}

int HttpConfigLoader::loadPlainConfigFile()
{
    plainconf::initKeywords();
    plainconf::setRootPath( MainServerConfig::getInstance().getServerRoot() );

    if ( !m_pRoot )
        m_pRoot = plainconf::parseFile( m_sPlainconfPath.c_str() );

#ifdef TEST_OUTPUT_PLAIN_CONF
    char sPlainFile[512] = {0};
    strcpy( sPlainFile, m_sPlainconfPath.c_str() );
    strcat( sPlainFile, ".txt" );
    plainconf::testOutputConfigFile( m_pRoot, sPlainFile );
#endif
    return ( m_pRoot == NULL ) ? -1 : 0;
}

int HttpConfigLoader::loadConfigFile( const char *pConfigFilePath )
{
    if ( pConfigFilePath == NULL )
        pConfigFilePath = m_sConfigFilePath.c_str();

    if ( !m_pRoot )
        m_pRoot = ConfigCtx::getCurConfigCtx()->parseFile( pConfigFilePath, "httpServerConfig" );

    return ( m_pRoot == NULL ) ? -1 : 0;
}

void HttpConfigLoader::releaseConfigXmlTree()
{
    if ( m_pRoot )
    {
        delete m_pRoot;
        m_pRoot = NULL;
    }
}
