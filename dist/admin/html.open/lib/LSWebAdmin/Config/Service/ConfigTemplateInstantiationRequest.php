<?php

namespace LSWebAdmin\Config\Service;

use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Config\IO\ConfigPathResolutionResult;
use LSWebAdmin\Product\Current\ConfigData;

class ConfigTemplateInstantiationRequest
{
    private $_serverData;
    private $_templateName;
    private $_virtualHostName;
    private $_templatePathResolver;
    private $_configDataClass;
    private $_routeState;

    public function __construct(
        $serverData,
        $templateName,
        $virtualHostName,
        $templatePathResolver = null,
        $configDataClass = null,
        $routeState = null
    ) {
        $this->_serverData = $serverData;
        $this->_templateName = $templateName;
        $this->_virtualHostName = $virtualHostName;
        $this->_templatePathResolver = $templatePathResolver;
        $this->_configDataClass = ($configDataClass != null) ? $configDataClass : ConfigData::class;
        $this->_routeState = $routeState;
    }

    public function GetServerData()
    {
        return $this->_serverData;
    }

    public function GetTemplateName()
    {
        return $this->_templateName;
    }

    public function GetVirtualHostName()
    {
        return $this->_virtualHostName;
    }

    public function ResolveTemplatePath()
    {
        if (!is_callable($this->_templatePathResolver)) {
            return ConfigPathResolutionResult::unresolved(DInfo::CT_TP);
        }

        return call_user_func($this->_templatePathResolver, $this->_templateName);
    }

    public function GetConfigDataClass()
    {
        return $this->_configDataClass;
    }

    public function GetRouteState()
    {
        return $this->_routeState;
    }
}
