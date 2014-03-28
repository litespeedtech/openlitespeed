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
#include "modulehandler.h"
#include "http/handlertype.h"
#include <http/httpreq.h>
#include <http/httpresp.h>
#include "modulemanager.h"
#include <lsiapi/internal.h>
#include <util/vmembuf.h>

ModuleHandler::ModuleHandler()
{
    ReqHandler::setType(HandlerType::HT_MODULE);
}

ModuleHandler::~ModuleHandler()
{
}

int ModuleHandler::cleanUp(HttpSession* pSession)
{
    return 0;
}

int ModuleHandler::onWrite(HttpSession* pSession)
{
//     if (!pSession->getStream()->isWantWrite())
//         return 0;
    
    const LsiModule* pHandler = ((const LsiModule *)pSession->getReq()->getHttpHandler());
   
    if (pHandler->m_pModule->_handler->on_write_resp)
    {
        int status = pHandler->m_pModule->_handler->on_write_resp(((void *)((LsiSession *)pSession)));
        if (status != LSI_WRITE_RESP_CONTINUE)
            pSession->endDynResp(0);
        return (status == LSI_WRITE_RESP_CONTINUE);
    }
    else
    {
        pSession->setFlag( HSF_MODULE_WRITE_SUSPENDED );
        return LSI_WRITE_RESP_CONTINUE;
    }
}

int ModuleHandler::process(HttpSession* pSession, const HttpHandler* pHandler)
{
    if (pHandler == NULL)
        return  SC_500;

    pSession->resetResp();
    pSession->setupRespCache();
    
    int ret = ((LsiModule*)pHandler)->m_pModule->_handler->begin_process(((void *)((LsiSession *)pSession)));
    if (ret != 0)
        return HttpStatusCode::codeToIndex( ret );
    else
    {
        if ( !pSession->getFlag( HSF_MODULE_WRITE_SUSPENDED | HSF_RESP_DONE ) )
            pSession->continueWrite();
        return 0;
    }
}

int ModuleHandler::onRead( HttpSession* pSession )
{
    const LsiModule* pHandler = ((const LsiModule *)pSession->getReq()->getHttpHandler());
    if (pHandler->m_pModule->_handler->on_read_req_body)
        return pHandler->m_pModule->_handler->on_read_req_body(((void *)((LsiSession *)pSession)));
    else
        return 0;
}
