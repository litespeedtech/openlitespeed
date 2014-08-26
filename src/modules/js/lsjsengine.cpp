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

#include "lsjsengine.h"
#include "../addon/include/ls.h"
#include <http/httplog.h>
#include <log4cxx/logger.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <util/fdpass.h>
// #include "/home/user/simon/unixdomain/nethelper.c"

// Node search path:
//
// run script /home/simon/projects/ls.js
//  require("fun.js")
// (1) /home/simon/projects/node_modules/fun.js
// (2) /home/simon/node_modules/fun.js
// (3) /home/node_modules/fun.js
// (4) /node_modules/fun.js
//
// if prefix with /, ../ and ./ then the corresponding path will be using.
//

int             LsJsEngine::s_firstTime = 1;          // firstTime parse param
int             LsJsEngine::s_ready = 0;              // not ready
char *          LsJsEngine::s_serverSocket = 0;       // default module name

//
//  Should use user param for this
//
#define DEF_SERVERSOCKET "/home/user/lsws/socket/LS_NODE"

LsJsEngine::LsJsEngine()
{
}

LsJsEngine::~LsJsEngine()
{
}

int LsJsEngine::init()
{
    s_ready = 0;     // set no ready
    
    if (! (s_serverSocket = (char *)malloc(strlen(DEF_SERVERSOCKET)+10)) )
    {
        g_api->log(NULL, LSI_LOG_NOTICE
                , "LiteSpeed Node.js handler NO MEMORY\n" );
        return 0;
    }
        
    strcpy(s_serverSocket, DEF_SERVERSOCKET);
    s_ready = 1;     // ready rock n roll
    
    g_api->log(NULL, LSI_LOG_NOTICE
                , "LiteSpeed Node.js socketPath [%s]\n"
                , s_serverSocket
                );
    
    return 0 ;
}

int	LsJsEngine::isReady(lsi_session_t *session)
{
    if (!s_ready)
        return 0;
    return 1;
}

//
//  runScript - working code.
//
int    LsJsEngine::runScript(lsi_session_t *session
                            , LsJsUserParam * pUser
                            , const char * scriptpath)
{
    char        buf[0x100];
    int         nb;
    static int  counter = 0;    // testing purpose
    counter++;
    
    if (pUser)
        g_api->log(session, LSI_LOG_DEBUG
                , "level %d ready %d script [%s] %d\n"
                , pUser->level() 
                , pUser->isReady()
                , scriptpath
                , counter
                );
    else
        g_api->log(session, LSI_LOG_DEBUG
                , "script [%s] %d\n"
                , scriptpath
                , counter
                );
        
    int node_fd = 0;
    if ( (node_fd = tcpDomainSocket(s_serverSocket)) < 0 )
    {
        g_api->log(NULL, LSI_LOG_NOTICE,
                   "FAILED TO CONNECT LiteSpeed Node.js SOCKET [%s] errno %d [%d]\r\n"
                   , s_serverSocket
                   , errno, counter);
        
        nb = snprintf(buf, 0x1000, "<html><body>\r\n"
                                    "<p>FAILED TO CONNECT litespeed.js SOCKET [%s]</p>\r\n"
                                    "<p>%s</p>\r\n"
                                    "</body></html>\r\n"
                        , s_serverSocket
                        , scriptpath
                     );
        g_api->append_resp_body( session ,  buf, nb);
        g_api->end_resp( session );
        return(0);
    }

    char *xbuf = 0;
    int xbuflen = 0;
    int http_fd = 0;
    http_fd = g_api->handoff_fd(session, &xbuf, &xbuflen);
    
    if (http_fd < 0)
    {
        g_api->log(NULL, LSI_LOG_NOTICE
                    , "LS-JS HTTP failed to handoff [%s]"
                    , scriptpath);
        nb = snprintf(buf, 0x1000, "<html><body>\r\n"
                                "<p>LiteSpeed Node.js is busy</p>\r\n"
                                "<p>LiteSpeed Node.js FAILED to handoff [%s]</p>\r\n"
                                "</body></html>\r\n", scriptpath);
        g_api->append_resp_body( session ,  buf, nb);
        g_api->end_resp( session );
        close(node_fd);
        return(0);
    }
    
    // At this point node_fd and http_fd should be good
    nb = snprintf(buf, 0x100, "Running %s\r\n", scriptpath);
    
    int byteSent;
    byteSent = FDPass::writex_fd(node_fd, xbuf, xbuflen, http_fd);
    if (byteSent != xbuflen)
    {
        nb = snprintf(buf, 0x100
                        , "FAILED TO SENDFD %s [sent %d got %d] errno %d\r\n"
                        , scriptpath, xbuflen, byteSent, errno);
        g_api->log(NULL, LSI_LOG_NOTICE, buf);
        
        // attemp to write msg back to blowser
        write(http_fd, "<html><body>\r\n", 14);
        write(http_fd, buf, nb);
        write(http_fd, "<p>check litespeed.js status</p>\r\n", 34);
        write(http_fd, "</body>\r\n", 9);
        write(http_fd, "</html>\r\n", 9);
    }
    if (xbuf && xbuflen)
        free(xbuf);
    close(node_fd);
    close(http_fd);
    return 0;
}

