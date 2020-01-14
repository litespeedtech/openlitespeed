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
#include "configctx.h"

#include <http/denieddir.h>
#include <http/httplog.h>
#include <http/httpserverconfig.h>
#include <http/serverprocessconfig.h>
#include <log4cxx/level.h>
#include <log4cxx/logger.h>
#include <lsr/ls_fileio.h>
#include <lsr/ls_strtool.h>

#include <main/mainserverconfig.h>
// #include <main/plainconf.h>
#include <util/accesscontrol.h>
#include <util/gpath.h>
#include <util/xmlnode.h>
#include <util/autostr.h>
#include <util/autobuf.h>

#include <sslpp/sslcontext.h>
#include <sslpp/sslcontextconfig.h>
#include <sslpp/sslengine.h>
#include <sslpp/sslocspstapling.h>
#include <sslpp/sslsesscache.h>
#include <sslpp/sslticket.h>
#include <sslpp/sslutil.h>

#include <assert.h>
#if __cplusplus <= 199711L && !defined(static_assert)
#define static_assert(a, b) _Static_assert(a, b)
#endif

#include <openssl/ssl.h>

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#define MAX_URI_LEN  1024

#define VH_ROOT     "VH_ROOT"
#define DOC_ROOT    "DOC_ROOT"
#define SERVER_ROOT "SERVER_ROOT"
#define VH_NAME     "VH_NAME"
#define VH_DOMAIN   "VH_DOMAIN"
//VH_USER


//static const char *MISSING_TAG = "missing <%s>";
static const char *MISSING_TAG_IN = "missing <%s> in <%s>";
//static const char *INVAL_TAG = "<%s> is invalid: %s";
//static const char * INVAL_TAG_IN = "[%s] invalid tag <%s> within <%s>!";
static const char *INVAL_PATH = "Path for %s is invalid: %s";
static const char *INACCESSIBLE_PATH = "Path for %s is not accessible: %s";
AutoStr2    ConfigCtx::s_vhName;
AutoStr2    ConfigCtx::s_vhDomain("");
AutoStr2    ConfigCtx::s_vhAliases("");
char        ConfigCtx::s_aVhRoot[MAX_PATH_LEN];
char        ConfigCtx::s_aDocRoot[MAX_PATH_LEN];
ConfigCtx  *ConfigCtx::s_pCurConfigCtx = NULL;


long long getLongValue(const char *pValue, int base)
{
    long long l = strlen(pValue);
    long long m = 1;
    char ch = * (pValue + l - 1);

    if (ch == 'G' || ch == 'g')
        m = 1024 * 1024 * 1024;
    else if (ch == 'M' || ch == 'm')
        m = 1024 * 1024;
    else if (ch == 'K' || ch == 'k')
        m = 1024;

    return strtoll(pValue, (char **) NULL, base) * m;
}


void ConfigCtx::logErrorPath(const char *pstr1,  const char *pstr2)
{
    LS_WARN(this, "Path for %s is invalid: %s", pstr1, pstr2);
}


void ConfigCtx::logErrorInvalTag(const char *pstr1,  const char *pstr2)
{
    LS_ERROR(this, "<%s> is invalid: %s", pstr1, pstr2);
}


void ConfigCtx::logErrorMissingTag(const char *pstr1)
{
    LS_ERROR(this, "missing <%s>", pstr1);
}


const char *ConfigCtx::getTag(const XmlNode *pNode, const char *pName,
                              int bKeyName, int errorIfNotExist)
{
    if (pNode == NULL)
    {
        LS_ERROR(this, "pNode is NULL while calling getTag( name: %s )" , pName);
        return NULL;
    }

    const char *pRet = pNode->getChildValue(pName, bKeyName);
    if (!pRet && errorIfNotExist)
        LS_ERROR(this, MISSING_TAG_IN, pName, pNode->getName());

    return pRet;
}


