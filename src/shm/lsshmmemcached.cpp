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
#include <shm/lsshmmemcached.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <arpa/inet.h>


uint64_t htonll(uint64_t val)
{
    return val;
}


uint64_t ntohll(uint64_t val)
{
    return val;
}


int LsShmMemCached::notImplemented(LsShmMemCached *pThis,
                                   char *pStr, int arg, uint8_t *pConv)
{
    pThis->respond("NOT_IMPLEMENTED");
    return 0;
}


LsMcCmdFunc LsShmMemCached::s_LsMcCmdFuncs[] =
{
    { "gat",        MC_BINCMD_GET,          doCmdGat },
    { "get",        MC_BINCMD_GET,          doCmdGet },
    { "bget",       MC_BINCMD_GET,          doCmdGet },
    { "gets",       MC_BINCMD_GET|LSMC_WITHCAS,         doCmdGet },
    { "add",        MC_BINCMD_ADD,          doCmdUpdate },
    { "set",        MC_BINCMD_SET,          doCmdUpdate },
    { "replace",    MC_BINCMD_REPLACE,      doCmdUpdate },
    { "append",     MC_BINCMD_APPEND,       doCmdUpdate },
    { "prepend",    MC_BINCMD_PREPEND,      doCmdUpdate },
    { "cas",        MC_BINCMD_REPLACE|LSMC_WITHCAS,     doCmdUpdate },
    { "incr",       MC_BINCMD_INCREMENT,    doCmdArithmetic },
    { "decr",       MC_BINCMD_DECREMENT,    doCmdArithmetic },
    { "delete",     MC_BINCMD_DELETE,       doCmdDelete },
    { "touch",      MC_BINCMD_TOUCH,        doCmdTouch },
    { "stats",      0,              doCmdStats },
    { "flush_all",  0,              notImplemented },
    { "version",    MC_BINCMD_VERSION,      doCmdVersion },
    { "quit",       MC_BINCMD_QUIT,         doCmdQuit },
    { "shutdown",   0,              notImplemented },
    { "verbosity",  MC_BINCMD_VERBOSITY,    doCmdVerbosity },
};

const char badCmdLineFmt[] = "CLIENT_ERROR bad command line format";

static int asc2bincmd(int arg, char *pkey, int keylen, char *pval, int vallen,
    uint32_t flags, uint32_t exptime, uint64_t cas, bool quiet, uint8_t *pout);

const char tokNoreply[] = "noreply";

static inline bool chkNoreply(char *tokPtr, int tokLen)
{
    return ((tokLen == sizeof(tokNoreply)-1) && (strcmp(tokPtr, tokNoreply) == 0));
}


void LsShmMemCached::respond(const char *str)
{
    if (m_noreply)
    {
        if (getVerbose() > 1)
            fprintf(stderr, ">%d NOREPLY %s\n", 0, str);
        m_noreply = false;
    }
    else
    {
        if (getVerbose() > 1)
            fprintf(stderr, ">%d %s\n", 0, str);
        fprintf(stdout, "%s\r\n", str);
    }
}


void LsShmMemCached::sendResult(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vfprintf(stdout, fmt, va);
    va_end(va);
}


void LsShmMemCached::binRespond(uint8_t *buf, int cnt)
{
    if (!m_noreply)
    {
        int i;
        uint8_t *p = buf;
        for (i = 0; i < cnt; ++i)
        {
            if ((i & 0x0f) == 0)
                fputc('\n', stdout);
            fprintf(stdout, " %02x", (unsigned)*p++);
        }
        fputc('\n', stdout);
    }
}


LsShmMemCached::LsShmMemCached(LsShmHash *pHash, bool usecas)
    : m_pHash(pHash)
    , m_iterOff(0)
    , m_rescas(0)
    , m_needed(0)
    , m_noreply(false)
    , m_usecas(usecas)
{
    ls_str_blank(&m_parms.key);
    ls_str_blank(&m_parms.value);
    LsShmOffset_t off;
    pHash->disableLock();     // manually lock on higher level
    lock();
    if ((off = pHash->getUserOff()) == 0)
    {
        int remapped;
        LsMcHdr *pHdr;
        LsShmSize_t size = sizeof(*pHdr);
        if (m_usecas)
            size += sizeof(pHdr->x_data->cas);
        if ((off = pHash->alloc2(size, remapped)) != 0)
        {
            pHdr = (LsMcHdr *)pHash->offset2ptr(off);
            pHdr->x_verbose = 0;
            pHdr->x_withcas = m_usecas;
            memset((void *)&pHdr->x_stats, 0, sizeof(pHdr->x_stats));
            if (m_usecas)
                pHdr->x_data->cas = 0;
            pHash->setUserOff(off);
            pHash->setUserSize(size);
        }
    }
    unlock();
    m_iHdrOff = off;
}


int LsShmMemCached::processCmd(char *pStr)
{
    char *pCmd;
    int len;
    LsMcCmdFunc *p;

    if (getVerbose() > 1)
        fprintf(stderr, "<%d %s\n", 0, pStr);

    pStr = advToken(pStr, &pCmd, &len);
    if (len <= 0)
        return -1;
    if ((p = getCmdFunction(pCmd)) == NULL)
    {
        respond("CLIENT_ERROR unknown command");
        return -1;
    }
    return (*p->func)(this, pStr, p->arg, NULL);
}


LsMcCmdFunc *LsShmMemCached::getCmdFunction(char *pCmd)
{
    LsMcCmdFunc *p = &s_LsMcCmdFuncs[0];
    while (p < &s_LsMcCmdFuncs[sizeof(s_LsMcCmdFuncs)/sizeof(s_LsMcCmdFuncs[0])])
    {
        if (strcmp(pCmd, p->cmd) == 0)
            return p;
        ++p;
    }
    return NULL;
}


/*
 * this code needs the lock continued from the parsed command function.
 */
int LsShmMemCached::doDataUpdate(uint8_t *pBuf)
{
    if (m_parms.key.length <= 0)
    {
        unlock();
        respond("CLIENT_ERROR invalid data update");
        return -1;
    }
    m_parms.key.length = 0;

    if (m_iterOff != 0)
    {
        dataItemUpdate(pBuf);
        unlock();
        if (m_retcode != UPDRET_NONE)
            respond("STORED");
        m_iterOff = 0;
    }
    else
    {
        unlock();
        if (m_retcode == UPDRET_NOTFOUND)
            respond("NOT_FOUND");
        else if (m_retcode == UPDRET_CASFAIL)
            respond("EXISTS");
        else
            respond("NOT_STORED");
    }

    return 0;
}


