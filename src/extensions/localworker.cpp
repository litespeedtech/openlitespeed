/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include "localworker.h"

#include "localworkerconfig.h"
#include "pidlist.h"
#include "cgi/suexec.h"
#include "registry/extappregistry.h"

#include <http/httpvhost.h>
#include <http/serverprocessconfig.h>
#include <log4cxx/logger.h>
#include <lsr/ls_fileio.h>
#include <main/configctx.h>
#include <main/mainserverconfig.h>
#include <main/serverinfo.h>
#include <socket/gsockaddr.h>
#include <util/datetime.h>
#include <util/env.h>

#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <fcntl.h>
#include <util/ni_fio.h>
#include <socket/coresocket.h>
#include <http/httpstatuscode.h>

#define GRACE_TIMEOUT 20
#define KILL_TIMEOUT 25

time_t LocalWorker::s_tmRestartPhp = 0;

LocalWorker::LocalWorker(int type)
    : ExtWorker(type)
    , m_fdApp(-1)
    , m_sigGraceStop(SIGTERM)
    , m_tmResourceLimited(0)
    , m_forceStop(0)
    , m_pidList(NULL)
    , m_pidListStop(NULL)
    , m_pRestartMarker(NULL)
    , m_pDetached(NULL)
{
    m_pidList = new PidList();
    m_pidListStop = new PidList();

}


LocalWorkerConfig &LocalWorker::getConfig() const
{   return *(static_cast<LocalWorkerConfig *>(getConfigPointer()));  }


LocalWorker::~LocalWorker()
{
    if (m_pidList)
        delete m_pidList;
    if (m_pidListStop)
        delete m_pidListStop;
}


static void killProcess(pid_t pid)
{
    if (((kill(pid, SIGTERM) == -1) && (errno == EPERM)) ||
        ((kill(pid, SIGUSR1) == -1) && (errno == EPERM)))
        PidRegistry::markToStop(pid, KILL_TYPE_TERM);
}


void LocalWorker::moveToStopList()
{
    PidList::iterator iter;
    for (iter = m_pidList->begin(); iter != m_pidList->end();
         iter = m_pidList->next(iter))
        m_pidListStop->add((int)(long)iter->first(), DateTime::s_curTime);
    m_pidList->clear();
}


void LocalWorker::moveToStopList(int pid)
{
    PidList::iterator iter = m_pidList->find((void *)(long)pid);
    if (iter != m_pidList->end())
    {
        killProcess(pid);
        m_pidListStop->add(pid, DateTime::s_curTime - GRACE_TIMEOUT);
        m_pidList->erase(iter);
    }
}


void LocalWorker::cleanStopPids()
{
    if ((m_pidListStop) &&
        (m_pidListStop->size() > 0))
    {
        pid_t pid;
        PidList::iterator iter, iterDel;
        for (iter = m_pidListStop->begin(); iter != m_pidListStop->end();)
        {
            pid = (pid_t)(long)iter->first();
            long delta = DateTime::s_curTime - (long)iter->second();
            int sig = 0;
            iterDel = iter;
            iter = m_pidListStop->next(iter);
            if (delta > GRACE_TIMEOUT)
            {
                if ((kill(pid, 0) == -1) && (errno == ESRCH))
                {
                    m_pidListStop->erase(iterDel);
                    PidRegistry::remove(pid);
                    continue;
                }
                if (delta > KILL_TIMEOUT)
                {
                    sig = SIGKILL;
                    LS_NOTICE("[%s] Send SIGKILL to process [%d] that won't stop.",
                              getName(), pid);
                }
                else
                {
                    sig = m_sigGraceStop;
                    LS_NOTICE("[%s] Send SIGTERM to process [%d].",
                              getName(), pid);
                }
                if (kill(pid , sig) != -1)
                    LS_DBG_L("[%s] kill pid: %d", getName(), pid);
                else if (errno == EPERM)
                    PidRegistry::markToStop(pid, KILL_TYPE_TERM);
            }
        }
    }
}


