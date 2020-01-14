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

#include <config.h>

#include "httpserver.h"

#include <adns/adns.h>

#include <edio/multiplexer.h>
#include <edio/multiplexerfactory.h>
#include <edio/sigeventdispatcher.h>

#include <extensions/extworker.h>
#include <extensions/localworker.h>
#include <extensions/localworkerconfig.h>
#include <extensions/cgi/cgidworker.h>
#include <extensions/cgi/lscgiddef.h>
#include <extensions/cgi/suexec.h>
#include <extensions/cgi/cgidconfig.h>
#include <extensions/registry/extappregistry.h>
#include <extensions/registry/appconfig.h>

#include <http/accesslog.h>
#include <http/clientcache.h>
#include <http/connlimitctrl.h>
#include <http/contextlist.h>
#include <http/denieddir.h>
#include <http/eventdispatcher.h>
#include <http/handlerfactory.h>
#include <http/handlertype.h>
#include <http/httpaiosendfile.h>
#include <http/httpcgitool.h>
#include <http/httpcontext.h>
#include <http/httpdefs.h>
#include <http/httplistener.h>
#include <http/httplistenerlist.h>
#include <http/httplog.h>
#include <http/httpmime.h>
#include <http/httpresourcemanager.h>
#include <http/httprespheaders.h>
#include <http/httpserverconfig.h>
#include <http/httpserverversion.h>
#include <http/httpsignals.h>
#include <http/httpstats.h>
#include <http/httpstatuscode.h>
#include <http/httpvhost.h>
#include <http/httpvhostlist.h>
#include <http/iptogeo2.h>
#include <http/iptoloc.h>
#include <http/ntwkiolink.h>
#include <http/platforms.h>
#include <http/recaptcha.h>
#include <http/serverprocessconfig.h>
#include <http/staticfilecache.h>
#include <http/staticfilecachedata.h>
#include <http/stderrlogger.h>
#include <http/vhostmap.h>
#include <http/clientinfo.h>

#include <log4cxx/appender.h>
#include <log4cxx/logger.h>
#include <lsr/ls_strtool.h>
#include <lsr/ls_time.h>

#include <main/configctx.h>
#include <main/mainserverconfig.h>
#include <main/plainconf.h>
#include <main/serverinfo.h>
#include <main/zconfclient.h>
#include <main/zconfmanager.h>

#include <shm/lsshm.h>
#include <sslpp/sslcontext.h>
#include <sslpp/sslcontextconfig.h>
#include <sslpp/sslengine.h>
#include <sslpp/sslocspstapling.h>
#include <sslpp/sslsesscache.h>
#include <sslpp/sslticket.h>
#include <sslpp/sslutil.h>

#include <util/accesscontrol.h>
#include <util/autostr.h>
#include <util/daemonize.h>
#include <util/datetime.h>
#include <util/gpath.h>
#include <util/pcutil.h>
#include <util/vmembuf.h>
#include <util/xmlnode.h>
#include <util/stringtool.h>
#include <util/sysinfo/nicdetect.h>
#include <util/sysinfo/systeminfo.h>
#include <util/ni_fio.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/utsname.h>

#include <new>
#include <util/httpfetch.h>

#ifdef RUN_TEST
#include <httpdtest.h>
#include <extensions/fcgi/fcgiapp.h>
#include <extensions/fcgi/fcgiappconfig.h>
#include <extensions/jk/jworker.h>
#include <extensions/jk/jworkerconfig.h>
#include <http/htauth.h>
#include <http/httpresp.h>
#include <http/httpsession.h>
#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#include <sys/prctl.h>
#endif

#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#include <sys/sysctl.h>
#endif

#define FILEMODE            0644

static int s_achPid[256];
static int s_curPid = 0;
//int     SslContext::s_iEnableMultiCerts = 0;

const char *sStatDir = DEFAULT_TMP_DIR;

static void sigchild(int sig)
{
    int status, pid;
    while (true)
    {
        pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
        if (pid <= 0)
        {
            //if ((pid < 1)&&( errno == EINTR ))
            //    continue;
            break;
        }
        if (s_curPid < 256)
            s_achPid[s_curPid++] = pid;
        //PidRegistry::remove( pid );
    }
    HttpSignals::orEvent(HS_CHILD);
}


void HttpServer::cleanPid()
{
    int pid;
    sigset_t newmask, oldmask;
    ExtWorker *pWorker;

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
        return ;

    while (s_curPid > 0)
    {
        pid = s_achPid[--s_curPid];
        LS_DBG_L("Remove pid: %d", pid);
        pWorker = PidRegistry::remove(pid);
        if (pWorker)
            pWorker->removePid(pid);
    }
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

}


LS_SINGLETON(HttpServer);


class HttpServerImpl
{
    friend class HttpServer;

private:

    // members
    HttpVHostMap        m_vhosts;
    HttpVHostMap        m_orgVHosts;
    VHostList           m_toBeReleasedVHosts;
    EventDispatcher     m_dispatcher;
    HttpListenerList    m_listeners;
    HttpListenerList    m_oldListeners;
    HttpListenerList    m_toBeReleasedListeners;
    ContextList         m_toBeReleasedContext;
    HttpContext         m_serverContext;
    AccessControl       m_accessCtrl;
    AutoStr             m_sSwapDirectory;
    AutoStr2            m_sRTReportFile;
    HttpMime            m_httpMime;
    long                m_lStartTime;
    pid_t               m_pid;
    gid_t               m_pri_gid;
    HttpFetch          *m_pAutoUpdFetch[2];

    HttpServerImpl(const HttpServerImpl &rhs);
    void operator=(const HttpServerImpl &rhs);

    // interface functions
    HttpServerImpl(HttpServer *pServer)
        : m_sSwapDirectory(DEFAULT_SWAP_DIR)
        , m_pri_gid(0)
    {
        ClientCache::initObjPool();
        ExtAppRegistry::init();
        HttpMime::setMime(&m_httpMime);
        m_serverContext.allocateInternal();
        HttpRespHeaders::buildCommonHeaders();
        m_sRTReportFile = DEFAULT_TMP_DIR "/.rtreport";
        SslContext::setSniLookupCb(VHostMapFindSslContext);
        memset(m_pAutoUpdFetch, 0, sizeof(m_pAutoUpdFetch));
    }

    ~HttpServerImpl()
    {
        for(int i=0; i<2; ++i)
        {
            if (m_pAutoUpdFetch[i])
                delete m_pAutoUpdFetch[i];
        }
    }

    int initAdns()
    {
        ServerProcessConfig &procConfig = ServerProcessConfig::getInstance();
        if (Adns::getInstance().init() == -1)
            return LS_FAIL;
        if (Adns::getInstance().initShm(procConfig.getUid(),
                                        procConfig.getGid()) == -1)
            return LS_FAIL;
        MultiplexerFactory::getMultiplexer()->add(&Adns::getInstance(),
                POLLIN | POLLHUP | POLLERR);
        return 0;
    }

    int initAioSendFile()
    {
        HttpAioSendFile *pHasf = new HttpAioSendFile();
        if (pHasf->initNotifier(MultiplexerFactory::getMultiplexer()))
            return LS_FAIL;
        else if (pHasf->startProcessor())
            return LS_FAIL;
        HttpAioSendFile::setHttpAioSendFile(pHasf);
        return HttpResourceManager::getInstance().initAiosfcbPool();
    }

    void releaseAll();
    int isServerOk();
    int setupSwap();
    void setSwapDir(const char *pDir);
    const char *getSwapDir() const     {   return m_sSwapDirectory.c_str();    }

    int start();
    int shutdown();
    int gracefulShutdown();
    int generateStatusReport();
    void setRTReportName(int proc);
    int generateRTReport();
    int generateProcessReport(int fd);
    HttpListener *newTcpListener(const char *pName, const char *pAddr);

    HttpListener *addListener(const char *pName, const char *pAddr);
    HttpListener *addListener(const char *pName)
    {   return addListener(pName, pName);     }

    int removeListener(const char *pName);

    HttpListener *getListener(const char *pName)
    {
        return m_listeners.get(pName, NULL);
    }

    int addVHost(HttpVHost *pVHost);
    int removeVHost(const char *pName);
    HttpVHost *getVHost(const char *pName) const
    {
        return m_vhosts.get(pName);
    }

    int enableVHost(const char *pVHostName, int enable);
    int updateVHost(const char *pVHostName, HttpVHost *pVHost);
    void updateMapping();
    void beginConfig();
    void endConfig(int error);
    void offsetChroot();

    int removeVHostFromListener(const char *pListenerName,
                                const char *pVHostName);
    int mapListenerToVHost(const char *pListenerName,
                           const char *pKey,
                           const char *pVHostName);
    int mapListenerToVHost(HttpListener *pListener,
                           const char *pKey,
                           const char *pVHostName);
    int mapListenerToVHost(HttpListener *pListener,
                           HttpVHost    *pVHost,
                           const char *pDomains);


    void checkOLSUpdate();
    void onTimer();
    void onTimer60Secs();
    void onTimer30Secs();
    void onTimer10Secs();
    void onTimerSecond();

    void setBlackBoard(char *pBB);

    int  cleanUp(int pid, char *pBlackBoard);
    int  initSampleServer();
    int  initLscpd();
    int  reinitMultiplexer();
    int  authAdminReq(char *pAuth);

    void recycleContext(HttpContext *pContext)
    {
        pContext->setLastMod(DateTime::s_curTime);
        m_toBeReleasedContext.add(pContext, 1);
    }
    int addVirtualHostMapping(HttpListener *pListener, const char *value,
                              const char *pVHostName);
    int addVirtualHostMapping(HttpListener *pListener, const XmlNode *pNode,
                              const char *pVHostName);
    int configVirtualHostMappings(HttpListener *pListener,
                                  const XmlNode *pNode,
                                  const char *pVHostName);
    int configListenerVHostMap(const XmlNode *pRoot,
                               const char *pVHostName);

    void configCRL(const XmlNode *pNode, SslContext *pSsl);
    SslContext *newSSLContext(const XmlNode *pNode,
                              const char *pName, SslContext *pOldContext);
    int initOcspCachePath();
    int configStapling(const XmlNode *pNode, SslContextConfig *pConf);

    HttpListener *configListener(const XmlNode *pNode, int isAdmin);
    int configListeners(const XmlNode *pRoot, int isAdmin);

    int startAdminListener(const XmlNode *pRoot, const char *pName);
    int startListeners(const XmlNode *pRoot);

    void mapDomainList(const XmlNode *pListenerNodes, HttpVHost *pVHost);
    int enableWebConsole();
    void setAdminThrottleLimits(HttpVHost *pVHostAdmin);
    HttpVHost *createAdminVhost(LocalWorker *pFcgiApp, int iChrootLen,
                                char *pchPHPBin, size_t phpBinLen);
    LocalWorker *createAdminPhpApp(const char *pChroot, int iChrootLen,
                                   const char *pURI, char *pchPHPBin,
                                   size_t szPhpBin);
    const char *configAdminPhpUri(const XmlNode *pNode);
    int configAdminConsole(const XmlNode *pNode);
    int configSysShmDirs(char *pConfDir);
    int configTuning(const XmlNode *pRoot);
    void setMaxConns(int32_t conns);
    void setMaxSSLConns(int32_t conns);
    int configSecurity(const XmlNode *pRoot);
    int configAccessDeniedDir(const XmlNode *pNode);
    int denyAccessFiles(HttpVHost *pVHost, const char *pFile, int regex);
    int configMime(const XmlNode *pRoot);
    void testAndFixDirs(const char *pSuffix, uid_t uid, gid_t gid, int mod);
    int configServerBasic2(const XmlNode *pRoot, const char *pSwapDir);
    int configMultiplexer(const XmlNode *pNode);
    void configVHTemplateToListenerMap(const XmlNodeList *pList,
                                       TPointerList<HttpListener> &listeners,
                                       XmlNode *pVhConfNode, XmlNode *pTmpConfNode, const char *pTemplateName);
    int configVHTemplate(const XmlNode *pNode);
    int configVHTemplates(const XmlNode *pRoot);
    int configVHosts(const XmlNode *pRoot);
    int configServerBasics(int reconfig, const XmlNode *pRoot);
    int configModules(const XmlNode *pRoot);
    int initGroups();
    int loadAdminConfig(XmlNode *pRoot);
    int configIpToGeo(const XmlNode *pNode);
    int configIpToLoc(const XmlNode *pNode);
    int configChroot(const XmlNode *pRoot);
    int configServer(int reconfig, XmlNode *pRoot);
    void enableCoreDump();
    int changeUserChroot();
//     int reconfigVHost( const char *pVHostName, HttpVHost * &pVHost, XmlNode* pRoot );
//     void reconfigVHost( char *pVHostName, XmlNode* pRoot );
    void setServerRoot(const char *pRoot);
    int initServer(XmlNode *pRoot, int reconfig);
    int initServer(XmlNode *pRoot, int &iReleaseXmlTree, int reconfig);

    void chmodDirToAll(const char *path, struct stat &sb);
    void verifyStatDir(const char *path);

    void configZconfSvrConf(const XmlNode *pRoot);

    int configLsrecaptchaWorker(const XmlNode *pNode);
    int configLsrecaptchaContexts();
    int initLsrecaptcha(const XmlNode * pNode);

public:
    void hideServerSignature(int sv);

};

#include <http/userdir.h>
#include <http/reqparserparam.h>

int HttpServerImpl::authAdminReq(char *pAuth)
{
    char *pEnd = strchr(pAuth, '\n');
    if (strncasecmp(pAuth, "auth:", 5) != 0)
    {
        LS_ERROR("[ADMIN] missing authentication.");
        return LS_FAIL;
    }
    if (!pEnd)
    {
        LS_ERROR("[ADMIN] missing '\n' in authentication request.");
        return LS_FAIL;
    }
    char *pUserName = pAuth + 5;
    char *pPasswd = (char *)memchr(pUserName, ':', pEnd - pUserName);
    int nameLen;
    if (!pPasswd)
    {
        LS_ERROR("[ADMIN] invald authenticator.");
        return LS_FAIL;
    }
    nameLen = pPasswd - pUserName;
    *pPasswd++ = 0;
    HttpVHost *pAdmin = m_vhosts.get(DEFAULT_ADMIN_SERVER_NAME);
    if (!pAdmin)
    {
        LS_ERROR("[ADMIN] missing admin vhost.");
        return LS_FAIL;
    }
    UserDir *pRealm = pAdmin->getRealm(ADMIN_USERDB);
    if (!pRealm)
    {
        LS_ERROR("[ADMIN] missing authentication realm.");
        return LS_FAIL;
    }
    *pEnd = 0;
    int ret = pRealm->authenticate(NULL, pUserName, nameLen, pPasswd,
                                   ENCRYPT_PLAIN, NULL);
    if (ret != 0)
        LS_ERROR("[ADMIN] authentication failed!");
    *pEnd = '\n';
    return ret;
}


void HttpServerImpl::setSwapDir(const char *pDir)
{
    if (pDir)
    {
        m_sSwapDirectory = pDir;
        chown(m_sSwapDirectory.c_str(),
              ServerProcessConfig::getInstance().getUid(),
              ServerProcessConfig::getInstance().getGid());
    }
}


int HttpServerImpl::start()
{
    m_pid = getpid();
    DateTime::s_curTime = time(NULL);
    HttpSignals::init(sigchild);
    HttpCgiTool::buildServerEnv();
    if (isServerOk() == -1)
        return LS_FAIL;
    LS_NOTICE("[Child: %d] Setup swapping space...", m_pid);
    if (setupSwap() == - 1)
    {
        LS_ERROR("[Child: %d] Failed to setup swapping space!", m_pid);
        return LS_FAIL;
    }
    LS_NOTICE("[Child: %d] %s starts successfully!", m_pid,
              HttpServerVersion::getVersion());
    ConnLimitCtrl::getInstance().setListeners(&m_listeners);
    m_lStartTime = time(NULL);

    // if child 1
    if (1 == HttpServerConfig::getInstance().getProcNo())
        ZConfManager::getInstance().sendStartUp();
    m_dispatcher.run();
    LS_NOTICE("[Child: %d] Start shutting down gracefully ...", m_pid);
    gracefulShutdown();
    LS_NOTICE("[Child: %d] Shut down successfully! ", m_pid);
    return 0;

}


int HttpServerImpl::shutdown()
{
    LS_NOTICE("[Child: %d] Shutting down ...!", m_pid);
    HttpSignals::setSigStop();
    return 0;
}


int HttpServerImpl::generateStatusReport()
{
    char achBuf[1024] = "";
    if (ServerProcessConfig::getInstance().getChroot() != NULL)
    {
        lstrncpy(achBuf,
                ServerProcessConfig::getInstance().getChroot()->c_str(),
                sizeof(achBuf));
    }
    lstrncat(achBuf, sStatDir, sizeof(achBuf));
    lstrncat(achBuf, "/.status", sizeof(achBuf));
    LOG4CXX_NS::Appender *pAppender = LOG4CXX_NS::Appender::getAppender(achBuf);
    pAppender->setAppendMode(0);
    if (pAppender->open())
    {
        LS_ERROR("Failed to open the status report: %s!", achBuf);
        return LS_FAIL;
    }
    int ret = m_listeners.writeStatusReport(pAppender->getfd());
    if (!ret)
        ret = m_vhosts.writeStatusReport(pAppender->getfd());
    if (ret)
        LS_ERROR("Failed to generate the status report!");
    char achBuf1[1024];
    ret = snprintf(achBuf1, 1024, "DEBUG_LOG: %d, VERSION: %s-%s\n",
                   (LS_LOG_ENABLED(LOG4CXX_NS::Level::DBG_LESS)) ? 1 : 0,
                   PACKAGE_VERSION, LS_PLATFORM);
    pAppender->append(achBuf1, ret);

    pAppender->append("EOF\n", 4);
    pAppender->close();
    return 0;
}


int generateConnReport(int fd)
{
    ConnLimitCtrl &ctrl = ConnLimitCtrl::getInstance();
    char achBuf[4096];
    int SSLConns = ctrl.getMaxSSLConns() - ctrl.availSSLConn();
    HttpStats::getReqStats()->finalizeRpt();
    if (ctrl.getMaxConns() - ctrl.availConn() - HttpStats::getIdleConns() < 0)
        HttpStats::setIdleConns(ctrl.getMaxConns() - ctrl.availConn());
    int n = ls_snprintf(achBuf, 4096,
                        "BPS_IN: %ld, BPS_OUT: %ld, "
                        "SSL_BPS_IN: %ld, SSL_BPS_OUT: %ld\n"
                        "MAXCONN: %d, MAXSSL_CONN: %d, PLAINCONN: %d, "
                        "AVAILCONN: %d, IDLECONN: %d, SSLCONN: %d, AVAILSSL: %d\n"
                        "REQ_RATE []: REQ_PROCESSING: %d, REQ_PER_SEC: %d, TOT_REQS: %d\n",
                        HttpStats::getBytesRead() / 1024,
                        HttpStats::getBytesWritten() / 1024,
                        HttpStats::getSSLBytesRead() / 1024,
                        HttpStats::getSSLBytesWritten() / 1024,
                        ctrl.getMaxConns(), ctrl.getMaxSSLConns(),
                        ctrl.getMaxConns() - SSLConns - ctrl.availConn(),
                        ctrl.availConn(), HttpStats::getIdleConns(),
                        SSLConns, ctrl.availSSLConn(),
                        ctrl.getMaxConns() - ctrl.availConn()
                        - HttpStats::getIdleConns(),
                        HttpStats::getReqStats()->getRPS(),
                        HttpStats::getReqStats()->getTotal());
    write(fd, achBuf, n);

    HttpStats::setBytesRead(0);
    HttpStats::setBytesWritten(0);
    HttpStats::setSSLBytesRead(0);
    HttpStats::setSSLBytesWritten(0);
    HttpStats::getReqStats()->reset();
    return 0;
}


int HttpServerImpl::generateProcessReport(int fd)
{
    char achBuf[4096];
    long delta = time(NULL) - m_lStartTime;
    long days;
    int hours, mins, seconds;
    seconds = delta % 60;
    delta /= 60;
    mins = delta % 60;
    delta /= 60;
    hours = delta % 24;
    days = delta / 24;
    char *p = achBuf;
    p += ls_snprintf(p, &achBuf[4096] - p,
                     "VERSION: LiteSpeed Web Server/%s/%s\n",
                     "Open",
                     PACKAGE_VERSION);
    p += ls_snprintf(p, &achBuf[4096] - p, "UPTIME:");

    if (days)
        p += ls_snprintf(p, &achBuf[4096] - p, " %ld day%s", days,
                         (days > 1) ? "s" : "");
    p += ls_snprintf(p, &achBuf[4096] - p, " %02d:%02d:%02d\n", hours, mins,
                     seconds);
    write(fd, achBuf, p - achBuf);
    return 0;

}


void HttpServerImpl::setRTReportName(int proc)
{
    if (proc != 1)
    {
        char achBuf[256];
        ls_snprintf(achBuf, 256, "%s/.rtreport.%d", sStatDir, proc);
        m_sRTReportFile.setStr(achBuf);
    }
}


