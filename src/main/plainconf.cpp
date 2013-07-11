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
/*
# "{" and "}" to quot a module, in a module, keys shouldn't be duplicate
#
# Comment begins with #  
#
# Backslash (\) can be used as the last character on a line to indicate to continues onto the next line 
# The next line will be added one space in the beginning
# Use "\" for a real \
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
        include         conf
        
        key3            v3

        submodule1 {
                skey1   svalue1 s2
                skey2   sv2
                
        }
}
*/

#include "plainconf.h"
#include "util/stringtool.h"
#include "util/gpointerlist.h"
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <util/stringlist.h>
#include <dirent.h>
#include <http/httplog.h>
#include <util/logtracker.h>
#include <string.h>
#include <http/httpreq.h>
#include <util/hashstringmap.h>
#include <http/httplog.h>

//All the keywords are lower case
static plainconfKeywords sKeywords[] = {

        {"accesscontrol",        "acc"},
        {"accessdenydir",        "acd"},
        {"accessfilename",        ""},
        {"accesslog",        ""},
        {"add",        ""},
        {"adddefaultcharset",        ""},
        {"address",        "addr"},
        {"adminemails",        ""},
        {"adminroot",        ""},
        {"aioblocksize",        ""},
        {"allow",        ""},
        {"allowbrowse",             ""},
        {"allowdirectaccess",        ""},
        {"allowedhosts",        ""},
        {"allowsymbollink",        ""},
        {"allowoverride",          ""},
        {"authname",        ""},
        {"authorizer",        ""},
        {"autofix503",        ""},
        {"autoindex",        ""},
        {"autoindexuri",            ""},
        {"autorestart",        "autoreboot"},
        {"autostart",        "autorun"},
        {"autoupdatedownloadpkg",        ""},
        {"autoupdateinterval",        ""},
        {"awstats",        ""},
        {"awstatsuri",        ""},
        {"backlog",        ""},
        {"banperiod",        ""},
        {"base",        ""},
        {"cachetimeout",        ""},
        {"cgirlimit",        ""},
        {"checksymbollink",        ""},
        {"chrootmode",      ""},
        {"chrootpath",        ""},
        {"compressarchive",        ""},
        {"compressibletypes",        ""},
        {"conntimeout",        ""},
        {"context",        ""},
        {"cpuhardlimit",        ""},
        {"cpusoftlimit",        ""},
        {"customerrorpages",        ""},
        {"debuglevel",        ""},
        {"defaultcharsetcustomized",        ""},
        {"deny",        ""},
        {"dir",        ""},
        {"disableadmin",        ""},
        {"docroot",        ""},
        {"dynreqpersec",        ""},
        {"enable",        ""},
        {"enablechroot",        ""},
        {"enablecontextac",        ""},
        {"enabledyngzipcompress",        ""},
        {"enableexpires",        ""},
        {"enablegzip",        ""},
        {"enablegzipcompress",        ""},
        {"enablehotlinkctrl",        ""},
        {"enablelve",        ""},
        {"enablescript",        ""},
        {"enablestderrlog",        ""},
        {"env",        ""},
        {"errcode",                 ""},
        {"errorlog",        ""},
        {"errorpage",               ""},
        {"eventdispatcher",        ""},
        {"expires",        ""},
        {"expiresbytype",        ""},
        {"expiresdefault",        ""},
        {"extmaxidletime",        ""},
        {"extprocessor",        ""},
        {"extraheaders",        ""},
        {"fileaccesscontrol",        ""},
        {"fileetag",        ""},
        {"filename",        ""},
        {"followsymbollink",        ""},
        {"forcestrictownership",        ""},
        {"general",        ""},
        {"gracefulrestarttimeout",        ""},
        {"graceperiod",        ""},
        {"group",        ""},
        {"groupdb",        ""},
        {"gzipautoupdatestatic",        ""},
        {"gzipcompresslevel",        ""},
        {"gzipmaxfilesize",        ""},
        {"gzipminfilesize",        ""},
        {"gzipstaticcompresslevel",        ""},
        {"hardlimit",        ""},
        {"htaccess",         ""},
        {"hotlinkctrl",        ""},
        {"inbandwidth",        ""},
        {"include",        ""},
        {"index",                   ""},
        {"indexfiles",        ""},
        {"inherit",        ""},
        {"inittimeout",        ""},
        {"inmembufsize",        ""},
        {"instances",        ""},
        {"keepalivetimeout",        ""},
        {"keepdays",        ""},
        {"listener",        ""},
        {"listeners",        ""},
        {"location",        ""},
        {"loglevel",        ""},
        {"logreferer",      ""},
        {"loguseragent",    ""},
        {"map",        ""},
        {"maxcachedfilesize",        ""},
        {"maxcachesize",        ""},
        {"maxcgiinstances",        ""},
        {"maxconnections",        ""},
        {"maxconns",        ""},
        {"maxdynrespheadersize",        ""},
        {"maxdynrespsize",        ""},
        {"maxkeepalivereq",        ""},
        {"maxmmapfilesize",        ""},
        {"maxreqbodysize",        ""},
        {"maxreqheadersize",        ""},
        {"maxrequrllen",        ""},
        {"maxsslconnections",        ""},
        {"memhardlimit",        ""},
        {"memsoftlimit",        ""},
        {"mime",        ""},
        {"mingid",        ""},
        {"minuid",        ""},
        {"name",        ""},
        {"note",        ""},
        {"onlyself",        ""},
        {"outbandwidth",        ""},
        {"path",        ""},
        {"pckeepalivetimeout",        ""},
        {"perclientconnlimit",        ""},
        {"persistconn",        ""},
        {"phpsuexec",        ""},
        {"phpsuexecmaxconn",        ""},
        {"priority",        ""},
        {"prochardlimit",        ""},
        {"procsoftlimit",        ""},
        {"railsdefaults",        ""},
        {"railsenv",        ""},
        {"rcvbufsize",        ""},
        {"realm",        ""},
        {"realmlist",        ""},
        {"required",        ""},
        {"requiredpermissionmask",        ""},
        {"respbuffer",        ""},
        {"restrained",        ""},
        {"restricteddirpermissionmask",        ""},
        {"restrictedpermissionmask",        ""},
        {"restrictedscriptpermissionmask",        ""},
        {"retrytimeout",        ""},
        {"rewrite",        ""},
        {"rewritebase",        ""},
        {"rewritecond",        ""},
        {"rewriteengine",        ""},
        {"rewriterule",        ""},
        {"rollingsize",        ""},
        {"rubybin",        ""},
        {"runonstartup",        ""},
        {"scripthandler",        ""},
        {"secure",        ""},
        {"security",        ""},
        {"servername",        ""},
        {"setuidmode",        ""},
        {"showversionnumber",        ""},
        {"sitealiases",        ""},
        {"sitedomain",        ""},
        {"smartkeepalive",        ""},
        {"sndbufsize",        ""},
        {"softlimit",        ""},
        {"sslcryptodevice",        ""},
        {"staticreqpersec",        ""},
        {"suffixes",        ""},
        {"suspendedvhosts",        ""},
        {"swappingdir",        ""},
        {"templatefile",        ""},
        {"totalinmemcachesize",        ""},
        {"totalmmapcachesize",        ""},
        {"tuning",        ""},
        {"type",        ""},
        {"updateinterval",        ""},
        {"updatemode",        ""},
        {"updateoffset",        ""},
        {"uri",        ""},
        {"url",                     ""},
        {"useaio",        ""},
        {"user",        ""},
        {"userdb",        ""},
        {"usesendfile",        ""},
        {"useserver",               ""},
        {"vhroot",        ""},
        {"vhtemplate",        ""},
        {"virtualhost",        ""},
        {"websocket",        ""},
        {"workingdir",        ""},

};