void LocalWorker::detectDiedPid()
{
    PidList::iterator iter;
    for (iter = m_pidList->begin(); iter != m_pidList->end();)
    {
        pid_t pid = (pid_t)(long)iter->first();
        if ((kill(pid, 0) == -1) && (errno == ESRCH))
        {
            LS_INFO("Process with PID: %d is dead ", pid);
            PidList::iterator iterNext = m_pidList->next(iter);
            m_pidList->erase(iter);
            PidRegistry::remove(pid);

            iter = iterNext;

        }
        else
            iter = m_pidList->next(iter);
    }
}


void LocalWorker::addPid(pid_t pid)
{
    m_pidList->insert((void *)(unsigned long)pid, this);
}


void LocalWorker::removePid(pid_t pid)
{
    m_pidList->remove(pid);
    m_pidListStop->remove(pid);
}


int LocalWorker::selfManaged() const
{   return getConfig().getSelfManaged();    }


int LocalWorker::runOnStartUp()
{
    if (getConfig().getRunOnStartUp())
        return startEx();
    return 0;
}


int LocalWorker::startOnDemond(int force)
{
    if (!m_pidList)
        return LS_FAIL;
    int nProc = m_pidList->size();
    if ((getConfig().getRunOnStartUp()) && (nProc > 0))
        return 0;
    if (force)
    {
        if (nProc >= getConfig().getInstances())
        {
//            if ( getConfig().getInstances() > 1 )
//                return restart();
//            else
            return LS_FAIL;
        }
    }
    else
    {
        if (nProc >= getConnPool().getTotalConns())
            return 0;
        if ((nProc == 0)
            && (getConnPool().getTotalConns() > 2))     //server socket is in use.
            return 0;
    }
    return startEx();
}

int LocalWorker::stopDetachedWorker()
{
    if (!getConfig().isDetached())
        return -1;
    if (!m_pDetached)
        return 0;
    if (isDetachedAlreadyRunning())
        killOldDetachedInstance(&m_pDetached->pid_info);
    else
        removeOldDetachedSocket();
    setState(ST_NOTSTARTED);
    return 0;
}


int LocalWorker::stop()
{
    if (getConfig().isDetached() && m_pDetached)
        return -1;
    pid_t pid;
    int ret;
    PidList::iterator iter;
    if (getConnPool().getTotalConns() > 0)
        LS_NOTICE("[%s] stop worker processes", getName());
    ret = clearCurConnPool();
    removeUnixSocket();
    if (ret <= 0)
    {
        for (iter = getPidList()->begin(); iter != getPidList()->end();)
        {
            pid = (pid_t)(long)iter->first();
            iter = getPidList()->next(iter);
            killProcess(pid);
            LS_DBG_L("[%s] kill pid: %d", getName(), pid);
        }
    }
    else
    {
        if (LS_LOG_ENABLED(log4cxx::Level::DBG_LOW))
            LS_INFO("[%s] %d request being processed, kill external app later.",
                    getName(), ret);
    }
    moveToStopList();
    setState(ST_NOTSTARTED);

    return 0;
}


void LocalWorker::removeUnixSocket()
{
    const GSockAddr &addr = ((LocalWorkerConfig *)
                             getConfigPointer())->getServerAddr();
    if ((m_fdApp >= 0) && (getPidList()->size() > 0) &&
        (addr.family() == PF_UNIX))
    {
        LS_DBG_L("[%s] remove unix socket: %s", getName(),
                 addr.getUnix());
        unlink(addr.getUnix());
        close(m_fdApp);
        m_fdApp = -2;
        getConfigPointer()->altServerAddr();
    }
}


int LocalWorker::addNewProcess()
{
    if ((getConfigPointer()->getURL()) &&
        (((LocalWorkerConfig *)getConfigPointer())->getCommand()))
        return startEx();
    return 1;
}

//every 10 seconds timer
void LocalWorker::onTimer()
{
    if (m_pRestartMarker && m_pDetached && isDetachedAlreadyRunning() &&
        m_pRestartMarker->checkRestart(DateTime::s_curTime))
        restart();
    checkAndStopWorker();
}

