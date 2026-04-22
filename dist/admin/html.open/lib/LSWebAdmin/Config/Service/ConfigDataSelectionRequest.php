<?php

namespace LSWebAdmin\Config\Service;

use LSWebAdmin\UI\DInfo;

class ConfigDataSelectionRequest
{
    private $_view;
    private $_pid;
    private $_tid;
    private $_confType;

    public function __construct($view, $pid, $tid, $confType)
    {
        $this->_view = $view;
        $this->_pid = $pid;
        $this->_tid = $tid;
        $this->_confType = $confType;
    }

    public static function fromRouteState($routeState)
    {
        return new self(
            $routeState->GetView(),
            $routeState->GetPid(),
            $routeState->GetTid(),
            $routeState->GetConfType()
        );
    }

    public function UsesSpecialData()
    {
        return (($this->_view == DInfo::CT_SERV && is_string($this->_tid) && strpos($this->_tid, 'S_MIME') !== false)
            || ($this->_view == DInfo::CT_ADMIN && $this->_pid == 'usr')
            || ($this->_view == DInfo::CT_VH && is_string($this->_tid)
                && (strpos($this->_tid, 'V_UDB') !== false || strpos($this->_tid, 'V_GDB') !== false)));
    }

    public function UsesCurrentData()
    {
        return (($this->_view == DInfo::CT_VH && $this->_pid != 'base')
            || ($this->_view == DInfo::CT_TP && $this->_pid != 'mbr'));
    }

    public function IsAdminConf()
    {
        return ($this->_confType == DInfo::CT_ADMIN);
    }
}
