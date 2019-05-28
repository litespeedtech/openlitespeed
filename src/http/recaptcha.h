/*
 * Copyright 2002 Lite Speed Technologies Inc, All Rights Reserved.
 * LITE SPEED PROPRIETARY/CONFIDENTIAL.
 */

#ifndef RECAPTCHA_H
#define RECAPTCHA_H

#include <inttypes.h>

class AutoStr2;
class HttpContext;
class Pcregex;
class StringList;
class Recaptcha
{
public:
    enum
    {
        TYPE_UNKNOWN,
        TYPE_V2,
        TYPE_INVISIBLE,
        TYPE_END,
        TYPE_V3, // not out yet.
    };
    
    Recaptcha();
    ~Recaptcha();

    void setType(uint16_t t)                {   m_iType = t;                }
    int getType() const                     {   return m_iType;             }

    void setMaxTries(uint16_t t)            {   m_iMaxTries = t;            }
    uint16_t getMaxTries() const            {   return m_iMaxTries;         }

    void setRegConnLimit(int l)             {   m_iRegConnLimit = l;        }
    int getRegConnLimit() const             {   return m_iRegConnLimit;     }

    void setSslConnLimit(int l)             {   m_iSslConnLimit = l;        }
    int getSslConnLimit() const             {   return m_iSslConnLimit;     }

    void setSiteKey(const char *pSiteKey);
    void setSiteKey(AutoStr2 *p)            {   m_pSiteKey = p;             }
    const AutoStr2 *getSiteKey() const      {   return m_pSiteKey;          }

    void setSecretKey(const char *pSiteKey);
    void setSecretKey(AutoStr2 *p)          {   m_pSecretKey = p;           }
    const AutoStr2 *getSecretKey() const    {   return m_pSecretKey;        }

    const char *getTypeParam() const    {   return s_apTypeParams[m_iType]; }

    void setBotWhitelist(const StringList *pList);
    bool isBotWhitelisted(const char *pAgent, int iAgentLen) const;

    static void setStaticUrl(const char *url);
    static const AutoStr2   *getStaticUrl() {   return s_pStaticUrl;        }
    static HttpContext      *getStaticCtx() {   return s_pStaticCtx;        }
    static void setStaticCtx(HttpContext *c){   s_pStaticCtx = c;           }
    static const AutoStr2      *getDynUrl() {   return s_pDynUrl;           }
    static HttpContext         *getDynCtx() {   return s_pDynCtx;           }
    static void   setDynCtx(HttpContext *c) {   s_pDynCtx = c;              }

    static const AutoStr2 *getSiteKeyName()   { return s_pEnvNameSiteKey;   }
    static const AutoStr2 *getSecretKeyName() { return s_pEnvNameSecretKey; }
    static const AutoStr2 *getTypeParamName() { return s_pEnvNameTypeParam; }

    static const char *getDefaultSiteKey()    { return s_pDefaultSiteKey;   }
    static const char *getDefaultSecretKey()  { return s_pDefaultSecretKey; }

private:
    uint16_t            m_iType;
    uint16_t            m_iMaxTries;
    int                 m_iRegConnLimit;
    int                 m_iSslConnLimit;
    AutoStr2           *m_pSiteKey;
    AutoStr2           *m_pSecretKey;
    Pcregex            *m_pBotWhitelist;

    static AutoStr2    *s_pStaticUrl;
    static HttpContext *s_pStaticCtx;
    static AutoStr2    *s_pDynUrl;
    static HttpContext *s_pDynCtx;
    static AutoStr2    *s_pEnvNameSiteKey;
    static AutoStr2    *s_pEnvNameSecretKey;
    static AutoStr2    *s_pEnvNameTypeParam;
    static const char  *s_pDefaultSiteKey;
    static const char  *s_pDefaultSecretKey;

    static const char  *s_apTypeParams[TYPE_END];
};

#endif
