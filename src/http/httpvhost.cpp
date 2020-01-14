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
#include <http/httpvhost.h>

#include <http/accesscache.h>
#include <http/accesslog.h>
#include <http/awstats.h>
#include <http/denieddir.h>
#include <http/handlerfactory.h>
#include <http/handlertype.h>
#include <http/hotlinkctrl.h>
#include <http/htauth.h>
#include <http/httplog.h>
#include <http/httpmime.h>
#include <http/httpserverconfig.h>
#include <http/httpstatuscode.h>
#include <http/recaptcha.h>
#include <http/rewriteengine.h>
#include <http/rewriterule.h>
#include <http/rewritemap.h>
#include <http/serverprocessconfig.h>
#include <http/userdir.h>
#include <http/staticfilecachedata.h>
#include <log4cxx/appender.h>
#include <log4cxx/layout.h>
#include <log4cxx/logger.h>
#include <log4cxx/logrotate.h>
#include <lsiapi/internal.h>
#include <lsiapi/lsiapi.h>
#include <lsiapi/modulemanager.h>
#include <lsr/ls_fileio.h>
#include <lsr/ls_strtool.h>
#include <main/configctx.h>
#include <main/httpserver.h>
#include <main/mainserverconfig.h>
#include <main/plainconf.h>
#include <sslpp/sslcontext.h>
#include <util/accesscontrol.h>
#include <util/datetime.h>
#include <util/daemonize.h>
#include <util/xmlnode.h>

#include <extensions/localworker.h>
#include <extensions/localworkerconfig.h>
#include <extensions/registry/extappregistry.h>
#include <extensions/registry/appconfig.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_URI_LEN  1024

RealmMap::RealmMap(int initSize)
    : _shmap(initSize)
{}


RealmMap::~RealmMap()
{
    release_objects();
}


const UserDir *RealmMap::find(const char *pScript) const
{
    iterator iter = _shmap::find(pScript);
    return (iter != end())
           ? iter.second()
           : NULL;
}


UserDir *RealmMap::get(const char *pFile, const char *pGroup)
{
    UserDir *pDir;
    HashDataCache *pGroupCache;
    iterator iter = begin();
    while (iter != end())
    {
        pDir = iter.second();
        pGroupCache = pDir->getGroupCache();
        if ((strcmp(pDir->getUserStoreURI(), pFile) == 0) &&
            ((!pGroup) ||
             ((pGroupCache) &&
              (strcmp(pDir->getGroupStoreURI(), pGroup) == 0))))
            return pDir;
        iter = next(iter);
    }
    return NULL;
}


UserDir *HttpVHost::getFileUserDir(
    const char *pName, const char *pFile, const char *pGroup)
{
    PlainFileUserDir *pDir = (PlainFileUserDir *)m_realmMap.get(pFile, pGroup);
    if (pDir)
        return pDir;
    if (pName)
    {
        pDir = (PlainFileUserDir *)getRealm(pName);
        if (pDir)
        {
            LS_ERROR("[%s] Realm %s exists.", m_sName.c_str(), pName);
            return NULL;
        }
    }
    pDir = new PlainFileUserDir();
    if (pDir)
    {
        char achName[100];
        if (!pName)
        {
            ls_snprintf(achName, 100, "AnonyRealm%d", rand());
            pName = achName;
        }
        pDir->setName(pName);
        m_realmMap.insert(pDir->getName(), pDir);
        pDir->setDataStore(pFile, pGroup);
    }
    return pDir;
}



void HttpVHost::offsetChroot(const char *pChroot, int len)
{
    char achTemp[512];
    const char *pOldName;
    if (m_pAccessLog[0])
    {
        pOldName = m_pAccessLog[0]->getAppender()->getName();
        if (strncmp(pChroot, pOldName, len) == 0)
        {
            lstrncpy(achTemp, pOldName + len, sizeof(achTemp));
            m_pAccessLog[0]->getAppender()->setName(achTemp);
        }
    }
    if (m_pLogger)
    {
        pOldName = m_pLogger->getAppender()->getName();
        if (strncmp(pChroot, pOldName, len) == 0)
        {
            m_pLogger->getAppender()->close();
            off_t rollSize = m_pLogger->getAppender()->getRollingSize();
            lstrncpy(achTemp, pOldName + len, sizeof(achTemp));
            setErrorLogFile(achTemp);
            m_pLogger->getAppender()->setRollingSize(rollSize);
        }
    }
}


int HttpVHost::setErrorLogFile(const char *pFileName)
{
    LOG4CXX_NS::Appender *appender
        = LOG4CXX_NS::Appender::getAppender(pFileName, "appender.ps");
    if (appender)
    {
        if (!m_pLogger)
            m_pLogger = LOG4CXX_NS::Logger::getLogger(m_sName.c_str());
        if (!m_pLogger)
            return LS_FAIL;
        LOG4CXX_NS::Layout *layout = LOG4CXX_NS::Layout::getLayout(
                                         ERROR_LOG_PATTERN, "layout.pattern");
        appender->setLayout(layout);
        m_pLogger->setAppender(appender);
        m_pLogger->setParent(LOG4CXX_NS::Logger::getRootLogger());
        return 0;
    }
    else
        return LS_FAIL;
}


int HttpVHost::setAccessLogFile(const char *pFileName, int pipe)
{
    int ret = 0;
    int i;
    m_lastAccessLog = NULL;

    if (! pFileName || !*pFileName)
        return -1;

    for (i = 0; i < MAX_ACCESS_LOG; ++i)
    {
        if (!m_pAccessLog[i])
        {
            m_pAccessLog[i] = new AccessLog();
            if (!m_pAccessLog[i])
                return -1;
            m_lastAccessLog = m_pAccessLog[i];
                break;
        }
    }
    if (!m_lastAccessLog)
        return -1;

    ret = m_lastAccessLog->init(pFileName, pipe);
    if (ret)
    {
        delete m_lastAccessLog;
        m_lastAccessLog = NULL;
        m_pAccessLog[i] = NULL;
    }
    return (m_lastAccessLog ? 0 : -1);
}


const char *HttpVHost::getAccessLogPath() const
{
    if (m_pAccessLog[0])
        return m_pAccessLog[0]->getLogPath();
    return NULL;
}


/*****************************************************************
 * HttpVHost funcitons.
 *****************************************************************/
HttpVHost::HttpVHost(const char *pHostName)
    : m_pLogger(NULL)
    , m_pBytesLog(NULL)
    , m_iMaxKeepAliveRequests(100)
    , m_iFeatures(VH_ENABLE | VH_SERVER_ENABLE |
                  VH_ENABLE_SCRIPT | LS_ALWAYS_FOLLOW |
                  VH_GZIP)
    , m_pAccessCache(NULL)
    , m_pHotlinkCtrl(NULL)
    , m_pAwstats(NULL)
    , m_sName(pHostName)
    , m_sAdminEmails("")
    , m_sAutoIndexURI("/_autoindex/default.php")
    , m_iMappingRef(0)
    , m_uid(500)
    , m_gid(500)
    , m_iRewriteLogLevel(0)
    , m_iGlobalMatchContext(1)
    , m_pRewriteMaps(NULL)
    , m_pSSLCtx(NULL)
    , m_pSSITagConfig(NULL)
    , m_pRecaptcha(NULL)
    , m_lastAccessLog(NULL)
    , m_PhpXmlNodeSSize(0)
{
    char achBuf[10] = "/";
    m_rootContext.set(achBuf, "/nON eXIST",
                      HandlerFactory::getInstance(0, NULL), 1);
    m_rootContext.allocateInternal();
    m_contexts.setRootContext(&m_rootContext);
    m_pUrlStxFileHash = new UrlStxFileHash(30, GHash::hfString,
                                           GHash::cmpString);

    m_pUrlIdHash = new UrlIdHash(64, GHash::hfString, GHash::cmpCiString);
    memset(m_pAccessLog, 0, sizeof(AccessLog *) * MAX_ACCESS_LOG);
}


HttpVHost::~HttpVHost()
{
    if (m_pLogger)
        m_pLogger->getAppender()->close();

    for (int i=0; i<MAX_ACCESS_LOG; ++i)
    {
        if (m_pAccessLog[i])
        {
            m_pAccessLog[i]->flush();
            delete m_pAccessLog[i];
            m_pAccessLog[i] = NULL;
        }
        else
            break;
    }
    if (m_pAccessCache)
        delete m_pAccessCache;
    if (m_pHotlinkCtrl)
        delete m_pHotlinkCtrl;
    if (m_pRewriteMaps)
        delete m_pRewriteMaps;
    if (m_pAwstats)
        delete m_pAwstats;
    if (m_pSSLCtx)
        delete m_pSSLCtx;
    m_pUrlStxFileHash->release_objects();
    delete m_pUrlStxFileHash;
    m_pUrlIdHash->release_objects();
    delete m_pUrlIdHash;
    LsiapiBridge::releaseModuleData(LSI_DATA_VHOST, &m_moduleData);
}


int HttpVHost::setDocRoot(const char *psRoot)
{
    assert(psRoot != NULL);
    assert(*(psRoot + strlen(psRoot) - 1) == '/');
    m_rootContext.setRoot(psRoot);
    return 0;
}


AccessControl *HttpVHost::getAccessCtrl() const
{   return (m_pAccessCache ? m_pAccessCache->getAccessCtrl() : NULL);   }



void HttpVHost::enableAccessCtrl()
{
    m_pAccessCache = new AccessCache(1543);
}


//const StringList* HttpVHost::getIndexFileList() const
//{
///*
//    //which lines are invalid? have fun!
//    const char * const pT[3] = { "A", "B", "C" };
//    const char * pTT[3] = { "A", "B", "C" };
//    const char ** pT1 = pT;
//    pT1 = pTT;
//    const char * const* pT2 = pT;
//    pT2 = pTT;
//    const char * const* const pT3 = pT;
//    const char * const* const pT4 = pTT;
//    const char * * const pT5 = pT;
//    const char * * const pT6 = pTT;
//    const char * pV1;
//    *pT1 = pV1;
//    *pT2 = pV1;
//    *pT3 = pV1;
//    pT3 = pT2;
//    pT2 = pT3;
//    pT2 = pT1;
//    pT1 = pT2;
//*/
//
//    return &(m_indexFileList);
//}




UserDir *HttpVHost::getRealm(const char *pRealm)
{
    return (UserDir *)m_realmMap.find(pRealm);
}


const UserDir *HttpVHost::getRealm(const char *pRealm) const
{
    return m_realmMap.find(pRealm);
}


void HttpVHost::setLogLevel(const char *pLevel)
{
    if (m_pLogger)
        m_pLogger->setLevel(pLevel);
}


void HttpVHost::setErrorLogRollingSize(off_t size, int keep_days)
{
    if (m_pLogger)
    {
        m_pLogger->getAppender()->setRollingSize(size);
        m_pLogger->getAppender()->setKeepDays(keep_days);
    }
}


void  HttpVHost::logAccess(HttpSession *pSession) const
{
    if (m_pAccessLog[0])
    {
        for (int i = 0; i < MAX_ACCESS_LOG; ++i)
        {
            if (m_pAccessLog[i])
                m_pAccessLog[i]->log(pSession);
        }
    }
    else
        HttpLog::logAccess(m_sName.c_str(), m_sName.len(), pSession);
}


void HttpVHost::setBytesLogFilePath(const char *pPath, off_t rollingSize)
{
    if (m_pBytesLog)
        m_pBytesLog->close();
    m_pBytesLog = LOG4CXX_NS::Appender::getAppender(pPath);
    m_pBytesLog->setRollingSize(rollingSize);
}


void HttpVHost::logBytes(long long bytes)
{
    char achBuf[80];
    int n = ls_snprintf(achBuf, 80, "%ld %lld .\n", DateTime::s_curTime,
                        bytes);
    m_pBytesLog->append(achBuf, n);
}


// const char * HttpVHost::getAdminEmails() const
// {   return m_sAdminEmails.c_str();  }


void HttpVHost::setAdminEmails(const char *pEmails)
{
    m_sAdminEmails = pEmails;
}


void HttpVHost::onTimer30Secs()
{
    urlStaticFileHashClean();
}


void HttpVHost::onTimer()
{
    using namespace LOG4CXX_NS;
    ServerProcessConfig &procConfig = ServerProcessConfig::getInstance();
    if (HttpServerConfig::getInstance().getProcNo())
    {
        if (m_pAccessLog[0])
        {
//             if (m_pAccessLog->reopenExist() == -1)
//             {
//                 LS_ERROR("[%s] Failed to open access log file %s.",
//                          m_sName.c_str(), m_pAccessLog->getLogPath());
//             }

            for (int i = 0; i < MAX_ACCESS_LOG; ++i)
            {
                if (m_pAccessLog[i])
                {
                    m_pAccessLog[i]->flush();
                    m_pAccessLog[i]->closeNonPiped();
                }
                else
                    break;
            }
        }
        if (m_pLogger)
            m_pLogger->getAppender()->reopenExist();
    }
    else
    {
        if (m_pAccessLog[0])
        {
            for (int i = 0; i < MAX_ACCESS_LOG; ++i)
            {
                if (m_pAccessLog[i] && !m_pAccessLog[i]->isPipedLog())
                {
                    if (LogRotate::testRolling(
                        m_pAccessLog[i]->getAppender(),
                        procConfig.getUid(),
                        procConfig.getGid()))
                    {
                        if (!i && m_pAwstats)
                            m_pAwstats->update(this);
                        else
                            LogRotate::testAndRoll(m_pAccessLog[i]->getAppender(),
                                                procConfig.getUid(),
                                                procConfig.getGid());
                    }
                    else if (!i && m_pAwstats)
                        m_pAwstats->updateIfNeed(time(NULL), this);
                }
            }
        }
        if (m_pBytesLog)
            LogRotate::testAndRoll(m_pBytesLog, procConfig.getUid(),
                                   procConfig.getGid());
        if (m_pLogger)
        {
            LogRotate::testAndRoll(m_pLogger->getAppender(),
                                   procConfig.getUid(), procConfig.getGid());
        }
    }
}


void HttpVHost::setChroot(const char *pRoot)
{
    m_sChroot = pRoot;
}


void HttpVHost::addRewriteMap(const char *pName, const char *pLocation)
{
    RewriteMap *pMap = new RewriteMap();
    if (!pMap)
    {
        ERR_NO_MEM("new RewriteMap()");
        return;
    }
    pMap->setName(pName);
    int ret = pMap->parseType_Source(pLocation);
    if (ret)
    {
        delete pMap;
        if (ret == 2)
            LS_ERROR("unknown or unsupported rewrite map type!");
        if (ret == -1)
            ERR_NO_MEM("parseType_Source()");
        return;
    }
    if (!m_pRewriteMaps)
    {
        m_pRewriteMaps = new RewriteMapList();
        if (!m_pRewriteMaps)
        {
            ERR_NO_MEM("new RewriteMapList()");
            delete pMap;
            return;
        }
    }
    m_pRewriteMaps->insert(pMap->getName(), pMap);
}