//
//  Configuration parameters for LiteSpeed js driver
//
void* LsJsEngine::parseParam( const char* param
                                , void* initial_config
                                , int level
                                , const char *name )
{
    char    key[0x1000];
    char    value[0x1000];
    int     n, nb;
    register char    * p;
    
    LsJsUserParam * pParent = (LsJsUserParam * )initial_config;
    LsJsUserParam * pUser = new LsJsUserParam(level);
    
    g_api->log(NULL, LSI_LOG_NOTICE
                    , "JS Param %s %d %s"
                    , name
                    , level
                    , param ? param : "");
    
    if ((!pUser) || (!pUser->isReady()))
    {
        g_api->log(NULL, LSI_LOG_ERROR, "JS PARSEPARAM NO MEMORY");
        return NULL;
    }
    if (pParent)
    {
        // inheritan!
        *pUser = *pParent;
    }

    if (!param)
    {
        s_firstTime = 0;
        return pUser;
    }
    
    p = (char *)param;
    g_api->log(NULL, LSI_LOG_NOTICE
                    , "JS Param name %s level %d %s parent %d param %s %d"
                    , name
                    , level
                    , (level == LSI_MODULE_DATA_HTTP) ? "HTTP" : ""
                    , pParent ? pParent->level() : -1
                    , param ? param : ""
                    , pParent ? pParent->isReady() : -1
                    );
    
    // search for each parameters here
    do
    {
        n = sscanf(p, "%s%s%n", key, value, &nb);
        if (n == 2)
        {
            // runtime parameters
            if (!strcasecmp("data", key))
            {
                // allow hex and decimal
                int val = 0;
                if ((sscanf(value, "%i", &val) == 1) && (val > 0))
                {
                    pUser->setData(val);
                }
                g_api->log(NULL, LSI_LOG_NOTICE
                            , "%s JS SET %s = %s [%d]\n"
                            , name , key, value, pUser->data());
            }
            else
            {
                // ignore this pair values
                g_api->log(NULL, LSI_LOG_NOTICE
                            , "%s IGNORE MODULE PARAMETERS [%s] [%s]\n"
                            , name, key, value);
            }
        }
        p += nb;
    } while (n == 2);
    s_firstTime = 0;
    return (void *)pUser;
}

void LsJsEngine::removeParam( void* config )
{
    g_api->log(NULL , LSI_LOG_NOTICE
            , "REMOVE PARAMETERS [%p]\n", config);
}

int LsJsEngine::tcpDomainSocket( const char* path )
{
    struct sockaddr_un  addr;
    register int        fd;
    
    if ( (strlen(path) > sizeof(addr.sun_path) - 1) ) {
        g_api->log(NULL, LSI_LOG_NOTICE
                    , "domainSocket Path too long %d max %d\n"
                    , strlen(path), sizeof(addr.sun_path) - 1);
        return -1;
    }
    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        g_api->log(NULL, LSI_LOG_NOTICE
                    , "FAILED TO Aquire domainSocket %s\n"
                    , path
                  );
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);

    /* client mode: just connect it */
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        g_api->log(NULL, LSI_LOG_NOTICE
                    , "FAILED TO connect domainSocket %s\n"
                    , path
               );
        close(fd);
        return -1;
    }
    return fd;
}