void LsShmMemCached::dataItemUpdate(uint8_t *pBuf)
{
    int valLen;
    uint8_t *valPtr;
    iterator iter = m_pHash->offset2iterator(m_iterOff);
    LsMcDataItem *pItem = mcIter2data(iter, m_usecas, &valPtr, &valLen);
    if (m_retcode == UPDRET_APPEND)
    {
        valLen = m_parms.value.length;
        valPtr = iter->getVal() + iter->getValLen() - valLen;
    }
    else if (m_retcode == UPDRET_PREPEND)
        valLen = m_parms.value.length;
    else
    {
        if ((m_item.x_exptime != 0)
          && (m_item.x_exptime <= LSMC_MAXDELTATIME))
        {
            m_item.x_exptime += iter->getLruLasttime();
        }
        ::memcpy((void *)pItem, (void *)&m_item, sizeof(m_item));
    }
    if (m_usecas)
        m_rescas = pItem->x_data->withcas.cas = getCas();
    if (valLen > 0)
        ::memcpy(valPtr, (void *)pBuf, valLen);
    m_needed = 0;
    return;
}


int LsShmMemCached::doCmdGat(LsShmMemCached *pThis,
                             char *pStr, int arg, uint8_t *pConv)
{
    char *tokPtr;
    int tokLen;
    char *pExptime;
    unsigned long exptime;
    pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                           &pThis->m_parms.key.length);
    pStr = pThis->advToken(pStr, &pExptime, &tokLen);
    if (tokLen <= 0)
    {
        pThis->respond(badCmdLineFmt);
        return -1;
    }
    if (!pThis->myStrtoul(pExptime, &exptime))
    {
        pThis->respond("CLIENT_ERROR invalid exptime argument");
        return -1;
    }
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional noreply
    pThis->m_noreply = chkNoreply(tokPtr, tokLen);

    if (pConv != NULL)
    {
        return asc2bincmd(MC_BINCMD_GAT,
            pThis->m_parms.key.pstr, pThis->m_parms.key.length,
            NULL, 0, (uint32_t)0, (uint32_t)exptime,
            (uint64_t)0, pThis->m_noreply, pConv);
    }
    return pThis->notImplemented(pThis, NULL, 0, NULL);
}


int LsShmMemCached::doCmdGet(LsShmMemCached *pThis,
                             char *pStr, int arg, uint8_t *pConv)
{
    LsShmHash *pHash = pThis->m_pHash;
    LsShmHash::iteroffset iterOff;
    iterator iter;
    LsMcDataItem *pItem;
    int valLen;
    uint8_t *valPtr;
    while (1)
    {
        pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                               &pThis->m_parms.key.length);
        if (pThis->m_parms.key.length <= 0)
            break;
        if (pConv != NULL)
        {
            return asc2bincmd(MC_BINCMD_GETK,
                pThis->m_parms.key.pstr, pThis->m_parms.key.length,
                NULL, 0, (uint32_t)0, (uint32_t)0,
                (uint64_t)0, pThis->m_noreply, pConv);
        }

        pThis->lock();
        if ((iterOff = pHash->findIterator(&pThis->m_parms)) != 0)
        {
            iter = pHash->offset2iterator(iterOff);
            pItem = mcIter2data(iter, pThis->m_usecas, &valPtr, &valLen);
            if (pThis->isExpired(pItem))
            {
                pHash->eraseIterator(iterOff);
                pThis->statGetMiss();
            }
            else if (arg & LSMC_WITHCAS)
            {
                pThis->statGetHit();
                pThis->sendResult("VALUE %.*s %d %d %llu\r\n%.*s\r\n",
                  iter->getKeyLen(), iter->getKey(),
                  pItem->x_flags,
                  valLen,
                  (pThis->m_usecas ? pItem->x_data->withcas.cas : (uint64_t)0),
                  valLen, valPtr
                );
            }
            else
            {
                pThis->statGetHit();
                pThis->sendResult("VALUE %.*s %d %d\r\n%.*s\r\n",
                  iter->getKeyLen(), iter->getKey(),
                  pItem->x_flags,
                  valLen,
                  valLen, valPtr
                );
            }
        }
        else
            pThis->statGetMiss();
        pThis->unlock();
    }
    pThis->sendResult("END\r\n");
    return 0;
}


int LsShmMemCached::doCmdUpdate(LsShmMemCached *pThis,
                                char *pStr, int arg, uint8_t *pConv)
{
    char *tokPtr;
    int tokLen;
    char *pFlags;
    char *pExptime;
    char *pLength;
    char *pCas;
    unsigned long flags;
    unsigned long exptime;
    long length;
    unsigned long long cas;
    LsShmUpdOpt updOpt;

    pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                           &pThis->m_parms.key.length);
    pStr = pThis->advToken(pStr, &pFlags, &tokLen);
    pStr = pThis->advToken(pStr, &pExptime, &tokLen);
    pStr = pThis->advToken(pStr, &pLength, &tokLen);
    if ((tokLen <= 0)
        || (!pThis->myStrtoul(pFlags, &flags))
        || (!pThis->myStrtoul(pExptime, &exptime))
        || (!pThis->myStrtol(pLength, &length))
    )
    {
        pThis->respond(badCmdLineFmt);
        return -1;
    }
    pThis->m_item.x_flags = (uint32_t)flags;
    pThis->m_item.x_exptime = (time_t)exptime;
    pThis->m_parms.value.pstr = NULL;
    pThis->m_parms.value.length = length;   // adjusted later
    updOpt.m_iFlags = arg;
    if ((arg == MC_BINCMD_APPEND) || (arg == MC_BINCMD_PREPEND))
    {
        if (pThis->m_usecas)
            updOpt.m_iFlags |= LSMC_WITHCAS;
    }
    else
    {
        pThis->m_parms.value.length = pThis->parmAdjLen(pThis->m_parms.value.length);
    }
    if (arg & LSMC_WITHCAS)
    {
        pStr = pThis->advToken(pStr, &pCas, &tokLen);
        if ((tokLen <= 0) || (!pThis->myStrtoull(pCas, &cas)))
        {
            pThis->respond(badCmdLineFmt);
            return -1;
        }
        updOpt.m_cas = (uint64_t)cas;
    }
    else
    {
        updOpt.m_cas = 0;
    }
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional noreply
    pThis->m_noreply = chkNoreply(tokPtr, tokLen);
    if (pThis->m_noreply)
        pThis->advToken(pStr, &tokPtr, &tokLen);

    if (pConv != NULL)
    {
        return asc2bincmd(arg,
            pThis->m_parms.key.pstr, pThis->m_parms.key.length,
            tokPtr, tokLen, (uint32_t)flags, (uint32_t)exptime,
            (uint64_t)updOpt.m_cas, pThis->m_noreply, pConv);
    }

    pThis->lock();
    pThis->statSetCmd();
    switch (arg)
    {
        case MC_BINCMD_ADD:
            pThis->m_iterOff = pThis->m_pHash->insertIterator(&pThis->m_parms,
                                                              &updOpt);
            pThis->m_retcode = UPDRET_DONE;
            break;
        case MC_BINCMD_SET:
            pThis->m_iterOff = pThis->m_pHash->setIterator(&pThis->m_parms);
            pThis->m_retcode = UPDRET_DONE;
            break;
        case MC_BINCMD_REPLACE:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms,
                                                              &updOpt);
            pThis->m_retcode = UPDRET_DONE;
            break;
        case MC_BINCMD_APPEND:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms,
                                                              &updOpt);
            pThis->m_retcode = UPDRET_APPEND;
            break;
        case MC_BINCMD_PREPEND:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms,
                                                              &updOpt);
            pThis->m_retcode = UPDRET_PREPEND;
            break;
        case MC_BINCMD_REPLACE|LSMC_WITHCAS:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms,
                                                              &updOpt);
            pThis->m_retcode = updOpt.m_iRetcode;
            if (pThis->m_retcode == UPDRET_NOTFOUND)
                pThis->statCasMiss();
            else if (pThis->m_retcode == UPDRET_CASFAIL)
                pThis->statCasBad();
            else
                pThis->statCasHit();

            break;
        default:
            pThis->unlock();
            pThis->respond("SERVER_ERROR unhandled type");
            return -1;
    }
    // unlock after data is updated
    pThis->m_needed = length;
    if (length == 0)
        pThis->doDataUpdate(NULL);      // empty item
    return length;
}