int HttpServerImpl::generateRTReport()
{
    LOG4CXX_NS::Appender *pAppender = LOG4CXX_NS::Appender::getAppender(
                                          m_sRTReportFile.c_str());
    pAppender->setAppendMode(0);
    if (pAppender->open())
    {
        LS_ERROR("Failed to open the real time report: %s!", m_sRTReportFile.c_str());
        return LS_FAIL;
    }
    int ret;
    ret = generateProcessReport(pAppender->getfd());
    ret = generateConnReport(pAppender->getfd());
    ret = m_listeners.writeRTReport(pAppender->getfd());
    if (!ret)
        ret = m_vhosts.writeRTReport(pAppender->getfd());
    if (ret)
        LS_ERROR("Failed to generate the real time report!");
    ret = ExtAppRegistry::generateRTReport(pAppender->getfd());
    ret = ClientCache::getClientCache()->generateBlockedIPReport(
              pAppender->getfd());

    pAppender->append("EOF\n", 4);
    pAppender->close();
    return 0;
}


HttpListener *HttpServerImpl::newTcpListener(const char *pName,
        const char  *pAddr)
{
    HttpListener *pListener = new HttpListener(pName, pAddr);
    if (pListener == NULL)
        return pListener;
    int ret;
    for (int i = 0; i < 3; ++i)
    {
        ret = pListener->start();
        if (!ret)
            return pListener;
        if (errno == EACCES)
            break;
        ls_sleep(100);
    }
    LS_ERROR("HttpListener::start(): Can't listen at address %s: %s!", pName,
             ::strerror(ret));
    //LS_ERROR( "Unable to add listener [%s]!", pName ));
    delete pListener;
    return NULL;
}


HttpListener *HttpServerImpl::addListener(const char *pName,
        const char *pAddr)
{
    HttpListener *pListener = NULL;
    if (!pName)
        return NULL;
    pListener = m_listeners.get(pName, pAddr);
    if (pListener)
    {
        LS_NOTICE("Reuse existing Listener [%s] [%s].", pName, pAddr);
        return pListener;
    }
    pListener = m_oldListeners.get(pName, pAddr);
    if (!pListener)
    {
        pListener = newTcpListener(pName, pAddr);
        if (pListener)
            LS_NOTICE("Created new Listener [%s].", pName);
        else
        {
            LS_ERROR("HttpServer::addListener(%s) failed to create new listener"
                     , pName);
            return NULL;
        }
    }
    else
    {
        pListener->beginConfig();
        m_oldListeners.remove(pListener);
        LS_NOTICE("Reuse current listener [%s].", pName);
    }
    m_listeners.add(pListener);
    return pListener;
}


int HttpServerImpl::removeListener(const char *pName)
{
    HttpListener *pListener = m_listeners.get(pName, NULL);
    if (pListener != NULL)
    {
        LS_DBG_L("Removed listener [%s].", pName);
        m_listeners.remove(pListener);
        pListener->stop();
        if (pListener->getVHostMap()->getRef() > 0)
            m_toBeReleasedListeners.add(pListener);
        else
            delete pListener;
    }
    return 0;
}


int HttpServerImpl::addVHost(HttpVHost *pVHost)
{
    assert(pVHost);
//    if ( m_vhosts.size() > MAX_VHOSTS )
//    {
//        return LS_FAIL;
//    }
    int ret = m_vhosts.add(pVHost);
    return ret;
}


int HttpServerImpl::updateVHost(const char *pVHostName, HttpVHost *pVHost)
{
    HttpVHost *pOld = getVHost(pVHostName);
    if (pOld)
    {
        m_vhosts.remove(pOld);
        pVHost->enable(pOld->isEnabled());
        m_listeners.removeVHostMappings(pOld);
        assert(pOld->getMappingRef() == 0);
        if (pOld->getRef() > 0)
        {
            pOld->getAccessLog()->setAsyncAccessLog(0);
            m_toBeReleasedVHosts.push_back(pOld);
        }
        else
            delete pOld;
    }
    return addVHost(pVHost);

}


int HttpServerImpl::removeVHost(const char *pName)
{
    HttpVHost *pVHost = getVHost(pName);
    if (pVHost != NULL)
    {
        int ret = m_vhosts.remove(pVHost);
        if (ret)
        {
            LS_ERROR("HttpServer::removeVHost(): virtual host %s failed! ",
                     pName);
        }
        else
            LS_DBG_L("Removed virtual host %s!", pName);
        return ret;
    }
    else
        return 0;
}


int HttpServerImpl::mapListenerToVHost(HttpListener *pListener,
                                       HttpVHost    *pVHost,
                                       const char *pDomains)
{
    if (!pListener)
        return LS_FAIL;
    return pListener->getVHostMap()->mapDomainList(pVHost, pDomains);
}


int HttpServerImpl::mapListenerToVHost(HttpListener *pListener,
                                       const char *pKey,
                                       const char *pVHostName)
{
    HttpVHost *pVHost = getVHost(pVHostName);
    return mapListenerToVHost(pListener, pVHost, pKey);
}


int HttpServerImpl::mapListenerToVHost(const char *pListenerName,
                                       const char *pKey,
                                       const char *pVHostName)
{
    HttpListener *pListener = getListener(pListenerName);
    return mapListenerToVHost(pListener, pKey, pVHostName);
}


int HttpServerImpl::removeVHostFromListener(const char *pListenerName,
        const char *pVHostName)
{
    HttpListener *pListener = getListener(pListenerName);
    HttpVHost *pVHost = getVHost(pVHostName);
    if ((pListener == NULL) || (pVHost == NULL))
        return EINVAL;
    int ret = pListener->getVHostMap()->removeVHost(pVHost);
    LS_ERROR("Deassociates [%s] with [%s] on %s!",
             pVHostName, pListenerName, (ret ? "failed" : "succeed"));
    return ret;
}


void HttpServerImpl::beginConfig()
{
    assert(m_oldListeners.size() == 0);
    m_oldListeners.swap(m_listeners);
    m_vhosts.swap(m_orgVHosts);
    HttpVHost *pVHost = m_orgVHosts.get(DEFAULT_ADMIN_SERVER_NAME);
    if (pVHost)
    {
        m_orgVHosts.remove(pVHost);
        m_vhosts.add(pVHost);
    }
    //ClientCache::getClientCache()->dirtyAll();
}


void HttpServerImpl::updateMapping()
{
    int n = m_listeners.size();
    for (int i = 0; i < n; ++i)
        m_listeners[i]->getVHostMap()->updateMapping(m_vhosts);
}


void HttpServerImpl::endConfig(int error)
{
    if (error)
    {
        m_vhosts.moveNonExist(m_orgVHosts);
        m_listeners.moveNonExist(m_oldListeners);
        updateMapping();
    }
    m_oldListeners.saveInUseListnersTo(m_toBeReleasedListeners);
    m_orgVHosts.appendTo(m_toBeReleasedVHosts);
    m_listeners.endConfig();
}


int HttpServerImpl::enableVHost(const char *pVHostName, int enable)
{
    HttpVHost *pVHost = getVHost(pVHostName);
    if (pVHost)
    {
        pVHost->enable(enable);
        return 0;
    }
    return LS_FAIL;
}


void HttpServerImpl::onTimerSecond()
{
    HttpRespHeaders::updateDateHeader();
    HttpLog::onTimer();
    ClientCache::getClientCache()->onTimer();
    m_vhosts.onTimer();
    if (m_lStartTime > 0)
        generateRTReport();

    ServerInfo::getServerInfo()->setAdnsOp(1);
    Adns::getInstance().trimCache();
    ServerInfo::getServerInfo()->setAdnsOp(0);
}


void HttpServerImpl::onTimer10Secs()
{
    ExtAppRegistry::onTimer();
    HttpResourceManager::getInstance().onTimer();
    static int s_timeOut = 3;
    s_timeOut --;
    if (!s_timeOut)
    {
        s_timeOut = 3;
        onTimer30Secs();
    }
}


void HttpServerImpl::onTimer30Secs()
{
    ClientCache::getClientCache()->onTimer30Secs();
    StaticFileCache::getInstance().onTimer();
    m_vhosts.onTimer30Secs();
    static int s_timeOut = 2;
    s_timeOut --;
    if (!s_timeOut)
    {
        s_timeOut = 2;
        onTimer60Secs();
    }
    if (HttpStats::get503AutoFix() && (HttpStats::get503Errors() >= 30))
    {
        LS_NOTICE("There are %d '503 Errors' in last 30 seconds, "
                  "request a graceful restart", HttpStats::get503Errors());
        ServerInfo::getServerInfo()->setRestart(1);

    }
    HttpStats::set503Errors(0);
    SslTicket::onTimer();
}


static int readVersionStr(const char *s)
{
    int ver = 0;
    //a.b.c(.d) will return (((a * 100 + b) * 100) + c) * 100 + d
    //a, b, c and d will take up to 2 digits
    int a = 0, b = 0, c = 0, d = 0;
    //The .d may not exist
    if (sscanf(s, "%d.%d.%d.%d", &a, &b, &c, &d) >= 3)
    {
        if (a > 0 && a < 10)
            ver = (((a * 100 + b % 100) * 100) + c % 100) * 100 + d % 100;
    }
    return ver;
}


static int readVersion(const char *path)
{
    //a.b.c(.d) will return (((a * 100 + b) * 100) + c) * 100 + d
    //a, b, c and d will take up to 2 digits
    char s[21] = {0};
    int ver = 0;
    FILE *fp = fopen(path, "r");
    if (fp)
    {
        ssize_t bytes = fread(s, 1, 20, fp);
        fclose(fp);
        if (bytes == -1)
            return ver;
        s[bytes] = 0;
        ver = readVersionStr(s);
    }
    return ver;
}

static int autoUpdCheck(void *pArg, HttpFetch *pHttpFetch)
{
    //HttpServerImpl *pServerImpl = (HttpServerImpl *)pArg;
    int istatusCode = pHttpFetch->getStatusCode() ;
    const char *path = pHttpFetch->getResult()->getTempFileName();
    if (istatusCode != 200)
        unlink(path);
    else
    {
        chmod(path, 0744);
        int newVer = readVersion(path);
        if (newVer)
        {
            int curVer = readVersionStr(PACKAGE_VERSION);
            if (newVer > curVer)
                LS_NOTICE("[!!!UPDATE!!!] new stable version %d.%d.%d.%d is available.\n",
                          newVer / 1000000, (newVer / 10000) % 100,
                          (newVer / 100) % 100, newVer % 100);
        }
    }
    return 0;
}

static int autoUpdCheckCb(void *pArg, HttpFetch *pHttpFetch)
{
    //HttpServerImpl *pServerImpl = (HttpServerImpl *)pArg;
    int istatusCode = pHttpFetch->getStatusCode() ;
    const char *path = pHttpFetch->getResult()->getTempFileName();
    if (istatusCode != 200)
        unlink(path);
    else
    {
        chmod(path, 0744);
        int newVer = readVersion(path);
        if (newVer)
        {
            int curVer = readVersionStr(PACKAGE_VERSION);
            if (newVer > curVer)
                LS_NOTICE("[!!!UPDATE!!!] current branch new version %d.%d.%d.%d is available.\n",
                          newVer / 1000000, (newVer / 10000) % 100,
                          (newVer / 100) % 100, newVer % 100);
        }
    }
    return 0;
}

static const char *detectCp()
{
    struct stat st;
    const char *type;
    /*if (stat("/usr/local/cpanel", &st) == 0)
        type = "cpanel";
    else */if (stat("/usr/local/CyberCP", &st) == 0)
        type = "cyberpanel";
    else if (stat("/usr/local/CyberPanel", &st) == 0)
        type = "cyberpanel";
    else if (stat("/usr/local/directadmin", &st) == 0)
        type = "da";
    else if (stat("/etc/httpd/conf/plesk.conf.d/", &st) == 0
             || stat("/etc/apache2/plesk.conf.d/", &st) == 0)
        type = "plesk";
    else if (stat("/usr/local/interworx", &st) == 0)
        type = "interworx";
    else if (!MainServerConfig::getInstance().getDisableWebAdmin())
        type = "webadmin";
    else
        type = "unknown";
    return type;
}

static char *OsDetect(char *s, int max_len)
{
    struct utsname name;
    memset(&name, 0, sizeof(name));
    if (uname(&name) == -1)
        lstrncpy(s, "unknown", max_len - 1);
    else
        snprintf(s, max_len - 1, "%s_%s_%s", name.sysname, name.release, name.machine);
    return s;
}

static char *detectPlat(char *str, int max_len)
{
    char *p = str;
    AutoStr2 sPlat;
    sPlat.setStr(MainServerConfig::getInstance().getServerRoot());
    sPlat.append("/PLAT", 5);
    FILE *fp = fopen(sPlat.c_str(), "r");
    if (fp)
    {
        if (!fgets(str, max_len - 1, fp))
            str[0] = 0x00;
        else
        {
            p = StringTool::strTrim(p);
            char *p1 = p;
            while(*p1 && !isspace(*p1))
                ++p1;
            *p1 = 0x00;
        }

        fclose(fp);
    }
    return p;
}

//autoupdate checking, this only do once per day, won't use much resource
void HttpServerImpl::checkOLSUpdate()
{
    time_t t = time(NULL);
    struct tm tstm;
    struct tm *tl = &tstm;
    localtime_r(&t,tl);

    struct stat sb;
    AutoStr2 sAutoUpdFile;
    sAutoUpdFile.setStr(MainServerConfig::getInstance().getServerRoot());
    sAutoUpdFile.append("autoupdate/release", 18);

    if (stat(sAutoUpdFile.c_str(), &sb) != -1)
    {
        if (t - sb.st_mtime < 86400) //Less than 1 day
            return ;
        else
            unlink(sAutoUpdFile.c_str());
    }

    if (m_pAutoUpdFetch[0])
        delete m_pAutoUpdFetch[0];
    m_pAutoUpdFetch[0] = new HttpFetch();
    m_pAutoUpdFetch[0]->setTimeout(15);  //Set Req timeout as 30 seconds
    m_pAutoUpdFetch[0]->setCallBack(autoUpdCheck, this);
    GSockAddr addrResponder, addrResponder2;
    char sUrl[256];
    char osstr[64] = {0};
    char plat[64] = {0};
    const char *httpUrl = "http://openlitespeed.org/";
    addrResponder.setHttpUrl(httpUrl, strlen(httpUrl));
    addrResponder2.setHttpUrl(httpUrl, strlen(httpUrl));
    snprintf(sUrl, sizeof(sUrl), "%s%s%s%s%s%s%s%s%s", httpUrl,
             "packages/release?ver=", PACKAGE_VERSION, "&os=",
             OsDetect(osstr, 64), "&env=", detectCp(),
#ifdef PREBUILT_VERSION
             "_pre_",
#else
             "_src_",
#endif
             detectPlat(plat, 64));
    m_pAutoUpdFetch[0]->startReq(sUrl, 1, 1, NULL, 0, sAutoUpdFile.c_str(), NULL,
                               addrResponder);

    /**
     * Since now on, we will fetch the latest version of current branch at
     * the same time and will send a notification if have newer version of
     * current branch (TBD)
     */
    sAutoUpdFile.setStr(MainServerConfig::getInstance().getServerRoot());
    sAutoUpdFile.append("autoupdate/releasecb", 20);
    if (m_pAutoUpdFetch[1])
        delete m_pAutoUpdFetch[1];
    m_pAutoUpdFetch[1] = new HttpFetch();

    m_pAutoUpdFetch[1]->setTimeout(15);  //Set Req timeout as 30 seconds
    m_pAutoUpdFetch[1]->setCallBack(autoUpdCheckCb, this);
    int curVer = readVersionStr(PACKAGE_VERSION);
    snprintf(sUrl, 255, "http://openlitespeed.org/packages/relbr%d.%d?",
             curVer / 1000000, (curVer / 10000) % 100);
    m_pAutoUpdFetch[1]->startReq(sUrl, 1, 1, NULL, 0, sAutoUpdFile.c_str(), NULL,
                              addrResponder2);
}


void HttpServerImpl::onTimer60Secs()
{
    m_toBeReleasedListeners.releaseUnused();
    m_toBeReleasedVHosts.releaseUnused();
    m_toBeReleasedContext.releaseUnused(DateTime::s_curTime,
                                        HttpServerConfig::getInstance().getConnTimeout());

#ifndef IS_LSCPD
    static int s_count = 0;
    if (s_count == 0)
    {
        checkOLSUpdate();
        s_count = 30;    //Check it every 30 minutes
    }
    --s_count;
#endif
}


void HttpServerImpl::onTimer()
{
    onTimerSecond();
    static int s_timeOut = 10;
    s_timeOut --;
    if (!s_timeOut)
    {
        s_timeOut = 10;
        onTimer10Secs();

    }
}


void HttpServerImpl::offsetChroot()
{

    char achTemp[512];
    AutoStr2 *pChroot = ServerProcessConfig::getInstance().getChroot();
    lstrncpy(achTemp, StdErrLogger::getInstance().getLogFileName(), sizeof(achTemp));
    StdErrLogger::getInstance().setLogFileName(achTemp + pChroot->len());
    HttpLog::offsetChroot(pChroot->c_str(), pChroot->len());
    ServerInfo::getServerInfo()->m_pChroot =
        ServerInfo::getServerInfo()->dupStr(pChroot->c_str(), pChroot->len());
    m_vhosts.offsetChroot(pChroot->c_str(), pChroot->len());
}

//FIXME: why we have this
// int HttpServerImpl::offsetChroot( char * dest, const char *path )
// {
//     AutoStr2 *pChroot = ServerProcessConfig::getInstance().getChroot();
//     if ( pChroot->c_str() &&
//         (strncmp( pChroot->c_str(), path, pChroot->len() ) == 0) &&
//         ( *(path + pChroot->len() ) == '/' ) )
//     {
//         struct stat st;
//         if ( nio_stat( dest, &st ) == -1 )
//         {
//             if ( nio_stat( dest + pChroot->len(), &st ) == 0 )
//             {
//                 memmove( dest, dest + pChroot->len(),
//                          strlen( dest ) - pChroot->len() + 1 );
//             }
//         }
//     }
//     return 0;
// }


void HttpServerImpl::releaseAll()
{
    ExtAppRegistry::stopAll();
    StaticFileCache::getInstance().releaseAll();
    m_listeners.clear();
    m_oldListeners.clear();
    m_toBeReleasedListeners.clear();
    m_vhosts.release_objects();
    m_toBeReleasedVHosts.release_objects();
    ::signal(SIGCHLD, SIG_DFL);
    ExtAppRegistry::shutdown();
    ClientCache::clearObjPool();
    HttpResourceManager::getInstance().releaseAll();
    delete HttpAioSendFile::getHttpAioSendFile();
    HttpMime::releaseMIMEList();
}


int HttpServerImpl::isServerOk()
{
    if (m_listeners.size() == 0)
    {
        LS_ERROR("No listener is available, stop server!");
        return LS_FAIL;
    }
    if (m_vhosts.size() == 0)
    {
        LS_ERROR("No virtual host is available, stop server!");
        return LS_FAIL;
    }
    return 0;
}


int removeMatchFile(const char *pDir, const char *prefix)
{
    DIR *d;
    struct dirent *entry;
    int i = 0;
    int prefixLen = strlen(prefix);
    char achTemp[512];
    memccpy(achTemp, pDir, 0, 510);
    achTemp[511] = 0;
    int dirLen = strlen(achTemp);
    if (achTemp[dirLen - 1] != '/')
    {
        achTemp[dirLen++] = '/';
        achTemp[dirLen] = 0;
    }

    if ((d = opendir(achTemp)) == NULL)
        return LS_FAIL;

    while ((entry = readdir(d)) != NULL)
    {
        if (!prefixLen || strncmp(entry->d_name, prefix, prefixLen) == 0)
        {
            memccpy(&achTemp[dirLen], entry->d_name, 0, 510 - dirLen);
            unlink(achTemp);
            ++i;
        }
    }
    if (closedir(d))
        return LS_FAIL;

    return i ;
}


int HttpServerImpl::gracefulShutdown()
{
    //suspend listener socket,
    //m_listeners.suspendAll();
    //close all listener socket.
    m_lStartTime = -1;
    m_listeners.stopAll();
    //close keepalive connections
    HttpServerConfig::getInstance().setMaxKeepAliveRequests(0);
    HttpServerConfig::getInstance().setKeepAliveTimeout(0);
    //HttpServerConfig::getInstance().setConnTimeOut(15);
    NtwkIOLink::setPrevToken(TIMER_PRECISION - 1);
    NtwkIOLink::setToken(0);
    MultiplexerFactory::getMultiplexer()->timerExecute();  //close keepalive connections
    // change to lower priority
    nice(3);
    //linger for a while
    return m_dispatcher.linger(
               HttpServerConfig::getInstance().getRestartTimeout());

}


