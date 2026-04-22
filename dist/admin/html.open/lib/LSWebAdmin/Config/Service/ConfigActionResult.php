<?php

namespace LSWebAdmin\Config\Service;

class ConfigActionResult
{
    private $_displayData;
    private $_hasDisplayData;
    private $_reloadConfig;
    private $_markChanged;
    private $_forceReLogin;

    public function __construct($displayData = null, $hasDisplayData = false, $reloadConfig = false, $markChanged = false, $forceReLogin = false)
    {
        $this->_displayData = $displayData;
        $this->_hasDisplayData = $hasDisplayData;
        $this->_reloadConfig = $reloadConfig;
        $this->_markChanged = $markChanged;
        $this->_forceReLogin = $forceReLogin;
    }

    public function HasDisplayData()
    {
        return $this->_hasDisplayData;
    }

    public function GetDisplayData()
    {
        return $this->_displayData;
    }

    public function ResolveDisplayData($defaultData)
    {
        if ($this->_hasDisplayData) {
            return $this->_displayData;
        }

        return $defaultData;
    }

    public function ShouldReloadConfig()
    {
        return $this->_reloadConfig;
    }

    public function ShouldMarkChanged()
    {
        return $this->_markChanged;
    }

    public function ShouldForceReLogin()
    {
        return $this->_forceReLogin;
    }
}
