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


int LsShmMemCached::notImplemented(LsShmMemCached *pThis, char *pStr, int arg)
{
    pThis->respond("NOT_IMPLEMENTED");
    return 0;
}


LsMcCmdFunc LsShmMemCached::s_LsMcCmdFuncs[] =
{
    { "get",        0,              doCmdGet },
    { "bget",       0,              doCmdGet },
    { "gets",       LSMC_WITHCAS,   doCmdGet },
    { "add",        LSMC_ADD,       doCmdUpdate },
    { "set",        LSMC_SET,       doCmdUpdate },
    { "replace",    LSMC_REPLACE,   doCmdUpdate },
    { "append",     LSMC_APPEND,    doCmdUpdate },
    { "prepend",    LSMC_PREPEND,   doCmdUpdate },
    { "cas",        LSMC_CAS,       doCmdUpdate },
    { "incr",       LSMC_INCR,      doCmdArithmetic },
    { "decr",       LSMC_DECR,      doCmdArithmetic },
    { "delete",     0,              doCmdDelete },
    { "touch",      0,              doCmdTouch },
    { "stat",       0,              notImplemented },
    { "flush_all",  0,              notImplemented },
    { "version",    0,              doCmdVersion },
    { "quit",       0,              doCmdQuit },
    { "shutdown",   0,              notImplemented },
    { "verbosity",  0,              doCmdVerbosity },
};

const char badCmdLineFmt[] = "CLIENT_ERROR bad command line format";

const char tokNoreply[] = "noreply";
static inline bool chkNoreply(char *tokPtr, int tokLen)
{
    return ((tokLen == sizeof(tokNoreply)-1) && (strcmp(tokPtr, tokNoreply) == 0));
}


uint64_t LsShmMemCached::getCas()
{
    return ((m_iHdrOff != 0) ? ++*(uint64_t *)m_pHash->offset2ptr(
        (LsShmOffset_t)(long)&((LsMcHdr *)m_iHdrOff)->x_data->cas) : 0);
}


void LsShmMemCached::respond(const char *str)
{
    if (m_noreply)
        m_noreply = false;
    else
        fprintf(stdout, "%s\r\n", str);
}


void LsShmMemCached::sendResult(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vfprintf(stdout, fmt, va);
    va_end(va);
}


LsShmMemCached::LsShmMemCached(LsShmHash *pHash, bool usecas)
    : m_pHash(pHash)
    , m_iterOff(0)
    , m_needed(0)
    , m_noreply(false)
    , m_usecas(usecas)
{
    ls_str_blank(&m_parms.key);
    ls_str_blank(&m_parms.value);
    LsShmOffset_t off;
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
            if (m_usecas)
                pHdr->x_data->cas = 0;
            pHash->setUserOff(off);
            pHash->setUserSize(size);
        }
    }
    m_iHdrOff = off;
}


int LsShmMemCached::processCmd(char *pStr)
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
    return (*p->func)(this, pStr, p->arg);
}


/*
 * this code needs the lock continued from the parsed command function.
 */
int LsShmMemCached::doDataUpdate(char *pBuf)
{
    if (m_parms.key.length <= 0)
    {
        respond("CLIENT_ERROR invalid data update");
        return -1;
    }

    if (m_iterOff != 0)
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
            pItem->x_data->withcas.cas = getCas();
        if (valLen > 0)
            ::memcpy(valPtr, (void *)pBuf, valLen);
        m_needed = 0;
        m_iterOff = 0;
        if (m_retcode != UPDRET_NONE)
            respond("STORED");
    }
    else if (m_retcode == UPDRET_NOTFOUND)
        respond("NOT_FOUND");
    else if (m_retcode == UPDRET_CASFAIL)
        respond("EXISTS");
    else
        respond("NOT_STORED");

    m_parms.key.length = 0;
    return 0;
}


int LsShmMemCached::doCmdGet(LsShmMemCached *pThis, char *pStr, int arg)
{
    LsShmHash *pHash = pThis->m_pHash;
    iterator iter;
    LsShmHash::iteroffset iterOff;
    LsMcDataItem *pItem;
    int valLen;
    uint8_t *valPtr;
    while (1)
    {
        pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                               &pThis->m_parms.key.length);
        if (pThis->m_parms.key.length <= 0)
            break;
        if ((iterOff = pHash->findIterator(&pThis->m_parms)) != 0)
        {
            iter = pHash->offset2iterator(iterOff);
            pItem = mcIter2data(iter, pThis->m_usecas, &valPtr, &valLen);
            if ((pItem->x_exptime != 0)
                && (pItem->x_exptime <= time((time_t *)NULL)))
            {
                pHash->eraseIterator(iterOff);
            }
            else if (arg & LSMC_WITHCAS)
            {
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
                pThis->sendResult("VALUE %.*s %d %d\r\n%.*s\r\n",
                  iter->getKeyLen(), iter->getKey(),
                  pItem->x_flags,
                  valLen,
                  valLen, valPtr
                );
            }
        }
    }
    pThis->sendResult("END\r\n");
    return 0;
}


