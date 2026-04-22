<?php

namespace LSWebAdmin\UI;

class RequestInput
{
    private $_sources;
    private $_useGlobals;

    public function __construct($sources = [], $useGlobals = false)
    {
        $this->_sources = [];
        $this->_useGlobals = $useGlobals;

        foreach ($sources as $origin => $values) {
            $this->_sources[strtoupper($origin)] = is_array($values) ? $values : [];
        }
    }

    public static function fromGlobals()
    {
        return new self([], true);
    }

    public function HasInput($origin, $name)
    {
        if ($name == '' || $origin == '') {
            return false;
        }

        $origin = strtoupper($origin);
        if ($origin == 'ANY') {
            $origin = 'REQUEST';
        }

        $source = $this->getSource($origin);
        return isset($source[$name]);
    }

    public function GrabInput($origin, $name, $type = '')
    {
        if ($name == '' || $origin == '') {
            return null;
        }

        $origin = strtoupper($origin);
        if ($origin == 'ANY') {
            $origin = 'REQUEST';
        }

        $source = $this->getSource($origin);
        $value = array_key_exists($name, $source) ? $source[$name] : null;

        switch ($type) {
            case 'int':
                return (int) $value;
            case 'float':
                return (float) str_replace(',', '', $value);
            case 'string':
                return $this->stringInput($value);
            case 'array':
                return is_array($value) ? $value : null;
            case 'object':
                return is_object($value) ? $value : null;
            default:
                return $this->stringInput($value);
        }
    }

    public function GrabGoodInput($origin, $name, $type = '')
    {
        $val = $this->GrabInput($origin, $name, $type);
        if ($val != null && strpos($val, '<') !== false) {
            $val = null;
        }

        return $val;
    }

    public function GrabGoodInputWithReset($origin, $name, $type = '')
    {
        $val = $this->GrabInput($origin, $name, $type);
        $needReset = (($val != null) && (strpos($val, '<') !== false && strpos($val, '?<') === false));

        if ($needReset) {
            $this->resetValue($origin, $name);
        }

        return $needReset ? null : $val;
    }

    private function stringInput($value)
    {
        if ($value === null) {
            return '';
        }

        if (is_scalar($value)) {
            return trim((string) $value);
        }

        return '';
    }

    private function getSource($origin)
    {
        switch ($origin) {
            case 'REQUEST':
            case 'GET':
            case 'POST':
            case 'COOKIE':
            case 'FILE':
            case 'SERVER':
                break;
            default:
                trigger_error('RequestInput: unsupported input origin ' . $origin, E_USER_ERROR);
        }

        if ($this->_useGlobals) {
            switch ($origin) {
                case 'REQUEST':
                    return $_REQUEST;
                case 'GET':
                    return $_GET;
                case 'POST':
                    return $_POST;
                case 'COOKIE':
                    return $_COOKIE;
                case 'FILE':
                    return $_FILES;
                case 'SERVER':
                    return $_SERVER;
            }
        }

        return isset($this->_sources[$origin]) ? $this->_sources[$origin] : [];
    }

    private function resetValue($origin, $name)
    {
        $origin = strtoupper($origin);
        if ($origin == 'ANY') {
            $origin = 'REQUEST';
        }

        if ($this->_useGlobals) {
            switch ($origin) {
                case 'REQUEST':
                    $_REQUEST[$name] = null;
                    return;
                case 'GET':
                    $_GET[$name] = null;
                    return;
                case 'POST':
                    $_POST[$name] = null;
                    return;
                case 'COOKIE':
                    $_COOKIE[$name] = null;
                    return;
                case 'FILE':
                    $_FILES[$name] = null;
                    return;
                case 'SERVER':
                    $_SERVER[$name] = null;
                    return;
            }
        }

        if (!isset($this->_sources[$origin])) {
            $this->_sources[$origin] = [];
        }
        $this->_sources[$origin][$name] = null;
    }
}
