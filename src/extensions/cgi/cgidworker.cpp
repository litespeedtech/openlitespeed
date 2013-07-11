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
#include "cgidworker.h"
#include "cgidconfig.h"
#include "cgidconn.h"
#include "lscgiddef.h"
#include "suexec.h"
#include "lscgid.h"
#include <http/httpdefs.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <socket/coresocket.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

extern char *argv0;


CgidWorker::CgidWorker( const char * pName )
    : m_pid(-1)
    , m_fdCgid(-1)
    , m_lve( 1 )
{
    setConfigPointer( new CgidConfig( pName ) );
}

CgidWorker::~CgidWorker()
{
}

ExtConn * CgidWorker::newConn()
{
    return new CgidConn();
}

#include <extensions/registry/extappregistry.h>
extern void generateSecret( char * pBuf);
extern int removeSimiliarFiles( const char * pPath, long tm );

const char * HttpGlobals::getCgidSecret()
{
    return s_pCgid->getConfig().getSecret();
}


int CgidWorker::start( const char * pServerRoot, const char * pChroot,
                        uid_t uid, gid_t gid, int priority )
{
    char achSocket[1024];
    int fd;
    int ret;
    if ( m_pid != -1 )
        return 0;

    CgidConfig& config = getConfig();
    generateSecret( config.getSecretBuf());
    HttpGlobals::s_pCgid = this;
    config.addEnv( "PATH=/bin:/usr/bin:/usr/local/bin" );
    config.addEnv( NULL );
    
    char *p = achSocket;
    int i, n;
    memccpy( p, config.getSocket(), 0, 128 );
    setURL( p );
    fd = ExtWorker::startServerSock( &config, 200 );
    if ( fd == -1 )
    {
        LOG_ERR(("Cannot create a valid unix domain socket for CGI daemon." ));
        return -1;
    }
    m_fdCgid = fd;

    n = snprintf( p, 255, "uds:/%s", getConfig().getServerAddrUnixSock() );
    if ( getuid() == 0 )
    {
        chown( p + 5, 0, gid );
        chmod( p + 5, 0760 );
    }
    else
    {
        chmod( p + 5, 0700 );
    }

    if ( pChroot )
    {
        i = strlen( pChroot );
        memmove( p + 5, p + 5 + i, n - 5 - i + 1 );
    }
    setURL( p );

    ret = spawnCgid( m_fdCgid, p, config.getSecret() );

    if ( ret ==-1 )
    {
        return -1;
    }
    setState( ST_GOOD );
    return ret;
}

static void CloseUnusedFd(int fd)
{
    for ( int i=3; i < 1024; ++i )
    {
        if ( i != fd )
            
            close(i);
    }
}

//return 0 means OK, -1 means ERROR
int CgidWorker::spawnCgid( int fd, char * pData, const char *secret )
{
    int pid;
    const char * pChroot = "";
    if ( HttpGlobals::s_psChroot )
        pChroot = HttpGlobals::s_psChroot->c_str();
    snprintf( pData, 255, "uds:/%s%s", pChroot,
            getConfig().getServerAddrUnixSock() );
    pid = fork();
    //in child
    if (pid == 0)
    {
        CloseUnusedFd(fd);
        int ret = lscgid_main(fd, argv0, secret, pData);
        exit(ret);
    }
    else if (pid > 0)
    {
        LOG_NOTICE(( "[PID: %d]: forked cgid: %d", getpid(), pid ));
        m_pid = pid;
        return pid;
    }
    else 
    {
        LOG_ERR(( "[PID: %d]: fork error", getpid() ));
        return -1;
    }
}

int CgidWorker::getCgidPid()
{
    CgidWorker * pWorker = (CgidWorker *)ExtAppRegistry::getApp(
                    EA_CGID, LSCGID_NAME );
    return pWorker->m_pid;
}

int CgidWorker::checkRestartCgid( const char * pServerRoot, const char * pChroot,
                        int priority, int switchToLscgid )
{
    CgidWorker * pWorker = (CgidWorker *)ExtAppRegistry::getApp(
                    EA_CGID, LSCGID_NAME );
    return pWorker->watchDog( pServerRoot, pChroot, priority, switchToLscgid );
}

int CgidWorker::watchDog( const char * pServerRoot, const char * pChroot,
                        int priority, int switchToLscgid )
{
    if ( ( switchToLscgid == 1 )& ( m_pid != -1 ))
    {
        kill( m_pid, SIGTERM );
        m_pid = -1;
    }
    if ( m_pid != -1 )
        if ( kill( m_pid, 0 ) == 0 )
            return 0;
    char achData[256];
    char achExec[2048];
    if ( switchToLscgid )
    {
        snprintf( achExec, 2048, "%s%sbin/lscgid", (pChroot)?pChroot:"",
              pServerRoot );
    }
    else
        snprintf( achExec, 2048, "%s%sbin/httpd", (pChroot)?pChroot:"",
              pServerRoot );

    int ret = spawnCgid( m_fdCgid, achData, getConfig().getSecret());
    if ( ret != -1 )
        setState( ST_GOOD );
    return ret;    
    
}

void CgidWorker::closeFdCgid()
{
    close( m_fdCgid );
}

                       
