<?php

namespace LSWebAdmin\Config\Service;

class ConfigMutationTarget
{
    private $_tid;
    private $_location;
    private $_ref;

    public function __construct($tid, $location, $ref)
    {
        $this->_tid = $tid;
        $this->_location = $location;
        $this->_ref = $ref;
    }

    public function GetTid()
    {
        return $this->_tid;
    }

    public function GetLocation()
    {
        return $this->_location;
    }

    public function GetRef()
    {
        return $this->_ref;
    }
}
