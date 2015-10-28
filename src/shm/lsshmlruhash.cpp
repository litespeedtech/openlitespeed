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
#include <shm/lsshmlruhash.h>
#include <shm/lsshmpool.h>

#include <unistd.h>

#define LSSHM_LRU_MAGIC     0x20141201


static int entryValMatch(
    const uint8_t *pVal, int valLen, shmlru_data_t *pData)
{
    if (((int)pData->size != valLen)
        || (memcmp(pData->data, pVal, valLen) != 0))
        return 0;
    pData->lasttime = time((time_t *)NULL);
    ++pData->count;

    return 1;
}

int LsShmWLruHash::setLruData(LsShmOffset_t offVal, const uint8_t *pVal, int valLen)
{
    int ret = 0;
    shmlru_data_t *pData = (shmlru_data_t *)offset2ptr(offVal);
    if ((pData->maxsize == 0)
        || (entryValMatch((const uint8_t *)pVal, valLen, pData) == 0))
        ret = newDataEntry((const uint8_t *)pVal, valLen, pData);
    return ret;
}


int LsShmXLruHash::setLruData(LsShmOffset_t offVal, const uint8_t *pVal, int valLen)
{
    int ret = 0;
    shmlru_val_t *pShmval = (shmlru_val_t *)offset2ptr(offVal);
    if ((pShmval->offdata == 0)
        || (entryValMatch((const uint8_t *)pVal, valLen,
                            (shmlru_data_t *)offset2ptr(pShmval->offdata)) == 0))
        ret = newDataEntry((const uint8_t *)pVal, valLen, pShmval);
    return ret;
}


int LsShmWLruHash::newDataEntry(
    const uint8_t *pVal, int valLen, shmlru_data_t *pData)
{
    uint16_t maxsize;
    if ((maxsize = pData->maxsize) != 0)
    {
        if (valLen > maxsize)
        {
            LsShmHElem *pElem = (LsShmHElem *)offset2ptr(pData->offiter);
            LsShmHKey key = pElem->x_hkey;
            ls_strpair_t parms;
            iteroffset iterOff;
            ls_str_set(&parms.key,
                       (char *)pElem->getKey(), pElem->getKeyLen());
            ls_str_set(&parms.value, NULL, valLen);
            if ((iterOff = insert2(key, &parms)) == 0)
                return LS_FAIL;
            eraseIteratorHelper(pElem);
            pData = (shmlru_data_t *)offset2iteratorData(iterOff);
            pData->offiter = iterOff;
            maxsize = 0;
        }
        ++(getLru()->ndatadel);
        --(getLru()->ndataset);
    }
    if (maxsize == 0)
    {
        pData->magic = LSSHM_LRU_MAGIC;
        pData->maxsize = valLen;
    }
    time_t now = time((time_t *)NULL);
    pData->strttime = now;
    pData->lasttime = now;
    pData->count = 1;
    pData->size = valLen;
    memcpy(pData->data, pVal, valLen);
    ++(getLru()->ndataset);

    return 0;
}


int LsShmWLruHash::clrdata(uint8_t *pValue)
{
    return 1;       // one data entry per value
}


int LsShmWLruHash::chkdata(uint8_t *pValue)
{
    return ((((shmlru_data_t *)pValue)->magic != LSSHM_LRU_MAGIC) ?
            SHMLRU_BADMAGIC : 1);
}


int LsShmXLruHash::newDataEntry(
    const uint8_t *pVal, int valLen, shmlru_val_t *pShmval)
{
    LsShmOffset_t offData;
    shmlru_data_t *pData;
    if ((offData = pShmval->offdata) != 0)
    {
        pData = (shmlru_data_t *)offset2ptr(offData);
        if (pData->count >= SHMLRU_MINSAVECNT)
            pShmval->offdata = 0;
        else
        {
            ++(getLru()->ndatadel);
            --(getLru()->ndataset);
            if (valLen > pData->maxsize)
            {
                offData = pData->offprev;
                release2(pShmval->offdata,
                         (LsShmSize_t)(sizeof(shmlru_data_t) + pData->maxsize));
                pShmval->offdata = 0;
            }
        }
    }
    if (pShmval->offdata == 0)
    {
        int remapped;
        LsShmOffset_t offNew;
        if ((offNew = alloc2(
            (LsShmSize_t)(sizeof(shmlru_data_t) + valLen), remapped)) == 0)
            return LS_FAIL;
        pData = (shmlru_data_t *)offset2ptr(offNew);
        pData->magic = LSSHM_LRU_MAGIC;
        pData->maxsize = valLen;
        pData->offprev = offData;
        pShmval->offdata = offNew;
    }
    time_t now = time((time_t *)NULL);
    pData->strttime = now;
    pData->lasttime = now;
    pData->count = 1;
    pData->size = valLen;
    memcpy(pData->data, pVal, valLen);
    ++(getLru()->ndataset);

    return 0;
}


int LsShmXLruHash::clrdata(uint8_t *pValue)
{
    shmlru_data_t *pData;
    LsShmOffset_t offData = ((shmlru_val_t *)pValue)->offdata;
    LsShmOffset_t prev;
    int cnt = 0;
    while (offData != 0)
    {
        pData = (shmlru_data_t *)offset2ptr(offData);
        prev = pData->offprev;
        release2(offData, (LsShmSize_t)(sizeof(shmlru_data_t) + pData->maxsize));
        ++cnt;
        offData = prev;
    }
    return cnt;
}


int LsShmXLruHash::chkdata(uint8_t *pValue)
{
    shmlru_data_t *pData;
    LsShmOffset_t offData = ((shmlru_val_t *)pValue)->offdata;
    int cnt = 0;
    while (offData != 0)
    {
        ++cnt;
        pData = (shmlru_data_t *)offset2ptr(offData);
        if (pData->magic != LSSHM_LRU_MAGIC)
            return SHMLRU_BADMAGIC;
        offData = pData->offprev;
    }
    return cnt;
}