int LsShmMemCached::doCmdArithmetic(LsShmMemCached *pThis,
                                    char *pStr, int arg, uint8_t *pConv)
{
    char *tokPtr;
    int tokLen;
    char *pDelta;
    unsigned long long delta;
    LsShmUpdOpt updOpt;
    char numBuf[ULL_MAXLEN+1];
    pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                           &pThis->m_parms.key.length);
    pStr = pThis->advToken(pStr, &pDelta, &tokLen);
    if (tokLen <= 0)
    {
        pThis->respond(badCmdLineFmt);
        return -1;
    }
    if (!pThis->myStrtoull(pDelta, &delta))
    {
        pThis->respond("CLIENT_ERROR invalid numeric delta argument");
        return -1;
    }
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional noreply
    pThis->m_noreply = chkNoreply(tokPtr, tokLen);
    pThis->m_parms.value.pstr = NULL;
    pThis->m_parms.value.length = pThis->parmAdjLen(0);
    updOpt.m_iFlags = arg;
    updOpt.m_value = (uint64_t)delta;
    updOpt.m_cas = (uint64_t)0;
    updOpt.m_pRet = (void *)&pThis->m_item;
    updOpt.m_pMisc = (void *)numBuf;
    if (pThis->m_usecas)
        updOpt.m_iFlags |= LSMC_WITHCAS;

    if (pConv != NULL)
    {
        uint32_t initval = 1000;    // testing
        uint32_t exptime = 0;       // testing
        return asc2bincmd(arg,
            pThis->m_parms.key.pstr, pThis->m_parms.key.length,
            NULL, 0, (uint32_t)initval, (uint32_t)exptime,
            (uint64_t)delta, pThis->m_noreply, pConv);
    }

    pThis->lock();
    if ((pThis->m_iterOff =
        pThis->m_pHash->updateIterator(&pThis->m_parms, &updOpt)) != 0)
    {
        if (arg == MC_BINCMD_INCREMENT)
            pThis->statIncrHit();
        else
            pThis->statDecrHit();
        pThis->m_retcode = UPDRET_NONE;
        pThis->doDataUpdate((uint8_t *)numBuf);
        pThis->respond(numBuf);
    }
    else if (updOpt.m_iRetcode == UPDRET_NOTFOUND)
    {
        if (arg == MC_BINCMD_INCREMENT)
            pThis->statIncrMiss();
        else
            pThis->statDecrMiss();
        pThis->unlock();
        pThis->respond("NOT_FOUND");
    }
    else
    {
        pThis->unlock();
        if (updOpt.m_iRetcode == UPDRET_NONNUMERIC)
            pThis->respond(
              "CLIENT_ERROR cannot increment or decrement non-numeric value");
        else
            pThis->respond("SERVER_ERROR unable to update");
    }
    return 0;
}


int LsShmMemCached::doCmdDelete(LsShmMemCached *pThis,
                                char *pStr, int arg, uint8_t *pConv)
{
    char *tokPtr;
    int tokLen;
    LsShmHash::iteroffset iterOff;
    bool expired;
    pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                           &pThis->m_parms.key.length);
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional
    if ((tokLen == 1) && (strcmp(tokPtr, "0") == 0))    // hold_is_zero???
        pStr = pThis->advToken(pStr, &tokPtr, &tokLen);

    pThis->m_noreply = chkNoreply(tokPtr, tokLen);
    if (pThis->m_noreply)
        pThis->advToken(pStr, &tokPtr, &tokLen);

    if (tokLen > 0)
    {
        pThis->respond("CLIENT_ERROR bad delete format");
        return -1;
    }

    if (pConv != NULL)
    {
        return asc2bincmd(arg,
            pThis->m_parms.key.pstr, pThis->m_parms.key.length,
            NULL, 0, (uint32_t)0, (uint32_t)0,
            (uint64_t)0, pThis->m_noreply, pConv);
    }

    pThis->lock();
    if ((iterOff = pThis->m_pHash->findIterator(&pThis->m_parms)) != 0)
    {
        expired = pThis->isExpired(
            (LsMcDataItem *)pThis->m_pHash->offset2iteratorData(iterOff));
        pThis->m_pHash->eraseIterator(iterOff);
    }
    if ((iterOff != 0) && !expired)
    {
        pThis->statDeleteHit();
        pThis->unlock();
        pThis->respond("DELETED");
    }
    else
    { 
        pThis->statDeleteMiss();
        pThis->unlock();
        pThis->respond("NOT_FOUND");
    }
    return 0;
}


