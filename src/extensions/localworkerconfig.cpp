/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#include "localworkerconfig.h"
#include "localworker.h"
#include "registry/extappregistry.h"

#include <http/httpserverconfig.h>
#include <http/serverprocessconfig.h>
#include <http/stderrlogger.h>
#include <log4cxx/logger.h>
#include <main/configctx.h>
#include <util/rlimits.h>
#include <util/xmlnode.h>
#include <util/daemonize.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>

LocalWorkerConfig::LocalWorkerConfig(const char *pName)
    : ExtWorkerConfig(pName)
    , m_pCommand(NULL)
    , m_iBackLog(100)
    , m_iInstances(1)
    , m_iPriority(0)
    , m_iRunOnStartUp(0)
    , m_umask(ServerProcessConfig::getInstance().getUMask())
    , m_iPhpHandler(0)
{
}


LocalWorkerConfig::LocalWorkerConfig()
    : m_pCommand(NULL)
    , m_iBackLog(100)
    , m_iInstances(1)
    , m_iPriority(0)
    , m_iRunOnStartUp(0)
    , m_umask(ServerProcessConfig::getInstance().getUMask())
    , m_iPhpHandler(0)
{
}


LocalWorkerConfig::LocalWorkerConfig(const LocalWorkerConfig &rhs)
    : ExtWorkerConfig(rhs)
{
    if (rhs.m_pCommand)
        m_pCommand = strdup(rhs.m_pCommand);
    else
        m_pCommand = NULL;
    m_iBackLog = rhs.m_iBackLog;
    m_iInstances = rhs.m_iInstances;
    m_iPriority = rhs.m_iPriority;
    m_rlimits = rhs.m_rlimits;
    m_iRunOnStartUp = rhs.m_iRunOnStartUp;
    m_umask = ServerProcessConfig::getInstance().getUMask();
    m_iPhpHandler = 0;
}


LocalWorkerConfig::~LocalWorkerConfig()
{
    if (m_pCommand)
        free(m_pCommand);
}


void LocalWorkerConfig::setAppPath(const char *pPath)
{
    if ((pPath != NULL) && (strlen(pPath) > 0))
    {
        if (m_pCommand)
            free(m_pCommand);
        m_pCommand = strdup(pPath);
    }
}


void LocalWorkerConfig::beginConfig()
{
    clearEnv();
}


void LocalWorkerConfig::endConfig()
{

}


void LocalWorkerConfig::setRLimits(const RLimits *pRLimits)
{
    if (!pRLimits)
        return;
    m_rlimits = *pRLimits;

}


#define DETACH_MODE_MIN_MAX_IDLE 30
#define DETACH_MODE_DEFAULT_MAX_IDLE 60
static void setDetachedAppEnv(Env *pEnv, int max_idle)
{
    char achBuf[200];
    snprintf(achBuf, 200, "LSAPI_PPID_NO_CHECK=1");
    pEnv->add(achBuf);
    snprintf(achBuf, 200, "LSAPI_PGRP_MAX_IDLE=%d", max_idle);
    pEnv->add(achBuf);
    snprintf(achBuf, 200,  "LSAPI_KEEP_LISTEN=2");
    pEnv->add(achBuf);
}


void LocalWorkerConfig::applyStderrLog()
{
    char buf[MAX_PATH_LEN];
    if (getEnv()->find("LS_STDERR_LOG"))
        return;
    const char *pLogFile = StdErrLogger::getInstance().getLogFileName();
    if (!pLogFile)
        pLogFile = "/dev/null";
    snprintf(buf, MAX_PATH_LEN, "LS_STDERR_LOG=%s", pLogFile);
    addEnv(buf);
}