void HttpVHost::updateUGid(const char *pLogId, const char *pPath)
{
    struct stat st;
    char achBuf[8192];
    char *p = achBuf;
    ServerProcessConfig &procConfig = ServerProcessConfig::getInstance();
    if (getRootContext().getSetUidMode() != UID_DOCROOT)
    {
        setUid(procConfig.getUid());
        setGid(procConfig.getGid());
        return;
    }
    if (procConfig.getChroot() != NULL)
    {
        lstrncpy(p, procConfig.getChroot()->c_str(), sizeof(achBuf));
        p += procConfig.getChroot()->len();
    }
    memccpy(p, pPath, 0, &achBuf[8191] - p);
    int ret = ls_fio_stat(achBuf, &st);
    if (ret)
    {
        LS_ERROR("[%s] stat() failed on %s!",
                 pLogId, achBuf);
    }
    else
    {
        if (st.st_uid < procConfig.getUidMin())
        {
            LS_WARN("[%s] Uid of %s is smaller than minimum requirement"
                    " %d, use server uid!",
                    pLogId, achBuf, procConfig.getUidMin());
            st.st_uid = procConfig.getUid();
        }
        if (st.st_gid < procConfig.getGidMin())
        {
            st.st_gid = procConfig.getGid();
            LS_WARN("[%s] Gid of %s is smaller than minimum requirement"
                    " %d, use server gid!",
                    pLogId, achBuf, procConfig.getGidMin());
        }
        setUid(st.st_uid);
        setGid(st.st_gid);
    }
}


HttpContext *HttpVHost::setContext(HttpContext *pContext,
                                   const char *pUri, int type, const char *pLocation, const char *pHandler,
                                   int allowBrowse, int match)
{
    const HttpHandler *pHdlr = HandlerFactory::getInstance(type, pHandler);

    if (!pHdlr)
    {
        LS_ERROR("[%s] Can not find handler with type: %d, name: %s.",
                 TmpLogId::getLogId(), type, (pHandler) ? pHandler : "");
    }
    else if (type > HandlerType::HT_CGI)
        pHdlr = isHandlerAllowed(pHdlr, type, pHandler);
    if (!pHdlr)
    {
        allowBrowse = 0;
        pHdlr = HandlerFactory::getInstance(0, NULL);
    }
    if (pContext)
    {
        int ret =  pContext->set(pUri, pLocation, pHdlr, allowBrowse, match);
        if (ret)
        {
            if (ret != -1)
                LOG_ERR_CODE(ret);
            else
                LS_ERROR("[%s] Error setting context\n", TmpLogId::getLogId());
            delete pContext;
            pContext = NULL;
        }
    }
    return pContext;

}


HttpContext *HttpVHost::addContext(const char *pUri, int type,
                                   const char *pLocation, const char *pHandler, int allowBrowse)
{
    int ret;
    HttpContext *pContext = new HttpContext();
    if (pContext)
    {
        setContext(pContext, pUri, type, pLocation, pHandler, allowBrowse, 0);
        ret = addContext(pContext);
        if (ret != 0)
        {
            delete pContext;
            pContext = NULL;
        }
    }
    return pContext;

}


bool HttpVHost::dirMatch(HttpContext * &pContext, const char *pURI,
                         size_t iUriLen, AutoStr2 *missURI,
                         AutoStr2 *missLoc) const
{
    assert(iUriLen > 0 && pURI[0] == '/');
    int curContextURILen = pContext->getURILen();

    if (pContext->isNullContext())
    {
        pContext = (HttpContext *)pContext->getParent();
        return true;
    }

    bool ret = true;
    const char *p = pURI + curContextURILen;
    while(p < pURI + iUriLen)
    {
        if (*p == '/')
        {
            ret = false;
            missURI->setStr(pURI, p - pURI + 1);
            missLoc->setStr(pURI + curContextURILen, p - (pURI + curContextURILen) + 1);
            break;
        }
        ++p;
    }

    return ret;
}


HttpContext *HttpVHost::bestMatch(const char *pURI, size_t iUriLen)
{
    HttpContext *pContext = (HttpContext *)m_contexts.bestMatch(pURI, iUriLen);

    AutoStr2 missURI; //A while URI start with /
    AutoStr2 missLoc;  //A loc should be added to pContext location for the full path
    while (pContext && !dirMatch(pContext, pURI, iUriLen, &missURI, &missLoc))
    {
        char achRealPath[MAX_PATH_LEN];
        int locLen = pContext->getLocationLen();
        lstrncat2(achRealPath, sizeof(achRealPath),
                  pContext->getLocation(), missLoc.c_str());

        HttpContext *pContext0 = addContext(missURI.c_str(), HandlerType::HT_NULL,
                              achRealPath, NULL, 1);
        LS_DBG_H(ConfigCtx::getCurConfigCtx(), "Tried to add new context: "
                "URI %s location %s, result %p",
                missURI.c_str(), achRealPath, pContext0);
        if (pContext0 == NULL)
            break;

        if (access(achRealPath, F_OK) != 0)
        {
            pContext0->setConfigBit2(BIT2_NULL_CONTEXT, 1);

            LS_DBG_L(ConfigCtx::getCurConfigCtx(),
                     "path %s not accessible, added null context %p.",
                     achRealPath, pContext0);
            break;
        }

        pContext0->inherit(pContext);
        pContext = pContext0;
        if (enableAutoLoadHt())
        {
            //If have .htaccess in this DIR, load it
            lstrncat(achRealPath, ".htaccess", sizeof(achRealPath));
            pContext->configRewriteRule(NULL, NULL, achRealPath);
        }
    }

    return pContext;
}


const HttpContext *HttpVHost::matchLocation(const char *pURI,
        size_t iUriLen,
        int regex) const
{
    const HttpContext *pContext;
    if (!regex)
        pContext = m_contexts.matchLocation(pURI, iUriLen);
    else
    {
        char achTmp[] = "/";
        HttpContext *pOld = getContext(achTmp, strlen(achTmp));
        if (!pOld)
            return NULL;
        pContext = pOld->findMatchContext(pURI, 1);
    }

    return pContext;

}


HttpContext *HttpVHost::getContext(const char *pURI, size_t iUriLen,
                                   int regex) const
{
    if (!regex)
        return m_contexts.getContext(pURI, iUriLen);
    const HttpContext *pContext = m_contexts.getRootContext();
    pContext = pContext->findMatchContext(pURI);
    return (HttpContext *)pContext;

}


void HttpVHost::setSslContext(SslContext *pCtx)
{
    if (pCtx == m_pSSLCtx)
        return;
    if (m_pSSLCtx)
        delete m_pSSLCtx;
    m_pSSLCtx = pCtx;
}


HTAuth *HttpVHost::configAuthRealm(HttpContext *pContext,
                                   const char *pRealmName)
{
    HTAuth *pAuth = NULL;

    if ((pRealmName != NULL) && (*pRealmName))
    {
        UserDir *pUserDB = getRealm(pRealmName);

        if (!pUserDB)
        {
            LS_WARN(ConfigCtx::getCurConfigCtx(), "<realm> %s is not configured,"
                    " deny access to this context", pRealmName);
        }
        else
        {
            pAuth = new HTAuth();

            if (!pAuth)
                ERR_NO_MEM("new HTAuth()");
            else
            {
                pAuth->setName(pRealmName);
                pAuth->setUserDir(pUserDB);
                pContext->setHTAuth(pAuth);
                pContext->setConfigBit(BIT_AUTH, 1);
                pContext->setAuthRequired("valid-user");
            }
        }

        if (!pAuth)
            pContext->allowBrowse(false);
    }

    return pAuth;
}


int HttpVHost::configContextAuth(HttpContext *pContext,
                                 const XmlNode *pContextNode)
{
    const char *pRealmName = NULL;
    const XmlNode *pAuthNode = pContextNode->getChild("auth");
    HTAuth *pAuth = NULL;

    if (!pAuthNode)
        pAuthNode = pContextNode;

    pRealmName = pAuthNode->getChildValue("realm");
    pAuth = configAuthRealm(pContext, pRealmName);

    if (pAuth)
    {
        const char *pData = pAuthNode->getChildValue("required");

        if (!pData)
            pData = "valid-user";

        pContext->setAuthRequired(pData);

        pData = pAuthNode->getChildValue("authName");

        if (pData)
            pAuth->setName(pData);
    }

    return 0;
}


int HttpVHost::configBasics(const XmlNode *pVhConfNode, int iChrootLen)
{
    const char *pDocRoot = ConfigCtx::getCurConfigCtx()->getTag(pVhConfNode,
                           "docRoot");

    if (pDocRoot == NULL)
        return LS_FAIL;

    char achBuf[MAX_PATH_LEN];
    char *pPath = achBuf;

    if (ConfigCtx::getCurConfigCtx()->getValidPath(pPath, pDocRoot,
            "document root") != 0)
        return LS_FAIL;

    /**
     * Test symlink once but store link file since it may change.
     */
    char achBuf2[MAX_PATH_LEN];
    char *pPath2 = achBuf2;
    lstrncpy(pPath2, pPath, sizeof(achBuf2));
    if (ConfigCtx::getCurConfigCtx()->checkPath(pPath2, "document root",
            followSymLink()) == -1)
        return LS_FAIL;

    pPath += iChrootLen;
    ConfigCtx::getCurConfigCtx()->clearDocRoot();
    if (setDocRoot(pPath) != 0)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "failed to set document root - %s!",
                 pPath);
        return LS_FAIL;
    }
    ConfigCtx::getCurConfigCtx()->setDocRoot(pPath);
    //if ( ConfigCtx::getCurConfigCtx()->checkDeniedSubDirs( this, "/", pPath ) )
    if (checkDeniedSubDirs("/", pPath))
        return LS_FAIL;

    enable(ConfigCtx::getCurConfigCtx()->getLongValue(pVhConfNode, "enabled",
            0, 1, 1));
    enableGzip((HttpServerConfig::getInstance().getGzipCompress()) ?
               ConfigCtx::getCurConfigCtx()->getLongValue(pVhConfNode, "enableGzip", 0, 1,
                       1) : 0);

    enableBr((HttpServerConfig::getInstance().getBrCompress()) ?
               ConfigCtx::getCurConfigCtx()->getLongValue(pVhConfNode, "enableBr", 0, 1,
                       1) : 0);
    int val = ConfigCtx::getCurConfigCtx()->getLongValue(pVhConfNode, "enableIpGeo", -1, 1, -1);
    if (val == -1)
        val = HttpServer::getInstance().getServerContext().isGeoIpOn();
    m_rootContext.setGeoIP(val);
    m_rootContext.setIpToLoc(HttpServer::getInstance().getServerContext().isIpToLocOn());


    const char *pAdminEmails = pVhConfNode->getChildValue("adminEmails");
    if (!pAdminEmails)
        pAdminEmails = "";
    setAdminEmails(pAdminEmails);

    return 0;

}


int HttpVHost::configWebsocket(const XmlNode *pWebsocketNode)
{
    const char *pUri = ConfigCtx::getCurConfigCtx()->getTag(pWebsocketNode,
                       "uri", 1);
    const char *pAddress = ConfigCtx::getCurConfigCtx()->getTag(pWebsocketNode,
                           "address");
    char achVPath[MAX_PATH_LEN];
    char achRealPath[MAX_PATH_LEN];

    if (pUri == NULL || pAddress == NULL)
        return LS_FAIL;

    HttpContext *pContext = getContext(pUri, strlen(pUri), 0);

    if (pContext == NULL)
    {
        lstrncat2(achVPath, sizeof(achVPath), "$DOC_ROOT", pUri);
        ConfigCtx::getCurConfigCtx()->getAbsoluteFile(achRealPath, achVPath);
        pContext = addContext(pUri, HandlerType::HT_NULL, achRealPath, NULL, 1);

        if (pContext == NULL)
            return LS_FAIL;
    }

    GSockAddr gsockAddr;
    gsockAddr.parseAddr(pAddress);
    pContext->setWebSockAddr(gsockAddr);
    return 0;
}


int HttpVHost::configVHWebsocketList(const XmlNode *pVhConfNode)
{
    const XmlNode *p0 = pVhConfNode->getChild("websocketlist", 1);

    const XmlNodeList *pList = p0->getChildren("websocket");

    if (pList)
    {
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
            configWebsocket(*iter);
    }

    return 0;
}


int HttpVHost::configHotlinkCtrl(const XmlNode *pNode)
{
    int enabled = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                  "enableHotlinkCtrl", 0, 1, 0);

    if (!enabled)
        return 0;

    HotlinkCtrl *pCtrl = new HotlinkCtrl();

    if (!pCtrl)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "out of memory while creating HotlinkCtrl Object!");
        return LS_FAIL;
    }
    if (pCtrl->config(pNode) == -1)
    {
        delete pCtrl;
        return LS_FAIL;
    }
    setHotlinkCtrl(pCtrl);
    return 0;
}


int HttpVHost::configSecurity(const XmlNode *pVhConfNode)
{
    const XmlNode *p0 = pVhConfNode->getChild("security", 1);

    if (p0 != NULL)
    {
        ConfigCtx currentCtx("security");

        if (AccessControl::isAvailable(p0))
        {
            enableAccessCtrl();
            currentCtx.configSecurity(getAccessCtrl(), p0);
        }

        configRealmList(p0);
        const XmlNode *p1 = p0->getChild("hotlinkCtrl");

        if (p1)
            configHotlinkCtrl(p1);
    }

    return 0;
}


int HttpVHost::setAuthCache(const XmlNode *pNode,
                            HashDataCache *pAuth)
{
    int timeout = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "expire",
                  0, LONG_MAX, 60);
    int maxSize = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                  "maxCacheSize", 0, LONG_MAX, 1024);
    pAuth->setExpire(timeout);
    pAuth->setMaxSize(maxSize);
    return 0;
}


int HttpVHost::configRealm(const XmlNode *pRealmNode)
{
    int iChrootLen = 0;

    const char *pName = ConfigCtx::getCurConfigCtx()->getTag(pRealmNode,
                        "name", 1);
    if (pName == NULL)
        return LS_FAIL;
    ConfigCtx currentCtx("realm", pName);


    if (ServerProcessConfig::getInstance().getChroot() != NULL)
        iChrootLen = ServerProcessConfig::getInstance().getChroot()->len();

    const XmlNode *p2 = pRealmNode->getChild("userDB");

    if (p2 == NULL)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "missing <%s> in <%s>", "userDB",
                 "realm");
        return LS_FAIL;
    }

    const char *pFile = NULL, *pGroup = NULL, *pGroupFile = NULL;
    char achBufFile[MAX_PATH_LEN];
    char achBufGroup[MAX_PATH_LEN];

    pFile = p2->getChildValue("location");

    if ((!pFile) ||
        (ConfigCtx::getCurConfigCtx()->getValidFile(achBufFile, pFile,
                "user DB") != 0))
        return LS_FAIL;

    const XmlNode *pGroupNode = pRealmNode->getChild("groupDB");

    if (pGroupNode)
    {
        pGroup = pGroupNode->getChildValue("location");

        if (pGroup)
        {
            if (ConfigCtx::getCurConfigCtx()->getValidFile(achBufGroup, pGroup,
                    "group DB") != 0)
                return LS_FAIL;
            else
                pGroupFile = &achBufGroup[iChrootLen];
        }

    }

    UserDir *pUserDir =
        getFileUserDir(pName, &achBufFile[iChrootLen],
                       pGroupFile);

    if (!pUserDir)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(),
                 "Failed to create authentication DB.");
        return LS_FAIL;
    }

    setAuthCache(p2, pUserDir->getUserCache());

    if (pGroup)
        setAuthCache(pGroupNode, pUserDir->getGroupCache());

    return 0;
}


