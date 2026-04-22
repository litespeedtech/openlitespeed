<?php

namespace LSWebAdmin\Config\Load;

use LSWebAdmin\UI\DInfo;

class ConfigLoadTarget
{
    private $_type;
    private $_name;
    private $_path;
    private $_id;

    public function __construct($type, $name = null, $path = null, $id = null)
    {
        $this->_type = $type;
        $this->_name = $name;
        $this->_path = $path;
        $this->_id = $id;
    }

    public static function currentData($type, $name)
    {
        return new self($type, $name);
    }

    public static function specialData($path, $id)
    {
        return new self(DInfo::CT_EX, null, $path, $id);
    }

    public function GetType()
    {
        return $this->_type;
    }

    public function GetName()
    {
        return $this->_name;
    }

    public function GetPath()
    {
        return $this->_path;
    }

    public function GetId()
    {
        return $this->_id;
    }
}