int LsShmMemCached::doCmdTouch(LsShmMemCached *pThis,
                               char *pStr, int arg, uint8_t *pConv)
{
    char *tokPtr;
    int tokLen;
    char *pExptime;
    unsigned long exptime;
    LsShmHash::iteroffset iterOff;
    LsMcDataItem *pItem;
    pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                           &pThis->m_parms.key.length);
    pStr = pThis->advToken(pStr, &pExptime, &tokLen);
    if (tokLen <= 0)
    {
        pThis->respond(badCmdLineFmt);
        return -1;
    }
    if (!pThis->myStrtoul(pExptime, &exptime))
    {
        pThis->respond("CLIENT_ERROR invalid exptime argument");
        return -1;
    }
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional noreply
    pThis->m_noreply = chkNoreply(tokPtr, tokLen);

    if (pConv != NULL)
    {
        return asc2bincmd(arg,
            pThis->m_parms.key.pstr, pThis->m_parms.key.length,
            NULL, 0, (uint32_t)0, (uint32_t)exptime,
            (uint64_t)0, pThis->m_noreply, pConv);
    }

    pThis->lock();
    if ((iterOff = pThis->m_pHash->findIterator(&pThis->m_parms)) != 0)
    {
        pItem = (LsMcDataItem *)pThis->m_pHash->offset2iteratorData(iterOff);
        if (pThis->isExpired(pItem))
            pThis->m_pHash->eraseIterator(iterOff);
        else
        {
            setItemExptime(pItem, (uint32_t)exptime);
            pThis->statTouchHit();
            pThis->unlock();
            pThis->respond("TOUCHED");
            return 0;
        }
    }
    pThis->statTouchMiss();
    pThis->unlock();
    pThis->respond("NOT_FOUND");
    return 0;
}


int LsShmMemCached::doCmdStats(LsShmMemCached *pThis,
                                 char *pStr, int arg, uint8_t *pConv)
{
    if (pConv != NULL)
    {
        return asc2bincmd(arg,
            NULL, 0,
            NULL, 0, (uint32_t)0, (uint32_t)0,
            (uint64_t)0, pThis->m_noreply, pConv);
    }
    if (pThis->m_iHdrOff == 0)
        return -1;
    LsMcHdr *pHdr = (LsMcHdr *)pThis->m_pHash->offset2ptr(pThis->m_iHdrOff);
    pThis->sendResult("STAT pid %lu\r\n", (long)getpid());
    pThis->sendResult("STAT version %s\r\n", VERSION);
    pThis->sendResult("STAT cmd_get %llu\r\n",
                      (unsigned long long)pHdr->x_stats.get_hits
                      + (unsigned long long)pHdr->x_stats.get_misses);
    pThis->sendResult("STAT cmd_set %llu\r\n",
                      (unsigned long long)pHdr->x_stats.set_cmds);
    pThis->sendResult("STAT cmd_flush %llu\r\n",
                      (unsigned long long)pHdr->x_stats.flush_cmds);
    pThis->sendResult("STAT cmd_touch %llu\r\n",
                      (unsigned long long)pHdr->x_stats.touch_hits
                      + (unsigned long long)pHdr->x_stats.touch_misses);
    pThis->sendResult("STAT get_hits %llu\r\n",
                      (unsigned long long)pHdr->x_stats.get_hits);
    pThis->sendResult("STAT get_misses %llu\r\n",
                      (unsigned long long)pHdr->x_stats.get_misses);
    pThis->sendResult("STAT delete_misses %llu\r\n",
                      (unsigned long long)pHdr->x_stats.delete_misses);
    pThis->sendResult("STAT delete_hits %llu\r\n",
                      (unsigned long long)pHdr->x_stats.delete_hits);
    pThis->sendResult("STAT incr_misses %llu\r\n",
                      (unsigned long long)pHdr->x_stats.incr_misses);
    pThis->sendResult("STAT incr_hits %llu\r\n",
                      (unsigned long long)pHdr->x_stats.incr_hits);
    pThis->sendResult("STAT decr_misses %llu\r\n",
                      (unsigned long long)pHdr->x_stats.decr_misses);
    pThis->sendResult("STAT decr_hits %llu\r\n",
                      (unsigned long long)pHdr->x_stats.decr_hits);
    pThis->sendResult("STAT cas_misses %llu\r\n",
                      (unsigned long long)pHdr->x_stats.cas_misses);
    pThis->sendResult("STAT cas_hits %llu\r\n",
                      (unsigned long long)pHdr->x_stats.cas_hits);
    pThis->sendResult("STAT cas_badval %llu\r\n",
                      (unsigned long long)pHdr->x_stats.cas_badval);
    pThis->sendResult("STAT touch_hits %llu\r\n",
                      (unsigned long long)pHdr->x_stats.touch_hits);
    pThis->sendResult("STAT touch_misses %llu\r\n",
                      (unsigned long long)pHdr->x_stats.touch_misses);
    pThis->sendResult("END\r\n");
    return 0;
}


int LsShmMemCached::doCmdVersion(LsShmMemCached *pThis,
                                 char *pStr, int arg, uint8_t *pConv)
{
    if (pConv != NULL)
    {
        return asc2bincmd(arg,
            NULL, 0,
            NULL, 0, (uint32_t)0, (uint32_t)0,
            (uint64_t)0, pThis->m_noreply, pConv);
    }
    pThis->respond("VERSION " VERSION);
    return 0;
}


int LsShmMemCached::doCmdQuit(LsShmMemCached *pThis,
                              char *pStr, int arg, uint8_t *pConv)
{
    if (pConv != NULL)
    {
        return asc2bincmd(arg,
            NULL, 0,
            NULL, 0, (uint32_t)0, (uint32_t)0,
            (uint64_t)0, pThis->m_noreply, pConv);
    }
    pThis->respond("bye!");
    return 0;
}


int LsShmMemCached::doCmdVerbosity(LsShmMemCached *pThis,
                                   char *pStr, int arg, uint8_t *pConv)
{
    char *tokPtr;
    int tokLen;
    char *pVerbose;
    unsigned long verbose;
    pStr = pThis->advToken(pStr, &pVerbose, &tokLen);
    if (tokLen <= 0)
    {
        pThis->respond(badCmdLineFmt);
        return -1;
    }
    if (!pThis->myStrtoul(pVerbose, &verbose))
    {
        pThis->respond("CLIENT_ERROR invalid verbosity argument");
        return -1;
    }
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional noreply
    pThis->m_noreply = chkNoreply(tokPtr, tokLen);

    if (pConv != NULL)
    {
        return asc2bincmd(arg,
            NULL, 0,
            NULL, 0, (uint32_t)verbose, (uint32_t)0,
            (uint64_t)0, pThis->m_noreply, pConv);
    }

    pThis->setVerbose((uint8_t)verbose);
    pThis->respond("OK");
    return 0;
}


