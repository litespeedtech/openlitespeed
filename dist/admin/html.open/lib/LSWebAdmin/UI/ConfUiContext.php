<?php

namespace LSWebAdmin\UI;

class ConfUiContext
{
    private $_routeState;
    private $_ctrlUrl;
    private $_confErr;
    private $_pageData;
    private $_confData;
    private $_servData;
    private $_topMsg;
    private $_isPrintingLinked;

    public function __construct(
        $routeState,
        $ctrlUrl,
        $confErr = null,
        $pageData = null,
        $confData = null,
        $servData = null,
        $topMsg = null,
        $isPrintingLinked = false
    ) {
        $this->_routeState = $routeState;
        $this->_ctrlUrl = $ctrlUrl;
        $this->_confErr = $confErr;
        $this->_pageData = $pageData;
        $this->_confData = $confData;
        $this->_servData = $servData;
        $this->_topMsg = $topMsg;
        $this->_isPrintingLinked = (bool) $isPrintingLinked;
    }

    public static function fromDisplay($display)
    {
        if ($display instanceof self) {
            return $display;
        }

        if (!is_object($display) || !method_exists($display, 'GetRouteState')) {
            return new self(ConfRouteState::emptyState(), '', null, null, null, null, null);
        }

        return new self(
            $display->GetRouteState(),
            $display->GetCtrlUrl(),
            $display->GetConfErr(),
            $display->GetPageData(),
            $display->GetConfData(),
            $display->GetServData(),
            $display->GetTopMsg()
        );
    }

    public function GetRouteState()
    {
        return $this->_routeState;
    }

    public function GetCtrlUrl()
    {
        return $this->_ctrlUrl;
    }

    public function GetView()
    {
        return $this->_routeState->GetView();
    }

    public function GetViewName()
    {
        return $this->_routeState->GetViewName();
    }

    public function GetTopMsg()
    {
        return $this->_topMsg;
    }

    public function GetConfType()
    {
        return $this->_routeState->GetConfType();
    }

    public function GetConfErr()
    {
        return $this->_confErr;
    }

    public function GetMid()
    {
        return $this->_routeState->GetMid();
    }

    public function GetPid()
    {
        return $this->_routeState->GetPid();
    }

    public function GetTid()
    {
        return $this->_routeState->GetTid();
    }

    public function GetRef()
    {
        return $this->_routeState->GetRef();
    }

    public function GetAct()
    {
        return $this->_routeState->GetAct();
    }

    public function GetPageData()
    {
        return $this->_pageData;
    }

    public function GetConfData()
    {
        return $this->_confData;
    }

    public function GetServData()
    {
        return $this->_servData;
    }

    public function GetToken()
    {
        return $this->_routeState->GetToken();
    }

    public function GetIconTitle()
    {
        return ConfIconTitleResolver::resolve($this->_routeState->GetView(), $this->_routeState->GetViewName());
    }

    public function GetLastTid()
    {
        return $this->_routeState->GetLastTid();
    }

    public function GetLastRef()
    {
        return $this->_routeState->GetLastRef();
    }

    public function InitUIProps($props)
    {
        ConfUiPropertyBuilder::apply($props, $this->_routeState, $this->_ctrlUrl, $this->_servData);
    }

    public function Get($field)
    {
        switch ($field) {
            case DInfo::FLD_CtrlUrl:
                return $this->GetCtrlUrl();
            case DInfo::FLD_View:
                return $this->GetView();
            case DInfo::FLD_ViewName:
                return $this->GetViewName();
            case DInfo::FLD_TopMsg:
                return $this->GetTopMsg();
            case DInfo::FLD_ConfType:
                return $this->GetConfType();
            case DInfo::FLD_ConfErr:
                return $this->GetConfErr();
            case DInfo::FLD_MID:
                return $this->GetMid();
            case DInfo::FLD_PID:
                return $this->GetPid();
            case DInfo::FLD_TID:
                return $this->GetTid();
            case DInfo::FLD_REF:
                return $this->GetRef();
            case DInfo::FLD_ACT:
                return $this->GetAct();
            case DInfo::FLD_PgData:
                return $this->GetPageData();
            case DInfo::FLD_ConfData:
                return $this->GetConfData();
            case DInfo::FLD_TOKEN:
                return $this->GetToken();
            case DInfo::FLD_ICONTITLE:
                return $this->GetIconTitle();
        }

        return null;
    }

    public function GetLast($field)
    {
        if ($field == DInfo::FLD_TID) {
            return $this->GetLastTid();
        }
        if ($field == DInfo::FLD_REF) {
            return $this->GetLastRef();
        }

        return null;
    }

    public function IsViewAction()
    {
        return $this->_routeState->IsViewAction();
    }

    public function GetActionData($actions, $editTid = '', $editRef = '', $addTid = '')
    {
        return ConfActionDataBuilder::build(
            $actions,
            $this->_routeState,
            $this->_ctrlUrl,
            $this->_isPrintingLinked,
            $editTid,
            $editRef,
            $addTid
        );
    }

    public function GetDerivedSelOptions($tid, $loc, $node)
    {
        return ConfDerivedOptionsResolver::resolve(
            $this->_routeState,
            $this->_servData,
            $this->_confData,
            $tid,
            $loc,
            $node
        );
    }

    public function SetPrintingLinked($printingLinked)
    {
        $this->_isPrintingLinked = (bool) $printingLinked;
    }
}
