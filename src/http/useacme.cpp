/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2025  LiteSpeed Technologies, Inc.                 *
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
#include "useacme.h"

#include "adns/adns.h"
#include <http/httpserverconfig.h>
#include "http/httpvhost.h"
#include "http/vhostmap.h"
#include "log4cxx/logger.h"
#include "main/configctx.h"
#include "main/httpserver.h"
#include "main/mainserverconfig.h"
#include "socket/gsockaddr.h"
#include "sslpp/sslcontext.h"
#include "util/autobuf.h"
#include "util/xmlnode.h"

#include <dirent.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

LS_SINGLETON(AcmeCertMap);
LS_SINGLETON(UseAcmeMap);
LS_SINGLETON(AcmeVHostMap);

char *UseAcme::s_server = NULL;

AcmeAlias *AcmeAliasMap::addAlias(const char *primary, const char *alias)
{
    class AcmeAliasMap::iterator it;
    LS_DBG("[ACME] addAlias: %s to %s\n", alias, primary);
    if ((it = find(alias)) != end())
        return it.second();
    const char *wildcard = strchr(alias, '*');
    if (wildcard)
    {
        if (wildcard != alias)
        {
            LS_NOTICE("[ACME] Certs: It is unsupported to use a wildcard in any other place than the first character, %s ignored\n", primary);
            return NULL;
        }
        if (alias[1] != '.' || !alias[2])
        {
            LS_NOTICE("[ACME] Certs: A wildcard must be the line high level qualifier, %s ignored\n", primary);
            return NULL;
        }
        if (strcmp(&alias[2], primary))
        {
            LS_NOTICE("[ACME] Certs: It is unsupported to have a wildcard domain be anything but the wildcard of the primary, %s ignored\n", primary);
            return NULL;
        }
    }
    
    AcmeAlias *acmeAlias = new AcmeAlias;
    if (!acmeAlias)
    {
        LS_ERROR("[ACME] Error creating alias entry for %s\n", alias);
        return NULL;
    }
    acmeAlias->setAlias(alias);
    insert(acmeAlias->getAlias(), acmeAlias);
    return acmeAlias;
}


AcmeCertMap::~AcmeCertMap()
{
    finished();
}


void AcmeCertMap::finished()
{
    release_objects();
    if (m_ifaddrs)
    {
        freeifaddrs(m_ifaddrs);
        m_ifaddrs = NULL;
    }
}


struct ifaddrs *AcmeCertMap::getIfAddrs()
{
    if (m_ifaddrs)
        return m_ifaddrs;
    if (getifaddrs(&m_ifaddrs)) 
    {
        LS_ERROR("[ACME] Unable to enumerate addresses: %s\n", strerror(errno));
        return NULL;
    }
    return m_ifaddrs;
}


struct ifaddrs *AcmeCertMap::isMatch(GSockAddr *sockAddr, void *addr)
{
    struct ifaddrs *addrs = NULL;
    if (!(addrs = getIfAddrs()))
        return NULL;
    LS_DBG("[ACME] isMatch Compare family : %d, %s\n", sockAddr->family(), sockAddr->toString());
    while (addrs)
    {
        if (!addrs->ifa_addr)
        {
            addrs = addrs->ifa_next;
            continue;
        }
        if (addrs->ifa_addr->sa_family == sockAddr->family())
        {
            GSockAddr glocaladdr(addrs->ifa_addr);

            void *localaddr = sockAddr->family() == AF_INET6 ? 
                                (void *)&((struct sockaddr_in6 *)addrs->ifa_addr)->sin6_addr :
                                (void *)&((struct sockaddr_in *)addrs->ifa_addr)->sin_addr;
            if (!memcmp(localaddr, addr, 
                        (sockAddr->family() == AF_INET6) ? 
                            sizeof(struct in6_addr) : 
                            sizeof(struct in_addr)))
            {
                LS_DBG("[ACME] isMatch compare worked for local %s\n", glocaladdr.toString());
                break;
            }
        }
        addrs = addrs->ifa_next;
    }
    if (!addrs)
        return NULL;
    if (sockAddr->family() == AF_INET)
    {
        if (((struct sockaddr_in *)sockAddr->get())->sin_addr.s_addr == INADDR_ANY ||
            ((struct sockaddr_in *)sockAddr->get())->sin_addr.s_addr == *(in_addr_t *)addr)
        {
            LS_DBG("[ACME] Final IPv4 match\n");
            return addrs;
        }
    }
    else
    {
        if (!memcmp(&in6addr_any, &((struct sockaddr_in6 *)sockAddr->get())->sin6_addr, sizeof(struct in6_addr)) ||
            !memcmp(&((struct sockaddr_in6 *)sockAddr->get())->sin6_addr, (struct in6_addr *)addr, sizeof(struct in6_addr)))
        {
            LS_DBG("[ACME] Final IPv6 match\n");
            return addrs;
        }
    }
    return NULL;
}


bool AcmeCertMap::validateDomain(const char *domain, const char *pAddr)
{
    GSockAddr sockAddr;
    struct ifaddrs *addrs = NULL;
    if (sockAddr.set(pAddr, 0) == -1)
    {
        LS_ERROR("[ACME] Unexpected error using passed in address: %s\n", pAddr);
        return false;
    }
    if (domain[0] == '*')
        domain = &domain[2]; // skip wildcard, previously validated
    if (sockAddr.family() == AF_INET6)
    {
        in6_addr in6addr;
        if (Adns::getInstance().getHostByNameV6Sync(domain, &in6addr))
        {
            LS_DBG("[ACME] Not found in IPv6\n");
            return false;
        }
        return true;
    } 
    else if (sockAddr.family() == AF_INET)
    {
        in_addr_t inaddr;
        if (Adns::getInstance().getHostByNameSync(domain, &inaddr))
        {
            LS_DBG("[ACME] Not found in IPv4\n");
            return false;
        }  
        return true;
    }
    else
        return false;
    if (!addrs)
        return false;
    return true;
}


AcmeCert *AcmeCertMap::doQueue(const char *vhost, const char *domain, bool vhostOk,
                               const char *pAddr)
{
    LS_DBG("[ACME] doQueue? %s, %s: %s\n", vhost, pAddr, domain);
    iterator it = find(domain);
    if (it != end())
    {
        LS_DBG("[ACME] Domain is already in the queue\n");
        return it.second();
    }

    AcmeCert *acmeCert = new AcmeCert();
    LS_DBG("[ACME] Create new cert entry %p\n", acmeCert);
    if (!acmeCert)
    {
        LS_ERROR("[ACME] Insufficient memory allocating cert entry\n");
        return NULL;
    }
    acmeCert->setDomainAddr(domain, pAddr);
    acmeCert->setVhost(vhost);
    acmeCert->setVhostOk(vhostOk);
    AcmeCertMap::getInstance().insert(acmeCert->getDomain(), acmeCert);
    AcmeVHostMap::getInstance().insertAcmeCert(acmeCert);
    return acmeCert;
}