int LsShmMemCached::processBinCmd(uint8_t *pBinBuf)
{
    McBinCmdHdr *pHdr = (McBinCmdHdr *)pBinBuf;
    uint8_t *pVal;
    LsShmUpdOpt updOpt;
    bool doTouch = false;

    if (getVerbose() > 1)
    {
        /* Dump the packet before we convert it to host order */
        int fd = 0;
        int i;
        uint8_t *p = pBinBuf;
        fprintf(stderr, "<%d Read binary protocol data:", fd);
        for (i = 0; i < (int)sizeof(McBinCmdHdr); ++i)
        {
            if ((i & 0x03) == 0)
                fprintf(stderr, "\n<%d   ", fd);
            fprintf(stderr, " 0x%02x", *p++);
        }
        fprintf(stderr, "\n");
    }

    pHdr->opcode = setupNoreplyCmd(pHdr->opcode);
    if ((pVal = setupBinCmd(pHdr, &updOpt)) == NULL)
        return -1;
    m_retcode = UPDRET_DONE;
    m_rescas = 0;

    switch (pHdr->opcode)
    {
        case MC_BINCMD_TOUCH:
        case MC_BINCMD_GAT:
        case MC_BINCMD_GATK:
            doTouch = true;
            // no break
        case MC_BINCMD_GET:
        case MC_BINCMD_GETK:
            doBinGet(pHdr, doTouch);
            break;
        case MC_BINCMD_SET:
            lock();
            statSetCmd();
            m_iterOff = m_pHash->setIterator(&m_parms);
            doBinDataUpdate(pVal, pHdr);
            break;
        case MC_BINCMD_ADD:
            lock();
            statSetCmd();
            m_iterOff = m_pHash->insertIterator(&m_parms, &updOpt);
            doBinDataUpdate(pVal, pHdr);
            break;
        case MC_BINCMD_REPLACE:
            lock();
            statSetCmd();
            m_iterOff = m_pHash->updateIterator(&m_parms, &updOpt);
            if (pHdr->cas != 0)
            {
                m_retcode = updOpt.m_iRetcode;
                if (m_retcode == UPDRET_NOTFOUND)
                    statCasMiss();
                else if (m_retcode == UPDRET_CASFAIL)
                    statCasBad();
                else
                    statCasHit();
            }
            doBinDataUpdate(pVal, pHdr);
            break;
        case MC_BINCMD_DELETE:
            doBinDelete(pHdr);
            break;
        case MC_BINCMD_INCREMENT:
        case MC_BINCMD_DECREMENT:
            doBinArithmetic(pHdr, &updOpt);
            break;
        case MC_BINCMD_APPEND:
        case MC_BINCMD_PREPEND:
            lock();
            statSetCmd();
            m_iterOff = m_pHash->updateIterator(&m_parms, &updOpt);
            // memcached compatibility
            m_retcode = ((updOpt.m_iRetcode == UPDRET_NOTFOUND) ?
                UPDRET_DONE: updOpt.m_iRetcode);
            doBinDataUpdate(pVal, pHdr);
            break;
        case MC_BINCMD_VERBOSITY:
        {
            McBinReqExtra *pReqX = (McBinReqExtra *)(pHdr + 1);
            setVerbose((uint8_t)ntohl(pReqX->verbosity.level));
        }
            binOkRespond(pHdr);
            break;
        case MC_BINCMD_VERSION:
            doBinVersion(pHdr);
            break;
        case MC_BINCMD_QUIT:
            respond("bye!");
            break;
        case MC_BINCMD_FLUSH:
        case MC_BINCMD_NOOP:
        case MC_BINCMD_STAT:
        default:
            notImplemented(this, NULL, 0, NULL);
            break;
    }

    return 0;
}


uint8_t LsShmMemCached::setupNoreplyCmd(uint8_t cmd)
{
    m_noreply = true;
    switch (cmd)
    {
        case MC_BINCMD_SETQ:
            cmd = MC_BINCMD_SET;
            break;
        case MC_BINCMD_ADDQ:
            cmd = MC_BINCMD_ADD;
            break;
        case MC_BINCMD_REPLACEQ:
            cmd = MC_BINCMD_REPLACE;
            break;
        case MC_BINCMD_DELETEQ:
            cmd = MC_BINCMD_DELETE;
            break;
        case MC_BINCMD_INCREMENTQ:
            cmd = MC_BINCMD_INCREMENT;
            break;
        case MC_BINCMD_DECREMENTQ:
            cmd = MC_BINCMD_DECREMENT;
            break;
        case MC_BINCMD_QUITQ:
            cmd = MC_BINCMD_QUIT;
            break;
        case MC_BINCMD_FLUSHQ:
            cmd = MC_BINCMD_FLUSH;
            break;
        case MC_BINCMD_APPENDQ:
            cmd = MC_BINCMD_APPEND;
            break;
        case MC_BINCMD_PREPENDQ:
            cmd = MC_BINCMD_PREPEND;
            break;
        case MC_BINCMD_GETQ:
            cmd = MC_BINCMD_GET;
            break;
        case MC_BINCMD_GETKQ:
            cmd = MC_BINCMD_GETK;
            break;
        case MC_BINCMD_GATQ:
            cmd = MC_BINCMD_GAT;
            break;
        case MC_BINCMD_GATKQ:
            cmd = MC_BINCMD_GATK;
            break;
        default:
            m_noreply = false;
            break;
    }
    return cmd;
}


uint8_t *LsShmMemCached::setupBinCmd(McBinCmdHdr *pHdr, LsShmUpdOpt *pOpt)
{
    uint8_t *pBody = (uint8_t *)(pHdr + 1);
    uint32_t bodyLen = (uint32_t)ntohl(pHdr->totbody);
    uint16_t keyLen = (uint16_t)ntohs(pHdr->keylen);
    pHdr->cas = (uint64_t)ntohll(pHdr->cas);

    if (bodyLen == keyLen)
    {
        m_parms.key.pstr = (char *)pBody;
        m_parms.key.length = keyLen;
    }
    else
    {
        McBinReqExtra *pReqX = (McBinReqExtra *)pBody;
        pBody += pHdr->extralen;

        m_parms.key.pstr = (char *)pBody;
        m_parms.key.length = keyLen;
        m_parms.value.pstr = NULL;
        m_parms.value.length = bodyLen - keyLen - pHdr->extralen;

        switch (pHdr->opcode)
        {
            case MC_BINCMD_REPLACE:
                pOpt->m_iFlags = pHdr->opcode;
                pOpt->m_cas = pHdr->cas;
                // no break
            case MC_BINCMD_ADD:
            case MC_BINCMD_SET:
                if (pHdr->extralen != sizeof(pReqX->value))
                    return NULL;
                m_item.x_flags = (uint32_t)ntohl(pReqX->value.flags);
                m_item.x_exptime = (time_t)ntohl(pReqX->value.exptime);
                m_parms.value.length = parmAdjLen(m_parms.value.length);
                break;

            case MC_BINCMD_TOUCH:
            case MC_BINCMD_GAT:
            case MC_BINCMD_GATK:
                if (pHdr->extralen != sizeof(pReqX->touch))
                    return NULL;
                m_item.x_exptime = (time_t)ntohl(pReqX->touch.exptime);
                break;

            case MC_BINCMD_INCREMENT:
            case MC_BINCMD_DECREMENT:
                if (pHdr->extralen != sizeof(pReqX->incrdecr))
                    return NULL;
                m_parms.value.length = parmAdjLen(0);
                pOpt->m_iFlags = pHdr->opcode;
                if (m_usecas)
                    pOpt->m_iFlags |= LSMC_WITHCAS;
                pOpt->m_value = (uint64_t)ntohll(pReqX->incrdecr.delta);
                pOpt->m_cas = pHdr->cas;
                pOpt->m_pRet = (void *)&m_item;
                break;

            case MC_BINCMD_APPEND:
                pOpt->m_iFlags = MC_BINCMD_APPEND;
                if (m_usecas)
                    pOpt->m_iFlags |= LSMC_WITHCAS;
                pOpt->m_cas = pHdr->cas;
                pOpt->m_iRetcode = UPDRET_APPEND;
                break;

            case MC_BINCMD_PREPEND:
                pOpt->m_iFlags = MC_BINCMD_PREPEND;
                if (m_usecas)
                    pOpt->m_iFlags |= LSMC_WITHCAS;
                pOpt->m_cas = pHdr->cas;
                pOpt->m_iRetcode = UPDRET_PREPEND;
                break;

            case MC_BINCMD_VERBOSITY:
                if (pHdr->extralen != sizeof(pReqX->verbosity))
                    return NULL;
                break;

            default:
                break;
        }
        pBody += keyLen;
    }
    return pBody;
}


