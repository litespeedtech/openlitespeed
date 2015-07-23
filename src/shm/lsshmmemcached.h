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

#define ULL_MAXLEN  24      // maximum buffer size of unsigned long long

#define LSMC_MAXDELTATIME   60*60*24*30     // seconds in 30 days

typedef struct
{
    uint64_t    get_cmds;
    uint64_t    set_cmds;
    uint64_t    touch_cmds;
    uint64_t    flush_cmds;
    uint64_t    get_hits;
    uint64_t    get_misses;
    uint64_t    touch_hits;
    uint64_t    touch_misses;
    uint64_t    delete_hits;
    uint64_t    delete_misses;
    uint64_t    incr_hits;
    uint64_t    incr_misses;
    uint64_t    decr_hits;
    uint64_t    decr_misses;
    uint64_t    cas_hits;
    uint64_t    cas_misses;
    uint64_t    cas_badval;
} LsMcStats;

typedef struct
{
    uint8_t     x_verbose;
    uint8_t     x_withcas;
    uint8_t     x_notused[2];
    LsMcStats   x_stats;
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

typedef enum
{
    MC_BINARY_REQ = 0x80,
    MC_BINARY_RES = 0x81
} McBinMagic;

typedef enum
{
    MC_BINCMD_GET           = 0x00,
    MC_BINCMD_SET           = 0x01,
    MC_BINCMD_ADD           = 0x02,
    MC_BINCMD_REPLACE       = 0x03,
    MC_BINCMD_DELETE        = 0x04,
    MC_BINCMD_INCREMENT     = 0x05,
    MC_BINCMD_DECREMENT     = 0x06,
    MC_BINCMD_QUIT          = 0x07,
    MC_BINCMD_FLUSH         = 0x08,
    MC_BINCMD_GETQ          = 0x09,
    MC_BINCMD_NOOP          = 0x0a,
    MC_BINCMD_VERSION       = 0x0b,
    MC_BINCMD_GETK          = 0x0c,
    MC_BINCMD_GETKQ         = 0x0d,
    MC_BINCMD_APPEND        = 0x0e,
    MC_BINCMD_PREPEND       = 0x0f,
    MC_BINCMD_STAT          = 0x10,
    MC_BINCMD_SETQ          = 0x11,
    MC_BINCMD_ADDQ          = 0x12,
    MC_BINCMD_REPLACEQ      = 0x13,
    MC_BINCMD_DELETEQ       = 0x14,
    MC_BINCMD_INCREMENTQ    = 0x15,
    MC_BINCMD_DECREMENTQ    = 0x16,
    MC_BINCMD_QUITQ         = 0x17,
    MC_BINCMD_FLUSHQ        = 0x18,
    MC_BINCMD_APPENDQ       = 0x19,
    MC_BINCMD_PREPENDQ      = 0x1a,
    MC_BINCMD_VERBOSITY     = 0x1b,
    MC_BINCMD_TOUCH         = 0x1c,
    MC_BINCMD_GAT           = 0x1d,
    MC_BINCMD_GATQ          = 0x1e,
    MC_BINCMD_GATK          = 0x23,
    MC_BINCMD_GATKQ         = 0x24
} McBinCmd;

#define LSMC_WITHCAS    0x0100
#define LSMC_CMDMASK    0x00ff

typedef enum
{
    MC_BINSTAT_SUCCESS      = 0x00,
    MC_BINSTAT_KEYENOENT    = 0x01,
    MC_BINSTAT_KEYEEXISTS   = 0x02,
    MC_BINSTAT_E2BIG        = 0x03,
    MC_BINSTAT_EINVAL       = 0x04,
    MC_BINSTAT_NOTSTORED    = 0x05,
    MC_BINSTAT_DELTABADVAL  = 0x06,
    MC_BINSTAT_AUTHERROR    = 0x20,
    MC_BINSTAT_AUTHCONTINUE = 0x21,
    MC_BINSTAT_UNKNOWNCMD   = 0x81,
    MC_BINSTAT_ENOMEM       = 0x82
} McBinStat;

typedef struct
{
    uint8_t     magic;
    uint8_t     opcode;
    uint16_t    keylen;
    uint8_t     extralen;
    uint8_t     datatype;
    uint16_t    status;
    uint32_t    totbody;
    uint32_t    opaque;
    uint64_t    cas;
} McBinCmdHdr;

typedef union
{
    struct
    {
        uint64_t    delta;
        uint64_t    initval;
        uint32_t    exptime;
    } incrdecr;
    struct
    {
        uint32_t    flags;
        uint32_t    exptime;
    } value;
    struct
    {
        uint32_t    level;
    } verbosity;
    struct
    {
        uint32_t    exptime;
    } touch;
} McBinReqExtra;

typedef union
{
    struct
    {
        uint64_t    newval;
    } incrdecr;
    struct
    {
        uint32_t    flags;
    } value;
} McBinResExtra;

class LsShmMemCached;
typedef struct
{
    const char *cmd;
    int arg;
    int (*func)(LsShmMemCached *pThis, char *pStr, int arg, uint8_t *pConv);
} LsMcCmdFunc;

class LsShmMemCached
{
public:
    LsShmMemCached(LsShmHash *pHash, bool usecas = true);
    ~LsShmMemCached() {}

    int  processCmd(char *pStr);
    int  doDataUpdate(uint8_t *pBuf);
    int  convertCmd(char *pStr, uint8_t *pBinBuf);
    int  processBinCmd(uint8_t *pBinBuf);

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
            *pValPtr = pItem->x_data->val;
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

    static inline bool isExpired(LsMcDataItem *pItem)
    {
        return ((pItem->x_exptime != 0)
                && (pItem->x_exptime <= time((time_t *)NULL)));
    }

    void lock()
    {   m_pHash->lockChkRehash();   }

    void unlock()
    {   m_pHash->unlock();   }

private:
    LsShmMemCached(const LsShmHash &other);
    LsShmMemCached &operator=(const LsShmMemCached &other);

private:
    void     respond(const char *str);
    void     sendResult(const char *fmt, ...);
    void     binRespond(uint8_t *buf, int cnt);

    uint64_t getCas()
    {
        return ((m_iHdrOff != 0) ?
                ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_data->cas : 0);
    }

    uint8_t  getVerbose()
    {
        return ((m_iHdrOff != 0) ?
                ((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_verbose : 0);
    }

    void     setVerbose(uint8_t iLevel)
    {
        if (m_iHdrOff != 0)
            ((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_verbose = iLevel;
        return;
    }

    void     saveCas(LsMcDataItem *pItem)
    {
        m_rescas = (m_usecas ? pItem->x_data->withcas.cas : 0);
    }

    LsMcCmdFunc *getCmdFunction(char *pCmd);
    void         dataItemUpdate(uint8_t *pBuf);

    static int   doCmdGat(LsShmMemCached *pThis,
                          char *pStr, int arg, uint8_t *pConv);
    static int   doCmdGet(LsShmMemCached *pThis,
                          char *pStr, int arg, uint8_t *pConv);
    static int   doCmdUpdate(LsShmMemCached *pThis,
                             char *pStr, int arg, uint8_t *pConv);
    static int   doCmdArithmetic(LsShmMemCached *pThis,
                                 char *pStr, int arg, uint8_t *pConv);
    static int   doCmdDelete(LsShmMemCached *pThis,
                             char *pStr, int arg, uint8_t *pConv);
    static int   doCmdTouch(LsShmMemCached *pThis,
                            char *pStr, int arg, uint8_t *pConv);
    static int   doCmdStats(LsShmMemCached *pThis,
                            char *pStr, int arg, uint8_t *pConv);
    static int   doCmdVersion(LsShmMemCached *pThis,
                              char *pStr, int arg, uint8_t *pConv);
    static int   doCmdQuit(LsShmMemCached *pThis,
                           char *pStr, int arg, uint8_t *pConv);
    static int   doCmdVerbosity(LsShmMemCached *pThis,
                                char *pStr, int arg, uint8_t *pConv);
    static int   notImplemented(LsShmMemCached *pThis,
                                char *pStr, int arg, uint8_t *pConv);

    uint8_t  setupNoreplyCmd(uint8_t cmd);
    uint8_t *setupBinCmd(McBinCmdHdr *pHdr, LsShmUpdOpt *pOpt);

    void     doBinGet(McBinCmdHdr *pHdr, bool doTouch);
    int      doBinDataUpdate(uint8_t *pBuf, McBinCmdHdr *pHdr);
    void     doBinDelete(McBinCmdHdr *pHdr);
    void     doBinArithmetic(McBinCmdHdr *pHdr, LsShmUpdOpt *pOpt);
    void     doBinVersion(McBinCmdHdr *pHdr);

    void     setupBinResHdr(uint8_t cmd,
                            uint8_t extralen, uint16_t keylen, uint32_t totbody,
                            uint16_t status, uint8_t *pBinBuf);
    void     binOkRespond(McBinCmdHdr *pHdr);
    void     binErrRespond(McBinCmdHdr *pHdr, McBinStat err);

    char    *advToken(char *pStr, char **pTokPtr, int *pTokLen);
    bool     myStrtol(const char *pStr, long *pVal);
    bool     myStrtoul(const char *pStr, unsigned long *pVal);
    bool     myStrtoll(const char *pStr, long long *pVal);
    static bool  myStrtoull(const char *pStr, unsigned long long *pVal);

    static void  setItemExptime(LsMcDataItem *pItem, uint32_t exptime)
    {
        if ((exptime != 0) && (exptime <= LSMC_MAXDELTATIME))
            exptime += time((time_t *)NULL);
        pItem->x_exptime = (time_t)exptime;
    }

    int   parmAdjLen(int valLen)
    {
        return (m_usecas ?
                (valLen + sizeof(m_item) + sizeof(m_item.x_data->withcas.cas)) :
                (valLen + sizeof(m_item)));
    }

    void statSetCmd()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.set_cmds;
    }

    void statGetHit()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.get_hits;
    }

    void statGetMiss()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.get_misses;
    }

    void statTouchHit()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.touch_hits;
    }

    void statTouchMiss()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.touch_misses;
    }

    void statDeleteHit()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.delete_hits;
    }

    void statDeleteMiss()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.delete_misses;
    }

    void statIncrHit()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.incr_hits;
    }

    void statIncrMiss()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.incr_misses;
    }

    void statDecrHit()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.decr_hits;
    }

    void statDecrMiss()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.decr_misses;
    }

    void statCasHit()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.cas_hits;
    }

    void statCasMiss()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.cas_misses;
    }

    void statCasBad()
    {
        if (m_iHdrOff != 0)
            ++((LsMcHdr *)m_pHash->offset2ptr(m_iHdrOff))->x_stats.cas_badval;
    }

    static LsMcCmdFunc s_LsMcCmdFuncs[];

    LsShmHash              *m_pHash;
    LsShmOffset_t           m_iHdrOff;
    ls_str_pair_t           m_parms;
    LsMcDataItem            m_item;     // cmd in progress
    LsShmHash::iteroffset   m_iterOff;  // cmd in progress
    uint64_t                m_rescas;   // cmd in progress
    int                     m_needed;
    bool                    m_noreply;
    bool                    m_usecas;
    uint8_t                 m_retcode;
};

#endif // LSSHMMEMCACHED_H

