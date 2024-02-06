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
 * https://developer.wordpress.org/reference/classes/wp_query/
 *
 * @since 1.17
 */
class WpQuery
{

    /**
     * @since 1.17
     * @var \WP_Query
     *
     * @noinspection PhpUndefinedClassInspection
     */
    private $wpQuery;

    /**
     * https://developer.wordpress.org/reference/classes/wp_query/__construct/
     *
     * @since 1.17
     *
     * @param string|array $query
     */
    public function __construct( $query = '' )
    {
        /** @noinspection PhpUndefinedClassInspection */
        $this->wpQuery = new \WP_Query($query);
    }

    /**
     *
     * @since 1.17
     *
     * @return \WP_Query
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public function getWpWpQueryObject()
    {
        return $this->wpQuery;
    }
}