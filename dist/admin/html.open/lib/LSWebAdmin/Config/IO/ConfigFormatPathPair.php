<?php

namespace LSWebAdmin\Config\IO;

class ConfigFormatPathPair
{
    private $_confPath;
    private $_xmlPath;

    public function __construct($confPath, $xmlPath)
    {
        $this->_confPath = $confPath;
        $this->_xmlPath = $xmlPath;
    }

    public static function fromPath($path)
    {
        if (self::endsWith($path, '.xml')) {
            return new self(substr($path, 0, -4) . '.conf', $path);
        }

        if (self::endsWith($path, '.conf')) {
            return new self($path, substr($path, 0, -5) . '.xml');
        }

        return new self($path . '.conf', $path . '.xml');
    }

    public function GetConfPath()
    {
        return $this->_confPath;
    }

    public function GetXmlPath()
    {
        return $this->_xmlPath;
    }

    private static function endsWith($value, $suffix)
    {
        $suffixLength = strlen($suffix);
        if ($suffixLength === 0) {
            return true;
        }

        if (!is_string($value) || strlen($value) < $suffixLength) {
            return false;
        }

        return substr_compare($value, $suffix, -$suffixLength, $suffixLength) === 0;
    }
}