namespace plainconf {
    
    static GPointerList gModuleList;
    static HashStringMap<plainconfKeywords *> allKeyword(29, GHash::i_hash_string, GHash::i_comp_string);
    static bool bKeywordsInited = false;
    static AutoStr2 rootPath = "";
    
    void tolower(char *sLine)
    {
        int len = strlen(sLine);
        for (int i=0; i<len; ++i)
        {
            if (sLine[i] >= 'A' && sLine[i] <= 'Z')
                sLine[i] += ('a' - 'A');
        }
    }
    
    void initKeywords()
    {
        if (bKeywordsInited)
            return ;
        
        int count = sizeof(sKeywords) / sizeof(plainconfKeywords);
        for (int i=0; i<count; ++i) 
        {
            allKeyword.insert(sKeywords[i].name.c_str(), &sKeywords[i]);
            if (sKeywords[i].alias.len() > 0)
            {
                allKeyword.insert(sKeywords[i].alias.c_str(), &sKeywords[i]);
            }
        }
        
        bKeywordsInited = true;
    }
    
    void setRootPath(const char *root) 
    {
        rootPath = root;
    }
    
    const char *getRealName(char *name)
    {
        assert(bKeywordsInited == true);
        tolower(name);
        HashStringMap<plainconfKeywords *>::iterator it = allKeyword.find(name);
        
        if ( it == allKeyword.end())
            return NULL;
        else
            return it.second()->name.c_str();
    }
     
   
    //pos: 0, start position, 1: end position
    void removeSpace(char *sLine, int pos)
    {
        int len = strlen(sLine);
        int index, offset = -1;
        for (int i=0; i<len; ++i)
        {
            index = ((pos == 0) ? i : (len - 1 - i));
            if (sLine[index] != ' ' &&
                sLine[index] != '\t' && 
                sLine[index] != '\r' &&
                sLine[index] != '\n')
                break;
            else
                offset = i;                
        }
        if (pos == 0)
        {
            offset += 1; //index change to count
            memmove(sLine, sLine + offset, len - offset); 
            sLine[len - offset] = 0x00;
        }
        else
            sLine[len - offset - 1] = 0x00;
    }
    