int LocalWorkerConfig::checkExtAppSelfManagedAndFixEnv(int maxIdleTime)
{
    static const char *instanceEnv[] =
    {
        "PHP_FCGI_CHILDREN",  "PHP_LSAPI_CHILDREN",
        "LSAPI_CHILDREN"
    };
    int selfManaged = 0;
    size_t i;
    Env *pEnv = getEnv();
    const char *pEnvValue = NULL;

    for (i = 0; i < sizeof(instanceEnv) / sizeof(char *); ++i)
    {
        pEnvValue = pEnv->find(instanceEnv[i]);

        if (pEnvValue != NULL)
            break;
    }


    if (pEnvValue)
    {
        int children = atol(pEnvValue);


        if ((children > 0) && (children * getInstances() < getMaxConns()))
        {
            LS_WARN(ConfigCtx::getCurConfigCtx(),
                    "Improper configuration: the value of "
                    "%s should not be less than 'Max "
                    "connections', 'Max connections' is reduced to %d."
                    , instanceEnv[i], children * getInstances());
            setMaxConns(children * getInstances());
        }

        selfManaged = 1;
    }


    if (isDetached())
    {
        setDetachedAppEnv(pEnv,
                          (maxIdleTime > DETACH_MODE_MIN_MAX_IDLE)
                          ? maxIdleTime : DETACH_MODE_MIN_MAX_IDLE);
        applyStderrLog();
    }

    pEnv->add(0, 0, 0, 0);
    return selfManaged;
}


int LocalWorkerConfig::config(const XmlNode *pNode)
{
    ServerProcessConfig &procConfig = ServerProcessConfig::getInstance();
    int selfManaged;
    int instances;
    int backlog = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "backlog",
                  1, 100, 100);
    int priority = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                   "priority", -20, 20, procConfig.getPriority() + 1);

    if (priority > 20)
        priority = 20;

    if (priority < procConfig.getPriority())
        priority = procConfig.getPriority();

    long l = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
             "extMaxIdleTime", -1, INT_MAX, INT_MAX);

    if (l == -1)
        l = INT_MAX;

    setPriority(priority);
    setBackLog(backlog);
    setMaxIdleTime(l);


    int umakeVal = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "umask",
                   000, 0777, procConfig.getUMask(), 8);
    setUmask(umakeVal);

    setRunOnStartUp(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                    "runOnStartUp", 0, 3, 3));

    RLimits limits;
    if (ExtAppRegistry::getRLimits() != NULL)
        limits = *(ExtAppRegistry::getRLimits());
    limits.setCPULimit(RLIM_INFINITY, RLIM_INFINITY);
    limits.setDataLimit(RLIM_INFINITY, RLIM_INFINITY);
    LocalWorker::configRlimit(&limits, pNode);
    setRLimits(&limits);
    Env *pEnv = getEnv();

    if (pEnv->find("PATH") == NULL)
        pEnv->add("PATH=/bin:/usr/bin");
    instances = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "instances",
                1, INT_MAX, 1);

    if (instances > 2000)
        instances = 2000;

    if (instances >
        HttpServerConfig::getInstance().getMaxFcgiInstances())
    {
        instances = HttpServerConfig::getInstance().getMaxFcgiInstances();
        LS_WARN(ConfigCtx::getCurConfigCtx(),
                "<instances> is too large, use default max:%d",
                instances);
    }
    setInstances(instances);


    long maxIdle = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                   "extMaxIdleTime", -1, INT_MAX, DETACH_MODE_DEFAULT_MAX_IDLE);
    selfManaged = checkExtAppSelfManagedAndFixEnv(maxIdle);
    setSelfManaged(selfManaged);
    if ((instances != 1) &&
        (getMaxConns() > instances))
    {
        LS_NOTICE(ConfigCtx::getCurConfigCtx(),
                  "Possible mis-configuration: 'Instances'=%d, "
                  "'Max connections'=%d, unless one Fast CGI process is "
                  "capable of handling multiple connections, "
                  "you should set 'Instances' greater or equal to "
                  "'Max connections'.", instances, getMaxConns());
        setMaxConns(instances);
    }

    RLimits *pLimits = getRLimits();
#if defined(RLIMIT_NPROC)
    int mini_nproc = (3 * getMaxConns() + 50)
                     * HttpServerConfig::getInstance().getChildren();
    struct rlimit   *pNProc = pLimits->getProcLimit();

    if (((pNProc->rlim_cur > 0) && ((int) pNProc->rlim_cur < mini_nproc))
        || ((pNProc->rlim_max > 0) && ((int) pNProc->rlim_max < mini_nproc)))
    {
        LS_NOTICE(ConfigCtx::getCurConfigCtx(),
                  "'Process Limit' probably is too low, "
                  "adjust the limit to: %d.", mini_nproc);
        pLimits->setProcLimit(mini_nproc, mini_nproc);
    }

#endif

    return 0;
}


