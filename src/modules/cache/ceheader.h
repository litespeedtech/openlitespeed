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
#ifndef CEHEADER_H
#define CEHEADER_H


#include <inttypes.h>

#define CE_ID "LSCH"

struct CeHeader
{
public:
    CeHeader();

    ~CeHeader();

    enum
    {
        CEH_COMPRESSIBLE = 1,
        CEH_COMPRESSED   = 1 << 1,
        CEH_IN_CONSTRUCT = 1 << 2,
        CEH_PRIVATE      = 1 << 3,
        CEH_STALE        = 1 << 4,
        CEH_UPDATING     = 1 << 5
    };

    int32_t m_tmCreated;        //Created Time
    int32_t m_tmExpire;         //Expire Time
    int32_t m_iFlag;             //Combination of CEH_xxx flags
    int32_t m_iKeyLen;           //Cache Key Length
    int32_t m_iStatusCode;       //Response Status Code
    int32_t m_iValPart1Len;      //Response Header Length
    int32_t m_iValPart2Len;      //Response Body Length
    int32_t m_tmLastMod;        //Last Modified time parsed from response header if set
    int16_t m_iOffETag;          //ETag header value location
    int16_t m_iLenETag;          //ETag Header size

};

#endif