int HttpServerImpl::reinitMultiplexer()
{
    int iProcNo;
    if (MultiplexerFactory::s_iMultiplexerType)
    {
        if (m_dispatcher.reinit())
            return LS_FAIL;
        MultiplexerFactory::getMultiplexer()->add(
            &StdErrLogger::getInstance(), POLLIN | POLLHUP | POLLERR);
    }
    int n = m_listeners.size();
    iProcNo = HttpServerConfig::getInstance().getProcNo();
    for (int i = 0; i < n; ++i)
    {
        if (!(m_listeners[i]->getBinding() & (1L << (iProcNo - 1))))
            m_listeners[i]->stop();
        else if (MultiplexerFactory::s_iMultiplexerType)
            MultiplexerFactory::getMultiplexer()->add(m_listeners[i],
                    POLLIN | POLLHUP | POLLERR);
    }
    ServerInfo::getServerInfo()->setAdnsOp(1);
    initAdns();
    ServerInfo::getServerInfo()->setAdnsOp(0);
#if defined(LS_AIO_USE_SIGFD) || defined(LS_AIO_USE_SIGNAL)
    SigEventDispatcher::init();
#endif
    return 0;
}


int HttpServerImpl::setupSwap()
{
    char achDir[512];
    lstrncpy(achDir, getSwapDir(), sizeof(achDir));
    if (*(strlen(achDir) - 1 + achDir) != '/')
        lstrncat(achDir, "/", sizeof(achDir));
    if (!GPath::isValid(achDir))
    {
        char *p = strchr(achDir + 1, '/');
        while (p)
        {
            *p = 0;
            mkdir(achDir, 0700);
            *p++ = '/';
            p = strchr(p, '/');
        }
    }
    if (!GPath::isWritable(achDir))
    {
        LS_WARN("Specified swapping directory is not writable:%s,"
                " use default!", achDir);
        lstrncpy(achDir, DEFAULT_SWAP_DIR, sizeof(achDir));
        if (*(strlen(achDir) - 1 + achDir) != '/')
            lstrncat(achDir, "/", sizeof(achDir));
        mkdir(achDir, 0700);
    }
    if (!GPath::isWritable(achDir))
    {
        LS_ERROR("Swapping directory is not writable:%s", achDir);
        fprintf(stderr, "Swapping directory is not writable:%s", achDir);
        return LS_FAIL;
    }
    if (HttpServerConfig::getInstance().getProcNo() != 1)
    {
        ls_snprintf(achDir + strlen(achDir), 256 - strlen(achDir),
                    "s%d/", HttpServerConfig::getInstance().getProcNo());
        mkdir(achDir, 0700);
    }
    if ((strncmp(achDir, DEFAULT_SWAP_DIR,
                 strlen(DEFAULT_SWAP_DIR)) == 0) || (getuid() != 0))
        removeMatchFile(achDir, "");
    lstrncat(achDir, "s-XXXXXX", sizeof(achDir));
    VMemBuf::setTempFileTemplate(achDir);
    return 0;

}


void HttpServerImpl::setBlackBoard(char *pBuf)
{
    ServerInfo *pInfo = new(pBuf) ServerInfo(
        pBuf + (sizeof(ServerInfo) + 15) / 16 * 16,
        pBuf + 32760);
    if (pInfo)
    {
        PidRegistry::setSimpleList(&(pInfo->m_pidList));
        ServerInfo::setServerInfo(pInfo);
    }
}


void HttpServerImpl::configZconfSvrConf(const XmlNode *pRoot)
{
    ConfigCtx currentCtx("server", "zconfclient");
    const XmlNode *pNode = pRoot->getChild("zconfClient");
    const char *pConfName, *pAdcList, *pAuth;

    if (!pNode)
    {
        LS_DBG_L("[ZCONFCLIENT] Not configured.");
        return;
    }
    if (0 == currentCtx.getLongValue(pNode, "zconfEnable", 0, 1, 0))
        return;

    pConfName = ConfigCtx::getCurConfigCtx()->getTag(pNode, "zconfName");

    if (NULL == pConfName)
    {
        LS_ERROR("[ZCONFCLIENT] A ZconfName is required if client is enabled.");
        return;
    }

    // domain:port or ip:port, comma delimited
    pAdcList = ConfigCtx::getCurConfigCtx()->getTag(pNode, "zconfAdcList");
    if (NULL == pAdcList)
    {
        LS_ERROR("[ZCONFCLIENT] A ZconfAdcList is required if client is enabled.");
        return;
    }

    pAuth = ConfigCtx::getCurConfigCtx()->getTag(pNode, "zconfAuth");
    if (NULL == pAuth)
    {
        LS_ERROR("[ZCONFCLIENT] A ZconfAuth parameter is required if client is enabled.");
        return;
    }

    ZConfManager::getInstance().init(pAuth, pAdcList, pConfName);
}


int HttpServerImpl::addVirtualHostMapping(HttpListener *pListener,
        const char *value,
        const char *pVHostName)
{
    char pVHost[256] = {0};
    const char *p = strchr(value, ' ');

    if (!p)
        p = strchr(value, '\t');

    if (p && p - value < 256)
        memcpy(pVHost, value, p - value);
    else
        return LS_FAIL;

    if ((pVHostName) && (strcmp(pVHostName, pVHost) != 0))
        return 1;

    if (strcmp(pVHost, DEFAULT_ADMIN_SERVER_NAME) == 0)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "can't bind administration server to normal listener %s, "
                 "instead, configure listeners for administration server in "
                 "$SERVER_ROOT/admin/conf/admin_config.conf", pListener->getAddrStr());
        return LS_FAIL;
    }

    //at least move ahead for one char position
    const char *pDomains = p;
    skip_leading_space(&pDomains);

    if (pDomains == NULL)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "missing <domain> in <vhostMap> - vhost = %s listener = %s",
                 pVHost, pListener->getAddrStr());
        return LS_FAIL;
    }

    return pListener->mapDomainList(getVHost(pVHost), pDomains);
}


int HttpServerImpl::addVirtualHostMapping(HttpListener *pListener,
        const XmlNode *pNode,
        const char *pVHostName)
{
    const char *pVHost = ConfigCtx::getCurConfigCtx()->getTag(pNode, "vhost");

    if (pVHost == NULL)
        return LS_FAIL;

    if ((pVHostName) && (strcmp(pVHostName, pVHost) != 0))
        return 1;

    if (strcmp(pVHost, DEFAULT_ADMIN_SERVER_NAME) == 0)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "can't bind administration server to normal listener %s, "
                 "instead, configure listeners for administration server in "
                 "$SERVER_ROOT/admin/conf/admin_config.conf", pListener->getAddrStr());
        return LS_FAIL;
    }

    const char *pDomains = pNode->getChildValue("domain");

    if (pDomains == NULL)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "missing <domain> in <vhostMap> - vhost = %s listener = %s",
                 pVHost, pListener->getAddrStr());
        return LS_FAIL;
    }

    return pListener->mapDomainList(getVHost(pVHost), pDomains);
}


int HttpServerImpl::configVirtualHostMappings(HttpListener *pListener,
        const XmlNode *pNode,
        const char *pVHostName)
{
    int add = 0;

    if ((pNode != NULL) && (pListener))
    {
        XmlNodeList list;
        pNode->getAllChildren(list);
        XmlNodeList::const_iterator iter;

        for (iter = list.begin(); iter != list.end(); ++iter)
        {
            if (strcasecmp((*iter)->getName(), "map") == 0)
            {
                if (addVirtualHostMapping(pListener, (*iter)->getValue(),
                                          pVHostName) == 0)
                    ++add;
            }
        }
    }

    return add;
}


int HttpServerImpl::configListenerVHostMap(const XmlNode *pRoot,
        const char *pVHostName)
{
    const XmlNode *pNode = pRoot;
    const XmlNodeList *pList = pNode->getChildren("listener");

    if (pList)
    {
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
        {
            const XmlNode *pListenerNode = *iter;
            const char *pName = pListenerNode->getChildValue("name", 1);
            HttpListener *pListener = getListener(pName);

            if (pListener)
            {
                if ((configVirtualHostMappings(pListener, pListenerNode, pVHostName) > 0)
                    && (pVHostName))
                    pListener->endConfig();

                if (pListener->isSendZConf())
                    ZConfManager::getInstance().appendListener(pListener);
            }
        }
    }

    return 0;
}




HttpListener *HttpServerImpl::configListener(const XmlNode *pNode,
        int isAdmin)
{
    int iNumChildren;
    // extract listner info
    while (1)
    {
        if (strcmp(pNode->getName(), "listener") != 0)
            break;

        const char *pName = ConfigCtx::getCurConfigCtx()->getTag(pNode,  "name",
                            1);
        if (pName == NULL)
            break;

        ConfigCtx currentCtx(pName);
        const char *pAddr = ConfigCtx::getCurConfigCtx()->getTag(pNode,
                            "address");
        SslContext *pSSLCtx = NULL;
        if (pAddr == NULL)
            break;

        int secure = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "secure", 0,
                     1, 0);
        if (secure)
        {
            ConfigCtx currentCtx("ssl");
            pSSLCtx = ConfigCtx::getCurConfigCtx()->newSSLContext(pNode, pAddr, NULL);
            if (!pSSLCtx)
            {
                delete pSSLCtx;
                pSSLCtx = NULL;
                break;
            }
        }

        LS_DBG_L("Config listener [%s] [%s]", pName, pAddr);

        HttpListener *pListener = NULL;
        pListener = addListener(pName, pAddr);
        if (pListener == NULL)
        {
            LS_ERROR(&currentCtx, "failed to start listener on address %s!", pAddr);
            break;
        }


        if (!isAdmin)
        {
            const XmlNode *p0 = pNode->getChild("modulelist", 1);

            pListener->m_moduleConfig.init(
                ModuleManager::getInstance().getModuleCount());
            pListener->m_moduleConfig.inherit(ModuleManager::getInstance().getGlobalModuleConfig());

            const XmlNodeList *pModuleList = p0->getChildren("module");
            if (pModuleList)
                ModuleConfig::parseConfigList(pModuleList, &pListener->m_moduleConfig,
                                              LSI_CFG_LISTENER, pName);

            pListener->getSessionHooks()->inherit(NULL, 1);
            ModuleManager::getInstance().applyConfigToIolinkRt(
                pListener->getSessionHooks(),
                &pListener->m_moduleConfig);
        }
//        else
//            pListener->getSessionHooks()->disableAll();

        if (pSSLCtx)
        {
            pListener->getVHostMap()->setSslContext(pSSLCtx);
            if (pSSLCtx->initSNI(pListener->getVHostMap()) == -1)
            {
                LS_WARN(&currentCtx,
                        "TLS extension is not available in openssl library on this server, "
                        "server name indication is disabled, you will not able to use use per vhost"
                        " SSL certificates sharing one IP. Please upgrade your OpenSSL lib if you want to use this feature."
                       );

            }
        }

        iNumChildren = HttpServerConfig::getInstance().getChildren();
        if (iNumChildren > 1)
        {
            int binding = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "binding",
                          LONG_MIN, LONG_MAX, -1);

            if ((binding & ((1 << iNumChildren) - 1)) == 0)
            {
                LS_WARN(&currentCtx, "This listener is not bound to any server process, "
                        "it is inaccessible.");
            }
            pListener->setBinding(binding);
        }

        const char *pAdcPortList = NULL;
        pListener->setSendZConf(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                                    "zconfSend", 0, 1, 1));
        if (!isAdmin && pListener->isSendZConf())
        {
            pAdcPortList = pNode->getChildValue("zconfPortList");
            if (pAdcPortList)
                pListener->setAdcPortList(pAdcPortList);
        }

        pListener->setName(pName);
        pListener->setAdmin(isAdmin);
        return pListener;
    }

    return NULL;
}


int HttpServerImpl::configListeners(const XmlNode *pRoot, int isAdmin)
{
    XmlNodeList list;
    int c = pRoot->getAllChildren(list);
    int add = 0 ;

    for (int i = 0 ; i < c ; ++ i)
    {
        XmlNode *pListenerNode = list[i];

        if (configListener(pListenerNode, isAdmin) != NULL)
            ++add ;
    }

    return add;
}


//#define ADMIN_CONFIG_NODE           "AdminConfigNode"
int HttpServerImpl::startAdminListener(const XmlNode *pRoot, const char *pName)
{
    ConfigCtx currentCtx("admin", "listener");

    if (configListeners(pRoot->getChild(pName), 1) <= 0)
    {
        LS_ERROR(&currentCtx, "No listener is available for admin virtual host!");
        return LS_FAIL;
    }
    return 0;
}


int HttpServerImpl::startListeners(const XmlNode *pRoot)
{

    ConfigCtx currentCtx("server", "listener");

    if (configListeners(pRoot, 0) <= 0)
        LS_WARN(&currentCtx, "No listener is available for normal virtual host!");

    return 0;
}
#define DEFAULT_ADMIN_FCGI_NAME     "AdminPHP"
#define DEFAULT_ADMIN_PHP_FCGI      "$VH_ROOT/fcgi-bin/admin_php"
#define DEFAULT_ADMIN_PHP_FCGI_URI  "UDS:/" DEFAULT_TMP_DIR "/admin_php.sock"
#define ADMIN_PHP_SESSION           "$SERVER_ROOT/admin/tmp"


int HttpServerImpl::configAdminConsole(const XmlNode *pNode)
{
    const char *pChroot = MainServerConfig::getInstance().getChroot();
    int iChrootLen = MainServerConfig::getInstance().getChrootlen();
    char achPHPBin[MAX_PATH_LEN];
    ConfigCtx currentCtx("admin");

    if (ConfigCtx::getCurConfigCtx()->getAbsoluteFile(achPHPBin,
            DEFAULT_ADMIN_PHP_FCGI) ||
        (access(achPHPBin, X_OK) != 0))
    {
        LS_ERROR(&currentCtx, "missing PHP binary for admin server - %s!",
                 achPHPBin);
        return LS_FAIL;
    }

    const char *pURI = configAdminPhpUri(pNode);
    if (pURI == NULL)
        return LS_FAIL;
    LocalWorker *pFcgiApp = createAdminPhpApp(pChroot, iChrootLen, pURI,
                            achPHPBin, sizeof(achPHPBin));
    HttpVHost *pVHostAdmin = getVHost(DEFAULT_ADMIN_SERVER_NAME);
    if (!pVHostAdmin)
    {
        if ((pVHostAdmin = createAdminVhost(pFcgiApp, iChrootLen,
                                            achPHPBin, sizeof(achPHPBin))) == NULL)
            return LS_FAIL;
    }
    setAdminThrottleLimits(pVHostAdmin);
    pVHostAdmin->configSecurity(pNode);
    pVHostAdmin->initErrorLog(pNode, 0);
    pVHostAdmin->initAccessLog(pNode, 0);
   
    //test if file $SERVER_ROOT/conf/disablewebconsole exist
    //skip admin listener configuration
    if (!enableWebConsole())
        return 0;

    char achRules[] =
        "RewriteCond %{REQUEST_URI} !^/(index|login)\\.php\n"
        "RewriteCond %{REQUEST_URI} !^/view/(serviceMgr|confMgr|ajax_data|dashboard|logviewer|compilePHP|realtimestats)\\.php \n"
        "RewriteRule \\.php - [F]\n";
    pVHostAdmin->getRootContext().configRewriteRule(NULL, achRules, NULL);
    pVHostAdmin->getRootContext().enableRewrite(1);
   
    mapDomainList(pNode, pVHostAdmin);
    return 0;
}


const char *HttpServerImpl::configAdminPhpUri(const XmlNode *pNode)
{
    const char *pURI;

    pURI = pNode->getChildValue("phpFcgiAddr");

    if (pURI)
    {
        if ((strncasecmp(pURI, "UDS://", 6) != 0) &&
            (strncasecmp(pURI, "127.0.0.1:", 10) != 0) &&
            (strncasecmp(pURI, "localhost:", 10) != 0))
        {
            LS_WARN(ConfigCtx::getCurConfigCtx(),
                    "The PHP fast CGI for admin server must"
                    " use localhost interface"
                    " or unix domain socket, use default!");
            pURI = DEFAULT_ADMIN_PHP_FCGI_URI;
        }
    }
    else
        pURI = DEFAULT_ADMIN_PHP_FCGI_URI;

    GSockAddr addr;

    if (addr.set(pURI, NO_ANY))
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "failed to set socket address %s for %s!",
                 pURI, DEFAULT_ADMIN_FCGI_NAME);
        return NULL;
    }
    return pURI;
}


static void setPHPHandler(HttpContext *pCtx, HttpHandler *pHandler,
                          char *pSuffix)
{
    char achMime[100];
    ls_snprintf(achMime, 100, "application/x-httpd-%s", pSuffix);
    pCtx->initMIME();
    pCtx->getMIME()->addMimeHandler(pSuffix, achMime,
                                    pHandler, NULL,  TmpLogId::getLogId());
}


static int detectIP(char family, char *str, char *pEnd)
{
    struct ifi_info *pHead = NICDetect::get_ifi_info(family, 1);
    struct ifi_info *iter;
    char *pBegin = str;
    char temp[80];

    for (iter = pHead; iter != NULL; iter = iter->ifi_next)
    {
        if (iter->ifi_addr)
        {
            GSockAddr::ntop(iter->ifi_addr, temp, 80);

            if (family == AF_INET6)
            {
                const struct in6_addr *pV6 = & ((const struct sockaddr_in6 *)
                                                iter->ifi_addr)->sin6_addr;

                if ((!IN6_IS_ADDR_LINKLOCAL(pV6)) &&
                    (!IN6_IS_ADDR_SITELOCAL(pV6)) &&
                    (!IN6_IS_ADDR_MULTICAST(pV6)))
                {
                    if (pBegin != str)
                        *str++ = ',';

                    str += ls_snprintf(str, pEnd - str, "%s:[%s]", iter->ifi_name, temp);
                }
            }
            else
            {
                if (pBegin != str)
                    *str++ = ',';

                str += ls_snprintf(str, pEnd - str, "%s:%s", iter->ifi_name, temp);
            }
        }
    }

    if (pHead)
        NICDetect::free_ifi_info(pHead);

    return 0;
}


LocalWorker *HttpServerImpl::createAdminPhpApp(const char *pChroot,
        int iChrootLen,
        const char *pURI, char *pchPHPBin, size_t szPhpBin)
{
    LocalWorker *pFcgiApp = (LocalWorker *) ExtAppRegistry::addApp(
                                EA_LSAPI, DEFAULT_ADMIN_FCGI_NAME);
    assert(pFcgiApp);
    pFcgiApp->setURL(pURI);
    lstrncat(pchPHPBin, " -c ../conf/php.ini", szPhpBin);
    pFcgiApp->getConfig().setAppPath(&pchPHPBin[iChrootLen]);
    pFcgiApp->getConfig().setBackLog(100);
    pFcgiApp->getConfig().setSelfManaged(0);
    pFcgiApp->getConfig().setStartByServer(1);
    pFcgiApp->setMaxConns(4);
    pFcgiApp->getConfig().setKeepAliveTimeout(30);
    pFcgiApp->getConfig().setInstances(4);
    pFcgiApp->getConfig().clearEnv();
    pFcgiApp->getConfig().addEnv("PHP_FCGI_MAX_REQUESTS=1000");
    /**
     * Disable the below code for this env will be added later
     */
//     snprintf(pchPHPBin, MAX_PATH_LEN,
//              "LSWS_EDITION=LiteSpeed Web Server/%s/%s",
//              "Open", PACKAGE_VERSION);
//     pFcgiApp->getConfig().addEnv(pchPHPBin);

    snprintf(pchPHPBin, MAX_PATH_LEN, "LSWS_STATDIR=%s", sStatDir);
    pFcgiApp->getConfig().addEnv(pchPHPBin);

    RLimits limits;
    limits.setDataLimit(500 * 1024 * 1024, 500 * 1024 * 1024);
    limits.setProcLimit(1000, 1000);
    pFcgiApp->getConfig().setRLimits(&limits);

    char pEnv[8192];

    snprintf(pEnv, 2048, "LS_SERVER_ROOT=%s",
             MainServerConfig::getInstance().getServerRoot());
    pFcgiApp->getConfig().addEnv(pEnv);

    if (pChroot)
    {
        snprintf(pEnv, 2048, "LS_CHROOT=%s", pChroot);
        pFcgiApp->getConfig().addEnv(pEnv);
    }

    snprintf(pEnv, 2048, "LS_PRODUCT=ows");
    pFcgiApp->getConfig().addEnv(pEnv);

    snprintf(pEnv, 2048, "LS_PLATFORM=%s", LS_PLATFORM);
    pFcgiApp->getConfig().addEnv(pEnv);

    snprintf(pEnv, 2048, "LSWS_CHILDREN=%d",
             HttpServerConfig::getInstance().getChildren());
    pFcgiApp->getConfig().addEnv(pEnv);

    if (HttpServerConfig::getInstance().getAdminSock() != NULL)
    {
        snprintf(pEnv, 2048, "LSWS_ADMIN_SOCK=%s",
                 HttpServerConfig::getInstance().getAdminSock());
        pFcgiApp->getConfig().addEnv(pEnv);
    }

    lstrncpy(pEnv, "LSWS_IPV4_ADDRS=", sizeof(pEnv));

    if (detectIP(AF_INET, pEnv + strlen(pEnv), &pEnv[8192]) == 0)
        pFcgiApp->getConfig().addEnv(pEnv);

    lstrncpy(pEnv, "LSWS_IPV6_ADDRS=", sizeof(pEnv));

    if (detectIP(AF_INET6, pEnv + strlen(pEnv), &pEnv[8192]) == 0)
        pFcgiApp->getConfig().addEnv(pEnv);

    pFcgiApp->getConfig().addEnv("PATH=/bin:/usr/bin:/usr/local/bin");
    pFcgiApp->getConfig().addEnv(NULL);
    return pFcgiApp;
}


