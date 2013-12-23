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
#include "lshttpdmain.h"

#include <extensions/cgi/cgidworker.h>
#include <extensions/cgi/suexec.h>
#include <extensions/registry/extappregistry.h>
#include <http/datetime.h>
#include <http/httpdefs.h>
#include <http/httpglobals.h>
#include <http/httplog.h>
#include <http/httpserverconfig.h>
#include <http/httpserverversion.h>
#include <http/httpsignals.h>
#include <http/platforms.h>
#include <http/stderrlogger.h>

#include <log4cxx/logger.h>
#include <log4cxx/logrotate.h>

#include <main/httpserver.h>
#include <main/httpserverbuilder.h>
#include <main/serverinfo.h>

#include <util/daemonize.h>
#include <util/emailsender.h>
#include <util/gpath.h>
#include <util/mysleep.h>
#include <util/ni_fio.h>
#include <util/pidfile.h>
#include <util/stringlist.h>
#include <util/stringtool.h>
#include <util/signalutil.h>
#include <util/vmembuf.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <util/ssnprintf.h>

#define PID_FILE            DEFAULT_TMP_DIR "/lshttpd.pid"
#define SYSSTATS_FILE       DEFAULT_TMP_DIR "/.sysstats"

static char s_iRunning = 0;
char *argv0 = NULL;


LshttpdMain::LshttpdMain()
    : m_pServer( NULL )
    , m_pBuilder( NULL )
    , m_noDaemon( 0 )
    , m_noCrashGuard( 0 )
    , m_pProcState( NULL )
    , m_curChildren( 0 )
    , m_fdAdmin( -1 )
{
    m_pServer = &HttpServer::getInstance();
    m_pBuilder = new HttpServerBuilder( m_pServer );
    HttpGlobals::s_pBuilder = m_pBuilder;
}
LshttpdMain::~LshttpdMain()
{
    if ( HttpGlobals::s_children >= 32 )
        free( m_pProcState );
    if ( m_pBuilder )
        delete m_pBuilder;
}


int LshttpdMain::forkTooFreq()
{
    LOG_WARN(( "[AutoRestarter] forking too frequently, suspend for a while!" ));
    return 0;
}

int LshttpdMain::preFork()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( "[AutoRestarter] prepare to fork new child process to handle request!" ));
    return 0;
}

int LshttpdMain::forkError( int err )
{
    return 0;
}

int LshttpdMain::postFork( pid_t pid )
{
    LOG_NOTICE(( "[AutoRestarter] new child process with pid=%d is forked!", pid ));
    return 0;
}

int LshttpdMain::childExit( pid_t ch_pid, int stat )
{
    LOG_NOTICE(( "[AutoRestarter] child process with pid=%d exited with status=%d!",
            ch_pid, stat ));
    if ( stat != 100 )
        return 0;
    return 0;
}


int LshttpdMain::childSignaled( pid_t pid, int signal, int coredump )
{
    static char pCoreFile[2][30] =
          { "no core file is created",
            "a core file is created" };
    LOG_NOTICE(( "[AutoRestarter] child process with pid=%d received signal=%d, %s!",
                (int)pid, signal, pCoreFile[ coredump != 0 ] ));
    //cleanUp();

    //We are in middle of graceful shutdown, do not restart another copy
        SendCrashNotification( pid, signal, coredump, pCoreFile[coredump != 0] );
    if ( coredump )
    {
        
        if ( access( "/tmp/lshttpd/bak_core", X_OK) == -1 )
            ::system( "mkdir /tmp/lshttpd/bak_core" );
        
        else
        {
            int status = ::system( "expr `ls /tmp/lshttpd/bak_core | wc -l` \\< 5" );
            if ( WEXITSTATUS( status ) )
                ::system( "rm /tmp/lshttpd/bak_core/*" );
        }
        ::system( "mv /tmp/lshttpd/core* /tmp/lshttpd/bak_core" );
    }
    return 0;
}

int LshttpdMain::SendCrashNotification( pid_t pid, int signal, int coredump,
                    char * pCoreFile )
{
    char achSubject[512];
    char achContent[2048];
    char achTime[50];
    static struct utsname      s_uname;
    const char * pAdminEmails = m_pBuilder->getAdminEmails();
    if (( !pAdminEmails )||( !*pAdminEmails ))
        return 0;
    memset( &s_uname, 0, sizeof( s_uname ) );
    if ( uname( &s_uname ) == -1)
        LOG_WARN(( "uname() failed!" ));
    safe_snprintf( achSubject, sizeof( achSubject ) - 1,
            "Web server %s on %s is automatically restarted",
            m_pBuilder->getServerName(), s_uname.nodename );
    DateTime::getLogTime( DateTime::s_curTime, achTime );
    achTime[28] = 0;
    int len = safe_snprintf( achContent, sizeof( achContent ) - 1,
            "At %s, web server with pid=%d received unexpected signal=%d, %s."
            " A new instance of web server will be started automatically!\n\n"
            , achTime, (int)pid, signal, pCoreFile);
    if ( coredump )
    {

        len += safe_snprintf( achContent + len, sizeof( achContent ) - len -1,
            "Please forward the following debug information to bug@litespeedtech.com.\n"
            "Environment:\n\n"
            "Server: %s\n"
            "OS: %s\n"
            "Release: %s\n"
            "Version: %s\n"
            "Machine: %s\n\n"
            "If the call stack information does not show up here, "
            "please compress and forward the core file located in /tmp/lshttpd/.\n\n",
            HttpServerVersion::getVersion(),
            s_uname.sysname,
            s_uname.release,
            s_uname.version,
            s_uname.machine );
    }
    char achFileName[50] = "/tmp/m-XXXXXX";
    int fd = mkstemp( achFileName );
    if ( fd == -1 )
    {
        return -1;
    }
    write( fd, achContent, len );
    close( fd );
    if ( coredump )
    {
        const char * pGDB = m_pBuilder->getGDBPath();
        if (( pGDB == NULL || !*pGDB ))
            pGDB = "gdb";
        char achCmd[1024];
        safe_snprintf( achCmd, 1024,
            "%s --batch --command=%s/admin/misc/gdb-bt %s/bin/lshttpd %s/core* >> %s",
            pGDB, m_pBuilder->getServerRoot(), m_pBuilder->getServerRoot(),
            DEFAULT_TMP_DIR, achFileName );

        if ( ::system( achCmd ) == 0 )
        {

            //removeMatchFile( DEFAULT_TMP_DIR, "core" );
        }
    }
    EmailSender::sendFile( achSubject, pAdminEmails, achFileName );
    ::unlink( achFileName );
    return 0;
}

