<?php

namespace LSWebAdmin\Product\Current;

class Resolver
{
    private static $variant = 'ows';
    private static $segment = 'Ows';
    private static $resolvableClasses = array(
        'ConfigData' => true,
        'ConfValidation' => true,
        'DAttr' => true,
        'DPageDef' => true,
        'DTblDef' => true,
        'Product' => true,
        'RealTimeStats' => true,
        'Service' => true,
        'UI' => true,
    );

    public static function canResolve($className)
    {
        return isset(self::$resolvableClasses[ltrim($className, '\\')]);
    }

    public static function resolve($className)
    {
        $className = ltrim($className, '\\');
        if (!self::canResolve($className)) {
            trigger_error('Unsupported product class alias ' . $className . ' for product ' . self::$variant, E_USER_ERROR);
        }

        $class = 'LSWebAdmin\\Product\\' . self::$segment . '\\' . $className;
        if (!class_exists($class)) {
            trigger_error('Unable to load product class ' . $class, E_USER_ERROR);
        }

        return $class;
    }

    private static function detectVariant()
    {
        return self::$variant;
    }
}