    inline bool isChunkedLine(const char *sLine)
    {
        return (sLine[strlen(sLine) - 1] == '\\');
    }
    
    bool strcatchr(char *s, char c, int maxStrLen)
    {
        int len = strlen(s);
        if (len == maxStrLen)
            return false;
        
        s[len] = c;
        s[len + 1] = 0x00;
        return true;
    }

    void parseLine(const char *sLine)
    {
        const int MAX_NAME_LENGTH = 4096;
        char name[MAX_NAME_LENGTH] = {0};
        char value[MAX_NAME_LENGTH] = {0};
        const char *attr = NULL;
        
        XmlNode *pNode = NULL;
        XmlNode *pCurNode = (XmlNode *)gModuleList.back();
        const char *p = sLine;
        const char *pEnd = sLine + strlen(sLine);
        
        bool bNameSet = false;
       
        for(; p < pEnd; ++p)
        {
            //"{" is a beginning of a block only if it is the last char of a line
            if (*p == '{' && pEnd - p == 1)
            {
                if (strlen(name) > 0)
                {
                    const char *pRealname = getRealName(name);
                    if (pRealname) 
                    {
                        pNode = new XmlNode;
                        pNode->init(pRealname, &attr);
                        pCurNode->addChild(pNode->getName(), pNode);
                        gModuleList.push_back(pNode);
                        pCurNode = pNode;
                        strcpy(name, "");
                        strcpy(value, "");
                        break;
                    }
                    else
                    {
                        LOG_ERR(("Error, parseline find module name [%s] is NOT keyword.", name));
                        break;
                    }
                }
                else
                {
                    LOG_ERR(("Error, parseline find module name is NULL in [%s].", sLine));
                    break;
                }
            }
            
            else if (*p == '}' && p == sLine)
            {
                gModuleList.pop_back();
                strcpy(name, "");
                strcpy(value, "");
            }
            else if ((*p == ' ' || *p == '\t') && strlen(value) == 0)
            {
                bNameSet = true;
                continue;
            }
            else
            {
                if (!bNameSet)
                    strcatchr(name, *p, MAX_NAME_LENGTH);
                else
                    strcatchr(value, *p, MAX_NAME_LENGTH);
            }
        }
        
        if (strlen(name) > 0)
        {
            const char *pRealname = getRealName(name);
            if (pRealname) 
            {
                assert(pNode == NULL);
                pNode = new XmlNode;
                pNode->init(pRealname, &attr);
                if (strlen(value) > 0)
                    pNode->setValue(value, strlen(value));
                pCurNode->addChild(pNode->getName(), pNode);
            }
            else
                LOG_ERR(("Error, parseline find name [%s] is NOT keyword, the value is set to %s.", name, value));
        }
        
    }
       