HttpVHost *HttpServerImpl::createAdminVhost(LocalWorker *pFcgiApp,
        int iChrootLen,
        char *pchPHPBin,
        size_t phpBinLen)
{
    const char *pAdminSock;
    char achRootPath[MAX_PATH_LEN];
    HttpVHost *pVHostAdmin = new HttpVHost(DEFAULT_ADMIN_SERVER_NAME);

    if (!pVHostAdmin)
    {
        ERR_NO_MEM("new HttpVHost()");
        return NULL;
    }

    if (addVHost(pVHostAdmin) != 0)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "failed to add admin virtual host!");
        delete pVHostAdmin;
        return NULL;
    }

    pFcgiApp->getConfig().setVHost(pVHostAdmin);

    snprintf(pchPHPBin, phpBinLen, "%s%s",
             ConfigCtx::getCurConfigCtx()->getVhRoot(), "html/");
    pVHostAdmin->setDocRoot(pchPHPBin);
    ConfigCtx::getCurConfigCtx()->setDocRoot(pchPHPBin);
    pVHostAdmin->addContext("/", HandlerType::HT_NULL, pchPHPBin, NULL, 1);

    ConfigCtx::getCurConfigCtx()->getAbsoluteFile(achRootPath,
            "$SERVER_ROOT/docs/");
    HttpContext *pDocs = pVHostAdmin->addContext("/docs/",
                         HandlerType::HT_NULL, &achRootPath[iChrootLen], NULL, 1);
//     pVHostAdmin->addContext(pDocs);
    pDocs = &pVHostAdmin->getRootContext();
    pDocs->addDirIndexes("index.html, index.php");
    char achPHPSuffix[10] = "php";
    setPHPHandler(pDocs, pFcgiApp, achPHPSuffix);

    char achMIME[] = "text/html";
    lstrncpy(achPHPSuffix, "html", sizeof(achPHPSuffix));
    pDocs->getMIME()->addMimeHandler(achPHPSuffix, achMIME,
                                     HandlerFactory::getInstance(HandlerType::HT_NULL, NULL), NULL,
                                     TmpLogId::getLogId());


    pVHostAdmin->enableScript(1);
    pVHostAdmin->followSymLink(2);
    pVHostAdmin->restrained(1);
    pVHostAdmin->getExpires().enable(1);
    pVHostAdmin->contextInherit();
    ConfigCtx::getCurConfigCtx()->getAbsoluteFile(achRootPath,
            "$SERVER_ROOT/conf/");
    pVHostAdmin->setUidMode(UID_DOCROOT);
    pVHostAdmin->updateUGid(TmpLogId::getLogId(),
                            &achRootPath[iChrootLen]);

    pAdminSock = HttpServerConfig::getInstance().getAdminSock();
    if (pAdminSock != NULL)
        chown(&pAdminSock[5], pVHostAdmin->getUid(), pVHostAdmin->getGid());
    const char *pUserFile = "$SERVER_ROOT/admin/conf/htpasswd";

    if (ConfigCtx::getCurConfigCtx()->getValidFile(pchPHPBin, pUserFile,
            "user DB") == 0)
    {
        UserDir *pUserDir =
            pVHostAdmin->getFileUserDir(ADMIN_USERDB, pchPHPBin,
                                        NULL);

        if (!pUserDir)
            LS_ERROR(ConfigCtx::getCurConfigCtx(),
                     "Failed to create authentication DB.");
    }
    return pVHostAdmin;
}


void HttpServerImpl::setAdminThrottleLimits(HttpVHost *pVHostAdmin)
{
    ThrottleLimits *pTC = pVHostAdmin->getThrottleLimits();
#ifdef DEV_DEBUG
    pTC->setDynReqLimit(100);
    pTC->setStaticReqLimit(3000);
#else
    pTC->setDynReqLimit(2);
    pTC->setStaticReqLimit(30);
#endif

    if (ThrottleControl::getDefault()->getOutputLimit() != INT_MAX)
    {
        pTC->setOutputLimit(40960000);
        pTC->setInputLimit(204800);
    }
    else
    {
        pTC->setOutputLimit(INT_MAX);
        pTC->setInputLimit(INT_MAX);
    }
}


int HttpServerImpl::enableWebConsole()
{
    char theWebConsolePathe[MAX_PATH_LEN];
    if ((ConfigCtx::getCurConfigCtx()->getAbsoluteFile(theWebConsolePathe,
            "$SERVER_ROOT/conf/disablewebconsole") == 0) &&
        (access(theWebConsolePathe, F_OK) == 0))
        return 0;
    return 1;
}


void HttpServerImpl::mapDomainList(const XmlNode *pListenerNodes,
                                   HttpVHost *pVHost)
{
    const XmlNodeList *pList = pListenerNodes->getChildren("listener");
    if (pList)
    {
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
        {
            const XmlNode *pListenerNode = *iter;
            const char *pName = pListenerNode->getChildValue("name", 1);
            HttpListener *pListener = getListener(pName);

            if (pListener)
            {
                pListener->setBinding(1);
                pListener->getVHostMap()->clear();
                pListener->mapDomainList(pVHost, "*");
            }
        }
    }
}


void HttpServerImpl::setMaxConns(int32_t conns)
{
#if DEFAULT_MAX_CONNS != INT_MAX
    if (conns > DEFAULT_MAX_CONNS)
        conns = DEFAULT_MAX_CONNS;
#endif
    ConnLimitCtrl::getInstance().setMaxConns(conns);
}


void HttpServerImpl::setMaxSSLConns(int32_t conns)
{
#if DEFAULT_MAX_SSL_CONNS != INT_MAX
    if (conns > DEFAULT_MAX_SSL_CONNS)
        conns = DEFAULT_MAX_SSL_CONNS;
#endif
    ConnLimitCtrl::getInstance().setMaxSSLConns(conns);
}


int HttpServerImpl::configSysShmDirs(char *pConfDir)
{
    const char *pAppSuffix = "ols";
    const char *pRamdisk, *pBackup2 = "$SERVER_ROOT/admin/tmp";
    char achDir[MAX_PATH_LEN];


    if ((pConfDir != NULL)
        && ((LsShm::checkDirSpace(pConfDir) != LSSHM_OK)
            || (LsShm::addBaseDir(pConfDir) != LSSHM_OK)))
        LS_ERROR("Add configured default directory failed! '%s'", pConfDir);
    pRamdisk = LsShm::detectDefaultRamdisk();
    if (pRamdisk != NULL)
    {
        snprintf(achDir, MAX_PATH_LEN, "%s/%s/", pRamdisk, pAppSuffix);
        if (GPath::createMissingPath(achDir, 0750) != 0)
            LS_ERROR("Create default directory failed! '%s'", achDir);
        else if ((LsShm::checkDirSpace(achDir) != LSSHM_OK)
                 || (LsShm::addBaseDir(achDir) != LSSHM_OK))
            LS_ERROR("Add default directory failed!  '%s'", achDir);
    }

    snprintf(achDir, MAX_PATH_LEN, "/tmp/%s/shm/", pAppSuffix);
    if (LsShm::addBaseDir(achDir) != LSSHM_OK)
        LS_ERROR("Add backup directory 1 failed! '%s'", achDir);

    if (ConfigCtx::getCurConfigCtx()->getAbsolutePath(achDir, pBackup2) != 0)
        LS_DBG_L("Load backup dir failed");

    if (LsShm::addBaseDir(achDir) != LSSHM_OK)
        LS_DBG_L("Add backup directory 2 failed! '%s'", achDir);

    return LsShm::getBaseDirCount() != 0;
}


int HttpServerImpl::configTuning(const XmlNode *pRoot)
{
    ConfigCtx currentCtx("server", "tuning");
    const XmlNode *pNode = pRoot->getChild("tuning");

    if (pNode == NULL)
    {
        LS_NOTICE(&currentCtx, "no tuning set up!");
        return LS_FAIL;
    }

    //connections
    setMaxConns(currentCtx.getLongValue(pNode, "maxConnections", 1, 1000000,
                                        2000));
    setMaxSSLConns(currentCtx.getLongValue(pNode, "maxSslConnections", 0,
                                           1000000, 1000));
    HttpListener::setSockSendBufSize(
        currentCtx.getLongValue(pNode, "sndBufSize", 0, 512 * 1024, 0));
    HttpListener::setSockRecvBufSize(
        currentCtx.getLongValue(pNode, "rcvBufSize", 0, 512 * 1024, 0));
    HttpServerConfig &config = HttpServerConfig::getInstance();
    config.setKeepAliveTimeout(
        currentCtx.getLongValue(pNode, "keepAliveTimeout", 1, 10000, 15));
    config.setConnTimeOut(currentCtx.getLongValue(pNode, "connTimeout", 1,
                          10000, 30));
    config.setMaxKeepAliveRequests(
        currentCtx.getLongValue(pNode, "maxKeepAliveReq", 0, 32767, 100));

    //HTTP request/response
    config.setMaxURLLen(currentCtx.getLongValue(pNode, "maxReqURLLen", 100,
                        MAX_URL_LEN , DEFAULT_URL_LEN));
    config.setMaxHeaderBufLen(currentCtx.getLongValue(pNode,
                              "maxReqHeaderSize",
                              1024, MAX_REQ_HEADER_BUF_LEN, DEFAULT_REQ_HEADER_BUF_LEN));
    config.setMaxReqBodyLen(currentCtx.getLongValue(pNode, "maxReqBodySize",
                            4096, MAX_REQ_BODY_LEN, DEFAULT_REQ_BODY_LEN));
    config.setMaxDynRespLen(currentCtx.getLongValue(pNode, "maxDynRespSize",
                            4096,
                            MAX_DYN_RESP_LEN, DEFAULT_DYN_RESP_LEN));
    config.setMaxDynRespHeaderLen(currentCtx.getLongValue(pNode,
                                  "maxDynRespHeaderSize",
                                  200, MAX_DYN_RESP_HEADER_LEN, DEFAULT_DYN_RESP_HEADER_LEN));
    FileCacheDataEx::setTotalInMemCacheSize(currentCtx.getLongValue(pNode,
                                            "totalInMemCacheSize",
                                            0, LONG_MAX, DEFAULT_TOTAL_INMEM_CACHE));
    FileCacheDataEx::setTotalMMapCacheSize(currentCtx.getLongValue(pNode,
                                           "totalMMapCacheSize",
                                           0, LONG_MAX, DEFAULT_TOTAL_MMAP_CACHE));
    FileCacheDataEx::setMaxInMemCacheSize(currentCtx.getLongValue(pNode,
                                          "maxCachedFileSize",
                                          0, 16384, 4096));
    FileCacheDataEx::setMaxMMapCacheSize(currentCtx.getLongValue(pNode,
                                         "maxMMapFileSize",
                                         0, LONG_MAX, 256 * 1024));
    int etag = currentCtx.getLongValue(pNode, "fileETag", 0, 4 + 8 + 16,
                                       4 + 8 + 16);
    HttpServer::getInstance().getServerContext().setFileEtag(etag);

    int val = currentCtx.getLongValue(pNode, "useSendfile", 0, 2, 0);
#if defined(LS_AIO_USE_AIO) && defined(LS_AIO_USE_KQ)
    if (val == 2 && !SigEventDispatcher::aiokoIsLoaded())
        val = 0;
#endif
    config.setUseSendfile(val);

//     if (val)
//         FileCacheDataEx::setMaxMMapCacheSize(0);

    const char *pValue = pNode->getChildValue("SSLCryptoDevice");

    if (SslEngine::init(pValue) == -1)
    {
        LS_WARN(&currentCtx, "Failed to initialize SSL Accelerator Device: %s,"
                " SSL hardware acceleration is disabled!", pValue);
    }
    SslContext::setUseStrongDH(currentCtx.getLongValue(pNode, "SSLStrongDhKey",
                               0, 1, 1));

    // GZIP compression
    config.setGzipCompress(currentCtx.getLongValue(pNode, "enableGzipCompress",
                           0, 1, 1));
    config.setDynGzipCompress(currentCtx.getLongValue(pNode,
                              "enableDynGzipCompress",
                              0, 1, 0));
    config.setCompressLevel(currentCtx.getLongValue(pNode, "gzipCompressLevel",
                            1, 9, 4));
    config.setBrCompress(
#ifdef USE_BROTLI
        currentCtx.getLongValue(pNode, "enableBrCompress", 0, 6, 4)
#else
        0
#endif
    );
    pValue = pNode->getChildValue("compressibleTypes");
    if (pValue == NULL)
        pValue = "default";
    HttpMime::getMime()->setCompressibleByType(
        pValue, NULL, TmpLogId::getLogId());
    StaticFileCacheData::setUpdateStaticGzipFile(
        currentCtx.getLongValue(pNode, "gzipAutoUpdateStatic", 0, 1, 0),
        currentCtx.getLongValue(pNode, "gzipStaticCompressLevel", 1, 9, 6),
        currentCtx.getLongValue(pNode, "gzipMinFileSize", 200, LONG_MAX, 300),
        currentCtx.getLongValue(pNode, "gzipMaxFileSize", 200, LONG_MAX,
                                1024 * 1024)
    );
    StaticFileCacheData::setStaticBrOptions(
        currentCtx.getLongValue(pNode, "brStaticCompressLevel", 1, 11, 6)
    );

    char achBuf[MAX_PATH_LEN];
    pValue = pNode->getChildValue("gzipCacheDir");

    if (!pValue)
        pValue = HttpServer::getInstance().getSwapDir();
    else
    {
        if (currentCtx.getAbsolutePath(achBuf, pValue) == -1)
        {
            LS_WARN(&currentCtx, "path of gzip cache is invalid, use default.");
            pValue = getSwapDir();
        }
        else
        {
            if (GPath::createMissingPath(achBuf, 0700) == -1)
            {
                LS_ERROR(&currentCtx, "Failed to create directory: %s .", achBuf);
                pValue = getSwapDir();
            }
            else
            {
                chown(achBuf,
                      ServerProcessConfig::getInstance().getUid(),
                      ServerProcessConfig::getInstance().getGid());
                pValue = achBuf;
                pValue += MainServerConfig::getInstance().getChrootlen();
            }
        }
    }

    StaticFileCacheData::setCompressCachePath(pValue);


    // shm
    const char *pShmDir = pNode->getChildValue("shmDefaultDir");
    char achShmDefDir[MAX_PATH_LEN];
    char *pConfDir = NULL;
    if (pShmDir != NULL)
    {
        if (currentCtx.getValidFile(achShmDefDir, pShmDir, "Shm Default Dir") == 0)
            pConfDir = achShmDefDir;
    }

    if (configSysShmDirs(pConfDir) == 0)
    {
        LS_ERROR("Failed to init any system shm directories.");
        return -1;
    }

    int v = currentCtx.getLongValue(pNode, "sslEnableMultiCerts", 0, 1, 0);
    config.setEnableMultiCerts(v);

    int iSslCacheSize;
    int32_t iSslCacheTimeout;
    if (currentCtx.getLongValue(pNode, "sslSessionCache", 0, 1, 1) != 0)
    {
        iSslCacheSize = currentCtx.getLongValue(pNode, "sslSessionCacheSize",
                                                0, INT_MAX, 1000000);

        iSslCacheTimeout = currentCtx.getLongValue(pNode,
                           "sslSessionCacheTimeout",
                           0, INT_MAX, 216000);
        if (SslSessCache::getInstance().init(iSslCacheTimeout,
            iSslCacheSize, getuid(), getgid()) != LS_OK)
        {
            LS_WARN("Failed to init SSL Session Id Cache");
            return -1;
        }
    }

    const char *pTKFile;
    char achTKFile[MAX_PATH_LEN];
    if (currentCtx.getLongValue(pNode, "sslSessionTickets", 0, 1, 1) == 1)
    {
        if ((pTKFile = pNode->getChildValue("sslSessionTicketKeyFile")) != NULL)
        {
            if (currentCtx.getValidFile(achTKFile, pTKFile,
                                        "Ticket Key File") == 0)
                pTKFile = achTKFile;
        }
        long iTicketLifetime = currentCtx.getLongValue(pNode,
                               "sslSessionTicketLifetime", 216000, INT_MAX, 216000);
        SslTicket::init(pTKFile, iTicketLifetime, getuid(), getgid());
    }

    const char *pCAFile;
    char achCAFile[MAX_PATH_LEN];
    const char *pCAPath;
    char achCAPath[MAX_PATH_LEN];
    if ((pCAFile = pNode->getChildValue("sslDefaultCAFile")) != NULL)
    {
        if (currentCtx.getValidFile(achCAFile, pCAFile, "Default CA File") == 0)
            pCAFile = achCAFile;
    }
    if ((pCAPath = pNode->getChildValue("sslDefaultCAPath")) != NULL)
    {
        if (currentCtx.getValidFile(achCAPath, pCAPath, "Default CA Path") == 0)
            pCAPath = achCAPath;
    }
    SslUtil::initDefaultCA(pCAFile, pCAPath);

    return 0;
}


int HttpServerImpl::configAccessDeniedDir(const XmlNode *pNode)
{
    int add = 0;
    DeniedDir *pDeniedDir = HttpServerConfig::getInstance().getDeniedDir();
    pDeniedDir->clear();
    const XmlNodeList *pList = pNode->getChildren("dir");

    if (pList)
    {
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
        {
            const XmlNode *pDir = *iter;

            if (pDir->getValue())
                if (pDeniedDir->addDir(pDir->getValue()) == 0)
                    add ++;
        }
    }

    return (add > 0);
}


int HttpServerImpl::configSecurity(const XmlNode *pRoot)
{
    const XmlNode *pNode = pRoot->getChild("security", 1);
    {
        ConfigCtx currentCtx("server", "security");

        //const XmlNode* pNode = pRoot->getChild("security");
        if (pNode == NULL)
        {
            LS_NOTICE(&currentCtx, "no <security> section at server level.");
            return 1;
        }

        const XmlNode *pNode1 = pNode->getChild("accessDenyDir");

        if (pNode1 != NULL)
            configAccessDeniedDir(pNode1);

        HttpServerConfig &config = HttpServerConfig::getInstance();
        pNode1 = pNode->getChild("fileAccessControl");

        config.setFollowSymLink(
            currentCtx.getLongValue(pNode1, "followSymbolLink", 0, 2, 1));
        config.checkDeniedSymLink(currentCtx.getLongValue(pNode1,
                                  "checkSymbolLink", 0, 1, 0));
        config.setRequiredBits(
            currentCtx.getLongValue(pNode1, "requiredPermissionMask", 0, 0177777, 004,
                                    8));
        config.setForbiddenBits(
            currentCtx.getLongValue(pNode1, "restrictedPermissionMask", 0, 0177777,
                                    041111, 8));

        config.setScriptForbiddenBits(
            currentCtx.getLongValue(pNode1, "restrictedScriptPermissionMask", 0,
                                    0177777, 000, 8));
        config.setDirForbiddenBits(
            currentCtx.getLongValue(pNode1, "restrictedDirPermissionMask", 0, 0177777,
                                    000, 8));

        pNode1 = pNode->getChild("perClientConnLimit");

        if (pNode1)
        {
            ThrottleControl::getDefault()->config(pNode1,
                                                  ThrottleControl::getDefault(), &currentCtx);
            NtwkIOLink::enableThrottle((ThrottleControl::getDefault()->getOutputLimit()
                                        != INT_MAX));
            ClientCache::getClientCache()->resetThrottleLimit();
            ClientInfo::setPerClientSoftLimit(currentCtx.getLongValue(pNode1,
                                              "softLimit", 1, INT_MAX,
                                              INT_MAX));
            ClientInfo::setPerClientHardLimit(currentCtx.getLongValue(pNode1,
                                              "hardLimit", 1, INT_MAX,
                                              INT_MAX));
            ClientInfo::setOverLimitGracePeriod(currentCtx.getLongValue(pNode1,
                                                "gracePeriod", 1, 3600,
                                                10));
            ClientInfo::setBanPeriod(currentCtx.getLongValue(pNode1,
                                     "banPeriod", 1, INT_MAX, 60));
        }

        // CGI
        CgidWorker *pWorker = (CgidWorker *) ExtAppRegistry::addApp(
                                  EA_CGID, LSCGID_NAME);
        pNode1 = pNode->getChild("CGIRLimit");

        if (pNode1)
        {
            if (pWorker)
                pWorker->config(pNode1);
        }
    }
    {

        ConfigCtx currentCtx("server", "security:accessControl");

        if (AccessControl::isAvailable(pNode))
        {
            currentCtx.configSecurity(&m_accessCtrl, pNode);
            AccessControl::setAccessCtrl(&m_accessCtrl);
        }
        else
            AccessControl::setAccessCtrl(NULL);
    }
    return 0;
}


