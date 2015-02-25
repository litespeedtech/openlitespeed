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
#include "lsiapihooks.h"
#include "lsiapi.h"
#include <http/httplog.h>
#include <http/httpsession.h>
#include <util/logtracker.h>
#include <lsiapi/internal.h>

#include <string.h>

const char *LsiApiHooks::s_pHkptName[LSI_HKPT_TOTAL_COUNT] =
{
    "L4_BEGINSESSION",
    "L4_ENDSESSION",
    "L4_RECVING",
    "L4_SENDING",
    "HTTP_BEGIN",
    "RECV_REQ_HEADER",
    "URI_MAP",
    "HTTP_AUTH",
    "RECV_REQ_BODY",
    "RCVD_REQ_BODY",
    "RECV_RESP_HEADER",
    "RECV_RESP_BODY",
    "RCVD_RESP_BODY",
    "HANDLER_RESTART",
    "SEND_RESP_HEADER",
    "SEND_RESP_BODY",
    "HTTP_END",
    "MAIN_INITED",
    "MAIN_PREFORK",
    "MAIN_POSTFORK",
    "WORKER_POSTFORK",
    "WORKER_ATEXIT",
    "MAIN_ATEXIT"
};

LsiApiHooks LsiApiHooks::s_hooks[LSI_HKPT_TOTAL_COUNT];
ServerSessionHooks *LsiApiHooks::s_pServerSessionHooks = NULL;
static LsiApiHooks *s_releaseDataHooks = NULL;

#define LSI_HOOK_FLAG_NO_INTERRUPT  1


void LsiApiHooks::initGlobalHooks()
{
    s_pServerSessionHooks = new ServerSessionHooks();
    s_releaseDataHooks = new LsiApiHooks[LSI_MODULE_DATA_COUNT];

    s_hooks[ LSI_HKPT_L4_BEGINSESSION ].setGlobalFlag(
        LSI_HOOK_FLAG_NO_INTERRUPT);
    s_hooks[ LSI_HKPT_L4_ENDSESSION ].setGlobalFlag(
        LSI_HOOK_FLAG_NO_INTERRUPT);

    s_hooks[ LSI_HKPT_HTTP_BEGIN ].setGlobalFlag(LSI_HOOK_FLAG_NO_INTERRUPT);
    s_hooks[ LSI_HKPT_HTTP_END ].setGlobalFlag(LSI_HOOK_FLAG_NO_INTERRUPT);
    s_hooks[ LSI_HKPT_HANDLER_RESTART ].setGlobalFlag(
        LSI_HOOK_FLAG_NO_INTERRUPT);
}


int LsiApiHooks::runForwardCb(lsi_cb_param_t *param)
{
    lsiapi_hookinfo_t *hookInfo = param->_hook_info;
    lsiapi_hook_t *hook = (lsiapi_hook_t *)param->_cur_hook;
    lsiapi_hook_t *hookEnd = hookInfo->hooks->end();
    int8_t *flag = hookInfo->enable_array + (hook -
                   hookInfo->hooks->begin());
    while (hook < hookEnd)
    {
        if (*flag++ == 0)
        {
            ++hook;
            continue;
        }
        param->_cur_hook = (void *)hook;
        return (*(((lsiapi_hook_t *)param->_cur_hook)->cb))(param);
    }

    return param->_hook_info->term_fn(
               (LsiSession *)param->_session, (void *)param->_param, param->_param_len);
}

int LsiApiHooks::runBackwardCb(lsi_cb_param_t *param)
{
    lsiapi_hookinfo_t *hookInfo = param->_hook_info;
    lsiapi_hook_t *hook = (lsiapi_hook_t *)param->_cur_hook;
    lsiapi_hook_t *hookBegin = hookInfo->hooks->begin();
    int8_t *flag = hookInfo->enable_array + (hook - hookBegin);
    while (hook >= hookBegin)
    {
        if (*flag-- == 0)
        {
            --hook;
            continue;
        }
        param->_cur_hook = (void *)hook;
        return (*(((lsiapi_hook_t *)param->_cur_hook)->cb))(param);
    }

    return param->_hook_info->term_fn(
               (LsiSession *)param->_session, (void *)param->_param, param->_param_len);
}