int LocalWorker::tryRestart()
{
    if (DateTime::s_curTime - getLastRestart() > 10)
    {
        LS_NOTICE("[%s] try to fix 503 error by restarting external application",
                  getName());
        return restart();
    }
    return 0;
}


int LocalWorker::restart()
{
    setLastRestart(DateTime::s_curTime);
    clearCurConnPool();
    if ((getConfigPointer()->getURL()) &&
        (((LocalWorkerConfig *)getConfigPointer())->getCommand()))
    {
        removeUnixSocket();
        moveToStopList();
        return start();
    }
    return 1;
}


int LocalWorker::getCurInstances() const
{   return getPidList()->size();  }


// void LocalWorker::setPidList( PidList * l)
// {
//     m_pidList = l;
//     if (( l )&& !m_pidListStop )
//         m_pidListStop = new PidList();
// }


// static int workerSUExec( LocalWorkerConfig& config, int fd )
// {
//     const HttpVHost * pVHost = config.getVHost();
//     if (( !HttpGlobals::s_pSUExec )||( !pVHost ))
//         return LS_FAIL;
//     int mode = pVHost->getRootContext().getSetUidMode();
//     if ( mode != UID_DOCROOT )
//         return LS_FAIL;
//     uid_t uid = pVHost->getUid();
//     gid_t gid = pVHost->getGid();
//     if (( uid == HttpGlobals::s_uid )&&
//         ( gid == HttpGlobals::s_gid ))
//         return LS_FAIL;
//
//     if (( uid < HttpGlobals::s_uidMin )||
//         ( gid < HttpGlobals::s_gidMin ))
//     {
//         LS_INFO( "[VHost:%s] Fast CGI [%s]: suExec access denied,"
//                     " UID or GID of VHost document root is smaller "
//                     "than minimum UID, GID configured. ", pVHost->getName(),
//                     config.getName() ));
//         return LS_FAIL;
//     }
//     const char * pChroot = NULL;
//     int chrootLen = 0;
// //    if ( HttpGlobals::s_psChroot )
// //    {
// //        pChroot = HttpGlobals::s_psChroot->c_str();
// //        chrootLen = HttpGlobals::s_psChroot->len();
// //    }
//     char achBuf[4096];
//     memccpy( achBuf, config.getCommand(), 0, 4096 );
//     char * argv[256];
//     char * pDir ;
//     SUExec::buildArgv( achBuf, &pDir, argv, 256 );
//     if ( pDir )
//         *(argv[0]-1) = '/';
//     else
//         pDir = argv[0];
//     HttpGlobals::s_pSUExec->prepare( uid, gid, config.getPriority(),
//           pChroot, chrootLen,
//           pDir, strlen( pDir ), config.getRLimits() );
//     int rfd = -1;
//     int pid = HttpGlobals::s_pSUExec->suEXEC( HttpGlobals::s_pServerRoot, &rfd, fd, argv,
//                 config.getEnv()->get(), NULL );
// //    if ( pid != -1)
// //    {
// //        char achBuf[2048];
// //        int ret;
// //        while( ( ret = read( rfd, achBuf, 2048 )) > 0 )
// //        {
// //            write( 2, achBuf, ret );
// //        }
// //    }
//     if ( rfd != -1 )
//         close( rfd );
//
//     return pid;
// }


