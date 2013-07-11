/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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
#ifndef STATICFILECACHE_H
#define STATICFILECACHE_H


#include <http/httpcache.h>

class HttpReq;
class StaticFileCacheData;
class MIMESetting;

class StaticFileCache : public HttpCache
{
    CacheElement* allocElement();
    void recycle( CacheElement* pElement );
    int  newCache( const HttpReq * pReq,
         const MIMESetting *pMime, StaticFileCacheData *&pData );
public:
    StaticFileCache( int initSize );
    ~StaticFileCache();

    int getCacheElement( HttpReq* pReq, StaticFileCacheData* &pData );
    void returnCacheElement( StaticFileCacheData* pElement );
};

#endif
