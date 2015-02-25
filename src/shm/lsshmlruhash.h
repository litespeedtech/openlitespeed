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

#include <string.h>
#include <shm/lsshmpool.h>
#include <shm/lsshmhash.h>
#include <shm/lsshmlru.h>

/**
 * @file
 */


class LsShmLruHash : public LsShmHash
{
public:
    int trim(time_t tmCutoff);
    int check();

    virtual int setLruData(
        LsShmOffset_t offVal, const uint8_t *pVal, int valLen)
    {
        if (m_pLru == NULL)
            return LS_FAIL;
        int ret = 0;
        shmlru_data_t *pData = (shmlru_data_t *)offset2ptr(offVal);
        if ((pData->maxsize == 0)
            || (entryValMatch((const uint8_t *)pVal, valLen, pData) == 0))
            ret = newDataEntry((const uint8_t *)pVal, valLen, pData);
        return ret;
    }

    virtual int getLruData(
        LsShmOffset_t offVal, LsShmOffset_t *pData, int cnt)
    {
        if (m_pLru == NULL)
            return LS_FAIL;
        return ((cnt > 0) ? (*pData = offVal, 1) : 0);
    }

    virtual int getLruDataPtrs(
        LsShmOffset_t offVal, int (*func)(void *pData))
    {
        if (m_pLru == NULL)
            return LS_FAIL;
        (*func)((void *)offset2ptr(offVal));
        return 1;
    }

    LsShmLruHash(LsShmPool *pool,
                 const char *name, size_t init_size, hash_fn hf, val_comp vc);
    virtual ~LsShmLruHash() {}

protected:
    void err_cleanup();

    virtual int clrdata(uint8_t *pValue);
    virtual int chkdata(uint8_t *pValue);

    // auxiliary double linked list of hash elements
    void set_linkNext(LsShmOffset_t offThis, LsShmOffset_t offNext)
    {
        ((LsShmHElem *)offset2ptr(offThis))->setLruLinkNext(offNext);
    }

    void set_linkPrev(LsShmOffset_t offThis, LsShmOffset_t offPrev)
    {
        ((LsShmHElem *)offset2ptr(offThis))->setLruLinkPrev(offPrev);
    }

    void linkHElem(LsShmHElem *pElem, LsShmOffset_t offElem)
    {
        LsShmHElemLink *pLink = pElem->getLruLinkPtr();
        if (m_pLru->linkFirst)
        {
            set_linkNext(m_pLru->linkFirst, offElem);
            pLink->x_iLinkPrev = m_pLru->linkFirst;
        }
        else
        {
            pLink->x_iLinkPrev = 0;
            m_pLru->linkLast = offElem;
        }
        pLink->x_iLinkNext = 0;
        m_pLru->linkFirst = offElem;
    }

    void unlinkHElem(LsShmHElem *pElem)
    {
        LsShmHElemLink *pLink = pElem->getLruLinkPtr();
        if (pLink->x_iLinkNext)
            set_linkPrev(pLink->x_iLinkNext, pLink->x_iLinkPrev);
        else
            m_pLru->linkFirst = pLink->x_iLinkPrev;
        if (pLink->x_iLinkPrev)
            set_linkNext(pLink->x_iLinkPrev, pLink->x_iLinkNext);
        else
            m_pLru->linkLast = pLink->x_iLinkNext;
    }

    void linkSetTop(LsShmHElem *pElem)
    {
        LsShmHElemLink *pLink = pElem->getLruLinkPtr();
        LsShmOffset_t next = pLink->x_iLinkNext;
        if (next != 0)      // not top of list already
        {
            LsShmOffset_t offElem = ptr2offset(pElem);
            LsShmOffset_t prev = pLink->x_iLinkPrev;
            if (prev == 0)  // last one
                m_pLru->linkLast = next;
            else
                set_linkNext(prev, next);
            set_linkPrev(next, prev);
            pLink->x_iLinkNext = 0;
            pLink->x_iLinkPrev = m_pLru->linkFirst;
            set_linkNext(m_pLru->linkFirst, offElem);
            m_pLru->linkFirst = offElem;
        }
        pLink->x_lasttime = time((time_t *)NULL);
    }

    int entryValMatch(const uint8_t *pVal, int valLen, shmlru_data_t *pData);

private:
    LsShmLruHash(const LsShmLruHash &other);
    LsShmLruHash &operator=(const LsShmLruHash &other);

    int newDataEntry(const uint8_t *pVal, int valLen, shmlru_data_t *pData);

    void lruSpecial(iterator Iter)
    {
        shmlru_data_t *pData = (shmlru_data_t *)Iter->getVal();
        pData->maxsize = 0;
        pData->offiter = ptr2offset(Iter);
        return;
    }
    void valueSetup(uint32_t *pValOff, int *pValLen)
    {
        *pValOff += sizeof(LsShmHElemLink);
        *pValLen += sizeof(shmlru_data_t);
        return;
    }
};

class LsShmXLruHash : public LsShmLruHash
{
public:
    LsShmXLruHash(LsShmPool *pool,
                  const char *name, size_t init_size, hash_fn hf, val_comp vc);
    ~LsShmXLruHash() {}

private:
    LsShmXLruHash(const LsShmXLruHash &other);
    LsShmXLruHash &operator=(const LsShmXLruHash &other);

    void lruSpecial(iterator Iter)
    {
        ((shmlru_val_t *)Iter->getVal())->offdata = 0;
        return;
    }
    void valueSetup(uint32_t *pValOff, int *pValLen)
    {
        *pValOff += sizeof(LsShmHElemLink);
        *pValLen = sizeof(shmlru_val_t);
        return;
    }

    int newDataEntry(const uint8_t *pVal, int valLen, shmlru_val_t *pShmval);

    int setLruData(LsShmOffset_t offVal, const uint8_t *pVal, int valLen)
    {
        if (m_pLru == NULL)
            return LS_FAIL;
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
        if (m_pLru == NULL)
            return LS_FAIL;
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
        if (m_pLru == NULL)
            return LS_FAIL;
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