int LocalWorker::workerExec(LocalWorkerConfig &config, int fd)
{
    ServerProcessConfig &procConfig = ServerProcessConfig::getInstance();
    if (SUExec::getSUExec() == NULL)
        return LS_FAIL;
    uid_t uid;
    gid_t gid;
    const HttpVHost *pVHost = config.getVHost();
    uid = config.getUid();
    gid = config.getGid();
    if ((int)uid == -1)
        uid = procConfig.getUid();
    if ((int)gid == -1)
        gid = procConfig.getGid();

    if (pVHost)
    {
        LS_NOTICE("[LocalWorker::workerExec] VHost:%s suExec check "
                "uid %d gid %d setuidmode %d.",
                pVHost->getName(), pVHost->getUid(), pVHost->getGid(),
                pVHost->getRootContext().getSetUidMode());
    }

    if (pVHost && pVHost->getRootContext().getSetUidMode() == UID_DOCROOT)
    {
        uid = pVHost->getUid();
        gid = pVHost->getGid();
        if (procConfig.getForceGid() != 0)
            gid = procConfig.getForceGid();

        if ((uid < procConfig.getUidMin()) ||
            (gid < procConfig.getGidMin()))
        {
            if (LS_LOG_ENABLED(LOG4CXX_NS::Level::DBG_LESS))
                LS_NOTICE("[VHost:%s] Fast CGI [%s]: suExec access denied,"
                        " UID or GID of VHost document root is smaller "
                        "than minimum UID, GID configured. ", pVHost->getName(),
                        config.getName());
            return LS_FAIL;
        }
    }
    //if (( uid == HttpGlobals::s_uid )&&
    //    ( gid == HttpGlobals::s_gid ))
    //    return LS_FAIL;
    const AutoStr2 *pChroot = NULL;
    const char *pChrootPath = NULL;
    int chrootLen = 0;
    int chMode = 0;
    if (pVHost)
    {
        chMode = pVHost->getRootContext().getChrootMode();
        switch (chMode)
        {
        case CHROOT_VHROOT:
            pChroot = pVHost->getVhRoot();
            break;
        case CHROOT_PATH:
            pChroot = pVHost->getChroot();
            if (!pChroot->c_str())
                pChroot = NULL;
        }
        //Since we already in the chroot jail, do not use the global jail path
        //If start external app with lscgid, apply global chroot path,
        //  as lscgid is not inside chroot
        if (config.getStartByServer() == 2)
        {
            if (!pChroot)
                pChroot = procConfig.getChroot();
        }
        if (pChroot)
        {
            pChrootPath = pChroot->c_str();
            chrootLen = pChroot->len();
        }
    }
    char achBuf[4096];
    memccpy(achBuf, config.getCommand(), 0, 4096);
    char *argv[256];
    char *pDir ;
    SUExec::buildArgv(achBuf, &pDir, argv, 256);
    if (pDir)
        *(argv[0] - 1) = '/';
    else
        pDir = argv[0];
    SUExec::getSUExec()->prepare(uid, gid, config.getPriority(),
                                 config.getUmask(),
                                 pChrootPath, chrootLen,
                                 pDir, strlen(pDir), config.getRLimits());

    LS_NOTICE("[LocalWorker::workerExec] Config[%s]: suExec uid %d gid %d cmd %s,"
            " final uid %d gid %d.",
            config.getName(), config.getUid(),
            config.getGid(), config.getCommand(),
            uid, gid);

    int rfd = -1;
    int pid;
    //if ( config.getStartByServer() == 2 )
    //{
    pid = SUExec::getSUExec()->cgidSuEXEC(
              MainServerConfig::getInstance().getServerRoot(), &rfd, fd, argv,
              config.getEnv()->get(), NULL);
    //}
    //else
    //{
    //    pid = HttpGlobals::s_pSUExec->suEXEC(
    //        HttpGlobals::s_pServerRoot, &rfd, fd, argv,
    //        config.getEnv()->get(), NULL );
    //}

    if (pid == -1)
        pid = SUExec::spawnChild(config.getCommand(), fd, -1,
                                 config.getEnv()->get(),
                                 config.getPriority(), config.getRLimits(),
                                 config.getUmask(), uid, gid);
    if (rfd != -1)
        close(rfd);

    return pid;
}