void LsShmMemCached::doBinGet(McBinCmdHdr *pHdr, bool doTouch)
{
    uint8_t resBuf[sizeof(McBinCmdHdr)+sizeof(McBinResExtra)];
    int keyLen = ((pHdr->opcode == MC_BINCMD_GATK)
        || (pHdr->opcode == MC_BINCMD_GETK)) ?
        m_parms.key.length : 0;
    lock();
    if ((m_iterOff = m_pHash->findIterator(&m_parms)) != 0)
    {
        iterator iter;
        LsMcDataItem *pItem;
        int valLen;
        uint8_t *valPtr;
        iter = m_pHash->offset2iterator(m_iterOff);
        pItem = mcIter2data(iter, m_usecas, &valPtr, &valLen);
        if (isExpired(pItem))
        {
            m_pHash->eraseIterator(m_iterOff);
            if (doTouch)
                statTouchMiss();
            else
                statGetMiss();
        }
        else
        {
            int extra = sizeof(((McBinResExtra *)0)->value.flags);
            McBinResExtra *pResX = (McBinResExtra *)(&resBuf[sizeof(McBinCmdHdr)]);
            pResX->value.flags = (uint32_t)htonl(pItem->x_flags);
            if (doTouch)
            {
                setItemExptime(pItem, m_item.x_exptime);
                statTouchHit();
            }
            else
                statGetHit();
            saveCas(pItem);
            if (pHdr->opcode == MC_BINCMD_TOUCH)
                valLen = 0;     // return only flags
            setupBinResHdr(pHdr->opcode,
                (uint8_t)extra, (uint16_t)keyLen, (uint32_t)extra + keyLen + valLen,
                MC_BINSTAT_SUCCESS, resBuf);
            binRespond(resBuf, sizeof(McBinCmdHdr) + extra);
            if (keyLen != 0)
                binRespond((uint8_t *)m_parms.key.pstr, m_parms.key.length);
            if (valLen != 0)
                binRespond(valPtr, valLen);
            unlock();
            return;
        }
    }
    else if (doTouch)
        statTouchMiss();
    else
        statGetMiss();
    unlock();
    if (keyLen != 0)
    {
        m_noreply = false;
        setupBinResHdr(pHdr->opcode,
            0, (uint16_t)keyLen, (uint32_t)keyLen,
            MC_BINSTAT_KEYENOENT, resBuf);
        binRespond(resBuf, sizeof(McBinCmdHdr));
        binRespond((uint8_t *)m_parms.key.pstr, m_parms.key.length);
    }
    else
    {
        binErrRespond(pHdr, MC_BINSTAT_KEYENOENT);
    }
    return;
}


int LsShmMemCached::doBinDataUpdate(uint8_t *pBuf, McBinCmdHdr *pHdr)
{
    m_parms.key.length = 0;
    if (m_iterOff != 0)
    {
        dataItemUpdate(pBuf);
        unlock();
        if (m_retcode != UPDRET_NONE)
            binOkRespond(pHdr);
        m_iterOff = 0;
    }
    else
    {
        McBinStat stat;
        unlock();
        if (m_retcode == UPDRET_NOTFOUND)
            stat = MC_BINSTAT_KEYENOENT;
        else if (m_retcode == UPDRET_CASFAIL)
            stat = MC_BINSTAT_KEYEEXISTS;
        else
            stat = MC_BINSTAT_NOTSTORED;
        binErrRespond(pHdr, stat);
    }
    return 0;
}


void LsShmMemCached::doBinDelete(McBinCmdHdr *pHdr)
{
    lock();
    if ((m_iterOff = m_pHash->findIterator(&m_parms)) != 0)
    {
        LsMcDataItem *pItem =
            (LsMcDataItem *)m_pHash->offset2iteratorData(m_iterOff);
        if (isExpired(pItem))
        {
            m_pHash->eraseIterator(m_iterOff);
            statDeleteMiss();
            unlock();
            binErrRespond(pHdr, MC_BINSTAT_KEYENOENT);
        }
        else if (m_usecas && (pHdr->cas != 0)
                 && (pHdr->cas != pItem->x_data->withcas.cas))
        {
            unlock();
            binErrRespond(pHdr, MC_BINSTAT_KEYEEXISTS);
        }
        else
        {
            saveCas(pItem);
            m_pHash->eraseIterator(m_iterOff);
            statDeleteHit();
            unlock();
            binOkRespond(pHdr);
        }
    }
    else
    {
        statDeleteMiss();
        unlock();
        binErrRespond(pHdr, MC_BINSTAT_KEYENOENT);
    }
    return;
}


