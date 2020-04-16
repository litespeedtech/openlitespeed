<?php

/** *********************************************
 * LiteSpeed Web Server WordPress Dash Notifier
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2019
 * *******************************************
 */

namespace Lsc\Wp;

use \Lsc\Wp\Context\Context;

class DashNotifier
{

    const BYPASS_FLAG = '.dash_notifier_bypass';
    const DASH_MD5 = 'dash_md5';
    const DASH_PLUGIN = 'dash-notifier/dash-notifier.php';
    const DEFAULT_PLUGIN_PATH = '/wp-content/plugins/dash-notifier';

    /**
     * @deprecated 1.9
     */
    const DOWNLOAD_DIR = '/usr/src/litespeed-wp-plugin';

    const PLUGIN_NAME = 'dash-notifier';
    const VER_FILE = 'dash_ver';

    private function __construct()
    {

    }

    /**
     *
     * @since 1.9
     *
     * @return void
     * @throws LSCMException  Indirectly thrown by self::getLatestVersion(),
     *                        self::downloadVersion(), and Logger::error().
     */
    public static function prepLocalDashPluginFiles()
    {
        $dashVerFile = Context::LOCAL_PLUGIN_DIR . '/' . self::VER_FILE;

        if ( ! file_exists($dashVerFile) ) {
            $latestVer = self::getLatestVersion();
            self::downloadVersion($latestVer);
            return;
        }

        $localVer = trim(file_get_contents($dashVerFile));

        $pluginDir = Context::LOCAL_PLUGIN_DIR . '/' . self::PLUGIN_NAME;

        if ( ! file_exists($pluginDir) ) {
            self::downloadVersion($localVer);
            return;
        }

        $md5File = Context::LOCAL_PLUGIN_DIR . '/' . self::DASH_MD5;
        $md5StoredVal = file_get_contents($md5File);
        $md5Val = Util::DirectoryMd5($pluginDir);

        if ( $md5StoredVal != $md5Val ) {
            self::downloadVersion($localVer);
            return;
        }

        clearstatcache();

        if ( (time() - filemtime($dashVerFile)) > 86400 ) {

            try {
                $latestVer = self::getLatestVersion();

                if ( version_compare($latestVer, $localVer, '<') ) {
                    self::downloadVersion($latestVer);
                }
            }
            catch ( LSCMException $e ) {
                Logger::error($e->getMessage());
            }

            touch($dashVerFile);
        }
    }

    /**
     *
     * @since 1.9
     *
     * @return string
     * @throws LSCMException
     */
    public static function getLatestVersion()
    {
        $latestVer = '';
        $latestVerUrl = 'https://www.litespeedtech.com/packages/lswpcache/dash_latest';

        $content = Util::get_url_contents($latestVerUrl);

        if ( empty($content) ) {
            throw new LSCMException('Could not retrieve latest Dash Notifier plugin version');
        }

        $latestVer = trim($content);

        return $latestVer;
    }

    /**
     *
     * @since 1.9
     *
     * @param string  $version
     * @throws LSCMException  Indirectly thrown by self::wgetPlugin().
     */
    protected static function downloadVersion( $version )
    {
        $pluginDir = Context::LOCAL_PLUGIN_DIR . '/' . self::PLUGIN_NAME;

        if ( file_exists($pluginDir) ) {
            exec("/bin/rm -rf {$pluginDir}");
        }

        self::wgetPlugin($version, true);
    }