int LocalWorker::startWorker()
{
    int fd = getfd();
//      if (m_tmResourceLimited == DateTime::s_curTime)
//         return -SC_508;
    LocalWorkerConfig &config = getConfig();
    struct stat st;
//    if (( stat( config.getCommand(), &st ) == -1 )||
//        ( access(config.getCommand(), X_OK) == -1 ))
//    {
//        LS_ERROR("Start FCGI [%s]: invalid path to executable - %s,"
//                 " not exist or not executable ",
//                config.getName(),config.getCommand() ));
//        return LS_FAIL;
//    }
//    if ( st.st_mode & S_ISUID )
//    {
//        LS_DBG_L( "Fast CGI [%s]: Setuid bit is not allowed : %s\n",
//                config.getName(), config.getCommand());
//        return LS_FAIL;
//    }
    if (fd < 0)
    {
        fd = ExtWorker::startServerSock(&config, config.getBackLog());
        if (fd != -1)
        {
            setfd(fd);
            if (config.getServerAddr().family() == PF_UNIX)
            {
                ls_fio_stat(config.getServerAddr().getUnix(), &st);
                ServerInfo::getServerInfo()->addUnixSocket(
                    config.getServerAddr().getUnix(), &st);
            }
        }
        else
            return LS_FAIL;
    }
    int instances = config.getInstances();
    int cur_instances = getCurInstances();
    int new_instances = getConnPool().getTotalConns() + 2 - cur_instances;
    if (new_instances <= 0)
        new_instances = 1;
    if (instances < new_instances + cur_instances)
        new_instances = instances - cur_instances;
    if (new_instances <= 0)
        return 0;
    int i;
    for (i = 0; i < new_instances; ++i)
    {
        int pid;
        pid = workerExec(config, fd);
        if (pid > 0)
        {
            LS_NOTICE("[%s] add child process pid: %d.", getName(), pid);
            PidRegistry::add(pid, this, 0);
        }
        else
        {
            LS_ERROR("Start [%s]: failed to start the # %d of %d instances.",
                     config.getName(), i + 1, instances);
            break;
        }
    }
    return (i == 0) ? LS_FAIL : LS_OK;
}

// int LocalWorker::startOneWorker()
// {
//     int pid;
//     int fd;
// //     if (m_tmResourceLimited == DateTime::s_curTime)
// //         return -SC_508;
//     fd = getfd();
//     LocalWorkerConfig &config = getConfig();
//
//     if (fd == -1)
//     {
//         //config.altServerAddr();
//         fd = ExtWorker::startServerSock(&config, config.getBackLog());
//     }
//     pid = workerExec(config, fd);
//     if (pid != 0)
//         return processPid(pid);
//     return 0;
// }


void LocalWorker::configRlimit(RLimits *pRLimits, const XmlNode *pNode)
{
    if (!pNode)
        return;

    pRLimits->setProcLimit(
        ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "procSoftLimit", 0,
                INT_MAX, 0),
        ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "procHardLimit", 0,
                INT_MAX, 0));

    pRLimits->setCPULimit(
        ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "CPUSoftLimit", 0,
                INT_MAX, 0),
        ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "CPUHardLimit", 0,
                INT_MAX, 0));
    long memSoft = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                   "memSoftLimit", 0, LONG_MAX, 0);
    long memHard = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                   "memHardLimit", 0, LONG_MAX, 0);

    if ((memSoft & (memSoft < 1024 * 1024)) ||
        (memHard & (memHard < 1024 * 1024)))
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "Memory limit is too low with %ld/%ld",
                 memSoft, memHard);
    }
    else
        pRLimits->setDataLimit(memSoft, memHard);
}

void LocalWorker::setRestartMarker(const char* path, int reset_me_path_pos)
{
    if (path && *path == '/')
    {
        if (m_pRestartMarker)
        {
            if (m_pRestartMarker->isSamePath(path))
                return;
            delete m_pRestartMarker;
        }
        m_pRestartMarker = new RestartMarker(path);
        m_pRestartMarker->set_lsapi_reset_me_path_pos(reset_me_path_pos);
        m_pRestartMarker->checkRestart(time(NULL));
    }
}




bool LocalWorker::serverLevelRestartPhp()
{
    return (getConfig().isPhpHandler() && m_pDetached
            && m_pDetached->pid_info.last_modify < s_tmRestartPhp);
}