void LshttpdMain::onGuardTimer()
{
    static int s_count = 0;
    DateTime::s_curTime = time( NULL );
//#if !defined( RUN_TEST )
    if ( m_pidFile.testAndRelockPidFile(PID_FILE, m_pid) )
    {
        LOG_NOTICE(( "Failed to lock PID file, restart server gracefully ..."));
        gracefulRestart();
        return;
    }
//#endif
    CgidWorker::checkRestartCgid( m_pBuilder->getServerRoot(),
                          m_pBuilder->getChroot(),
                          HttpGlobals::s_priority );
    HttpLog::onTimer();
    m_pServer->onVHostTimer();
    s_count = (s_count + 1) % 5;
    clearToStopApp();

    if ( !s_count )
    {
        writeSysStats();
    }
    //processAdminCtrlFile( m_sCtrlFile.c_str());

    checkRestartReq();
}




int LshttpdMain::processAdminCmd( char * pCmd, char * pEnd, int &apply )
{
    if ( strncasecmp( pCmd, "reload:", 7 ) == 0 )
    {
        apply = 1;
        pCmd += 7;
        if ( strncasecmp( pCmd, "config", 6 ) == 0 )
        {
            LOG_NOTICE(( "Reload configuration request from admin interface!" ));
            reconfig();
        }
        else if ( strncasecmp( pCmd, "vhost:", 6 ) == 0 )
        {
            pCmd += 6;
            LOG_NOTICE(( "Reload configuration for virtual host %s!", pCmd ));
            m_pBuilder->reconfigVHost( pCmd );
        }
    }
    else if ( strncasecmp( pCmd, "enable:", 7 ) == 0 )
    {
        apply = 1;
        pCmd += 7;
        if ( strncasecmp( pCmd, "vhost:", 6 ) == 0 )
        {
            pCmd += 6;
            if ( !m_pServer->enableVHost( pCmd, 1 ) )
                LOG_NOTICE(( "Virtual host %s is enabled!", pCmd ));
            else
                LOG_ERR(( "Virtual host %s can not be enabled, reload first!", pCmd ));
        }
    }
    else if ( strncasecmp( pCmd, "disable:", 8 ) == 0 )
    {
        apply = 1;
        pCmd += 8;
        if ( strncasecmp( pCmd, "vhost:", 6 ) == 0 )
        {
            pCmd += 6;
            if ( !m_pServer->enableVHost( pCmd, 0 ) )
                LOG_NOTICE(( "Virtual host %s is disabled!", pCmd ));
            else
                LOG_ERR(( "Virtual host %s can not be disabled, reload first!", pCmd ));
        }
    }
    else if ( strncasecmp( pCmd, "restart", 7 ) == 0 )
    {
        LOG_NOTICE(( "Server restart request from admin interface!" ));
        gracefulRestart();
    }
    else if ( strncasecmp( pCmd, "toggledbg", 7 ) == 0 )
    {
        LOG_NOTICE(( "Toggle debug logging request from admin interface!" ));
        broadcastSig( SIGUSR2, 0 );
        apply = 0;
    }
    return 0;
}

int LshttpdMain::getFullPath( const char * pRelativePath, char * pBuf, int bufLen )
{
    return safe_snprintf( pBuf, bufLen, "%s%s%s",
                (HttpGlobals::s_psChroot)?HttpGlobals::s_psChroot->c_str():"",
                HttpGlobals::s_pServerRoot, pRelativePath );
    
}

int LshttpdMain::execute( const char * pExecCmd, const char * pParam )
{
    char achBuf[512];
    int n = getFullPath( pExecCmd, achBuf, 512 );
    safe_snprintf( &achBuf[n], 511 - n, " %s", pParam );
    int ret = system( achBuf );
    return ret;
}

