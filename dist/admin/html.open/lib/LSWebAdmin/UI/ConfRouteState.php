<?php

namespace LSWebAdmin\UI;

class ConfRouteState
{
    private $_confType;
    private $_view;
    private $_viewName;
    private $_mid;
    private $_pid;
    private $_tid;
    private $_ref;
    private $_act;
    private $_token;
    private $_tabs;

    public function __construct($confType, $view, $viewName, $mid, $pid, $tid, $ref, $act, $token, $tabs)
    {
        $this->_confType = $confType;
        $this->_view = $view;
        $this->_viewName = $viewName;
        $this->_mid = $mid;
        $this->_pid = $pid;
        $this->_tid = $tid;
        $this->_ref = $ref;
        $this->_act = $act;
        $this->_token = $token;
        $this->_tabs = is_array($tabs) ? $tabs : [];
    }

    public static function emptyState()
    {
        return new self(null, null, null, 'serv', null, null, null, null, null, []);
    }

    public static function fromContext($context)
    {
        return new self(
            $context->GetConfType(),
            $context->GetView(),
            $context->GetViewName(),
            $context->GetMid(),
            $context->GetPid(),
            $context->GetTid(),
            $context->GetRef(),
            $context->GetAct(),
            $context->GetToken(),
            $context->GetTabs()
        );
    }

    public function GetConfType()
    {
        return $this->_confType;
    }

    public function GetView()
    {
        return $this->_view;
    }

    public function GetViewName()
    {
        return $this->_viewName;
    }

    public function GetMid()
    {
        return $this->_mid;
    }

    public function GetPid()
    {
        return $this->_pid;
    }

    public function GetTid()
    {
        return $this->_tid;
    }

    public function GetRef()
    {
        return $this->_ref;
    }

    public function GetAct()
    {
        return $this->_act;
    }

    public function GetToken()
    {
        return $this->_token;
    }

    public function GetTabs()
    {
        return $this->_tabs;
    }

    public function SetAct($act)
    {
        $this->_act = $act;
    }

    public function SetRef($ref)
    {
        $this->_ref = $ref;
    }

    public function SetViewName($viewName)
    {
        $this->_viewName = $viewName;

        if (!is_string($this->_mid) || ($pos = strpos($this->_mid, '_')) <= 0) {
            return;
        }

        if ($viewName == null) {
            $suffix = '';
        } else {
            $suffix = '_' . $viewName;
        }

        $this->_mid = substr($this->_mid, 0, $pos) . $suffix;
    }

    public function IsViewAction()
    {
        if (!is_string($this->_act) || $this->_act === '') {
            return true;
        }

        return (strpos('EaScn', $this->_act) === false);
    }

    public function GetLastTid()
    {
        return $this->getLastSegment($this->_tid);
    }

    public function GetFirstTid()
    {
        return $this->getFirstSegment($this->_tid);
    }

    public function GetLastRef()
    {
        return $this->getLastSegment($this->_ref);
    }

    public function GetFirstRef()
    {
        return $this->getFirstSegment($this->_ref);
    }

    public function GetParentRef()
    {
        if ($this->_ref && ($pos = strrpos($this->_ref, '`')) !== false) {
            return substr($this->_ref, 0, $pos);
        }

        return '';
    }

    public function TrimLastId()
    {
        if ($this->_tid && ($pos = strrpos($this->_tid, '`')) !== false) {
            $this->_tid = substr($this->_tid, 0, $pos);
        } else {
            $this->_tid = null;
        }

        if ($this->_ref && ($pos = strrpos($this->_ref, '`')) !== false) {
            $this->_ref = substr($this->_ref, 0, $pos);
        } elseif ($this->_view == 'sl_' || $this->_view == 'al_' || $this->_pid == 'base' || $this->_pid == 'mbr') {
            $this->_ref = $this->_viewName;
        } else {
            $this->_ref = null;
        }
    }

    public function SwitchToSubTid($extracted, $tableDefClass)
    {
        if ($this->_tid && ($pos = strrpos($this->_tid, '`')) !== false) {
            $tid0 = substr($this->_tid, 0, $pos + 1);
            $tid = substr($this->_tid, $pos + 1);
        } else {
            $tid0 = '';
            $tid = $this->_tid;
        }

        $tbl = call_user_func([$tableDefClass, 'GetInstance'])->GetTblDef($tid);
        $subtbls = $tbl->Get(DTbl::FLD_SUBTBLS);
        $newkey = $extracted->GetChildVal($subtbls[0]);
        $subtid = '';
        if ($newkey != null) {
            if ($newkey == '0' || !isset($subtbls[$newkey])) {
                $subtid = $subtbls[1];
            } else {
                $subtid = $subtbls[$newkey];
            }
        }

        $this->_tid = $tid0 . $subtid;
    }

    private function getLastSegment($id)
    {
        if (is_string($id) && ($pos = strrpos($id, '`')) !== false) {
            if (strlen($id) > $pos + 1) {
                return substr($id, $pos + 1);
            }

            return '';
        }

        return $id;
    }

    private function getFirstSegment($id)
    {
        if (is_string($id) && ($pos = strpos($id, '`')) !== false) {
            return substr($id, 0, $pos);
        }

        return $id;
    }
}
