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
 * https://developer.wordpress.org/reference/classes/plugin_upgrader/
 *
 * @since 1.17
 */
class PluginUpgrader
{

    /**
     * https://developer.wordpress.org/reference/classes/plugin_upgrader/
     *
     * @since 1.17
     * @var \Plugin_Upgrader
     *
     * @noinspection PhpUndefinedClassInspection
     */
    private $pluginUpgrader;

    /**
     * https://developer.wordpress.org/reference/classes/wp_upgrader/__construct/
     *
     * @since 1.17
     *
     * @param \WP_Upgrader_Skin|null $skin
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public function __construct( \WP_Upgrader_Skin $skin = null )
    {
        /** @noinspection PhpUndefinedClassInspection */
        $this->pluginUpgrader = new \Plugin_Upgrader($skin);
    }

    /**
     *
     * @since 1.17
     *
     * @return \Plugin_Upgrader
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public function getWpPluginUpgraderObject()
    {
        return $this->pluginUpgrader;
    }

    /**
     *
     * @since 1.17
     *
     * @return array|WP_Error
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public function getResult()
    {
        return $this->pluginUpgrader->result;
    }

    /**
     * https://developer.wordpress.org/reference/classes/wp_upgrader/init/
     *
     * @since 1.17
     */
    public function init()
    {
        $this->pluginUpgrader->init();
    }

    /**
     * https://developer.wordpress.org/reference/classes/plugin_upgrader/upgrade_strings/
     *
     * @since 1.17
     */
    public function upgradeStrings()
    {
        $this->pluginUpgrader->upgrade_strings();
    }

    /**
     * https://developer.wordpress.org/reference/classes/wp_upgrader/run/
     *
     * @since 1.17
     *
     * @param array $options
     *
     * @return array|false|WP_Error
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public function run( array $options )
    {
        return $this->pluginUpgrader->run($options);
    }
}