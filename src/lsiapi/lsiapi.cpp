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
#include <lsiapi/lsiapi.h>
#include <http/httpsession.h>
#include <http/httprespheaders.h>
#include <http/httpvhost.h>
#include <util/vmembuf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <lsiapi/modulehandler.h>
#include <lsiapi/internal.h>
#include <lsiapi/lsiapilib.h>
#include <lsiapi/lsiapigd.h>
#include <http/requestvars.h>
#include <http/staticfilecachedata.h>
#include <lsiapi/lsiapi.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include "modulemanager.h"
#include "util/ghash.h"
#include <lsr/ls_fileio.h>
#include <util/gpath.h>
#include <util/datetime.h>
#include <stdio.h>

lsi_api_t LsiapiBridge::g_lsiapiFunctions;
const lsi_api_t *g_api = &LsiapiBridge::g_lsiapiFunctions;
GDataContainer *LsiapiBridge::g_aGDataContainer[LSI_CONTAINER_COUNT] = {0};

void  LsiapiBridge::releaseModuleData(int level, LsiModuleData *pData)
{
    if (!pData->isDataInited())
        return;
    LsiApiHooks *pHooks = LsiApiHooks::getReleaseDataHooks(level);
    void *data = NULL;
    for (lsiapi_hook_t *hook = pHooks->begin(); hook < pHooks->end(); ++hook)
    {
        data = pData->get(MODULE_DATA_ID(hook->module)[level]);
        if (data)
        {
            lsi_release_callback_pf cb = (lsi_release_callback_pf)hook->cb;
            cb(data);
            pData->set(MODULE_DATA_ID(hook->module)[level], NULL);
        }
    }
}

void LsiapiBridge::checkExpiredGData()
{
    time_t tm = DateTime::s_curTime;

    GDataContainer *pLsiGDataContHashT = NULL;
    lsi_gdata_cont_t *containerInfo = NULL;
    GDataContainer::iterator iter;
    GDataHash::iterator iter2;

    for (int i = 0; i < LSI_CONTAINER_COUNT; ++i)
    {
        pLsiGDataContHashT = g_aGDataContainer[i];
        if (!pLsiGDataContHashT)
            continue;
        for (iter = pLsiGDataContHashT->begin(); iter != pLsiGDataContHashT->end();
             iter = pLsiGDataContHashT->next(iter))
        {
            containerInfo = iter.second();
            GDataHash *pCont = containerInfo->container;
            for (iter2 = pCont->begin(); iter2 != pCont->end();
                 iter2 = pCont->next(iter2))
            {
                if (iter.second() && iter2.second()->tmexpire < tm)
                    erase_gdata_elem(containerInfo, iter2);
            }
        }
    }
}


int LsiapiBridge::initLsiapi()
{
    g_lsiapiFunctions.get_gdata_container = get_gdata_container;
    g_lsiapiFunctions.empty_gdata_container = empty_gdata_container;
    g_lsiapiFunctions.purge_gdata_container = purge_gdata_container;

    g_lsiapiFunctions.get_gdata = get_gdata;
    g_lsiapiFunctions.delete_gdata = delete_gdata;
    g_lsiapiFunctions.set_gdata = set_gdata;

    lsiapi_init_server_api();

    init_gdata_hashes();
    return 0;
}

void LsiapiBridge::uninitLsiapi()
{
    uninit_gdata_hashes();
}


// void ModuleEventNotifier::removeEventObj(EventObj **pEventObj)
// {
//     if ((*pEventObj) && (*pEventObj)->m_iState == 1)
//     {
//         (*pEventObj)->remove();
//         (*pEventObj)->m_iId = 0;
//         (*pEventObj)->m_iLevel = 0;
//         (*pEventObj)->m_pModule = 0;
//         (*pEventObj)->m_pSession = 0;
//         (*pEventObj)->m_iState = 0;
//         delete *pEventObj;
//         *pEventObj = NULL;
//     }
// }
//
// int ModuleEventNotifier::onNotified(int count)
// {
//     while (!m_eventObjListDone.empty())
//     {
//         EventObj *pObj = (EventObj *)m_eventObjListDone.begin();
//         if (pObj)
//         {
//             if (pObj->m_pModule)
//                 pObj->m_pSession->hookResumeCallback(pObj->m_iLevel, pObj->m_pModule);
//             removeEventObj(&pObj);
//         }
//         else
//             break;
//     }
//     return 0;
// }
//
// EventObj *ModuleEventNotifier::addEventObj(lsi_session_t *pSession,
//         lsi_module_t *pModule, int level)
// {
//     EventObj *pEventObj = new EventObj;
//     if (pEventObj)
//     {
//         pEventObj->m_iId = m_iId ++;
//         pEventObj->m_iLevel = level;
//         pEventObj->m_pModule = pModule;
//         pEventObj->m_pSession = (LsiSession *)pSession;
//         pEventObj->m_iState = 1;
//         m_eventObjListWait.push_front(pEventObj);
//     }
//     return pEventObj;
// }
//
// int ModuleEventNotifier::isEventObjValid(EventObj *pEventObj)
// {
//     return (pEventObj->next() != NULL && pEventObj->prev() != NULL);
// }
//
// int ModuleEventNotifier::notifyEventObj(EventObj **pEventObj)
// {
//     //pEventObj should be in Wait list
//     //assert();
//
//     if (*pEventObj == NULL)
//         return -1;
//
//     if (!isEventObjValid(*pEventObj))
//         removeEventObj(pEventObj);
//
//     //Move from Wait list to Done list
//     EventObj *pNewObj = new EventObj;
//     if (!pNewObj)
//         return -1; //ERROR
//
//     pNewObj->m_iId = (*pEventObj)->m_iId;
//     pNewObj->m_iLevel = (*pEventObj)->m_iLevel;
//     pNewObj->m_pModule = (*pEventObj)->m_pModule;
//     pNewObj->m_pSession = (*pEventObj)->m_pSession;
//     pNewObj->m_iState = 1;
//     m_eventObjListDone.push_front(pNewObj);
//     removeEventObj(pEventObj);
//
//     notify();
//     return 0;
// }