#include <socket/coresocket.h>
int LshttpdMain::startAdminSocket()
{
    int i;
    char achBuf[1024];
    if ( m_fdAdmin != -1 )
        return 0;
    srand( time( NULL ) );
    for( i = 0; i < 100; ++i )
    {
        snprintf(achBuf, 132, "uds:/%s%sadmin/tmp/admin.sock.%d", (HttpGlobals::s_psChroot)?HttpGlobals::s_psChroot->c_str():"",
                 HttpGlobals::s_pServerRoot, 7000 + rand() % 1000 );
        //snprintf(achBuf, 255, "127.0.0.1:%d", 7000 + rand() % 1000 );
        if ( CoreSocket::listen( achBuf, 10, &m_fdAdmin, 0 , 0 ) == 0 )
            break;
        if (!( i % 20 ))
        {
            snprintf( achBuf, 1024, "rm -rf '%s%sadmin/tmp/admin.sock.*'",
                      (HttpGlobals::s_psChroot)?HttpGlobals::s_psChroot->c_str():"",
                                                                               HttpGlobals::s_pServerRoot  );
                      system( achBuf );
        }
    }
    if ( i == 100 )
        return -1;
    ::fcntl( m_fdAdmin, F_SETFD, FD_CLOEXEC );
    HttpGlobals::s_pAdminSock = strdup( achBuf );
    LOG_NOTICE(( "[ADMIN] server socket: %s", achBuf ));
    return 0;
}

int LshttpdMain::closeAdminSocket()
{
    if ( HttpGlobals::s_pAdminSock )
    {
        if ( strncmp( HttpGlobals::s_pAdminSock, "uds://", 6 ) == 0 )
        {
            unlink( &HttpGlobals::s_pAdminSock[5] );
        }
    }
    close( m_fdAdmin );
    return 0;
}

int LshttpdMain::acceptAdminSockConn()
{
    struct sockaddr_in peer;
    socklen_t addrlen;
    addrlen = sizeof( peer );
    int fd = accept( m_fdAdmin, (struct sockaddr *)&peer, &addrlen );
    if ( fd == -1 )
        return -1;
    int ret = 0;
    ::fcntl( fd, F_SETFD, FD_CLOEXEC );
    //FIXME: add access control here
    ret = processAdminSockConn( fd );
    if ( ret != 2 )
    {
        static const char * pCode[2] = {"OK", "Failed"};
        if ( ret != 0 )
            ret = 1;
        nio_write( fd, pCode[ret], strlen( pCode[ret] ) );
    }
    close( fd );
    return 0;
}


int LshttpdMain::processAdminSockConn( int fd )
{
    char achBuf[4096];
    char * pEnd = &achBuf[4096];
    char * p = achBuf;
    int len;
    while( (len = nio_read( fd, p, pEnd - p )) > 0 )
    {
        p += len;
    }
    
    if (( len == -1 )||( pEnd == p ))
    {
        LOG_ERR(( "[ADMIN] failed to read command, command buf len=%d",
                    (int)(p - achBuf) ));
        return -1;
    }
    char * pEndAuth = strchr( achBuf, '\n' );
    if ( !pEndAuth )
        return -1;
    if ( m_pServer->authAdminReq( achBuf ) != 0 )
        return -1;
    ++pEndAuth;
    return processAdminBuffer( pEndAuth, p );
    
}

int LshttpdMain::processAdminBuffer( char * p, char * pEnd )
{
    while(( pEnd > p )&&isspace( pEnd[-1] ))
        --pEnd;
    if ( pEnd - p < 14 )
        return -1;
    if ( strncasecmp( pEnd - 14, "end of actions", 14 ) != 0 )
    {
        LOG_ERR(( "[ADMIN] failed to read command, command buf len=%d",
                    (int)(pEnd - p) ));
        return -1;
    }
    pEnd -= 14;
    int apply;
    char * pLineEnd;
    while( p < pEnd )
    {
        pLineEnd = (char *)memchr( p, '\n', pEnd - p );
        if (pLineEnd == NULL )
            pLineEnd = pEnd;
        char *pTemp = pLineEnd;
        while(( pLineEnd > p )&&isspace( pLineEnd[-1] ))
            --pLineEnd;
        *pLineEnd = 0;
        if ( processAdminCmd(  p, pLineEnd, apply ) )
            break;
        p = pTemp + 1;
    }
    m_pBuilder->releaseConfigXmlTree();
    if ( s_iRunning )
        m_pServer->generateStatusReport();
    if ( apply )
        applyChanges();
    return 0;
}


void LshttpdMain::writeSysStats()
{
    char achStatsFile[256] = "";
    char achTmpStatsFile[256];
    if ( HttpGlobals::s_psChroot )
    {
        //prefix chroot here
        strcpy( achStatsFile, HttpGlobals::s_psChroot->c_str() );
    }
    strcat( achStatsFile, SYSSTATS_FILE );
    strcpy( achTmpStatsFile, achStatsFile );
    strcat( achTmpStatsFile, ".tmp" );
    int fd  = open( achTmpStatsFile, O_WRONLY | O_CREAT, 0644 );
    if ( fd == -1 )
        LOG_ERR(( "Failed to create system statistic report file: %s", achTmpStatsFile ));
    else
    {
        writeProcessData( fd );
        close( fd );
        rename( achTmpStatsFile, achStatsFile );
    }
}

