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
#ifndef LSSHMLRUHASH_H
#define LSSHMLRUHASH_H

#ifdef LSSHM_DEBUG_ENABLE
class debugBase;
#endif

#include <shm/lsshmhash.h>
#include <shm/lsshmlru.h>

/**
 * @file
 */


class LsShmPool;

class LsShmWLruHash : public LsShmHash
{
public:
    LsShmWLruHash(LsShmPool *pool, const char *name, size_t init_size,
                  hash_fn hf, val_comp vc)
    : LsShmHash(pool, name, init_size, hf, vc, LSSHM_LRU_MODE2)
    {   return;  }
    virtual ~LsShmWLruHash() {}

private:
    LsShmWLruHash(const LsShmWLruHash &other);
    LsShmWLruHash &operator=(const LsShmWLruHash &other);

    void valueSetup(uint32_t *pValOff, int *pValLen)
    {
        *pValOff += sizeof(LsShmHElemLink);
        *pValLen += sizeof(shmlru_data_t);
        return;
    }

    int newDataEntry(const uint8_t *pVal, int valLen, shmlru_data_t *pData);

    int setLruData(LsShmOffset_t offVal, const uint8_t *pVal, int valLen)
    {
        int ret = 0;
        shmlru_data_t *pData = (shmlru_data_t *)offset2ptr(offVal);
        if ((pData->maxsize == 0)
            || (entryValMatch((const uint8_t *)pVal, valLen, pData) == 0))
            ret = newDataEntry((const uint8_t *)pVal, valLen, pData);
        return ret;
    }

    int getLruData(LsShmOffset_t offVal, LsShmOffset_t *pData, int cnt)
    {
        return ((cnt > 0) ? (*pData = offVal, 1) : 0);
    }

    int getLruDataPtrs(LsShmOffset_t offVal, int (*func)(void *pData))
    {
        (*func)((void *)offset2ptr(offVal));
        return 1;
    }

    int clrdata(uint8_t *pValue);
    int chkdata(uint8_t *pValue);
};

class LsShmXLruHash : public LsShmHash
{
public:
    LsShmXLruHash(LsShmPool *pool, const char *name, size_t init_size,
                  hash_fn hf, val_comp vc)
    : LsShmHash(pool, name, init_size, hf, vc, LSSHM_LRU_MODE3)
    {   return;  }
    virtual ~LsShmXLruHash() {}

private:
    LsShmXLruHash(const LsShmXLruHash &other);
    LsShmXLruHash &operator=(const LsShmXLruHash &other);

    void valueSetup(uint32_t *pValOff, int *pValLen)
    {
        *pValOff += sizeof(LsShmHElemLink);
        *pValLen = sizeof(shmlru_val_t);
        return;
    }

    int newDataEntry(const uint8_t *pVal, int valLen, shmlru_val_t *pShmval);

    int setLruData(LsShmOffset_t offVal, const uint8_t *pVal, int valLen)
    {
        int ret = 0;
        shmlru_val_t *pShmval = (shmlru_val_t *)offset2ptr(offVal);
        if ((pShmval->offdata == 0)
            || (entryValMatch((const uint8_t *)pVal, valLen,
                              (shmlru_data_t *)offset2ptr(pShmval->offdata)) == 0))
            ret = newDataEntry((const uint8_t *)pVal, valLen, pShmval);
        return ret;
    }

    int getLruData(LsShmOffset_t offVal, LsShmOffset_t *pData, int cnt)
    {
        int ret = 0;
        LsShmOffset_t data = ((shmlru_val_t *)offset2ptr(offVal))->offdata;
        while ((--cnt >= 0) && (data != 0))
        {
            *pData++ = data;
            ++ret;
            data = ((shmlru_data_t *)offset2ptr(data))->offprev;
        }
        return ret;
    }

    int getLruDataPtrs(LsShmOffset_t offVal, int (*func)(void *pData))
    {
        int ret = 0;
        LsShmOffset_t data = ((shmlru_val_t *)offset2ptr(offVal))->offdata;
        while (data != 0)
        {
            shmlru_data_t *pData = (shmlru_data_t *)offset2ptr(data);
            ++ret;
            if ((*func)((void *)pData) < 0)
                break;
            data = pData->offprev;
        }
        return ret;
    }

    int clrdata(uint8_t *pValue);
    int chkdata(uint8_t *pValue);
};

#endif // LSSHMLRUHASH_H
