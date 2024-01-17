<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2023 LiteSpeed Technologies, Inc.
 * @since     1.7
 * *******************************************
 */

namespace Lsc\Wp\WpWrapper;

/**
 * https://developer.wordpress.org/reference/classes/wpdb/
 *
 * @since 1.17
 */
class Wpdb
{

    /**
     * https://developer.wordpress.org/reference/classes/wpdb/
     *
     * @since 1.17
     *
     * @global \wpdb $wpdb
     *
     * @return string
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public static function getBlogs()
    {
        global $wpdb;

        return $wpdb->blogs;
    }

    /**
     * https://developer.wordpress.org/reference/classes/wpdb/get_col/
     *
     * @since 1.17
     *
     * @global \wpdb $wpdb
     *
     * @param string|null $query
     * @param int         $x
     *
     * @return array
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public static function getCol( $query = null, $x = 0 )
    {
        global $wpdb;

        return $wpdb->get_col($query, $x);
    }
}