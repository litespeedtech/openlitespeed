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
#ifndef STATICFILEDATA_H
#define STATICFILEDATA_H


  
#include <string.h>

class StaticFileCacheData;
class FileCacheDataEx;

class StaticFileData 
{
    StaticFileCacheData * m_pCache;
    FileCacheDataEx     * m_pECache;
    void                * m_pParam;
    
    long     m_lCurPos;
    long     m_lCurEnd;
    
    StaticFileData( const StaticFileData& rhs );
    void operator=( const StaticFileData& rhs );

public:

    StaticFileData();
    ~StaticFileData();

    void reset()
    {
        //m_pCache = NULL;
        //memset( &m_pCache, 0, (char *)(&m_pECache + 1) - (char *)&m_pCache );
        memset( this, 0, sizeof( *this ) );
    }

    StaticFileCacheData * getCache() const
    {   return m_pCache;    }
    StaticFileCacheData *& getCache()
    {   return m_pCache;    }
    
    
    void * getParam() const     {   return m_pParam;    }
    void setParam( void * p )   {   m_pParam = p;       }
    
    void setCache( StaticFileCacheData* pCache )
    {   m_pCache = pCache;      }

    void setECache( FileCacheDataEx* pCache ) {   m_pECache = pCache;   }

    FileCacheDataEx * getECache() const
    {   return m_pECache;    }
    FileCacheDataEx * &getECache() 
    {   return m_pECache;    }
    
    void setCurPos( long pos )  {   m_lCurPos = pos;    }
    void incCurPos( long inc )  {   m_lCurPos += inc;   }
    long getCurPos() const      {   return m_lCurPos;   }
    void setCurEnd( long end )  {   m_lCurEnd = end;    }
    long getCurEnd() const      {   return m_lCurEnd;   }
    long getRemain() const  {   return m_lCurEnd - m_lCurPos;   }
};

#endif