void AcmeCertMap::deleteCert(AcmeCert *acmeCert)
{
    UseAcmeMap::iterator itUse = UseAcmeMap::getInstance().find(acmeCert->getDomain());
    if (itUse != UseAcmeMap::getInstance().end())
    {
        UseAcme *useAcme = itUse.second();
        LS_DBG("[ACME] Remove UseAcmeMap: %s", useAcme->getDomain());
        UseAcmeMap::getInstance().remove(useAcme->getDomain());
        AcmeVHostMap::getInstance().removeUseAcme(useAcme);
        delete useAcme;
    }
    LS_DBG("[ACME] Remove acmeCert\n");
    remove(acmeCert->getDomain());
    AcmeVHostMap::getInstance().removeAcmeCert(acmeCert);
    delete acmeCert;
}


bool AcmeCertMap::beginCerts(bool programStart)
{
    char acmeDir[MAX_PATH_LEN], configDir[MAX_PATH_LEN];
    if (!UseAcme::acmeInstalled(acmeDir, configDir, sizeof(configDir)))
        return true;
    UseAcmeMap::getInstance().findDeleted();
    if (m_ifaddrs)
    {
        freeifaddrs(m_ifaddrs);
        m_ifaddrs = NULL;
    }
    if (!size())
    {
        LS_DBG("[ACME] beginCerts None queued\n");
        return true;
    }
    LS_DBG("[ACME] There are %ld certs to process!\n", size());
    m_pidRunning = fork();
    if (m_pidRunning == -1)
    {
        LS_ERROR("[ACME] Unable to fork to process certs %s\n", strerror(errno));
        return false;
    }
    if (m_pidRunning)
    {
        LS_DBG("[ACME] Pid %d will process certs\n", m_pidRunning);
        return true;
    }
    processCerts(acmeDir, configDir, programStart);
    return true;
}


int AcmeCertMap::envCount(char *p)
{
    if (!p)
        return 0;
    int count = 0;
    while (*p)
    {
        const char *equals = strchr(p + 1, '=');
        if (!equals)
            break;
        if (*(equals + 1) != '"' || !*(equals + 2))
            break;
        const char *closeQuote = strchr(equals + 2, '"');
        if (!closeQuote)
            break;
        p = (char *)(closeQuote + 1);
        count++;
        if (*p)
        {
            p += strspn(p, " \t\n");
            *((char *)closeQuote + 1) = 0;
        }
    }
    return count;
}


#define STD_SIZE    2048
int AcmeCertMap::reportStd(AutoBuf *autoBuf, const char *fdname, int fd, 
                           char *msg, int *pos, int *closed)
{
    char *line = msg, *nl;
    ssize_t sz;
    //LS_DBG("ReportStd: %s, pid: %d\n", fdname, getpid());
    char title[40];
    int titleLen = snprintf(title, sizeof(title), "%s: ", fdname);
    sz = read(fd, &msg[*pos], STD_SIZE - *pos - 1);
    if (sz > 0)
    {
        //LS_DBG("ReportStd: read %ld bytes\n", sz);
        msg[*pos + sz] = 0;
        while ((nl = strchr(line, '\n')))
        {
            autoBuf->append(title, titleLen);
            autoBuf->append(line, (nl + 1) - line);
            line = nl + 1;
        }
        *pos = strlen(line);
        if (*pos == STD_SIZE - 1)
        {
            // Line is longer than our buffer
            autoBuf->append(title, titleLen);
            autoBuf->append(line);
            autoBuf->append("\n", 1);
            *pos = 0;
            line = msg;
        }
        else
        {
            memmove(msg, line, *pos);
            msg[*pos] = 0;
            line = msg;
        }
    }
    else if (sz == 0)
    {
        //LS_DBG("ReportStd %s, closed!\n", fdname);
        *closed = 1;
    }
    else if (sz < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            //LS_DBG("ReportStd, %s: %s\n", strerror(errno), fdname);
            return 0;
        }
        int msgLen = snprintf(msg, STD_SIZE - 1, "ERROR READING FROM %s: %s\n", fdname, strerror(errno));
        autoBuf->append(msg, msgLen);
    }
    //LS_DBG("ReportStd: %s rc: %d\n", fdname, (sz < 0 ? -1 : 0));
    return (sz < 0 ? -1 : 0);
}


static int fd_set_blocking(int fd, int blocking) {
    /* Save the current flags */
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blocking)
        /* Clear the O_NONBLOCK flag using bitwise AND with NOT O_NONBLOCK */
        flags &= ~O_NONBLOCK;
    else
        /* Set the O_NONBLOCK flag using bitwise OR */
        flags |= O_NONBLOCK;

    return fcntl(fd, F_SETFL, flags) != -1;
}


void AcmeCertMap::reportStds(AutoBuf *autoBuf, int outfd, int errfd)
{
    struct pollfd pfd[2];
    char outmsg[STD_SIZE], errmsg[STD_SIZE];
    int outpos = 0, errpos = 0, numfds = 2, outclosed = 0, errclosed = 0;

    fd_set_blocking(outfd, 0);
    fd_set_blocking(errfd, 0);
    memset(pfd, 0, sizeof(pfd));
    LS_DBG("ReportStds %d %d\n", outfd, errfd);
    pfd[0].fd = outfd;
    pfd[0].events = POLLIN | POLLHUP;
    pfd[0].revents = 0;
    pfd[1].fd = errfd;
    pfd[1].events = POLLIN | POLLHUP;
    pfd[1].revents = 0;

    int pollrc;
    while (numfds && (pollrc = poll(pfd, numfds, -1)) > 0)
    {
        for (int i = 0; i < numfds; i++)
        {
            if (pfd[i].revents)
            {
                if (pfd[i].fd == outfd)
                {
                    int outerr = 0;
                    if (reportStd(autoBuf, "STDOUT", outfd, outmsg, &outpos, &outclosed))
                        outerr = 1;
                    if (outclosed || outerr)
                    {
                        pfd[0].fd = errfd;
                        numfds--;
                    }
                }
                else 
                {
                    int errerr = 0;
                    if (reportStd(autoBuf, "STDERR", errfd, errmsg, &errpos, &errclosed))
                        errerr = 1;
                    if (errclosed || errerr)
                    {
                        numfds--;
                    }
                }
            }
        }
    }
    LS_DBG("ReportStds done\n");
}


void AcmeCertMap::reportAutoBuf(const char *desc, AutoBuf *autoBuf, int err)
{
    char *pos = autoBuf->begin();
    while (pos < autoBuf->end())
    {
        char *nl = strchr(pos, '\n');
        if (!nl)
            break;
        int msgsz = (nl + 1) - pos;
        char msg[msgsz + 1];
        memcpy(msg, pos, msgsz);
        msg[msgsz] = 0;
        if (err)
            LS_ERROR("[ACME] %s: %s", desc, msg);
        else 
            LS_DBG("[ACME] %s: %s", desc, msg);
        pos = nl + 1;
    }
}