void LshttpdMain::writeProcessData( int fd )
{
    char achPid[20];
    static char achPs[3][3] = { "ps", "-o", "-p" };
    static char achParam[] = "pid,nice,pri,pcpu,pmem,rss,vsz,time";
    static char achPath[] = "PATH=/bin:/usr/bin";
    char *psCmd[32] =
    {
    achPs[0], achPs[1], achParam
    };
    psCmd[3] = achPs[2];

    psCmd[4] = achPid;
    psCmd[5] = NULL;
    write( fd, "PS\n", 3 );
    
    ChildProc * pProc;
    pProc = (ChildProc *)m_childrenList.begin();
    while( pProc )
    {
        if (( pProc->m_iState == CP_RUNNING )&&( pProc->m_pid > 0 ) )
        {
            safe_snprintf( achPid, 20, "%d", pProc->m_pid );
            int pid = fork();
            if ( pid < 0)
                LOG_ERR(( "fork() failed" ));
            else if ( pid == 0 )
            {
                dup2( fd, STDOUT_FILENO );
                putenv( achPath );
                pid = execvp( "ps", psCmd );
                if ( D_ENABLED( DL_LESS ) )
                    LOG_D(( "Failed to execute 'ps' command: %s ", strerror( errno ) ));
                exit( 0 );
            }
            else
                waitpid( pid, NULL, 0 );
        }
        pProc = (ChildProc *)pProc->next();
    }

}

#define DEFAULT_XML_CONFIG_FILE         "conf/httpd_config.xml"
#define DEFAULT_PLAIN_CONFIG_FILE       "conf/httpd_config.conf"

int LshttpdMain::testServerRoot( const char * pRoot )
{
    struct stat st;
    if ( nio_stat( pRoot, &st ) == -1 )
        return -1;
    if ( !S_ISDIR( st.st_mode ) )
        return -1;
    char achBuf[MAX_PATH_LEN] = {0};
    
    int confType = -1;
    if ( GPath::getAbsoluteFile(achBuf, MAX_PATH_LEN, pRoot,
                            DEFAULT_XML_CONFIG_FILE) == 0 )
    {
        if ( access( achBuf, R_OK ) == 0 )
        {
            m_pBuilder->setConfigFilePath( achBuf );
            confType = 0; //XML
        }
    }
    
    if ( confType == -1 )
    {
        if ( GPath::getAbsoluteFile(achBuf, MAX_PATH_LEN, pRoot,
                            DEFAULT_PLAIN_CONFIG_FILE) == 0 )
        {
            if ( access( achBuf, R_OK ) == 0 )
            {
                m_pBuilder->setPlainConfigFilePath( achBuf );
                confType = 1;
            }
        }
    }   
    
    if (confType == -1)
        return -1;
    
    int len = strlen( pRoot );
    if ( pRoot[len-1] == '/' )
        achBuf[len] = 0;
    else
        achBuf[++len] = 0;
    if ( GPath::checkSymLinks( achBuf, &achBuf[len],
                    &achBuf[MAX_PATH_LEN], achBuf, 1 ) == -1 )
        return -1;
    m_pBuilder->setServerRoot( achBuf );
    
    //load the config
    if (confType == 0)
        m_pBuilder->loadConfigFile();
    else
        m_pBuilder->loadPlainConfigFile();

    return 0;
}

int LshttpdMain::getServerRootFromExecutablePath( const char * command, char * pBuf, int len )
{
    char achBuf[512];
    if ( *command != '/' )
    {
        getcwd( achBuf, 512 );
        strcat( achBuf, "/" );
        strcat( achBuf, command );
    }
    else
        strcpy( achBuf, command );
    char * p = strrchr( achBuf, '/' );
    if ( p )
        *(p+1) = 0;
    strcat( p, "../" );
    GPath::clean( achBuf );
    memccpy( pBuf, achBuf, 0, len );
    return 0;

}

int LshttpdMain::guessCommonServerRoot()
{
    const char * pServerRoots[] =
    {
        NULL,
        "/usr/local/",
        "/opt/",
        "/usr/",
        "/var/",
        "/home/",
        "/usr/share/",
        "/usr/local/share/"
    };
    const char * pServerDirs[] =
    {
        "lsws/",
        "lshttpd/"
    };
    char * pHome = getenv( "HOME" );
    pServerRoots[0] = pHome;
    char achBuf[MAX_PATH_LEN];
    for( size_t i = 0; i < sizeof( pServerRoots ) / sizeof( char *); ++i )
    {
        if ( !pServerRoots[i] )
            continue;
        strcpy( achBuf, pServerRoots[i] );
        for( size_t j = 0; j < sizeof( pServerDirs ) / sizeof( char *); ++j )
        {
            strcat( achBuf, pServerDirs[j] );
            if ( testServerRoot( achBuf ) == 0 )
                return 0;
        }
    }
    return -1;
}


int LshttpdMain::getServerRoot( int argc, char * argv[] )
{
    char achServerRoot[MAX_PATH_LEN];

    char pServerRootEnv[2][20] = {"LSWS_HOME", "LSHTTPD_HOME"};
    for( int i = 0; i < 2; ++i )
    {
        const char * pRoot = getenv( pServerRootEnv[i] );
        if ( pRoot )
        {
            if ( testServerRoot( pRoot ) == 0 )
            {
                return 0;
            }
        }
    }
    if ( getServerRootFromExecutablePath( argv[0],
                achServerRoot, MAX_PATH_LEN ) == 0 )
    {
        if ( testServerRoot( achServerRoot ) == 0 )
        {
            return 0;
        }
    }
    if ( guessCommonServerRoot() == 0 )
        return 0;
    return -1;
}



