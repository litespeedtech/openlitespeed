<?php

namespace LSWebAdmin\Config\Service;

use LSWebAdmin\Config\IO\ConfigPathResolutionResult;

class ConfigBootstrapRequest
{
    private $_dataClass;
    private $_loadRequest;
    private $_loadAdmin;
    private $_allowCurrentData;
    private $_serverResolution;
    private $_adminResolution;
    private $_currentPathResolver;
    private $_routeState;

    public function __construct(
        $dataClass,
        $loadRequest,
        $loadAdmin,
        $allowCurrentData,
        $serverResolution,
        $adminResolution,
        $currentPathResolver = null,
        $routeState = null
    ) {
        $this->_dataClass = $dataClass;
        $this->_loadRequest = $loadRequest;
        $this->_loadAdmin = $loadAdmin;
        $this->_allowCurrentData = $allowCurrentData;
        $this->_serverResolution = $this->normalizeResolution('serv', null, $serverResolution);
        $this->_adminResolution = $this->normalizeResolution('admin', null, $adminResolution);
        $this->_currentPathResolver = $currentPathResolver;
        $this->_routeState = $routeState;
    }

    public function GetDataClass()
    {
        return $this->_dataClass;
    }

    public function GetLoadRequest()
    {
        return $this->_loadRequest;
    }

    public function ShouldLoadAdmin()
    {
        return $this->_loadAdmin;
    }

    public function AllowCurrentData()
    {
        return $this->_allowCurrentData;
    }

    public function GetServerResolution()
    {
        return $this->_serverResolution;
    }

    public function GetAdminResolution()
    {
        return $this->_adminResolution;
    }

    public function ResolveCurrentPathResult($type, $name, $serverData)
    {
        if (!is_callable($this->_currentPathResolver)) {
            return ConfigPathResolutionResult::unresolved($type, null, $name);
        }

        return $this->normalizeResolution(
            $type,
            $name,
            call_user_func($this->_currentPathResolver, $type, $name, $serverData)
        );
    }

    public function GetRouteState()
    {
        return $this->_routeState;
    }

    private function normalizeResolution($type, $name, $resolution)
    {
        if ($resolution instanceof ConfigPathResolutionResult) {
            return $resolution;
        }

        if (is_string($resolution) && $resolution !== '') {
            return ConfigPathResolutionResult::resolved($type, $resolution, $name);
        }

        return ConfigPathResolutionResult::unresolved($type, null, $name);
    }
}
