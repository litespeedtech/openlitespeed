/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2015  LiteSpeed Technologies, Inc.                 *
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
#ifndef SSLSESSCACHE_H
#define SSLSESSCACHE_H

#include <lsdef.h>
#include <shm/lsshmtypes.h>
#include <util/hashstringmap.h>
#include <util/tsingleton.h>
#include <stdint.h>
#include <unistd.h>


#define LS_SSLSESSCACHE_DEFAULTSIZE 40*1024

class LsShmHash;

class LsShmHashObserver;
typedef struct SslSessData_s SslSessData_t;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct ssl_session_st SSL_SESSION;

class SslClientSessElem;
typedef LsStrHashMap<SslClientSessElem *> SslClientSessHash;

//
//  SslSessCache
//

class SslSessCache : public TSingleton<SslSessCache>
{
    friend class TSingleton<SslSessCache>;

public:
    static int watchCtx(SSL_CTX *pCtx);

    int     init(int32_t iTimeout, int iMaxEntries, int uid, int gid);
    int     isReady() const              {   return m_pSessStore != NULL;   }
    int32_t getExpireSec() const         {   return m_expireSec;            }
    void    setExpireSec(int32_t iExpire){   m_expireSec = iExpire;         }

    int     sessionFlush();
    int     stat();
    int     addSession(time_t lruTm, const uint8_t *pId, int idLen,
                       unsigned char *pData, int iDataLen)
    {
        return (addSessionEx(getSessStore(), lruTm, pId, idLen,
                             pData, iDataLen) != 0);
    }

    LsShmOffset_t addSessionEx(LsShmHash *pHash, time_t lruTm, const uint8_t *pId,
                        int idLen, unsigned char *pData, int iDataLen);
    SSL_SESSION *getSession(unsigned char *id, int len);
    SslSessData_t *getLockedSessionData(const unsigned char *id, int len, LsShmHash *&pHash);
    int deleteSession(const char *pId, int len);
    int deleteSessionEx(LsShmHash *pHash, const char *pId, int len);

    void setObserver(LsShmHashObserver *pObserver)
    {   m_pObserver = pObserver;        }
    LsShmHashObserver *getObserver() const
    {   return m_pObserver;         }

    void setRemoteStore(LsShmHash *pHash)
    {   m_pRemoteStore = pHash;         }
    LsShmHash *getRemoteStore() const
    {   return m_pRemoteStore;          }

    void    unlock(LsShmHash *pHash);
    LsShmHash *getSessStore() const
    {   return m_pSessStore;    }

    uint32_t    getSessCnt() const;

private:
    SslSessCache();
    ~SslSessCache();
    int    initShm(int uid, int gid);
    SslSessData_t *getLockedSessionDataEx(LsShmHash *pHash,
            const unsigned char *id, int len);

private:
    int32_t                 m_expireSec;
    int                     m_maxEntries;
    LsShmHash              *m_pSessStore;
    LsShmHash              *m_pRemoteStore;
    LsShmHashObserver      *m_pObserver;

    LS_NO_COPY_ASSIGN(SslSessCache);
};

LS_SINGLETON_DECL(SslSessCache);

class SslClientSessCache
{
public:
    SslClientSessCache()
        : m_sessions()
        {}
    ~SslClientSessCache();

    int saveSession(const char *id, int len, SSL_SESSION *session);
    SSL_SESSION *getSession(const char *id, int len);

    static void onTimer();

private:
    SslClientSessHash   m_sessions;
    LS_NO_COPY_ASSIGN(SslClientSessCache);
};


#endif // SSLSESSCACHE_H