    bool isValidline(const char * sLine)
    {
        int len = strlen(sLine);
        if (len == 0 || sLine[0] == '#')
            return false;

        const char invalidChars[] = "# \t\r\n\\";
        bool bValid = false;
        for ( int i = 0; i < len; ++i )
        {
            if (!strchr(invalidChars, sLine[i]))
            {
                bValid = true;
                break;
            }
        }
        
        return bValid;
    }
   
    //if true return true, and also set the path 
    bool isInclude(const char * sLine, AutoStr2 &path)
    {
        if (strncasecmp(sLine, "include",strlen("include")) == 0)
        {
            char *p = (char *)sLine;
            p += strlen("include");
            removeSpace(p, 0);
            path = p;
            return true;
        }
        
        return false;
    }
    
    //return 0: not exist, 1: file, 2: directory, 3: directory with wildchar
    int checkFiletype(const char *path)
    {
        struct stat sb;
        
        //Already has wildcahr, treat as directory
        if (strchr(path, '*') ||
            strchr(path, '?') ||
            (strchr(path, '[') && strchr(path, ']')))
        return 3;
        
        if (stat(path, &sb) == -1)
            return 0;
        
        if ((sb.st_mode & S_IFMT) == S_IFDIR)
            return 2;
        
        else
            return 1;
    }
    
    
    void LoadDirectory(const char *pPath, const char * pPattern)
    {
        DIR * pDir = opendir( pPath );
    
        if ( !pDir )
        {
            LOG_ERR(( "Failed to open directory [%s].", pPath ));
            return ;
        }
        
        char achPattern[1024] = "";
        if( pPattern )
            memccpy( achPattern, pPattern, 0, 1023 );
    
        struct dirent * dir_ent;
        StringList AllEntries;
        while( ( dir_ent = readdir( pDir ) ) )
        {
            const char *pName = dir_ent->d_name;
            if (( strcmp( pName, "." ) == 0 )||
                ( strcmp( pName, ".." ) == 0 )||
                ( *( pName + strlen( pName ) - 1) == '~' ) )
                continue;
            if ( achPattern[0] )
            {
                if ( fnmatch( achPattern, pName, FNM_PATHNAME ) )
                    continue;
            }
            
            char str[4096] = {0};
            strcpy(str, pPath);
            strcatchr(str, '/', 4096);
            strcat(str, pName);
            AllEntries.add(str);
        }
        closedir( pDir );
        
        //AllEntries.sort();
        StringList::iterator iter;
        for( iter = AllEntries.begin(); iter != AllEntries.end(); ++iter )
        {
            const char *p = (*iter)->c_str();
            LOG_INFO(( "Processing config file: %s", p ));
            LoadConfFile(p);
        }
    }
    
    void getIncludeFile(const char *orgFile, char *targetFile)
    {
        int len = strlen(orgFile);
        if (len == 0)
            return;

        //Absolute path
        if (orgFile[0] == '/')
            strcpy(targetFile, orgFile);
            
        if (orgFile[0] == '$')
        {
            if (strncasecmp(orgFile, "$server_root/", 13) == 0)
            {
                strcpy(targetFile, rootPath.c_str());
                strcat(targetFile, orgFile + 13);
            }
            else
                LOG_ERR(( "Can't resolve include file %s\n", orgFile));
        }
        
        else
        {
            strcpy(targetFile, rootPath.c_str());
            strcat(targetFile, orgFile);
        }
 
         if ( access( targetFile, R_OK ) != 0 )
             strcpy(targetFile, "");
         
    }
    