int LshttpdMain::config()
{
    if ( m_pBuilder->initServer() != 0 )
        return 1;
    if ( m_pServer->isServerOk() )
        return 2;
//    if ( HttpGlobals::s_psChroot )
//    {
//        mkdir( DEFAULT_TMP_DIR,  0755 );
//        PidFile PidFile;
//        PidFile.writePidFile( PID_FILE, m_pid );
//    }
    m_pServer->generateStatusReport();
    return 0;

}

int LshttpdMain::reconfig()
{
    if ( m_pBuilder->initServer( 1 ) != 0 )
    {
        LOG_WARN(( "Reconfiguration failed, server is restored to "
                   "the state before reconfiguration as much as possible, "
                   "if any problem, please restart server!" ));
    }
    else
    {
        LOG_NOTICE(( "Reconfiguration succeed! " ));
    }
    m_pServer->generateStatusReport();
    return 0;
}


static void perr( const char * pErr )
{
    fprintf( stderr,  "[ERROR] %s\n", pErr );
}

int LshttpdMain::testRunningServer()
{
    int count = 0;
    int ret;
    do
    {
        ret = m_pidFile.lockPidFile( PID_FILE );
        if ( ret )
        {
            if (( ret == -2 )&&( errno == EACCES || errno == EAGAIN ))
            {
                ++count;
                if ( count >= 10 )
                {
                    perr("LiteSpeed Web Server is running!" );
                    return 2;
                }
                my_sleep( 100 );
            }
            else
            {
                fprintf( stderr, "[ERROR] Failed to write to pid file:%s!\n", PID_FILE );
                return ret;
            }
        }
        else
            break;
    }while( true );
    return ret;
}


void LshttpdMain::parseOpt( int argc, char *argv[] )
{
    const char * opts = "cdnv";
    int c;
    char achCwd[512];
    getServerRootFromExecutablePath( argv[0], achCwd, 512 );
    opterr = 0;
    while ( ( c = getopt( argc, argv, opts )) != EOF )
    {
        switch (c) {
        case 'c':
            m_noCrashGuard = 1;
            break;
        case 'd':
            m_noDaemon = 1;
            m_noCrashGuard = 1;
            break;
        case 'n':
            m_noDaemon = 1;
            break;
        case 'v':
            printf( "%s\n", HttpServerVersion::getVersion() );
            exit( 0 );
            break;
        case '?':
            break;

        default:
            printf ("?? getopt returned character code -%o ??\n", c);
        }
    }
}




static void enableCoreDump()
{
    struct  rlimit rl;
    if ( getrlimit(RLIMIT_CORE, &rl ) == -1 )
        LOG_WARN(( "getrlimit( RLIMIT_CORE, ...) failed!" ));
    else
    {
        //LOG_D(( "rl.rlim_cur=%d, rl.rlim_max=%d", rl.rlim_cur, rl.rlim_max ));
        if (( getuid() == 0 )&&( rl.rlim_max < 10240000 ))
            rl.rlim_max = 10240000;
        rl.rlim_cur = rl.rlim_max;
        ::setrlimit( RLIMIT_CORE, &rl );
    }

    ::chdir( DEFAULT_TMP_DIR );
}

void LshttpdMain::changeOwner()
{
    chown( DEFAULT_TMP_DIR, HttpGlobals::s_uid, HttpGlobals::s_gid );
    //chown( PID_FILE, HttpGlobals::s_uid, HttpGlobals::s_gid );
    chown( DEFAULT_SWAP_DIR, HttpGlobals::s_uid, HttpGlobals::s_gid );
}

void LshttpdMain::removeOldRtreport()
{
    char achBuf[8192];
    int i = 1;
    int ret, len;
    m_pBuilder->getCurConfigCtx()->getAbsolute( achBuf, RTREPORT_FILE, 0 );
    len = strlen( achBuf );
    while( 1 )
    {
        ret = ::unlink( achBuf );
        if ( ret == -1 )
            break;
        ++i;
        achBuf[len] = '.';
        snprintf( &achBuf[len+1],sizeof( achBuf ) - len -1, "%d", i );
    };
}