int HttpVHost::configRealmList(const XmlNode *pRoot)
{
    const XmlNode *pNode = pRoot->getChild("realmList", 1);
    if (pNode != NULL)
    {
        const XmlNodeList *pList = pNode->getChildren("realm");

        if (pList)
        {
            XmlNodeList::const_iterator iter;

            for (iter = pList->begin(); iter != pList->end(); ++iter)
            {
                const XmlNode *pRealmNode = *iter;
                configRealm(pRealmNode);
            }
        }
    }

    return 0;
}


void HttpVHost::configRewriteMap(const XmlNode *pNode)
{
    const XmlNodeList *pList = pNode->getChildren("map");

    if (pList)
    {
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
        {
            XmlNode *pN1 = *iter;
            const char *pName = pN1->getChildValue("name", 1);
            const char *pLocation = pN1->getChildValue("location");
            if (!pName || !pLocation)
                continue;

            char achBuf[1024];
            const char *p = strchr(pLocation, '$');

            if (p)
            {
                memmove(achBuf, pLocation, p - pLocation);

                if (ConfigCtx::getCurConfigCtx()->getAbsolute(&achBuf[ p - pLocation], p,
                        0) == 0)
                    pLocation = achBuf;
            }

            addRewriteMap(pName, pLocation);
        }
    }
}


int HttpVHost::configRewrite(const XmlNode *pNode)
{
    long long v = ConfigCtx::getCurConfigCtx()->getLongValue(
                                       pNode, "enable", 0, 1, 0);
    getRootContext().enableRewrite(v);
    //assert(getRootContext().isRewriteEnabled() == v);

    setRewriteLogLevel(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                       "logLevel", 0, 9, 0));

    int defVal = (HttpServerConfig::getInstance().getAutoLoadHtaccess());
    enableAutoLoadHt(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                                "autoLoadHtaccess", 0, 1, defVal));

    configRewriteMap(pNode);

    RewriteRule::setLogger(NULL, TmpLogId::getLogId());
    char *pRules = (char *) pNode->getChildValue("rules");
    if (pRules)
        getRootContext().configRewriteRule(getRewriteMaps(), pRules, "");


    return 0;
}


const XmlNode *HttpVHost::configIndex(const XmlNode *pVhConfNode,
                                      const StringList *pStrList)
{
    const XmlNode *pNode;
    pNode = pVhConfNode->getChild("index");
    const char *pUSTag = "indexFiles_useServer";

    if (!pNode)
        pNode = pVhConfNode;
    else
        pUSTag += 11;

    int useServer = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, pUSTag,
                    0, 2, 1);
    StringList *pIndexList = NULL;

    if (useServer != 1)
    {
        getRootContext().configDirIndex(pNode);

        if (useServer == 2)
        {
            pIndexList = getRootContext().getIndexFileList();

            if (pIndexList)
            {
                if (pStrList)
                    pIndexList->append(*pStrList);
            }
            else
                getRootContext().setConfigBit(BIT_DIRINDEX, 0);
        }
    }

    getRootContext().configAutoIndex(pNode);
    return pNode;
}


int HttpVHost::configIndexFile(const XmlNode *pVhConfNode,
                               const StringList *pStrList, const char *strIndexURI)
{
    const XmlNode *pNode = configIndex(pVhConfNode, pStrList);
    const char *pUSTag = pNode->getChildValue("autoIndexURI");
    if (pUSTag)
        if (*pUSTag != '/')
        {
            LS_ERROR(ConfigCtx::getCurConfigCtx(),
                     "Invalid AutoIndexURI, must be started with a '/'");
            pUSTag = NULL;
        }
    setAutoIndexURI((pUSTag) ? pUSTag : strIndexURI);

    char achURI[] = "/_autoindex/";
    HttpContext *pContext = configContext(achURI , HandlerType::HT_NULL,
                                          "$SERVER_ROOT/share/autoindex/", NULL, 1);
    if (pContext == NULL)
        return LS_FAIL;
    pContext->enableScript(1);
    return 0;
}


HttpContext *HttpVHost::addContext(int match, const char *pUri, int type,
                                   const char *pLocation, const char *pHandler, int allowBrowse)
{
    if (match)
    {
        char achTmp[] = "/";
        HttpContext *pOld;

        if (isGlobalMatchContext())
            pOld = &getRootContext();
        else
            pOld = getContext(achTmp, strlen(achTmp));

        if (!pOld)
            return NULL;

        HttpContext *pContext = new HttpContext();
        setContext(pContext, pUri, type, pLocation, pHandler, allowBrowse, match);
        pOld->addMatchContext(pContext);
        return pContext;
    }
    else
    {
        setGlobalMatchContext(1);
        HttpContext *pOld = getContext(pUri, strlen(pUri));

        if (pOld)
        {
            if (strcmp(pUri, "/") == 0)
            {
                pOld = setContext(pOld, pUri, type, pLocation, pHandler,
                                  allowBrowse, match);
                return pOld;
            }

            return NULL;
        }

        return addContext(pUri, type, pLocation, pHandler, allowBrowse);
    }
}


HttpContext *HttpVHost::configContext(const char *pUri, int type,
                                      const char *pLocation,
                                      const char *pHandler, int allowBrowse)
{
    char achRealPath[4096];
    int match = (strncasecmp(pUri, "exp:", 4) == 0);
    AutoStr2 *pProcChroot;
    assert(type != HandlerType::HT_REDIRECT);

    if (type >= HandlerType::HT_CGI)
        allowBrowse = 1;

    if (type < HandlerType::HT_FASTCGI && type != HandlerType::HT_MODULE)
    {
        if (allowBrowse)
        {
            if (pLocation == NULL)
            {
                if ((type == HandlerType::HT_REDIRECT) ||
                    (type == HandlerType::HT_RAILS) || match)
                {
                    ConfigCtx::getCurConfigCtx()->logErrorMissingTag("location");
                    return NULL;
                }
                else
                    pLocation = pUri + 1;
            }

            int ret = 0;

            switch (*pLocation)
            {
            case '$':

                if ((match) && (isdigit(* (pLocation + 1))))
                {
                    lstrncpy(achRealPath, pLocation, sizeof(achRealPath));
                    break;
                }

            //fall through
            default:
                ret = ConfigCtx::getCurConfigCtx()->getAbsoluteFile(achRealPath,
                        pLocation);
                break;
            }

            if (ret)
            {
                ConfigCtx::getCurConfigCtx()->logErrorPath("context location",  pLocation);
                return NULL;
            }

            if (!match)
            {
                int PathLen = strlen(achRealPath);
                if (access(achRealPath, F_OK) != 0)
                {
                    LS_ERROR(ConfigCtx::getCurConfigCtx(), "path is not accessible: %s",
                             achRealPath);
                    return NULL;
                }

                if (PathLen > 512)
                {
                    LS_ERROR(ConfigCtx::getCurConfigCtx(), "path is too long: %s",
                             achRealPath);
                    return NULL;
                }

                pLocation = achRealPath;
                pProcChroot = ServerProcessConfig::getInstance().getChroot();
                if (pProcChroot != NULL)
                    pLocation += pProcChroot->len();

                if (allowBrowse)
                {
                    //ret = ConfigCtx::getCurConfigCtx()->checkDeniedSubDirs( this, pUri, pLocation );
                    ret = checkDeniedSubDirs(pUri, pLocation);

                    if (ret)
                        return NULL;
                }
            }
            else
                pLocation = achRealPath;
        }
    }
    else
    {
        if (pHandler == NULL)
        {
            ConfigCtx::getCurConfigCtx()->logErrorMissingTag("handler");
            return NULL;
        }
    }

    return addContext(match, pUri, type, pLocation, pHandler, allowBrowse);
}

static int buildUdsSocket(char *buf, int max_len, const char *pVhostName,
                          const char *pAppName)
{
    char *p = buf;
    int vhost_len = strlen(pVhostName);
    int app_name_len = strlen(pAppName);
    if (vhost_len + app_name_len > max_len - 24)
        vhost_len = max_len - 24 - app_name_len;
    memcpy(p, "uds://tmp/lshttpd/", 18);
    p += 18;
    if (vhost_len < 0)
        app_name_len += vhost_len;
    else
    {
        memcpy(p, pVhostName, vhost_len);
        p += vhost_len;
    }
    *p++ = ':';
    memcpy(p, pAppName, app_name_len);
    p += app_name_len;
    memcpy(p, ".sock", 6);
    p += 5;
    for (char *p1 = &buf[18]; *p1; ++p1)
    {
        if (*p1 == '/')
            *p1 = '_';
    }
    return p - buf;
}


void addHomeEnv(HttpVHost *pVHost, Env *pEnv)
{
    if (pVHost->getVhRoot()->c_str())
    {
        char buf[4096];
        // ~/nodevenv/.../<ver>/bin/activate needs $HOME
        snprintf(buf, sizeof(buf), "HOME=%s",
                pVHost->getVhRoot()->c_str());
        pEnv->add(buf);
    }
}


void HttpVHost::setDefaultConfig(LocalWorkerConfig &config,
                                 const char *pBinPath,
                                 int maxConns,
                                 int maxIdle,
                                 LocalWorkerConfig *pDefault)
{
    config.setVHost(this);
    config.setAppPath(pBinPath);
    config.setBackLog(pDefault->getBackLog());
    config.setSelfManaged(1);
    config.setStartByServer(EXTAPP_AUTOSTART_ASYNC_CGID);
    config.setMaxConns(maxConns);

    if (config.isDetached())
    {
        config.setKeepAliveTimeout(0);
        config.setPersistConn(0);
    }
    else
    {
        config.setKeepAliveTimeout(pDefault->getKeepAliveTimeout());
        config.setPersistConn(1);
    }
    config.setTimeout(pDefault->getTimeout());
    config.setRetryTimeout(pDefault->getRetryTimeout());
    config.setBuffering(pDefault->getBuffering());
    config.setPriority(pDefault->getPriority());
    config.setMaxIdleTime((maxIdle > 0) ? maxIdle : pDefault->getMaxIdleTime());
    config.setRLimits(pDefault->getRLimits());

    config.setInstances(1);
}

static void setDetachedAppEnv(Env *pEnv, int max_idle)
{
    char achBuf[200];
    snprintf(achBuf, 200, "LSAPI_PPID_NO_CHECK=1");
    pEnv->add(achBuf);
    snprintf(achBuf, 200, "LSAPI_PGRP_MAX_IDLE=%d", max_idle);
    pEnv->add(achBuf);
}




int HttpVHost::configRailsRunner(char *pRunnerCmd, int cmdLen,
                                 const char *pRubyBin)
{
    const char *rubyBin[2] = { "/usr/local/bin/ruby", "/usr/bin/ruby" };
    if ((pRubyBin) && (access(pRubyBin, X_OK) != 0))
    {
        LS_NOTICE("[%s] Ruby path is not vaild: %s",
                  TmpLogId::getLogId(), pRubyBin);
        pRubyBin = NULL;
    }

    if (!pRubyBin)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (access(rubyBin[i], X_OK) == 0)
            {
                pRubyBin = rubyBin[i];
                break;
            }
        }
    }

    if (!pRubyBin)
    {
        LS_NOTICE("[%s] Cannot find ruby interpreter, Rails easy configuration is turned off",
                  TmpLogId::getLogId());
        return LS_FAIL;
    }

    snprintf(pRunnerCmd, cmdLen, "%s %sfcgi-bin/RackRunner.rb", pRubyBin,
             MainServerConfig::getInstance().getServerRoot());
    return 0;
}



LocalWorker *HttpVHost::addRailsApp(const char *pAppName, const char *appPath,
                                    int maxConns, const char *pRailsEnv,
                                    int maxIdle, const Env *pEnv,
                                    int runOnStart, const char *pBinPath)
{
    char achFileName[MAX_PATH_LEN];
    char achRunnerCmd[MAX_PATH_LEN];
    const char *pRailsRunner = NULL;

    LocalWorkerConfig* pAppDefault = (LocalWorkerConfig*)AppConfig::s_rubyAppConfig.getpAppDefault();
    if (!pAppDefault)
        return NULL;

    int pathLen = snprintf(achFileName, MAX_PATH_LEN, "%s", appPath);
    if (pathLen > MAX_PATH_LEN - 20)
    {
        LS_ERROR("[%s] path to Rack application is too long!",
                 TmpLogId::getLogId());
        return NULL;
    }

    if (access(achFileName, F_OK) != 0)
    {
        LS_WARN("[%s] path to Rack application is invalid!",
                 TmpLogId::getLogId());
        return NULL;
    }

    if (!pBinPath || *pBinPath == 0x00)
        pBinPath = AppConfig::s_rubyAppConfig.s_binPath.c_str();

    if (pBinPath
        && configRailsRunner(achRunnerCmd, sizeof(achRunnerCmd), pBinPath) != -1)
    {
        pRailsRunner = achRunnerCmd;
    }


    if (!pRailsRunner)
    {
        LS_ERROR("[%s] 'Ruby path' is not set properly, Rack context is disabled!",
                 TmpLogId::getLogId());
        return NULL;
    }

    LocalWorker *pWorker;
    char achAppName[1024];
    char achName[MAX_PATH_LEN];
    snprintf(achAppName, 1024, "Rack:%s:%s", getName(), pAppName);

    pWorker = (LocalWorker *)ExtAppRegistry::getApp(EA_LSAPI, achAppName);
    if (pWorker)
        return pWorker;
    pWorker = (LocalWorker *)ExtAppRegistry::addApp(EA_LSAPI, achAppName);

    lstrncpy(&achFileName[pathLen], "tmp/sockets", sizeof(achFileName) - pathLen);
    //if ( access( achFileName, W_OK ) == -1 )
    {
        buildUdsSocket(achName, 108 + 5, getName(), pAppName);
    }
    //else
    //{
    //    snprintf( achName, MAX_PATH_LEN, "uds://%s/RailsRunner.sock",
    //            &achFileName[m_sChroot.len()] );
    //}
    pWorker->setURL(achName);


    LocalWorkerConfig &config = pWorker->getConfig();
    setDefaultConfig(config, pRailsRunner, maxConns, maxIdle, pAppDefault);

    config.clearEnv();
    if (pEnv)
        config.getEnv()->add(pEnv);

    {
        achFileName[pathLen] = 0;
        snprintf(achName, MAX_PATH_LEN, "RAILS_ROOT=%s",
                &achFileName[m_sChroot.len()]);
        config.addEnv(achName);
        snprintf(achName, MAX_PATH_LEN, "LS_CWD=%s",
                &achFileName[m_sChroot.len()]);
        config.addEnv(achName);
        if (pRailsEnv)
        {
            snprintf(achName, MAX_PATH_LEN, "RAILS_ENV=%s", pRailsEnv);
            config.addEnv(achName);
        }
    }
    config.addEnv("RUBYOPT=rubygems");
    addHomeEnv(this, config.getEnv());

    if (maxConns > 1)
    {
        snprintf(achName, MAX_PATH_LEN, "LSAPI_CHILDREN=%d", maxConns);
        config.addEnv(achName);
        snprintf(achName, MAX_PATH_LEN, "LSAPI_KEEP_LISTEN=2");
        config.addEnv(achName);
    }
    else
        config.setSelfManaged(0);
    if (config.isDetached())
    {
        if (maxIdle < DETACH_MODE_MIN_MAX_IDLE)
            maxIdle = DETACH_MODE_MIN_MAX_IDLE;
        setDetachedAppEnv(config.getEnv(), maxIdle);
    }
    else if (maxIdle != INT_MAX && maxIdle > 0)
    {
        snprintf(achName, MAX_PATH_LEN, "LSAPI_MAX_IDLE=%d", maxIdle);
        config.addEnv(achName);
        snprintf(achName, MAX_PATH_LEN, "LSAPI_PGRP_MAX_IDLE=%d", maxIdle);
        config.addEnv(achName);
    }

    achFileName[pathLen] = 0;
    snprintf(achName, MAX_PATH_LEN, "LSAPI_STDERR_LOG=%s/log/stderr.log",
             &achFileName[m_sChroot.len()]);
    config.addEnv(achName);

    config.getEnv()->add(pAppDefault->getEnv());
    config.addEnv(NULL);
    config.setUGid(getUid(), getGid());
    config.setRunOnStartUp(runOnStart);

    snprintf(achName, MAX_PATH_LEN, "%stmp/restart.txt", achFileName);
    pWorker->setRestartMarker(achName, 0);

    return pWorker;
}