int LocalWorkerConfig::isUserBlocked(const char *pUser)
{
    static const char *forbidden_users[] = { "root", "lsadm", };
    const char **name;

    for (name = forbidden_users; name < forbidden_users
            + sizeof(forbidden_users) / sizeof(forbidden_users[0]); ++name)
        if (0 == strcmp(*name, pUser))
            return 1;

    return 0;
}


int LocalWorkerConfig::isGidBlackListed(gid_t gid)
{
    static const char *forbidden_groups[] = { "root", "lsadm", "sudo",
                                                    "wheel", "shadow", };
    struct group group, *group_result;
    const char **name;
    size_t buflen;
    char *buf;
    int retval = 1;
    char local_buf[0x400];

    // Start with buffer on the stack.  If it's not large enough, we will
    // use dynamic memory.
    buf = local_buf;
    buflen = sizeof(local_buf);

  getgrgid_r_again:
    if (0 == getgrgid_r(gid, &group, buf, buflen, &group_result))
    {
        for (name = forbidden_groups; name < forbidden_groups
                + sizeof(forbidden_groups) / sizeof(forbidden_groups[0]); ++name)
            if (0 == strcmp(*name, group_result->gr_name))
            {
                LS_DBG_L("LocalWorkerConfig::isGidBlackListed: group `%s'"
                    " is forbidden", *name);
                goto end;
            }
        LS_DBG_L("LocalWorkerConfig::isGidBlackListed: group `%s' OK",
            group_result->gr_name);
    }
    else if (errno == ERANGE && buf == local_buf)
    {
        buflen = sysconf(_SC_GETGR_R_SIZE_MAX);
        if ((size_t) -1 == buflen)
        {
            LS_DBG_L("LocalWorkerConfig::isGidBlackListed: unknown size "
                "needed for getgrgid_r");
            goto end;
        }
        buf = (char *) malloc(buflen);
        if (!buf)
        {
            LS_WARN("LocalWorkerConfig::isGidBlackListed: malloc(%zu) failed",
                                                                        buflen);
            goto end;
        }
        LS_DBG_L("LocalWorkerConfig::isGidBlackListed: retry with larger "
            "buffer of %zu bytes", buflen);
        goto getgrgid_r_again;
    }
    else
    {
        LS_DBG_H("LocalWorkerConfig::isGidBlackListed: getgrgid_r failed: %s",
            strerror(errno));
        goto end;
    }

    LS_DBG_L("LocalWorkerConfig::isGidBlackListed: gid %d is not "
                                                        "blacklisted", gid);
    retval = 0;

  end:
    if (buf != local_buf)
        free(buf);
    return retval;
}


void LocalWorkerConfig::configExtAppUserGroup(const XmlNode *pNode,
        int iType, char *sHomeDir, size_t szHomeDir)
{

    const char *pUser = pNode->getChildValue("extUser");
    const char *pGroup = pNode->getChildValue("extGroup");
    uid_t uid = ServerProcessConfig::getInstance().getUid();
    gid_t gid = -1;
    struct passwd *pw = Daemonize::configUserGroup(pUser, pGroup, gid);

    if (pw)
    {
        if ((int) gid == -1)
            gid = pw->pw_gid;
        if (isGidBlackListed(gid))
            gid = ServerProcessConfig::getInstance().getGid();
        if (isUserBlocked(pw->pw_name))
        {
            LS_NOTICE(ConfigCtx::getCurConfigCtx(), "ExtApp suExec: configured "
                "user %s is blocked", pw->pw_name);
        }
        else if ((iType != EA_LOGGER)
            && ((pw->pw_uid < ServerProcessConfig::getInstance().getUidMin())
                || (gid < ServerProcessConfig::getInstance().getGidMin())))
        {
            LS_NOTICE(ConfigCtx::getCurConfigCtx(), "ExtApp suExec access denied,"
                      " UID or GID of VHost document root is smaller "
                      "than minimum UID, GID configured. ");
        }
        else
            uid = pw->pw_uid;

        lstrncpy(sHomeDir, pw->pw_dir, szHomeDir);
    }
    else
    {
        gid = ServerProcessConfig::getInstance().getGid();
        pw = getpwuid(uid);
        if (pw)
            lstrncpy(sHomeDir, pw->pw_dir, szHomeDir);
        else
            lstrncpy(sHomeDir, "/home/nobody", szHomeDir); //If failed, use default as
    }
    setUGid(uid, gid);
    if (!HttpServerConfig::getInstance().getAllowExtAppSetuid())
        setDropCaps(1);
}