int HttpServerImpl::configMime(const XmlNode *pRoot)
{
    const char *pValue = ConfigCtx::getCurConfigCtx()->getTag(pRoot, "mime");

    if (pValue != NULL)
    {
        char achBuf[MAX_PATH_LEN];

        if (ConfigCtx::getCurConfigCtx()->getValidFile(achBuf, pValue,
                "MIME config") != 0)
            return LS_FAIL;

        if (HttpMime::getMime()->loadMime(achBuf) == 0)
        {
            //Check in the mime file
            plainconf::checkInFile(achBuf);
            return 0;
        }

        if (HttpMime::getMime()->getDefault() == 0)
            HttpMime::getMime()->initDefault();
    }

    return LS_FAIL;
}


int HttpServerImpl::denyAccessFiles(HttpVHost *pVHost, const char *pFile,
                                    int regex)
{
    HttpContext *pContext = new HttpContext();

    if (pContext)
    {
        pContext->setFilesMatch(pFile, regex);
        pContext->addAccessRule("*", 0);

        if (pVHost)
            pVHost->getRootContext().addFilesMatchContext(pContext);
        else
            m_serverContext.addFilesMatchContext(pContext);

        return 0;
    }

    return LS_FAIL;
}


static const char *getAutoIndexURI(const XmlNode *pNode)
{
    const char *pURI = pNode->getChildValue("autoIndexURI");

    if (pURI)
    {
        if (*pURI != '/')
        {
            LS_ERROR(ConfigCtx::getCurConfigCtx(),
                     "Invalid AutoIndexURI, must be started with a '/'");
            return NULL;
        }
    }

    return pURI;
}

void HttpServerImpl::testAndFixDirs(const char *pSuffix, uid_t uid, gid_t gid, int mod)
{
    char  achBuf[4096];
    ls_snprintf(achBuf, 4096, "%s/%s/",
                MainServerConfig::getInstance().getServerRoot(), pSuffix);

    bool rootuser = (getuid() == 0);
    struct stat sb;
    int iStat = stat(achBuf, &sb);
    if (iStat == -1)
    {
        if (GPath::createMissingPath(achBuf, (rootuser ? mod : 0777)) == 0)
        {
            iStat = stat(achBuf, &sb);
            LS_NOTICE("[testAndFixDirs] \"%s\" not exist, created stat %d.",
                       achBuf, iStat);
        }
    }

    if (iStat != -1 && rootuser)
    {
        if (sb.st_uid != uid || sb.st_gid != gid)
        {
            LS_NOTICE("[testAndFixDirs] \"%s\" exist and change own to %d:%d.",
                    achBuf, uid, gid);
            chown(achBuf, uid, gid);
        }

        if ((sb.st_mode & 0777) != mod )
        {
            LS_NOTICE("[testAndFixDirs] \"%s\" exist and changed mod to %04o.",
                    achBuf, mod);
            chmod(achBuf, mod);
        }
    }
}


int HttpServerImpl::configServerBasic2(const XmlNode *pRoot,
                                       const char *pSwapDir)
{
    while (1)
    {
        const XmlNode *pNode;

        ConfigCtx currentCtx("server", "basics2");

        long inMemBufSize = ConfigCtx::getCurConfigCtx()->getLongValue(pRoot,
                            "inMemBufSize", 0,
                            LONG_MAX, 20 * 1024 * 1024);
        VMemBuf::setMaxAnonMapSize(inMemBufSize);
        //const char *pValue = m_pRoot->getChildValue( "swappingDir" );

        if (pSwapDir)
            setSwapDir(pSwapDir);

        char  achBuf[4096];
        ls_snprintf(achBuf, 4096, "%s/tmp/ocspcache/",
                    MainServerConfig::getInstance().getServerRoot());
        SslOcspStapling::setCachePath(achBuf);

        m_serverContext.configAutoIndex(pRoot);
        m_serverContext.configDirIndex(pRoot);
        const char *pURI = getAutoIndexURI(pRoot);

        if (pURI)
            MainServerConfig::getInstance().setAutoIndexURI(pURI);

        int sv = ConfigCtx::getCurConfigCtx()->getLongValue(pRoot,
                 "showVersionNumber", 0, 2, 0);
        HttpRespHeaders::hideServerSignature(sv);

        if (!sv)
        {
            LS_INFO(&currentCtx,
                    "For better obscurity, server version number is hidden"
                    " in the response header.");
        }

        HttpServerConfig::getInstance().setUseProxyHeader(
            ConfigCtx::getCurConfigCtx()->getLongValue(pRoot,
                    "useIpInProxyHeader", 0, 3, 0));

        denyAccessFiles(NULL, ".ht*", 0);


        if (configMime(pRoot) != 0)
        {
            LS_ERROR(&currentCtx, "failed to load mime configure");
            break;
        }

        pNode = pRoot->getChild("expires");

        if (pNode)
        {
            m_serverContext.getExpires().config(pNode,
                                                NULL, &m_serverContext);

            HttpMime::getMime()->setExpiresByType(
                pNode->getChildValue("expiresByType"), NULL, TmpLogId::getLogId());
        }

        configSecurity(pRoot);

        pNode = pRoot->getChild("lsrecaptcha");
        if (pNode) {
            initLsrecaptcha(pNode);
        }

        m_serverContext.setModuleConfig(ModuleManager::getInstance().getGlobalModuleConfig(), 0);
        m_serverContext.initExternalSessionHooks();
        return 0;
    }

    return LS_FAIL;

}


int HttpServerImpl::configMultiplexer(const XmlNode *pNode)
{
    const char *pType = NULL;
    //const XmlNode *pNode = m_pRoot->getChild( "tuning" );

    if (pNode)
        pType = pNode->getChildValue("eventDispatcher");

    if (m_dispatcher.init(pType) == -1)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "Failed to initialize I/O event dispatcher type: %s, error: %s",
                 pType, strerror(errno));

        if (pType && (strcasecmp(pType, "poll") != 0))
        {
            LS_NOTICE(ConfigCtx::getCurConfigCtx(),
                      "Fall back to I/O event dispatcher type: poll");
            return m_dispatcher.init("poll");
        }

        return LS_FAIL;
    }

    return 0;
}


void HttpServerImpl::configVHTemplateToListenerMap(
    const XmlNodeList *pList, TPointerList<HttpListener> &listeners,
    XmlNode *pVhConfNode, XmlNode *pTmpConfNode, const char *pTemplateName)
{
    XmlNodeList::const_iterator iter;

    for (iter = pList->begin(); iter != pList->end(); ++iter)
    {
        const char *pName = ConfigCtx::getCurConfigCtx()->getTag((*iter),
                            "name", 1);
        const char *pDomain = (*iter)->getChildValue("vhDomain");
        const char *pAliases = (*iter)->getChildValue("vhAliases");
        const char *pVhRoot = (*iter)->getChildValue("vhRoot");

        if (!pName)
            continue;

        if (!pDomain)
            pDomain = pName;

        if (getVHost(pName))
        {
            ConfigCtx currentCtx(pTemplateName);
            LS_INFO(&currentCtx,
                    "Virtual host %s already exists, skip template configuration.",
                    pName);
            continue;
        }
        ConfigCtx::getCurConfigCtx()->setVhName(pName);

        ConfigCtx currentCtx(pName);

        HttpVHost *pVHost = HttpVHost::configVHost(pTmpConfNode, pName,
                            pDomain, pAliases, pVhRoot, pVhConfNode);

        if (!pVHost)
            continue;

        if (addVHost(pVHost))
        {
            delete pVHost;
            continue;
        }

        if (listeners.size() > 0)
        {
            TPointerList<HttpListener>::iterator iter;

            for (iter = listeners.begin(); iter != listeners.end(); ++iter)
            {
                mapListenerToVHost((*iter), pVHost, pDomain);

                if (pAliases)
                    mapListenerToVHost((*iter), pVHost, pAliases);

            }

        }

    }
}


int HttpServerImpl::configVHTemplate(const XmlNode *pNode)
{
    XmlNode *pVhConfNode;
    XmlNode *pTmpConfNode;
    TPointerList<HttpListener> listeners;
    const char *pTemplateName = ConfigCtx::getCurConfigCtx()->getTag(pNode,
                                "name", 1);
    {
        if (!pTemplateName)
            return LS_FAIL;

        ConfigCtx currentCtx(pTemplateName);

        const char *pConfFile = ConfigCtx::getCurConfigCtx()->getTag(pNode,
                                "templateFile");

        if (!pConfFile)
            return LS_FAIL;

        char achTmpConf[MAX_PATH_LEN];

        if (ConfigCtx::getCurConfigCtx()->getValidFile(achTmpConf, pConfFile,
                "vhost template config") != 0)
            return LS_FAIL;

        pTmpConfNode = plainconf::parseFile(achTmpConf, "virtualHostTemplate");

        if (pTmpConfNode == NULL)
        {
            LS_ERROR(&currentCtx, "cannot load configure file - %s !", achTmpConf);
            return LS_FAIL;
        }

        pVhConfNode = pTmpConfNode->getChild("virtualHostConfig");

        if (!pVhConfNode)
        {
            LS_ERROR(&currentCtx, "missing <virtualHostConfig> tag in the template");
            delete pTmpConfNode;
            return LS_FAIL;
        }

        const char *pListeners = pNode->getChildValue("listeners");

        if (pListeners)
        {
            StringList  listenerNames;
            listenerNames.split(pListeners, strlen(pListeners) + pListeners, ",");
            StringList::iterator iter;

            for (iter = listenerNames.begin(); iter != listenerNames.end(); ++iter)
            {
                HttpListener *p = getListener((*iter)->c_str());

                if (!p)
                    LS_ERROR(&currentCtx, "Listener [%s] does not exist", (*iter)->c_str());
                else
                    listeners.push_back(p);
            }
        }
    }
    const XmlNodeList *pList = pNode->getChildren("member");

    if (pList)
        configVHTemplateToListenerMap(pList, listeners, pVhConfNode, pTmpConfNode,
                                      pTemplateName);

    delete pTmpConfNode;
    return 0;
}


int HttpServerImpl::configVHTemplates(const XmlNode *pRoot)
{
    ConfigCtx currentCtx("template");
    const XmlNode *pNode = pRoot->getChild("vhTemplateList", 1);

    const XmlNodeList *pList = pNode->getChildren("vhTemplate");

    if (pList)
    {
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
            configVHTemplate(*iter);
    }

    return 0;
}


int HttpServerImpl::configVHosts(const XmlNode *pRoot)
{
    ConfigCtx currentCtx("server", "vhosts");
    const XmlNode *pNode = pRoot->getChild("virtualHostList", 1);

    const XmlNodeList *pList = pNode->getChildren("virtualHost");

    if (pList)
    {
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
        {
            const XmlNode *pVhostNode = *iter;
            //m_achVhRoot[0] = 0;
            ConfigCtx::getCurConfigCtx()->clearVhRoot();
            HttpVHost *pVHost = HttpVHost::configVHost(const_cast <XmlNode *>
                                (pVhostNode));

            if (pVHost)
            {
                if (addVHost(pVHost))
                    delete pVHost;
            }
        }
    }

    return 0;
}


void HttpServerImpl::chmodDirToAll(const char *path, struct stat &sb)
{
    if (!(sb.st_mode & S_IROTH) || !(sb.st_mode & S_IXOTH))
    {
        sb.st_mode |= (S_IROTH | S_IXOTH);
        if (chmod (path, sb.st_mode))
        {
            /**
             * Unusual, this eror should never happen, just print to screen.
             */
            printf("Failed to chmod %s to S_IROTH | S_IXOTH.\n", path);
        }
    }
}

void HttpServerImpl::verifyStatDir(const char *path)
{
    int l;
    if (path && (l = strlen(path)) > 0)
    {
        int error = 1;
        bool rootuser = (getuid() == 0);
        struct passwd *pw = getpwnam(MainServerConfig::getInstance().getUser());
        struct stat sb;
        if (stat(path, &sb) == -1)
        {
            AutoStr2 str = path;
            if (path[l -1 ] != '/')
                str.append("/", 1);

            int mod = (rootuser ? 0755 : 0777);
            if (GPath::createMissingPath((char *)str.c_str(), mod) == 0)
            {
                if (rootuser)
                    chown(str.c_str(), pw->pw_uid, pw->pw_gid);
                error =0;
            }
        }
        else
        {
            chmodDirToAll(path, sb);
            if (sb.st_uid != pw->pw_uid)
            {
                if (rootuser)
                {
                    chown(path, pw->pw_uid, pw->pw_gid);
                    error = 0;
                }
                else
                {
                    printf("ERROR: %s own by user/group other than '%s:%s'.\n",
                           path,
                           MainServerConfig::getInstance().getUser(),
                           MainServerConfig::getInstance().getGroup());
                }
            }
            else
                error =0;
        }

        if (!error)
            sStatDir = path;
    }
}

int HttpServerImpl::configServerBasics(int reconfig, const XmlNode *pRoot)
{
    struct passwd *pw;
    MainServerConfig  &MainServerConfigObj =  MainServerConfig::getInstance();
    ServerProcessConfig &procConf = ServerProcessConfig::getInstance();

    ConfigCtx currentCtx("server", "basics");

    while (1)
    {
#if defined(LS_AIO_USE_KQ)
        SigEventDispatcher::setAiokoLoaded();
#endif

        if (!MainServerConfigObj.getConfTestMode())
        {
            //if ( m_pServer->initErrorLog( pRoot, 1 ) )
            if (HttpServer::getInstance().initErrorLog(pRoot, 1))
                break;
        }
        else
        {
            HttpServer::getInstance().setErrorLogFile(TEST_CONF_LOG);
            HttpServer::getInstance().setLogLevel("WARN");
        }

        const char *pValue = pRoot->getChildValue("serverName");
        if (pValue != NULL)
            MainServerConfigObj.setServerName(pValue);
        else
        {
            //Since may run multiple instances, if empty here, use binary path for it
            pid_t pid = getpid();
            char bin_path[1024];
            ssize_t len;
            char sExe[128];
            sprintf(sExe, "/proc/%d/exe", pid);

            if ((len = readlink(sExe, bin_path, sizeof(bin_path)-1)) != -1)
            {
               bin_path[len] = '\0';
               MainServerConfigObj.setServerName(bin_path);
            }
            else
            {
                //MAC OS may go here
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX       64
#endif
                char hostname[HOST_NAME_MAX];
                if (gethostname(hostname, HOST_NAME_MAX)==0)
                    MainServerConfigObj.setServerName(hostname);
                else
                    MainServerConfigObj.setServerName("anonymous");
            }
        }


        if (!reconfig)
        {
            HttpServerConfig::getInstance().setAutoLoadHtaccess(
                currentCtx.getLongValue(pRoot, "autoLoadHtaccess", 0, 1, 0));

            const char *pUser = pRoot->getChildValue("user");
            const char *pGroup = pRoot->getChildValue("group");
            if (pGroup)
                MainServerConfigObj.setGroup(pGroup);
            if (pUser)
                MainServerConfigObj.setUser(pUser);
            gid_t gid = procConf.getGid();
            pw = Daemonize::configUserGroup(pUser, pGroup, gid);
            procConf.setGid(gid);
            if (!pw)
            {
                LS_ERROR(ConfigCtx::getCurConfigCtx(), "Invalid User Name(%s) "
                         "or Group Name(%s)!", pUser, pGroup);
                break;
            }

            procConf.setUid(pw->pw_uid);
            m_pri_gid = pw->pw_gid;

            if (getuid() == 0)
            {
                chown(HttpLog::getErrorLogFileName(),
                      procConf.getUid(), procConf.getGid());
                chown(HttpLog::getAccessLogFileName(),
                      procConf.getUid(), procConf.getGid());

                /**
                 * Fix /cgid/ DIR permission because user can change
                 * user/group setting and will cause permission error.
                 */
                AutoStr2 sDir = MainServerConfig::getInstance().getServerRoot();
                sDir.append("cgid/", 5);

                /**
                 * Some user may not have such DIR, Mkdir first
                 */
                struct stat sb;
                if (stat(sDir.c_str(), &sb) == -1)
                    mkdir(sDir.c_str(), 0710);
                chown(sDir.c_str(), procConf.getUid(), procConf.getGid());

                AutoStr2 sAutoUpd = MainServerConfig::getInstance().getServerRoot();
                sAutoUpd.append("autoupdate/", 11);
                if (stat(sAutoUpd.c_str(), &sb) == -1)
                    mkdir(sAutoUpd.c_str(), 0755);
                chown(sAutoUpd.c_str(), procConf.getUid(), procConf.getGid());
            }
        }

        const char *pAdminEmails = pRoot->getChildValue("adminEmails");

        if (!pAdminEmails)
            pAdminEmails = "";

        MainServerConfigObj.setAdminEmails(pAdminEmails);

        const char *sDisableWebAdmin = pRoot->getChildValue("disableWebAdmin");
        if (sDisableWebAdmin != NULL)
            MainServerConfigObj.setDisableWebAdmin(atoi(sDisableWebAdmin));

        procConf.setPriority(ConfigCtx::getCurConfigCtx()->getLongValue(pRoot,
                             "priority", -20, 20, 0));

#ifndef IS_LSCPD
        int iNumProc = PCUtil::getNumProcessors();
        iNumProc = (iNumProc > 8 ? 8 : iNumProc);
        HttpServerConfig::getInstance().setChildren(
            ConfigCtx::getCurConfigCtx()->getLongValue(pRoot,
                    "httpdWorkers", 1, 16, iNumProc));
        LS_NOTICE(ConfigCtx::getCurConfigCtx(), "httpdWorkers: %d, "
                "Num of Processors: %d",
                HttpServerConfig::getInstance().getChildren(),
                iNumProc);
#else
        HttpServerConfig::getInstance().setChildren(1);
#endif

        const char *pGDBPath = pRoot->getChildValue("gdbPath");

        if (pGDBPath)
            MainServerConfigObj.setGDBPath(pGDBPath);

        MainServerConfigObj.setDisableLogRotateAtStartup(
            ConfigCtx::getCurConfigCtx()->getLongValue(pRoot, "disableInitLogRotation",
                    0, 1, 0));

        HttpStats::set503AutoFix(ConfigCtx::getCurConfigCtx()->getLongValue(
                                     pRoot, "AutoFix503", 0, 1, 1));
        HttpServerConfig::getInstance().setEnableH2c(
            ConfigCtx::getCurConfigCtx()->getLongValue(pRoot, "enableh2c",
                    0, 1, 0));

        long l = ConfigCtx::getCurConfigCtx()->getLongValue(pRoot,
                 "gracefulRestartTimeout", -1, INT_MAX, 300);
        if (l == -1)
            l = 3600 * 24;
        HttpServerConfig::getInstance().setRestartTimeOut(l);

        HttpServerConfig::getInstance().setEnableLve(
            ConfigCtx::getCurConfigCtx()->getLongValue(pRoot, "enableLVE", 0,
                                                       3, 0));

        HttpServerConfig::getInstance().setCpuAffinity(
            ConfigCtx::getCurConfigCtx()->getLongValue(pRoot, "cpuAffinity", 0,
                                                       64, 0));

        //this value can only be set once when server start.
        if (MainServerConfigObj.getCrashGuard() == 2)
            MainServerConfigObj.setCrashGuard(1);

        pValue = pRoot->getChildValue("statDir");
        verifyStatDir(pValue);
        m_sRTReportFile = sStatDir;
        m_sRTReportFile.append("/.rtreport", 10);

        testAndFixDirs((char *)"cachedata", procConf.getUid(), procConf.getGid(), 0755);
        testAndFixDirs((char *)"autoupdate", procConf.getUid(), procConf.getGid(), 0755);
        testAndFixDirs((char *)"tmp", procConf.getUid(), procConf.getGid(), 0755);
        testAndFixDirs((char *)"tmp/ocspcache", procConf.getUid(), procConf.getGid(), 0700);

        //Fix Conf now
        pw = getpwnam("lsadm");
        if (!pw)
            LS_ERROR(ConfigCtx::getCurConfigCtx(), "Get lsadm passwd failed.");
        else
            testAndFixDirs((char *)"conf", pw->pw_uid, procConf.getGid(), 0750);
        
        return 0;
    }

    return LS_FAIL;
}


//Global level module config
int HttpServerImpl::configModules(const XmlNode *pRoot)
{
    const XmlNode *pNode = pRoot->getChild("modulelist", 1);
    if (ModuleManager::getInstance().initModule() != 0)
    {
        LS_DBG_L("ModuleManager initModule failed.");
        return LS_FAIL;
    }

    const XmlNodeList *pList = pNode->getChildren("module");
    int moduleCount = ModuleManager::getInstance().loadModules(pList);
    ModuleManager::getInstance().getGlobalModuleConfig()->init(moduleCount);
    //If global level is "not set", by default is enable, so set to 1 here, other level won't do that
    for (int i = 0; i < moduleCount; ++i)
        ModuleManager::getInstance().getGlobalModuleConfig()->get(i)->filters_enable = 1;
    ModuleConfig::parseConfigList(pList,
                                  ModuleManager::getInstance().getGlobalModuleConfig(), LSI_CFG_SERVER,
                                  pRoot->getName());
    ModuleManager::getInstance().runModuleInit();

    //all hooks are ready, init the RtHooks for ServerHooks
    LsiApiHooks::s_pServerSessionHooks->inherit(NULL, 1);
    ModuleManager::getInstance().applyConfigToServerRt(
        LsiApiHooks::s_pServerSessionHooks,
        ModuleManager::getInstance().getGlobalModuleConfig());

    LsiApiHooks::initModuleEnableHooks();
    return 0;
}


