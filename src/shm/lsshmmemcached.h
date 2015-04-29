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
#ifndef LSSHMMEMCACHED_H
#define LSSHMMEMCACHED_H

#include <lsdef.h>
#include <shm/lsshmhash.h>
#include <lsr/ls_str.h>


typedef struct
{
    uint32_t x_exptime;
    uint32_t x_flags;
    union
    {
        struct
        {
            uint64_t cas;
            uint8_t val[1];
        } withcas;
        uint8_t val[1];
    } x_data[];
} LsMcDataItem;

class LsShmMemCached;
typedef struct
{
    const char *cmd;
    int arg;
    int (*func)(LsShmMemCached *pThis, char *pStr, int arg);
} LsMcCmdFunc;

class LsShmMemCached
{
public:
    LsShmMemCached(LsShmHash *pHash)
      : m_pHash(pHash)
      , m_iterOff(0)
      , m_needed(0)
      , m_noreply(false)
      , m_usecas(true)
    {   ls_str_blank(&m_parms.key); ls_str_blank(&m_parms.value);  }
    ~LsShmMemCached() {}

    int  processCmd(char *pStr);
    int  doDataUpdate(char *pBuf);
    
private:
    LsShmMemCached(const LsShmHash &other);
    LsShmMemCached &operator=(const LsShmMemCached &other);

private:
    static int   doCmdGet(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdUpdate(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdDelete(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdQuit(LsShmMemCached *pThis, char *pStr, int arg);
    LsMcCmdFunc *getCmdFunction(char *pCmd);
    void  respond(const char *str);
    void  sendResult(const char *fmt, ...);
    char *advToken(char *pStr, char **pTokPtr, int *pTokLen);
    bool  myStrtol(const char *pStr, long *pVal);
    bool  myStrtoul(const char *pStr, unsigned long *pVal);
    bool  myStrtoll(const char *pStr, long long *pVal);
    bool  myStrtoull(const char *pStr, unsigned long long *pVal);

    static LsMcCmdFunc s_LsMcCmdFuncs[];

    LsShmHash              *m_pHash;
    ls_str_pair_t           m_parms;
    LsMcDataItem            m_item;     // cmd in progress
    LsShmHash::iteroffset   m_iterOff;  // cmd in progress
    int                     m_needed;
    bool                    m_noreply;
    bool                    m_usecas;
};

#endif // LSSHMMEMCACHED_H

