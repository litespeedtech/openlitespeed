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


#define MCCMD_ADD       0x01
#define MCCMD_SET       0x02
#define MCCMD_REPLACE   0x03
#define MCCMD_PREPEND   0x04
#define MCCMD_APPEND    0x05
#define MCCMD_MASK      0xff

#define MCCMD_WITHCAS   0x0100

LsMcCmdFunc LsShmMemCached::s_LsMcCmdFuncs[] =
{
    { "get",        0,  doCmdGet },
    { "bget",       0,  doCmdGet },
    { "gets",       MCCMD_WITHCAS,  doCmdGet },
    { "add",        MCCMD_ADD,      doCmdUpdate },
    { "set",        MCCMD_SET,      doCmdUpdate },
    { "replace",    MCCMD_REPLACE,  doCmdUpdate },
    { "prepend",    MCCMD_PREPEND,  doCmdUpdate },
    { "append",     MCCMD_APPEND,   doCmdUpdate },
    { "cas",        0,  doCmdUpdate },
    { "delete",     0,  doCmdDelete },
    { "quit",       0,  doCmdQuit },
};


static uint64_t getCas()
{
    return (uint64_t)12345678;
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
        char *valPtr;
        iterator iter = m_pHash->offset2iterator(m_iterOff);
        LsMcDataItem *pItem = (LsMcDataItem *)iter->getVal();
        ::memcpy((void *)pItem, (void *)&m_item, sizeof(m_item));
        valLen = m_parms.value.length - sizeof(*pItem);
        if (m_usecas)
        {
            pItem->x_data->withcas.cas = getCas();
            valLen -= sizeof(pItem->x_data->withcas.cas);
            valPtr = (char *)pItem->x_data->withcas.val;
        }
        else
        {
            valPtr = (char *)pItem->x_data->val;
        }
        if (valLen > 0)
            ::memcpy(valPtr, (void *)pBuf, valLen);
        m_needed = 0;
        m_iterOff = 0;
        respond("STORED");
    }
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
    char *valPtr;
    while (1)
    {
        pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                               &pThis->m_parms.key.length);
        if (pThis->m_parms.key.length <= 0)
            break;
        if ((iterOff = pHash->findIterator(&pThis->m_parms)) != 0)
        {
            iter = pHash->offset2iterator(iterOff);
            pItem = (LsMcDataItem *)iter->getVal();
            valLen = iter->getValLen() - sizeof(*pItem);
            if (pThis->m_usecas)
            {
                valLen -= sizeof(pItem->x_data->withcas.cas);
                valPtr = (char *)pItem->x_data->withcas.val;
            }
            else
            {
                valPtr = (char *)pItem->x_data->val;
            }
            if (arg & MCCMD_WITHCAS)
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
    unsigned long flags;
    long exptime;
    long length;
    pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                           &pThis->m_parms.key.length);
    pStr = pThis->advToken(pStr, &pFlags, &tokLen);
    pStr = pThis->advToken(pStr, &pExptime, &tokLen);
    pStr = pThis->advToken(pStr, &pLength, &tokLen);
    if ((tokLen <= 0)
        || (!pThis->myStrtoul(pFlags, &flags))
        || (!pThis->myStrtol(pExptime, &exptime))
        || (!pThis->myStrtol(pLength, &length))
    )
    {
        pThis->respond("CLIENT_ERROR bad command format");
        return -1;
    }
    pThis->m_parms.value.pstr = NULL;
    pThis->m_parms.value.length = sizeof(m_item) + length;
    if (pThis->m_usecas)
        pThis->m_parms.value.length += sizeof(pThis->m_item.x_data->withcas.cas);
    pThis->m_item.x_flags = (uint32_t)flags;
    pThis->m_item.x_exptime = (uint32_t)exptime;
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional noreply
    pThis->m_noreply = ((tokLen == 7) && (strcmp(tokPtr, "noreply") == 0));

    switch (arg)
    {
        case MCCMD_ADD:
            pThis->m_iterOff = pThis->m_pHash->insertIterator(&pThis->m_parms);
            break;
        case MCCMD_SET:
            pThis->m_iterOff = pThis->m_pHash->setIterator(&pThis->m_parms);
            break;
        case MCCMD_REPLACE:
            pThis->m_iterOff = pThis->m_pHash->updateIterator(&pThis->m_parms);
            break;
        case MCCMD_PREPEND:
        case MCCMD_APPEND:
        default:
            pThis->respond("SERVER_ERROR unhandled type");
            return -1;
    }
    pThis->m_needed = length;
    if (length == 0)
        pThis->doDataUpdate(NULL);      // empty item
    return length;
}


int LsShmMemCached::doCmdDelete(LsShmMemCached *pThis, char *pStr, int arg)
{
    char *tokPtr;
    int tokLen;
    LsShmHash::iteroffset iterOff;
    pStr = pThis->advToken(pStr, &pThis->m_parms.key.pstr,
                           &pThis->m_parms.key.length);
    pStr = pThis->advToken(pStr, &tokPtr, &tokLen);     // optional
    if ((tokLen == 1) && (strcmp(tokPtr, "0") == 0))    // hold_is_zero???
        pStr = pThis->advToken(pStr, &tokPtr, &tokLen);

    pThis->m_noreply = ((tokLen == 7) && (strcmp(tokPtr, "noreply") == 0));
    if (pThis->m_noreply)
        pThis->advToken(pStr, &tokPtr, &tokLen);

    if (tokLen > 0)
    {
        pThis->respond("CLIENT_ERROR bad delete format");
        return -1;
    }
    if ((iterOff = pThis->m_pHash->findIterator(&pThis->m_parms)) != 0)
        pThis->m_pHash->eraseIterator(iterOff);
    pThis->respond((iterOff != 0) ? "DELETED" : "NOT_FOUND");
    return 0;
}


int LsShmMemCached::doCmdQuit(LsShmMemCached *pThis, char *pStr, int arg)
{
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