int LshttpdMain::init(int argc, char * argv[])
{
    int ret;
    if ( argc > 1 )
    {
        parseOpt( argc, argv );
    }
    if ( getServerRoot( argc, argv ) != 0 )
    {
        //LOG_ERR(("Failed to determine the root directory of server!" ));
        fprintf( stderr, "Can't determine the Home of LiteSpeed Web Server, exit!\n" );
        return 1;
    }

    mkdir( DEFAULT_TMP_DIR,  0755 );

    if ( testRunningServer() != 0 )
        return 2;
    if ( m_pBuilder->configServerBasics() )
        return 1;
    LOG4CXX_NS::LogRotate::roll( HttpLog::getErrorLogger()->getAppender(),
                     HttpGlobals::s_uid, HttpGlobals::s_gid, 1 );

    m_pBuilder->releaseConfigXmlTree();    
    
    if ( HttpGlobals::s_uid <= 10 || HttpGlobals::s_gid < 10 )
    {
        LOG_ERR(( "It is not allowed to run LiteSpeed web server on behalf of a "
                "privileged user/group, user id must not be "
                "less than 50 and group id must not be less than 10."
                "UID of user '%s' is %d, GID of group '%s' is %d. "
                "Please fix above problem first!",
                m_pBuilder->getUser(), HttpGlobals::s_uid,
                m_pBuilder->getGroup(), HttpGlobals::s_gid ));
        return 1;
    }
    changeOwner();
    LOG_NOTICE(( "Loading %s ...", HttpServerVersion::getVersion() ));
    if ( !m_noDaemon )
    {
        if ( Daemonize::daemonize( 1, 1 ) )
            return 3;
        LOG_D(( "Daemonized!" ));
#ifndef RUN_TEST
        Daemonize::close();
#endif
    }

    enableCoreDump();


    if ( testRunningServer() != 0 )
        return 2;
    m_pid = getpid();
    if ( m_pidFile.writePid( m_pid ) )
        return 2;
    startAdminSocket();
    ret = config();
    if ( ret )
    {
        LOG_ERR(("Fatal error in configuration, exit!" ));
        fprintf( stderr, "[ERROR] Fatal error in configuration, shutdown!\n" );
        return ret;
    }
    removeOldRtreport();
    {
        char achBuf[8192];

        if ( HttpGlobals::s_psChroot )
        {
            PidFile pidfile;
            m_pBuilder->getCurConfigCtx()->getAbsolute( achBuf, PID_FILE, 0 );
            pidfile.writePidFile( achBuf, m_pid);
        }
    }
    
#if defined(__FreeBSD__)
    //setproctitle( "%s", "lshttpd" );
#else
    argv[1] = NULL;
    strcpy( argv[0], "openlitespeed (lshttpd - main)" );
#endif    
    
    if ( !m_noCrashGuard && ( m_pBuilder->getCrashGuard() ))
    {
        if ( guardCrash() )
            return 8;
        m_pidFile.closePidFile();
    }
    else
    {
        HttpGlobals::s_iProcNo = 1;
        allocatePidTracker();
        m_pServer->initAdns();
    }
    //if ( fcntl( 5, F_GETFD, 0 ) > 0 )
    //    printf( "find it!\n" );
    if ( getuid() == 0 )
    {
        if ( m_pBuilder->changeUserChroot() == -1 )
            return -1;
        if ( HttpGlobals::s_psChroot )
            m_pServer->offsetChroot();
    }

    if ( 1 == HttpGlobals::s_iProcNo )
    {
        ExtAppRegistry::runOnStartUp();
    }
    
    return 0;
}


int LshttpdMain::main( int argc, char * argv[] )
{
    argv0 = argv[0];

    VMemBuf::initAnonPool();
    umask( 022 );
    HttpLog::init();
#ifdef RUN_TEST
    if (( argc == 2 )&&( strcmp( argv[1], "-x" ) == 0))
    {
        m_pBuilder->testDomainMod();

        allocatePidTracker();
        m_pServer->initAdns();
        m_pServer->test_main( );
    }
    else
#endif
    {

        //atexit( releaseServer );
        int ret = init( argc, argv);
        if ( ret != 0 )
            return ret;
        m_pServer->start();
        m_pServer->releaseAll();
    }
    return 0;
}


static void sigchild( int sig )
{
    //printf( "signchild()!\n" );
    HttpSignals::orEvent( HS_CHILD );
}



#define BB_SIZE 32768
int LshttpdMain::allocatePidTracker()
{
    char * pBuf =allocateBlackBoard();
    if ( pBuf == MAP_FAILED )
    {
        LOG_ERR_CODE(( errno ));
        return -1;

    }
    m_pServer->setBlackBoard( pBuf );
    return 0;
}


char * LshttpdMain::allocateBlackBoard()
{
    char * pBuf =( char*) mmap( NULL, BB_SIZE, PROT_READ | PROT_WRITE,
        MAP_ANON | MAP_SHARED, -1, 0 );
    return pBuf;
}

void LshttpdMain::deallocateBlackBoard( char * pBuf )
{
    munmap( pBuf, BB_SIZE );
}


void LshttpdMain::releaseExcept( ChildProc * pCurProc )
{
    ChildProc * pProc;
    pProc = (ChildProc *)m_childrenList.pop();
    while( pProc )
    {
        if (( pProc != pCurProc )&&( pProc->m_pBlackBoard ))
        {
            deallocateBlackBoard( pProc->m_pBlackBoard );
        }
        m_pool.recycle( pProc );
        pProc = (ChildProc *)m_childrenList.pop();
    }
}

int LshttpdMain::getFirstAvailSlot()
{
    int mask = 2;
    int * pState;
    int i;
    if ( HttpGlobals::s_children >= 32 )
        pState = m_pProcState;
    else
        pState = (int *)&m_pProcState;
    for( i = 1; i <= HttpGlobals::s_children; ++i )
    {
        if ( !( *pState & mask ) )
            break;
        mask <<= 1;
        if ( mask == 0 )
        {
            ++pState;
            mask = 1;
        }
    }
    return i;
}


void LshttpdMain::setChildSlot( int num, int val )
{
    int * pState;
    if ( num > HttpGlobals::s_children )
        return;
    if ( HttpGlobals::s_children >= 32 )
        pState = m_pProcState;
    else
        pState = (int*)&m_pProcState;
    pState += num >> 5;
    int mask = 1 << ( num & 31 );
    *pState = (val)? *pState | mask : *pState & ~mask;

}


