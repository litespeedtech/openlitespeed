<?php

namespace LSWebAdmin\UI;

class ConfRouteContext
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
        $this->_tabs = $tabs;
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
}
