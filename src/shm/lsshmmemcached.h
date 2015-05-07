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


#define VERSION         "0.0.0"

#define LSMC_ADD        0x01
#define LSMC_SET        0x02
#define LSMC_REPLACE    0x03
#define LSMC_APPEND     0x04
#define LSMC_PREPEND    0x05
#define LSMC_CAS        0x06
#define LSMC_INCR       0x07
#define LSMC_DECR       0x08
#define LSMC_CMDMASK    0xff

#define LSMC_WITHCAS    0x0100

#define ULL_MAXLEN  24      // maximum buffer size of unsigned long long

#define LSMC_MAXDELTATIME   60*60*24*30     // seconds in 30 days

typedef struct
{
    uint8_t x_verbose;
    uint8_t x_withcas;
    uint8_t x_notused[2];
    union
    {
        uint64_t cas;
        uint8_t  val[1];
    } x_data[];
} LsMcHdr;

typedef struct
{
    time_t   x_exptime;
    uint32_t x_flags;
    union
    {
        struct
        {
            uint64_t cas;
            uint8_t  val[1];
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
    LsShmMemCached(LsShmHash *pHash, bool usecas = true);
    ~LsShmMemCached() {}

    int  processCmd(char *pStr);
    int  doDataUpdate(char *pBuf);
    
    static inline LsMcDataItem *mcIter2data(
        iterator iter, int useCas, uint8_t **pValPtr, int *pValLen)
    {
        LsMcDataItem *pItem;
        pItem = (LsMcDataItem *)iter->getVal();
        *pValLen = iter->getValLen() - sizeof(*pItem);
        if (useCas)
        {
            *pValLen -= sizeof(pItem->x_data->withcas.cas);
            *pValPtr = pItem->x_data->withcas.val;
        }
        else
        {
            *pValPtr = pItem->x_data->val;
        }
        return pItem;
    }

    static inline LsMcDataItem *mcIter2num(
        iterator iter, int useCas, char *pNumBuf, uint64_t *pNumVal)
    {
        unsigned long long ullnum;
        int valLen;
        uint8_t *valPtr;
        LsMcDataItem *pItem = mcIter2data(iter, useCas, &valPtr, &valLen);
        if ((valLen <= 0) || (valLen > ULL_MAXLEN))
            return NULL;
        memcpy(pNumBuf, (char *)valPtr, valLen);
        pNumBuf[valLen] = '\0';
        if (!myStrtoull(pNumBuf, &ullnum))
            return NULL;
        *pNumVal = (uint64_t)ullnum;
        return pItem;
    }

private:
    LsShmMemCached(const LsShmHash &other);
    LsShmMemCached &operator=(const LsShmMemCached &other);

private:
    static int   doCmdGet(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdUpdate(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdArithmetic(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdDelete(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdTouch(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdVersion(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdQuit(LsShmMemCached *pThis, char *pStr, int arg);
    static int   doCmdVerbosity(LsShmMemCached *pThis, char *pStr, int arg);
    static int   notImplemented(LsShmMemCached *pThis, char *pStr, int arg);
    LsMcCmdFunc *getCmdFunction(char *pCmd);
    void  respond(const char *str);
    void  sendResult(const char *fmt, ...);
    uint64_t     getCas();
    char *advToken(char *pStr, char **pTokPtr, int *pTokLen);
    bool  myStrtol(const char *pStr, long *pVal);
    bool  myStrtoul(const char *pStr, unsigned long *pVal);
    bool  myStrtoll(const char *pStr, long long *pVal);
    static bool  myStrtoull(const char *pStr, unsigned long long *pVal);

    bool  isExpired(LsMcDataItem *pItem)
    {
        return ((pItem->x_exptime != 0)
            && (pItem->x_exptime <= time((time_t *)NULL)));
    }

    static LsMcCmdFunc s_LsMcCmdFuncs[];

    LsShmHash              *m_pHash;
    LsShmOffset_t           m_iHdrOff;
    ls_str_pair_t           m_parms;
    LsMcDataItem            m_item;     // cmd in progress
    LsShmHash::iteroffset   m_iterOff;  // cmd in progress
    int                     m_needed;
    bool                    m_noreply;
    bool                    m_usecas;
    uint8_t                 m_retcode;
};

#endif // LSSHMMEMCACHED_H