LsiApiHooks *LsiApiHooks::getReleaseDataHooks(int index)
{   return &s_releaseDataHooks[index];   }

LsiApiHooks::LsiApiHooks(const LsiApiHooks &other)
    : m_pHooks(NULL)
    , m_iCapacity(0)
    , m_iBegin(0)
    , m_iEnd(0)
    , m_iFlag(0)
{
    if (reallocate(other.size() + 4, 2) != -1)
    {
        memcpy(m_pHooks + m_iBegin, other.begin(),
               (char *)other.end() - (char *)other.begin());
        m_iEnd = m_iBegin + other.size();
    }
}

int LsiApiHooks::copy(const LsiApiHooks &other)
{
    if (m_iCapacity < other.size())
        if (reallocate(other.size() + 4, 2) == -1)
            return LS_FAIL;
    m_iBegin = (m_iCapacity - other.size()) / 2;
    memcpy(m_pHooks + m_iBegin, other.begin(),
           (char *)other.end() - (char *)other.begin());
    m_iEnd = m_iBegin + other.size();
    return 0;
}


int LsiApiHooks::reallocate(int capacity, int newBegin)
{
    lsiapi_hook_t *pHooks;
    int size = this->size();
    if (capacity < size)
        capacity = size;
    if (capacity < size + newBegin)
        capacity = size + newBegin;
    pHooks = (lsiapi_hook_t *)malloc(capacity * sizeof(lsiapi_hook_t));
    if (!pHooks)
        return LS_FAIL;
    if (newBegin < 0)
        newBegin = (capacity - size) / 2;
    if (m_pHooks)
    {
        memcpy(pHooks + newBegin, begin(), size * sizeof(lsiapi_hook_t));
        free(m_pHooks);
    }

    m_pHooks = pHooks;
    m_iEnd = newBegin + size;
    m_iBegin = newBegin;
    m_iCapacity = capacity;
    return capacity;

}

//For same cb and same priority in same module, it will fail to add, but return 0
short LsiApiHooks::add(const lsi_module_t *pModule, lsi_callback_pf cb,
                       short priority, short flag)
{
    lsiapi_hook_t *pHook;
    if (size() == m_iCapacity)
    {
        if (reallocate(m_iCapacity + 4) == -1)
            return LS_FAIL;

    }
    for (pHook = begin(); pHook < end(); ++pHook)
    {
        if (pHook->priority > priority)
            break;
        else if (pHook->priority == priority && pHook->module == pModule
                 && pHook->cb == cb)
            return 0;  //already added
    }

    if ((pHook != begin()) || (m_iBegin == 0))
    {
        //cannot insert front
        if ((pHook != end()) || (m_iEnd == m_iCapacity))
        {
            //cannot append directly
            if (m_iEnd < m_iCapacity)
            {
                //moving entries toward the end
                memmove(pHook + 1, pHook , (char *)end() - (char *)pHook);
                ++m_iEnd;
            }
            else
            {
                //moving entries before the insert point
                memmove(begin() - 1, begin(), (char *)pHook - (char *)begin());
                --m_iBegin;
                --pHook;
            }
        }
        else
            ++m_iEnd;
    }
    else
    {
        --pHook;
        --m_iBegin;
    }
    pHook->module = pModule;
    pHook->cb = cb;
    pHook->priority = priority;
    pHook->flag = flag;
    return pHook - begin();
}

