<?php

namespace LSWebAdmin\Tool\BuildPhp;

class BuildConfig
{
    const OPTION_VERSION = 1;
    const BUILD_DIR = 2;
    const LAST_CONF = 3;
    const DEFAULT_INSTALL_DIR = 4;
    const DEFAULT_PARAMS = 5;
    const PHP_VERSION = 10;
    const LSAPI_VERSION = 11;
    const SUHOSIN_VERSION = 12;
    const MEMCACHE_VERSION = 15;
    const MEMCACHED_VERSION = 17;
    const MEMCACHED7_VERSION = 18;
    const MEMCACHE7_VERSION = 19;
    const MEMCACHE8_VERSION = 20;

    public static function GetVersion($field)
    {
        switch ($field) {
            case self::PHP_VERSION:
                return [
                    '8.5.4',
                    '8.4.19',
                    '8.3.30',
                    '8.2.30',
                    '8.1.33',
                    '8.0.30',
                    '7.4.33',
                    '7.3.31',
                    '7.2.34',
                    '7.1.33',
                    '7.0.33',
                    '5.6.40',
                ];

            case self::LSAPI_VERSION:
                return '8.2';

            case self::SUHOSIN_VERSION:
                return '0.9.38';

            case self::MEMCACHE_VERSION:
                return '2.2.7';

            case self::MEMCACHE7_VERSION:
                return '4.0.5.2';

            case self::MEMCACHE8_VERSION:
                return '8.2';

            case self::MEMCACHED_VERSION:
                return '2.2.0';

            case self::MEMCACHED7_VERSION:
                return '3.3.0';

            default:
                trigger_error('BuildConfig: illegal field ' . $field, E_USER_ERROR);
        }
    }

    public static function Get($field)
    {
        switch ($field) {
            case self::OPTION_VERSION:
                return 5;

            case self::BUILD_DIR:
                return SERVER_ROOT . 'phpbuild';

            case self::LAST_CONF:
                return SERVER_ROOT . 'phpbuild/savedconfig.';

            case self::DEFAULT_INSTALL_DIR:
                return SERVER_ROOT . 'lsphp';

            case self::DEFAULT_PARAMS:
                return [
                    '8' => '--with-mysqli --with-zlib --enable-gd --enable-shmop --enable-sockets --enable-sysvsem --enable-sysvshm --enable-mbstring --with-iconv --with-pdo-mysql --enable-ftp --with-zip --with-curl --enable-soap --enable-xml --with-openssl --enable-bcmath',
                    '7' => '--with-mysqli --with-zlib --with-gd --enable-shmop --enable-sockets --enable-sysvsem --enable-sysvshm --enable-mbstring --with-iconv --with-mcrypt --with-pdo-mysql --enable-ftp --enable-zip --with-curl --enable-soap --enable-xml --enable-json --with-openssl --enable-bcmath',
                    '5' => '--with-mysqli --with-zlib --with-gd --enable-shmop --enable-sockets --enable-sysvsem --enable-sysvshm --enable-mbstring --with-iconv --with-mysql --with-mcrypt --with-pdo --with-pdo-mysql --enable-ftp --enable-zip --with-curl --enable-soap --enable-xml --enable-json  --with-openssl --enable-bcmath',
                ];

            default:
                trigger_error('BuildConfig: illegal field ' . $field, E_USER_ERROR);
        }
    }

    public static function GetTemplateDir()
    {
        return __DIR__ . '/templates';
    }

    public static function GetVersionFilePath()
    {
        return SERVER_ROOT . 'admin/html/lib/LSWebAdmin/Tool/BuildPhp/BuildConfig.php';
    }
}