void LsShmMemCached::doBinArithmetic(McBinCmdHdr *pHdr, LsShmUpdOpt *pOpt)
{
    char numBuf[ULL_MAXLEN+1];
    uint8_t resBuf[sizeof(McBinCmdHdr)+sizeof(McBinResExtra)];

    // return value in `extra' field
    int extra = sizeof(((McBinResExtra *)0)->incrdecr.newval);
    McBinResExtra *pResX = (McBinResExtra *)(&resBuf[sizeof(McBinCmdHdr)]);

    pOpt->m_pMisc = (void *)numBuf;
    lock();
    if ((m_iterOff = m_pHash->updateIterator(&m_parms, pOpt)) != 0)
    {
        if (pHdr->opcode == MC_BINCMD_INCREMENT)
            statIncrHit();
        else
            statDecrHit();
        m_retcode = UPDRET_NONE;
        doBinDataUpdate((uint8_t *)numBuf, pHdr);
        setupBinResHdr(pHdr->opcode,
            (uint8_t)0, (uint16_t)0, (uint32_t)extra,
            MC_BINSTAT_SUCCESS, resBuf);
        pResX->incrdecr.newval = (uint64_t)htonll(strtoull(numBuf, NULL, 10));
        binRespond(resBuf, sizeof(McBinCmdHdr) + extra);
    }
    else if (pOpt->m_iRetcode == UPDRET_NOTFOUND)
    {
        McBinReqExtra *pReqX = (McBinReqExtra *)(pHdr + 1);
        if (pReqX->incrdecr.exptime == 0xffffffff)
        {
            if (pHdr->opcode == MC_BINCMD_INCREMENT)
                statIncrMiss();
            else
                statDecrMiss();
            unlock();
            binErrRespond(pHdr, MC_BINSTAT_KEYENOENT);
        }
        else
        {
            m_item.x_flags = 0;
            m_item.x_exptime = (time_t)ntohl(pReqX->incrdecr.exptime);
            m_parms.value.length = parmAdjLen(
                snprintf(numBuf, sizeof(numBuf), "%llu",
                  (unsigned long long)ntohll(pReqX->incrdecr.initval)));
            if ((m_iterOff = m_pHash->insertIterator(&m_parms)) != 0)
            {
                m_retcode = UPDRET_NONE;
                doBinDataUpdate((uint8_t *)numBuf, pHdr);
                setupBinResHdr(pHdr->opcode,
                    (uint8_t)0, (uint16_t)0, (uint32_t)extra,
                    MC_BINSTAT_SUCCESS, resBuf);
                pResX->incrdecr.newval = pReqX->incrdecr.initval;
                binRespond(resBuf, sizeof(McBinCmdHdr) + extra);
            }
            else
            {
                unlock();
                binErrRespond(pHdr, MC_BINSTAT_NOTSTORED);
            }
        }
    }
    else
    {
        McBinStat stat;
        unlock();
        if (pOpt->m_iRetcode == UPDRET_NONNUMERIC)
            stat = MC_BINSTAT_DELTABADVAL;
        else if (pOpt->m_iRetcode == UPDRET_CASFAIL)
            stat = MC_BINSTAT_KEYEEXISTS;
        else
            stat = MC_BINSTAT_NOTSTORED;
        binErrRespond(pHdr, stat);
    }
    return;
}


void LsShmMemCached::doBinVersion(McBinCmdHdr *pHdr)
{
    uint8_t resBuf[sizeof(McBinCmdHdr)];
    int len = strlen(VERSION);
    setupBinResHdr(pHdr->opcode,
        (uint8_t)0, (uint16_t)0, (uint32_t)len, MC_BINSTAT_SUCCESS, resBuf);
    binRespond(resBuf, sizeof(McBinCmdHdr));
    binRespond((uint8_t *)VERSION, len);
}


void LsShmMemCached::setupBinResHdr(uint8_t cmd,
    uint8_t extralen, uint16_t keylen, uint32_t totbody,
    uint16_t status, uint8_t *pBinBuf)
{
    if (m_noreply)
        return;

    McBinCmdHdr *pHdr = (McBinCmdHdr *)pBinBuf;
    pHdr->magic = MC_BINARY_RES;
    pHdr->opcode = cmd;
    pHdr->keylen = (uint16_t)htons(keylen);
    pHdr->extralen = extralen;
    pHdr->datatype = 0;
    pHdr->status = (uint16_t)htons(status);
    pHdr->totbody = (uint32_t)htonl(totbody);
    pHdr->opaque = 0;
    pHdr->cas = (uint64_t)htonll(m_rescas);

    if (getVerbose() > 1)
    {
        int fd = 0;
        int i;
        uint8_t *p = pBinBuf;
        fprintf(stderr, ">%d Writing bin response:", fd);
        for (i = 0; i < (int)sizeof(McBinCmdHdr); ++i)
        {
            if ((i & 0x03) == 0)
                fprintf(stderr, "\n>%d  ", fd);
            fprintf(stderr, " 0x%02x", *p++);
        }
        fprintf(stderr, "\n");
    }
    return;
}


void LsShmMemCached::binOkRespond(McBinCmdHdr *pHdr)
{
    uint8_t resBuf[sizeof(McBinCmdHdr)];
    setupBinResHdr(pHdr->opcode, (uint8_t)0, (uint16_t)0, (uint32_t)0,
                   MC_BINSTAT_SUCCESS, resBuf);
    binRespond(resBuf, sizeof(McBinCmdHdr));
    return;
}


void LsShmMemCached::binErrRespond(McBinCmdHdr *pHdr, McBinStat err)
{
    const char *text;
    uint8_t resBuf[sizeof(McBinCmdHdr)];
    switch (err)
    {
    case MC_BINSTAT_KEYENOENT:
        text = "Not found";
        break;
    case MC_BINSTAT_KEYEEXISTS:
        text = "Data exists for key.";
        break;
    case MC_BINSTAT_E2BIG:
        text = "Too large.";
        break;
    case MC_BINSTAT_EINVAL:
        text = "Invalid arguments";
        break;
    case MC_BINSTAT_NOTSTORED:
        text = "Not stored.";
        break;
    case MC_BINSTAT_DELTABADVAL:
        text = "Non-numeric server-side value for incr or decr";
        break;
    case MC_BINSTAT_AUTHERROR:
        text = "Auth failure.";
        break;
    case MC_BINSTAT_UNKNOWNCMD:
        text = "Unknown command";
        break;
    case MC_BINSTAT_ENOMEM:
        text = "Out of memory";
        break;
    default:
        text = "UNHANDLED ERROR";
        fprintf(stderr, "UNHANDLED ERROR: %d\n", err);
    }

    if (getVerbose() > 1)
    {
        fprintf(stderr, ">%d Writing an error: %s\n", 0, text);
    }

    int len = strlen(text);
    m_iterOff = 0;
    m_rescas = 0;
    m_noreply = false;
    setupBinResHdr(pHdr->opcode, 0, (uint16_t)0, (uint32_t)len, err, resBuf);
    binRespond(resBuf, sizeof(McBinCmdHdr));
    binRespond((uint8_t *)text, len);
    return;
}


char *LsShmMemCached::advToken(char *pStr, char **pTokPtr, int *pTokLen)
{
    char c;
    while (*pStr ==  ' ')
       ++pStr;
    *pTokPtr = pStr;
    while (((c = *pStr) != ' ') && (c != '\0'))
        ++pStr;
    *pTokLen = pStr - *pTokPtr;
    if (c == ' ')
        *pStr++ = '\0';     /* null terminate */
    return pStr;
}