HttpContext *HttpVHost::addRailsContext(const char *pURI, const char *pLocation,
                                        ExtWorker *pWorker,
                                        HttpContext *pOldCtx)
{
    char achURI[MAX_URI_LEN];
    int uriLen = strlen(pURI);

    lstrncpy(achURI, pURI, sizeof(achURI));
    if (achURI[uriLen - 1] != '/')
    {
        achURI[uriLen++] = '/';
        achURI[uriLen] = 0;
    }
    if (uriLen > MAX_URI_LEN - 100)
    {
        LS_ERROR("[%s] context URI is too long!", TmpLogId::getLogId());
        return NULL;
    }
    char achBuf[MAX_PATH_LEN];
    snprintf(achBuf, MAX_PATH_LEN, "%spublic/", pLocation);
    HttpContext *pContext = NULL;

    if (pOldCtx)
    {
        pContext = pOldCtx;
        pContext->setRoot(achBuf);
    }
    else
        pContext = addContext(achURI, HandlerType::HT_NULL,
                                 achBuf, NULL, 1);
    if (!pContext)
        return NULL;

    lstrncpy(&achURI[uriLen], "dispatch.rb", sizeof(achURI) - uriLen);
    HttpContext *pDispatch = addContext(achURI,
                                        HandlerType::HT_NULL,
                                        NULL, NULL, 1);
    if (pDispatch)
        pDispatch->setHandler(pWorker);
    /*
        strcpy( &achURI[uriLen], "dispatch.cgi" );
        pDispatch = addContext( pVHost, 0, achURI, HandlerType::HT_NULL,
                                NULL, NULL, 1 );
        if ( pDispatch )
            pDispatch->setHandler( pWorker );
        strcpy( &achURI[uriLen], "dispatch.fcgi" );
        pDispatch = addContext( pVHost, 0, achURI, HandlerType::HT_NULL,
                                NULL, NULL, 1 );
        if ( pDispatch )
            pDispatch->setHandler( pWorker );
    */
    lstrncpy(&achURI[uriLen], "dispatch.lsapi", sizeof(achURI) - uriLen);
    pDispatch = addContext(achURI, HandlerType::HT_NULL,
                           NULL, NULL, 1);
    if (pDispatch)
    {
        pDispatch->setHandler(pWorker);
        pDispatch->setRailsContext();
        pDispatch->setParent(pContext);
    }
    pContext->setAutoIndex(0);
    pContext->setAutoIndexOff(1);
    pContext->setCustomErrUrls("404", achURI);
    pContext->setRailsContext();
    return pContext;

}


HttpContext *HttpVHost::configRailsContext(const char *contextUri,
                                           const char *appPath,
                                           int maxConns, const char *pRailsEnv,
                                           int maxIdle, const Env *pEnv,
                                           const char *pBinPath,
                                           HttpContext *pOldCtx)
{
    char achFileName[MAX_PATH_LEN];

    LocalWorkerConfig* pAppDefault = (LocalWorkerConfig*)AppConfig::s_rubyAppConfig.getpAppDefault();
    if (!pAppDefault)
        return NULL;

    int ret = ConfigCtx::getCurConfigCtx()->getAbsolutePath(achFileName, appPath);
    if (ret == -1)
    {
        LS_WARN("[%s] path to Rails application is invalid!",
                 TmpLogId::getLogId());
        return NULL;
    }


    LocalWorker *pWorker = addRailsApp(contextUri, achFileName,
                                       maxConns, pRailsEnv, maxIdle, pEnv,
                                       pAppDefault->getRunOnStartUp(),
                                       pBinPath) ;
    if (!pWorker)
        return NULL;
    return addRailsContext(contextUri, achFileName, pWorker, pOldCtx);
}


LocalWorker *HttpVHost::addPythonApp(const char *pAppName, const char *appPath,
                                     const char *pStartupFile, int maxConns,
                                     const char *pPythonEnv,
                                     int maxIdle, const Env *pEnv,
                                     int runOnStart, const char *pBinPath)
{
    int iChrootlen = m_sChroot.len();
    char achFileName[MAX_PATH_LEN];

    LocalWorkerConfig* pAppDefault = (LocalWorkerConfig*)AppConfig::s_wsgiAppConfig.getpAppDefault();
    if (!pAppDefault)
        return NULL;

    int pathLen = snprintf(achFileName, MAX_PATH_LEN, "%s", appPath);

    if (pathLen > MAX_PATH_LEN - 20)
    {
        LS_ERROR("[%s] path to Python application is too long!",
                 TmpLogId::getLogId());
        return NULL;
    }

    if (access(achFileName, F_OK) != 0)
    {
        LS_WARN("[%s] Path for Python application is invalid: %s",
                 TmpLogId::getLogId(),  achFileName);
        return NULL;
    }

    LocalWorker *pWorker;
    char achAppName[1024];
    char achName[MAX_PATH_LEN];
    snprintf(achAppName, 1024, "wsgi:%s:%s", getName(), pAppName);

    pWorker = (LocalWorker *)ExtAppRegistry::getApp(EA_LSAPI, achAppName);
    if (pWorker)
        return pWorker;
    pWorker = (LocalWorker *) ExtAppRegistry::addApp(EA_LSAPI, achAppName);
    buildUdsSocket(achName, 108 + 5, getName(), pAppName);
    pWorker->setURL(achName);

    if (!pBinPath || *pBinPath == 0x00)
        pBinPath = AppConfig::s_wsgiAppConfig.s_binPath.c_str();

    achFileName[pathLen] = 0;
    if (pStartupFile)
    {
        if (*pStartupFile == '/')
            snprintf(achName, MAX_PATH_LEN, "%s -m %s", pBinPath, pStartupFile);
        else
            snprintf(achName, MAX_PATH_LEN, "%s -m %s%s", pBinPath,
                     achFileName, pStartupFile);
        pBinPath = achName;
    }

    LocalWorkerConfig &config = pWorker->getConfig();
    setDefaultConfig(config, pBinPath, maxConns, maxIdle, pAppDefault);

    config.clearEnv();
    if (pEnv)
        config.getEnv()->add(pEnv);

    snprintf(achName, MAX_PATH_LEN, "WSGI_ROOT=%s", &achFileName[iChrootlen]);
    config.addEnv(achName);

    snprintf(achName, MAX_PATH_LEN, "PYTHONPATH=.:%s", &achFileName[iChrootlen]);
    config.addEnv(achName);

    snprintf(achName, MAX_PATH_LEN, "LSAPI_STDERR_LOG=%sstderr.log",
             &achFileName[m_sChroot.len()]);
    config.addEnv(achName);
    addHomeEnv(this, config.getEnv());

    if (pPythonEnv)
    {
        snprintf(achName, MAX_PATH_LEN, "WSGI_ENV=%s", pPythonEnv);
        config.addEnv(achName);
    }
    if (maxConns > 1)
    {
        snprintf(achName, MAX_PATH_LEN, "LSAPI_CHILDREN=%d", maxConns);
        config.addEnv(achName);
        snprintf(achName, MAX_PATH_LEN, "LSAPI_KEEP_LISTEN=2");
        config.addEnv(achName);
    }
    else
        config.setSelfManaged(0);

    if (config.isDetached())
    {
        if (maxIdle < DETACH_MODE_MIN_MAX_IDLE)
            maxIdle = DETACH_MODE_MIN_MAX_IDLE;
        setDetachedAppEnv(config.getEnv(), maxIdle);
    }
    else if (maxIdle != INT_MAX && maxIdle > 0)
    {
        snprintf(achName, MAX_PATH_LEN, "LSAPI_MAX_IDLE=%d", maxIdle);
        config.addEnv(achName);
        snprintf(achName, MAX_PATH_LEN, "LSAPI_PGRP_MAX_IDLE=%d", maxIdle);
        config.addEnv(achName);
    }

    config.getEnv()->add(pAppDefault->getEnv());
    config.addEnv(NULL);
    config.setUGid(getUid(), getGid());
    config.setRunOnStartUp(runOnStart);

    snprintf(achName, MAX_PATH_LEN, "%stmp/restart.txt", achFileName);
    pWorker->setRestartMarker(achName, 0);

    return pWorker;
}

HttpContext *HttpVHost::addPythonContext(const char *pURI,
                                         const char *pLocation,
                                         const char *pStartupFile,
                                         ExtWorker *pWorker,
                                         HttpContext *pOldCtx)
{
    char achURI[MAX_URI_LEN];
    int uriLen = strlen(pURI);

    lstrncpy(achURI, pURI, sizeof(achURI));

    if (achURI[uriLen - 1] != '/')
    {
        achURI[uriLen++] = '/';
        achURI[uriLen] = 0;
    }

    if (uriLen > MAX_URI_LEN - 100)
    {
        LS_ERROR("[%s] context URI is too long!", TmpLogId::getLogId());
        return NULL;
    }

    char achBuf[MAX_PATH_LEN];
    snprintf(achBuf, MAX_PATH_LEN, "%spublic/", pLocation);
    HttpContext *pContext;
    if (pOldCtx)
    {
        pContext = pOldCtx;
        pContext->setRoot(achBuf);
    }
    else
        pContext = addContext(achURI, HandlerType::HT_NULL,
                                 achBuf, NULL, 1);

    if (!pContext)
        return NULL;

    //FIXME any reason for the below code
    pContext->setWebSockAddr(pWorker->getConfigPointer()->getServerAddr());

    char appURL[MAX_URI_LEN];
    if (!pStartupFile || *pStartupFile == '\0')
        pStartupFile = "index.wsgi";
    memccpy(&achURI[uriLen], pStartupFile, 0, MAX_URI_LEN - uriLen);
    snprintf(appURL, sizeof(appURL), "%s%s", pLocation, pStartupFile);

    HttpContext *pDispatch;

    pDispatch = addContext(0, achURI, HandlerType::HT_NULL, //NULL TYPE is static
                           appURL, NULL, 1);
    pDispatch->setHandler(pWorker);
    pDispatch->setPythonContext();
    pDispatch->setParent(pContext);

    pContext->setAutoIndex(0);
    pContext->setAutoIndexOff(1);
    pContext->setCustomErrUrls("404", achURI);

    pContext->setPythonContext();
    return pContext;
}


int HttpVHost::configNodeJsStarter(char *pRunnerCmd, int cmdLen,
                                   const char *pBinPath)
{
    const char *defaultBin[2] = { "/usr/local/bin/node", "/usr/bin/node" };
    if (!pBinPath)
    {
        for (int i = 0; i < 2; ++i)
        {
            if (access(defaultBin[i], X_OK) == 0)
            {
                pBinPath = defaultBin[i];
                break;
            }
        }
    }
    if (!pBinPath)
    {
        LS_NOTICE("[%s] Cannot find node interpreter, node easy configuration "
                    "is turned off", TmpLogId::getLogId());
        return LS_FAIL;
    }
    snprintf(pRunnerCmd, cmdLen, "%s %sfcgi-bin/lsnode.js", pBinPath,
             MainServerConfig::getInstance().getServerRoot());
    return 0;
}


LocalWorker *HttpVHost::addNodejsApp(const char *pAppName,
                                     const char *appPath,
                                     const char *pStartupFile, int maxConns,
                                     const char *pRunModeEnv, int maxIdle,
                                     const Env *pEnv, int runOnStart,
                                     const char *pBinPath)
{
    int iChrootlen = m_sChroot.len();
    char achFileName[MAX_PATH_LEN];
    char achRunnerCmd[MAX_PATH_LEN];
    const char *pNodejsStarter = NULL;

    LocalWorkerConfig* pAppDefault = (LocalWorkerConfig*)AppConfig::s_nodeAppConfig.getpAppDefault();
    if (!pAppDefault)
        return NULL;

    int pathLen = snprintf(achFileName, MAX_PATH_LEN, "%s", appPath);
    if (pathLen > MAX_PATH_LEN - 20)
    {
        LS_ERROR("[%s] path to NodeJS application is too long!", TmpLogId::getLogId());
        return NULL;
    }

    if (access(achFileName, F_OK) != 0)
    {
        LS_ERROR("[%s] path to NodeJS application %s not exist!",
                 TmpLogId::getLogId(), achFileName);
        return NULL;
    }

    if (!pBinPath || *pBinPath == 0x00)
        pBinPath = AppConfig::s_nodeAppConfig.s_binPath.c_str();

    if (pBinPath
        && configNodeJsStarter(achRunnerCmd, sizeof(achRunnerCmd), pBinPath) != -1)
    {
        pNodejsStarter = achRunnerCmd;
    }

    if (!pNodejsStarter)
    {
        LS_ERROR("[%s] 'Node path' is not set properly, NodeJS context is disabled!",
                 TmpLogId::getLogId());
        return NULL;
    }

    LocalWorker *pWorker;
    char achAppName[1024];
    char achName[MAX_PATH_LEN];
    snprintf(achAppName, 1024, "Node:%s:%s", this->getName(), pAppName);
    pWorker = (LocalWorker *)ExtAppRegistry::getApp(EA_PROXY, achAppName);
    if (pWorker)
        return pWorker;
    pWorker = (LocalWorker *)ExtAppRegistry::addApp(EA_PROXY, achAppName);

    lstrncpy(&achFileName[pathLen], "tmp/sockets", sizeof(achFileName) - pathLen);
    //if ( access( achFileName, W_OK ) == -1 )
    {
        buildUdsSocket(achName, 108 + 5, this->getName(), pAppName);
    }
    //else
    //{
    //    snprintf( achName, MAX_PATH_LEN, "uds://%s/RailsRunner.sock",
    //            &achFileName[m_sChroot.len()] );
    //}
    pWorker->setURL(achName);


    LocalWorkerConfig &config = pWorker->getConfig();
    setDefaultConfig(config, pNodejsStarter, maxConns, maxIdle, pAppDefault);

    config.clearEnv();
    if (pEnv)
        config.getEnv()->add(pEnv);
    achFileName[pathLen] = 0;
    snprintf(achName, MAX_PATH_LEN, "LSNODE_ROOT=%s",
            &achFileName[iChrootlen]);
    config.addEnv(achName);
    addHomeEnv(this, config.getEnv());

    if (pStartupFile)
    {
        snprintf(achName, MAX_PATH_LEN, "LSNODE_STARTUP_FILE=%s", pStartupFile);
        config.addEnv(achName);
    }

    snprintf(achName, MAX_PATH_LEN, "LSNODE_SOCKET=%s",
             config.getServerAddr().getUnix());
    config.addEnv(achName);

    if (pRunModeEnv)
    {
        snprintf(achName, MAX_PATH_LEN, "NODE_ENV=%s", pRunModeEnv);
        config.addEnv(achName);
    }

    if (config.isDetached())
    {
        if (maxIdle < DETACH_MODE_MIN_MAX_IDLE)
            maxIdle = DETACH_MODE_MIN_MAX_IDLE;
        setDetachedAppEnv(config.getEnv(), maxIdle);
    }

//     achFileName[pathLen] = 0;
//     snprintf(achName, MAX_PATH_LEN, "LSAPI_STDERR_LOG=%s/log/stderr.log",
//              &achFileName[iChrootlen]);
//    config.addEnv(achName);

    config.getEnv()->add(pAppDefault->getEnv());
    config.addEnv(NULL);
    config.setUGid(getUid(), getGid());
    config.setRunOnStartUp(runOnStart);

    snprintf(achName, MAX_PATH_LEN, "%stmp/restart.txt", achFileName);
    pWorker->setRestartMarker(achName, 0);

    return pWorker;
}