void LocalWorker::checkAndStopWorker()
{
    int s = 0;
    if (getState() == ST_GOOD && m_pRestartMarker)
    {
        if (m_pRestartMarker->checkRestart(DateTime::s_curTime)
            || serverLevelRestartPhp())
        {
            LS_INFO("[%s] detect restart request, stopping ...", getName());
            s = 1;
        }
    }

    if (getState() == ST_GOOD && m_pDetached && m_pDetached->pid_info.pid > 0)
    {
        if (detectBinaryChange())
        {
            LS_INFO("[%s] detected binary change [%s], stopping ...",
                    getName(),
                    ((LocalWorkerConfig *)getConfigPointer())->getCommand());
            s = 2;
        }
    }

    if (!s)
    {
        if (m_forceStop)
        {
            m_forceStop = 0;
            LS_INFO("[%s] force stop requested, stopping ...", getName());
        }
        else
            return;
    }
    if (getConfig().isDetached())
    {
        if (s ==2 ||
            (m_pDetached && m_pDetached->pid_info.pid > 0
            && DateTime::s_curTime - m_pDetached->last_stop_time > 60))
        {
            m_pDetached->last_stop_time = DateTime::s_curTime;
            killOldDetachedInstance(&m_pDetached->pid_info);
        }
    }
    else
    {
        stop();
    }
}



static int lockFile(int fd, short lockType)
{
    int ret;
    struct flock lock;
    lock.l_type = lockType;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    while (1)
    {
        ret = fcntl(fd, F_SETLK, &lock);
        if ((ret == -1) && (errno == EINTR))
            continue;
        if (ret == 0)
            return ret;
        int err = errno;
        lock.l_pid = 0;
        if (fcntl(fd, F_GETLK, &lock) == 0 && lock.l_pid > 0)
            ret = lock.l_pid;
        errno = err;
        return ret;
    }
}


int LocalWorker::openLockPidFile(const char *pSocketPath)
{
    char bufPidFile[4096];
    snprintf(bufPidFile, sizeof(bufPidFile), "%s.pid",
             pSocketPath);
    int fd = nio_open(bufPidFile, O_WRONLY | O_CREAT | O_NOFOLLOW | O_TRUNC, 0644);
    if (fd == -1)
    {
        int err = errno;
        LS_NOTICE("[%s]: Failed to open pid file [%s]: %s",
                 getName(), bufPidFile, strerror(errno));
        errno = err;
        return -1;
    }
    int ret;
    if ((ret = lockFile(fd, F_WRLCK)) != 0)
    {
        int err = errno;
        LS_NOTICE("[%s]: Failed to lock pid file [%s]: %s, locked by PID: %d",
                  getName(), bufPidFile, strerror(errno), ret);
        close(fd);
        errno = err;
        return -1;
    }
    LS_INFO("[%s]: locked pid file [%s].", getName(), bufPidFile);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
}


static int unlockClosePidFile(int fd)
{
    lockFile(fd, F_UNLCK);
    close(fd);
    return 0;
}


int LocalWorker::processPid(int pid, const char * path)
{
    if (pid > 0)
    {
        const char *pLogId = getName();
        if (getConfig().getRunOnStartUp() != EXTAPP_RUNONSTART_DAEMON
            && !getConfig().isDetached())
        {
            //if ( getConfig().getStartByServer() == EXTAPP_AUTOSTART_CGID )
            //    addPid( pid );
            //else
            PidRegistry::add(pid, this, 0);
        }

        LS_NOTICE("[%s] add child process pid: %d", pLogId, pid);
        if (getConfig().isDetached())
        {
            m_pDetached->pid_info.pid = pid;
            saveDetachedPid(m_pDetached->fd_pid_file,
                           &m_pDetached->pid_info, path);
        }

    }
    else
    {

        if (pid <= -600)
            pid = -SC_500;
        if (pid < -100)
            pid = -HttpStatusCode::getInstance().codeToIndex(-pid);
        LS_ERROR("[%s]: Failed to start one instance. pid: %d %s",
                 getName(), pid, (pid == -SC_508) ? "Resource limit reached!" : "");
        if (pid == -SC_508)
            m_tmResourceLimited = DateTime::s_curTime;
        if (getConfig().isDetached())
            setState(ST_NOTSTARTED);
    }
    if (getConfig().isDetached())
    {

        if (m_pDetached->fd_pid_file != -1)
        {
            unlockClosePidFile(m_pDetached->fd_pid_file);
            LS_INFO("[%s]: unlocked pid file [%s.pid].",
                    getName(), getConfig().getServerAddr().getUnix());
            m_pDetached->fd_pid_file = -1;
        }
    }
    return pid;
}