int AcmeCertMap::acmeRequest(char *acmeFile, char *desc, char * const *argv,
                                const char *env)
{
    int pipesErr[2], pipesOut[2];
    LS_DBG("[ACME] AcmeRequest %s\n", acmeFile);
    for (int i = 0; argv[i]; i++)
        LS_DBG("[ACME]    %s\n", argv[i]);
    if (pipe(pipesErr) == -1 || pipe(pipesOut))
    {
        LS_ERROR("[ACME] %s pipes failed: %s\n", desc, strerror(errno));
        return -1;
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        LS_ERROR("[ACME] %s fork2 failed %s\n", desc, strerror(errno));
        return -1;
    }
    if (pid == 0)
    {
        char acmeDir[MAX_PATH_LEN / 2], configDir[MAX_PATH_LEN / 2], 
             configEnv[MAX_PATH_LEN], acmeEnv[MAX_PATH_LEN], serverEnv[MAX_PATH_LEN];
        UseAcme::acmeInstalled(acmeDir, configDir, sizeof(configDir));
        snprintf(configEnv, MAX_PATH_LEN, "LE_CONFIG_HOME=%s/data", configDir);
        snprintf(acmeEnv, sizeof(acmeEnv), "LE_WORKING_DIR=%s", acmeDir);
        snprintf(serverEnv, sizeof(serverEnv), "ACME_DIRECTORY=%s", UseAcme::getServer());
        char *p = env ? strdup(env) : NULL;
        int count = envCount(p);
        const char *envp[4 + count] = { configEnv, acmeEnv, serverEnv };
        int index = 3;
        for (int i = 0; i < count; i++)
        {
            envp[index++] = p;
            if (i < count - 1)
            {
                int plen = strlen(p);
                p += plen + 1 + strspn(p + plen + 1, " \t\n");
            }
        }
        envp[index] = NULL;
        for (int i = 0; i < index; i++)
            LS_DBG("[ACME]   Env[%d]: %s\n", i, envp[i]);
        struct passwd *pw = getpwnam("lsadm");
        if (pw)
            setuid(pw->pw_uid);
        else
            LS_DBG("[ACME] Unable to find user lsadm: %s\n", strerror(errno));
        LS_DBG("[ACME] Running as user: %u\n", getuid());
        dup2(pipesErr[1], STDERR_FILENO);
        close(pipesErr[0]);
        close(pipesErr[1]);
        dup2(pipesOut[1], STDOUT_FILENO);
        close(pipesOut[0]);
        close(pipesOut[1]);
        execve(acmeFile, argv, (char * const *)envp);
        LS_ERROR("[ACME] %s exec failed: %s\n", desc, strerror(errno));
        exit(1);
    }
    close(pipesErr[1]);
    close(pipesOut[1]);
    int status;
    AutoBuf autoBuf;
    reportStds(&autoBuf, pipesOut[0], pipesErr[0]);
    waitpid(pid, &status, 0);
    close(pipesOut[0]);
    close(pipesErr[0]);
    int err = 0;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        LS_NOTICE("[ACME] %s was successful\n", desc);  
    else
    {
        LS_ERROR("[ACME] %s failed: \n", desc);
        err = 1;
    }
    if (err || log4cxx::Level::isEnabled( log4cxx::Level::DEBUG))
        reportAutoBuf(desc, &autoBuf, err);
    return err ? -1 : 0;
}


void AcmeCertMap::processCerts(char *acmeDir, char *configDir, bool programStart)
{
    char acmeFile[MAX_PATH_LEN];
    snprintf(acmeFile, sizeof(acmeFile), "%s/acme.sh", acmeDir);
    for (AcmeCertMap::iterator it = begin(); it != end(); it = next(it))
    {
        AcmeCert *cert = it.second();
        LS_DBG("[ACME] Process cert %s, %ld aliases\n", cert->getDomain(), cert->getAcmeAliasMap()->size());
        pid_t pid = fork();
        if (pid == -1)
        {
            LS_ERROR("[ACME] Cert fork1 failed %s\n", strerror(errno));
            exit(1);
        }
        if (pid == 0)
        {
            const char *argv[12 + 2 * cert->getAcmeAliasMap()->size()];
            argv[0] = acmeFile;
            argv[1] = cert->getRenew() ? "--renew" : "--issue";
            argv[2] = "-d";
            argv[3] = cert->getDomain();
            int index = 4;
            if (cert->getWildcard())
            {
                argv[index++] = "--dns";
                argv[index++] = cert->getDnsApi();
            }
            else
            {
                argv[index++] = "--stateless";
            }
            for (AcmeAliasMap::iterator it = cert->getAcmeAliasMap()->begin(); 
                 it != cert->getAcmeAliasMap()->end(); 
                 it = cert->getAcmeAliasMap()->next(it))
            {
                argv[index] = "-d";
                argv[index + 1] = it.first();
                index += 2;
            }
            if (cert->getRenew() && cert->getForce())
                argv[index++] = "--force";
            argv[index++] = "--server";
            argv[index++] = UseAcme::getServer();
            if ( log4cxx::Level::isEnabled( log4cxx::Level::DEBUG ) )
                argv[index++] = "--debug";
            argv[index] = NULL;
            char desc[MAX_PATH_LEN];
            snprintf(desc, sizeof(desc), "%s certificate for %s", 
                     cert->getRenew() ? "Renew" : "Create", cert->getDomain());
            if (acmeRequest(acmeFile, desc, (char *const*)argv, 
                            cert->getDnsEnv()) == 0)
                exit(0); // Success reported
            exit(1); // Error reported
        }
    }
    int ok = 0, failed = 0, status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, 0)))
    {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            ok++;
        else
            failed++;
        if (ok + failed >= (int)size())
            break;
    }
    LS_NOTICE("[ACME] %d certs succeeded and %d certs failed\n", ok, failed);
    if (ok && programStart)
    {
        LS_NOTICE("[ACME] Doing graceful restart to apply new certs\n");
        kill(MainServerConfig::getInstance().getWatchDogPid(), SIGUSR1);
    }    
    exit(ok ? 0 : 1);
}


bool AcmeCertMap::isRunning(char *configDir)
{
    if (m_pidRunning > 0)
    {
        LS_DBG("[ACME] AcmeCertMap::isRunning %d\n", m_pidRunning);
        int status = 0;
        pid_t pid = waitpid(m_pidRunning, &status, WNOHANG);
        if (pid == -1)
            LS_DBG("[ACME] Error in waitpid: %s\n", strerror(errno));
        else if (pid == 0)
        {
            LS_DBG("[ACME] Process %d still running?\n", m_pidRunning);
            return true;
        }
        m_pidRunning = -1;
        for (AcmeCertMap::iterator it = begin(); it != end(); it = next(it))
        {
            LS_DBG("[ACME] Finishing up read using domain: %s\n", it.first());
            UseAcmeMap::iterator useAcmeIt = UseAcmeMap::getInstance().find(it.first());
            if (useAcmeIt == UseAcmeMap::getInstance().end())
                LS_DBG("[ACME] Not able to find domain!\n");
            else
            {
                UseAcme *useAcme = useAcmeIt.second();
                useAcme->rereadRenewal(configDir);
            }
        }
        finished();
    }
    return false;
}


const char *UseAcme::getFileDomain(const char *domain)
{
    return domain;
}


char *UseAcme::getDomainDirNameOnly(const char *domain, char dir[], 
                                    size_t dirlen)
{
    snprintf(dir, dirlen, "%s_ecc", getFileDomain(domain));
    return dir;
}