int LshttpdMain::startChild( ChildProc * pProc )
{
    if ( !pProc->m_pBlackBoard )
    {
        pProc->m_pBlackBoard = allocateBlackBoard();
        if ( !pProc->m_pBlackBoard )
            return -1;
    }
    pProc->m_iProcNo = getFirstAvailSlot();
    if ( pProc->m_iProcNo > HttpGlobals::s_children )
        return -1;
    preFork();
    pProc->m_pid = fork();
    if ( pProc->m_pid == -1 )
    {
        forkError( errno );
        return -1;
    }
    if ( pProc->m_pid == 0 )
    {   //child process
        m_pServer->setBlackBoard( pProc->m_pBlackBoard );
        m_pServer->setProcNo( pProc->m_iProcNo );
        releaseExcept( pProc );
        m_pServer->reinitMultiplexer();
        close( m_fdAdmin );
        snprintf( argv0, 80, "openlitespeed (lshttpd - #%02d)", pProc->m_iProcNo );
        
        return 0;
    }
    postFork( pProc->m_pid );
    m_childrenList.push( pProc );
    pProc->m_iState = CP_RUNNING;
    setChildSlot( pProc->m_iProcNo, 1 );
    ++m_curChildren;
    return pProc->m_pid;
}


void LshttpdMain::waitChildren()
{
    int rpid;
    int  stat;
    int ret;
    //printf( "waitpid()\n" );
    while(( rpid = ::waitpid( -1, &stat, WNOHANG ) )> 0 )
    {
        if ( WIFEXITED( stat ) )
        {
            ret = WEXITSTATUS( stat );
            if ( childDead( rpid ) )
                childExit( rpid, ret );
        }
        else if ( WIFSIGNALED( stat ))
        {
            int sig_num = WTERMSIG( stat );
            ret = childSignaled( rpid, sig_num,
#ifdef WCOREDUMP
                                         WCOREDUMP( stat )
#else
                                         -1
#endif
                                         );
            childDead( rpid );
        }
    }
}

int LshttpdMain::cleanUp( int pid, char * pBB )
{
    if ( pBB )
    {
        LOG_NOTICE(( "[AutoRestarter] cleanup children processes and "
                    "unix sockets belong to process %d !", pid ));
        ((ServerInfo *)pBB)->cleanUp();
    }
    return 0;
}

int LshttpdMain::checkRestartReq( )
{
    ChildProc * pProc;
//    LinkedObj * pPrev = m_childrenList.head();
    pProc = (ChildProc *)m_childrenList.begin();
    while( pProc )
    {
        if ( ((ServerInfo *)pProc->m_pBlackBoard)->getRestart() )
        {
            LOG_NOTICE(( "Child Process:%d request a graceful server restart ...",
                         pProc->m_pid ));
            const char * pAdminEmails = m_pBuilder->getAdminEmails();
            if (( pAdminEmails )&&( *pAdminEmails ))
            {
                char achSubject[512];
                static struct utsname      s_uname;
                memset( &s_uname, 0, sizeof( s_uname ) );
                if ( uname( &s_uname ) == -1)
                    LOG_WARN(( "uname() failed!" ));
                safe_snprintf( achSubject, sizeof( achSubject ) - 1,
                        "LiteSpeed Web server %s on %s restarts "
                        "automatically to fix 503 Errors",
                        m_pBuilder->getServerName(), s_uname.nodename );
                EmailSender::send(
                    achSubject, pAdminEmails, "" );
            }
            gracefulRestart();
            return 0;
        }
//        pPrev = pProc;
        pProc = (ChildProc *)pProc->next();
    }
    return 0;
}


int LshttpdMain::childDead( int pid )
{
    ChildProc * pProc;
    LinkedObj * pPrev = m_childrenList.head();
    pProc = (ChildProc *)m_childrenList.begin();
    while( pProc )
    {
        if ( pProc->m_pid == pid )
        {
            cleanUp( pid, pProc->m_pBlackBoard );
            if ( pProc->m_iState == CP_RUNNING )
            {
                setChildSlot( pProc->m_iProcNo, 0 );
                --m_curChildren;
            }
            m_childrenList.removeNext( pPrev );
            m_pool.recycle( pProc );
            return 1;
        }
        else
        {
            pPrev = pProc;
            pProc = (ChildProc *)pProc->next();
        }
    }
    return 0;
}


int LshttpdMain::clearToStopApp()
{
    ChildProc * pProc;
    pProc = (ChildProc *)m_childrenList.begin();
    while( pProc )
    {
        ((ServerInfo *)pProc->m_pBlackBoard )->cleanPidList( 1 );
        pProc = (ChildProc *)pProc->next();
    }
    return 0;
}


void LshttpdMain::broadcastSig( int sig, int changeState )
{
    ChildProc * pProc;
    pProc = (ChildProc *)m_childrenList.begin();
    while( pProc )
    {
        if ( pProc->m_pid > 0 )
            kill( pProc->m_pid, sig );
        if (( changeState )&&( pProc->m_iState == CP_RUNNING ))
        {
            pProc->m_iState = CP_SHUTDOWN;
            setChildSlot( pProc->m_iProcNo, 0 );
            --m_curChildren;
        }
        pProc = (ChildProc *)pProc->next();
    }
}