#define PYWSGIENTRY   "wsgi.py"
#define NODEJSENTRY   "app.js"

static const char *getDefaultStartupFile(int type)
{
    static const char *s_pDefaults[3] = {
        "config.ru", PYWSGIENTRY, NODEJSENTRY
    };
    type -= HandlerType::HT_RAILS;
    if (type >=0 && type < 3)
        return s_pDefaults[type];
    return NULL;
}



HttpContext *HttpVHost::addNodejsContext(const char *pURI,
                                         const char *pLocation,
                                         const char *pStartupFile,
                                         ExtWorker *pWorker,
                                         HttpContext *pOldCtx)
{
    char achURI[MAX_URI_LEN];
    int uriLen = strlen(pURI);

    lstrncpy(achURI, pURI, sizeof(achURI));

    if (achURI[uriLen - 1] != '/')
    {
        achURI[uriLen++] = '/';
        achURI[uriLen] = 0;
    }

    if (uriLen > MAX_URI_LEN - 100)
    {
        LS_ERROR("[%s] context URI is too long!", TmpLogId::getLogId());
        return NULL;
    }

    char achBuf[MAX_PATH_LEN];
    snprintf(achBuf, MAX_PATH_LEN, "%spublic/", pLocation);
    HttpContext *pContext;
    if (pOldCtx)
    {
        pContext = pOldCtx;
        pContext->setRoot(achBuf);
    }
    else
        pContext = addContext(0, achURI, HandlerType::HT_NULL,
                                 achBuf, NULL, 1);


    if (!pContext)
        return NULL;

    //pContext->setHandler(pWorker);
    pContext->setWebSockAddr(pWorker->getConfigPointer()->getServerAddr());

    HttpContext *pDispatch;
    char appURL[MAX_URI_LEN];
    if (!pStartupFile || *pStartupFile == '\0')
        pStartupFile = NODEJSENTRY;
    memccpy(&achURI[uriLen], pStartupFile, 0, MAX_URI_LEN - uriLen);
    snprintf(appURL, sizeof(appURL), "%s%s", pLocation, pStartupFile);

    pDispatch = addContext(0, achURI, HandlerType::HT_NULL,
                           appURL, NULL, 1);
    pDispatch->setHandler(pWorker);
    pDispatch->setNodejsContext();
    pDispatch->setParent(pContext);

    pContext->setCustomErrUrls("404", achURI);

    pContext->clearDirIndexes();
    pContext->addDirIndexes("-");
    pContext->setNodejsContext();
    pContext->setAutoIndex(0);
    pContext->setAutoIndexOff(1);
    return pContext;
}


int HttpVHost::testAppType(const char *pAppRoot)
{
    struct stat st;
    char achFileName[MAX_PATH_LEN];
    int i;
    for(i = HandlerType::HT_RAILS; i <= HandlerType::HT_NODEJS; ++i)
    {
        snprintf(achFileName, sizeof(achFileName), "%s/%s", pAppRoot,
                 getDefaultStartupFile(i));
        if (stat(achFileName, &st) == 0)
        {
            return i;
        }
    }
    return LS_FAIL;
}


HttpContext *HttpVHost::configAppContext(const XmlNode *pNode,
                                         const char *contextUri,
                                         const char *appPath)
{
    char achAppRoot[MAX_PATH_LEN];
    const char *pStartupFile = ConfigCtx::getCurConfigCtx()->getTag(pNode, "startupFile", 0, 0);
    const char *pBinPath = pNode->getChildValue("binPath");

    int ret = ConfigCtx::getCurConfigCtx()->getAbsolutePath(achAppRoot, appPath);
    if (ret == -1)
    {
        LS_WARN("[%s] app path to application is invalid!",
                 TmpLogId::getLogId());
        return NULL;
    }


    int appType = HandlerType::HT_APPSERVER;
    const char *sType = ConfigCtx::getCurConfigCtx()->getTag(pNode, "appType");
    if (sType)
    {
        switch(*sType | 0x20)
        {
        case 'r': //rails
            appType = HandlerType::HT_RAILS;
            break;
        case 'n': //node
            appType = HandlerType::HT_NODEJS;
            break;
        case 'w':  //wsgi
            appType = HandlerType::HT_PYTHON;
            break;
        default:
            break;
        }
    }

    if (appType == HandlerType::HT_APPSERVER)
    {
        appType = testAppType(achAppRoot);
        if (appType == -1)
        {
            LS_WARN (ConfigCtx::getCurConfigCtx(), "Cannot determine "
                     "application type at %s, disable it.", achAppRoot);
            return NULL;
        }
    }

    AppConfig *pAppConfig = NULL;
    switch(appType)
    {
    case HandlerType::HT_RAILS:
        pAppConfig = &AppConfig::s_rubyAppConfig;
        break;

    case HandlerType::HT_NODEJS:
        pAppConfig = &AppConfig::s_nodeAppConfig;
        break;

    case HandlerType::HT_PYTHON:
        pAppConfig = &AppConfig::s_wsgiAppConfig;
        break;

    }

    const LocalWorkerConfig* pAppDefault = pAppConfig->getpAppDefault();
    int maxConns = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                   "maxConns", 1, 10000, pAppDefault->getMaxConns());
    const char *pModeEnv[3] = { "development", "production", "staging" };
    int production = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                     "appserverEnv", 0, 2, pAppConfig->getAppEnv());
    const char *pAppMode = pModeEnv[production];

    long maxIdle = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                   "extMaxIdleTime", -1, INT_MAX,
                   pAppDefault->getMaxIdleTime());

    if (maxIdle == -1)
        maxIdle = INT_MAX;


    const XmlNodeList *pEnvList = pNode->getChildren("env");
    Env env;
    const char *pValue;
    if (pEnvList)
    {
        XmlNodeList::const_iterator iter;
        for (iter = pEnvList->begin(); iter != pEnvList->end(); ++iter)
        {
            pValue = (*iter)->getValue();
            if (pValue)
                env.add(pValue);
        }
    }

    HttpContext *pOldCtx = getContext(contextUri, strlen(contextUri));
    return configAppContext(appType, contextUri,
            achAppRoot, pStartupFile, maxConns, pAppMode, maxIdle,
            &env, pAppDefault->getRunOnStartUp(), pBinPath, pOldCtx);
}


HttpContext *HttpVHost::configAppContext( int appType, const char *contextUri,
        const char *pAppRoot, const char *pStartupFile,
        int maxConns, const char * pAppMode, int maxIdle,
        const Env *pProcessEnv, int runOnStartup, const char *pBinPath,
        HttpContext *pOldCtx)
{
    if (!pStartupFile)
        pStartupFile = getDefaultStartupFile(appType);

    LocalWorker *pWorker;
    switch(appType)
    {
    case HandlerType::HT_RAILS:
        return configRailsContext(contextUri, pAppRoot, maxConns, pAppMode,
                                  maxIdle, pProcessEnv, pBinPath, pOldCtx);
        break;

    case HandlerType::HT_PYTHON:
        pWorker = addPythonApp(contextUri, pAppRoot, pStartupFile,
                               maxConns, pAppMode, maxIdle, pProcessEnv,
                               runOnStartup, pBinPath);
        if (pWorker)
             return addPythonContext(contextUri, pAppRoot,
                                     pStartupFile, pWorker, pOldCtx);

        break;

    case HandlerType::HT_NODEJS:
        pWorker = addNodejsApp(contextUri, pAppRoot, pStartupFile,
                               maxConns, pAppMode, maxIdle, pProcessEnv,
                               runOnStartup, pBinPath) ;
        if (pWorker)
            return addNodejsContext(contextUri, pAppRoot,
                                    pStartupFile, pWorker, pOldCtx);
        break;
    }
    return NULL;
}



int HttpVHost::configAwstats(const char *vhDomain, int vhAliasesLen,
                             const XmlNode *pNode)
{
    const XmlNode *pAwNode = pNode->getChild("awstats");

    if (!pAwNode)
        return 0;

    int val;
    const char *pValue;
    char iconURI[] = "/awstats/icon/";
    char achBuf[8192];
    int iChrootLen = 0;
    AutoStr2 *pProcChroot = ServerProcessConfig::getInstance().getChroot();
    if (pProcChroot != NULL)
        iChrootLen = pProcChroot->len();
    ConfigCtx currentCtx("awstats");
    val = ConfigCtx::getCurConfigCtx()->getLongValue(pAwNode, "updateMode", 0,
            2, 0);

    if (val == 0)
        return 0;

    if (!getAccessLog())
    {
        LS_ERROR(&currentCtx,
                 "Virtual host does not have its own access log file, "
                 "AWStats integration is disabled.");
        return LS_FAIL;
    }

    if (ConfigCtx::getCurConfigCtx()->getValidPath(achBuf,
            "$SERVER_ROOT/add-ons/awstats/",
            "AWStats installation") == -1)
    {
        LS_ERROR(&currentCtx, "Cannot find AWStats installation at [%s],"
                 " AWStats add-on is disabled!",
                 "$SERVER_ROOT/add-ons/awstats/");
        return LS_FAIL;
    }

    if (ConfigCtx::getCurConfigCtx()->getValidPath(achBuf,
            "$SERVER_ROOT/add-ons/awstats/wwwroot/icon/",
            "AWStats icon directory") == -1)
    {
        LS_ERROR(&currentCtx, "Cannot find AWStats icon directory at [%s],"
                 " AWStats add-on is disabled!",
                 "$SERVER_ROOT/add-ons/awstats/wwwroot/icon/");
        return LS_FAIL;
    }

    addContext(iconURI, HandlerType::HT_NULL,
               &achBuf[iChrootLen], NULL, 1);

    pValue = pAwNode->getChildValue("workingDir");

    if ((!pValue)
        || (ConfigCtx::getCurConfigCtx()->getAbsolutePath(achBuf, pValue) == -1) ||
        (ConfigCtx::getCurConfigCtx()->checkAccess(achBuf) == -1))
    {
        LS_ERROR(&currentCtx,
                 "AWStats working directory: %s does not exist or access denied, "
                 "please fix it, AWStats integration is disabled.", achBuf);
        return LS_FAIL;
    }

    if ((restrained()) &&
        (strncmp(&achBuf[iChrootLen], getVhRoot()->c_str(),
                 getVhRoot()->len())))
    {
        LS_ERROR(&currentCtx,
                 "AWStats working directory: %s is not inside virtual host root: "
                 "%s%s, AWStats integration is disabled.", achBuf,
                 ((iChrootLen == 0) ? ("") : (pProcChroot->c_str())),
                 getVhRoot()->c_str());
        return LS_FAIL;
    }

    Awstats *pAwstats = new Awstats();
    pAwstats->config(this, val, achBuf, sizeof(achBuf), pAwNode,
                     iconURI, vhDomain, vhAliasesLen);
    return 0;
}


HttpContext *HttpVHost::importWebApp(const char *contextUri,
                                     const char *appPath,
                                     const char *pWorkerName, int allowBrowse)
{
    char achFileName[MAX_PATH_LEN];

    int ret = ConfigCtx::getCurConfigCtx()->getAbsolutePath(achFileName,
              appPath);

    if (ret == -1)
    {
        LS_WARN(ConfigCtx::getCurConfigCtx(), "path to Web-App is invalid!");
        return NULL;
    }

    int pathLen = strlen(achFileName);

    if (pathLen > MAX_PATH_LEN - 20)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "path to Web-App is too long!");
        return NULL;
    }

    if (access(achFileName, F_OK) != 0)
    {
        ConfigCtx::getCurConfigCtx()->logErrorPath("Web-App",  achFileName);
        return NULL;
    }

    lstrncpy(&achFileName[pathLen], "WEB-INF/web.xml", sizeof(achFileName) - pathLen);

    if (access(achFileName, F_OK) != 0)
    {
        ConfigCtx::getCurConfigCtx()->logErrorPath("Web-App configuration",
                achFileName);
        return NULL;
    }

    char achURI[MAX_URI_LEN];
    int uriLen = strlen(contextUri);
    lstrncpy(achURI, contextUri, sizeof(achURI));

    if (achFileName[pathLen - 1] != '/')
        ++pathLen;

    if (achURI[uriLen - 1] != '/')
    {
        achURI[uriLen++] = '/';
        achURI[uriLen] = 0;
    }

    if (uriLen > MAX_URI_LEN - 100)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "context URI is too long!");
        return NULL;
    }

    XmlNode *pRoot = ConfigCtx::getCurConfigCtx()->parseFile(achFileName,
                     "web-app");

    if (!pRoot)
    {
        LS_WARN(ConfigCtx::getCurConfigCtx(),
                 "invalid Web-App configuration file: %s."
                 , achFileName);
        return NULL;
    }

    achFileName[pathLen] = 0;
    HttpContext *pContext = configContext(achURI, HandlerType::HT_NULL,
                                          achFileName, NULL, allowBrowse);

    if (!pContext)
    {
        delete pRoot;
        return NULL;
    }

    lstrncpy(&achURI[uriLen], "WEB-INF/", sizeof(achURI) - uriLen);
    lstrncpy(&achFileName[pathLen], "WEB-INF/", sizeof(achFileName) - pathLen);
    HttpContext *pInnerContext;
    pInnerContext = configContext(achURI, HandlerType::HT_NULL,
                                  achFileName, NULL, false);

    if (!pInnerContext)
    {
        delete pRoot;
        return NULL;
    }
    configServletMapping(pRoot, achURI, uriLen, pWorkerName, allowBrowse);

    delete pRoot;
    return pContext;
}