long long ConfigCtx::getLongValue(const XmlNode *pNode, const char *pTag,
                                  long long min, long long max, long long def, int base)
{
    if (pNode == NULL)
        return def;

    const char *pValue = pNode->getChildValue(pTag);
    long long val;

    if (pValue)
    {
        val = ::getLongValue(pValue, base);

        if ((max == INT_MAX) && (val > max))
            val = max;

        if (((min != LLONG_MIN) && (val < min)) ||
            ((max != LLONG_MAX) && (val > max)))
        {
            LS_WARN(this, "invalid value of <%s>:%s, use default=%lld",
                    pTag, pValue, def);
            return def;
        }

        return val;
    }
    else
        return def;

}

int ConfigCtx::getRootPath(const char *&pRoot, const char *&pFile)
{
    int defRootType = 0;
    int ret = 0;
    if (*pFile == '$')
    {
        if (strncasecmp(pFile + 1, VH_ROOT, 7) == 0)
        {
            defRootType = 2;
            pFile += 8;
        }
        else if (strncasecmp(pFile + 1, DOC_ROOT, 8) == 0)
        {
            defRootType = 1;
            pFile += 9;
        }
        else if (strncasecmp(pFile + 1, SERVER_ROOT, 11) == 0)
        {
            defRootType = 0;
            pFile += 12;
        }
        else
            defRootType = -1;
    }
    else if (*pFile == '/')
        defRootType = -1;

    switch(defRootType)
    {
    case 0:
        pRoot = MainServerConfig::getInstance().getServerRoot();
        break;

    case 1:
        if (s_aDocRoot[0])
            pRoot = getDocRoot();
        else
        {
            LS_ERROR(this, "Document root path is not available for %s.", pFile);
            ret = LS_FAIL;
        }
        break;

    case 2:
        if (s_aVhRoot[0])
            pRoot = s_aVhRoot;
        else
        {
            LS_ERROR(this, "Virtual host root path is not available for %s.", pFile);
            ret = LS_FAIL;
        }
        break;


    default:
        break;
    }

    if (ret == 0 && (defRootType != -1) && (*pFile == '/'))
        ++pFile;

    return ret;
}


int ConfigCtx::expandVariable(const char *pValue, char *pBuf,
                              int bufLen, int allVariable)
{
    const char *pBegin = pValue;
    char *pBufEnd = pBuf + bufLen - 1;
    char *pCur = pBuf;
    int len;

    while (*pBegin)
    {
        len = strcspn(pBegin, "$");

        if (len > 0)
        {
            if (len > pBufEnd - pCur)
                return LS_FAIL;

            memmove(pCur, pBegin, len);
            pCur += len;
            pBegin += len;
        }

        if (*pBegin == '$')
        {
            const char *pName = NULL;
            int nameLen = -1;

            if (strncasecmp(pBegin + 1, VH_NAME, 7) == 0)
            {
                pBegin += 8;
                pName = s_vhName.c_str();
                nameLen = s_vhName.len();
            }
            else if (strncasecmp(pBegin + 1, VH_DOMAIN, 9) == 0)
            {
                pBegin += 10;
                pName = s_vhDomain.c_str();
                nameLen = s_vhDomain.len();
            }
            else if (allVariable && strncasecmp(pBegin + 1, VH_ROOT, 7) == 0)
            {
                pBegin += 8;
                pName = s_aVhRoot;
            }
            else if (allVariable && strncasecmp(pBegin + 1, DOC_ROOT, 8) == 0)
            {
                pBegin += 9;
                pName = s_aDocRoot;
            }
            else if (allVariable && strncasecmp(pBegin + 1, SERVER_ROOT, 11) == 0)
            {
                pBegin += 12;
                pName = MainServerConfig::getInstance().getServerRoot();
            }
            else
            {
                if (pCur == pBufEnd)
                    return LS_FAIL;

                *pCur++ = '$';
                ++pBegin;
                continue;
            }

            if (pName && nameLen == -1)
                nameLen = strlen(pName);

            if (pName && nameLen > 0)
            {
                if (nameLen > pBufEnd - pCur)
                    return LS_FAIL;

                memmove(pCur, pName, nameLen);
                pCur += nameLen;
            }
        }
    }

    *pCur = 0;
    return pCur - pBuf;
}


