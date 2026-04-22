<?php

namespace LSWebAdmin\Config\Service;

class ConfigMutationResult
{
    private $_target;
    private $_newRef;
    private $_deleted;

    public function __construct($target, $newRef, $deleted = false)
    {
        $this->_target = $target;
        $this->_newRef = $newRef;
        $this->_deleted = $deleted;
    }

    public function GetTarget()
    {
        return $this->_target;
    }

    public function GetTid()
    {
        return $this->_target->GetTid();
    }

    public function GetLocation()
    {
        return $this->_target->GetLocation();
    }

    public function GetRef()
    {
        return $this->_target->GetRef();
    }

    public function GetNewRef()
    {
        return $this->_newRef;
    }

    public function IsDelete()
    {
        return $this->_deleted;
    }
}