    /**
     *
     * @since 1.9
     *
     * @param string   $version
     * @param boolean  $saveMD5
     * @throws LSCMException  Indirectly thrown by Logger::info() and
     *                        Util::unzipFile().
     */
    protected static function wgetPlugin( $version, $saveMD5 = false )
    {
        Logger::info("Downloading Dash Notifier v{$version}...");

        $pluginDir = Context::LOCAL_PLUGIN_DIR . '/' . self::PLUGIN_NAME;
        $zipFileName = self::PLUGIN_NAME . ".{$version}.zip";
        $localZipFile = Context::LOCAL_PLUGIN_DIR . "/{$zipFileName}";

        $url = "https://downloads.wordpress.org/plugin/{$zipFileName}";
        $wget_command = "wget -q --tries=1 --no-check-certificate {$url} -P "
                . Context::LOCAL_PLUGIN_DIR;

        exec($wget_command, $output, $return_var);

        if ( $return_var !== 0 ) {
            throw new LSCMException("Failed to download Dash Notifier v{$version} with wget "
                    . "exit status {$return_var}.", LSCMException::E_NON_FATAL);
        }

        $extracted = Util::unzipFile($localZipFile, Context::LOCAL_PLUGIN_DIR);
        unlink($localZipFile);

        if ( ! $extracted ) {
            throw new LSCMException("Unable to unzip {$localZipFile}",
                    LSCMException::E_NON_FATAL);
        }

        $testfile = "{$pluginDir}/" . self::PLUGIN_NAME . '.php';

        if ( ! file_exists($testfile) ) {
            throw new LSCMException("Unable to download Dash Notifier v{$version}.",
                    LSCMException::E_NON_FATAL);
        }

        if ( $saveMD5 ) {
            $md5Val = Util::DirectoryMd5($pluginDir);
            file_put_contents(Context::LOCAL_PLUGIN_DIR . '/'
                    . self::DASH_MD5, $md5Val);
        }

        file_put_contents(Context::LOCAL_PLUGIN_DIR . '/' . self::VER_FILE,
                $version);
    }

    /**
     * Check if WordPress installation should be notified using the Dash
     * Notifier plugin.
     *
     * @param string  $wpPath  WordPress installation root directory.
     * @return boolean
     */
    public static function canNotify( $wpPath )
    {
        if ( file_exists("{$wpPath}/" . self::BYPASS_FLAG) ) {
            return false;
        }

        return true;
    }

    /**
     * Checks the current installation for existing LSCWP plugin files and
     * copies them to the installation's plugins directory if not found.
     * This function should only be run as the user.
     *
     * @return boolean            True when new Dash Notifier plugin files are
     *                             used.
     * @throws LSCMException
     */
    public static function prepareUserInstall()
    {
        $pluginDir = WP_PLUGIN_DIR;

        $dashNotifierPlugin = "{$pluginDir}/dash-notifier/dash-notifier.php";

        if ( file_exists($dashNotifierPlugin) ) {
            /**
             * Existing installation detected.
             */
            return false;
        }

        $pluginSrc = Context::LOCAL_PLUGIN_DIR . '/' . self::PLUGIN_NAME;
        exec("/bin/cp -rf {$pluginSrc} {$pluginDir}");

        if ( !file_exists($dashNotifierPlugin) ) {
            throw new LSCMException("Failed to copy Dash Notifier plugin files to "
            . "{$pluginDir}.", LSCMException::E_NON_FATAL);
        }

        Logger::debug("Copied Dash Notifier plugin files into plugins directory {$pluginDir}");

        return true;
    }

    /**
     * Activate Dash Notifier plugin if it is not already activated, and give
     * the plugin any notification data in the form of a JSON encoded array.
     *
     * @param string  $jsonInfo  Dash Notifier plugin info.
     * @return boolean
     * @throws LSCMException
     */
    public static function doNotify( $jsonInfo )
    {

        if ( file_exists(WP_PLUGIN_DIR . '/' . self::DASH_PLUGIN) ) {

            /**
             * Used to pass info to the Dash Notifier Plugin.
             */
            define( 'DASH_NOTIFIER_MSG', $jsonInfo);

            if ( !is_plugin_active(self::DASH_PLUGIN) ) {

                /**
                * Should not check directly, can error on success due to object
                * cache.
                */
               activate_plugin(self::DASH_PLUGIN, '', false, false);

               if ( !is_plugin_active(self::DASH_PLUGIN) ) {
                   return false;
               }
            }
            else {
                include WP_PLUGIN_DIR . '/' . self::DASH_PLUGIN;
            }
        }
        else {
            throw new LSCMException('Dash Notifier plugin files are missing. Cannot notify.',
                    LSCMException::E_NON_FATAL);
        }

        return true;
    }

    /**
     * WP Functions: deactivate_plugins(), delete_plugins()
     *
     * @param boolean  $uninstall
     */
    public static function deactivate( $uninstall )
    {
        deactivate_plugins(self::DASH_PLUGIN);

        if ( $uninstall ) {
            //add some msg about having removed plugin files?
            delete_plugins(array( self::DASH_PLUGIN ));
        }
    }

}