int ConfigCtx::getAbsolute(char *res, const char *path, int pathOnly)
{
    const char *pChroot = MainServerConfig::getInstance().getChroot();
    int iChrootLen = MainServerConfig::getInstance().getChrootlen();
    const char *pRoot = "";
    const char *pPath = path;
    int ret;
    char achBuf[MAX_PATH_LEN];
    char *dest = achBuf;
    int len = MAX_PATH_LEN;

    if (getRootPath(pRoot, pPath))
        return LS_FAIL;

    if (pChroot)
    {
        if ((*pRoot) || (strncmp(path, pChroot,
                                 iChrootLen) != 0))
        {
            memmove(dest, pChroot, iChrootLen);
            dest += iChrootLen;
            len -= iChrootLen;
        }
    }

    if (pathOnly)
        ret = GPath::getAbsolutePath(dest, len, pRoot, pPath);
    else
        ret = GPath::getAbsoluteFile(dest, len, pRoot, pPath);

    if (ret)
    {
        LS_ERROR(this, "Failed to tanslate to absolute path with root=%s, "
                 "path=%s!", pRoot, path);
    }
    else
    {
        // replace "$VH_NAME" with the real name of the virtual host.
        if (expandVariable(achBuf, res, MAX_PATH_LEN, 1) < 0)
        {
            LS_NOTICE(this, "Path is too long: %s", pPath);
            return LS_FAIL;
        }
    }

    return ret;
}


int ConfigCtx::getAbsoluteFile(char *dest, const char *file)
{
    return getAbsolute(dest, file, 0);
}


int ConfigCtx::getAbsolutePath(char *dest, const char *path)
{
    return getAbsolute(dest, path, 1);
}


char *ConfigCtx::getExpandedTag(const XmlNode *pNode,
                                const char *pName, char *pBuf, int bufLen, int bKeyName)
{
    const char *pVal = getTag(pNode, pName, bKeyName);

    if (!pVal)
        return NULL;
    //if ( expandVariable( pVal, pBuf, bufLen ) >= 0 )
    if (expandVariable(pVal, pBuf, bufLen) >= 0)
        return pBuf;

    LS_NOTICE(this, "String is too long for tag: %s, value: %s, maxlen: %d",
              pName, pVal, bufLen);
    return NULL;
}


int ConfigCtx::getValidFile(char *dest, const char *file, const char *desc)
{
    if ((getAbsoluteFile(dest, file) != 0)
        || access(dest, F_OK) != 0)
    {
        LS_ERROR(INVAL_PATH, desc,  dest);
        return LS_FAIL;
    }

    return 0;
}


int ConfigCtx::getValidPath(char *dest, const char *path, const char *desc)
{
    if (getAbsolutePath(dest, path) != 0)
    {
        LS_WARN(INVAL_PATH, desc,  path);
        return LS_FAIL;
    }

    if (access(dest, F_OK) != 0)
    {
        LS_WARN(INACCESSIBLE_PATH, desc,  dest);
        return LS_FAIL;
    }

    return 0;
}


int ConfigCtx::getValidChrootPath(const char *path, const char *desc)
{
    const char *pChroot = MainServerConfig::getInstance().getChroot();
    int iChrootLen = MainServerConfig::getInstance().getChrootlen();
    if (getValidPath(s_aVhRoot, path, desc) == -1)
        return LS_FAIL;

    if ((pChroot) &&
        (strncmp(s_aVhRoot, pChroot,  iChrootLen) == 0))
    {
        memmove(s_aVhRoot, s_aVhRoot + iChrootLen,
                strlen(s_aVhRoot) - iChrootLen + 1);
    }
    return 0;
}


int ConfigCtx::getLogFilePath(char *pBuf, const XmlNode *pNode)
{
    const char *pValue = getTag(pNode, "fileName", 1);

    if (pValue == NULL)
        return 1;

    if (getAbsoluteFile(pBuf, pValue) != 0)
    {
        LS_WARN(this, "Path for %s is invalid: %s", "log file",  pValue);
        return 1;
    }

    if (GPath::isWritable(pBuf) == false)
    {
        LS_ERROR(this, "log file is not writable - %s", pBuf);
        return 1;
    }

    return 0;
}


