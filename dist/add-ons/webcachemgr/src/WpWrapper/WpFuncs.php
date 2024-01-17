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
 * Wrapper class containing functions used to make WordPress core code specific
 * calls.
 *
 * @since 1.17
 */
class WpFuncs
{

    /**
     * https://developer.wordpress.org/reference/functions/switch_to_blog/
     *
     * @since 1.17
     *
     * @param int $id
     *
     * @return true
     */
    public static function switchToBlog( $id )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return switch_to_blog($id);
    }

    /**
     * https://developer.wordpress.org/reference/functions/restore_current_blog/
     *
     * @since 1.17
     *
     * @return bool
     */
    public static function restoreCurrentBlog()
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return restore_current_blog();
    }

    /**
     * https://developer.wordpress.org/reference/functions/get_option/
     *
     * @since 1.17
     *
     * @param string $option
     * @param mixed  $defaultValue
     *
     * @return mixed
     */
    public static function getOption( $option, $defaultValue = false )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return get_option($option, $defaultValue);
    }

    /**
     * https://developer.wordpress.org/reference/classes/wpdb/get_var/
     *
     * @since 1.17
     *
     * @param string|null $query
     * @param int         $x
     * @param int         $y
     *
     * @return string|null
     */
    public static function getVar( $query = null, $x = 0, $y = 0 )
    {
        global $wpdb;

        return $wpdb->get_var($query, $x, $y);
    }

    /**
     * https://developer.wordpress.org/reference/functions/is_plugin_active/
     *
     * @since 1.17
     *
     * @param string $plugin
     *
     * @return bool
     */
    public static function isPluginActive( $plugin )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return is_plugin_active($plugin);
    }

    /**
     * https://developer.wordpress.org/reference/functions/is_plugin_active_for_network/
     *
     * @since 1.17
     *
     * @param string $plugin
     *
     * @return bool
     */
    public static function isPluginActiveForNetwork( $plugin )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return is_plugin_active_for_network($plugin);
    }

    /**
     * https://developer.wordpress.org/reference/functions/wp_filesystem/
     *
     * @since 1.17
     *
     * @param array|false  $args
     * @param string|false $context
     * @param bool         $allow_relaxed_file_ownership
     *
     * @return bool|null
     */
    public static function WpFilesystem(
        $args = false,
        $context = false,
        $allow_relaxed_file_ownership = false )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return WP_Filesystem($args, $context, $allow_relaxed_file_ownership);
    }

    /**
     * https://developer.wordpress.org/reference/functions/unzip_file/
     *
     * @since 1.17
     *
     * @param string $file
     * @param string $to
     *
     * @return true|WP_Error
     *
     * @noinspection PhpUndefinedClassInspection
     *
     */
    public static function unzipFile( $file, $to )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return unzip_file($file, $to);
    }

    /**
     * https://developer.wordpress.org/reference/functions/deactivate_plugins/
     *
     * @since 1.17
     *
     * @param string|string[] $plugins
     * @param bool            $silent
     * @param bool|null       $network_wide
     */
    public static function deactivatePlugins(
        $plugins,
        $silent = false,
        $network_wide = null )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        deactivate_plugins($plugins, $silent, $network_wide);
    }

    /**
     * https://developer.wordpress.org/reference/functions/delete_plugins/
     *
     * @since 1.17
     *
     * @param string[] $plugins
     *
     * @return bool|null|WP_Error
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public static function deletePlugins( array $plugins )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return delete_plugins($plugins);
    }

    /**
     * https://developer.wordpress.org/reference/functions/activate_plugin/
     *
     * @since 1.17
     *
     * @param string $plugin
     * @param string $redirect
     * @param bool   $network_wide
     * @param bool   $silent
     *
     * @return null|WP_Error
     *
     * @noinspection PhpUndefinedClassInspection
     */
    public static function activatePlugin(
        $plugin,
        $redirect = '',
        $network_wide = false,
        $silent = false )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return activate_plugin($plugin, $redirect, $network_wide, $silent);
    }

    /**
     * https://developer.wordpress.org/reference/functions/add_filter/
     *
     * @since 1.17
     *
     * @param string   $hook_name
     * @param callable $callback
     * @param int      $priority
     * @param int      $accepted_args
     *
     * @return true
     */
    public static function addFilter(
                 $hook_name,
        callable $callback,
                 $priority = 10,
                 $accepted_args = 1 )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return add_filter($hook_name, $callback, $priority, $accepted_args);
    }

    /**
     * https://developer.wordpress.org/reference/functions/remove_filter/
     *
     * @since 1.17
     *
     * @param string                $hook_name
     * @param callable|string|array $callback
     * @param int                   $priority
     *
     * @return bool
     */
    public static function removeFilter( $hook_name, $callback, $priority = 10 )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return remove_filter($hook_name, $callback, $priority);
    }

    /**
     * https://developer.wordpress.org/reference/functions/is_wp_error/
     *
     * @since 1.17
     *
     * @param mixed $thing
     *
     * @return bool
     */
    public static function isWpError( $thing )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return is_wp_error($thing);
    }

    /**
     * https://developer.wordpress.org/reference/functions/wp_clean_plugins_cache/
     *
     * @since 1.17
     *
     * @param bool $clear_update_cache
     */
    public static function wpCleanPluginsCache( $clear_update_cache = true )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        wp_clean_plugins_cache($clear_update_cache);
    }

    /**
     * https://developer.wordpress.org/reference/functions/get_plugin_data/
     *
     * @since 1.17
     *
     * @param string $plugin_file
     * @param bool   $markup
     * @param bool   $translate
     *
     * @return array
     */
    public static function getPluginData(
        $plugin_file,
        $markup = true,
        $translate = true )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return get_plugin_data($plugin_file, $markup, $translate);
    }

    /**
     * https://developer.wordpress.org/reference/functions/get_locale/
     *
     * @since 1.17
     *
     * @return string
     */
    public static function getLocale()
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return get_locale();
    }

    /**
     * https://developer.wordpress.org/reference/functions/apply_filters/
     *
     * @since 1.17
     *
     * @param string $hook_name
     * @param mixed  $value
     * @param mixed  $args
     *
     * @return mixed
     */
    public static function applyFilters( $hook_name, $value, $args )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        return apply_filters($hook_name, $value, $args);
    }

    /**
     * https://developer.wordpress.org/reference/functions/wp_plugin_directory_constants/
     *
     * @since 1.17
     */
    public static function wpPluginDirectoryConstants()
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        wp_plugin_directory_constants();
    }

    /**
     * https://developer.wordpress.org/reference/functions/wp_cookie_constants/
     *
     * @since 1.17
     */
    public static function wpCookieConstants()
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        wp_cookie_constants();
    }
}