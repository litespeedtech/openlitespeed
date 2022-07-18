/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2022  LiteSpeed Technologies, Inc.                 *
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
#ifndef REQSTATS_H
#define REQSTATS_H



class ReqStats
{
    int     m_iReqPerSec;
    int     m_iTotalReqs;
    int     m_iStxCacheHitsPerSec;
    int     m_iTotalStxCacheHits;

    int     m_iPubCacheHitsPerSec;
    int     m_iTotalPubCacheHits;
    int     m_iPrivCacheHitsPerSec;
    int     m_iTotalPrivCacheHits;


    ReqStats(const ReqStats &rhs);
    void operator=(const ReqStats &rhs);
public:
    ReqStats();
    ~ReqStats();
    void incReqProcessed()  {   ++m_iReqPerSec;         }
    void incStxCacheHits()  {   ++m_iStxCacheHitsPerSec;         }
    void incPubCacheHits()  {   ++m_iPubCacheHitsPerSec;         }
    void incPrivCacheHits()  {   ++m_iPrivCacheHitsPerSec;         }

    int  getRPS() const     {   return m_iReqPerSec;    }
    int  getTotal() const   {   return m_iTotalReqs;    }
    int  getHitsPS() const     {   return m_iStxCacheHitsPerSec;    }
    int  getTotalHits() const  {   return m_iTotalStxCacheHits;     }

    int  getPubHitsPS() const     {   return m_iPubCacheHitsPerSec;    }
    int  getTotalPubHits() const  {   return m_iTotalPubCacheHits;     }
    int  getPrivHitsPS() const     {   return m_iPrivCacheHitsPerSec;    }
    int  getTotalPrivHits() const  {   return m_iTotalPrivCacheHits;     }



    void reset()
    {
        m_iReqPerSec = 0;
        m_iStxCacheHitsPerSec = 0;
        m_iPubCacheHitsPerSec = 0;
        m_iPrivCacheHitsPerSec = 0;
    }

    void resetTotal()
    {
        m_iTotalReqs = 0;
        m_iTotalStxCacheHits = 0;
        m_iTotalPubCacheHits = 0;
        m_iTotalPrivCacheHits = 0;
    }

    void finalizeRpt();
};

#endif

