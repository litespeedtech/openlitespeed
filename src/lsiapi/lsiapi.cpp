/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013  LiteSpeed Technologies, Inc.                        *
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

#include <http/httpglobals.h>
#include <http/staticfilecachedata.h>
#include <lsiapi/lsiapi.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include "modulemanager.h"
#include "util/ghash.h"
#include <util/ni_fio.h>
#include <util/gpath.h>
#include <util/datetime.h>
#include <stdio.h>

lsi_api_t LsiapiBridge::gLsiapiFunctions;
const lsi_api_t * g_api = &LsiapiBridge::gLsiapiFunctions;
__LsiGDataContHashT *LsiapiBridge::gLsiGDataContHashT[LSI_CONTAINER_COUNT] = {0};

void  LsiapiBridge::releaseModuleData( int level, LsiModuleData * pData )
{
    LsiApiHooks* pHooks = LsiApiHooks::getReleaseDataHooks(level);
    void *data = NULL;
    for (LsiApiHook* hook = pHooks->begin(); hook < pHooks->end(); ++hook)
    {
        data = pData->get( MODULE_DATA_ID(hook->_module)[level]);
        if ( data )
        {
            lsi_release_callback_pf cb = (lsi_release_callback_pf)hook->_cb;
            cb(data);
            pData->set( MODULE_DATA_ID(hook->_module)[level], NULL );
        }
    }
}

void LsiapiBridge::expire_gdata_check()
{
    time_t tm = DateTime::s_curTime;
    
    __LsiGDataContHashT *pLsiGDataContHashT = NULL;
    lsi_gdata_cont_val_t *containerInfo = NULL;
    __LsiGDataContHashT::iterator iter;
    __LsiGDataItemHashT::iterator iter2;

    for(int i=0;i<LSI_CONTAINER_COUNT; ++i )
    {
        pLsiGDataContHashT = gLsiGDataContHashT[i];
        if ( !pLsiGDataContHashT )
            continue;
        for(iter = pLsiGDataContHashT->begin(); iter != pLsiGDataContHashT->end(); iter = pLsiGDataContHashT->next(iter))
        {
            containerInfo = iter.second();
            __LsiGDataItemHashT *pCont = containerInfo->container;
            for(iter2 = pCont->begin(); iter2 != pCont->end(); iter2 = pCont->next(iter2))
            {
                if (iter.second() && iter2.second()->tmExpire < tm)
                    erase_gdata_element(containerInfo, iter2);
            }
        }
    }
}


int LsiapiBridge::init_lsiapi()
{
    gLsiapiFunctions.get_gdata_container = get_gdata_container;
    gLsiapiFunctions.empty_gdata_container = empty_gdata_container;
    gLsiapiFunctions.purge_gdata_container = purge_gdata_container;
            
    gLsiapiFunctions.get_gdata = get_gdata;
    gLsiapiFunctions.delete_gdata = delete_gdata;
    gLsiapiFunctions.set_gdata = set_gdata;
        
    lsiapi_init_server_api();

    AllGlobalDataHashTInit();
    return 0;
}

void LsiapiBridge::uninit_lsiapi()
{
    AllGlobalDataHashTUnInit();
}