void HttpVHost::configServletMapping(XmlNode *pRoot, char *pachURI,
                                     int iUriLen,
                                     const char *pWorkerName, int allowBrowse)
{
    HttpContext *pInnerContext;
    const XmlNodeList *pList = pRoot->getChildren("servlet-mapping");

    if (pList)
    {
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
        {
            const XmlNode *pMappingNode = *iter;
            const char   *pUrlPattern =
                pMappingNode->getChildValue("url-pattern");

            if (pUrlPattern)
            {
                if (*pUrlPattern == '/')
                    ++pUrlPattern;

                lstrncpy(&pachURI[iUriLen], pUrlPattern, MAX_URI_LEN - iUriLen);
                int patternLen = strlen(pUrlPattern);

                //remove the trailing '*'
                if (* (pUrlPattern + patternLen - 1) == '*')
                    pachURI[iUriLen + patternLen - 1] = 0;

                pInnerContext = configContext(pachURI, HandlerType::HT_SERVLET,
                                              NULL, pWorkerName, allowBrowse);

                if (!pInnerContext)
                {
                    LS_ERROR(ConfigCtx::getCurConfigCtx(),
                             "Failed to import servlet mapping for %s",
                             pMappingNode->getChildValue("url-pattern"));
                }
            }
        }
    }
}


static int getRedirectCode(const XmlNode *pContextNode, int &code,
                           const char *pLocation)
{
    code = ConfigCtx::getCurConfigCtx()->getLongValue(pContextNode,
            "externalRedirect", 0, 1, 1);

    if (code == 0)
        --code;
    else
    {
        int orgCode = ConfigCtx::getCurConfigCtx()->getLongValue(pContextNode,
                      "statusCode", 0, 505, 0);

        if (orgCode)
        {
            code = HttpStatusCode::getInstance().codeToIndex(orgCode);

            if (code == -1)
            {
                LS_WARN(ConfigCtx::getCurConfigCtx(),
                        "Invalid status code %d, use default: 302!",
                        orgCode);
                code = SC_302;
            }
        }
    }

    if ((code == -1) || ((code >= SC_300) && (code < SC_400)))
    {
        if ((pLocation == NULL) || (*pLocation == 0))
        {
            LS_ERROR(ConfigCtx::getCurConfigCtx(),
                     "Destination URI must be specified!");
            return LS_FAIL;
        }
    }

    return 0;
}


int HttpVHost::configContext(const XmlNode *pContextNode)
{
    const char *pUri = NULL;
    const char *pLocation = NULL;
    const char *pHandler = NULL;
    bool allowBrowse = false;
    int match;
    pUri = ConfigCtx::getCurConfigCtx()->getTag(pContextNode, "uri", 1);
    if (pUri == NULL)
        return LS_FAIL;

    match = (strncasecmp(pUri, "exp:", 4) == 0);

    ConfigCtx currentCtx("context", pUri);
    //int len = strlen( pUri );

    const char *pValue = ConfigCtx::getCurConfigCtx()->getTag(pContextNode,
                         "type", 0, 0);

    if (pValue == NULL)
        pValue = "null";

    int role = HandlerType::ROLE_RESPONDER;
    int type = HandlerType::getHandlerType(pValue, role);

    if ((type == HandlerType::HT_END) || (role != HandlerType::ROLE_RESPONDER))
    {
        currentCtx.logErrorInvalTag("type", pValue);
        return LS_FAIL;
    }

    pLocation = pContextNode->getChildValue("location");
    AutoStr2 defLocation;
    bool needUpdate = false;
    if (!pLocation || strlen(pLocation) == 0)
    {
        if (match)
            defLocation.setStr("$DOC_ROOT$0");
        else
        {
            defLocation.setStr("$DOC_ROOT");
            defLocation.append(pUri, strlen(pUri));
        }

        pLocation = defLocation.c_str();
    }
    else if (type != HandlerType::HT_REDIRECT)
    {
        if (*pLocation != '$' && *pLocation != '/')
        {
            defLocation.setStr("$DOC_ROOT/");
            defLocation.append(pLocation, strlen(pLocation));
            needUpdate = true;
        }
  
        //If pLocation does not have tail /, add it now
        if (!match && *(pLocation + strlen(pLocation) - 1) != '/' )
        {
            if (!needUpdate)
                defLocation.setStr(pLocation);
            defLocation.append("/", 1);
            needUpdate = true;
        }
        
        if (needUpdate)
            pLocation = defLocation.c_str();
    }

    pHandler = pContextNode->getChildValue("handler");

    char achHandler[256] = {0};

    if (pHandler)
    {
        if (ConfigCtx::getCurConfigCtx()->expandVariable(pHandler, achHandler,
                256) < 0)
        {
            LS_NOTICE(&currentCtx,
                      "add String is too long for scripthandler, value: %s",
                      pHandler);
            return LS_FAIL;
        }

        pHandler = achHandler;
    }

    allowBrowse = ConfigCtx::getCurConfigCtx()->getLongValue(pContextNode,
                  "allowBrowse", 0, 1, 1);

    if ((*pUri != '/') && (!match))
    {
        LS_ERROR(&currentCtx,
                 "URI must start with '/' or 'exp:', invalid URI - %s",
                 pUri);
        return LS_FAIL;
    }

    HttpContext *pContext = NULL;

    switch (type)
    {
    case HandlerType::HT_JAVAWEBAPP:
        allowBrowse = true;
        pContext = importWebApp(pUri, pLocation, pHandler,
                                allowBrowse);
        break;
    case HandlerType::HT_APPSERVER:
        allowBrowse = true;
        pContext = configAppContext(pContextNode, pUri, pLocation);
        break;
    case HandlerType::HT_REDIRECT:
        if (configRedirectContext(pContextNode, pLocation, pUri, pHandler,
                                  allowBrowse,
                                  match, type) == -1)
            return LS_FAIL;
        break;
    default:
        pContext = configContext(pUri, type, pLocation,
                                 pHandler, allowBrowse);
    }

    if (pContext)
    {
        if (configContextAuth(pContext, pContextNode) == -1)
            return LS_FAIL;
        int val = ConfigCtx::getCurConfigCtx()->getLongValue(pContextNode,
                                   "enableIpGeo", -1, 1, -1);
        if (val != -1)
            pContext->setGeoIP(val);
        pContext->setIpToLoc(m_rootContext.isIpToLocOn());
        return pContext->config(getRewriteMaps(), pContextNode, type,
                                getRootContext(), enableAutoLoadHt());
    }

    return LS_FAIL;
}


int HttpVHost::configRedirectContext(const XmlNode *pContextNode,
                                     const char *pLocation,
                                     const char *pUri, const char *pHandler, bool allowBrowse, int match,
                                     int type)
{
    HttpContext *pContext;
    int code;
    if (getRedirectCode(pContextNode, code, pLocation) == -1)
        return LS_FAIL;

    if (!pLocation)
        pLocation = "";

    pContext = addContext(match, pUri, type, pLocation,
                          pHandler, allowBrowse);

    if (pContext)
    {
        pContext->redirectCode(code);
        pContext->setConfigBit(BIT_ALLOW_OVERRIDE, 1);
        pContext->enableRewrite(0); //Force redirect context to disable rewrite
    }
    return 0;
}


static int compareContext(const void *p1, const void *p2)
{
    return strcmp((*((XmlNode **)p1))->getChildValue("uri", 1),
                  (*((XmlNode **)p2))->getChildValue("uri", 1));
}


int HttpVHost::configVHContextList(const XmlNode *pVhConfNode,
                                   const XmlNodeList *pModuleList)
{
    //we add / context specially, but not config it, such as rewrite
    bool slashCtxCfgRewrite = false;
    addContext("/", HandlerType::HT_NULL,
               getDocRoot()->c_str(), NULL, 1);

    setGlobalMatchContext(1);

    const XmlNode *p0 = pVhConfNode->getChild("contextList", 1);

    XmlNodeList *pList = (XmlNodeList *)(p0->getChildren("context"));

    if (pList)
    {
        pList->sort(compareContext);
        XmlNodeList::const_iterator iter;

        for (iter = pList->begin(); iter != pList->end(); ++iter)
        {
            const char *ctxName = (*iter)->getValue();
            LS_INFO("[%s] config context %s.",
                    TmpLogId::getLogId(), ctxName);
            configContext(*iter);

            //If we did for "/", no need to do again
            if (strcmp(ctxName, "/") == 0)
                slashCtxCfgRewrite = true;
        }
    }

    if (!slashCtxCfgRewrite && enableAutoLoadHt())
    {
        HttpContext *pSlashContext = getContext("/", 1);
        AutoStr2 htaccessPath;
        htaccessPath.setStr(pSlashContext->getLocation(),
                            pSlashContext->getLocationLen());
        htaccessPath.append(".htaccess", 9);
        pSlashContext->configRewriteRule(NULL, NULL, htaccessPath.c_str());
    }

    /***
     * Since modulelist may contain URIs, and these URIs may be new for the current conetxts,
     * so they should be added as new one to the current contexts just beofre call contextInherit()
     * The benifit of doing that is the children inheritance will be good
     */
    if (pModuleList)
        checkAndAddNewUriFormModuleList(pModuleList);

    contextInherit();
    return 0;
}


void HttpVHost::checkAndAddNewUriFormModuleList(const XmlNodeList
        *pModuleList)
{
    XmlNodeList::const_iterator iter0;
    for (iter0 = pModuleList->begin(); iter0 != pModuleList->end(); ++iter0)
    {
        const XmlNode *p0 = (*iter0)->getChild("urlfilterlist", 1);
        const XmlNodeList *pfilterList = p0->getChildren("urlfilter");
        if (pfilterList)
        {
            XmlNode *pNode = NULL;
            XmlNodeList::const_iterator iter;
            const char *pValue;
            HttpContext *pContext;
            int bRegex = 0;

            for (iter = pfilterList->begin(); iter != pfilterList->end(); ++iter)
            {
                pNode = *iter;
                pValue = pNode->getChildValue("uri", 1);
                if (!pValue || strlen(pValue) == 0)
                    break;

                if (pValue[0] != '/')
                    bRegex = 1;

                pContext = getContext(pValue, strlen(pValue), bRegex);
                if (pContext == NULL)
                {
                    char achVPath[MAX_PATH_LEN];
                    char achRealPath[MAX_PATH_LEN];
                    lstrncat2(achVPath, sizeof(achVPath), "$DOC_ROOT", pValue);
                    ConfigCtx::getCurConfigCtx()->getAbsoluteFile(achRealPath, achVPath);
                    pContext = addContext(pValue, HandlerType::HT_NULL, achRealPath, NULL, 1);
                    if (pContext == NULL)
                    {
                        LS_ERROR("[%s] checkAndAddNewUriFormModuleList try to add the context [%s] failed.",
                                 TmpLogId::getLogId(), pValue);
                        break;
                    }
                }
            }
        }
    }
}


//only save the param, not parse it and do not inherit the sessionhooks
int HttpVHost::configVHModuleUrlFilter1(lsi_module_t *pModule,
                                        const XmlNodeList *pfilterList)
{
    int ret = 0;
    XmlNode *pNode = NULL;
    XmlNodeList::const_iterator iter;
    const char *pValue;
    HttpContext *pContext;
    int bRegex = 0;


    for (iter = pfilterList->begin(); iter != pfilterList->end(); ++iter)
    {
        pNode = *iter;
        pValue = pNode->getChildValue("uri", 1);
        if (!pValue || strlen(pValue) == 0)
        {
            ret = -1;
            break;
        }

        if (pValue[0] != '/')
            bRegex = 1;

        pContext = getContext(pValue, strlen(pValue), bRegex);
        assert(pContext != NULL
               && "this Context should have already been added to context by checkAndAddNewUriFormModuleList().\n");

        HttpContext *parentContext = ((HttpContext *)pContext->getParent());
        ModuleConfig *parentConfig = parentContext->getModuleConfig();
        assert(parentConfig != NULL && "parentConfig should not be NULL here.");

        if (pContext->getModuleConfig() == NULL
            || pContext->getSessionHooks() == NULL)
        {
            pContext->setInternalSessionHooks(parentContext->getSessionHooks());
            pContext->setModuleConfig(parentConfig, 0);
        }

        //Check the ModuleConfig of this context is own or just a pointer
        lsi_module_config_t *module_config;
        if (pContext->isModuleConfigOwn())
        {
            module_config = pContext->getModuleConfig()->get(MODULE_ID(pModule));
            ModuleConfig::saveConfig(pNode, pModule, module_config);
        }
        else
        {
            module_config = new lsi_module_config_t;
            memcpy(module_config, pContext->getModuleConfig()->get(MODULE_ID(pModule)),
                   sizeof(lsi_module_config_t));
            module_config->data_flag = LSI_CONFDATA_NONE;
            module_config->sparam = NULL;
            ModuleConfig::saveConfig(pNode, pModule, module_config);

            pContext->setOneModuleConfig(MODULE_ID(pModule), module_config);
            delete module_config;
        }
    }
    return ret;
}


lsi_module_config_t *parseModuleConfigParam(lsi_module_t *pModule,
        const HttpContext *pContext)
{
    lsi_module_config_t *config = ((HttpContext *)
                                   pContext)->getModuleConfig()->get(MODULE_ID(pModule));
    if ((config->data_flag == LSI_CONFDATA_PARSED)
        || (pContext->getParent() == NULL))
        return config;

    ModuleConfig *parentModuleConfig = ((HttpContext *)
                                        pContext->getParent())->getModuleConfig();

    void *init_config = parseModuleConfigParam(pModule,
                        pContext->getParent())->config;

    if (config->filters_enable == -1)
        config->filters_enable = parentModuleConfig->get(MODULE_ID(
                                     pModule))->filters_enable;

    if (config->data_flag == LSI_CONFDATA_OWN)
    {
        assert(config->sparam != NULL);

        lsi_config_key_t *keys = pModule->config_parser->config_keys;
        if (keys)
        {
            //FIXME:Use a vector may be better
            module_param_info_t param_arr[MAX_MODULE_CONFIG_LINES];
            int param_arr_sz = MAX_MODULE_CONFIG_LINES;
            ModuleConfig::preParseModuleParam(config->sparam->c_str(),
                                               config->sparam->len(),
                                               LSI_CFG_CONTEXT,
                                               keys,
                                               param_arr, &param_arr_sz);


            if (param_arr_sz > 0)
            {
                config->config = pModule->config_parser->
                                    parse_config(param_arr, param_arr_sz,
                                                 init_config,
                                                 LSI_CFG_CONTEXT,
                                                 pContext->getURI());
            }
        }
        delete config->sparam;
        config->sparam = NULL;
        config->data_flag = LSI_CONFDATA_PARSED;
    }
    else
        config->config = init_config;

    ((HttpContext *)pContext)->initExternalSessionHooks();
    ModuleManager::getInstance().updateHttpApiHook(((HttpContext *)
            pContext)->getSessionHooks(), ((HttpContext *)pContext)->getModuleConfig(),
            MODULE_ID(pModule));

    return config;
}