int ConfigCtx::expandDomainNames(const char *pDomainNames,
                                 char *pDestDomains, int len, char dilemma)
{
    if (!pDomainNames)
    {
        pDestDomains[0] = 0;
        return 0;
    }

    const char *p = pDomainNames;

    char *pDest = pDestDomains;

    char *pEnd = pDestDomains + len - 1;

    const char *pStr;

    int n;

    while ((*p) && (pDest < pEnd))
    {
        n = strspn(p, " ,");

        if (n)
            p += n;

        n = strcspn(p, " ,");

        if (!n)
            continue;

        if ((strncasecmp(p, "$vh_domain", 10) == 0) &&
            (10 == n))
        {
            pStr = s_vhDomain.c_str();
            len = s_vhDomain.len();
        }
        else if ((strncasecmp(p, "$vh_aliases", 11) == 0) &&
                 (11 == n))
        {
            pStr = s_vhAliases.c_str();
            len = s_vhAliases.len();
        }
        else
        {
            pStr = p;
            len = n;
        }

        if ((pDest != pDestDomains) && (pDest < pEnd))
            *pDest++ = dilemma;

        if (len > pEnd - pDest)
            len = pEnd - pDest;

        memmove(pDest, pStr, len);
        pDest += len;
        p += n;
    }

    *pDest = 0;
    *pEnd = 0;
    return pDest - pDestDomains;
}


int ConfigCtx::checkPath(char *pPath, const char *desc, int follow)
{
    char achOld[MAX_PATH_LEN];
    struct stat st;
    int ret = ls_fio_stat(pPath, &st);

    if (ret == -1)
    {
        logErrorPath(desc,  pPath);
        return LS_FAIL;
    }

    memccpy(achOld, pPath, 0, MAX_PATH_LEN - 1);
    ret = GPath::checkSymLinks(pPath, pPath + strlen(pPath),
                               pPath + MAX_PATH_LEN, pPath, follow);

    if (ret == -1)
    {
        if (errno == EACCES)
        {
            LS_ERROR(this, "Path of %s contains symbolic link or"
                     " ownership does not match:%s",
                     desc, pPath);
        }
        else
            logErrorPath(desc,  pPath);

        return LS_FAIL;
    }

    if (S_ISDIR(st.st_mode))
    {
        if (* (pPath + ret - 1) != '/')
        {
            * (pPath + ret) = '/';
            * (pPath + ret + 1) = 0;
        }
    }

    if (strcmp(achOld, pPath) != 0)
        LS_DBG_L("the real path of %s is %s.", achOld, pPath);

    if (ServerProcessConfig::getInstance().getChroot() != NULL)
        pPath += ServerProcessConfig::getInstance().getChroot()->len();

    if (checkAccess(pPath))
        return LS_FAIL;

    return 0;
}


int ConfigCtx::checkAccess(char *pReal)
{
    if (HttpServerConfig::getInstance().getDeniedDir()->isDenied(pReal))
    {
        LS_ERROR(this, "Path is in the access denied list:%s", pReal);
        return LS_FAIL;
    }

    return 0;
}


