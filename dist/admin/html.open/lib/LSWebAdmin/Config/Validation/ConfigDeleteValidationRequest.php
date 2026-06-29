<?php

namespace LSWebAdmin\Config\Validation;

class ConfigDeleteValidationRequest
{
    private $_actionRequest;
    private $_target;
    private $_root;

    public function __construct($actionRequest, $target, $root)
    {
        $this->_actionRequest = $actionRequest;
        $this->_target = $target;
        $this->_root = $root;
    }

    public function GetActionRequest()
    {
        return $this->_actionRequest;
    }

    public function GetTarget()
    {
        return $this->_target;
    }

    public function GetRootNode()
    {
        return $this->_root;
    }
}