int HttpServerImpl::initGroups()
{
    char achBuf[256];

    if (Daemonize::initGroups(
            MainServerConfig::getInstance().getUser(),
            ServerProcessConfig::getInstance().getGid(),
            m_pri_gid, achBuf, 256))
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "%s", achBuf);
        return LS_FAIL;
    }

    return 0;
}


#define DEFAULT_ADMIN_CONFIG_FILE   "$VH_ROOT/conf/admin_config.conf"
#define ADMIN_CONFIG_NODE           "AdminConfigNode"
int HttpServerImpl::loadAdminConfig(XmlNode *pRoot)
{
    ConfigCtx currentCtx("admin");
    const char *pAdminRoot = "$SERVER_ROOT/admin";

    if (ConfigCtx::getCurConfigCtx()->getValidChrootPath(pAdminRoot,
            "admin vhost root") != 0)
    {
        LS_ERROR(&currentCtx, "The value of root directory to "
                 "admin server is invalid - %s!", pAdminRoot);
        return LS_FAIL;
    }

    char achConfFile[MAX_PATH_LEN];

    if (ConfigCtx::getCurConfigCtx()->getValidFile(achConfFile,
            DEFAULT_ADMIN_CONFIG_FILE,
            "configuration file for admin vhost") != 0)
    {
        LS_ERROR(&currentCtx, "missing configuration file for admin server: %s!",
                 achConfFile);
        return LS_FAIL;
    }

    XmlNode *pAdminConfNode = plainconf::parseFile(achConfFile, "adminConfig");

    if (pAdminConfNode == NULL)
    {
        LS_ERROR(&currentCtx, "cannot load configure file for admin server - %s !",
                 achConfFile);
        return LS_FAIL;
    }

    pRoot->addChild(ADMIN_CONFIG_NODE, pAdminConfNode);

    MainServerConfig::getInstance().setEnableCoreDump(
        ConfigCtx::getCurConfigCtx()->getLongValue(pAdminConfNode,
                "enableCoreDump", 0, 1, 0));
    return 0;
}


/**
 * detectMmdb: With XmlNodeList
 * "GeoIP DB": If the old keyword is generated with a mmdb file, let's
 *             offer this option to support it.  This function lets you know
 *             which object to instantiate (iptogeo2 or iptogeo).
 * returns 1 if new file, something else (0 or -1 if not).
 */
static int detectMmdb(const XmlNodeList *pList)
{
    XmlNodeList::const_iterator iter;

    LS_DBG("[GEO] configOldKeywordNewFile?\n");
    for (iter = pList->begin(); iter != pList->end(); ++iter)
    {
        XmlNode *p = *iter;
        const char *pFile = p->getValue();

        if (pFile && strstr(pFile, ".mmdb"))
        {
            LS_DBG("[GEO] detects MMDB file: %s\n", pFile);
            return 1;
        }
    }
    LS_DBG("[GEO] Did not find new file format\n");
    return 0;
}


int HttpServerImpl::configIpToGeo(const XmlNode *pNode)
{
    const XmlNodeList *pList = pNode->getChildren("geoipDB");
    Ip2Geo *pIp2Geo = NULL;

    if ((!pList) || (pList->size() == 0))
        return 0;

#ifdef ENABLE_IPTOGEO2
    if (detectMmdb(pList))
        pIp2Geo = new IpToGeo2();
#endif

    if (!pIp2Geo)
        return LS_FAIL;

    if (pIp2Geo->config(pList) == -1)
    {
        delete pIp2Geo;
        return LS_FAIL;
    }
    ClientCache::setIp2Geo(pIp2Geo);
    return 0;
}


int HttpServerImpl::configIpToLoc(const XmlNode *pNode)
{
    const XmlNode *p = pNode->getChild("ip2locDB");

    if (!p)
        return 0;

#ifdef USE_IP2LOCATION
    LS_DBG_H("Try to configure IP2Location.");
    IpToLoc *pIpToLoc = new IpToLoc();

    if (!pIpToLoc)
    {
        LS_NOTICE("Failed to create IpToLoc instance.");
        return LS_FAIL;
    }
    if (pIpToLoc->config(p) == LS_FAIL)
        delete pIpToLoc;
    else
    {
        LS_DBG_H("Successfully set up an IP2Location DB!");
        m_serverContext.setIpToLoc(1);
    }
#endif

    return 0;
}


#define DEFAULT_LSRECAPTCHA_SOCK    "uds:/" DEFAULT_TMP_DIR "/lsrecaptcha.sock"
int HttpServerImpl::configLsrecaptchaWorker(const XmlNode *pNode)
{
    const char *pName = "lsrecaptcha";
    int iAutoStart = 1;
    int backlog = 10;
    int instances = 1;
    int iMaxConns = 35;
    int iRetryTimeout = 15;
    int iInitTimeout = 60;
    int iBuffer = 0;
    int iKeepAlive = 1;
    int iKeepAliveTimeout = 60;

    const char *pAddr = DEFAULT_LSRECAPTCHA_SOCK;

    // FIXME: is this necessary?
    GSockAddr addr;
    if ( addr.set( pAddr, NO_ANY ) )
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "failed to set socket address %s!", pAddr);
        return -1;
    }

    const char *pPath = "$SERVER_ROOT/lsrecaptcha/_recaptcha";
    char achPath[MAX_PATH_LEN];
    if (ConfigCtx::getCurConfigCtx()->getAbsoluteFile(achPath, pPath) != 0) {
        LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Module path is invalid.");
        return -1;
    }
    pPath = achPath;

    int selfManaged = 1;

    ExtWorker * pWorker = ExtAppRegistry::addApp(EA_LSAPI, pName);
    if ( !pWorker )
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "failed to add external processor!");
        return -1;
    }

    ExtWorkerConfig * pConfig = pWorker->getConfigPointer();
    assert( pConfig );
    if ( pAddr )
        if ( pWorker->setURL( pAddr ) )
        {
            LS_ERROR(ConfigCtx::getCurConfigCtx(), "failed to set socket address to %s!",
                    pAddr);
            return -1;
        }

    pWorker->setRole( HandlerType::ROLE_RESPONDER );
    pConfig->setPersistConn( iKeepAlive );
    pConfig->setKeepAliveTimeout( iKeepAliveTimeout );
    pWorker->setMaxConns( iMaxConns );
    pConfig->setTimeout( iInitTimeout );
    pConfig->setRetryTimeout( iRetryTimeout );
    pConfig->setBuffering( iBuffer );

    pConfig->clearEnv();

    LocalWorker * pApp = dynamic_cast<LocalWorker *>( pWorker );
    if ( NULL == pApp )
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "Worker is not a localworker!. Fail configuration");
        return -1;
    }
    LocalWorkerConfig& config = pApp->getConfig();
    config.setBackLog( backlog );
    config.setAppPath( pPath );
    config.setInstances( instances );
    config.setPriority(ServerProcessConfig::getInstance().getPriority());
    config.setSelfManaged( selfManaged );
    if ( instances == 1 && selfManaged )
    {
        config.setRunOnStartUp( 0 );
        config.setStartByServer( iAutoStart );
    }
    config.setMaxIdleTime( INT_MAX );
    //if (( instances != 1 )&&
    //    ( config.getMaxConns() > instances ))
    //{
    //    LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Possible mis-configuration: 'Instances'=%d, "
    //            "'Max connections'=%d, unless one Fast CGI process is "
    //            "capable of handling multiple connections, "
    //            "you should set 'Instances' greater or equal to "
    //            "'Max connections'.", instances, config.getMaxConns());
    //    pApp->setMaxConns( instances );
    //}

    RLimits limits;
    memset( &limits, 0, sizeof( limits ) );
    config.setRLimits( &limits );

    Env * pEnv = config.getEnv();
    if ( pEnv->find( "PATH" ) == NULL )
    {
        pEnv->add( "PATH=/bin:/usr/bin" );
    }
    pEnv->add( 0, 0, 0, 0 );
    return 0;
}


int HttpServerImpl::configLsrecaptchaContexts()
{
    char achPath[MAX_PATH_LEN], achContext[MAX_PATH_LEN];

    HttpVHost *pGlobalVHost = HttpServerConfig::getInstance().getGlobalVHost();
    const char *pStaticUrl = Recaptcha::getStaticUrl()->c_str();
    const char *pStaticUrlEnd = (const char *)strrchr(pStaticUrl, '/');
    assert(pStaticUrlEnd != NULL);
    memcpy(achContext, pStaticUrl, pStaticUrlEnd - pStaticUrl);
    achContext[pStaticUrlEnd - pStaticUrl] = '\0';

    if (ConfigCtx::getCurConfigCtx()->getAbsolutePath(achPath, "$SERVER_ROOT/lsrecaptcha/") != 0) {
        LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Recaptcha path is invalid.");
        return -1;
    }
    HttpContext *pStaticContext = pGlobalVHost->addContext(achContext,
            HandlerType::HT_SSI, achPath, NULL, 1);
    if (NULL == pStaticContext)
    {
        LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Failed to add static lsrecaptcha context.");
        return -1;
    }

    if (ConfigCtx::getCurConfigCtx()->getAbsoluteFile(achPath, "$SERVER_ROOT/lsrecaptcha/_recaptcha_custom.shtml") == 0
        && access(achPath, F_OK) == 0 )
    {
        LS_INFO(ConfigCtx::getCurConfigCtx(),
                "Detect and use custom recaptcha page, %s.", achPath);
        Recaptcha::setStaticUrl("/.lsrecap/_recaptcha_custom.shtml");
    }

    HttpContext *pVerifierCtx = pGlobalVHost->addContext(Recaptcha::getDynUrl()->c_str(),
            HandlerType::HT_LSAPI, NULL, "lsrecaptcha", 1);
    if (NULL == pVerifierCtx)
    {
        LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Failed to add static lsrecaptcha context.");
        return -1;
    }
    pVerifierCtx->setCheckCaptcha(1);
    Recaptcha::setStaticCtx(pStaticContext);
    Recaptcha::setDynCtx(pVerifierCtx);
    return 0;
}


int HttpServerImpl::initLsrecaptcha(const XmlNode *pNode)
{
    ConfigCtx currentCtx("server", "recaptcha");

    uint16_t allowedBotHits = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                                    "allowedRobotHits", 0, SHRT_MAX, 3);
    ClientInfo::setMaxAllowedBotHits(allowedBotHits);

    if (HttpServerConfig::getInstance().getGlobalVHost()->configRecaptcha(pNode))
        return -1;

    int ret = configLsrecaptchaWorker( pNode );
    if (ret)
        return ret;

    return configLsrecaptchaContexts();
}


int HttpServerImpl::configChroot(const XmlNode *pRoot)
{
    MainServerConfig  &MainServerConfigObj =  MainServerConfig::getInstance();
    if ((getuid() == 0)
        && (ConfigCtx::getCurConfigCtx()->getLongValue(pRoot, "enableChroot", 0, 1,
                0)))
    {
        const char *pValue = pRoot->getChildValue("chrootPath");

        if (pValue)
        {
            char achTemp[512];
            char *pChroot = achTemp;
            lstrncpy(pChroot, pValue, sizeof(achTemp));
            int len = strlen(pChroot);
            len = GPath::checkSymLinks(pChroot, pChroot + len,
                                       pChroot + sizeof(achTemp), pChroot, 1);

            if (len == -1)
            {
                LS_ERROR(ConfigCtx::getCurConfigCtx(), "Invalid chroot path.");
                return LS_FAIL;
            }

            if (* (pChroot + len - 1) != '/')
            {
                * (pChroot + len++) = '/';
                * (pChroot + len) = 0;
            }

            if ((*pChroot != '/')
                || (access(pChroot, F_OK) != 0))
            {
                LS_ERROR(ConfigCtx::getCurConfigCtx(),
                         "chroot must be valid absolute path: %s",
                         pChroot);
                lstrncpy(pChroot, "/", sizeof(achTemp) - strlen(achTemp));
                len = 1;
            }

            if (strncmp(pChroot, MainServerConfigObj.getServerRoot(), len) != 0)
            {
                LS_ERROR(ConfigCtx::getCurConfigCtx(),
                         "Server root: %s falls out side of chroot: %s, "
                         "disable chroot!", MainServerConfigObj.getServerRoot(), pChroot);
                lstrncpy(pChroot, "/", sizeof(achTemp) - strlen(achTemp));
            }

            if (strcmp(pChroot, "/") != 0)
            {
                * (pChroot + --len) = 0;
                MainServerConfigObj.setChroot(pChroot);
                ServerProcessConfig::getInstance().setChroot(
                    (AutoStr2 *)MainServerConfigObj.getpsChroot());
                char achTemp[512];
                lstrncpy(achTemp, MainServerConfigObj.getServerRoot() +
                        MainServerConfigObj.getChrootlen(), sizeof(achTemp));
                setServerRoot(achTemp);
            }
        }
    }
    else
        LS_NOTICE(ConfigCtx::getCurConfigCtx(), "chroot is disabled.");

    return 0;
}


int HttpServerImpl::configServer(int reconfig, XmlNode *pRoot)
{
    int ret;

    if (!reconfig)
    {
        SystemInfo::maxOpenFile(4096);
        configMultiplexer(pRoot->getChild("tuning"));
        m_oldListeners.recvListeners();
        StdErrLogger::getInstance().initLogger(
            MultiplexerFactory::getMultiplexer());
#if defined(LS_AIO_USE_SIGFD) || defined(LS_AIO_USE_SIGNAL)
        SigEventDispatcher::init();
#endif
        if (configChroot(pRoot))
            return LS_FAIL;

        int pri = getpriority(PRIO_PROCESS, 0);
        setpriority(PRIO_PROCESS, 0,
                    ServerProcessConfig::getInstance().getPriority());
        int new_pri = getpriority(PRIO_PROCESS, 0);
        LS_INFO(ConfigCtx::getCurConfigCtx(), "old priority: %d, new priority: %d",
                pri, new_pri);
    }


    ret = configServerBasic2(pRoot, pRoot->getChildValue("swappingDir"));

    if (ret)
        return ret;


    if (!MainServerConfig::getInstance().getDisableWebAdmin())
    {
        ret = loadAdminConfig(pRoot);
        if (ret)
            return ret;
    }
    else
    {
        //When webAdmin disabled, set to enableCoreDump to 1.
        MainServerConfig::getInstance().setEnableCoreDump(1);
    }

    configTuning(pRoot);

    //Must load modules before parse and set scriptHandlers
    configModules(pRoot);

    if (!MainServerConfig::getInstance().getConfTestMode())
    {
        if (!MainServerConfig::getInstance().getDisableWebAdmin())
        {
            if (startAdminListener(pRoot, ADMIN_CONFIG_NODE))
                return LS_FAIL;
        }

        //All other server listeners
        startListeners(pRoot);
    }

    int maxconns = ConnLimitCtrl::getInstance().getMaxConns();
    unsigned long long maxfds = SystemInfo::maxOpenFile(maxconns * 5);
    LS_NOTICE(ConfigCtx::getCurConfigCtx(),
              "The maximum number of file descriptor limit is set to %llu.",
              maxfds);

    if ((unsigned long long) maxconns + 100 > maxfds)
    {
        LS_WARN(ConfigCtx::getCurConfigCtx(),
                "Current per process file descriptor limit: %llu is too low comparing to "
                "you currnet 'Max connections' setting: %d, consider to increase "
                "your system wide file descriptor limit by following the instruction "
                "in our HOWTO #1.", maxfds, maxconns);
    }
    if (initGroups())
        return LS_FAIL;


    if (!MainServerConfig::getInstance().getDisableWebAdmin())
    {
        ret = configAdminConsole(pRoot->getChild(ADMIN_CONFIG_NODE));
        if (ret)
        {
            LS_ERROR(ConfigCtx::getCurConfigCtx(),
                     "Failed to setup the WEB administration interface!");
            return ret;
        }
    }

    {
        ConfigCtx currentCtx("server", "epsr");
        ExtAppRegistry::configExtApps(pRoot, NULL);
    }

    {
        ConfigCtx currentCtx("server", "rails");
        AppConfig::s_rubyAppConfig.loadAppDefault(pRoot->getChild("railsDefaults"));
    }

    {
        ConfigCtx currentCtx("server", "python");
        AppConfig::s_wsgiAppConfig.loadAppDefault(pRoot->getChild("wsgiDefaults"));
    }

    {
        ConfigCtx currentCtx("server", "nodejs");
        AppConfig::s_nodeAppConfig.loadAppDefault(pRoot->getChild("nodeDefaults"));
    }

    const XmlNode *p0 = pRoot->getChild("scriptHandler");
    if (p0 != NULL)
    {
        const XmlNodeList *pList = p0->getChildren("add");

        if (pList && pList->size() > 0)
            HttpMime::configScriptHandler(pList, NULL, NULL);
    }

    int val = ConfigCtx::getCurConfigCtx()->getLongValue(pRoot, "enableIpGeo", -1, 1, -1);
    configIpToGeo(pRoot);
    configIpToLoc(pRoot);
    if (val == -1)
        val = ClientCache::getIp2Geo() != NULL;
    HttpServer::getInstance().getServerContext().setGeoIP(val);

    const char *pVal = pRoot->getChildValue("suspendedVhosts");
    if (pVal)
    {
        MainServerConfig::getInstance().getSuspendedVhosts().split(pVal,
                pVal + strlen(pVal), ",");
        MainServerConfig::getInstance().getSuspendedVhosts().sort();
    }


    HttpServer::getInstance().initAccessLog(pRoot, 1);
    configVHosts(pRoot);
    configZconfSvrConf(pRoot);
    configListenerVHostMap(pRoot, NULL);
    if (!MainServerConfig::getInstance().getConfTestMode())
    {
        configVHTemplates(pRoot);
    }

    ZConfManager::getInstance().prepareServerUp();

    return ret;
}


int HttpServerImpl::changeUserChroot()
{
    if (getuid() != 0)
        return 0;
    const char *pChroot = MainServerConfig::getInstance().getChroot();
    const char *pUser = MainServerConfig::getInstance().getUser();

#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    enableCoreDump();
#endif
    LS_DBG_L(ConfigCtx::getCurConfigCtx(),
             "try to give up super user privilege!");
    char achBuf[256];

    if (Daemonize::changeUserChroot(pUser,
                                    ServerProcessConfig::getInstance().getUid(),
                                    pChroot, achBuf, 256))
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "%s", achBuf);
        return LS_FAIL;
    }
    else
    {
        LS_NOTICE(ConfigCtx::getCurConfigCtx(),
                  "[child: %d] Successfully change current user to %s",
                  getpid(), pUser);
#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
        enableCoreDump();
#endif

        if (pChroot)
        {
            LS_NOTICE(ConfigCtx::getCurConfigCtx(),
                      "[child: %d] Successfully change root directory to %s",
                      getpid(), pChroot);
        }
    }

    return 0;
}


void HttpServerImpl::enableCoreDump()
{
#if defined(__FreeBSD__ ) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    int  mib[2];
    size_t len;

    len = 2;
    int ienableCoreDump = MainServerConfig::getInstance().getEnableCoreDump();
    if (sysctlnametomib("kern.sugid_coredump", mib, &len) == 0)
    {
        len = sizeof(ienableCoreDump);

        if (sysctl(mib, 2, NULL, 0, &ienableCoreDump, len) == -1)
            LS_WARN(ConfigCtx::getCurConfigCtx(),
                    "sysctl: Failed to set 'kern.sugid_coredump', "
                    "core dump may not be available!");
        else
        {
            int dumpable;
            MainServerConfig::getInstance().setEnableCoreDump(ienableCoreDump);

            if (sysctl(mib, 2, &dumpable, &len, NULL, 0) != -1)
            {
                LS_WARN(ConfigCtx::getCurConfigCtx(), "Core dump is %s.",
                        (dumpable) ? "enabled" : "disabled");
            }
        }
    }


#endif

#if defined(linux) || defined(__linux) || defined(__linux__) || defined(__gnu_linux__)

    if (prctl(PR_SET_DUMPABLE,
              MainServerConfig::getInstance().getEnableCoreDump()) == -1)
        LS_WARN(ConfigCtx::getCurConfigCtx(), "prctl: Failed to set dumpable, "
                "core dump may not be available!");

    {
        int dumpable = prctl(PR_GET_DUMPABLE);

        if (dumpable == -1)
            LS_WARN(ConfigCtx::getCurConfigCtx(), "prctl: get dumpable failed ");
        else
            LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Child: %d] Core dump is %s.",
                      getpid(),
                      (dumpable) ? "enabled" : "disabled");
    }