#define DETACHED_PIDINFO_MIN_SIZE 24
bool LocalWorker::loadDetachedPid(int fd, DetachedPidInfo_t *detached_pid)
{
     memset(detached_pid, 0, sizeof(*detached_pid));
    return pread(fd, detached_pid, sizeof(DetachedPidInfo_t), 0)
            >= DETACHED_PIDINFO_MIN_SIZE;
}


void LocalWorker::saveDetachedPid(int fd, DetachedPidInfo_t *detached_pid,
                                 const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0)
    {
        m_pDetached->pid_info.inode = st.st_ino;
        m_pDetached->pid_info.last_modify = st.st_mtime;

        const char *bin_path = ((LocalWorkerConfig *)getConfigPointer())->getCommand();
        if (bin_path && nio_stat(bin_path, &st) == 0)
        {
            m_pDetached->pid_info.bin_last_mod = st.st_mtime;
        }
    }
    pwrite(fd, detached_pid, sizeof(DetachedPidInfo_t), 0);
}

bool LocalWorker::detectBinaryChange()
{
    struct stat st;
    const char *bin_path = ((LocalWorkerConfig *)getConfigPointer())->getCommand();
    if (bin_path && nio_stat(bin_path, &st) == 0)
    {
        if (m_pDetached->pid_info.bin_last_mod != st.st_mtime)
            return true;
    }
    return false;
}


void LocalWorker::removeOldDetachedSocket()
{
    LS_INFO("[%s] remove unix socket for detached process: %s",
            getName(), getConfig().getServerAddr().getUnix());
    unlink( getConfig().getServerAddr().getUnix() );
}


void LocalWorker::killOldDetachedInstance(DetachedPidInfo_t *detached_pid)
{
    LS_INFO("[%s] kill current detached process: %d",
            getName(), m_pDetached->pid_info.pid);
    removeOldDetachedSocket();
    int pid = detached_pid->pid;
    if (pid <= 1)
        return;
    PidRegistry::addMarkToStop(pid, KILL_TYPE_TERM,
                                m_pDetached->pid_info.last_modify);
}



int LocalWorker::isDetachedAlreadyRunning()
{
    char bufPidFile[4096];
    int ret;
    if (m_pDetached)
    {
        if (m_pDetached->last_check_time == DateTime::s_curTime
            && m_pDetached->pid_info.pid > 0)
        {
            LS_DBG_L("[%s] is running as pid: %d.",
                     getName(), m_pDetached->pid_info.pid);
            return 1;
        }
        else if (m_pDetached->fd_pid_file != -1) //starting
        {
            LS_DBG_L("[%s] is starting up by current process.", getName());
            return 2;
        }
    }

    const GSockAddr &service_addr = getConfig().getServerAddr();
    if (service_addr.family() != PF_UNIX)
        return -1;

    struct stat st;
    if (nio_stat(service_addr.getUnix(), &st) == -1)
        return -1;

    snprintf(bufPidFile, sizeof(bufPidFile), "%s.pid",
             service_addr.getUnix());
    int fd = open(bufPidFile, O_RDONLY );
    if (fd == -1)
        return -1;

    if (!m_pDetached)
        m_pDetached = new DetachedProcess_t();
    ret = loadDetachedPid(fd, &m_pDetached->pid_info);
    close(fd);
    if (ret != 1)
    {
        m_pDetached->pid_info.pid = -1;
        return -1;
    }
    if (m_pDetached->pid_info.pid > 0
        && m_pDetached->pid_info.inode == st.st_ino
        && m_pDetached->pid_info.last_modify == st.st_mtime)
    {
        ret = kill(m_pDetached->pid_info.pid, 0);
        if (ret == 0 || errno != ESRCH)
            return 1;
    }
    m_pDetached->pid_info.pid = -1;
    return 0;
}


// return 0    start in progress
// return > 0  started/already running
// return < 0  something wrong, cannot start
//             can be `-HTTP_Status_Code`

