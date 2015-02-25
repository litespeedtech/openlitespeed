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
#ifndef HASHDATACACHE_H
#define HASHDATACACHE_H



#include <stdio.h>
#include <sys/types.h>
#include <util/hashstringmap.h>

class KeyData;

class HashDataCache : public HashStringMap< KeyData *>
{
    int     m_cacheExpire;
    int     m_maxCacheSize;

    HashDataCache(const HashDataCache &);
    HashDataCache &operator=(const HashDataCache &);
public:
    HashDataCache(int expire, int cacheSize)
        : m_cacheExpire(expire)
        , m_maxCacheSize(cacheSize)
    {}

    HashDataCache()
        : m_cacheExpire(10)
        , m_maxCacheSize(1024)
    {}
    ~HashDataCache();
    void setExpire(int expire)    {   m_cacheExpire = expire;     }
    int  getExpire() const          {   return m_cacheExpire;       }
    void setMaxSize(int size)     {   m_maxCacheSize = size;      }
    int  getMaxSize() const         {   return m_maxCacheSize;      }

    const KeyData *getData(const char *pKey);
};

class DataStore
{
    char   *m_uriDataStore;

public:
    DataStore()
        : m_uriDataStore(NULL)
    {}
    virtual ~DataStore();
    void setDataStoreURI(const char *pURI);
    const char *getDataStoreURI() const   {   return m_uriDataStore;  }

    virtual KeyData *getDataFromStore(const char *pKey, int len) = 0;
    virtual int       isStoreChanged(long time) = 0;
    virtual KeyData *newEmptyData(const char *pKey, int len) = 0;
    virtual KeyData *getNext()     {   return NULL;    }
};

class FileStore : public DataStore
{
    time_t        m_lModifiedTime;
    time_t        m_lLastCheckTime;

    FILE         *m_fp;

protected:
    KeyData *getDataFromStore(const char *pKey, int keyLen);
    virtual KeyData *parseLine(char *pLine, char *pLineEnd) = 0;
    virtual KeyData *parseLine(const char *pKey, int keyLen,
                               char *pLine, char *pLineEnd) = 0;

public:
    FileStore()
        : m_lModifiedTime(1)
        , m_lLastCheckTime(0)
        , m_fp(NULL)
    {}
    virtual ~FileStore() {}
    virtual int isStoreChanged(long time);
    virtual KeyData *getNext();
    int open();
    void close();
};

#endif
