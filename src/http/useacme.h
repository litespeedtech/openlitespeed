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
#ifndef USEACME_H
#define USEACME_H

#include <util/autostr.h>
#include <util/gpointerlist.h>
#include <util/hashstringmap.h>
#include <util/tsingleton.h>

class AcmeAlias
{
public:
    AcmeAlias()     {}
    ~AcmeAlias()    {}
    const char *getAlias()          {   return m_alias.c_str();     }
    void setAlias(const char *a)    {   m_alias = a;                }
private:
    AutoStr2    m_alias;
};

class AcmeAliasMap : public HashStringMap< AcmeAlias * >
{
public:
    AcmeAliasMap()  {}
    ~AcmeAliasMap() {   release_objects();  }
    AcmeAlias *addAlias(const char *primary, const char *alias);
};

class AcmeCert 
{
public:
    AcmeCert() 
        : m_renew(false)  
        , m_force(false)
        , m_wildcard(false)
        , m_vhostOk(false)
        , m_pDnsApi(NULL)
        , m_pDnsEnv(NULL)   {}
    ~AcmeCert()     {}
    void setDomainAddr(const char *domain, const char *addr)
        {   m_domain = domain; m_addr = addr;   }
    const char *getDomain()         {   return m_domain.c_str();    }
    const char *getAddr()           {   return m_addr.c_str();      }
    void setVhost(const char *v)    {   m_vhost = v;                }
    const char *getVhost()          {   return m_vhost.c_str();     }
    AcmeAliasMap *getAcmeAliasMap() {   return &m_acmeAliasMap;     }
    void setRenew(bool renew)       {   m_renew = renew;            }
    bool getRenew()                 {   return m_renew;             }
    void setForce(bool force)       {   m_force = force;            }
    bool getForce()                 {   return m_force;             }
    void setWildcard(bool wildcard) {   m_wildcard = wildcard;      }
    bool getWildcard()              {   return m_wildcard;          }
    void setVhostOk(bool ok)        {   m_vhostOk = ok;             }
    bool getVhostOk()               {   return m_vhostOk;           }
    void setDnsApi(const char *p)   {   m_pDnsApi = p;              }
    const char *getDnsApi()         {   return m_pDnsApi;           }
    void setDnsEnv(const char *p)   {   m_pDnsEnv = p;              }
    const char *getDnsEnv()         {   return m_pDnsEnv;           }
private:
    AutoStr2        m_domain;
    AutoStr2        m_addr;
    AutoStr2        m_vhost;
    AcmeAliasMap    m_acmeAliasMap;
    bool            m_renew;
    bool            m_force;
    bool            m_wildcard;
    bool            m_vhostOk;
    const char     *m_pDnsApi;
    const char     *m_pDnsEnv;
};

struct ifaddrs;
class AutoBuf;
class GSockAddr;
class HttpVHost;
class XmlNode;
/* The key is the primary domain */
class AcmeCertMap : public HashStringMap< AcmeCert * >, public TSingleton<AcmeCertMap>
{
public:
   AcmeCertMap()  
        : m_ifaddrs(NULL)
        , m_pidRunning(-1)   {}
    ~AcmeCertMap();
    void finished();
    struct ifaddrs *getIfAddrs();
    AcmeCert *doQueue(const char *vhost, const char *domain, bool vhostOk,
                      const char *pAddr);
    bool beginCerts(bool programStart);
    int acmeRequest(char *acmeFile, char *desc, char * const *argv, 
                    const char *env);
    bool validateDomain(const char *domain, const char *pAddr);
    bool isRunning(char *configDir);
private:
    char **getEnv(char **argv);
    struct ifaddrs *isMatch(GSockAddr *sockaddr, void *addr);
    void processCerts(char *acmeDir, char *configDir, bool programStart);
    int envCount(char *pos);
    void deleteCert(AcmeCert *acmeCert);
    int reportStd(AutoBuf *autobuf, const char *fdname, int fd, 
                  char *msg, int *pos, int *closed);
    void reportStds(AutoBuf *autoBuf, int outfd, int errfd);
    void reportAutoBuf(const char *desc, AutoBuf *autobuf, int err);
    struct ifaddrs *m_ifaddrs;
    pid_t           m_pidRunning;
    const int       s_num_env = 4;
};
LS_SINGLETON_DECL(AcmeCertMap);

class SslContext;
class UseAcme
{
public:
    UseAcme()
        : m_renewal(0)
        , m_wildcard(false)
        , m_vhostOk(false)
        , m_sslContext(NULL)   {}
    ~UseAcme() {}

