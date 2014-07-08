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
#ifndef __PLCONF__
#define __PLCONF__
/*
# "{" and "}" to quot a module, in a module, keys shouldn't be duplicate
#
# Comment begins with #  
#
# Backslash "\" can be used as the last character on a line to indicate to continues onto the next line 
# There must be no other characters or space between the backslash and the end of the line
# The next line will be added one space in the beginning
# Use "\" for a real "\"
#
# Space(s) or tab(s) in the beginning of a value will be removed, use " " instead of space or tab
#
# "include" can be used to include files or directories, wild character can be used here

include mime.types

module {
        key1    value1 value1...
        key2    value2 is a multlple lines value with space in beginning of the 2nd line.\
                    "  "This is  the 2nd line of value2 and end with backslash"\"

        include         1.conf
        include         /conf
        
        key3            v3

        submodule1 {
                skey1   svalue1 s2
                skey2   sv2
                
        }
}
*/



/*Usage of this module
 * parseFile() to parse a whole file includes "include" and return the root node of the config tree
 * release() to release all the resources of the config tree
 * getConfDeepValue() to get the value of a multi level branch, such as "security|fileAccessControl|checkSymbolLink"
 * use XmlNode::getChild to get the XmlNode pointer of a node
 * use the XmlNodeList * XmlNode::getAllChildren to get a list of a certen node, and 
 *      use the iterator to check all the value in this list
*/


#include <util/xmlnode.h>
#include "util/autostr.h"
//#define TEST_OUTPUT_PLAIN_CONF

struct plainconfKeywords
{
    const char* name;
    const char* alias;
};

namespace plainconf {
    
    void initKeywords();
    void setRootPath(const char *root);
    const char *getRealName(char *name);
    
    
    //internnal calling functions
    void LoadConfFile(const char *path);
    
    
    XmlNode* parseFile(const char* configFilePath);
    void release(XmlNode *pNode);
    const char *getConfDeepValue(const XmlNode *pNode, const char *name);
    
    
    //testing functions
    void testOutputConfigFile(const XmlNode *pNode, const char *file);
    
    void flushErrorLog();
    
}

#endif  //__PLCONF__