int ConfigCtx::convertToRegex(const char   *pOrg, char *pDestBuf,
                              int bufLen)
{
    const char *p = pOrg;
    char *pDest = pDestBuf;
    char *pEnd = pDest + bufLen - 1;
    int n;

    while ((*p) && (pDest < pEnd - 3))
    {
        n = strspn(p, " ,");

        if (n)
            p += n;

        n = strcspn(p, " ,");

        if (!n)
            continue;

        if ((pDest != pDestBuf) && (pDest < pEnd))
            *pDest++ = ' ';

        if ((strncasecmp(p, "REGEX[", 6) != 0) &&
            ((memchr(p, '?', n)) || (memchr(p, '*', n))) &&
            (pEnd - pDest > 10))
        {
            const char *pB = p;
            const char *pE = p + n;
            memmove(pDest, "REGEX[", 6);
            pDest += 6;

            while ((pB < pE) && (pEnd - pDest > 3))
            {
                if ('?' == *pB)
                    *pDest++ = '.';
                else if ('*' == *pB)
                {
                    *pDest++ = '.';
                    *pDest++ = '*';
                }
                else if ('.' == *pB)
                {
                    *pDest++ = '\\';
                    *pDest++ = '.';
                }
                else
                    *pDest++ = *pB;

                ++pB;
            }

            *pDest++ = ']';
        }
        else
        {
            int len = n;

            if (pEnd - pDest < n)
                len = pEnd - pDest;

            memmove(pDest, p, len);
            pDest += len;
        }

        p += n;
    }

    *pDest = 0;
    *pEnd = 0;
    return pDest - pDestBuf;
}


XmlNode *ConfigCtx::parseFile(const char *configFilePath,
                              const char *rootTag)
{
    char achError[4096];
    XmlTreeBuilder tb;
    XmlNode *pRoot = tb.parse(configFilePath, achError, 4095);

    if (pRoot == NULL)
    {
        LS_ERROR(this, "%s", achError);
        return NULL;
    }

    // basic validation
    if (strcmp(pRoot->getName(), rootTag) != 0)
    {
        LS_ERROR(this, "%s: root tag expected: <%s>, real root tag : <%s>!\n",
                 configFilePath, rootTag, pRoot->getName());
        delete pRoot;
        return NULL;
    }

#ifdef TEST_OUTPUT_PLAIN_CONF
    char sPlainFile[512] = {0};
    lstrncpy(sPlainFile, configFilePath, sizeof(sPlainFile));
    lstrncat(sPlainFile, ".txt", sizeof(sPlainFile));
//    plainconf::testOutputConfigFile( pRoot, sPlainFile );
#endif

    return pRoot;

}


int ConfigCtx::configSecurity(AccessControl *pCtrl, const XmlNode *pNode)
{
    int c;
    const char *pValue;
    const XmlNode *pNode1 = pNode->getChild("accessControl");
    pCtrl->clear();

    if (pNode1)
    {
        pValue = pNode1->getChildValue("allow");

        if (pValue)
        {
            c = pCtrl->addList(pValue, true);

            LS_DBG_L(this, "add %d entries into allowed list.", c);
        }
        else
            LS_WARN(this, "Access Control: No entries in allowed list");

        pValue = pNode1->getChildValue("deny");

        if (pValue)
        {
            c = pCtrl->addList(pValue, false);

            LS_DBG_L(this, "add %d entries into denied list.", c);
        }
    }
    else
        LS_DBG_L(this, "no rule for access control.");
    return 0;
}



void ConfigCtx::configCRL(const XmlNode *pNode, SslContext *pSsl)
{
    char achCrlFile[MAX_PATH_LEN];
    char achCrlPath[MAX_PATH_LEN];
    const char *pCrlPath;
    const char *pCrlFile;
    pCrlPath = pNode->getChildValue("crlPath");
    pCrlFile = pNode->getChildValue("crlFile");
    if (pCrlPath)
    {
        if (getValidPath(achCrlPath, pCrlPath, "CRL path") != 0)
            return;
        pCrlPath = achCrlPath;
    }
    if (pCrlFile)
    {
        if (getValidFile(achCrlFile, pCrlFile, "CRL file") != 0)
            return;
        pCrlFile = achCrlFile;
    }

#ifdef _ENTERPRISE_
    if (pCrlPath || pCrlFile)
        pSsl->addCRL(achCrlFile, achCrlPath);
#endif
}


