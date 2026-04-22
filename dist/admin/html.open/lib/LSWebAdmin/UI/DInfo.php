<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\Product\Current\DPageDef;

class DInfo
{

    const FLD_ConfType = 1;
    const FLD_ConfErr = 2;
    const FLD_View = 3;
    const FLD_ViewName = 4;
    const FLD_TopMsg = 5;
    const FLD_ICONTITLE = 6;
    const FLD_CtrlUrl = 10;
    const FLD_MID = 11;
    const FLD_PID = 12;
    const FLD_TID = 13;
    const FLD_REF = 14;
    const FLD_ACT = 15;
    const FLD_TOKEN = 16;
    const FLD_PgData = 17;
    const FLD_ConfData = 18;
    const FLD_ServData = 19;
    // conftype
    const CT_SERV = 'serv';
    const CT_VH = 'vh_';
    const CT_TP = 'tp_';
    const CT_ADMIN = 'admin';
    const CT_EX = 'special';

    private $_routeState;
    private $_ctrlUrl = 'index.php?view=confMgr&';
    private $_confErr;
    private $_pageData;
    private $_confData;
    private $_servData; // for populate vh level derived options
    private $_topMsg;
    private $_isPrintingLinkedTbl;

    private static $_instance;

	public static function singleton()
	{
		if (!isset(self::$_instance)) {
			self::$_instance = new self();
		}

		return self::$_instance;
	}

    private function __construct()
    {
        $this->_routeState = ConfRouteState::emptyState();
    }

    public function ShowDebugInfo()
    {
        return sprintf(
            "DINFO: conftype=%s view=%s viewname=%s mid=%s pid=%s tid=%s ref=%s act=%s\n",
            $this->_routeState->GetConfType(),
            $this->_routeState->GetView(),
            $this->_routeState->GetViewName(),
            $this->_routeState->GetMid(),
            $this->_routeState->GetPid(),
            $this->_routeState->GetTid(),
            $this->_routeState->GetRef(),
            $this->_routeState->GetAct()
        );
    }

    public function InitConf()
    {
        try {
            $route = ConfRouteParser::parse(
                UIBase::GetInputSource(),
                DPageDef::class,
                isset($_SESSION['token']) ? $_SESSION['token'] : null
            );
        } catch (\RuntimeException $ex) {
            trigger_error('DInfo: route parse failed – ' . $ex->getMessage(), E_USER_ERROR);
        }

        $this->applyRouteContext($route);
	}

    public function ResetConfigState()
    {
        $this->_routeState = ConfRouteState::emptyState();
        $this->_confErr = null;
        $this->_pageData = null;
        $this->_confData = null;
        $this->_servData = null;
        $this->_topMsg = null;
        $this->_isPrintingLinkedTbl = false;
    }

    private function applyRouteContext($route)
    {
        $this->_routeState = ConfRouteState::fromContext($route);
    }

    public function GetRouteState()
    {
        return $this->_routeState;
    }

    public function GetServData()
    {
        return $this->_servData;
    }

    public function GetCtrlUrl()
    {
        return sprintf('%sm=%s&p=%s', $this->_ctrlUrl, urlencode($this->_routeState->GetMid()), $this->_routeState->GetPid());
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

    public function GetFirstTid()
    {
        return $this->_routeState->GetFirstTid();
    }

    public function GetFirstRef()
    {
        return $this->_routeState->GetFirstRef();
    }

    public function SetConfErr($confErr)
    {
        $this->_confErr = $confErr;
    }

    public function SetAct($act)
    {
        $this->_routeState->SetAct($act);
    }

    public function SetPageData($pageData)
    {
        $this->_pageData = $pageData;
    }

    public function SetConfData($confData)
    {
        $this->_confData = $confData;
    }

    public function SetServData($servData)
    {
        $this->_servData = $servData;
    }

    public function SetRef($ref)
    {
        $this->_routeState->SetRef($ref);
    }

    public function SetViewName($viewName)
    {
        $this->_routeState->SetViewName($viewName);
    }

    public function AddTopMsg($message)
    {
        if (!is_array($this->_topMsg)) {
            $this->_topMsg = [];
        }

        $this->_topMsg[] = $message;
    }

    public function Get($field)
    {
        switch ($field) {
            case self::FLD_CtrlUrl:
				return $this->GetCtrlUrl();
            case self::FLD_View: return $this->GetView();
            case self::FLD_ViewName: return $this->GetViewName();
            case self::FLD_TopMsg: return $this->GetTopMsg();
            case self::FLD_ConfType: return $this->GetConfType();
            case self::FLD_ConfErr: return $this->GetConfErr();
            case self::FLD_MID: return $this->GetMid();
            case self::FLD_PID: return $this->GetPid();
            case self::FLD_TID: return $this->GetTid();
            case self::FLD_REF: return $this->GetRef();
            case self::FLD_ACT: return $this->GetAct();
            case self::FLD_PgData: return $this->GetPageData();
            case self::FLD_ConfData: return $this->GetConfData();
            case self::FLD_TOKEN: return $this->GetToken();
            case self::FLD_ICONTITLE:
                return $this->GetIconTitle();
            default: error_log("invalid DInfo field : $field\n");
        }
    }

    public function Set($field, $value)
    {
        switch ($field) {
            case self::FLD_ConfErr:
                $this->SetConfErr($value);
                break;
            case self::FLD_ACT:
                $this->SetAct($value);
                break;
            case self::FLD_PgData:
                $this->SetPageData($value);
                break;
            case self::FLD_ConfData:
                $this->SetConfData($value);
                break;
            case self::FLD_ServData:
                $this->SetServData($value);
                break;
            case self::FLD_REF:
                $this->SetRef($value);
                break;
            case self::FLD_ViewName:
                $this->SetViewName($value);
                break;
            case self::FLD_TopMsg:
                $this->AddTopMsg($value);
                break;
            default: trigger_error("DInfo::Set: unsupported field $field", E_USER_ERROR);
        }
    }

    public function SetPrintingLinked($printinglinked)
    {
        $this->_isPrintingLinkedTbl = $printinglinked;
    }

    public function InitUIProps($props)
    {
        ConfUiContext::fromDisplay($this)->InitUIProps($props);
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
            $this->_isPrintingLinkedTbl,
            $editTid,
            $editRef,
            $addTid
        );
    }

    public function TrimLastId()
    {
        $this->_routeState->TrimLastId();
    }

    public function GetLast($field)
    {
        if ($field == self::FLD_TID) {
            return $this->GetLastTid();
		}
        if ($field == self::FLD_REF) {
            return $this->GetLastRef();
		}

        return null;
    }

    public function GetFirst($field)
    {
        if ($field == self::FLD_TID) {
            return $this->GetFirstTid();
		}
        if ($field == self::FLD_REF) {
            return $this->GetFirstRef();
		}

        return null;
    }

    public function GetParentRef()
    {
        return $this->_routeState->GetParentRef();
    }

    public function SwitchToSubTid($extracted)
    {
        $this->_routeState->SwitchToSubTid($extracted, \LSWebAdmin\Product\Current\DTblDef::class);
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

    public function GetVHRoot()
    {
        return ConfDerivedOptionsResolver::resolveVHRoot($this->_routeState, $this->_servData);
    }

}
