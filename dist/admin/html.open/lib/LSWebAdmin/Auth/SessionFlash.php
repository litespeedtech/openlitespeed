<?php

namespace LSWebAdmin\Auth;

class SessionFlash
{
    const SESSION_KEY = 'lswebadmin_flash';

    public static function put($key, $value)
    {
        $key = (string) $key;
        if ($key === '') {
            return false;
        }

        if (!isset($_SESSION) || !is_array($_SESSION)) {
            $_SESSION = array();
        }

        if (!isset($_SESSION[self::SESSION_KEY]) || !is_array($_SESSION[self::SESSION_KEY])) {
            $_SESSION[self::SESSION_KEY] = array();
        }

        $_SESSION[self::SESSION_KEY][$key] = $value;
        return true;
    }

    public static function pull($key, $default = null)
    {
        $key = (string) $key;
        if ($key === ''
            || !isset($_SESSION)
            || !is_array($_SESSION)
            || !isset($_SESSION[self::SESSION_KEY])
            || !is_array($_SESSION[self::SESSION_KEY])
            || !array_key_exists($key, $_SESSION[self::SESSION_KEY])) {
            return $default;
        }

        $value = $_SESSION[self::SESSION_KEY][$key];
        unset($_SESSION[self::SESSION_KEY][$key]);

        if (empty($_SESSION[self::SESSION_KEY])) {
            unset($_SESSION[self::SESSION_KEY]);
        }

        return $value;
    }
}
