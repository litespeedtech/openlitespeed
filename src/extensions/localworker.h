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
#ifndef LOCALWORKER_H
#define LOCALWORKER_H

#include <lsdef.h>
#include <extensions/extworker.h>
#include <extensions/detached.h>

class PidList;
class LocalWorkerConfig;

class RestartMarker
{
public:
    RestartMarker(const char *pPath)
        : m_markerPath(pPath)
        , m_lastCheck(-1)
        , m_lastmod(-1)
        , m_lastmod_reset_me(-1)
        , m_lsapi_reset_me_path_pos(0)
        {}

    ~RestartMarker()
    {
    }

    bool checkRestart(time_t now);
    bool isSamePath(const char* path);
    void set_lsapi_reset_me_path_pos(int pos)
    {   m_lsapi_reset_me_path_pos = pos;    }
    time_t getLastReset() const
    {
        return (m_lastmod > m_lastmod_reset_me)? m_lastmod : m_lastmod_reset_me;
    }

private:
    AutoStr     m_markerPath;
    time_t      m_lastCheck;
    time_t      m_lastmod;
    time_t      m_lastmod_reset_me;
    int         m_lsapi_reset_me_path_pos;
};

class LocalWorker : public ExtWorker
{
    int                 m_fdApp;
    int                 m_sigGraceStop;
    time_t              m_tmResourceLimited;
    int                 m_forceStop;
    PidList            *m_pidList;
    PidList            *m_pidListStop;
    RestartMarker      *m_pRestartMarker;
    DetachedProcess_t  *m_pDetached;

    
    void        moveToStopList();
public:
    static time_t       s_tmRestartPhp;
    
    explicit LocalWorker(int type);

    
    ~LocalWorker();

    LocalWorkerConfig &getConfig() const;

    PidList *getPidList() const    {   return m_pidList;   }
    void    addPid(pid_t pid);
    void    removePid(pid_t pid);
    int     startOnDemond(int force);

    void    cleanStopPids();
    void detectDiedPid();

    void setfd(int fd)            {   m_fdApp = fd;       }
    int getfd() const               {   return m_fdApp;     }

    int selfManaged() const;

    int runOnStartUp();

    void removeUnixSocket();
    int getCurInstances() const;
    int stop();
    void moveToStopList(int pid);
    int tryRestart();
    int restart();
    int addNewProcess();
    
    virtual void onTimer();

    int startWorker();
    void setRestartMarker(const char* path, int reset_me_path_pos);
    
    static int workerExec(LocalWorkerConfig &config, int fd);
    static void configRlimit(RLimits *pRLimits, const XmlNode *pNode);

    bool serverLevelRestartPhp();
    void checkAndStopWorker();

    int openLockPidFile(const char *pSocketPath);
    bool detectBinaryChange();
    int processPid(int pid, const char * path);

    
    bool loadDetachedPid(int fd, DetachedPidInfo_t *detached_pid);
    void saveDetachedPid(int fd, DetachedPidInfo_t *detached_pid,
                                    const char *path);
    void removeOldDetachedSocket();
    void killOldDetachedInstance(DetachedPidInfo_t *detached_pid);

    int stopDetachedWorker();
    int isDetachedAlreadyRunning();


    // return 0    start in progress
    // return > 0  started/already running
    // return < 0  something wrong, cannot start
    //             can be `-HTTP_Status_Code`

    int startDetachedWorker(int force);
    
    
    
    LS_NO_COPY_ASSIGN(LocalWorker);
};



#endif