bool LsShmMemCached::myStrtol(const char *pStr, long *pVal)
{
    char *endptr;
    errno = 0;
    long val = strtol(pStr, &endptr, 10);
    if ((errno == ERANGE) || (endptr == pStr) || (*endptr && !isspace(*endptr)))
        return false;
    *pVal = val;
    return true;
}


bool LsShmMemCached::myStrtoul(const char *pStr, unsigned long *pVal)
{
    char *endptr;
    errno = 0;
    unsigned long val = strtoul(pStr, &endptr, 10);
    if ((errno == ERANGE) || (endptr == pStr) || (*endptr && !isspace(*endptr)))
        return false;
    *pVal = val;
    return true;
}


bool LsShmMemCached::myStrtoll(const char *pStr, long long *pVal)
{
    char *endptr;
    errno = 0;
    long long val = strtoll(pStr, &endptr, 10);
    if ((errno == ERANGE) || (endptr == pStr) || (*endptr && !isspace(*endptr)))
        return false;
    *pVal = val;
    return true;
}


bool LsShmMemCached::myStrtoull(const char *pStr, unsigned long long *pVal)
{
    char *endptr;
    errno = 0;
    unsigned long long val = strtoull(pStr, &endptr, 10);
    if ((errno == ERANGE) || (endptr == pStr) || (*endptr && !isspace(*endptr)))
        return false;
    *pVal = val;
    return true;
}


int LsShmMemCached::convertCmd(char *pStr, uint8_t *pBinBuf)
{
    char *pCmd;
    int len;
    LsMcCmdFunc *p;

    pStr = advToken(pStr, &pCmd, &len);
    if (len <= 0)
        return -1;
    if ((p = getCmdFunction(pCmd)) == NULL)
    {
        respond("CLIENT_ERROR unknown command");
        return -1;
    }
    if ((*p->func)(this, pStr, p->arg, pBinBuf) < 0)
    {
        respond("ERROR converting to binary");
        return -1;
    }
    return 0;
}


static int asc2bincmd(int arg, char *pkey, int keylen, char *pval, int vallen,
    uint32_t flags, uint32_t exptime, uint64_t cas, bool quiet, uint8_t *pout)
{
    McBinCmdHdr *pHdr = (McBinCmdHdr *)pout;
    char *pbody = (char *)(pHdr + 1);
    McBinReqExtra *pReqX;
    pHdr->magic = MC_BINARY_REQ;
    pHdr->keylen = (uint16_t)htons(keylen);
    pHdr->datatype = 0;
    pHdr->status = 0;
    pHdr->opaque = 0;
    pHdr->cas = cas;

    pReqX = (McBinReqExtra *)pbody;
    switch(arg)
    {
        case MC_BINCMD_ADD:
            pHdr->opcode = (quiet ? MC_BINCMD_ADDQ : MC_BINCMD_ADD);
            pReqX->value.flags = htonl(flags);
            pReqX->value.exptime = htonl(exptime);
            pHdr->extralen = sizeof(pReqX->value);
            break;
        case MC_BINCMD_SET:
            pHdr->opcode = (quiet ? MC_BINCMD_SETQ : MC_BINCMD_SET);
            pReqX->value.flags = htonl(flags);
            pReqX->value.exptime = htonl(exptime);
            pHdr->extralen = sizeof(pReqX->value);
            break;
        case MC_BINCMD_REPLACE:
        case MC_BINCMD_REPLACE|LSMC_WITHCAS:
            pHdr->opcode = (quiet ? MC_BINCMD_REPLACEQ : MC_BINCMD_REPLACE);
            pReqX->value.flags = htonl(flags);
            pReqX->value.exptime = htonl(exptime);
            pHdr->extralen = sizeof(pReqX->value);
            break;
        case MC_BINCMD_DELETE:
            pHdr->opcode = (quiet ? MC_BINCMD_DELETEQ : MC_BINCMD_DELETE);
            pHdr->extralen = 0;
            break;
        case MC_BINCMD_INCREMENT:   // definitely a hack,
        case MC_BINCMD_DECREMENT:   // but this is a temporary test function
            if (arg == MC_BINCMD_INCREMENT)
                pHdr->opcode =
                    (quiet ? MC_BINCMD_INCREMENTQ : MC_BINCMD_INCREMENT);
            else /* if (arg == MC_BINCMD_DECREMENT) */
                pHdr->opcode =
                    (quiet ? MC_BINCMD_DECREMENTQ : MC_BINCMD_DECREMENT);
            pHdr->cas = 0;      // cannot set this from ascii mode
            pReqX->incrdecr.delta = htonll(cas);
            pReqX->incrdecr.initval = (uint64_t)flags;
            pReqX->incrdecr.initval = htonll(pReqX->incrdecr.initval);
            pReqX->incrdecr.exptime = htonl(exptime);
            pHdr->extralen = sizeof(pReqX->incrdecr);
            break;
        case MC_BINCMD_APPEND:
            pHdr->opcode = (quiet ? MC_BINCMD_APPENDQ : MC_BINCMD_APPEND);
            pHdr->extralen = 0;
            break;
        case MC_BINCMD_PREPEND:
            pHdr->opcode = (quiet ? MC_BINCMD_PREPENDQ : MC_BINCMD_PREPEND);
            pHdr->extralen = 0;
            break;
        case MC_BINCMD_VERBOSITY:
            pHdr->opcode = MC_BINCMD_VERBOSITY;
            pReqX->verbosity.level = htonl(flags);
            pHdr->extralen = sizeof(pReqX->verbosity);
            break;
        case MC_BINCMD_GAT:
            pHdr->opcode = (quiet ? MC_BINCMD_GATQ : MC_BINCMD_GAT);
            pReqX->touch.exptime = htonl(exptime);
            pHdr->extralen = sizeof(pReqX->touch);
            break;
        case MC_BINCMD_GATK:
            pHdr->opcode = (quiet ? MC_BINCMD_GATKQ : MC_BINCMD_GATK);
            pReqX->touch.exptime = htonl(exptime);
            pHdr->extralen = sizeof(pReqX->touch);
            break;
        case MC_BINCMD_TOUCH:
            pHdr->opcode = MC_BINCMD_TOUCH;
            pReqX->touch.exptime = htonl(exptime);
            pHdr->extralen = sizeof(pReqX->touch);
            break;
        default:
            pHdr->opcode = arg;
            pHdr->extralen = 0;
            break;
    }
    pbody += pHdr->extralen;
    ::memcpy(pbody, pkey, keylen);
    if (vallen != 0)
        ::memcpy(pbody + keylen, pval, vallen);
    pHdr->totbody = (uint32_t)htonl(pHdr->extralen + keylen + vallen);

    return 0;
}

