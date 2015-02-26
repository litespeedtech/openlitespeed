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

#include <lsr/ls_lfqueue.h>
#include <lsr/ls_pool.h>
#include <lsr/ls_internal.h>
#include <lsr/ls_atomic.h>

//#define LSR_LLQ_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>

#define MYPAUSE     sched_yield()
//#define MYPAUSE     usleep( 250 )

#ifdef LSR_LLQ_DEBUG
static int MYINDX;
#endif


ls_lfqueue_t *ls_lfqueue_new()
{
    ls_lfqueue_t *pThis;
    if ((pThis = (ls_lfqueue_t *)ls_palloc(sizeof(*pThis))) != NULL)
    {
        if (ls_lfqueue_init(pThis) < 0)
        {
            ls_pfree(pThis);
            pThis = NULL;
        }
    }

    return pThis;
}

int ls_lfqueue_init(ls_lfqueue_t *pThis)
{
    pThis->tail.m_ptr = NULL;
    pThis->tail.m_seq = 0;
    pThis->phead = (volatile ls_lfnodei_t **)&pThis->tail.m_ptr;

    return 0;
}

void ls_lfqueue_destroy(ls_lfqueue_t *pThis)
{
    if (pThis)
        memset(pThis, 0, sizeof(*pThis));
    return;
}

void ls_lfqueue_delete(ls_lfqueue_t *pThis)
{
    if (pThis)
    {
        ls_lfqueue_destroy(pThis);
        ls_pfree(pThis);
    }
    return;
}

int ls_lfqueue_put(ls_lfqueue_t *pThis, ls_lfnodei_t *data)
{
    data->next = NULL;
    ls_lfnodei_t *prev = ls_atomic_setptr((void **)&pThis->phead, data);
    prev->next = data;

    return 0;
}

int ls_lfqueue_putn(
    ls_lfqueue_t *pThis, ls_lfnodei_t *data1, ls_lfnodei_t *datan)
{
    datan->next = NULL;
    ls_lfnodei_t *prev = ls_atomic_setptr((void **)&pThis->phead, datan);
    prev->next = data1;

    return 0;
}

ls_lfnodei_t *ls_lfqueue_get(ls_lfqueue_t *pThis)
{
    ls_atom_ptr_t tail;

    tail.m_ptr = pThis->tail.m_ptr;
    tail.m_seq = pThis->tail.m_seq;
    while (1)
    {
        ls_atom_ptr_t prev;
        ls_atom_ptr_t xchg;
        ls_lfnodei_t *pnode = (ls_lfnodei_t *)tail.m_ptr;
        if (pnode == NULL)
            return NULL;

        if (pnode->next)
        {
            xchg.m_ptr = (void *)pnode->next;
            xchg.m_seq = tail.m_seq + 1;

            ls_atomic_dcasv(
                (ls_atom_ptr_t *)&pThis->tail, &tail, &xchg, &prev);

            if ((prev.m_ptr == tail.m_ptr)
                && (prev.m_seq == tail.m_seq))
                return pnode;
        }
        else
        {
            ls_lfnodei_t *p = (ls_lfnodei_t *)pThis->phead;
            if (pnode != p)
            {
                MYPAUSE;
                tail.m_ptr = pThis->tail.m_ptr;
                tail.m_seq = pThis->tail.m_seq;
                continue;
            }
            xchg.m_ptr = NULL;
            xchg.m_seq = tail.m_seq + 1;

            ls_atomic_dcasv(
                (ls_atom_ptr_t *)&pThis->tail, &tail, &xchg, &prev);

            if ((prev.m_ptr == tail.m_ptr)
                && (prev.m_seq == tail.m_seq))
            {
                ls_lfnodei_t *prevhead = (ls_lfnodei_t *)ls_atomic_casvptr(
                                             (void **)&pThis->phead, pnode, (void *)&pThis->tail.m_ptr);

                if (prevhead == pnode)
                    return pnode;

                while (pnode->next == NULL)
                    MYPAUSE;
                pThis->tail.m_ptr = (void *)pnode->next;

                return pnode;
            }
        }
        tail.m_ptr = prev.m_ptr;
        tail.m_seq = prev.m_seq;
    }
}

int ls_lfqueue_empty(ls_lfqueue_t *pThis)
{
    return (pThis->tail.m_ptr == NULL);
}