void LshttpdMain::stopAllChildren()
{
    ChildProc * pProc;
    broadcastSig( SIGTERM, 0 );

    long tmBeginWait = time(NULL);
    while(( m_childrenList.size() > 0 )&&
          ( time(NULL) - tmBeginWait
                < HttpServerConfig::getInstance().getConnTimeout() ))
    {
        sleep( 1 );
        waitChildren();
    }

    if ( m_childrenList.size() > 0 )
    {
        pProc = (ChildProc *)m_childrenList.begin();
        while( pProc )
        {
            kill( pProc->m_pid, SIGKILL );
            cleanUp( pProc->m_pid, pProc->m_pBlackBoard );
            pProc = (ChildProc *)pProc->next();
        }
    }
}


void LshttpdMain::applyChanges()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( "Applying new configuration. " ));
    broadcastSig( SIGTERM, 1 );
}

void LshttpdMain::gracefulRestart()
{
    if ( D_ENABLED( DL_LESS ) )
        LOG_D(( "Graceful Restart... " ));
    close( m_fdAdmin );
    broadcastSig( SIGTERM, 1 );
    s_iRunning = 0;
    m_pidFile.closePidFile();
    m_pServer->passListeners();
    int pid = fork();
    if ( !pid )
    {
        char achCmd[1024];
        int fd = HttpGlobals::getStdErrLogger()->getStdErr();
        if ( fd != 2 )
            close( fd );
        int len = getFullPath( "bin/litespeed", achCmd, 1024 );
        achCmd[len - 10] = 0;
        chdir( achCmd );
        achCmd[len - 10] = '/';
        if ( execl( achCmd, "litespeed", NULL  ) )
        {
            LOG_ERR(( "Failed to start new instance of LiteSpeed Web server!" ));
        }
        exit( 0 );
    }
    if ( pid == -1 )
        LOG_ERR(( "Failed to restart the server!" ));
}


static void startTimer()
{
    struct itimerval tmv;
    memset( &tmv, 0, sizeof( struct itimerval ) );
    tmv.it_interval.tv_sec = 1;
    gettimeofday( &tmv.it_value, NULL );
    tmv.it_value.tv_sec = 0;
    tmv.it_value.tv_usec = 1000000 - tmv.it_value.tv_usec;
    setitimer (ITIMER_REAL, &tmv, NULL);
}


int LshttpdMain::guardCrash()
{
    long lLastForkTime  = DateTime::s_curTime = time(NULL);
    int  iForkCount     = 0;
    int  ret;
    struct pollfd   pfds[2];
    HttpSignals::init( sigchild );
    if ( HttpGlobals::s_children >= 32 )
    {
        m_pProcState = (int *)malloc( ((HttpGlobals::s_children >> 5) + 1) * sizeof(int));
    }
    startAdminSocket();
    pfds[0].fd = m_fdAdmin;
    pfds[0].events = POLLIN;
    pfds[1].fd = -1;
    pfds[1].events = 0;
    s_iRunning = 1;
    startTimer();
    while( s_iRunning )
    {
        if ( DateTime::s_curTime - lLastForkTime > 60 )
        {
            iForkCount = 0;
            lLastForkTime = DateTime::s_curTime;
        }
        else
        {

            if ( iForkCount > ( HttpGlobals::s_children << 3 ) )
            {
                ret = forkTooFreq();
                if ( ret > 0 )
                {
                    break;
                }
                iForkCount = -1;
            }
            else if ( iForkCount >= 0 )
            {
                while( m_curChildren < HttpGlobals::s_children )
                {
                    ++iForkCount;
                    ChildProc * pProc = m_pool.get();
                    if ( pProc )
                    {
                        ret = startChild( pProc );
                        if ( ret == 0 )  //children process
                        {
                            return 0;
                        }
                        else if ( ret == -1 )
                            m_pool.recycle( pProc );
                    }
                }
            }
        }
        if ( m_fdAdmin != -1 )
        {
            ret = ::poll( pfds, 1, 1000 );
            if ( ret > 0 )
            {
                if ( pfds[0].revents )
                    acceptAdminSockConn();
                continue;
            }
            
        }
        else
            ::sleep( 1 );
        if ( HttpSignals::gotEvent() )
        {
            processSignal();
        }
    }
    if ( m_childrenList.size() > 0 )
        stopAllChildren();
    HttpLog::notice( "[PID:%d] Server Stopped!\n", getpid() );
    exit( ret );
}

void LshttpdMain::processSignal()
{
    if ( HttpSignals::gotSigAlarm() )
        onGuardTimer();
    if ( HttpSignals::gotSigStop() )
    {
        LOG_NOTICE(( "SIGTERM received, stop server..."));
        s_iRunning = 0;
    }
    if ( HttpSignals::gotSigUsr1() || HttpSignals::gotSigHup() )
    {
        LOG_NOTICE(( "Server Restart Request via Signal..."));
        gracefulRestart();
    }
//     if ( HttpSignals::gotSigHup() )
//     {
//         LOG_NOTICE(( "SIGHUP received, Reloading configuration file..."));
//         if ( reconfig() == -1)
//         {
//             s_iRunning = 0;
//             return;
//         }
//         applyChanges();
//     }
    if ( HttpSignals::gotSigChild() )
        waitChildren();
    HttpSignals::resetEvents();
}

