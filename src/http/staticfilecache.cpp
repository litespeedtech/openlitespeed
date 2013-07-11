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
#include "staticfilecache.h"
#include "staticfilecachedata.h"
#include "httpglobals.h"
#include "httpmime.h"
#include "httpreq.h"
#include "httpstatuscode.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


StaticFileCache::StaticFileCache( int initSize)
    : HttpCache( initSize )
{
}
StaticFileCache::~StaticFileCache()
{
}


int StaticFileCache::getCacheElement(
        HttpReq * pReq, StaticFileCacheData* &pData )
{
    const MIMESetting* pMime = pReq->getMimeType();
    if ( !pData )
    {
        HttpCache::iterator iter = find( pReq->getRealPath()->c_str() );
        if ( iter )
        {
            pData = (StaticFileCacheData* )(iter.second());
        }
    }
    if ( pData )
    {
        if ( pData->isDirty( pReq->getFileStat(),
                    pMime, pReq->getDefaultCharset() ) )
        {
            if ( dirty( pData ) )
                return SC_500;
        }
        else
            return 0;
    }

    int ret = newCache( pReq, pMime, pData );
    if ( ret  )
        return ret;
    add( pData );
    
    return 0;
}

int StaticFileCache::newCache( const HttpReq * pReq,
     const MIMESetting* pMime, StaticFileCacheData *& pData )
{
    int ret = SC_500;
    pData = ( StaticFileCacheData* )allocElement();
    if ( pData != NULL )
    {
        pData->setMimeType( pMime );
        pData->setCharset( pReq->getDefaultCharset() );
//         ret = pData->build( *pReq->getRealPath(),
//                             pReq->getFileStat() );
        ret = pData->build( *pReq->getRealPath(),
                            pReq->getFileStat(), pReq->getETagFlags() );        
        if ( ret )
        {
            delete pData;
            pData = NULL;
        };
    }
    return ret;
}


CacheElement* StaticFileCache::allocElement()
{
    return new StaticFileCacheData();
}

void StaticFileCache::recycle( CacheElement* pElement )
{
    if ( pElement )
        delete pElement;
}
