<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2021
 * @since 1.13.10
 * *******************************************
 */

/**
 * As of PHP 8.x, functions disabled using ini directive 'disable_functions'
 * are no longer included in PHP's functions table and must be redefined to
 * avoid fatal error "Call to undefined function" when called.
 *
 * This file is used to redefine internal PHP functions intentionally disabled
 * before loading the WordPress environment.
 */

if ( !function_exists('ini_set') ) {

    /**
     *
     * @since 1.13.10
     *
     * @param string $option
     * @param string $value
     *
     * @return false
     */
    function ini_set( $option, $value)
    {
        return false;
    }
}