char *UseAcme::getDomainDir(char *configDir, char dir[], size_t dirlen)
{
    snprintf(dir, dirlen, "%s/certs", configDir);
    return dir;
}


char *UseAcme::getDomainDirName(char *configDir, const char *domain, char dir[], 
                                size_t dirlen)
{
    char dironly[dirlen / 2];
    snprintf(dir, dirlen, "%s/certs/%s", configDir, 
             getDomainDirNameOnly(domain, dironly, sizeof(dironly)));
    return dir;
}


UseAcme *UseAcme::addDomain(char *configDir, const char *domain, const char *vhostName, 
                            bool vhostOk, const char *pAddr, AcmeAliasMap **acmeAliasMap, 
                            AcmeCert **ppAcmeCert)
{
    LS_DBG("[ACME] addDomain: %s, vhost: %s\n", domain, vhostName);
    const char *wildcard = strchr(domain, '*');
    *ppAcmeCert = NULL;
    if (wildcard)
    {
        LS_NOTICE("[ACME] Certs: It is unsupported to have the primary domain be a wildcard domain, specify the non-wildcard entry as the primary\n");
        return NULL;
    }
    char domainDir[MAX_PATH_LEN];
    getDomainDirName(configDir, getFileDomain(domain), domainDir, MAX_PATH_LEN);
    char acmeKey[MAX_PATH_LEN * 2], acmeCert[MAX_PATH_LEN * 2], ca[MAX_PATH_LEN];
    snprintf(acmeKey, sizeof(acmeKey), "%s/%s.key", domainDir, getFileDomain(domain));
    snprintf(acmeCert, sizeof(acmeCert), "%s/%s.cer", domainDir, getFileDomain(domain));
    if (access(acmeKey, 0) || access(acmeCert, 0))
    {
        LS_DBG("[ACME] Issue finding %s or %s dir: %d, key: %d cert: %d\n", acmeKey, acmeCert,
               access(domainDir, 0), access(acmeKey, 0), access(acmeCert, 0));
        AcmeCert *acmeCert = AcmeCertMap::getInstance().doQueue(vhostName, domain, vhostOk, pAddr);
        if (acmeCert)
        { 
            *acmeAliasMap = acmeCert->getAcmeAliasMap();
            *ppAcmeCert = acmeCert;
            return NULL;
        }
    }
    LS_DBG("[ACME] Found domain %s in acme dir, addr: %s\n", domain, pAddr);
    UseAcme *useAcme = new UseAcme();
    if (!useAcme)
    {
        LS_ERROR("[ACME] Unable to allocate Acme class");
        return NULL;
    }
    useAcme->setDomain(domain);
    useAcme->m_certPath = domainDir;
    useAcme->m_key = acmeKey;
    useAcme->m_cert = acmeCert;
    useAcme->m_vhost = vhostName;
    useAcme->m_addr = pAddr;
    useAcme->m_vhostOk = vhostOk;
    snprintf(ca, sizeof(ca), "%s/ca.cer", domainDir);
    useAcme->m_CA = ca;
    *acmeAliasMap = useAcme->getAcmeAliasMap();
    return useAcme;
}


bool UseAcme::acmeInstalled(char acmeDir[], char configDir[], int dirlen)
{
    static bool s_testedInstalled = false;
    static bool s_installed = false;
    char lacmeDir[MAX_PATH_LEN];

    if (!acmeDir)
        acmeDir = lacmeDir;
    else
        snprintf(configDir, dirlen, "%s/conf/cert/acme", MainServerConfig::getInstance().getServerRoot());
    snprintf(acmeDir, dirlen, "%s/acme", MainServerConfig::getInstance().getServerRoot());
    if (s_testedInstalled)
        return s_installed;
    s_testedInstalled = true;
    if (access(acmeDir, 0))
        return false;
    char serverFile[MAX_PATH_LEN];
    snprintf(serverFile, sizeof(serverFile), "%s/server.conf", configDir);
    FILE *fp = fopen(serverFile, "r");
    if (!fp)
    {
        LS_DBG("[ACME] Cert server file %s: %s\n", serverFile, strerror(errno));
        return false;
    }
    char line[MAX_PATH_LEN];
    if (fgets(line, sizeof(line) - 1, fp))
    {
        s_server = strdup(line);
        int len = strlen(line);
        if (!s_server || !len)
        {
            LS_DBG("[ACME] Cert server can't be found\n");
            fclose(fp);
            return false;
        }
        if (s_server[len - 1] == '\n')
            s_server[len - 1] = 0;
        LS_DBG("[ACME] Set server to be: %s\n", getServer());
    }
    else 
    {
        LS_DBG("[ACME] Unable to read cert server: %s\n", strerror(errno));
        fclose(fp);
        return false;
    }
    fclose(fp);
    s_installed = true;
    return true;
}


void UseAcme::failMap(UseAcme **useAcme, AcmeCert **acmeCert, 
                      UseAcme **firstUseAcme, UseAcme **useAcmeMap)
{
    if (*acmeCert)
    {
        AcmeCertMap::getInstance().remove((*acmeCert)->getDomain());
        AcmeVHostMap::getInstance().removeAcmeCert(*acmeCert);
        delete *acmeCert;
        *acmeCert = NULL;
    }
    else if (*useAcme)
    {
        UseAcmeMap::getInstance().remove((*useAcme)->getDomain());
        AcmeVHostMap::getInstance().removeUseAcme(*useAcme);
        delete *useAcme;
        if (*useAcme == *useAcmeMap)
            *useAcmeMap = NULL;
        if (*useAcme == *firstUseAcme)
            *firstUseAcme = NULL;
        *useAcme = NULL;
    }
}


char *UseAcme::getDomainFromDomains(const char *vhost, const char **domains, 
                                    char *domain, size_t domain_len)
{
    LS_DBG("[ACME] Look for domains in %s\n", *domains);
    size_t sz = strspn(*domains, " \t,");
    (*domains) += sz;
    if (!**domains)
        return NULL;
    const char *sep = strpbrk(*domains, " \t,");
    if (sep)
    {
        memcpy(domain, *domains, sep - *domains);
        domain[sep - *domains] = 0;
        *domains = sep;
    }
    else 
    {
        int len = strlen(*domains);
        strncpy(domain, *domains, domain_len - 1);
        *domains += len;
    }
    if (domain[0] == '*' && domain[1] == 0)
    {
        LS_DBG("[ACME] For the purposes of certs, a '*' domain is the vhost name\n");
        strncpy(domain, vhost, domain_len);
    }
    LS_DBG("[ACME] Domain: %s\n", domain);
    return domain;
}


