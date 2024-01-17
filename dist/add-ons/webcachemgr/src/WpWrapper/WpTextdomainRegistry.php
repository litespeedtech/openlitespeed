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
 * https://developer.wordpress.org/reference/classes/wp_textdomain_registry/
 *
 * @since 1.17
 */
class WpTextdomainRegistry
{

    /**
     * @since 1.17
     * @var \WP_Textdomain_Registry
     *
     * @noinspection PhpUndefinedClassInspection
     */
    private $wpTextdomainRegistry;

    /**
     *
     * @since 1.17
     */
    public function __construct()
    {
        /** @noinspection PhpUndefinedClassInspection */
        $this->wpTextdomainRegistry = new \WP_Textdomain_Registry();
    }

    /**
     *
     * @since 1.17
     *
     * @return \WP_Textdomain_Registry
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public function getWpWpTextdomainRegistryObject()
    {
        return $this->wpTextdomainRegistry;
    }
}