/*****************************************************************************
*    Open LiteSpeed is an open source HTTP server.                           *
*    Copyright (C) 2013 - 2018  LiteSpeed Technologies, Inc.                 *
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
#include <http/httpsession.h>
#include <http/httpstatuscode.h>
#include <http/mtsessdata.h>
#include <log4cxx/logger.h>
#include <lsiapi/internal.h>
#include <lsiapi/modulemanager.h>
#include <lsr/ls_lfqueue.h>
#include <thread/workcrew.h>


static WorkCrew *s_pGlobal = NULL;


void *MtHandlerProcess(ls_lfnodei_t *item)
{
    HttpSession *pSession = (HttpSession *)item;
    pSession->lockMtRace();
    LS_DBG_M(pSession->getLogSession(),
            "[T%d] lock MtRace.", ls_thr_seq());

    const lsi_reqhdlr_t *pModuleHandler = pSession->getModHandler();
    if (!pSession->isMtHandlerCancelled())
    {
        assert(pSession->getMtFlag(HSF_MT_HANDLER));
        pModuleHandler->begin_process(pSession);
    }
    LS_DBG_M(pSession->getLogSession(),
             "MtHandlerProcess: MT handler finished.");

    //if (!pSession->getMtFlag(HSF_MT_RECYCLE | HSF_MT_CANCEL | HSF_MT_END_RESP))
    //    pSession->setMtFlag(HSF_MT_END_RESP);
    pSession->unlockMtRace();
    LS_DBG_M(pSession->getLogSession(),
            "[T%d] unlock MtRace.", ls_thr_seq());

    evtcb_pf cb = (evtcb_pf)(&HttpSession::mtNotifyCallback);
    int sn = pSession->getSn();
    pSession->setMtFlag(HSF_MT_NOTIFIED);
    g_api->create_event(cb, pSession, sn, (void *)HSF_MT_END, true);
    return NULL;
}


void ModuleHandler::initGlobalWorkCrew()
{
    if (s_pGlobal)
        return;

    s_pGlobal = new WorkCrew(100, MtHandlerProcess, NULL, NULL, 5, 10);
    s_pGlobal->blockSig(SIGCHLD);
}

WorkCrew *ModuleHandler::getGlobalWorkCrew()
{
    if (!s_pGlobal)
        initGlobalWorkCrew();

    return s_pGlobal;
};



ModuleHandler::ModuleHandler()
{
    ReqHandler::setType(HandlerType::HT_MODULE);
}


ModuleHandler::~ModuleHandler()
{
}


static const lsi_reqhdlr_t *getHandler(const HttpHandler *pHandler)
{

    const lsi_reqhdlr_t *pModuleHandler = NULL;
    if (pHandler)
        pModuleHandler = ((const LsiModule *)pHandler)->getModule()->reqhandler;

    if (pHandler && !pModuleHandler)
    {
        LS_ERROR("Internal Server Error, Module %s missing _handler definition,"
                 " cannot be used as a handler",
                 MODULE_NAME(((const LsiModule *)pHandler)->getModule()));
    }

    return pModuleHandler;
}


int ModuleHandler::cleanUp(HttpSession *pSession)
{
    const HttpHandler *pHandler;
    const lsi_reqhdlr_t *pModuleHandler = pSession->getModHandler();

    if (!pModuleHandler)
        return SC_500;

    if (pModuleHandler->ts_hdlr_ctx)
        return mt_cleanUp(pSession, pModuleHandler);

    if (pModuleHandler->on_clean_up)
    {
        int ret = pModuleHandler->on_clean_up(pSession);
        pHandler = pSession->getReq()->getHttpHandler();
        LS_DBG_M(pSession->getLogSession(),
                 "[%s] _handler->on_clean_up() return %d",
                 MODULE_NAME(((const LsiModule *)pHandler)->getModule()), ret);
        return ret;
    }
    return 0;
}


int ModuleHandler::onWrite(HttpSession *pSession)
{
    const HttpHandler *pHandler;
    const lsi_reqhdlr_t *pModuleHandler = pSession->getModHandler();

    if (!pModuleHandler)
        return SC_500;

    if (pModuleHandler->ts_hdlr_ctx)
        return mt_onWrite(pSession, pModuleHandler);

    pHandler = pSession->getReq()->getHttpHandler();
    if (pModuleHandler->on_write_resp)
    {
        int status = pModuleHandler->on_write_resp(pSession);
        LS_DBG_M(pSession->getLogSession(),
                 "[%s] _handler->on_write_resp() return %d",
                 MODULE_NAME(((const LsiModule *)pHandler)->getModule()),
                 status);
        if (status != LSI_RSP_MORE)
            pSession->endResponse(1);
        return (status == LSI_RSP_MORE);
    }
    else
    {
        LS_DBG_M(pSession->getLogSession(),
                 "[%s] _handler->on_write_resp() is not available,"
                 " suspend WRITE event",
                 MODULE_NAME(((const LsiModule *)pHandler)->getModule())
                );
        pSession->setFlag(HSF_HANDLER_WRITE_SUSPENDED);
        return LSI_RSP_MORE;
    }
}


int ModuleHandler::onRead(HttpSession *pSession)
{
    const HttpHandler *pHandler;
    const lsi_reqhdlr_t *pModuleHandler = pSession->getModHandler();

    if (!pModuleHandler)
        return SC_500;
    if (pModuleHandler->ts_hdlr_ctx)
        return mt_onRead(pSession, pModuleHandler);

    if (pModuleHandler->on_read_req_body)
    {
        int ret = pModuleHandler->on_read_req_body(pSession);
        pHandler = pSession->getReq()->getHttpHandler();
        LS_DBG_M(pSession->getLogSession(),
                 "[%s] _handler->on_write_resp() return %d",
                 MODULE_NAME(((const LsiModule *)pHandler)->getModule()),
                 ret);
        return ret;
    }
    else
        return 0;
}


int ModuleHandler::process(HttpSession *pSession,
                           const HttpHandler *pHandler)
{
    const lsi_reqhdlr_t *pModuleHandler = getHandler(pHandler);

    if (!pModuleHandler)
        return SC_500;
    if (pModuleHandler->ts_hdlr_ctx)
        return mt_process(pSession, pModuleHandler);

    if (!pModuleHandler->begin_process)
    {
        LS_ERROR(pSession->getLogSession(),
                 "Internal Server Error, Module %s missing"
                 " begin_process() callback function, cannot be used"
                 " as a handler", MODULE_NAME(((const LsiModule *)
                                               pHandler)->getModule()));
        return SC_500;
    }

//     pSession->resetResp();
//    pSession->setupRespCache();

    pSession->setModHandler(pModuleHandler);

    int ret = pModuleHandler->begin_process(pSession);
    LS_DBG_M(pSession->getLogSession(),
             "[%s] _handler->begin_process() return %d",
             MODULE_NAME(((const LsiModule *)pHandler)->getModule()),
             ret);
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


int ModuleHandler::mt_cleanUp(HttpSession *pSession, const lsi_reqhdlr_t *pModuleHandler)
{
    const HttpHandler *pHandler;

    if (pModuleHandler->on_clean_up)
    {
        int ret = pModuleHandler->on_clean_up(pSession);
        pHandler = pSession->getReq()->getHttpHandler();
        LS_DBG_M(pSession->getLogSession(),
                 "[%s] _handler->on_clean_up() return %d",
                 MODULE_NAME(((const LsiModule *)pHandler)->getModule()), ret);
        return ret;
    }
    return 0;
}


int ModuleHandler::mt_onWrite(HttpSession *pSession, const lsi_reqhdlr_t *pModuleHandler)
{
    LS_DBG_M(pSession->getLogSession(),
                "mt_onWrite: notify MT write to continue");
    if (pSession->getMtSessData()->getWriteNotifier()->notify() == 0)
    {
        LS_DBG_M(pSession->getLogSession(),
                 "mt_onWrite: module not blocked on write(),"
                 " suspend WRITE event");
        pSession->setFlag(HSF_HANDLER_WRITE_SUSPENDED);
    }
    return LSI_RSP_MORE;
}


int ModuleHandler::mt_onRead(HttpSession *pSession, const lsi_reqhdlr_t *pModuleHandler)
{
    MtNotifier *pReadNotifier;
    if (pSession->testMtFlag(HSF_MT_READING))  //MT handler blocking on read()
    {
        LS_DBG_M(pSession->getLogSession(),
                    "mt_onRead: notify MT read to continue");
        pReadNotifier = pSession->getMtSessData()->getReadNotifier();
        if (pReadNotifier->notify() == 0)
        {
            LS_DBG_M(pSession->getLogSession(),
                    "mt_onRead: module is not blocked on read()");
        }
    }
    return 0;
}


int ModuleHandler::mt_process(HttpSession *pSession,
                           const lsi_reqhdlr_t *pModuleHandler)
{
    const HttpHandler *pHandler;
    pHandler = pSession->getReq()->getHttpHandler();

    if (!pModuleHandler->ts_enqueue_req && !pModuleHandler->begin_process)
    {
        LS_ERROR(pSession->getLogSession(),
                 "Internal Server Error, MT Module %s missing"
                 " enqueue_req() and process_req() callback function, cannot be used"
                 " as a handler", MODULE_NAME(((const LsiModule *)
                                               pHandler)->getModule()));
        return SC_500;
    }

    if (pSession->beginMtHandler(pModuleHandler))
    {
            LS_ERROR(pSession->getLogSession(),
                    "[%s] Internal Server Error, cannot start MT handler. "
                    , MODULE_NAME(((const LsiModule *)pHandler)->getModule()));
        return SC_500;
    }

    //add to job queue with pSession,
    if (pModuleHandler->ts_enqueue_req)
    {
        LS_DBG_M(pSession->getLogSession(), "[Tm] unlock MtRace.");
        pSession->unlockMtRace();
        if (pModuleHandler->ts_enqueue_req(pModuleHandler->ts_hdlr_ctx,
                                        pSession) == LS_FAIL)
        {
            pSession->lockMtRace();
            LS_ERROR(pSession->getLogSession(),
                    "[Tm] lock MtRace, Internal Server Error, Module %s failed to queue"
                    " request ", MODULE_NAME(((const LsiModule *)
                                                pHandler)->getModule()));
            return SC_500;
        }
        return 0;
    }
    if (!pModuleHandler->begin_process)
    {
        LS_ERROR(pSession->getLogSession(),
                "Internal Server Error, Module %s has no handler defined"
                " request ", MODULE_NAME(((const LsiModule *)
                                            pHandler)->getModule()));
        return SC_500;

    }
    WorkCrew *pCrew;
    if (pModuleHandler->ts_hdlr_ctx != (void *)-1l)
        pCrew = (WorkCrew *)pModuleHandler->ts_hdlr_ctx;
    else
        pCrew = s_pGlobal;

    LS_DBG_M(pSession->getLogSession(), "[Tm] unlock MtRace.");
    pSession->unlockMtRace();
    if (pCrew->addJob(pSession) == LS_FAIL)
    {
        pSession->lockMtRace();
        LS_ERROR(pSession->getLogSession(),
                "[Tm] lock MtRace, Module %s, Internal Server Error, WorkCrew->addJob() failed",
                MODULE_NAME(((const LsiModule *)pHandler)->getModule()));
        return SC_500;
    };

    return 0;
}

