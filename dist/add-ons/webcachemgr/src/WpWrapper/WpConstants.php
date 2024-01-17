<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2023 LiteSpeed Technologies, Inc.
 * @since     1.17
 * *******************************************
 */

namespace Lsc\Wp\WpWrapper;

/**
 * Wrapper class used to retrieve the value of WordPress core code specific
 * global constants.
 *
 * @since 1.17
 */
class WpConstants
{

    /**
     *
     * @since 1.17
     *
     * @param string $constantName
     *
     * @return mixed
     */
    public static function getWpConstant( $constantName )
    {
        return constant($constantName);
    }
}