int HttpVHost::configVHModuleUrlFilter2(lsi_module_t *pModule,
                                        const XmlNodeList *pfilterList)
{
    if (!pModule || !pModule->config_parser
        || !pModule->config_parser->parse_config)
        return 0;

    int ret = 0;
    XmlNode *pNode = NULL;
    XmlNodeList::const_iterator iter;
    const char *pValue;
    HttpContext *pContext;
    int bRegex = 0;

    for (iter = pfilterList->begin(); iter != pfilterList->end(); ++iter)
    {
        pNode = *iter;
        pValue = pNode->getChildValue("uri", 1);
        if (!pValue || strlen(pValue) == 0)
        {
            ret = -1;
            break;
        }

        if (pValue[0] != '/')
            bRegex = 1;

        pContext = getContext(pValue, strlen(pValue), bRegex);
        parseModuleConfigParam(pModule, (const HttpContext *)pContext);
    }
    return ret;
}


int HttpVHost::configModuleConfigInContext(const XmlNode *pContextNode,
        int saveParam)
{
    const char *pUri = NULL;
    const char *pHandler = NULL;
    const char *pParam = NULL;
    const char *pType = NULL;

    pUri = pContextNode->getChildValue("uri", 1);
    pHandler = pContextNode->getChildValue("handler");
    pParam = pContextNode->getChildValue("param");
    pType = pContextNode->getChildValue("type");

    if (!pUri || !pType || !pParam || !pHandler ||
        strlen(pUri) == 0 || strlen(pType) == 0 ||
        strlen(pParam) == 0 || strlen(pHandler) == 0)
        return LS_FAIL;

    if (strcmp(pType, "module") != 0)
        return LS_FAIL;

    int bRegex = 0;
    if (pUri[0] != '/')
        bRegex = 1;

    HttpContext *pContext = getContext(pUri, strlen(pUri), bRegex);
    if (!pContext)
        return LS_FAIL;

    //find the module and check if exist, if NOT just return
    ModuleManager::iterator moduleIter;
    moduleIter = ModuleManager::getInstance().find(pHandler);
    if (moduleIter == ModuleManager::getInstance().end())
        return LS_FAIL;

    lsi_module_t *pModule = moduleIter.second()->getModule();
    lsi_module_config_t *config = pContext->getModuleConfig()->get(MODULE_ID(
                                      pModule));

    if (!pModule->config_parser || !pModule->config_parser->parse_config)
        return LS_FAIL;


    if (saveParam)
    {
        lsi_module_config_t *module_config;
        int toBeDel = 0;
        if (pContext->isModuleConfigOwn())
            module_config = config;
        else
        {
            module_config = new lsi_module_config_t;
            toBeDel = 1;
            memcpy(module_config, config, sizeof(lsi_module_config_t));
            module_config->data_flag = LSI_CONFDATA_NONE;
            module_config->sparam = NULL;
        }

        if (!module_config->sparam)
            module_config->sparam = new AutoStr2;

        if (module_config->sparam)
        {
            module_config->data_flag = LSI_CONFDATA_OWN;
            module_config->sparam->append("\n", 1);
            module_config->sparam->append(pParam, strlen(pParam));
        }
        if (toBeDel)
        {
            pContext->setOneModuleConfig(MODULE_ID(pModule), module_config);
            delete module_config;
        }
    }
    else
        parseModuleConfigParam(pModule, (const HttpContext *)pContext);
    return 0;
}


int HttpVHost::parseVHModulesParams(const XmlNode *pVhConfNode,
                                    const XmlNodeList *pModuleList, int saveParam)
{
    int ret = 0;
    const XmlNode *p0;
    XmlNode *pNode = NULL;
    const char *moduleName;
    XmlNodeList::const_iterator iter;
    if (pModuleList)
    {
        for (iter = pModuleList->begin(); iter != pModuleList->end(); ++iter)
        {
            pNode = *iter;
            p0 = pNode->getChild("urlfilterlist", 1);

            const XmlNodeList *pfilterList = p0->getChildren("urlfilter");
            if (pfilterList)
            {
                moduleName = pNode->getChildValue("name", 1);
                if (!moduleName)
                {
                    ret = -1;
                    break;
                }

                ModuleManager::iterator moduleIter;
                moduleIter = ModuleManager::getInstance().find(moduleName);
                if (moduleIter == ModuleManager::getInstance().end())
                {
                    ret = -2;
                    break;
                }

                if (saveParam)
                    configVHModuleUrlFilter1(moduleIter.second()->getModule(), pfilterList);
                else
                    configVHModuleUrlFilter2(moduleIter.second()->getModule(), pfilterList);
            }
        }
    }

    //now for the contextlist part to find out the modulehandler param
    p0 = pVhConfNode->getChild("contextList", 1);
    XmlNodeList *pList = (XmlNodeList *)(p0->getChildren("context"));

    if (pList)
    {
        XmlNodeList::const_iterator iter;
        for (iter = pList->begin(); iter != pList->end(); ++iter)
            configModuleConfigInContext(*iter, saveParam);
    }

    return ret;
}

/**
 * ON default "php" handler, since server level have set such a hanlder
 * all Vhost will ingerite it.
 * But if VHost set a different user/group other than that extApp
 * Need to susExec a php with this set user/group.
 */

int HttpVHost::configUserGroup(const XmlNode *pNode)
{
    const char *pUser = pNode->getChildValue("user");
    const char *pGroup = pNode->getChildValue("group");
    if (!pUser)
        return LS_FAIL;
    gid_t gid = -1;
    struct passwd *pw = Daemonize::configUserGroup(pUser, pGroup, gid);
    if (!pw)
    {
        LS_ERROR(ConfigCtx::getCurConfigCtx(), "Invalid User Name(%s) "
                    "or Group Name(%s)!", pUser, pGroup);
        return LS_FAIL;
    }
    setUid(pw->pw_uid);
    setGid(gid);
    setUidMode(UID_DOCROOT);
    return LS_OK;
}


/****
 * COMMENT: About the context under a VHost
 * In the XML of the VHost, there are Modulelist and ContextList which may contain
 * uri which relatives with module, that will cause the context of this uri having its own internal_ctx,
 * before all the context inherit, I need to malloc the internal_ctx for these contexts.
 */
int HttpVHost::config(const XmlNode *pVhConfNode, int is_uid_set)
{
    int iChrootlen = 0;
    if (ServerProcessConfig::getInstance().getChroot() != NULL)
        iChrootlen = ServerProcessConfig::getInstance().getChroot()->len();

    assert(pVhConfNode != NULL);
    if (configBasics(pVhConfNode, iChrootlen) != 0)
        return 1;

    enableCGroup((ServerProcessConfig::getInstance().getCGroupAllow()) ?
                 ConfigCtx::getCurConfigCtx()->getLongValue(
                     pVhConfNode, "cgroups", 0, 1,
                     ServerProcessConfig::getInstance().getCGroupDefault()) : 0);

    if (!is_uid_set)
        updateUGid(TmpLogId::getLogId(), getDocRoot()->c_str());

    const XmlNode *p0;
    HttpContext *pRootContext = &getRootContext();
    initErrorLog(pVhConfNode, 0);
    int recaptchaEnable = HttpServerConfig::getInstance().getGlobalVHost()->isRecaptchaEnabled();
    if (recaptchaEnable)
    {
        const XmlNode *pRecaptchaNode = pVhConfNode->getChild("lsrecaptcha");
        if (pRecaptchaNode)
            configRecaptcha(pRecaptchaNode);
        else
            setFeature(VH_RECAPTCHA, recaptchaEnable);
    }
    configSecurity(pVhConfNode);
    {
        ConfigCtx currentCtx("epsr");
        ExtAppRegistry::configExtApps(pVhConfNode, this);
    }

    getRootContext().setParent(
        &HttpServer::getInstance().getServerContext());
    initAccessLogs(pVhConfNode, 0);
    configVHScriptHandler(pVhConfNode);

    /**
     * Check if we have server level php with different guid,
     * if yes, need set the extApp and config scriptHanlder
     *
     * So now if we have vhost "Example" with "add  lsapi:lsphp php",
     * we will check if we have "Example:lsphp"  extapp first, then will check
     * "lsphp" (server level).
     * Before that if Vhost do not lsphp defined, will not add "Example:lsphp",
     * but if vhost have diff user/group than server level, will create
     * "Example:lsphp" automatically with the vhost user/group.
     *
     */
    if (getPhpXmlNodeSSize() > 0)
    {
        LS_DBG_L("[VHost:%s] start to config its own php handler, size %d.",
                 getName(), getPhpXmlNodeSSize());
        if (ExtAppRegistry::configVhostOwnPhp(this) == 0)
            configVHScriptHandler2();
    }

    configAwstats(ConfigCtx::getCurConfigCtx()->getVhDomain()->c_str(),
                  ConfigCtx::getCurConfigCtx()->getVhAliases()->len(),
                  pVhConfNode);
    p0 = pVhConfNode->getChild("vhssl");

    if (p0)
    {
        ConfigCtx currentCtx("ssl");
        SslContext *pSSLCtx = ConfigCtx::getCurConfigCtx()->newSSLContext(p0, getName(), NULL);
        if (pSSLCtx)
            setSslContext(pSSLCtx);
        else
            delete pSSLCtx;
    }

    p0 = pVhConfNode->getChild("expires");

    if (p0)
    {
        getExpires().config(p0,
                            &(&HttpServer::getInstance())->getServerContext().getExpires(),
                            pRootContext);
        HttpMime::getMime()->getDefault()->getExpires()->copyExpires(
            (&HttpServer::getInstance())->getServerContext().getExpires());
        const char *pValue = p0->getChildValue("expiresByType");

        if (pValue && (*pValue))
        {
            pRootContext->initMIME();
            pRootContext->getMIME()->setExpiresByType(pValue,
                    HttpMime::getMime(), TmpLogId::getLogId());
        }
    }

    p0 = pVhConfNode->getChild("rewrite");

    if (p0)
        configRewrite(p0);

    configIndexFile(pVhConfNode,
                    (&HttpServer::getInstance())->getIndexFileList(),
                    MainServerConfig::getInstance().getAutoIndexURI());

    p0 = pVhConfNode->getChild("customErrorPages", 1);
    if (p0)
    {
        ConfigCtx currentCtx("errorpages");
        pRootContext->configErrorPages(p0);
    }

    p0 = pVhConfNode->getChild("phpIniOverride");
    if (p0)
    {
        pRootContext->configPhpConfig(p0);
    }


    pRootContext->inherit(NULL);


    p0 = pVhConfNode->getChild("modulelist", 1);
    const XmlNodeList *pModuleList = p0->getChildren("module");
    if (pModuleList)
    {
        ModuleConfig *pConfig = new ModuleConfig;
        pConfig->init(ModuleManager::getInstance().getModuleCount());
        pConfig->inherit(ModuleManager::getInstance().getGlobalModuleConfig());
        ModuleConfig::parseConfigList(pModuleList, pConfig, LSI_CFG_VHOST,
                                      this->getName());
        pRootContext->setModuleConfig(pConfig, 1);
    }
    else
        pRootContext->setModuleConfig(((HttpContext *)
                                       pRootContext->getParent())->getModuleConfig(), 0);

    ModuleConfig *pModuleConfig = pRootContext->getModuleConfig();

//     if (pModuleConfig->isMatchGlobal())
//     {
//         HttpSessionHooks *parentHooks = ((HttpContext *)pRootContext->getParent())->getSessionHooks();
//         pRootContext->setInternalSessionHooks(parentHooks);
//     }
//     else
    {
        pRootContext->initExternalSessionHooks();
        ModuleManager::getInstance().applyConfigToHttpRt(
            pRootContext->getSessionHooks(), pModuleConfig);
    }

    //Here, checking pModuleList because modulelist may have new context
    configVHContextList(pVhConfNode, pModuleList);
    configVHWebsocketList(pVhConfNode);

    //check again for "urlfilterList" part and "contextList" part,
    //First, just save all params and filterEnbale value
    parseVHModulesParams(pVhConfNode, pModuleList, 1);

    //Now since all params and filterEnbale value saved,
    //inheirt and parse,
    //finally, release the temp aurostr2
    parseVHModulesParams(pVhConfNode, pModuleList, 0);

    return 0;
}


/**
 * Only for a special case, we need to handler spcially, otherwise
 * use the existing routing
 * For the php script handler, which is defined in Server level
 * and if VHost has defifferent User/Group
 * 1, VHost defined to use the ExtAPP for PHP
 * 2, Vhsot does not define any ExtApp for PHP
 *
 */

int HttpVHost::configVHScriptHandler2()
{
    HttpMime *pHttpMime = getMIME();
    const char *suffix  = NULL;
    const char *orgAppName = NULL;
    php_xml_st *pPhpXmlNodeS;
    char appName[256];

    //HttpMime::configScriptHandler(pList, getMIME(), this);
    for (int i=0; i<getPhpXmlNodeSSize(); ++i)
    {
        pPhpXmlNodeS = getPhpXmlNodeS(i);
        suffix = pPhpXmlNodeS->suffix.c_str();
        orgAppName = pPhpXmlNodeS->app_name.c_str();
        getUniAppName(orgAppName, appName, 256);
        const HttpHandler *pHdlr = HandlerFactory::getHandler("lsapi", appName);
        HttpMime::addMimeHandler(pHdlr, NULL, pHttpMime, suffix);
    }
    return 0;
}

int HttpVHost::configVHScriptHandler(const XmlNode *pVhConfNode)
{
    /**
     * Comments, even if the list is NULL, we still need to config because
     * VHost may have different user/group than server
     */
    const XmlNodeList *pList = NULL;
    const XmlNode *p0 = pVhConfNode->getChild("scriptHandler");
    if (p0)
        pList = p0->getChildren("add");

    getRootContext().initMIME();
    HttpMime::configScriptHandler(pList, getMIME(), this);

    return 0;
}


const HttpHandler *HttpVHost::isHandlerAllowed(const HttpHandler *pHdlr,
        int type,
        const char *pHandler)
{
    const HttpVHost *pVHost1 = ((const ExtWorker *) pHdlr)->
                               getConfigPointer()->getVHost();

    if (pVHost1)
    {
        if (this != pVHost1)
        {
            LS_ERROR("[%s] Access to handler [%s:%s] from [%s] is denied!",
                     TmpLogId::getLogId(),
                     HandlerType::getHandlerTypeString(type),
                     pHandler, getName());
            return  NULL;
        }
    }
    return pHdlr;
}