SslContext *ConfigCtx::newSSLContext(const XmlNode *pNode,
                                    const char *pName, SslContext *pOldContext)
{
    SslContextConfig config;
    int cv;
    SslContext *pSsl;
    const char *pTag, *pCertFile, *pKey;
    char achCert[MAX_PATH_LEN], achKey[MAX_PATH_LEN],
         achCAPath[MAX_PATH_LEN], achCAFile[MAX_PATH_LEN],
         achDHParam[MAX_PATH_LEN];

    if (pNode->getChild("certFile") == NULL)
    {
        LS_NOTICE( "[%s] No SSL certificate configured for [%s]",
                  getLogId(), pName);
        return NULL;
    }
    if ((pCertFile = getTag(pNode, "certFile")) == NULL)
        return NULL;
    else if ((pKey = getTag(pNode, "keyFile")) == NULL)
        return NULL;
    
    config.m_iEnableMultiCerts = 
                    HttpServerConfig::getInstance().getEnableMultiCerts();
    if (config.m_iEnableMultiCerts)
    {
        if (getAbsoluteFile(achCert, pCertFile) != 0)
            return NULL;
        else if (getAbsoluteFile(achKey, pKey) != 0)
            return NULL;
    }
    else
    {
        if (getValidFile(achCert, pCertFile, "certificate file") != 0)
            return NULL;
        else if (getValidFile(achKey, pKey, "key file") != 0)
            return NULL;
    }
    config.m_sCertFile[0] = achCert;
    config.m_sKeyFile[0] = achKey;
    config.m_sName = pName;

    const char *pCipher = pNode->getChildValue( "ciphers" );
    if (pCipher == NULL)
        pCipher = "ALL:!ADH:!EXPORT56:RC4+RSA:+HIGH:+MEDIUM:+SSLv2:+EXP";
    config.m_sCiphers = pCipher;

    if (( pTag = pNode->getChildValue( "CACertPath" )) != NULL )
    {
        if ( getValidFile(achCAPath, pTag, "CA Certificate path" ) != 0 )
            return NULL;
        config.m_sCAPath = achCAPath;
    }

    if (( pTag = pNode->getChildValue( "CACertFile" )) != NULL )
    {
        if ( getValidFile(achCAFile, pTag, "CA Certificate file" ) != 0 )
            return NULL;
        config.m_sCAFile = achCAFile;
    }


    cv = getLongValue( pNode, "clientVerify", 0, 3, 0 );
    config.m_iClientVerify = cv;

    config.m_iCertChain = getLongValue( pNode, "certChain", 0, 1, 0 );
    config.m_iProtocol = getLongValue(pNode, "sslProtocol", 1, 31, 28);
    config.m_iEnableECDHE = getLongValue(pNode, "enableECDHE", 0, 1, 1);
    config.m_iEnableDHE = getLongValue(pNode, "enableDHE", 0, 1, 0);

    if ( config.m_iEnableDHE != 0 )
    {
        if (( pTag = pNode->getChildValue("DHParam")) != NULL )
        {
            if ( getValidFile(achDHParam, pTag, "DH Parameter file" ) != 0 )
            {
                LS_WARN( "[%s] invalid path for DH paramter: %s, "
                         "ignore and use built-in DH parameter!",
                         getLogId(), pTag );
            }
            else
                config.m_sDHParam = achDHParam;
        }
    }

    config.m_iEnableSpdy = getLongValue(pNode, "enableSpdy", 0, 7, 7);
    config.m_iEnableCache = getLongValue(pNode, "sslSessionCache", 0, 1, 0);
    config.m_iInsecReneg = !getLongValue(pNode, "regenProtection", 0, 1, 1);
    config.m_iEnableTicket = getLongValue(pNode, "sslSessionTickets",
                                               0, 1, 1);
    int enableStapling = getLongValue(pNode, "enableStapling", 0, 1, 0);
    if ((enableStapling) && (pCertFile != NULL))
        if (configStapling(pNode, &config) != -1)
            config.m_iEnableStapling = enableStapling;

    pSsl = SslContext::config(pOldContext, &config);
    if ( pSsl == NULL )
    {
        LS_ERROR( "[%s] failed to create new SSLContext for %s",
                  getLogId(), pName );
        return NULL;
    }

    if (cv)
    {
#ifdef _ENTERPRISE_
        pSsl->setClientVerify(cv, getLongValue(pNode, "verifyDepth", 1, INT_MAX,
                                               1));
#endif
        configCRL(pNode, pSsl);
    }

    return pSsl;
}