    static bool acmeInstalled(char *acmeDir, char *configDir, int dirlen);
    static UseAcme *acmeFound(const XmlNode *pNode, const char *pAddr);
    static UseAcme *acmeVhost(HttpVHost *vhost, const char *domain, 
                              const char *aliases, const char *pAddr);
    static UseAcme *vhostActivate(HttpVHost *vhost);
    static char *getServer()            {   return s_server;            }
    const char *getDomain()             {   return m_domain.c_str();    }
    void setDomain(const char *d)       {   m_domain = d;               }
    const char *getVhost()              {   return m_vhost.c_str();     }
    void setVhost(const char *v)        {   m_vhost = v;                }
    const char *getAddr()               {   return m_addr.c_str();      }
    const char *getKey()                {   return m_key.c_str();       }
    const char *getCert()               {   return m_cert.c_str();      }
    const char *getCertPath()           {   return m_certPath.c_str();  }
    const char *getCA()                 {   return m_CA.c_str();        }
    const char *getFullChain()          {   return m_fullChain.c_str(); }
    AcmeAliasMap *getAcmeAliasMap()     {   return &m_acmeAliasMap;     }
    time_t getRenewal()                 {   return m_renewal;           }
    bool getWildcard()                  {   return m_wildcard;          }
    void setWildcard(bool wildcard)     {   m_wildcard = wildcard;      }
    void setVhostOk(bool ok)            {   m_vhostOk = ok;             }
    bool getVhostOk()                   {   return m_vhostOk;           }
    void setSslContext(SslContext *c);
    SslContext *getSslContext()         {   return m_sslContext;        }
    bool rereadRenewal(char *configDir);
    static const char *getFileDomain(const char *domain);
    static char *getDomainDir(char *configDir, char dir[], size_t dirlen);
    static char *getDomainDirName(char *configDir, const char *domain, char dir[], 
                                  size_t dirlen);
    static FILE *openConfFile(char *configDir, const char *domain, char *confFile);
    static UseAcme *setupRenew(UseAcme *useAcme, char *confFile, bool force);
private:
    static char *getDomainDirNameOnly(const char *domain, char *dironly, 
                                      size_t dironlylen);
    static UseAcme *addDomain(char *configDir, const char *domain, const char *vhostName, 
                              bool vhostOk, const char *pAddr, AcmeAliasMap **acmeAliasMap, 
                              AcmeCert **acmeCert);
    static UseAcme *validateAcme(char *configDir, UseAcme *useAcme);
    static time_t findRenewal(FILE *fp);
    static char *getDomainFromDomains(const char *vhost, const char **domains, 
                                      char *domain, size_t domain_len);
    static bool processMap(char *configDir, const char *map, const char *pAddr, 
                           UseAcme **firstUseAcme, UseAcme **useAcmeMap);
    static bool processDomains(char *configDir, const char *vhost, bool vhostOk,
                               const char *domains, const char *pAddr,
                               UseAcme **firstUseAcme, UseAcme **useAcmeMap);
    static void failMap(UseAcme **useAcme, AcmeCert **acmeCert, 
                        UseAcme **firstUseAcme, UseAcme **useAcmeMap);

    AutoStr2        m_domain;
    AutoStr2        m_vhost;
    AutoStr2        m_addr;
    AutoStr2        m_key;
    AutoStr2        m_cert;
    AutoStr2        m_certPath;
    AutoStr2        m_CA;
    AutoStr2        m_fullChain;
    AcmeAliasMap    m_acmeAliasMap;
    time_t          m_renewal;
    bool            m_wildcard;
    bool            m_vhostOk;
    SslContext     *m_sslContext;
    static bool     s_testedInstalled;
    static bool     s_installed;
    static char    *s_server;
};

/* The key is the domain */
class UseAcmeMap : public HashStringMap< UseAcme * >, public TSingleton<UseAcmeMap>
{
public:
    UseAcmeMap()
        :   m_lastRenew(0)  {} 
    ~UseAcmeMap() {}
    void findDeleted();
    bool deleteFromAcme(char *acmeDir, char *configDir, const char *domain);
    bool deleteDir(char *configDir, char *domainDir);
    void renewCerts();
private:
    time_t m_lastRenew;
};
LS_SINGLETON_DECL(UseAcmeMap);

class AcmeCertList : public TPointerList<AcmeCert>
{
public:
    AcmeCertList() {}
    ~AcmeCertList() {}
};

class UseAcmeList : public TPointerList<UseAcme>
{
public:
    UseAcmeList() {}
    ~UseAcmeList() {}
};

class AcmeLists
{
public:
    AcmeLists() {}
    ~AcmeLists() {}
    AcmeCertList *getAcmeCertList()     {   return &m_acmeCertList;     }
    UseAcmeList *getUseAcmeList()       {   return &m_useAcmeList;      }
private:
    AcmeCertList    m_acmeCertList;
    UseAcmeList     m_useAcmeList;
};

/* The key is the vhost */
class AcmeVHostMap : public HashStringMap<AcmeLists *>, public TSingleton<AcmeVHostMap>
{
public:
    AcmeVHostMap()   {}
    ~AcmeVHostMap();
    int insertAcmeCert(AcmeCert *acmeCert);
    int insertUseAcme(UseAcme *useAcme);
    int removeAcmeCert(AcmeCert *acmeCert);
    int removeUseAcme(UseAcme *useAcme);
};
LS_SINGLETON_DECL(AcmeVHostMap);


#endif