bool UseAcme::processDomains(char *configDir, const char *vhost, bool vhostOk,
                             const char *domains, const char *pAddr,
                             UseAcme **firstUseAcme, UseAcme **useAcmeMap)
{
    UseAcme *useAcme = NULL;
    AcmeAliasMap *acmeAliasMap = NULL;
    AcmeCert *acmeCert = NULL;
    LS_DBG("processDomains: vhost: %s\n", vhost);
    while (*domains)
    {
        char domain[MAX_PATH_LEN];
        if (!getDomainFromDomains(vhost, &domains, domain, sizeof(domain)))
            break;
        if (!AcmeCertMap::getInstance().validateDomain(UseAcme::getFileDomain(domain), pAddr))
        {
            if (!acmeAliasMap)
                LS_NOTICE("[ACME] Could not validate domain: %s a cert will not be generated.\n", domain);
            else
                LS_NOTICE("[ACME] Could not validate domain: %s. None of the certs %s will be generated\n", domain, vhost);
            failMap(&useAcme, &acmeCert, firstUseAcme, useAcmeMap);
            break;
        }
        if (!acmeAliasMap)
        {
            useAcme = addDomain(configDir, domain, vhost, vhostOk, pAddr, 
                                &acmeAliasMap, &acmeCert);
            if (useAcme)
            {
                LS_DBG("processDomains:addDomain, useAcme found: %s\n", useAcme->getVhost());
                if (!*firstUseAcme)
                    *firstUseAcme = useAcme;
                if (!*useAcmeMap) 
                    *useAcmeMap = useAcme;
            }
            else if (!acmeCert)
            {
                LS_DBG("processDomains:addDomain, no acmeCert");
                break;
            }
            else
                LS_DBG("processDomains:addDomain, HAVE acmeCert vhost: %s, acmeCert: %s, domain: %s\n", vhost, acmeCert->getVhost(), acmeCert->getDomain());
        }
        else 
        {   
            if ((useAcme && useAcme->getWildcard()) ||
                (acmeCert && acmeCert->getWildcard()))
            {
                LS_NOTICE("[ACME] It is invalid to use a wildcard domain and some other domain other than the primary: %s\n", 
                          useAcme ? useAcme->getDomain() : acmeCert->getDomain());
                failMap(&useAcme, &acmeCert, firstUseAcme, useAcmeMap);
                break;
            }
            if (!acmeAliasMap->addAlias(useAcme ? useAcme->getDomain() : acmeCert->getDomain(),
                                        domain))
            {
                failMap(&useAcme, &acmeCert, firstUseAcme, useAcmeMap);
                break;
            } 
            if (*domain == '*')
            {
                if (useAcme)
                    useAcme->setWildcard(true);
                else if (acmeCert)
                    acmeCert->setWildcard(true);
            }
        }
    }
    if (useAcme)
    {
        UseAcme *lastUseAcme = useAcme;
        useAcme = validateAcme(configDir, useAcme);
        if (!useAcme)
        {
            if (*firstUseAcme == lastUseAcme)
                *firstUseAcme = NULL;
            if (*useAcmeMap == lastUseAcme)
                *useAcmeMap = NULL;
        }
    }
    return true;
}


bool UseAcme::processMap(char *configDir, const char *map, const char *pAddr, 
                         UseAcme **firstUseAcme, UseAcme **useAcmeMap)
{
    LS_DBG("  -> %s\n", map);
    char vhostName[MAX_PATH_LEN];
    const char *vhost = strchr(map, ' ');
    if (vhost)
    {
        memcpy(vhostName, map, vhost - map);
        vhostName[vhost - map] = 0;
        const char *domains = vhost;
        processDomains(configDir, vhostName, false, domains, pAddr, 
                       firstUseAcme, useAcmeMap);
    }
    return true;
}


UseAcme *UseAcme::acmeFound(const XmlNode *pNode, const char *pAddr)
{
    char acmeDir[MAX_PATH_LEN], configDir[MAX_PATH_LEN];
    if (!acmeInstalled(acmeDir, configDir, sizeof(configDir)))
        return NULL;
    XmlNodeList list;
    pNode->getAllChildren(list);
    LS_DBG("[ACME] Check all maps for %s\n", pNode->getValue());
    UseAcme *firstUseAcme = NULL;

    for (XmlNodeList::iterator it = list.begin(); it != list.end(); it++)
    {
        LS_DBG("[ACME] Is %p map: %s\n", *it, (*it)->getName());
        UseAcme *useAcmeMap = NULL;
        if ((*it)->getName() && strcasecmp((*it)->getName(), "map") == 0)
        {
            const char *map = (*it)->getValue();
            if (!processMap(configDir, map, pAddr, &firstUseAcme, &useAcmeMap))
                return NULL;
        }
        if (useAcmeMap)
        {
            LS_DBG("[ACME] acmeFound found domain %s for vhost: %s, addr: %s\n", 
                   useAcmeMap->getDomain(), useAcmeMap->getVhost(), pAddr);
            UseAcmeMap::getInstance().insert(useAcmeMap->getDomain(), useAcmeMap);
            AcmeVHostMap::getInstance().insertUseAcme(useAcmeMap);
        }
    }
    return firstUseAcme;
}


UseAcme *UseAcme::vhostActivate(HttpVHost *vhost)
{
    bool activate = HttpServerConfig::getInstance().getAcme() != HttpServerConfig::ACME_DISABLED &&
                    (HttpServerConfig::getInstance().getAcme() == HttpServerConfig::ACME_ON ||
                     (vhost && vhost->getAcme()));
    if (activate && vhost && vhost->getFileSsl())
    {
        LS_DBG("[ACME] vhost using file Ssl!\n");
        activate = false;
    }
    LS_DBG("[ACME] UseAcme::vhostActivate: %s %s activate\n", vhost->getName(), 
           activate ? "DO" : "DO NOT");
    UseAcme *useAcme = NULL;
    AcmeVHostMap::iterator it = AcmeVHostMap::getInstance().find(vhost->getName());
    if (it == AcmeVHostMap::getInstance().end())
    {
        if (activate)
            LS_ERROR("[ACME] VHostActivate: VHost: %s not found!\n", vhost->getName());
        else
            LS_DBG("[ACME] Don't activate VHost %s not found\n", vhost->getName());
        return NULL;
    }
    AcmeLists *acmeLists = it.second();
    if (!activate)
    {
        LS_DBG("[ACME] Deep delete, remove %s\n", vhost->getName());
        AcmeVHostMap::getInstance().remove(vhost->getName());
        LS_DBG("[ACME] Deep delete, delete UseAcme\n");
        while (acmeLists->getUseAcmeList()->size())
        {
            UseAcme *useAcme = acmeLists->getUseAcmeList()->pop_back();
            LS_DBG("[ACME] Deleting UseAcme: vhost: %s, domain: %s\n", useAcme->getVhost(), useAcme->getDomain());
            UseAcmeMap::getInstance().remove(useAcme->getDomain());
            delete useAcme;
        }
        LS_DBG("[ACME] Deep delete, delete AcmeCert\n");
        while (acmeLists->getAcmeCertList()->size())
        {
            AcmeCert *acmeCert = acmeLists->getAcmeCertList()->pop_back();
            LS_DBG("[ACME] Deleting AcmeCert %p: vhost: %s, domain: %s\n", acmeCert, acmeCert->getVhost(), acmeCert->getDomain());
            AcmeCertMap::getInstance().remove(acmeCert->getDomain());
            delete acmeCert;
        }
        LS_DBG("[ACME] Deep delete, remove acmeLists\n");
        delete acmeLists;
        LS_DBG("[ACME] Deep delete, Done\n");
    }
    else
    {
        for (UseAcmeList::iterator itl = acmeLists->getUseAcmeList()->begin();
             itl != acmeLists->getUseAcmeList()->end(); itl++)
        {
            useAcme = *itl;
            if (useAcme->getSslContext())
            {
                LS_DBG("[ACME] Activate SSL cert %p from %s\n", 
                       useAcme->getSslContext(), useAcme->getDomain());
                vhost->setParentSsl(1);
                vhost->setSslContext(useAcme->getSslContext());
                break;
            }
            else
            {
                LS_DBG("[ACME] No SSL cert found, try to create one!\n");
                vhost->setSslContext(ConfigCtx::getCurConfigCtx()->justSSLContext(
                    NULL, useAcme->getAddr(), NULL, useAcme));
            }
        }
        for (AcmeCertList::iterator ita = acmeLists->getAcmeCertList()->begin();
             ita != acmeLists->getAcmeCertList()->end();)
        {
            AcmeCert *acmeCert = *ita;
            if (!acmeCert)
                break;
            if (acmeCert->getWildcard())
            {
                if (!vhost->getAcmeApi() || !vhost->getAcmeEnv())
                {
                    bool needBreak = false;
                    LS_NOTICE("[ACME] Certs: You must specify a VHost api to use a wildcard cert, %s for %s ignored\n", 
                              vhost->getName(), acmeCert->getDomain());
                    AcmeCertList::iterator itt = ita;
                    ita++;
                    if (ita == acmeLists->getAcmeCertList()->end())
                        needBreak = true;
                    acmeLists->getAcmeCertList()->erase(itt);
                    AcmeCertMap::getInstance().remove(acmeCert->getDomain());
                    delete acmeCert;    
                    if (needBreak)
                        break;
                }
                else
                {
                    LS_DBG("[ACME] Activate vhost %s wildcard cert\n", vhost->getName());
                    acmeCert->setDnsApi(vhost->getAcmeApi());
                    acmeCert->setDnsEnv(vhost->getAcmeEnv());
                    ita++;
                }
            }
            else
                ita++;
        }
    }
    return useAcme;
}


