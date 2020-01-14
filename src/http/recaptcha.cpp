/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#include "recaptcha.h"

#include <log4cxx/logger.h>
#include <util/autobuf.h>
#include <util/autostr.h>
#include <util/pcregex.h>
#include <util/stringlist.h>

#include <limits.h>
#include <stdio.h>

HttpContext *Recaptcha::s_pStaticCtx = NULL;
HttpContext *Recaptcha::s_pDynCtx = NULL;
AutoStr2 *Recaptcha::s_pStaticUrl = new AutoStr2("/.lsrecap/_recaptcha.shtml");
AutoStr2 *Recaptcha::s_pDynUrl = new AutoStr2("/.lsrecap/recaptcha");
AutoStr2 *Recaptcha::s_pEnvNameSiteKey = new AutoStr2("LSRECAPTCHA_SITE_KEY");
AutoStr2 *Recaptcha::s_pEnvNameSecretKey = new AutoStr2("LSRECAPTCHA_SECRET_KEY");
AutoStr2 *Recaptcha::s_pEnvNameTypeParam = new AutoStr2("LSRECAPTCHA_TYPE_PARAM");
const char *Recaptcha::s_pDefaultSiteKey = "6Ld4O3QUAAAAANo81VuJbgKdb2vqLQyrh28VFyas";
const char *Recaptcha::s_pDefaultSecretKey = "6Ld4O3QUAAAAAIdc-9U6-NgjIYgTjgduemYA_MUp";

const char *Recaptcha::s_apTypeParams[Recaptcha::TYPE_END] =
{
    NULL,
    NULL,
    "'size': 'invisible'"
};


void Recaptcha::setStaticUrl(const char *url)
{
    s_pStaticUrl->setStr(url);
}


Recaptcha::Recaptcha()
    : m_iType(TYPE_UNKNOWN)
    , m_iMaxTries(3)
    , m_iRegConnLimit(INT_MAX)
    , m_iSslConnLimit(INT_MAX)
    , m_pSiteKey(NULL)
    , m_pSecretKey(NULL)
    , m_pBotWhitelist(NULL)
{
}


Recaptcha::~Recaptcha()
{
    if (m_pSiteKey)
        delete m_pSiteKey;
    if (m_pSecretKey)
        delete m_pSecretKey;
    if (m_pBotWhitelist)
        delete m_pBotWhitelist;
}


void Recaptcha::setSiteKey(const char *p)
{
    m_pSiteKey = new AutoStr2(p);
}


void Recaptcha::setSecretKey(const char *p)
{
    m_pSecretKey = new AutoStr2(p);
}


void Recaptcha::setBotWhitelist(const StringList *pList)
{
    StringList::const_iterator iter;
    Pcregex *pRegex = NULL;
    AutoBuf buf;
    const char *pPattern;
    int iPatternLen;

    if (pList->size() > 1)
    {
        for (iter = pList->begin(); iter != pList->end(); ++iter)
        {
            pPattern = (*iter)->c_str();
            iPatternLen = (*iter)->len();
            if (buf.guarantee(iPatternLen + 6) != LS_FAIL)
            {
                int len = snprintf(buf.end(), buf.available(), "(?:%.*s)|",
                                    iPatternLen, pPattern);
                buf.used(len);
            }
            else
                LS_NOTICE("reCAPTCHA unable to allocate space for bot whitelist, %.*s. Skip append.",
                          iPatternLen, pPattern);
        }
        if (!buf.empty())
        {
            buf.pop_end(1);
            buf.append_unsafe('\0');
        }
        pPattern = buf.begin();
    }
    else
    {
        iter = pList->begin();
        pPattern = (*iter)->c_str();
    }

    pRegex = new Pcregex();
    if (pRegex->compile(pPattern, REG_EXTENDED) != 0)
    {
        LS_ERROR("Failed to compile recaptcha regex %s", buf.begin());
        delete pRegex;
    }
    else
        m_pBotWhitelist = pRegex;
}


bool Recaptcha::isBotWhitelisted(const char *pAgent, int iAgentLen) const
{
    if (NULL == m_pBotWhitelist)
        return false;
    return (m_pBotWhitelist->exec(pAgent, iAgentLen, 0, 0, NULL, 0) >= 0);
}