int HttpVHost::configRecaptcha(const XmlNode *pNode)
{
    ConfigCtx currentCtx("vhost", getName());
    LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Config recaptcha.");
    HttpVHost *pGlobalVHost = HttpServerConfig::getInstance().getGlobalVHost();
    int enabled = pGlobalVHost->isRecaptchaEnabled();
    enabled = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "enabled", 0, 1, enabled);
    if (!enabled)
        return 0;

    const char *pSiteKey = pNode->getChildValue("siteKey");
    const char *pSecretKey = pNode->getChildValue("secretKey");

    if (pGlobalVHost == this && (NULL == pSiteKey || NULL == pSecretKey))
    {
        pSiteKey = Recaptcha::getDefaultSiteKey();
        pSecretKey = Recaptcha::getDefaultSecretKey();
    }

    {
        const Recaptcha *pGlobalRecaptcha = pGlobalVHost->getRecaptcha();
        const AutoStr2 *pGlobalSiteKey = (pGlobalRecaptcha ? pGlobalRecaptcha->getSiteKey() : NULL);
        const AutoStr2 *pGlobalSecretKey = (pGlobalRecaptcha ? pGlobalRecaptcha->getSecretKey() : NULL);

        if ((NULL == pSiteKey && NULL == pGlobalSiteKey)
            || (NULL == pSecretKey && NULL == pGlobalSecretKey))
        {
            LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Missing site or secret key.");
            return -1;
        }
    }

    uint16_t type = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "type",
                        Recaptcha::TYPE_UNKNOWN, Recaptcha::TYPE_END, Recaptcha::TYPE_UNKNOWN);
    uint16_t maxTries = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                                "maxTries", 0, SHRT_MAX, 3);
    int regLimit = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                            "regConnLimit", 0, INT_MAX, 15000);
    int sslLimit = ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                            "sslConnLimit", 0, INT_MAX, 10000);

    Recaptcha *pRecaptcha = new Recaptcha();

    pRecaptcha->setType(type);
    pRecaptcha->setMaxTries(maxTries);
    pRecaptcha->setRegConnLimit(regLimit);
    pRecaptcha->setSslConnLimit(sslLimit);
    if (pSiteKey)
        pRecaptcha->setSiteKey(pSiteKey);
    if (pSecretKey)
        pRecaptcha->setSecretKey(pSecretKey);

    const char *pValue = pNode->getChildValue("botWhiteList");
    if (pValue)
    {
        StringList list;
        list.split(pValue, strlen(pValue) + pValue, "\r\n");
        pRecaptcha->setBotWhitelist(&list);
    }

    setRecaptcha(pRecaptcha);
    setFeature(VH_RECAPTCHA, enabled);

    LS_NOTICE(ConfigCtx::getCurConfigCtx(), "Recaptcha configured and enabled.");
    return 0;
}


void HttpVHost::configVHChrootMode(const XmlNode *pNode)
{
    int val = ConfigCtx::getCurConfigCtx()->getLongValue(pNode, "chrootMode",
              0, 2, 0);
    const char *pValue = pNode->getChildValue("chrootPath");
    int len = 0;
    if (ServerProcessConfig::getInstance().getChroot() != NULL)
        len = ServerProcessConfig::getInstance().getChroot()->len();

    if (pValue)
    {
        char achPath[4096];
        char *p  = achPath;
        int  ret = ConfigCtx::getCurConfigCtx()->getAbsoluteFile(p, pValue);

        if (ret)
            val = 0;
        else
        {
            char *p1 = p + len;

            if (restrained() &&
                strncmp(p1, getVhRoot()->c_str(),
                        getVhRoot()->len()) != 0)
            {
                LS_ERROR(ConfigCtx::getCurConfigCtx(),
                         "Chroot path %s must be inside virtual host root %s",
                         p1, getVhRoot()->c_str());
                val = 0;
            }

            setChroot(p);
        }

    }
    else
        val = 0;

    setChrootMode(val);

}


HttpVHost *HttpVHost::configVHost(const XmlNode *pNode, const char *pName,
                                  const char *pDomain, const char *pAliases, const char *pVhRoot,
                                  const XmlNode *pConfigNode)
{
    while (1)
    {
        if (strcmp(pName, DEFAULT_ADMIN_SERVER_NAME) == 0)
        {
            LS_ERROR(ConfigCtx::getCurConfigCtx(),
                     "invalid <name>, %s is used for the "
                     "administration server, ignore!",  pName);
            break;
        }

        ConfigCtx currentCtx("vhost",  pName);

        if (!pVhRoot)
            pVhRoot = ConfigCtx::getCurConfigCtx()->getTag(pNode, "vhRoot");

        if (pVhRoot == NULL)
            break;

        if (ConfigCtx::getCurConfigCtx()->getValidChrootPath(pVhRoot,
                "vhost root") != 0)
            break;

        if (!pDomain)
            pDomain = pName;

        ConfigCtx::getCurConfigCtx()->setVhDomain(pDomain);

        if (!pAliases)
            pAliases = "";

        ConfigCtx::getCurConfigCtx()->setVhAliases(pAliases);
        HttpVHost *pVHnew = new HttpVHost(pName);
        assert(pVHnew != NULL);
        pVHnew->setVhRoot(ConfigCtx::getCurConfigCtx()->getVhRoot());
        pVHnew->followSymLink(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                              "allowSymbolLink", 0, 2,
                              HttpServerConfig::getInstance().getFollowSymLink()));
        pVHnew->enableScript(ConfigCtx::getCurConfigCtx()->getLongValue(
                                 pNode, "enableScript", 0, 1, 1));
        pVHnew->serverEnabled(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                              "enabled", 0, 1, 1));
        pVHnew->restrained(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                           "restrained", 0, 1, 0));

        pVHnew->setUidMode(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                           "setUIDMode", 0, 2, 0));




        pVHnew->setMaxKAReqs(ConfigCtx::getCurConfigCtx()->getLongValue(pNode,
                             "maxKeepAliveReq", 0, 32767,
                             HttpServerConfig::getInstance().getMaxKeepAliveRequests()));

        pVHnew->getThrottleLimits()->config(pNode,
                                            ThrottleControl::getDefault(), &currentCtx);

        int is_uid_set = (pVHnew->configUserGroup(pNode) == LS_OK);


        if (pVHnew->config(pConfigNode, is_uid_set) == 0)
        {
            HttpServer::getInstance().checkSuspendedVHostList(pVHnew);

            /**
             * Just call below after the docRoot is parsed.
             * If not exist, create it.
             */
//             struct stat stBuf;
//             const char *path =
//                 pVHnew->m_ReqParserConfig.m_sUploadFilePathTemplate.c_str();
//             if (stat(path, &stBuf) == -1)
//             {
//                 mkdir(path, 02771);
//                 chmod(path, 02771);
//                 if (pVHnew->m_rootContext.getSetUidMode() == 2)
//                 {
//                     struct stat st;
//                     if (stat(pVHnew->m_rootContext.getRoot()->c_str(), &st) != -1)
//                         chown(path, st.st_uid, st.st_gid);
//                 }
//             }
            return pVHnew;
        }

        delete pVHnew;
        LS_WARN(&currentCtx, "configuration failed!");

        break;
    }

    return NULL;
}

/**
 * Example:
 * listeners  Default, DefaultHttps
 */
int HttpVHost::configListenerMappings(const char *pListeners,
                                      const char *pDomain,
                                      const char *pAliases)
{
    int add = 0;
    StringList  listenerNames;
    listenerNames.split(pListeners, strlen(pListeners) + pListeners, ",");
    StringList::iterator iter;
    for (iter = listenerNames.begin(); iter != listenerNames.end(); ++iter)
    {
        const char *p = (*iter)->c_str();
        if (HttpServer::getInstance().mapListenerToVHost(p, this,
            pDomain ? pDomain : "*") == 0)
            ++add;
        if (pAliases &&
            HttpServer::getInstance().mapListenerToVHost(p, this, pAliases) == 0)
            ++add;
    }

    ConfigCtx currentCtx("vhost", getName());
    LS_INFO(&currentCtx, "configListenerMappings vhost [%s], listeners [%s], "
            " domain [%s], aliase [%s], added %d mappings.",
            getName(), pListeners, pDomain, pAliases, add);
    return add;
}



HttpVHost *HttpVHost::configVHost(XmlNode *pNode)
{
    XmlNode *pVhConfNode = NULL;
    HttpVHost *pVHost = NULL;
    bool gotConfigFile = false;

    while (1)
    {
        const char *pName = ConfigCtx::getCurConfigCtx()->getTag(pNode, "name", 1);

        if (pName == NULL)
            break;

        //m_sVhName.setStr( pName );
        ConfigCtx::getCurConfigCtx()->setVhName(pName);
        const char *pVhRoot = ConfigCtx::getCurConfigCtx()->getTag(pNode,
                              "vhRoot");

        if (pVhRoot == NULL)
            break;

        char achVhConf[MAX_PATH_LEN];

        if (ConfigCtx::getCurConfigCtx()->getValidChrootPath(pVhRoot,
                "vhost root") != 0)
            break;

        ConfigCtx::getCurConfigCtx()->setDocRoot(
            ConfigCtx::getCurConfigCtx()->getVhRoot());

        const char *pConfFile = pNode->getChildValue("configFile");
        if (pConfFile != NULL)
        {
            if (ConfigCtx::getCurConfigCtx()->getValidFile(achVhConf, pConfFile,
                    "vhost config") == 0)
            {
                pVhConfNode = plainconf::parseFile(achVhConf, "virtualHostConfig");
                if (pVhConfNode == NULL)
                {
                    LS_ERROR(ConfigCtx::getCurConfigCtx(), "cannot load configure file - %s !",
                             achVhConf);
                }
                else
                    gotConfigFile = true;
            }
        }

        /**
         * If do not have the configFile, will use the current XmlNode
         */
        if (!pVhConfNode)
        {
            pVhConfNode = pNode;
            LS_INFO(ConfigCtx::getCurConfigCtx(), "Since no configFile "
                    "defined, use current node.");
        }

        const char *pDomain  = pVhConfNode->getChildValue("vhDomain");
        const char *pAliases = pVhConfNode->getChildValue("vhAliases");
        pVHost = HttpVHost::configVHost(pNode, pName, pDomain, pAliases, pVhRoot,
                                        pVhConfNode);

        /**
         * Comments: listeners must be in the root level of Vhost,
         * for compatible with vhosttemplate
         */
        const char *pListeners = pNode->getChildValue("listeners");
        if (pListeners && pVHost)
            pVHost->configListenerMappings(pListeners, pDomain, pAliases);

        break;
    }

    if (gotConfigFile)
        delete pVhConfNode;

    return pVHost;
}


int HttpVHost::checkDeniedSubDirs(const char *pUri, const char *pLocation)
{
    int len = strlen(pLocation);
    DeniedDir *pDeniedDir = HttpServerConfig::getInstance().getDeniedDir();
    if (* (pLocation + len - 1) != '/')
        return 0;

    DeniedDir::iterator iter = pDeniedDir->lower_bound(pLocation);
    iter = pDeniedDir->next_included(iter, pLocation);

    while (iter != pDeniedDir->end())
    {
        const char *pDenied = pDeniedDir->getPath(iter);
        const char *pSubDir = pDenied + len;
        char achNewURI[MAX_URI_LEN + 1];
        int n = ls_snprintf(achNewURI, MAX_URI_LEN, "%s%s", pUri, pSubDir);

        if (n == MAX_URI_LEN)
        {
            LS_ERROR(ConfigCtx::getCurConfigCtx(),
                     "URI is too long when add denied dir %s for context %s",
                     pDenied, pUri);
            return LS_FAIL;
        }

        if (getContext(achNewURI, strlen(achNewURI)))
            continue;

        if (!addContext(achNewURI, HandlerType::HT_NULL, pDenied,
                        NULL, false))
        {
            LS_ERROR(ConfigCtx::getCurConfigCtx(),
                     "Failed to block denied dir %s for context %s",
                     pDenied, pUri);
            return LS_FAIL;
        }

        iter = pDeniedDir->next_included(iter, pLocation);
    }

    return 0;
}


void HttpVHost::enableAioLogging()
{
    if (m_iAioAccessLog < 0)
        m_iAioAccessLog = HttpLogSource::getAioServerAccessLog();
    if (m_iAioErrorLog < 0)
        m_iAioErrorLog = HttpLogSource::getAioServerErrorLog();

    if (m_iAioAccessLog == 1 && m_pAccessLog[0])
    {
        for (int i = 0; i < MAX_ACCESS_LOG; ++i)
        {
            if (m_pAccessLog[i] && !m_pAccessLog[i]->isPipedLog())
            {
                m_pAccessLog[i]->getAppender()->setAsync();
                LS_DBG("[VHost:%s] Enable AIO for Access Logging!",
                       getName());
            }
        }
    }
    if (m_iAioErrorLog == 1)
    {
        getLogger()->getAppender()->setAsync();
        LS_DBG_L("[VHost:%s] Enable AIO for Error Logging!",
                 getName());
    }

    if (m_pBytesLog)
    {
        m_pBytesLog->setAsync();
        LS_DBG("[VHost:%s] Enable AIO for Bandwidth Logging!",
               getName());
    }
}

void HttpVHost::addUrlStaticFileMatch(StaticFileCacheData *pData,
                                      const char *url, int urlLen)
{
    static_file_data_t *data = new static_file_data_t;
    data->pData = pData;
    data->tmaccess = DateTime::s_curTime;
    data->url.setStr(url, urlLen);
    GHash::iterator it = m_pUrlStxFileHash->insert(data->url.c_str(), data);
    if (!it)
    {
        it = m_pUrlStxFileHash->find(data->url.c_str());
        m_pUrlStxFileHash->erase(it);
        it = m_pUrlStxFileHash->insert(data->url.c_str(), data);
        if (!it)
        {
            LS_ERROR("[static file cache] try to insert again still failed.");
        }
    }
    pData->incRef();
}


int HttpVHost::checkFileChanged(static_file_data_t *data, struct stat &sb)
{
    if (sb.st_size == data->pData->getFileSize() &&
        sb.st_mtime == data->pData->getLastMod() &&
        sb.st_ino == data->pData->getINode())
        return 0;
    else
    {
        UrlStxFileHash::iterator it = m_pUrlStxFileHash->find(data->url.c_str());
        if (it != m_pUrlStxFileHash->end())
        {
            m_pUrlStxFileHash->erase(it);
            data->pData->decRef();
            delete data;
        }
        return -1;
    }
}


static_file_data_t *HttpVHost::getUrlStaticFileData(const char *url)
{
    UrlStxFileHash::iterator it = m_pUrlStxFileHash->find(url);
    if (it == m_pUrlStxFileHash->end())
        return NULL;
    else
        return it.second();
}


void HttpVHost::urlStaticFileHashClean()
{
    UrlStxFileHash::iterator itt, itn;
    for (itt = m_pUrlStxFileHash->begin(); itt != m_pUrlStxFileHash->end();)
    {
        static_file_data_t *data = (static_file_data_t *)itt.second();
        itn = m_pUrlStxFileHash->next(itt);
        if (data->pData->getRef() <= 1)
        {
            LS_DBG_L("[VHost:%s][static file cache] Clean HashT.", getName());
            data->pData->decRef();
            m_pUrlStxFileHash->erase(itt);
            delete data;
        }
        itt = itn;
    }
}


void HttpVHost::removeurlStaticFile(static_file_data_t *data)
{
    UrlStxFileHash::iterator it = m_pUrlStxFileHash->find(data->url.c_str());
    if (it != m_pUrlStxFileHash->end())
    {
        m_pUrlStxFileHash->erase(it);
        delete data;
    }
}

/**
 * return the just assigned id
 */
int HttpVHost::addUrlToUrlIdHash(const char *url)
{
    int size = m_pUrlIdHash->size();
    url_id_data_t *data = new url_id_data_t;
    data->id = size;
    data->url.setStr(url);
    m_pUrlIdHash->insert(data->url.c_str(), data);
    return size;
}

/**
 * return the bit of the url added to the hash
 */
int HttpVHost::getIdBitOfUrl(const char *url)
{
    UrlIdHash::iterator itr = m_pUrlIdHash->find(url);
    if (itr != m_pUrlIdHash->end())
    {
        url_id_data_t *data = itr.second();
        return data->id;
    }

    return -1;
}
