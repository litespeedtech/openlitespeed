<?php

namespace LSWebAdmin\Config\IO;

class ConfigPathResolutionResult
{
    private $_type;
    private $_name;
    private $_path;
    private $_error;

    public function __construct($type, $name = null, $path = null, $error = null)
    {
        $this->_type = $type;
        $this->_name = $name;
        $this->_path = $path;
        $this->_error = $error;
    }

    public static function resolved($type, $path, $name = null)
    {
        return new self($type, $name, $path, null);
    }

    public static function unresolved($type, $error = null, $name = null)
    {
        return new self($type, $name, null, $error);
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

    public function HasPath()
    {
        return (is_string($this->_path) && $this->_path !== '');
    }

    public function GetError()
    {
        return $this->_error;
    }

    public function HasError()
    {
        return (is_string($this->_error) && $this->_error !== '');
    }
}