#endif
}


/* COMMENT: Not support reconfigVHost NOW.
void HttpServerImpl::reconfigVHost( char *pVHostName, XmlNode* pRoot )
{
    HttpVHost *pNew;

    if ( !reconfigVHost( pVHostName, pNew, pRoot ) )
        if ( updateVHost( pVHostName, pNew ) )
            if ( pNew )
                delete pNew;

    configListenerVHostMap( pRoot, pVHostName );
}

int HttpServerImpl::reconfigVHost( const char *pVHostName, HttpVHost * &pVHost, XmlNode* pRoot )
{
    pVHost = NULL;

    XmlNode *pNode = pRoot->getChild( "virtualHostList" );

    if ( !pNode )
        pNode = pRoot;

    const XmlNodeList *pList = pNode->getChildren( "virtualHost" );

    if ( pList )
    {
        XmlNodeList::const_iterator iter;

        for( iter = pList->begin(); iter != pList->end(); ++iter )
        {
            XmlNode *pVhostNode = *iter;
            ConfigCtx::getCurConfigCtx()->clearVhRoot();
            const char *pName = pVhostNode->getChildValue( "name" );

            if ( ( pName ) && ( strcmp( pName, pVHostName ) == 0 ) )
            {
                pVHost = HttpVHost::configVHost( pVhostNode );
                break;
            }
        }
    }

    return 0;

}
*/


void HttpServerImpl::setServerRoot(const char *pRoot)
{
    MainServerConfig::getInstance().setServerRoot(pRoot);
}


static void setupSUExec()
{
    SUExec::initSUExec();
}


int HttpServerImpl::initServer(XmlNode *pRoot, int &iReleaseXmlTree,
                               int reconfig)
{
    int ret;
    iReleaseXmlTree = 0;

    if (ConfigCtx::getCurConfigCtx())
    {
        ConfigCtx *pCurConfigCtx = new ConfigCtx();
        LS_NOTICE(pCurConfigCtx, "initServer.... ");
    }

    if (!reconfig)
    {
        HttpVHost *pVHost = new HttpVHost("LswsDefault");
        pVHost->setDocRoot("/Does not exist path/");
        HttpServerConfig::getInstance().setGlobalVHost(pVHost);
        setupSUExec();
    }

    //ret = configServerBasics( reconfig );
    ret = configServerBasics(reconfig, pRoot);

    if (ret)
        return ret;

    ClientCache::initClientCache(1000);

    beginConfig();
    //ret = configServer( reconfig );
    ret = configServer(reconfig, pRoot);
    endConfig(ret);
    iReleaseXmlTree = 1;

    //releaseConfigXmlTree();
    return ret;
}

int HttpServerImpl::initLscpd()
{
    int ret = 0;
#ifdef IS_LSCPD

#define LSCPD_PROXY_APP_NAME    "gunicorn"
#define LSCPD_FILEMANAGER_PROXY_APP_NAME    "filemanager"
#define LSCPD_VHOST_NAME        "cyberpanel"
#define LSCPD_PROXY_ADDRESS     "127.0.0.1:5003"
#define LSCPD_FILEMANAGER_PROXY_ADDRESS     "localhost:80"
#define LSCPD_LISTENER_ADDRESS  "*:8090"

    MainServerConfig  &mainServerConfig =  MainServerConfig::getInstance();
    char achBuf[256], achBuf1[256];
    char *p = achBuf;
    lstrncpy(p, mainServerConfig.getServerRoot(), sizeof(achBuf));
    assert(p != NULL);
    char *pEnd = p + strlen(p);
    lstrncpy(achBuf1, achBuf, sizeof(achBuf1));

    beginConfig();
    ServerProcessConfig &procConfig = ServerProcessConfig::getInstance();
    HttpServerConfig &httpServerConfig = HttpServerConfig::getInstance();

#if defined(LS_AIO_USE_KQ)
        SigEventDispatcher::setAiokoLoaded();
#endif

    //similar as configServerBasics
    HttpServer::getInstance().initAllLog(mainServerConfig.getServerRoot());
    mainServerConfig.setServerName("lscp");

    const char *pUser = OPENLSWS_USER;
    const char *pGroup = OPENLSWS_GROUP;
    mainServerConfig.setGroup(pGroup);
    mainServerConfig.setUser(pUser);

    gid_t gid = procConfig.getGid();
    struct passwd *pw = Daemonize::configUserGroup(pUser, pGroup, gid);
    procConfig.setGid(gid);
    if (!pw)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "Invalid User Name(%s) "
                 "or Group Name(%s)!", pUser, pGroup);
        return -1;
    }

    procConfig.setUid(pw->pw_uid);
    m_pri_gid = pw->pw_gid;

    if (getuid() == 0)
    {
        chown(HttpLog::getErrorLogFileName(),
              procConfig.getUid(), procConfig.getGid());
        chown(HttpLog::getAccessLogFileName(),
              procConfig.getUid(), procConfig.getGid());
    }

    mainServerConfig.setAdminEmails("root@localhost");
    mainServerConfig.setDisableWebAdmin(1);
    procConfig.setPriority(0);

    httpServerConfig.setChildren(1);
    mainServerConfig.setDisableLogRotateAtStartup(0);
    HttpStats::set503AutoFix(1);
    httpServerConfig.setEnableH2c(0);
    httpServerConfig.setRestartTimeOut(300);

    //this value can only be set once when server start.
    if (mainServerConfig.getCrashGuard() == 2)
        mainServerConfig.setCrashGuard(1);

    m_sRTReportFile = sStatDir;
    m_sRTReportFile.append("/.rtreport", 10);

    setupSUExec();
    //
    SystemInfo::maxOpenFile(4096);
    m_dispatcher.init("best");
    m_oldListeners.recvListeners();

    //configServerBasic2
    VMemBuf::setMaxAnonMapSize(20 * 1024 * 1024);
    setSwapDir(DEFAULT_TMP_DIR "/swap");

    ls_snprintf(achBuf, 256, "%s/tmp/ocspcache/",
                mainServerConfig.getServerRoot());
    SslOcspStapling::setCachePath(achBuf);

    HttpRespHeaders::hideServerSignature(0);
    HttpServer::getInstance().getServerContext().setGeoIP(0);
    HttpServer::getInstance().getServerContext().setIpToLoc(0);
    httpServerConfig.setUseProxyHeader(0);
    denyAccessFiles(NULL, ".ht*", 0);

    ls_snprintf(achBuf, 256, "%s/conf/mime.properties",
                mainServerConfig.getServerRoot());
    HttpMime::getMime()->loadMime(achBuf);
    if (HttpMime::getMime()->getDefault() == 0)
        HttpMime::getMime()->initDefault();

    m_serverContext.setConfigBit(BIT_ENABLE_EXPIRES, 1);
    m_serverContext.getExpires().enable(1);
    m_serverContext.getExpires().parse("image/*=A604800, text/css=A604800, application/x-javascript=A604800");
    m_serverContext.setConfigBit(BIT_EXPIRES_DEFAULT, 1);

    //configSecurity
    DeniedDir *pDeniedDir = httpServerConfig.getDeniedDir();
    pDeniedDir->clear();
    pDeniedDir->addDir("/");
    pDeniedDir->addDir("/etc/*");
    pDeniedDir->addDir("/dev/*");
    ls_snprintf(achBuf, 256, "%s/conf/*",
                mainServerConfig.getServerRoot());
    pDeniedDir->addDir(achBuf);


    httpServerConfig.setFollowSymLink(1);
    httpServerConfig.checkDeniedSymLink(0);
    httpServerConfig.setRequiredBits(000);
    httpServerConfig.setForbiddenBits(041111);
    httpServerConfig.setScriptForbiddenBits(000);
    httpServerConfig.setDirForbiddenBits(0000);

    NtwkIOLink::enableThrottle(1);
    ClientCache::initClientCache(1000);
    ClientCache::getClientCache()->resetThrottleLimit();
    ClientInfo::setPerClientSoftLimit(10000);
    ClientInfo::setPerClientHardLimit(10000);
    ClientInfo::setOverLimitGracePeriod(15);
    ClientInfo::setBanPeriod(300);

    RLimits limits;
    limits.setDataLimit(460 * 1024 * 1024, 470 * 1024 * 1024);
    limits.setProcLimit(400, 450);
    ExtAppRegistry::setRLimits(&limits);
    ExtAppRegistry::getRLimits()->setCPULimit(10, 50);

    procConfig.setUidMin(11);
    procConfig.setGidMin(10);
    procConfig.setForceGid(0);

    char achMIME[] = "application/x-httpd-cgi";
    HttpMime::getMime()->addMimeHandler("", achMIME,
                                        HandlerFactory::getInstance(HandlerType::HT_CGI, NULL), NULL,
                                        TmpLogId::getLogId());

    m_serverContext.setModuleConfig(ModuleManager::getInstance().getGlobalModuleConfig(), 0);
    m_serverContext.initExternalSessionHooks();


    httpServerConfig.setConnTimeOut(300);
    httpServerConfig.setKeepAliveTimeout(5);
    httpServerConfig.setSmartKeepAlive(0);
    httpServerConfig.setFollowSymLink(1);
    httpServerConfig.setMaxKeepAliveRequests(1000);
    httpServerConfig.setMaxDynRespLen(30 * 1024 * 1024); //30MB

    ThrottleLimits *pLimit = ThrottleControl::getDefault();
    pLimit->setDynReqLimit(INT_MAX);
    pLimit->setStaticReqLimit(INT_MAX);
    pLimit->setInputLimit(INT_MAX);
    pLimit->setOutputLimit(INT_MAX);

    ConnLimitCtrl::getInstance().setMaxConns(2000);
    ConnLimitCtrl::getInstance().setMaxSSLConns(1000);
    m_accessCtrl.addSubNetControl("ALL", 1);
    AccessControl::setAccessCtrl(&m_accessCtrl);

    //proxy
    ExtWorker *pProxy = ExtAppRegistry::addApp(EA_PROXY, LSCPD_PROXY_APP_NAME);
    pProxy->setURL(LSCPD_PROXY_ADDRESS);
    pProxy->getConfigPointer()->setMaxConns(100);
    pProxy->getConfigPointer()->setKeepAliveTimeout(60);
    pProxy->getConfigPointer()->setTimeout(60);
    pProxy->getConfigPointer()->setRetryTimeout(0);
    pProxy->getConfigPointer()->setBuffering(0);

    //proxy filemanager

    ExtWorker *fileProxy = ExtAppRegistry::addApp(EA_PROXY, LSCPD_FILEMANAGER_PROXY_APP_NAME);
    fileProxy->setURL(LSCPD_FILEMANAGER_PROXY_ADDRESS);
    fileProxy->getConfigPointer()->setMaxConns(100);
    fileProxy->getConfigPointer()->setKeepAliveTimeout(60);
    fileProxy->getConfigPointer()->setTimeout(60);
    fileProxy->getConfigPointer()->setRetryTimeout(0);
    fileProxy->getConfigPointer()->setBuffering(0);

    //php
    LocalWorker *pPhp = (LocalWorker *)ExtAppRegistry::addApp(EA_LSAPI, "php");
    assert(pPhp);
    pPhp->setURL("UDS://tmp/lscpd/lsphp.sock");
    lstrncpy(pEnd, "/fcgi-bin/lsphp", sizeof(achBuf) - (pEnd - achBuf));
    pPhp->getConfig().setAppPath(achBuf);
    pPhp->getConfig().setMaxConns(50);
    pPhp->getConfig().setTimeout(60);
    pPhp->getConfig().setRetryTimeout(0);
    pPhp->getConfig().setBuffering(0);
    limits.setDataLimit(2047 * 1024 * 1024, 2047 * 1024 * 1024);
    limits.setProcLimit(400, 500);
    pPhp->getConfig().setRLimits(&limits);


    pPhp->getConfig().addEnv("PHP_LSAPI_MAX_REQUESTS=500");
    pPhp->getConfig().addEnv("PHP_LSAPI_CHILDREN=20");

    //listener
    lstrncat(achBuf1, "/key.pem", achBuf1);
    lstrncpy(pEnd, "/cert.pem", sizeof(achBuf) - (pEnd - achBuf));
    HttpListener *pListener = addListener("DefaultSSL", LSCPD_LISTENER_ADDRESS);
    if (!pListener)
    {
        LS_ERROR("Failed to addListener <DefaultSSL> to port <%s>.",
                 LSCPD_LISTENER_ADDRESS);
        return LS_FAIL;
    }

    SslContextConfig config;
    config.m_sName = "DefaultSSL";
    config.m_sKeyFile[0] = achBuf1;
    config.m_sCertFile[0] = achBuf;
    config.m_sCiphers = "ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+SSLv2:+EXP";
    config.m_iProtocol = 28;
    config.m_iEnableECDHE = 1;
    config.m_iEnableDHE = 1;
    config.m_iEnableTicket = -1;
    pListener->getVHostMap()->setSslContext(SslContext::config(NULL, &config));

    //vhost
    HttpVHost *pVHost = new HttpVHost(LSCPD_VHOST_NAME);
    if (pVHost == NULL)
    {
        LS_ERROR("Failed to create VHost <%s>.", LSCPD_VHOST_NAME);
        return LS_FAIL;
    }

    pVHost->getRootContext().setParent(
        &HttpServer::getInstance().getServerContext());
    pVHost->getRootContext().inherit(&HttpServer::getInstance().getServerContext());
    pVHost->contextInherit();

    lstrncpy(pEnd, "/" LSCPD_VHOST_NAME "/", sizeof(achBuf) - (pEnd - achBuf));
    pVHost->setVhRoot(achBuf);
    lstrncpy(pEnd, "/" LSCPD_VHOST_NAME "/logs/error.log",
            sizeof(achBuf) - (pEnd - achBuf));
    pVHost->setErrorLogFile(achBuf);
    pVHost->setErrorLogRollingSize( 10 * 1024 * 1024, 30 );
    pVHost->setLogLevel( "DEBUG" );

    lstrncpy(pEnd, "/" LSCPD_VHOST_NAME "/access.log",
            sizeof(achBuf) - (pEnd - achBuf));
    pVHost->setAccessLogFile(achBuf, 1 );
    pVHost->getLogger()->getAppender()->setRollingSize(30 * 1024 * 1024);
    pVHost->getLogger()->getAppender()->setKeepDays(30);
    pVHost->getLogger()->getAppender()->setCompress(0);
    pVHost->setUidMode(0);
    pVHost->followSymLink(1);
    pVHost->enableScript(1);
    pVHost->restrained(1);
    pVHost->enableGzip(1);
    HttpContext *pContext = pVHost->addContext("/", HandlerType::HT_NULL,
                                               pVHost->getVhRoot()->c_str(),
                                               NULL, true);
    pContext->addDirIndexes("index.php, index.html, index.htm, default.html");

    //update mime
    char achMIMEHtml[] = "text/html";
    HttpMime::getMime()->updateMIME(achMIMEHtml,
                                    HttpMime::setCompressible, (void *)1, NULL);
    StaticFileCacheData::setUpdateStaticGzipFile(1, 6, 300, 1024 * 1024);

    //add scripthandler
    HttpMime *pMime = pVHost->getMIME();
    HttpMime::getMime()->addMimeHandler("php", NULL, pPhp, NULL, "");

    //Vhost config
    pVHost->setDocRoot(pVHost->getVhRoot()->c_str());
    pVHost->getRootContext().setAutoIndex(1);

    pVHost->setAutoIndexURI("/_autoindex/default.php");
    pVHost->getRootContext().enableScript(1);
    pVHost->getRootContext().setCustomErrUrls("404", "/error404.html");
    pVHost->getRootContext().getExpires().enable(1);
    pVHost->getRootContext().setConfigBit(BIT_ENABLE_EXPIRES, 1);

    pVHost->getRootContext().enableRewrite(1);
    pVHost->setRewriteLogLevel(0);

    char *pRules = "RewriteRule ^/preview/([^/]*)/(.*) http://" LSCPD_FILEMANAGER_PROXY_APP_NAME "/$2 [P,E=Proxy-Host:$1]\r\n"
                   "RewriteRule ^/filemanager/([^/]*)/php/fileManager.php http://" LSCPD_FILEMANAGER_PROXY_APP_NAME "/.filemanager/php/fileManager.php [P,E=Proxy-Host:$1]\r\n"
                   "RewriteRule ^/filemanager/([^/]*)/php/caller.php http://" LSCPD_FILEMANAGER_PROXY_APP_NAME "/.filemanager/php/caller.php [P,E=Proxy-Host:$1]\r\n"
                   "RewriteCond %{ORG_REQ_URI} !/static\r\n"
                   "RewriteCond %{ORG_REQ_URI} !/rainloop\r\n"
                   "RewriteCond %{ORG_REQ_URI} !/phpmyadmin\r\n"
                   "RewriteCond %{ORG_REQ_URI} !/noVNC\r\n"
                   "RewriteRule ^/(.*)$ http://" LSCPD_PROXY_APP_NAME "/$1 [P]\r\n";

    pVHost->getRootContext().configRewriteRule(pVHost->getRewriteMaps(), pRules, "");

    if (addVHost(pVHost) == 0)
    {
    }
    if (mapListenerToVHost(pListener, "*", LSCPD_VHOST_NAME) == 0)
    {
    }

    CgidWorker * pWorker = (CgidWorker *)ExtAppRegistry::addApp(
                            EA_CGID, LSCGID_NAME );

    pWorker->getConfig().setSocket("uds:/" DEFAULT_TMP_DIR "/cgid/cgid.sock");
    pWorker->start(mainServerConfig.getServerRoot(), NULL,
                   getuid(), getgid(), getpriority(PRIO_PROCESS, 0));
    endConfig(0);
#endif
    return ret;
}


#ifdef RUN_TEST