void UseAcme::setSslContext(SslContext *sslContext)
{   
    m_sslContext = sslContext; 
}


UseAcme *UseAcme::acmeVhost(HttpVHost *vhost, const char *domain, 
                            const char *aliases, const char *pAddr)
{
    char acmeDir[MAX_PATH_LEN], configDir[MAX_PATH_LEN];
    if (!acmeInstalled(acmeDir, configDir, sizeof(configDir)))
        return NULL;
    UseAcmeMap::iterator itu = UseAcmeMap::getInstance().find(domain);
    LS_DBG("[ACME] acmeVhost For domain %s found vhost %s\n", domain, vhost->getName());
    if (itu != UseAcmeMap::getInstance().end())
    {
        LS_NOTICE("[ACME] Specified ACME for domain %s in VH %s, use that entry\n",
                  domain, vhost->getName());
        return itu.second();
    }
    AcmeCertMap::iterator itc = AcmeCertMap::getInstance().find(domain);
    if (itc != AcmeCertMap::getInstance().end()) 
    {
        LS_NOTICE("[ACME] Specified ACME for domain %s in VH %s create from that entry\n",
                  domain, vhost->getName());
        AcmeCertMap::getInstance().remove(domain);
        AcmeVHostMap::getInstance().removeAcmeCert(itc.second());
        delete itc.second();
    }

    char domains[MAX_PATH_LEN];
    snprintf(domains, sizeof(domains), "%s%s%s", domain, aliases ? "," : "",
             aliases ? aliases : "");
    UseAcme *firstUseAcme = NULL, *useAcmeMap = NULL;
    processDomains(configDir, vhost->getName(), true, domains, pAddr,
                   &firstUseAcme, &useAcmeMap);

    if (useAcmeMap)
    {
        LS_DBG("[ACME] acmeFound found domain %s for vhost: %s, addr: %s (vhost)\n", 
               useAcmeMap->getDomain(), useAcmeMap->getVhost(), pAddr);
        UseAcmeMap::getInstance().insert(useAcmeMap->getDomain(), useAcmeMap);
        AcmeVHostMap::getInstance().insertUseAcme(useAcmeMap);
        if (!useAcmeMap->getSslContext() && HttpServerConfig::getInstance().getAcme() == HttpServerConfig::ACME_OFF)
        {
            LS_DBG("[ACME] No existing SSL context, create it\n");
            ConfigCtx::getCurConfigCtx()->justSSLContext(NULL, pAddr, NULL, useAcmeMap);
        }
    }
    return firstUseAcme;
}


FILE *UseAcme::openConfFile(char *configDir, const char *domain, char *confFile)
{
    char domainDir[MAX_PATH_LEN / 2];
    LS_DBG("[ACME] openConfFile for %s\n", domain);
    snprintf(confFile, MAX_PATH_LEN, "%s/%s.conf", 
             getDomainDirName(configDir, domain, domainDir, sizeof(domainDir)),
             getFileDomain(domain));
    FILE *fp = fopen(confFile, "r");
    if (!fp)
        LS_ERROR("[ACME] Error opening domain configuration file: %s, %s\n", confFile, strerror(errno));
    return fp;
}


time_t UseAcme::findRenewal(FILE *fp)
{
    time_t renewal;
    char line[MAX_PATH_LEN];
    while (fgets(line, sizeof(line) - 1, fp))
    {
        if (!strncmp(line, "Le_NextRenewTime='", 18))
        {
            char *renewStr = &line[18];
            char *quote = strchr(renewStr, '\'');
            if (quote)
                *quote = 0;
            renewal = atol(renewStr);
            LS_DBG("Found Le_NextRenewTime: %lu\n", renewal);
            return renewal;
        }
    }
    LS_DBG("[ACME] Did not find Le_NextRenewTime\n");
    return 0;
}