    //This function may be recruse called
    void LoadConfFile(const char *path)
    {
        int type = checkFiletype(path);
        if (type == 0)
            return;
        
        else if (type == 2)
            LoadDirectory(path, NULL);
        
        else if (type == 3)
        {
            AutoStr2 prefixPath = path;
            const char * p = strrchr(path, '/');
            if (p)
                prefixPath.setStr(path, p - path);
            
            struct stat sb;
            //removed the wildchar filename, should be a directory if exist
            if (stat(prefixPath.c_str(), &sb) == -1)
            {
                LOG_ERR(( "LoadConfFile error 1: fullpath:%s directory:%s\n",
                         path, prefixPath.c_str() ));
                return ;
            }
            
            if ((sb.st_mode & S_IFMT) != S_IFDIR)
            {
                LOG_ERR(( "LoadConfFile error 2: fullpath:%s directory:%s\n",
                          path, prefixPath.c_str() ));
                return ;
            }
           
            LoadDirectory(prefixPath.c_str(), p + 1);
        }
        
        else //existed file
        {
            //gModuleList.push_back();
            //XmlNode *xmlNode = new XmlNode;
            FILE* fp = fopen(path, "r");
            if ( fp == NULL )
            {
                LOG_ERR(( "Cannot open plain conf file: %s\n", path ));
                return;
            }
            
            const int MAX_LINE_LENGTH = 8192;
            char sLine[MAX_LINE_LENGTH];
            char * p;
            char sLines[MAX_LINE_LENGTH] = {0};

            while(fgets(sLine, MAX_LINE_LENGTH, fp), !feof(fp))
            {
                p = sLine;
                removeSpace(p, 0);
                removeSpace(p, 1);
                
                if (!isValidline(p))
                    continue;
                
                AutoStr2 pathInclude;
                if (isInclude(p, pathInclude))
                {
                    char achBuf[512] = {0};
                    getIncludeFile(pathInclude.c_str(), achBuf);
                    LoadConfFile(achBuf);
                }
                else 
                {
                    //need to cotinue
                    if (isChunkedLine(p))
                    {
                        strncat(sLines, p, strlen(p) - 1);
                        strcatchr(sLines, ' ', MAX_LINE_LENGTH); //add a space at the end of the line which has a '\\'
                    }
                    
                    else
                    {
                        strcat(sLines, p);
                        parseLine(sLines);
                        strcpy(sLines, "");
                    }
                }
            }
            fclose(fp);
        }
    }
    
    //return the root node of the tree
    XmlNode* parseFile(const char* configFilePath)
    {
        XmlNode *rootNode = new XmlNode;
        const char *attr = NULL;
        rootNode->init("httpServerConfig", &attr);
        gModuleList.push_back(rootNode);
        
        LoadConfFile(configFilePath);
        gModuleList.pop_back();
        
        return rootNode;
    }
    
    void release(XmlNode *pNode)
    {
        XmlNodeList list;
        int count = pNode->getAllChildren(list);
        if (count > 0)
        {
            XmlNodeList::const_iterator iter;
            for( iter = list.begin(); iter != list.end(); ++iter )
                release(*iter);
        }
        if (!pNode->getParent())
            delete pNode;
    }
    //name: form like "moduleName|submodlue|itemname"
    const char *getConfDeepValue(const XmlNode *pNode, const char *name)
    {
        const char *p = strchr(name, '|');
        if (!p)
            return pNode->getChildValue( name );
        else
        {
            AutoStr2 subName;
            subName.setStr(name, p - name);
            const XmlNode *subNode = pNode->getChild(subName.c_str());
            if (subNode)
                return getConfDeepValue(subNode, p + 1);
            else
                return NULL;
        }
    }

    //The below functions are only for test.
    inline void outputSpaces(int level, FILE *fp)
    {
        for (int i=0; i<level; ++i)
            fprintf(fp, "  ");
    }
        
    void outputConfigFile(const XmlNode *pNode, FILE *fp, int level)
    {
        XmlNodeList list;
        int count = pNode->getAllChildren(list);
        if (count > 0)
        {
            outputSpaces(level, fp);
            fprintf(fp, "%s {\n", pNode->getName());
            
            XmlNodeList::const_iterator iter;
            for( iter = list.begin(); iter != list.end(); ++iter )
                outputConfigFile((*iter), fp, level + 1);
            
            outputSpaces(level, fp);
            fprintf(fp, "}\n");
        }
        else
        {
            outputSpaces(level, fp);
            fprintf(fp, "%s %s\n", pNode->getName(), pNode->getValue());
        }
    }
    
    void testOutputConfigFile(const XmlNode *pNode, const char *file)
    {
        FILE *fp = fopen(file, "w");
        int level = 0;
        outputConfigFile(pNode, fp, level);
        fclose(fp);
    }       
    
}
