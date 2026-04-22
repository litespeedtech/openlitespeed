<?php

namespace LSWebAdmin\Config\Load;

use LSWebAdmin\UI\DInfo;

class ConfigLoadRequest
{
    private $_view;
    private $_pid;
    private $_tid;
    private $_confType;
    private $_viewName;
    private $_firstRef;
    private $_vhRoot;

    public function __construct($view, $pid, $tid, $confType, $viewName, $firstRef, $vhRoot)
    {
        $this->_view = $view;
        $this->_pid = $pid;
        $this->_tid = $tid;
        $this->_confType = $confType;
        $this->_viewName = $viewName;
        $this->_firstRef = $firstRef;
        $this->_vhRoot = $vhRoot;
    }

    public static function fromRouteState($routeState, $vhRoot)
    {
        return new self(
            $routeState->GetView(),
            $routeState->GetPid(),
            $routeState->GetTid(),
            $routeState->GetConfType(),
            $routeState->GetViewName(),
            $routeState->GetFirstRef(),
            $vhRoot
        );
    }

    public function NeedsCurrentData()
    {
        return (($this->_view == DInfo::CT_VH && $this->_pid != 'base')
            || ($this->_view == DInfo::CT_TP && $this->_pid != 'mbr'));
    }

    public function GetCurrentDataType()
    {
        return $this->NeedsCurrentData() ? $this->_view : null;
    }

    public function GetCurrentDataName()
    {
        return $this->NeedsCurrentData() ? $this->_viewName : null;
    }

    public function IsMimeDataRequested()
    {
        return ($this->_view == DInfo::CT_SERV
            && is_string($this->_tid)
            && strpos($this->_tid, 'S_MIME') !== false);
    }

    public function IsAdminUsersDataRequested()
    {
        return ($this->_view == DInfo::CT_ADMIN && $this->_pid == 'usr');
    }

    public function GetRealmDataId()
    {
        if ($this->_view != DInfo::CT_VH || !is_string($this->_tid)) {
            return null;
        }

        if (strpos($this->_tid, 'V_UDB') !== false) {
            return 'V_UDB';
        }

        if (strpos($this->_tid, 'V_GDB') !== false) {
            return 'V_GDB';
        }

        return null;
    }

    public function GetViewName()
    {
        return $this->_viewName;
    }

    public function GetFirstRef()
    {
        return $this->_firstRef;
    }

    public function GetVHRoot()
    {
        return $this->_vhRoot;
    }
}