UseAcme *UseAcme::validateAcme(char *configDir, UseAcme *useAcme)
{
    LS_DBG("[ACME] ValidateAcme for %s\n", useAcme->getDomain());
    char line[MAX_PATH_LEN];
    FILE *fp;
    int invalid_cert = 0, renew_cert = 0;
    AcmeAliasMap acmeAliasMap;
    char confFile[MAX_PATH_LEN];
    if (!(fp = openConfFile(configDir, useAcme->getDomain(), confFile)))
        invalid_cert = 1;
    else while (fgets(line, sizeof(line) - 1, fp))
    {
        LS_DBG("[ACME] Looking for Le_Alt in %s", line);
        if (!strncmp(line, "Le_Alt='", 8))
        {
            char alts[MAX_PATH_LEN];
            strncpy(alts, &line[8], sizeof(alts));
            char *quote = strchr(alts, '\'');
            if (quote)
                *quote = 0;
            if (!(strcmp(alts, "no")))
            {
                if (!useAcme->m_acmeAliasMap.size())
                {
                    LS_DBG("[ACME] Aliases matched (0)!\n");
                    useAcme->m_renewal = findRenewal(fp);
                    fclose(fp);
                    return useAcme;
                }
                LS_NOTICE("[ACME] Change of automatic cert aliases for %s (lost, were %ld, first: %s).  Regenerate\n", 
                          useAcme->getDomain(), useAcme->m_acmeAliasMap.size(), (char *)useAcme->m_acmeAliasMap.begin()->first());
                renew_cert = 1;
                break;
            }
            char *domain = alts;
            size_t num_domains = 0;
            while (*domain && *domain != '\n')
            {
                char *comma = strchr(domain, ',');
                if (comma)
                    *comma = 0;
                acmeAliasMap.insert(domain, NULL);
                if (useAcme->m_acmeAliasMap.find(domain) == useAcme->m_acmeAliasMap.end())
                {
                    if (!renew_cert)
                    {
                        LS_NOTICE("[ACME] Change of automatic cert aliases for %s.  Regenerate\n", useAcme->getDomain());
                        renew_cert = 1;
                    }
                }
                num_domains++;
                if (comma)
                    domain = comma + 1;
                else
                    domain += strlen(domain);
            }
            if (!renew_cert)
            {
                if (num_domains == useAcme->m_acmeAliasMap.size())
                {
                    LS_DBG("[ACME] liases matched (%ld)!\n", num_domains);
                    useAcme->m_renewal = findRenewal(fp);
                    fclose(fp);
                    return useAcme;
                }
                LS_NOTICE("[ACME] Change of count of automatic cert ailiases for %s.  Regnerate\n", useAcme->getDomain());
                renew_cert = 1;
            }
            break;
        }
    }
    if (fp)
        fclose(fp);
    if (!invalid_cert && !renew_cert)
    {
        LS_NOTICE("[ACME] Missing Le_Alt entry in %s configuration file, invalid\n", useAcme->getDomain());
        invalid_cert = 1;
    }
    if (invalid_cert)
    {
        UseAcmeMap::getInstance().remove(useAcme->getDomain());
        AcmeVHostMap::getInstance().removeUseAcme(useAcme);
        delete useAcme;
        return NULL;
    }
    return setupRenew(useAcme, confFile, true);
}


UseAcme *UseAcme::setupRenew(UseAcme *useAcme, char *confFile, bool force)
{
    LS_DBG("[ACME] Renew: use the cert, but also generate a AcmeCertMap entry.\n");
    AcmeCert *acmeCert = AcmeCertMap::getInstance().doQueue(useAcme->getVhost(), 
                                                            useAcme->getDomain(), 
                                                            useAcme->getVhostOk(),
                                                            useAcme->getAddr());
    if (!acmeCert)
        return useAcme;
    acmeCert->setRenew(true);
    if (force)
        acmeCert->setForce(true);
    char newDomain[MAX_PATH_LEN] = { 0 }, *pos = newDomain;
    for (AcmeAliasMap::iterator it = useAcme->getAcmeAliasMap()->begin(); 
         it != useAcme->getAcmeAliasMap()->end(); 
         it = useAcme->getAcmeAliasMap()->next(it))
    {
        if (strchr(it.first(), '*'))
            acmeCert->setWildcard(true);
        if (pos != newDomain)
            *(pos++) = ',';
        int len = strlen(it.first());
        if ((pos + 2 + len - newDomain) > (ssize_t)sizeof(newDomain))
        {
            LS_ERROR("[ACME] Exceeded string length in alias entries for %s\n", useAcme->getDomain());
            return useAcme;
        }
        memcpy(pos, it.first(), len);
        pos += len;
        AcmeAlias *acmeAlias = acmeCert->getAcmeAliasMap()->addAlias(useAcme->getDomain(), it.first());
        if (!acmeAlias)
        {
            delete acmeCert;
            return useAcme;
        }
    }
    *pos = 0;
    if (confFile)
    {
        LS_DBG("[ACME] New Le_Alt string: %s\n", newDomain);
        char line[MAX_PATH_LEN], confFile2[MAX_PATH_LEN];
        snprintf(confFile2, sizeof(confFile2), "%s.new", confFile);
        FILE *fpread = fopen(confFile, "r"), *fpwrite = NULL;
        int copying = 0;
        if (!fpread)
            LS_ERROR("[ACME] Error reopening domain configuration file: %s, %s\n", confFile, strerror(errno));
        else if (!(fpwrite = fopen(confFile2, "w")))
            LS_ERROR("[ACME] Error opening alternate configuration file: %s: %s\n", confFile2, strerror(errno));
        else while (fgets(line, sizeof(line) - 1, fpread))
        {
            if (!strncmp(line, "Le_Alt='", 8))
                fprintf(fpwrite, "Le_Alt='%s'\n", newDomain);
            else
                fputs(line, fpwrite);
            copying = 1;
        }
        if (fpwrite)
            fclose(fpwrite);
        if (fpread)
            fclose(fpread);
        if (copying)
        {
            remove(confFile);
            rename(confFile2, confFile);
        }
    }
    return useAcme;
}


bool UseAcme::rereadRenewal(char *configDir)
{
    char confFile[MAX_PATH_LEN];
    FILE *fp;
    if ((fp = openConfFile(configDir, getDomain(), confFile)))
    {
        time_t renew;
        renew = findRenewal(fp);
        if (renew)
            m_renewal = renew;
        fclose(fp);
        return m_renewal != 0;
    }
    return false;
}


void UseAcmeMap::findDeleted()
{
    char acmeDir[MAX_PATH_LEN], domainDir[MAX_PATH_LEN], configDir[MAX_PATH_LEN];
    LS_DBG("[ACME] UseAcmeMap::findDeleted()\n");
    if (!UseAcme::acmeInstalled(acmeDir, configDir, sizeof(configDir)))
    {
        LS_DBG("[ACME] NOT INSTALLED!\n");
        return;
    }
    UseAcme::getDomainDir(configDir, domainDir, sizeof(domainDir));
    DIR *dir;
    struct dirent *ent;
    if (!(dir = opendir(domainDir)))
    {
        LS_DBG("[ACME] Unable to open certs dir: %s\n", strerror(errno));
        return;
    }
    while ((ent = readdir(dir))) 
    {
        if (!strstr(ent->d_name, "_ecc"))
            continue;
        char domain[256];
        memcpy(domain, ent->d_name, strlen(ent->d_name) - 4);
        domain[strlen(ent->d_name) - 4] = 0;
        LS_DBG("[ACME] Domain: %s\n", domain);
        if (find(domain) != end())
        {
            LS_DBG("[ACME]   IN USE!\n");
            continue;
        }
        if (AcmeCertMap::getInstance().find(domain) != AcmeCertMap::getInstance().end())
        {
            LS_DBG("[ACME]    IN QUEUE TO BE CREATED\n");
            continue;
        }
        LS_DBG("[ACME]    NOT IN USE: DELETE!\n");
        deleteFromAcme(acmeDir, configDir, domain);
        deleteDir(domainDir, ent->d_name);
        remove(domain);
    }
    closedir(dir);
}


