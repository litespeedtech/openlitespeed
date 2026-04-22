<?php

namespace LSWebAdmin\Config;

class DKeywordAlias
{

    private static $instance = null;

    private $_aliasmap;
    private $_aliaskey;

    public static function GetInstance()
    {
        if (self::$instance == null) {
            self::$instance = new self();
        }

        return self::$instance;
    }

    public static function NormalizedKey($rawkey)
    {
        $tool = self::GetInstance();
        return $tool->get_normalized_key($rawkey);
    }

    public static function GetShortPrintKey($normalizedkey)
    {
        $tool = self::GetInstance();
        return $tool->get_short_print_key($normalizedkey);
    }

    private function __construct()
    {
        $this->define_alias();
        $this->_aliaskey = [];
        foreach ($this->_aliasmap as $nk => $sk) {
            $this->_aliaskey[strtolower($sk)] = $nk;
        }
    }

    private function get_normalized_key($rawkey)
    {
        $key = strtolower($rawkey);
        if (isset($this->_aliaskey[$key])) {
            return $this->_aliaskey[$key];
        }

        return $key;
    }

    private function get_short_print_key($normalizedkey)
    {
        if (isset($this->_aliasmap[$normalizedkey])) {
            return $this->_aliasmap[$normalizedkey];
        }

        return null;
    }

    private function define_alias()
    {
        $this->_aliasmap = [
            //'accesscontrol' => 'acc',
            //'address' => 'addr',
        ];
    }

}