int HttpServerImpl::initSampleServer()
{
    beginConfig();
    HttpListener *pListener;
    HttpServerConfig &serverConfig = HttpServerConfig::getInstance();
    char achBuf[256], achBuf1[256];
    char *p = achBuf;
    lstrncpy(p, MainServerConfig::getInstance().getServerRoot(), sizeof(achBuf));
    assert(p != NULL);
    char *pEnd = p + strlen(p);
    lstrncpy(achBuf1, achBuf, sizeof(achBuf1));

    m_dispatcher.init("poll");

    serverConfig.setConnTimeOut(3000000);
    serverConfig.setKeepAliveTimeout(15);
    serverConfig.setSmartKeepAlive(0);
    serverConfig.setFollowSymLink(1);
    serverConfig.setMaxKeepAliveRequests(10000);

    ThrottleLimits *pLimit = ThrottleControl::getDefault();
    pLimit->setDynReqLimit(INT_MAX);
    pLimit->setStaticReqLimit(INT_MAX);
    pLimit->setInputLimit(INT_MAX);
    pLimit->setOutputLimit(INT_MAX);

    serverConfig.setDebugLevel(0);
    ClientInfo::setPerClientSoftLimit(1000);
    serverConfig.setGzipCompress(6);
    serverConfig.setDynGzipCompress(1);
    serverConfig.setMaxURLLen(8192);
    serverConfig.setMaxHeaderBufLen(16380);
    serverConfig.setMaxDynRespLen(2047 * 1024 * 1024); //2047M

    ConnLimitCtrl::getInstance().setMaxConns(1000);
    ConnLimitCtrl::getInstance().setMaxSSLConns(1);
    //enableScript( 1 );
    setSwapDir(DEFAULT_TMP_DIR "1/swap");

    m_accessCtrl.addSubNetControl("192.168.2.0", "255.255.255.0", false);
    AccessControl::setAccessCtrl(&m_accessCtrl);
    //strcpy( pEnd, "/logs/error.log" );
    //setErrorLogFile( achBuf );
    lstrncpy(pEnd, "/logs/access.log", sizeof(achBuf) - (pEnd - achBuf));
    HttpLog::setAccessLogFile(achBuf, 0);
    lstrncpy(pEnd, "/logs/stderr.log", sizeof(achBuf) - (pEnd - achBuf));
    StdErrLogger::getInstance().setLogFileName(achBuf);
    HttpLog::setLogPattern("%d [%p] %m");
    HttpLog::setLogLevel("DEBUG");
    lstrncat(achBuf1, "/cert/server.crt", sizeof(achBuf1));
    lstrncpy(pEnd, "/cert/server.pem", sizeof(achBuf) - (pEnd - achBuf));



    if (addListener("*:3080") == 0)
    {
    }
    SslContextConfig config;
    config.m_sName = "*:1443";
    config.m_sKeyFile[0] = achBuf1;
    config.m_sCertFile[0] = achBuf;
    config.m_sCiphers = "ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+SSLv2:+EXP";
    config.m_iProtocol = 13;
    config.m_iEnableECDHE = 1;
    config.m_iEnableDHE = 1;
    config.m_iEnableTicket = -1;
    pListener = addListener( "*:1443" );
    pListener->getVHostMap()->setSslContext(SslContext::config(NULL, &config));



    HttpVHost *pVHost = new HttpVHost("vhost1");
    HttpVHost *pVHost2 = new HttpVHost("vhost2");
    assert(pVHost != NULL);
    pVHost->getRootContext().setParent(
        &HttpServer::getInstance().getServerContext());
    pVHost2->getRootContext().setParent(
        &HttpServer::getInstance().getServerContext());
    lstrncpy(pEnd, "/logs/vhost1.log", sizeof(achBuf) - (pEnd - achBuf));
    pVHost->setErrorLogFile(achBuf);
    pVHost->setErrorLogRollingSize(8192, 10);
    pVHost->setLogLevel("DEBUG");
    //strcpy( pEnd, "/logs/vhost1_access.log" );
    //pVHost->setAccessLogFile( achBuf );

    pVHost->enableAccessCtrl();
    pVHost->getAccessCtrl()->addSubNetControl("192.168.1.0", "255.255.255.0",
            false);

    pVHost->getRootContext().addDirIndexes(
        "index.htm, index.php, index.html, default.html");
    pVHost->setMaxKAReqs(100);

    pLimit = pVHost->getThrottleLimits();
    pLimit->setDynReqLimit(2);
    pLimit->setStaticReqLimit(20);

    //load Http mime types
    lstrncpy(pEnd, "/conf/mime.properties", sizeof(achBuf) - (pEnd - achBuf));
    if (HttpMime::getMime()->loadMime(achBuf) == 0)
    {
    }
    char achMIMEHtml[] = "text/html";
    HttpMime::getMime()->updateMIME(achMIMEHtml,
                                    HttpMime::setCompressible, (void *)1, NULL);
    StaticFileCacheData::setUpdateStaticGzipFile(1, 6, 300, 1024 * 1024);

    //protected context
    lstrncpy(pEnd, "/wwwroot/protected/", sizeof(achBuf) - (pEnd - achBuf));
    HttpContext *pContext = pVHost->addContext("/protected/",
                            HandlerType::HT_NULL, achBuf, NULL, true);



    lstrncpy(pEnd, "/htpasswd", sizeof(achBuf) - (pEnd - achBuf));
    lstrncpy(&achBuf1[pEnd - achBuf], "/htgroup",
            sizeof(achBuf1) - (pEnd - achBuf));
    UserDir *pDir
        = pVHost->getFileUserDir("RistrictedArea", achBuf, achBuf1);

    HTAuth *pAccess = new HTAuth();
    pContext->setAuthRequired("user test");
    pAccess->setUserDir(pDir);
    pAccess->setName(pDir->getName());
    pContext->setHTAuth(pAccess);

    AccessControl *pControl = new AccessControl();
    pControl->addSubNetControl("192.168.0.0", "255.255.255.0", false);
    pControl->addIPControl("192.168.0.10", true);
    pContext->setAccessControl(pControl);

    //FileMatch Context
    pContext = new HttpContext();
    if (pContext)
    {
        pContext->setFilesMatch(".ht*", 0);
        pContext->addAccessRule("*", 0);
        m_serverContext.addFilesMatchContext(pContext);
    }

    lstrncpy(pEnd, "/wwwroot/", sizeof(achBuf) - (pEnd - achBuf));
    pContext =  pVHost->addContext("/", HandlerType::HT_NULL, achBuf, NULL,
                                   true);

    lstrncpy(pEnd, "/wwwroot/phpinfo.php$3?username=$1",
            sizeof(achBuf) - (pEnd - achBuf));
    HttpContext *pMatchContext = new HttpContext();
    pVHost->setContext(pMatchContext, "exp: ^/~(([a-z])[a-z0-9]+)(.*)",
                       HandlerType::HT_NULL, achBuf, NULL, true);
    pContext->addMatchContext(pMatchContext);

    pMatchContext = new HttpContext();
    pVHost->setContext(pMatchContext, "exp: ^/!(.*)", HandlerType::HT_REDIRECT,
                       "/phpinfo.php/path/info?username=$1", NULL, true);
    pMatchContext->redirectCode(SC_302);
    pContext->addMatchContext(pMatchContext);

    pContext = pVHost->addContext("/redirauth/",
                                  HandlerType::HT_REDIRECT, "/private/", NULL , false);

    pContext = pVHost->addContext("/redirect/",
                                  HandlerType::HT_REDIRECT, "/test", NULL, false);

    pContext = pVHost->addContext("/redirabs/", HandlerType::HT_REDIRECT,
                                  "http:/index.html", NULL, false);
    pContext->redirectCode(SC_301);

    pContext = pVHost->addContext("/redircgi/", HandlerType::HT_REDIRECT,
                                  "/cgi-bin/", NULL, false);

    pContext = pVHost->addContext("/denied/", HandlerType::HT_NULL,
                                  "denied/", NULL, false);


    //CGI context
    lstrncpy(pEnd, "/cgi-bin/", sizeof(achBuf) - (pEnd - achBuf));
    pContext = pVHost->addContext("/cgi-bin/", HandlerType::HT_CGI,
                                  achBuf, "lscgid", true);

    //servlet engines
    JWorker *pSE = (JWorker *)ExtAppRegistry::addApp(EA_JENGINE, "tomcat1");
    assert(pSE);
    pSE->setURL("localhost:8009");
    pSE->getConfig().setMaxConns(10);

//    m_builder.importWebApp( pVHost, "/examples/",
//        "/opt/tomcat/webapps/examples/", "tomcat1", true );


    ExtWorker *pProxy = ExtAppRegistry::addApp(EA_PROXY, "proxy1");
    assert(pProxy);
    pProxy->setURL("192.168.0.10:5080");
    pProxy->getConfigPointer()->setMaxConns(10);

    //Fast cgi application
    lstrncpy(pEnd, "/fcgi-bin/lt-echo-cpp", sizeof(achBuf) - (pEnd - achBuf));
    //strcpy( pEnd, "/fcgi-bin/echo" );
    //FcgiApp * pFcgiApp = addFcgiApp( "localhost:5558" );
    FcgiApp *pFcgiApp = (FcgiApp *)ExtAppRegistry::addApp(
                            EA_FCGI, "localhost:5558");
    assert(pFcgiApp != NULL);
    pFcgiApp->setURL("UDS://tmp/echo");
    pFcgiApp->getConfig().setAppPath(achBuf);
    pFcgiApp->getConfig().setBackLog(20);
    pFcgiApp->getConfig().setInstances(3);
    pFcgiApp->getConfig().setMaxConns(3);


    lstrncpy(pEnd, "/fcgi-bin/logger.pl", sizeof(achBuf) - (pEnd - achBuf));
    pFcgiApp = (FcgiApp *)ExtAppRegistry::addApp(
                   EA_LOGGER, "logger");
    assert(pFcgiApp);
    //pFcgiApp.setURL( "localhost:5556" );
    pFcgiApp->setURL("UDS://tmp/logger.sock");
    pFcgiApp->getConfig().setAppPath(achBuf);
    pFcgiApp->getConfig().setBackLog(20);
    pFcgiApp->getConfig().setMaxConns(10);
    pFcgiApp->getConfig().setInstances(10);
    pVHost->setAccessLogFile("logger", 1);
    pVHost->getAccessLog()->setLogHeaders(LOG_REFERER | LOG_USERAGENT);
    pFcgiApp->getConfig().setVHost(pVHost);

    lstrncpy(pEnd, "/fcgi-bin/php", sizeof(achBuf) - (pEnd - achBuf));
    pFcgiApp = (FcgiApp *)ExtAppRegistry::addApp(
                   EA_FCGI, "php-fcgi");
    assert(pFcgiApp);
    //pFcgiApp.setURL( "localhost:5556" );
    pFcgiApp->setURL("UDS://tmp/php.sock");
    pFcgiApp->getConfig().setAppPath(achBuf);
    pFcgiApp->getConfig().setBackLog(20);
    pFcgiApp->getConfig().setMaxConns(20);
    pFcgiApp->getConfig().addEnv("FCGI_WEB_SERVER_ADDRS=127.0.0.1");
    pFcgiApp->getConfig().addEnv("PHP_FCGI_CHILDREN=2");
    pFcgiApp->getConfig().addEnv("PHP_FCGI_MAX_REQUESTS=1000");

    //echo Fast CGI context
    pContext = pVHost->addContext("/fcgi-bin/echo", HandlerType::HT_FASTCGI,
                                  NULL, "localhost:5558", false);
    //setContext( pContext, "/", HandlerType::HT_FASTCGI, NULL, "localhost:5558", false );


    pContext = pVHost->addContext("/fcgi-bin/php", HandlerType::HT_FASTCGI,
                                  NULL, "localhost:5556", false);

    pContext = pVHost->addContext("/proxy/", HandlerType::HT_PROXY,
                                  NULL, "proxy1", false);
    //pContext->allowOverride( 0 );
    //pContext->setConfigBit( BIT_ALLOW_OVERRIDE, 1 );

//    ScriptHandlerMap& map = pVHost->getScriptHandlerMap();
    pVHost->getRootContext().initMIME();
    HttpMime *pMime = pVHost->getMIME();
    HttpMime::getMime()->addMimeHandler("php", NULL, pFcgiApp, NULL, "");
    //pMime->addMimeHandler( "cgi", NULL, HandlerType::HT_CGI, NULL, NULL, "" );
    pMime->addMimeHandler("jsp", NULL, pSE, NULL, "");
    pVHost->enableScript(1);

    pContext = new HttpContext();
    if (pContext)
    {
        char achType[] = "application/x-httpd-php";
        pContext->setFilesMatch("phpinfo", 0);
        pContext->setForceType(achType, "");
        m_serverContext.addFilesMatchContext(pContext);
    }

    //pVHost->setCustomErrUrls( 404, "/baderror404.html" );
    //pVHost->setCustomErrUrls( 403, "/cgi-bin/error403" );

//    CgidWorker * pWorker = (CgidWorker *)ExtAppRegistry::addApp(
//                        EA_CGID, LSCGID_NAME );
//    CgidWorker::start("/home/gwang/lsws/", NULL, getuid(), getgid(),
//                        getpriority(PRIO_PROCESS, 0));

    lstrncpy(pEnd, "/wwwroot/", sizeof(achBuf) - (pEnd - achBuf));
    //printf( "WWW root = %s\n", achBuf );
    pVHost->setDocRoot(achBuf);
    ConfigCtx::getCurConfigCtx()->setDocRoot(achBuf);
    if (addVHost(pVHost) == 0)
    {
    }
    if (addVHost(pVHost2) == 0)
    {
    }
    if (mapListenerToVHost("*:3080", "*", "vhost1") == 0)
    {
    }
    if (mapListenerToVHost("*:3080", "test.com", "vhost2"))
    {
    }
    if (mapListenerToVHost("*:1443", "*", "vhost1") == 0)
    {
    }
    pVHost->getRootContext().inherit(NULL);
    pVHost->contextInherit();
    endConfig(0);

    lstrncpy(pEnd, "/conf/httpd.conf", sizeof(achBuf) - (pEnd - achBuf));
    return 0;
}


int HttpServer::test_main(const char *pArgv0)
{
    char achServerRoot[1024];
    printf("sizeof( HttpSession ) = %d, \n"
           "sizeof( HttpReq ) = %d, \n"
           "sizeof( HttpResp ) = %d, \n"
           "sizeof( NtwkIOLink ) = %d, \n"
           "sizeof( HttpVHost ) = %d, \n"
           "sizeof( LogTracker ) = %d, \n",
           (int)sizeof(HttpSession),
           (int)sizeof(HttpReq),
           (int)sizeof(HttpResp),
           (int)sizeof(NtwkIOLink),
           (int)sizeof(HttpVHost),
           (int)sizeof(LogSession));
    HttpFetch fetch;
    if (*pArgv0 != '/')
    {
        getcwd(achServerRoot, sizeof(achServerRoot) - 1);
        lstrncat(achServerRoot,"/", sizeof(achServerRoot));
    }
    else
        achServerRoot[0] = 0;
    lstrncat(achServerRoot, pArgv0, sizeof(achServerRoot));
    const char *pEnd = strrchr(achServerRoot, '/');
    if (pEnd)
    {
        --pEnd;
        while (pEnd > achServerRoot && *pEnd != '/')
            --pEnd;
        --pEnd;
        while (pEnd > achServerRoot && *pEnd != '/')
            --pEnd;
        ++pEnd;

        lstrncpy(&achServerRoot[pEnd - achServerRoot], "test/serverroot",
                 sizeof(achServerRoot) - (pEnd - achServerRoot));
    }
    MainServerConfig::getInstance().setServerRoot(achServerRoot);

//    if ( fetch.startReq( "http://www.litespeedtech.com/index.html", 0, "lst_index.html" ) == 0 )
//        fetch.process();
    ClientCache::initClientCache(1000);

    HttpServerConfig::getInstance().setGzipCompress(1);
    HttpdTest::runTest();
    SystemInfo::maxOpenFile(2048);
    m_impl->initSampleServer();
    /*
    int pid = fork();
    if ( pid == 0 )
    {
        char achCmd[4096];
        lstrncpy( achCmd, MainServerConfig::getInstance().getServerRoot(), sizeof(achCmd) );
        lstrncat( achCmd, "/wwwroot/systemTest", sizeof(achCmd) );
        sleep( 5 );
        printf( "run systemTest..." );
        system( achCmd );
        printf( "finished systemTest" );
        exit( 0 );
    }
    */
    start();
    releaseAll();
    return 0;
}
#endif


/////////////////////////////////////////////////////////////////
//      Interface function
/////////////////////////////////////////////////////////////////

HttpServer::HttpServer()
{
    m_impl = new HttpServerImpl(this);
}


HttpServer::~HttpServer()
{
//    if ( m_impl )
//        delete m_impl;
}


HttpListener *HttpServer::addListener(const char *pName, const char *pAddr)
{
    return m_impl->addListener(pName, pAddr);
}


int HttpServer::removeListener(const char *pName)
{
    return m_impl->removeListener(pName);
}


HttpListener *HttpServer::getListener(const char *pName) const
{
    return m_impl->getListener(pName);
}


int HttpServer::addVHost(HttpVHost *pVHost)
{
    return m_impl->addVHost(pVHost);
}


int HttpServer::removeVHost(const char *pName)
{
    return m_impl->removeVHost(pName);
}


HttpVHost *HttpServer::getVHost(const char *pName) const
{
    return m_impl->getVHost(pName);
}


void HttpServer::checkSuspendedVHostList(HttpVHost *pVHost)
{
    if (MainServerConfig::getInstance().getSuspendedVhosts().bfind(
            pVHost->getName()))
    {
        pVHost->enable(0);
        LS_DBG_L("VHost %s disabled.", pVHost->getName());
    }
}


int HttpServer::mapListenerToVHost(const char *pListener,
                                   const char *pKey,
                                   const char *pVHost)
{
    return m_impl->mapListenerToVHost(pListener, pKey, pVHost);
}

int HttpServer::mapListenerToVHost(const char *pListenerName,
                                   HttpVHost    *pVHost,
                                   const char *pDomains)
{
    HttpListener *pListener = getListener(pListenerName);
    return m_impl->mapListenerToVHost(pListener, pVHost, pDomains);
}

int HttpServer::mapListenerToVHost(HttpListener *pListener,
                                   HttpVHost    *pVHost, const char *pDomains)
{
    return m_impl->mapListenerToVHost(pListener, pVHost, pDomains);
}


int HttpServer::removeVHostFromListener(const char *pListener,
                                        const char *pVHost)
{
    return m_impl->removeVHostFromListener(pListener, pVHost);
}


int HttpServer::start()
{
    return m_impl->start();
}


int HttpServer::shutdown()
{
    return m_impl->shutdown();
}


AccessControl *HttpServer::getAccessCtrl() const
{
    return &(m_impl->m_accessCtrl);
}


int HttpServer::getVHostCounts() const
{
    return m_impl->m_vhosts.size();
}


HttpVHost *HttpServer::getVHost(int index) const
{
    return m_impl->m_vhosts.get(index);
}


void HttpServer::beginConfig()
{
    m_impl->beginConfig();
}


void HttpServer::endConfig(int error)
{
    m_impl->endConfig(error);
}


void HttpServer::onTimer()
{
    m_impl->onTimer();
}


const AutoStr2 *HttpServer::getErrDocUrl(int statusCode) const
{
    return m_impl->m_serverContext.getErrDocUrl(statusCode);
}


void HttpServer::releaseAll()
{
    if (m_impl)
    {
        m_impl->releaseAll();
        delete m_impl;
        m_impl = NULL;
    }
}


void HttpServer::setLogLevel(const char *pLevel)
{
    HttpLog::setLogLevel(pLevel);
}


int HttpServer::setErrorLogFile(const char *pFileName)
{
    return HttpLog::setErrorLogFile(pFileName);
}


int HttpServer::setAccessLogFile(const char *pFileName, int pipe)
{
    return HttpLog::setAccessLogFile(pFileName, pipe);
}


void HttpServer::setErrorLogRollingSize(off_t size, int keep_days)
{
    HttpLog::getErrorLogger()->getAppender()->setRollingSize(size);
    HttpLog::getErrorLogger()->getAppender()->setKeepDays(keep_days);
}


AccessLog *HttpServer::getAccessLog() const
{
    return HttpLog::getAccessLog();
}


void HttpServer::enableAioLogging()
{
#if defined(LS_AIO_USE_AIO)
    int i, count = getVHostCounts();
#if defined(LS_AIO_USE_KQ)
    if (SigEventDispatcher::aiokoIsLoaded())
    {
#endif
        HttpLogSource::setAioServerAccessLog(m_iAioAccessLog);
        HttpLogSource::setAioServerErrorLog(m_iAioErrorLog);
        if (m_iAioAccessLog == 1)
        {
            getAccessLog()->getAppender()->setAsync();
            LS_DBG_L("Enabling AIO for Server Access Logging!");
        }
        if (m_iAioErrorLog == 1)
        {
            HttpLog::getErrorLogger()->getAppender()->setAsync();
            LS_DBG_L("Enabling AIO for Server Error Logging!");
        }

        for (i = 0; i < count; ++i)
            getVHost(i)->enableAioLogging();
        return;
#if defined(LS_AIO_USE_KQ)
    }
#endif
#endif // defined(LS_AIO_USE_AIO)
    LS_NOTICE("AIO is not supported on this machine!");
}


const StringList *HttpServer::getIndexFileList() const
{
    return m_impl->m_serverContext.getIndexFileList();
}


void HttpServer::setSwapDir(const char *pDir)
{
    m_impl->setSwapDir(pDir);
}


const char *HttpServer::getSwapDir()
{
    return m_impl->getSwapDir();
}


int HttpServer::setupSwap()
{
    return m_impl->setupSwap();
}


void HttpServer::onVHostTimer()
{
    m_impl->m_vhosts.onTimer();
}


int HttpServer::enableVHost(const char *pVHostName, int enable)
{
    return m_impl->enableVHost(pVHostName, enable);
}


int HttpServer::isServerOk()
{
    return m_impl->isServerOk();
}


void HttpServer::offsetChroot()
{
    return m_impl->offsetChroot();
}


void HttpServer::setProcNo(int proc)
{
    HttpServerConfig::getInstance().setProcNo(proc);
    m_impl->setRTReportName(proc);
}


void HttpServer::setBlackBoard(char *pBuf)
{   m_impl->setBlackBoard(pBuf);      }


void HttpServer::generateStatusReport()
{   m_impl->generateStatusReport();     }


int HttpServer::updateVHost(const char *pName, HttpVHost *pVHost)
{
    return m_impl->updateVHost(pName, pVHost);
}


void HttpServer::passListeners()
{
    m_impl->m_listeners.passListeners();
}


void HttpServer::recoverListeners()
{
    m_impl->m_oldListeners.recvListeners();
}


int HttpServer::initMultiplexer(const char *pType)
{
    return m_impl->m_dispatcher.init(pType);
}


int HttpServer::reinitMultiplexer()
{
    return m_impl->reinitMultiplexer();
}


int HttpServer::initAdns()
{
    return m_impl->initAdns();
}


int HttpServer::initAioSendFile()
{
    return m_impl->initAioSendFile();
}


int HttpServer::initLscpd()
{
    return m_impl->initLscpd();
}

int HttpServer::authAdminReq(char *pAuth)
{
    return m_impl->authAdminReq(pAuth);
}


HttpContext &HttpServer::getServerContext()
{
    return m_impl->m_serverContext;
}


int HttpServer::configServerBasics(int reconfig, const XmlNode *pRoot)
{
    return m_impl->configServerBasics(reconfig, pRoot);
}


int HttpServer::configServer(int reconfig, XmlNode *pRoot)
{
    return m_impl->configServer(reconfig, pRoot);
}


int HttpServer::changeUserChroot()
{
    return m_impl->changeUserChroot();
}


//COMMENT: Not support reconfigVHost NOW.
// void HttpServer::reconfigVHost( char *pVHostName, XmlNode* pRoot )
// {
//     return m_impl->reconfigVHost( pVHostName, pRoot );
// }

void HttpServer::setServerRoot(const char *pRoot)
{
    return m_impl->setServerRoot(pRoot);
}


int HttpServer::initServer(XmlNode *pRoot, int &iReleaseXmlTree,
                           int reconfig)
{
    return m_impl->initServer(pRoot, iReleaseXmlTree, reconfig);
}