bool UseAcmeMap::deleteFromAcme(char *acmeDir, char *configDir, const char *domain)
{
    char acmeFile[MAX_PATH_LEN];
    snprintf(acmeFile, sizeof(acmeFile), "%s/acme.sh", acmeDir);
    const char *argv1[] = { acmeFile, "--revoke", "-d", domain, "--server", 
                            UseAcme::getServer(), NULL};
    char desc[MAX_PATH_LEN];
    snprintf(desc, sizeof(desc), "Revoke certificate for %s", domain);
    int pid = fork();
    if (pid == -1)
    {
        LS_ERROR("[ACME] Unable to fork to delete cert: %s, %s\n", domain, strerror(errno));
        return false;
    }
    if (pid)
    {
        LS_DBG("[ACME] Pid %d will delete cert %s\n", pid, domain);
        return true;
    }
    AcmeCertMap::getInstance().acmeRequest(acmeFile, desc, 
                                           (char *const*)argv1, NULL);

    const char *argv2[] = { acmeFile, "--remove", "-d", domain, "--server", 
                            UseAcme::getServer(), NULL};
    snprintf(desc, sizeof(desc), "Remove certificate for %s", domain);
    AcmeCertMap::getInstance().acmeRequest(acmeFile, desc, 
                                           (char *const*)argv2, NULL);
    exit(0);
}


bool UseAcmeMap::deleteDir(char *domainDir, char *certDir)
{
    char fullDir[MAX_PATH_LEN];
    snprintf(fullDir, sizeof(fullDir), "%s/%s", domainDir, certDir);
    DIR *dir;
    struct dirent *ent;
    if (!(dir = opendir(fullDir)))
    {
        LS_ERROR("[ACME] Unable to open certs dir %s: %s\n", fullDir, strerror(errno));
        return false;
    }
    while ((ent = readdir(dir))) 
    {
        if (ent->d_name[0] == '.')
            continue;
        char fileName[MAX_PATH_LEN];
        snprintf(fileName, sizeof(fileName), "%s/%s", fullDir, ent->d_name);
        if (unlink(fileName)) 
        {
            LS_ERROR("[ACME] Unable to delete cert file %s: %s\n", fileName, strerror(errno));
            closedir(dir);
            return false;
        }
    }
    closedir(dir);
    if (rmdir(fullDir))
    {
        LS_ERROR("[ACME] Unable to delete cert dir: %s: %s\n", fullDir, strerror(errno));
        return false;
    }
    LS_DBG("[ACME] Deleted dir: %s\n", fullDir);
    return true;
}


void UseAcmeMap::renewCerts()
{
    char acmeDir[MAX_PATH_LEN], configDir[MAX_PATH_LEN];
    if (!UseAcme::acmeInstalled(acmeDir, configDir, sizeof(configDir)))
        return;
    time_t now;
    time(&now);
    if (now - m_lastRenew < 86400)
        return;
    m_lastRenew = now;
    LS_DBG("[ACME] UseAcmeMap::renewCerts(), I am %d\n", geteuid());
    if (AcmeCertMap::getInstance().isRunning(configDir))
    {
        LS_DBG("[ACME] Can't do cert renewal, running\n");
        return;
    }
    bool renewalQueued = false;
    for (UseAcmeMap::iterator it = begin(); it != end(); it = next(it))
    {
        UseAcme *useAcme = it.second();
        LS_DBG("[ACME] UseAcmeMap check renewal for domain: %s\n", useAcme->getDomain());
        if (now >= useAcme->getRenewal())
        {
            LS_DBG("[ACME] Do renewal!\n");
            if (UseAcme::setupRenew(useAcme, NULL, false))
                renewalQueued = true;
        }
    }
    if (renewalQueued)
        AcmeCertMap::getInstance().beginCerts(false);
}


AcmeVHostMap::~AcmeVHostMap()
{ 
    LS_DBG("[ACME] ~AcmeVHostMap() release_objects()");
    release_objects();  
}

  
int AcmeVHostMap::insertAcmeCert(AcmeCert *acmeCert)
{
    AcmeLists *acmeLists;
    iterator it = find(acmeCert->getVhost());
    if (it == end())
    {
        LS_DBG("[ACME] insertAcmeCert, inserting %s\n", acmeCert->getVhost());
        acmeLists = new AcmeLists();
        if (!acmeLists)
        {
            LS_ERROR("[ACME] Insufficient memory allocating acmeLists\n");
            return -1;
        }
        if (insert(acmeCert->getVhost(), acmeLists) == end())
            LS_ERROR("[ACME] insertAcmeCert inserting vhost %s failed!\n", acmeCert->getVhost());
    }
    else
    {
        LS_DBG("[ACME] insertAcmeCert, appending %s\n", acmeCert->getVhost());
        acmeLists = it.second();
    }
    acmeLists->getAcmeCertList()->push_back(acmeCert);
    return 0;
}


int AcmeVHostMap::removeAcmeCert(AcmeCert *acmeCert)
{
    iterator it = find(acmeCert->getVhost());
    if (it == end())
    {
        LS_DBG("[ACME] removeAcmeCert: %s not found\n", acmeCert->getVhost());
        return 0;
    }
    LS_DBG("[ACME] removeAcmeCert: %s\n", acmeCert->getVhost());
    AcmeLists *acmeLists = it.second();
    for (AcmeCertList::iterator iter = acmeLists->getAcmeCertList()->begin(); 
         iter != acmeLists->getAcmeCertList()->end(); ++iter)
    {
        if (acmeCert == (*iter))
        {
            remove(acmeCert->getVhost());
            acmeLists->getAcmeCertList()->erase(iter);
            break;
        }
    }
    if (!acmeLists->getAcmeCertList()->size() && !acmeLists->getUseAcmeList()->size())
    {
        LS_DBG("[ACME] removeAcmeCert: delete lists\n");
        delete acmeLists;
    }
    return 0;
}


int AcmeVHostMap::insertUseAcme(UseAcme *useAcme)
{
    AcmeLists *acmeLists;
    iterator it = find(useAcme->getVhost());
    if (it == end())
    {
        LS_DBG("[ACME] insertUseAcme, inserting %s\n", useAcme->getVhost());
        acmeLists = new AcmeLists();
        if (!acmeLists)
        {
            LS_ERROR("[ACME] Insufficient memory allocating acmeLists (useAcme)\n");
            return -1;
        }
        insert(useAcme->getVhost(), acmeLists);
    }
    else
    {
        LS_DBG("[ACME] insertUseAcme, appending %s\n", useAcme->getVhost());
        acmeLists = it.second();
    }
    acmeLists->getUseAcmeList()->push_back(useAcme);
    return 0;
}


int AcmeVHostMap::removeUseAcme(UseAcme *useAcme)
{
    iterator it = find(useAcme->getVhost());
    if (it == end())
    {
        LS_DBG("[ACME] removeUseAcme: %s not found\n", useAcme->getVhost());
        return 0;
    }
    AcmeLists *acmeLists = it.second();
    for (UseAcmeList::iterator iter = acmeLists->getUseAcmeList()->begin(); 
         iter != acmeLists->getUseAcmeList()->end(); ++iter)
    {
        if (useAcme == (*iter))
        {
            acmeLists->getUseAcmeList()->erase(iter);
            break;
        }
    }
    if (!acmeLists->getAcmeCertList()->size() && !acmeLists->getUseAcmeList()->size())
    {
        LS_DBG("[ACME] removeUseAcme: delete lists\n");
        remove(useAcme->getVhost());
        delete acmeLists;
    }
    return 0;
}
