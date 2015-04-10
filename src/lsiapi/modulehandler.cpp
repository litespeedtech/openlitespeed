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
#include "modulehandler.h"

#include <http/handlertype.h>
#include <http/httphandler.h>
#include <http/httplog.h>
#include <http/httpsession.h>
#include <http/httpstatuscode.h>
#include <lsiapi/internal.h>
#include <lsiapi/modulemanager.h>

ModuleHandler::ModuleHandler()
{
    ReqHandler::setType(HandlerType::HT_MODULE);
}


ModuleHandler::~ModuleHandler()
{
}


static const lsi_handler_t *getHandler(const HttpHandler *pHandler)
{

    const lsi_handler_t *pModuleHandler = NULL;
    if (pHandler)
        pModuleHandler = ((const LsiModule *)pHandler)->getModule()->_handler;

    if (!pModuleHandler)
    {
        LOG_ERR(("Internal Server Error, Module %s misses _handler definition,"
                 " cannot be used as a handler",
                 MODULE_NAME(((const LsiModule *)pHandler)->getModule())));
    }

    return pModuleHandler;
}


int ModuleHandler::cleanUp(HttpSession *pSession)
{
    const HttpHandler *pHandler;
    const lsi_handler_t *pModuleHandler = pSession->getModHandler();

    if (!pModuleHandler)
        return SC_500;

    if (pModuleHandler->on_clean_up)
    {
        int ret = pModuleHandler->on_clean_up(pSession);
        if (D_ENABLED(DL_MEDIUM))
        {
            pHandler = pSession->getReq()->getHttpHandler();
            LOG_D((pSession->getLogger(),
                   "[%s] [%s] _handler->on_clean_up() return %d",
                   pSession->getLogId(),
                   MODULE_NAME(((const LsiModule *)pHandler)->getModule()),
                   ret));
        }
        return ret;
    }
    return 0;
}


int ModuleHandler::onWrite(HttpSession *pSession)
{
    const HttpHandler *pHandler;
    const lsi_handler_t *pModuleHandler = pSession->getModHandler();

    if (!pModuleHandler)
        return SC_500;

    if (pModuleHandler->on_write_resp)
    {
        int status = pModuleHandler->on_write_resp(pSession);
        if (D_ENABLED(DL_MEDIUM))
        {
            pHandler = pSession->getReq()->getHttpHandler();
            LOG_D((pSession->getLogger(),
                   "[%s] [%s] _handler->on_write_resp() return %d",
                   pSession->getLogId(),
                   MODULE_NAME(((const LsiModule *)pHandler)->getModule()),
                   status));
        }
        if (status != LSI_WRITE_RESP_CONTINUE)
            pSession->endResponse(1);
        return (status == LSI_WRITE_RESP_CONTINUE);
    }
    else
    {
        if (D_ENABLED(DL_MEDIUM))
        {
            pHandler = pSession->getReq()->getHttpHandler();
            LOG_D((pSession->getLogger(),
                   "[%s] [%s] _handler->on_write_resp() is not available,"
                   " suspend WRITE event", pSession->getLogId(),
                   MODULE_NAME(((const LsiModule *)pHandler)->getModule())
                  ));
        }
        pSession->setFlag(HSF_HANDLER_WRITE_SUSPENDED);
        return LSI_WRITE_RESP_CONTINUE;
    }
}


int ModuleHandler::process(HttpSession *pSession,
                           const HttpHandler *pHandler)
{
    const lsi_handler_t *pModuleHandler = getHandler(pHandler);

    if (!pModuleHandler)
        return SC_500;

    if (!pModuleHandler->begin_process)
    {
        LOG_ERR((pSession->getLogger(),
                 "[%s] Internal Server Error, Module %s missing"
                 " begin_process() callback function, cannot be used"
                 " as a handler",
                 pSession->getLogger(), MODULE_NAME(((const LsiModule *)
                         pHandler)->getModule())));
        return SC_500;
    }

//     pSession->resetResp();
//    pSession->setupRespCache();

    pSession->setModHandler(pModuleHandler);

    int ret = pModuleHandler->begin_process(pSession);
    if (D_ENABLED(DL_MEDIUM))
    {
        LOG_D((pSession->getLogger(),
               "[%s] [%s] _handler->begin_process() return %d",
               pSession->getLogId(),
               MODULE_NAME(((const LsiModule *)pHandler)->getModule()),
               ret));
    }
    if (ret != 0)
    {
        if (ret < 0)
            return SC_500;
        else
            return HttpStatusCode::getInstance().codeToIndex(ret);
    }
    if (!pSession->getFlag(HSF_HANDLER_WRITE_SUSPENDED | HSF_HANDLER_DONE))
        pSession->continueWrite();
    return 0;
}


int ModuleHandler::onRead(HttpSession *pSession)
{
    const HttpHandler *pHandler;
    const lsi_handler_t *pModuleHandler = pSession->getModHandler();

    if (!pModuleHandler)
        return SC_500;

    if (pModuleHandler->on_read_req_body)
    {
        int ret = pModuleHandler->on_read_req_body(pSession);
        if (D_ENABLED(DL_MEDIUM))
        {
            pHandler = pSession->getReq()->getHttpHandler();
            LOG_D((pSession->getLogger(),
                   "[%s] [%s] _handler->on_write_resp() return %d",
                   pSession->getLogId(),
                   MODULE_NAME(((const LsiModule *)pHandler)->getModule()),
                   ret));
        }
        return ret;
    }
    else
        return 0;
}