int LocalWorker::startDetachedWorker(int force)
{
    int pid;
    int fd;
    int ret;
    int tries = 0;
    if (m_tmResourceLimited == DateTime::s_curTime)
        return -SC_508;

TRY_AGAIN:
    ret = isDetachedAlreadyRunning();
    if (ret == 1)
    {
        if (!force && m_pRestartMarker)
        {
            if (m_pRestartMarker->getLastReset()
                > m_pDetached->pid_info.last_modify)
                force = 1;
        }
        if (force || serverLevelRestartPhp())
        {
            killOldDetachedInstance(&m_pDetached->pid_info);
        }
        else
            return 1;
    }
    else if (ret == 2)
        return 0;

    LocalWorkerConfig &config = getConfig();
    const GSockAddr &service_addr = config.getServerAddr();
    int fd_pid_file = openLockPidFile(service_addr.getUnix());
    if (fd_pid_file == -1)
    {
        if (errno == EAGAIN)
        {
            if (++tries < 5)
            {
                if (tries < 4)
                {
                    LS_NOTICE("[%s]: Could be detached process being started, wait and retry: %d",
                        getName(), tries);
                    usleep(10000);
                }
                else
                {
                    char bufPidFile[4096];
                    snprintf(bufPidFile, sizeof(bufPidFile), "%s.pid",
                             service_addr.getUnix());

                    LS_NOTICE("[%s]: Could be dead lock, remove pid file [%s] and retry",
                              getName(), bufPidFile);
                    unlink(bufPidFile);
                }
                goto TRY_AGAIN;
            }
        }
        LS_ERROR("[%s]: Failed to lock pid file for [%s]: %s",
                 getName(), service_addr.getUnix(), strerror(errno));
        return -1;
    }

    removeOldDetachedSocket();
    ret = CoreSocket::listen(service_addr, config.getBackLog(), &fd, -1, -1);
    if (fd == -1)
    {
        LS_ERROR("[%s]: Failed to listen socket [%s]: %s",
                 getName(), service_addr.getUnix(), strerror(errno));
        unlockClosePidFile(fd_pid_file);
        return -1;
    }
    fcntl(fd, F_SETFD, FD_CLOEXEC);

    if (!m_pDetached)
        m_pDetached = new DetachedProcess();
    m_pDetached->fd_pid_file = fd_pid_file;
    m_pDetached->last_check_time = DateTime::s_curTime;
    m_pDetached->pid_info.pid = -1;
    saveDetachedPid(fd_pid_file, &m_pDetached->pid_info,
                   service_addr.getUnix());


    pid = workerExec(config, fd);
    if (pid != 0)
    {
        ret = processPid(pid, service_addr.getUnix());
        return ret;
    }
    else
    {
        LS_DBG_L("[%s] async exec in background.", getName());
        m_pDetached->pid_info.pid = 0;
    }
    return 0;
}







bool RestartMarker::isSamePath(const char* path)
{
    return strcmp(path, m_markerPath.c_str()) == 0;
}


bool RestartMarker::checkRestart(time_t now)
{
    bool ret = false;
    if ( now == m_lastCheck)
        return false;
    if (m_markerPath.c_str())
    {
        struct stat st;
        if (stat(m_markerPath.c_str(), &st) == 0)
        {
            if (st.st_mtime != m_lastmod)
            {
                m_lastmod = st.st_mtime;
                ret = (m_lastCheck != -1);
            }
        }
        if (!ret && m_lsapi_reset_me_path_pos > 0)
        {
            char reset_me_path[PATH_MAX];
            snprintf(reset_me_path, PATH_MAX, "%.*smod_lsapi_reset_me",
                     m_lsapi_reset_me_path_pos, m_markerPath.c_str());
            if (stat(reset_me_path, &st) == 0)
            {
                if (st.st_mtime != m_lastmod_reset_me)
                {
                    m_lastmod_reset_me = st.st_mtime;
                    ret = (m_lastCheck != -1);
                }
            }

        }
    }
    m_lastCheck = now;
    return ret;
}