int LsShmMemCached::doCmdUpdate(LsShmMemCached *pThis, char *pStr, int arg)
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
    if ((arg == LSMC_APPEND) || (arg == LSMC_PREPEND))
    {
        if (pThis->m_usecas)
            updOpt.m_iFlags |= LSMC_WITHCAS;
    }
    else
    {
        pThis->m_parms.value.length += sizeof(pThis->m_item);
        if (pThis->m_usecas)
            pThis->m_parms.value.length += sizeof(pThis->m_item.x_data->withcas.cas);
    }
    if (arg == LSMC_CAS)
    {
        pStr = pThis->advToken(pStr, &pCas, &tokLen);
        if ((tokLen <= 0) || (!pThis->myStrtoull(pCas, &cas)))
        {
            pThis->respond(badCmdLineFmt);
            return -1;
        }
        updOpt.m_value = (uint64_t)cas;
    }
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional noreply
    pThis->m_noreply = chkNoreply(tokPtr, tokLen);

    switch (arg)
    {
        case LSMC_ADD:
            pThis->m_iterOff = pThis->m_pHash->insertIterator(&pThis->m_parms);
            pThis->m_retcode = UPDRET_DONE;
            break;
        case LSMC_SET:
            pThis->m_iterOff = pThis->m_pHash->setIterator(&pThis->m_parms);
            pThis->m_retcode = UPDRET_DONE;
            break;
        case LSMC_REPLACE:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms);
            pThis->m_retcode = UPDRET_DONE;
            break;
        case LSMC_APPEND:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms,
                                                              &updOpt);
            pThis->m_retcode = UPDRET_APPEND;
            break;
        case LSMC_PREPEND:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms,
                                                              &updOpt);
            pThis->m_retcode = UPDRET_PREPEND;
            break;
        case LSMC_CAS:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms,
                                                              &updOpt);
            pThis->m_retcode = updOpt.m_iRetcode;
            break;
        default:
            pThis->respond("SERVER_ERROR unhandled type");
            return -1;
    }
    pThis->m_needed = length;
    if (length == 0)
        pThis->doDataUpdate(NULL);      // empty item
    return length;
}


int LsShmMemCached::doCmdArithmetic(LsShmMemCached *pThis, char *pStr, int arg)
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
    pThis->m_parms.value.length = sizeof(pThis->m_item);
    updOpt.m_iFlags = arg;
    updOpt.m_value = (uint64_t)delta;
    updOpt.m_pRet = (void *)&pThis->m_item;
    updOpt.m_pMisc = (void *)numBuf;
    if (pThis->m_usecas)
    {
        pThis->m_parms.value.length += sizeof(pThis->m_item.x_data->withcas.cas);
        updOpt.m_iFlags |= LSMC_WITHCAS;
    }

    if ((pThis->m_iterOff =
        pThis->m_pHash->updateIterator(&pThis->m_parms, &updOpt)) != 0)
    {
        pThis->m_retcode = UPDRET_NONE;
        pThis->doDataUpdate(numBuf);
        pThis->respond(numBuf);
    }
    else if (updOpt.m_iRetcode == UPDRET_NOTFOUND)
        pThis->respond("NOT_FOUND");
    else if (updOpt.m_iRetcode == UPDRET_NONNUMERIC)
        pThis->respond(
          "CLIENT_ERROR cannot increment or decrement non-numeric value");
    else
        pThis->respond("SERVER_ERROR unable to update");
    return 0;
}


int LsShmMemCached::doCmdDelete(LsShmMemCached *pThis, char *pStr, int arg)
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
    if ((iterOff = pThis->m_pHash->findIterator(&pThis->m_parms)) != 0)
    {
        expired = pThis->isExpired(
            (LsMcDataItem *)pThis->m_pHash->offset2iteratorData(iterOff));
        pThis->m_pHash->eraseIterator(iterOff);
    }
    pThis->respond(((iterOff != 0) && !expired) ? "DELETED" : "NOT_FOUND");
    return 0;
}


int LsShmMemCached::doCmdTouch(LsShmMemCached *pThis, char *pStr, int arg)
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
    if ((iterOff = pThis->m_pHash->findIterator(&pThis->m_parms)) != 0)
    {
        pItem = (LsMcDataItem *)pThis->m_pHash->offset2iteratorData(iterOff);
        if (pThis->isExpired(pItem))
            pThis->m_pHash->eraseIterator(iterOff);
        else
        {
            if ((exptime != 0) && (exptime <= LSMC_MAXDELTATIME))
                exptime += time((time_t *)NULL);
            pItem->x_exptime = (time_t)exptime;
            pThis->respond("TOUCHED");
            return 0;
        }
    }
    pThis->respond("NOT_FOUND");
    return 0;
}


int LsShmMemCached::doCmdVersion(LsShmMemCached *pThis, char *pStr, int arg)
{
    pThis->respond("VERSION " VERSION);
    return 0;
}


int LsShmMemCached::doCmdQuit(LsShmMemCached *pThis, char *pStr, int arg)
{
    return 0;
}


int LsShmMemCached::doCmdVerbosity(LsShmMemCached *pThis, char *pStr, int arg)
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
    if (pThis->m_iHdrOff != 0)
    {
        ((LsMcHdr *)pThis->m_pHash->offset2ptr(pThis->m_iHdrOff))->x_verbose = verbose;
        pThis->respond("OK");
    }
    else
        pThis->respond("FAILED");
    return 0;
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

