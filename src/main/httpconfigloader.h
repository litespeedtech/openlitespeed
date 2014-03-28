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
#ifndef HTTPCONFIGLOADER_H
#define HTTPCONFIGLOADER_H

#include <util/logidtracker.h>

#include <limits.h>
class XmlNode;


class HttpConfigLoader {
private:
    XmlNode       * m_pRoot;

    AutoStr         m_sConfigFilePath;
    AutoStr         m_sPlainconfPath;
   
public:
    HttpConfigLoader( )
        : m_pRoot(NULL)
        , m_sPlainconfPath ( "" )
        {};

    ~HttpConfigLoader();
    void releaseConfigXmlTree();
    int loadConfigFile( const char * pConfigFile = NULL);
    int loadPlainConfigFile();
    void setConfigFilePath(const char * pConfig)
    {   m_sConfigFilePath = pConfig;      }
    void setPlainConfigFilePath(const char * pConfig)
    {   m_sPlainconfPath = pConfig;      }
    XmlNode* getRoot()                  {   return m_pRoot;         }
};

#endif