int ConfigCtx::initOcspCachePath()
{
    if (SslOcspStapling::getCachePath() == NULL)
    {
        char achBuf[MAX_PATH_LEN];
        if (getAbsolutePath(achBuf, "$SERVER_ROOT/tmp/ocspcache/") != -1)
            SslOcspStapling::setCachePath(achBuf);
        else
            return -1;
    }
    return 0;
}

int ConfigCtx::configStapling(const XmlNode *pNode,
                                      SslContextConfig *pConf)
{
    if (initOcspCachePath() == -1)
        return -1;
    pConf->m_iOcspMaxAge = getLongValue(pNode, "ocspRespMaxAge", 60,
                                        360000, 3600);
    const char *pResponder = pNode->getChildValue("ocspResponder");
    if (pResponder)
        pConf->m_sOcspResponder.setStr(pResponder);
    const char *pCombineCAfile = pNode->getChildValue("ocspCACerts");
    char CombineCAfile[MAX_PATH_LEN];
    if (pCombineCAfile)
    {
        if (getValidFile(CombineCAfile, pCombineCAfile, "Combine CA file") != 0)
            return 0;
        pCombineCAfile = CombineCAfile;

        //pSslOcspStapling->setCombineCAfile ( pCombineCAfile );
    }
    //else
    //    pCombineCAfile = pCAFile;
    if (pCombineCAfile)
        pConf->m_sCaChainFile.setStr(pCombineCAfile);
    return 0;
}

static int bioToBuf(BIO *pBio, AutoBuf *pBuf)
{
    int len, written;
    len = BIO_number_written(pBio);
    pBuf->reserve(pBuf->size() + len + 1);
    written = BIO_read(pBio, pBuf->end(), len);

    if (written <= 0)
    {
        if (!BIO_should_retry(pBio))
            return -1;
        written = BIO_read(pBio, pBuf->end(), len);
        if (written <= 0)
            return -1;
    }
    if (written != len)
        return -1;

    pBuf->used(written - 1); // - 1 to get rid of extra newline.
    return written;
}

int  ConfigCtx::getPrivateKeyPem(SSL_CTX *pCtx, AutoBuf *pBuf)
{
    int ret = -1;
    BIO *pOut = BIO_new(BIO_s_mem());
    EVP_PKEY *pKey = SSL_CTX_get0_privatekey(pCtx);

    if (PEM_write_bio_PrivateKey(pOut, pKey, NULL, NULL, 0, NULL, NULL))
        ret = bioToBuf(pOut, pBuf);
    BIO_free(pOut);
    return ret;
}

int  ConfigCtx::getCertPem(SSL_CTX *pCtx, AutoBuf *pBuf)
{
    int ret = -1;
    BIO *pOut = BIO_new(BIO_s_mem());
    X509 *pCert = SSL_CTX_get0_certificate(pCtx);

    if (PEM_write_bio_X509(pOut, pCert))
        ret = bioToBuf(pOut, pBuf);
    BIO_free(pOut);
    return ret;
}

int  ConfigCtx::getCertChainPem(SSL_CTX *pCtx, AutoBuf *pBuf)
{
    int i, cnt, ret = -1;
    X509 *pCert;
    STACK_OF(X509) *pChain;
    BIO *pOut = BIO_new(BIO_s_mem());

    if (!SSL_CTX_get_extra_chain_certs(pCtx, &pChain))
    {
        return 0;
    }
    else if (NULL == pChain)
    {
        return 0;
    }
    cnt = sk_X509_num(pChain);

    for (i = 0; i < cnt; ++i)
    {
        pCert = sk_X509_value(pChain, i);
        if (PEM_write_bio_X509(pOut, pCert))
            ret = bioToBuf(pOut, pBuf);
        BIO_free(pOut);
    }
    return ret;
}


