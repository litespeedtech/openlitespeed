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
/**
 * FIXME
 * For reference of the usage of plain text configuration, please review URL 
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
#include <util/logtracker.h>
#include <string.h>
#include <http/httpreq.h>
#include <util/hashstringmap.h>
#include <stdarg.h>
#include <http/httplog.h>

//Special case
//:: means module and module name

#define SERVER_ROOT_XML_NAME    "httpServerConfig"
#define UNKNOWN_KEYWORDS        "unknownkeywords"
//All the keywords are lower case
static plainconfKeywords sKeywords[] = 
{
    {"a_ext_fcgi",                               NULL},
    {"a_ext_fcgiauth",                           NULL},
    {"a_ext_loadbalancer",                       NULL},
    {"a_ext_logger",                             NULL},
    {"a_ext_lsapi",                              NULL},
    {"a_ext_proxy",                              NULL},
    {"a_ext_servlet",                            NULL},
    {"accesscontrol",                                   "acc"},
    {"accessdenydir",                                   "acd"},
    {"accessfilename",                           NULL},
    {"accesslog",                                NULL},
    {"add",                                      NULL},
    {"adddefaultcharset",                        NULL},
    {"addmimetype",                              NULL},
    {"address",                                                 "addr"},
    {"adminconfig",                              NULL},
    {"adminemails",                              NULL},
    {"adminroot",                                NULL},
    {"aioblocksize",                             NULL},
    {"allow",                                    NULL},
    {"allowbrowse",                              NULL},
    {"allowdirectaccess",                        NULL},
    {"allowedhosts",                             NULL},
    {"allowoverride",                            NULL},
    {"allowsetuid",                              NULL},
    {"allowsymbollink",                          NULL},
    {"attrgroupmember",                          NULL},
    {"attrmemberof",                             NULL},
    {"attrpasswd",                               NULL},
    {"authname",                                 NULL},
    {"authorizer",                               NULL},
    {"autofix503",                               NULL},
    {"autoindex",                                NULL},
    {"autoindexuri",                             NULL},
    {"autorestart",                              NULL},
    {"autostart",                                NULL},
    {"autoupdatedownloadpkg",                    NULL},
    {"autoupdateinterval",                       NULL},
    {"awstats",                                  NULL},
    {"awstatsuri",                               NULL},
    {"backlog",                                  NULL},
    {"banperiod",                                NULL},
    {"base",                                     NULL},
    {"binding",                                  NULL},
    {"blockbadreq",                              NULL},
    {"byteslog",                                 NULL},
    {"cacertfile",                               NULL},
    {"cacertpath",                               NULL},
    {"cachetimeout",                             NULL},
    {"certchain",                                NULL},
    {"certfile",                                 NULL},
    {"cgidsock",                                 NULL},
    {"cgirlimit",                                NULL},
    {"checksymbollink",                          NULL},
    {"chrootmode",                               NULL},
    {"chrootpath",                               NULL},
    {"ciphers",                                  NULL},
    {"clientverify",                             NULL},
    {"compressarchive",                          NULL},
    {"compressibletypes",                        NULL},
    {"configfile",                               NULL},
    {"conntimeout",                              NULL},
    {"context",                                  NULL},
    {"contextlist",                              NULL},
    {"cpuhardlimit",                             NULL},
    {"cpusoftlimit",                             NULL},
    {"crlfile",                                  NULL},
    {"crlpath",                                  NULL},
    {"customerrorpages",                         NULL},
    {"debuglevel",                               NULL},
    {"defaultcharsetcustomized",                 NULL},
    {"defaulttype",                              NULL},
    {"deny",                                     NULL},
    {"dhparam",                                  NULL},
    {"dir",                                      NULL},
    {"disableadmin",                             NULL},
    {"disableinitlogrotation",                   NULL},
    {"docroot",                                  NULL},
    {"domain",                                   NULL},
    {"dynreqpersec",                             NULL},
    {"enable",                                   NULL},
    {"enablechroot",                             NULL},
    {"enablecontextac",                          NULL},
    {"enablecoredump",                           NULL},
    {"enabled",                                  NULL},
    {"enabledhe",                                NULL},
    {"enabledyngzipcompress",                    NULL},
    {"enableecdhe",                              NULL},
    {"enableexpires",                            NULL},
    {"enablegzip",                               NULL},
    {"enablegzipcompress",                       NULL},
    {"enablehotlinkctrl",                        NULL},
    {"enableipgeo",                              NULL},
    {"enablelve",                                NULL},
    {"enablescript",                             NULL},
    {"enablespdy",                               NULL},
    {"enablestapling",                           NULL},
    {"enablestderrlog",                          NULL},
    {"env",                                      NULL},
    {"errcode",                                  NULL},
    {"errorlog",                                 NULL},
    {"errorpage",                                NULL},
    {"eventdispatcher",                          NULL},
    {"expires",                                  NULL},
    {"expiresbytype",                            NULL},
    {"expiresdefault",                           NULL},
    {"externalredirect",                         NULL},
    {"extgroup",                                 NULL},
    {"extmaxidletime",                           NULL},
    {"extprocessor",                             NULL},
    {"extprocessorlist",                         NULL},
    {"extraheaders",                             NULL},
    {"extuser",                                  NULL},
    {"fileaccesscontrol",                        NULL},
    {"fileetag",                                 NULL},
    {"filename",                                 NULL},
    {"followsymbollink",                         NULL},
    {"forcegid",                                 NULL},
    {"forcestrictownership",                     NULL},
    {"forcetype",                                NULL},
    {"general",                                  NULL},
    {"geoipdb",                                  NULL},
    {"geoipdbcache",                             NULL},
    {"geoipdbfile",                              NULL},
    {"gracefulrestarttimeout",                   NULL},
    {"graceperiod",                              NULL},
    {"group",                                    NULL},
    {"groupdb",                                  NULL},
    {"gzipautoupdatestatic",                     NULL},
    {"gzipcachedir",                             NULL},
    {"gzipcompresslevel",                        NULL},
    {"gzipmaxfilesize",                          NULL},
    {"gzipminfilesize",                          NULL},
    {"gzipstaticcompresslevel",                  NULL},
    {"handler",                                  NULL},
    {"hardlimit",                                NULL},
    {"hookpriority",                             NULL},
    {"hotlinkctrl",                              NULL},
    {"htaccess",                                 NULL},
    {"http_begin",                               NULL},
    {"http_end",                                 NULL},
    {"httpdworkers",                             NULL},
    {"httpserverconfig",                         NULL},
    {"inbandwidth",                              NULL},
    {"include",                                  NULL},
    {"index",                                    NULL},
    {"indexfiles",                               NULL},
    {"inherit",                                  NULL},
    {"inittimeout",                              NULL},
    {"inmembufsize",                             NULL},
    {"instances",                                NULL},
    {"iptogeo",                                  NULL},
    {"keepalivetimeout",                         NULL},
    {"keepdays",                                 NULL},
    {"keyfile",                                  NULL},
    {"l4_begsession",                            NULL},
    {"l4_endsession",                            NULL},
    {"l4_recving",                               NULL},
    {"l4_sending",                               NULL},
    {"ldapbinddn",                               NULL},
    {"ldapbindpasswd",                           NULL},
    {"listener",                                 NULL},
    {"listenerlist",                             NULL},
    {"listeners",                                NULL},
    {"location",                                 NULL},
    {"log",                                      NULL},
    {"logformat",                                NULL},
    {"logging",                                  NULL},
    {"logheaders",                               NULL},
    {"loglevel",                                 NULL},
    {"logreferer",                               NULL},
    {"loguseragent",                             NULL},
    {"map",                                      NULL},
    {"maxcachedfilesize",                        NULL},
    {"maxcachesize",                             NULL},
    {"maxcgiinstances",                          NULL},
    {"maxconnections",                           NULL},
    {"maxconns",                                 NULL},
    {"maxdynrespheadersize",                     NULL},
    {"maxdynrespsize",                           NULL},
    {"maxkeepalivereq",                          NULL},
    {"maxmmapfilesize",                          NULL},
    {"maxreqbodysize",                           NULL},
    {"maxreqheadersize",                         NULL},
    {"maxrequrllen",                             NULL},
    {"maxsslconnections",                        NULL},
    {"member",                                   NULL},
    {"memhardlimit",                             NULL},
    {"memsoftlimit",                             NULL},
    {"mime",                                     NULL},
    {"mingid",                                   NULL},
    {"minuid",                                   NULL},
    {"module",                                   NULL},
    {"modulelist",                               NULL},
    {"name",                                     NULL},
    {"note",                                     NULL},
    {"ocspcacerts",                              NULL},
    {"ocsprespmaxage",                           NULL},
    {"ocspresponder",                            NULL},
    {"onlyself",                                 NULL},
    {"outbandwidth",                             NULL},
    {"param",                                    NULL},
    {"path",                                     NULL},
    {"pckeepalivetimeout",                       NULL},
    {"perclientconnlimit",                       NULL},
    {"persistconn",                              NULL},
    {"phpsuexec",                                NULL},
    {"phpsuexecmaxconn",                         NULL},
    {"pipedlogger",                              NULL},
    {"priority",                                 NULL},
    {"prochardlimit",                            NULL},
    {"procsoftlimit",                            NULL},
    {"railsdefaults",                            NULL},
    {"railsenv",                                 NULL},
    {"rcvbufsize",                               NULL},
    {"realm",                                    NULL},
    {"realmlist",                                NULL},
    {"recv_req_bdy",                             NULL},
    {"recv_req_hdr",                             NULL},
    {"recv_rsp_bdy",                             NULL},
    {"recv_rsp_hdr",                             NULL},
    {"recved_req_bdy",                           NULL},
    {"recved_rsp_bdy",                           NULL},
    {"renegprotection",                          NULL},
    {"required",                                 NULL},
    {"requiredpermissionmask",                   NULL},
    {"respbuffer",                               NULL},
    {"restrained",                               NULL},
    {"restricteddirpermissionmask",              NULL},
    {"restrictedpermissionmask",                 NULL},
    {"restrictedscriptpermissionmask",           NULL},
    {"retrytimeout",                             NULL},
    {"rewrite",                                  NULL},
    {"rewritebase",                              NULL},
    {"rewritecond",                              NULL},
    {"rewriteengine",                            NULL},
    {"rewriterule",                              NULL},
    {"rollingsize",                              NULL},
    {"rubybin",                                  NULL},
    {"rules",                                    NULL},
    {"runonstartup",                             NULL},
    {"scripthandler",                            NULL},
    {"scripthandlerlist",                        NULL},
    {"secure",                                   NULL},
    {"security",                                 NULL},
    {"send_rsp_bdy",                             NULL},
    {"send_rsp_hdr",                             NULL},
    {"servername",                               NULL},
    {"sessiontimeout",                           NULL},
    {"setuidmode",                               NULL},
    {"showversionnumber",                        NULL},
    {"sitealiases",                              NULL},
    {"sitedomain",                               NULL},
    {"smartkeepalive",                           NULL},
    {"sndbufsize",                               NULL},
    {"softlimit",                                NULL},
    {"spdyadheader",                             NULL},
    {"sslcryptodevice",                          NULL},
    {"sslprotocol",                              NULL},
    {"staticreqpersec",                          NULL},
    {"statuscode",                               NULL},
    {"suffix",                                   NULL},
    {"suffixes",                                 NULL},
    {"suspendedvhosts",                          NULL},
    {"swappingdir",                              NULL},
    {"templatefile",                             NULL},
    {"totalinmemcachesize",                      NULL},
    {"totalmmapcachesize",                       NULL},
    {"tp_ext_fcgi",                              NULL},
    {"tp_ext_fcgiauth",                          NULL},
    {"tp_ext_loadbalancer",                      NULL},
    {"tp_ext_logger",                            NULL},
    {"tp_ext_lsapi",                             NULL},
    {"tp_ext_proxy",                             NULL},
    {"tp_ext_servlet",                           NULL},
    {"tp_realm_file",                            NULL},
    {"tuning",                                   NULL},
    {"type",                                     NULL},
    {"umask",                                    NULL},
    {"updateinterval",                           NULL},
    {"updatemode",                               NULL},
    {"updateoffset",                             NULL},
    {"uri",                                      NULL},
    {"uri_map",                                  NULL},
    {"url",                                      NULL},
    {"urlfilter",                                NULL},
    {"urlfilterlist",                            NULL},
    {"useaio",                                   NULL},
    {"useipinproxyheader",                       NULL},
    {"user",                                     NULL},
    {"userdb",                                   NULL},
    {"usesendfile",                              NULL},
    {"useserver",                                NULL},
    {"verifydepth",                              NULL},
    {"vhaliases",                                NULL},
    {"vhdomain",                                 NULL},
    {"vhname",                                   NULL},
    {"vhost",                                    NULL},
    {"vhostmap",                                 NULL},
    {"vhostmaplist",                             NULL},
    {"vhroot",                                   NULL},
    {"vhssl",                                    NULL},
    {"vhtemplate",                               NULL},
    {"vhtemplatelist",                           NULL},
    {"virtualhost",                              NULL},
    {"virtualhostconfig",                        NULL},
    {"virtualhostlist",                              NULL},
    {"virtualhosttemplate",                      NULL},
    {"websocket",                                NULL},
    {"websocketlist",                                NULL},
    {"workers",                                      NULL},
    {"workingdir",                               NULL},

};

namespace plainconf {
    enum
    {
        LOG_LEVEL_ERR = 'E',
        LOG_LEVEL_INFO = 'I',
    };
    static GPointerList gModuleList;
    static HashStringMap<plainconfKeywords *> allKeyword(29, GHash::i_hash_string, GHash::i_comp_string);
    static bool bKeywordsInited = false;
    static AutoStr2 rootPath = "";
    StringList errorLogList;
    
    static void logToMem( char errorLevel, const char *format, ...)
    {
        char buf[512];
        sprintf(buf, "%c[PlainConf] ", errorLevel);
        int len = strlen(buf);
        if (gModuleList.size() > 0)
        {
            XmlNode *pCurNode = (XmlNode *)gModuleList.back();
            sprintf(buf + len, "[%s:%s] ", pCurNode->getName(), 
                    ((pCurNode->getValue() == NULL) ? "" : pCurNode->getValue()));
        }
        
        len = strlen(buf);
        va_list ap;
        va_start( ap, format );
        int ret = vsnprintf(buf + len, 512 - len, format, ap);
        va_end( ap );
        errorLogList.add(buf, ret + len);
    }
    
    static int for_each_fn( void *s)
    {
        const char *p = ((AutoStr2 *)s)->c_str();
        switch(*p)
        {
        case LOG_LEVEL_ERR:
            LOG_ERR (( p + 1 ));
            break;
        case LOG_LEVEL_INFO:
            LOG_INFO (( p + 1 ));
            break;
        default:
            break;
        }
        return 0;
    }
    
    void flushErrorLog()
    {
        //int n = errorLogList.size();
        errorLogList.for_each(errorLogList.begin(), errorLogList.end(), for_each_fn  );
        errorLogList.clear();
    }
    
    
    void tolowerstr(char *sLine)
    {
        while(*sLine)
        {
            if (*sLine >= 'A' && *sLine <= 'Z')
                *sLine |= 0x20;
            ++sLine;
        }
    }
    
    void initKeywords()
    {
        if (bKeywordsInited)
            return ;
        
        int count = sizeof(sKeywords) / sizeof(plainconfKeywords);
        for (int i=0; i<count; ++i) 
        {
            allKeyword.insert(sKeywords[i].name, &sKeywords[i]);
            if (sKeywords[i].alias && strlen(sKeywords[i].alias) > 0)
            {
                allKeyword.insert(sKeywords[i].alias, &sKeywords[i]);
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
        tolowerstr(name);
        HashStringMap<plainconfKeywords *>::iterator it = allKeyword.find(name);
        
        if ( it == allKeyword.end())
            return NULL;
        else
            return it.second()->name;
    }
     
   
    void trimWhiteSpace(const char **p)
    {
        while(**p == 0x20 || **p == '\t')
            ++(*p);
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
        int len = strlen(sLine);
        if (sLine[len - 1] != '\\')
            return false;
        
        //Already know the last one is a '\\'
        int continuesBackSlashCount = 1;
        const char *pStart = sLine;
        const char *pLast = sLine + len - 2;
        while( pLast >= pStart &&  *pLast-- == '\\')
        {
            ++continuesBackSlashCount;
        }
        
        return (continuesBackSlashCount % 2);
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

    void saveUnknownItems(const char *fileName, int lineNumber, XmlNode *pCurNode, const char *name, const char *value)
    {
        //if not inside a "module" and without a "::", treated as error
        if (strcasecmp(pCurNode->getName(), "module") != 0 &&
            strstr(name, "::") == NULL )
        {
            logToMem(LOG_LEVEL_ERR, "Not support [%s %s] in file %s:%d", name, value,  fileName, lineNumber);
            return ;
        }
        
        char newvalue[4096] = {0};
        XmlNode *pParamNode = new XmlNode;
        const char *attr = NULL;
        pParamNode->init(UNKNOWN_KEYWORDS, &attr);
        strcpy(newvalue, name);
        strcat(newvalue, " ");
        strcat(newvalue, value);
        pParamNode->setValue(newvalue, strlen(newvalue));
        pCurNode->addChild(pParamNode->getName(), pParamNode);
    }

    void appendModuleParam(XmlNode *pModuleNode,const char *param)
    {
        XmlNode *pParamNode = pModuleNode->getChild("param");
        if (pParamNode == NULL)
        {
            pParamNode = new XmlNode;
            const char *attr = NULL;
            pParamNode->init("param", &attr);
            pParamNode->setValue(param, strlen(param));
            pModuleNode->addChild(pParamNode->getName(), pParamNode);
        }
        else
        {
            AutoStr2 totalValue = pParamNode->getValue();
            totalValue.append("\n", 1);
            totalValue.append(param, strlen(param));
            pParamNode->setValue(totalValue.c_str(), totalValue.len());
        }
        logToMem(LOG_LEVEL_INFO, "[%s:%s] module [%s] add param [%s]", 
                 pModuleNode->getParent()->getName(), 
                 ((pModuleNode->getParent()->getValue())? pModuleNode->getParent()->getValue() : ""),
                 pModuleNode->getValue(), param );
    }
    
    void addModuleWithParam(XmlNode *pCurNode, const char *moduleName, const char *param)
    {
        XmlNode *pModuleNode = NULL;
        XmlNodeList::const_iterator iter;
        const XmlNodeList *pModuleList = pCurNode->getChildren("module");
        if (pModuleList)
        {
            for( iter = pModuleList->begin(); iter != pModuleList->end(); ++iter )
            {
                if (strcasecmp((*iter)->getChildValue("name", 1), moduleName) == 0)
                {
                    pModuleNode = (*iter);
                    break;
                }
            }
        }
        
        if(!pModuleNode)
        {
            pModuleNode = new XmlNode;
            //XmlNode *pParamNode = new XmlNode;
            const char *attr = NULL;
            pModuleNode->init("module", &attr);
            pModuleNode->setValue(moduleName, strlen(moduleName));
            pCurNode->addChild(pModuleNode->getName(), pModuleNode);
            logToMem(LOG_LEVEL_INFO, "[%s:%s]addModuleWithParam ADD module %s", 
                     pCurNode->getName(), pCurNode->getValue(), moduleName );
        }
        appendModuleParam(pModuleNode, param);
    }
    
    void handleSpecialCase(XmlNode *pNode)
    {
        const XmlNodeList *pUnknownList = pNode->getChildren(UNKNOWN_KEYWORDS);
        if (!pUnknownList)
            return ;
        
        int bModuleNode = (strcasecmp(pNode->getName(), "module") == 0);
        XmlNodeList::const_iterator iter;
        for( iter = pUnknownList->begin(); iter != pUnknownList->end(); ++iter )
        {
            const char *value = (*iter)->getValue();
            if (bModuleNode)
                appendModuleParam(pNode, value);
            else
            {
                const char *p = strstr(value, "::");
                
                //Only hanlde has :: case
                if (p)
                {
                    /**
                     * CASE such as cache::enablecache 1, will be treated as 
                     * module chace {
                     * param enablecache 1
                     * }
                     */
                    char newname[1024] = {0};
                    char newvalue[4096] = {0};
                    strncpy(newname, value, p - value);
                    strcpy(newvalue, p + 2);
                    
                    XmlNode *pRootNode = pNode;
                    while(pRootNode->getParent())
                        pRootNode = pRootNode->getParent();
                    const XmlNodeList *pModuleList = pRootNode->getChildren("module");
                    if (pModuleList)
                    {
                        XmlNodeList::const_iterator iter2;
                        for( iter2 = pModuleList->begin(); iter2 != pModuleList->end(); ++iter2 )
                        {
                            if (strcasecmp((*iter2)->getValue(), newname) == 0)
                            {
                                addModuleWithParam(pNode, newname, newvalue);
                                break;
                            }
                        }
                        if (iter2 == pModuleList->end())
                            logToMem(LOG_LEVEL_ERR, "Module[%s] not defined in server leve while checking [%s].",newname, value );

                    }
                    else
                        logToMem(LOG_LEVEL_ERR, "No module defined in server leve while checking [%s].", value );
                }
            }
        }
    }
    
    void handleSpecialCaseLoop(XmlNode *pNode)
    {
        XmlNodeList list;
        int count = pNode->getAllChildren(list);
        if (count > 0)
        {
            XmlNodeList::const_iterator iter;
            for( iter = list.begin(); iter != list.end(); ++iter )
                handleSpecialCaseLoop(*iter);
            handleSpecialCase(pNode);
        }
    }
    
    void clearNamdAndValue(char* name, char* value)
    {
        name[0] = 0;
        value[0] = 0;
    }
    
    void parseLine(const char *fileName, int lineNumber, const char *sLine)
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
                        if (strlen(value) > 0)
                            pNode->setValue(value, strlen(value));
                        pCurNode->addChild(pNode->getName(), pNode);
                        gModuleList.push_back(pNode);
                        pCurNode = pNode;
                        clearNamdAndValue(name, value);
                        break;
                    }
                    else
                    {
                        logToMem(LOG_LEVEL_ERR, "parseline find block name [%s] is NOT keyword in %s:%d", name, fileName, lineNumber);
                        break;
                    }
                }
                else
                {
                    logToMem(LOG_LEVEL_ERR, "parseline found '{' without a block name in %s:%d", fileName, lineNumber);
                    break;
                }
            }
            
            else if (*p == '}' && p == sLine)
            {
                if (gModuleList.size() > 1)
                {
                    gModuleList.pop_back();
                    clearNamdAndValue(name, value);
                    if (*(p + 1))
                    {
                        ++p;
                        trimWhiteSpace(&p);
                        parseLine(fileName, lineNumber, p);
                        break;
                    }
                }
                else
                {
                    logToMem(LOG_LEVEL_ERR, "parseline found more '}' in %s:%d", fileName, lineNumber);
                    clearNamdAndValue(name, value);
                    break;
                }
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
                {
                    if (*p != '\\')
                        strcatchr(value, *p, MAX_NAME_LENGTH);
                    else if (p + 1 != pEnd)
                    {
                        ++p;
                        char c = 0;
                        switch(*p)
                        {
                            case 'r':  c = '\r';  break;
                            case 'n':  c = '\n';  break;
                            case 't':  c = '\t';  break;
                            default:  c = *p;     break;
                        }
                        strcatchr(value, c, MAX_NAME_LENGTH);
                    }
                }
            }
        }
        
        if ( name[0] != 0 )
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
            {
                //There is no special case in server level
                //if (memcmp(pCurNode->getName(), SERVER_ROOT_XML_NAME, sizeof(SERVER_ROOT_XML_NAME) - 1) != 0)
                    saveUnknownItems(fileName, lineNumber, pCurNode, name, value);
                //else
                //    logToMem(LOG_LEVEL_ERR, "%s Server level find unknown keyword [%s], ignored.", SERVER_ROOT_XML_NAME, name );
            }
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
        if (strncasecmp(sLine, "include", 7) == 0)
        {
            char *p = (char *)sLine + 7;
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
            logToMem(LOG_LEVEL_ERR, "Failed to open directory [%s].", pPath );
            return ;
        }

        struct dirent * dir_ent;
        StringList AllEntries;
        while( ( dir_ent = readdir( pDir ) ) )
        {
            const char *pName = dir_ent->d_name;
            if (( strcmp( pName, "." ) == 0 )||
                ( strcmp( pName, ".." ) == 0 )||
                ( *( pName + strlen( pName ) - 1) == '~' ) )
                continue;
            if ( pPattern )
            {
                if ( fnmatch( pPattern, pName, FNM_PATHNAME ) )
                    continue;
            }
            
            char str[4096] = {0};
            strcpy(str, pPath);
            strcatchr(str, '/', 4096);
            strcat(str, pName);
            AllEntries.add(str);
        }
        closedir( pDir );
        
        //Sort the filename order
        AllEntries.sort();
        StringList::iterator iter;
        for( iter = AllEntries.begin(); iter != AllEntries.end(); ++iter )
        {
            const char *p = (*iter)->c_str();
            logToMem(LOG_LEVEL_INFO, "Processing config file: %s", p );
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
            
        else if (orgFile[0] == '$')
        {
            if (strncasecmp(orgFile, "$server_root/", 13) == 0)
            {
                strcpy(targetFile, rootPath.c_str());
                strcat(targetFile, orgFile + 13);
            }
            else
            {
                logToMem(LOG_LEVEL_ERR, "Can't resolve include file %s", orgFile);
                return ;
            }
        }
        
        else
        {
            strcpy(targetFile, rootPath.c_str());
            strcat(targetFile, orgFile);
        }
    }
    
    //This function may be recruse called
    void LoadConfFile(const char *path)
    {
        logToMem(LOG_LEVEL_INFO, "start parsing file %s", path);
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
                logToMem(LOG_LEVEL_ERR, "LoadConfFile error 1, path:%s directory:%s",
                         path, prefixPath.c_str() );
                return ;
            }
            
            if ((sb.st_mode & S_IFMT) != S_IFDIR)
            {
                logToMem(LOG_LEVEL_ERR, "LoadConfFile error 2, path:%s directory:%s",
                          path, prefixPath.c_str() );
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
                logToMem(LOG_LEVEL_ERR, "Cannot open configuration file: %s", path );
                return;
            }
            
            const int MAX_LINE_LENGTH = 8192;
            char sLine[MAX_LINE_LENGTH];
            char * p;
            char sLines[MAX_LINE_LENGTH] = {0};
            int lineNumber = 0;

            while(fgets(sLine, MAX_LINE_LENGTH, fp), !feof(fp))
            {
                ++lineNumber;
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
                        //strcatchr(sLines, ' ', MAX_LINE_LENGTH); //add a space at the end of the line which has a '\\'
                    }
                    
                    else
                    {
                        strcat(sLines, p);
                        parseLine(path, lineNumber, sLines);
                        sLines[0] = 0x00;
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
        rootNode->init(SERVER_ROOT_XML_NAME, &attr);
        gModuleList.push_back(rootNode);
        
        LoadConfFile(configFilePath);
        if (gModuleList.size() != 1)
        {
            logToMem(LOG_LEVEL_ERR, "parseFile find '{' and '}' do not match in the end of file %s.", configFilePath);
        }
        gModuleList.clear();
        
        handleSpecialCaseLoop(rootNode);
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
        
    void outputValue(FILE *fp, const char *value, int length)
    {
        fwrite(value, length, 1, fp);
    }
    
    void outputSigleNode(FILE *fp, const XmlNode *pNode, int level)
    {
        const char *pStart = pNode->getValue();
        fprintf(fp, "%s ", pNode->getName());
        if (pStart)
        {
            const char *p;
            while(*pStart && (p = strchr(pStart, '\n')))
            {
                outputValue(fp, pStart, p - pStart);
                fwrite("\\n\\\n", 4, 1, fp);
                outputSpaces(level + 1, fp);
                pStart = p + 1;
            }
            if (*pStart)
            {
                outputValue(fp, pStart, strlen(pStart));
            }
        }
        fprintf(fp, "\n");
    }
    static int s_compare( const void * p1, const void * p2 )
    {
        return (*((XmlNode**)p1))->getName() - (*((XmlNode**)p2))->getName();
        //strcasecmp( (*((XmlNode**)p1))->getName(),
               //        (*((XmlNode**)p2))->getName() );
    }

    void outputConfigFile(const XmlNode *pNode, FILE *fp, int level)
    {
        XmlNodeList list;
        int count = pNode->getAllChildren(list);
        list.sort(s_compare);
        if (count > 0)
        {
            if (level > 0)
            {
                outputSpaces(level, fp);
                const char *value = pNode->getValue();
                if (!value)
                    value = "";
                fprintf(fp, "%s %s{\n", pNode->getName(), value);
            }
            
            XmlNodeList::const_iterator iter;
            for( iter = list.begin(); iter != list.end(); ++iter )
                outputConfigFile((*iter), fp, level + 1);
            
            if (level > 0)
            {
                outputSpaces(level, fp);
                fprintf(fp, "}\n");
            }
        }
        else
        {
            outputSpaces(level, fp);
            outputSigleNode(fp, pNode, level);
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