lsiapi_hook_t *LsiApiHooks::find(const lsi_module_t *pModule,
                                 lsi_callback_pf cb)
{
    lsiapi_hook_t *pHook;
    for (pHook = begin(); pHook < end(); ++pHook)
    {
        if ((pHook->module == pModule) && (pHook->cb == cb))
            return pHook;
    }
    return NULL;
}

int LsiApiHooks::remove(lsiapi_hook_t *pHook)
{
    if ((pHook < begin()) || (pHook >= end()))
        return LS_FAIL;
    if (pHook == begin())
        ++m_iBegin;
    else
    {
        if (pHook != end() - 1)
            memmove(pHook, pHook + 1, (char *)end() - (char *)pHook);
        --m_iEnd;
    }
    return 0;
}

//COMMENT: if one module add more hooks at this level, the return is the first one!!!!
lsiapi_hook_t *LsiApiHooks::find(const lsi_module_t *pModule) const
{
    lsiapi_hook_t *pHook;
    for (pHook = begin(); pHook < end(); ++pHook)
    {
        if (pHook->module == pModule)
            return pHook;
    }
    return NULL;

}

lsiapi_hook_t *LsiApiHooks::find(const lsi_module_t *pModule,
                                 int *index) const
{
    lsiapi_hook_t *pHook = find(pModule);
    if (pHook)
        *index = pHook - begin();
    return pHook;
}

//COMMENT: if one module add more hooks at this level, the return of
//LsiApiHooks::find( const lsi_module_t *pModule ) is the first one!!!!
//But the below function will remove all the hooks
int LsiApiHooks::remove(const lsi_module_t *pModule)
{
    lsiapi_hook_t *pHook;
    for (pHook = begin(); pHook < end(); ++pHook)
    {
        if (pHook->module == pModule)
            remove(pHook);
    }

    return 0;
}


//need to check Hook count before call this function
int LsiApiHooks::runCallback(int level, lsi_cb_param_t *param) const
{
    int ret = 0;
    lsi_cb_param_t rec1;
    int8_t *pEnableArray = param->_hook_info->enable_array;

    lsiapi_hook_t *hook = (lsiapi_hook_t *)param->_cur_hook;
    lsiapi_hook_t *hookEnd = end();
    pEnableArray += (lsiapi_hook_t *)param->_cur_hook - begin();

    while (hook < hookEnd)
    {
        if (*pEnableArray++ == 0)
        {
            ++hook;
            continue;
        }

        rec1 = *param;
        rec1._cur_hook = (void *)hook;

        if (D_ENABLED(DL_MORE))
        {
            if (param->_session)
            {
                LogTracker *pTracker = ((LsiSession *)param->_session)->getLogTracker();
                LOG_D((pTracker->getLogger(),
                       "[%s] [%s] run Hook function for [Module:%s]",
                       pTracker->getLogId(), s_pHkptName[ level ], MODULE_NAME(hook->module)));
            }
            else
            {
                LOG_D((NULL, "[ServerHook: %s] run Hook function for [Module:%s]",
                       s_pHkptName[ level ], MODULE_NAME(hook->module)));
            }
        }

        ret = hook->cb(&rec1);

        if (D_ENABLED(DL_MORE))
        {
            if (param->_session)
            {
                LogTracker *pTracker = ((LsiSession *)param->_session)->getLogTracker();
                LOG_D((pTracker->getLogger(), "[%s] [%s] [Module:%s] ret %d",
                       pTracker->getLogId(), s_pHkptName[ level ], MODULE_NAME(hook->module),
                       ret));
            }
            else
            {
                LOG_D((NULL, "[ServerHook: %s] [Module:%s] ret %d",
                       s_pHkptName[ level ], MODULE_NAME(hook->module), ret));
            }
        }
        if ((ret == LSI_HK_RET_SUSPEND) || ((ret != 0)
                                         && !(m_iFlag & LSI_HOOK_FLAG_NO_INTERRUPT)))
            break;

        ++hook;
    }
    param->_cur_hook = hook;

    return ret;
}



