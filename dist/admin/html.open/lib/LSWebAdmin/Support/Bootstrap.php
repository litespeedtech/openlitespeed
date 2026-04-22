<?php

namespace LSWebAdmin\Support;

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\Config\CNode;
use LSWebAdmin\Config\Validation\CValidation;
use LSWebAdmin\Product\Current\ConfValidation;
use LSWebAdmin\Product\Current\DAttr;
use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Config\DKeywordAlias;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\DPageDef;
use LSWebAdmin\UI\DTbl;
use LSWebAdmin\Product\Current\DTblDef;
use LSWebAdmin\Product\Base\DTblMap;
use LSWebAdmin\Config\FileLine;
use LSWebAdmin\Config\Parser\LsXmlParser;
use LSWebAdmin\Util\PathTool;
use LSWebAdmin\Config\Parser\PlainConfParser;
use LSWebAdmin\Config\Parser\RawFiles;
use LSWebAdmin\Runtime\SInfo;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\UI\UIBase;

class Bootstrap
{
    private static $autoloadRegistered = false;
    private static $libRoot;

    private static $legacyClassMap = [
        'CAuthorizer' => 'LSWebAdmin\\Auth\\CAuthorizer',
        'CData' => 'LSWebAdmin\\Product\\Current\\ConfigData',
        'CNode' => 'LSWebAdmin\\Config\\CNode',
        'CValidation' => 'LSWebAdmin\\Config\\CValidation',
        'ControllerBase' => 'LSWebAdmin\\Controller\\ControllerBase',
        'DAttr' => 'LSWebAdmin\\Product\\Current\\DAttr',
        'DAttrBase' => 'LSWebAdmin\\UI\\DAttrBase',
        'DAttrHelp' => 'LSWebAdmin\\UI\\DAttrHelp',
        'ConfValidation' => 'LSWebAdmin\\Product\\Current\\ConfValidation',
        'DInfo' => 'LSWebAdmin\\UI\\DInfo',
        'DKeywordAlias' => 'LSWebAdmin\\Config\\DKeywordAlias',
        'DMsg' => 'LSWebAdmin\\I18n\\DMsg',
        'DPage' => 'LSWebAdmin\\UI\\DPage',
        'DPageDef' => 'LSWebAdmin\\Product\\Current\\DPageDef',
        'DTbl' => 'LSWebAdmin\\UI\\DTbl',
        'DTblDef' => 'LSWebAdmin\\Product\\Current\\DTblDef',
        'DTblDefBase' => 'LSWebAdmin\\Product\\Base\\DTblDefBase',
        'DTblMap' => 'LSWebAdmin\\Product\\Base\\DTblMap',
        'FileLine' => 'LSWebAdmin\\Config\\FileLine',
        'LogFilter' => 'LSWebAdmin\\Log\\LogFilter',
        'LogViewer' => 'LSWebAdmin\\Log\\LogViewer',
        'LsXmlParser' => 'LSWebAdmin\\Config\\Parser\\LsXmlParser',
        'PathTool' => 'LSWebAdmin\\Util\\PathTool',
        'PlainConfParser' => 'LSWebAdmin\\Config\\Parser\\PlainConfParser',
        'Product' => 'LSWebAdmin\\Product\\Current\\Product',
        'RequestProbe' => 'LSWebAdmin\\Product\\Current\\RequestProbe',
        'RawFiles' => 'LSWebAdmin\\Config\\Parser\\RawFiles',
        'RealTimeStats' => 'LSWebAdmin\\Product\\Current\\RealTimeStats',
        'Service' => 'LSWebAdmin\\Product\\Current\\Service',
        'SInfo' => 'LSWebAdmin\\Runtime\\SInfo',
        'UI' => 'LSWebAdmin\\Product\\Current\\UI',
        'UIBase' => 'LSWebAdmin\\UI\\UIBase',
        'UIProperty' => 'LSWebAdmin\\UI\\UIProperty',
    ];

    public static function registerDefaultAutoload()
    {
        if (self::$autoloadRegistered) {
            return;
        }

        self::initLibRoot();
        spl_autoload_register([__CLASS__, 'autoload']);
        self::$autoloadRegistered = true;
    }

    public static function autoload($class)
    {
        $class = ltrim((string) $class, '\\');

        if ($class === '') {
            return;
        }

        if (isset(self::$legacyClassMap[$class])) {
            $targetClass = self::$legacyClassMap[$class];

            if (class_exists($targetClass, true) && !class_exists($class, false)) {
                class_alias($targetClass, $class);
            }

            return;
        }

        $currentPrefix = 'LSWebAdmin\\Product\\Current\\';
        if (strpos($class, $currentPrefix) === 0) {
            $shortName = substr($class, strlen($currentPrefix));
            require_once dirname(__DIR__) . '/Product/Current/Resolver.php';
            if (\LSWebAdmin\Product\Current\Resolver::canResolve($shortName)) {
                $targetClass = \LSWebAdmin\Product\Current\Resolver::resolve($shortName);

                if (class_exists($targetClass, true) && !class_exists($class, false)) {
                    class_alias($targetClass, $class);
                }

                return;
            }
        }

        $resolvedPath = self::resolveClassFile($class);

        if ($resolvedPath !== false) {
            require_once $resolvedPath;
        }
    }

    private static function initLibRoot()
    {
        if (self::$libRoot !== null) {
            return;
        }

        self::$libRoot = dirname(dirname(__DIR__));
    }

    private static function resolveClassFile($class)
    {
        self::initLibRoot();

        $relativePath = str_replace('\\', '/', $class) . '.php';
        if (strpos($class, 'LSWebAdmin\\') === 0) {
            $legacyRelativePath = 'OLSWebAdmin/' . substr($relativePath, strlen('LSWebAdmin/'));
            $classPath = self::$libRoot . '/' . $legacyRelativePath;
            if (is_file($classPath)) {
                return $classPath;
            }
        }

        return stream_resolve_include_path($relativePath);
    }
}
