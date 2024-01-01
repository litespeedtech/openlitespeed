<?php

/** *********************************************
 * LiteSpeed Web Server WordPress Dash Notifier
 *
 * @author    Michael Alegre
 * @copyright 2019-2023 LiteSpeed Technologies, Inc.
 * *******************************************
 */

namespace Lsc\Wp;

use Lsc\Wp\Context\Context;
use Lsc\Wp\WpWrapper\WpConstants;
use Lsc\Wp\WpWrapper\WpFuncs;

class DashNotifier
{

    /**
     * @var string
     */
    const BYPASS_FLAG = '.dash_notifier_bypass';

    /**
     * @var string
     */
    const DASH_MD5 = 'dash_md5';

    /**
     * @var string
     */
    const DASH_PLUGIN = 'dash-notifier/dash-notifier.php';

    /**
     * @var string
     */
    const DEFAULT_PLUGIN_PATH = '/wp-content/plugins/dash-notifier';

    /**
     * @deprecated 1.9
     *
     * @var string
     */
    const DOWNLOAD_DIR = '/usr/src/litespeed-wp-plugin';

    /**
     * @var string
     */
    const PLUGIN_NAME = 'dash-notifier';

    /**
     * @var string
     */
    const VER_FILE = 'dash_ver';

    private function __construct()
    {

    }

    /**
     *
     * @since 1.9
     *
     * @return void
     *
     * @throws LSCMException  Thrown when read dash version file command fails.
     * @throws LSCMException  Thrown indirectly by self::getLatestVersion()
     *     call.
     * @throws LSCMException  Thrown indirectly by self::downloadVersion() call.
     * @throws LSCMException  Thrown indirectly by self::downloadVersion() call.
     * @throws LSCMException  Thrown indirectly by self::downloadVersion() call.
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     */
    public static function prepLocalDashPluginFiles()
    {
        $dashVerFile = Context::LOCAL_PLUGIN_DIR . '/' . self::VER_FILE;

        if ( ! file_exists($dashVerFile) ) {
            self::downloadVersion(self::getLatestVersion());
            return;
        }

        if ( ($content = file_get_contents($dashVerFile)) === false ) {
            throw new LSCMException(
                'prepLocalDashPluginFiles(): Failed to read file '
                    . self::VER_FILE . ' contents.'
            );
        }

        $localVer = trim($content);

        $pluginDir = Context::LOCAL_PLUGIN_DIR . '/' . self::PLUGIN_NAME;

        if ( ! file_exists($pluginDir) ) {
            self::downloadVersion($localVer);
            return;
        }

        $isStoredMd5Valid = (
            file_get_contents(Context::LOCAL_PLUGIN_DIR . '/' . self::DASH_MD5)
            ==
            Util::DirectoryMd5($pluginDir)
        );

        if ( !$isStoredMd5Valid ) {
            self::downloadVersion($localVer);
            return;
        }

        clearstatcache();

        if ( (time() - filemtime($dashVerFile)) > 86400 ) {

            try {
                $latestVer = self::getLatestVersion();

                if ( Util::betterVersionCompare($latestVer, $localVer, '<') ) {
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
     *
     * @throws LSCMException  Thrown when unable to retrieve latest Dash
     *     Notifier plugin version.
     */
    public static function getLatestVersion()
    {
        $content = Util::get_url_contents(
            'https://www.litespeedtech.com/packages/lswpcache/dash_latest'
        );

        if ( empty($content) ) {
            throw new LSCMException(
                'Could not retrieve latest Dash Notifier plugin version'
            );
        }

        return trim($content);
    }

    /**
     *
     * @since 1.9
     *
     * @param string $version
     *
     * @throws LSCMException  Thrown indirectly by self::wgetPlugin() call.
     */
    protected static function downloadVersion( $version )
    {
        $pluginDir = Context::LOCAL_PLUGIN_DIR . '/' . self::PLUGIN_NAME;

        if ( file_exists($pluginDir) ) {
            exec("/bin/rm -rf $pluginDir");
        }

        self::wgetPlugin($version, true);
    }

    /**
     *
     * @since 1.9
     *
     * @param string $version
     * @param bool   $saveMD5
     *
     * @throws LSCMException  Thrown when wget command fails to download
     *     requested Dash Notifier plugin version.
     * @throws LSCMException  Thrown when unable to unzip downloaded Dash
     *     Notifier plugin package.
     * @throws LSCMException  Thrown when downloaded Dash Notifier plugin
     *     package is missing an expected file.
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     * @throws LSCMException  Thrown indirectly by Util::unzipFile() call.
     */
    protected static function wgetPlugin( $version, $saveMD5 = false )
    {
        Logger::info("Downloading Dash Notifier v$version...");

        $zipFileName  = self::PLUGIN_NAME . ".$version.zip";

        exec(
            "wget -q --tries=1 --no-check-certificate "
                . "https://downloads.wordpress.org/plugin/$zipFileName -P "
                . Context::LOCAL_PLUGIN_DIR,
            $output,
            $return_var
        );

        if ( $return_var !== 0 ) {
            throw new LSCMException(
                "Failed to download Dash Notifier v$version with wget exit "
                    . "status $return_var.",
                LSCMException::E_NON_FATAL
            );
        }

        $localZipFile = Context::LOCAL_PLUGIN_DIR . "/$zipFileName";

        $extracted = Util::unzipFile($localZipFile, Context::LOCAL_PLUGIN_DIR);
        unlink($localZipFile);

        if ( ! $extracted ) {
            throw new LSCMException(
                "Unable to unzip $localZipFile",
                LSCMException::E_NON_FATAL
            );
        }

        $pluginDir    = Context::LOCAL_PLUGIN_DIR . '/' . self::PLUGIN_NAME;

        if ( ! file_exists("$pluginDir/" . self::PLUGIN_NAME . '.php') ) {
            throw new LSCMException(
                "Unable to download Dash Notifier v$version.",
                LSCMException::E_NON_FATAL
            );
        }

        if ( $saveMD5 ) {
            file_put_contents(
                Context::LOCAL_PLUGIN_DIR . '/' . self::DASH_MD5,
                Util::DirectoryMd5($pluginDir)
            );
        }

        file_put_contents(
            Context::LOCAL_PLUGIN_DIR . '/' . self::VER_FILE,
            $version
        );
    }

    /**
     * Check if WordPress installation should be notified using the Dash
     * Notifier plugin.
     *
     * @param string $wpPath  Root directory for WordPress installation.
     *
     * @return bool
     */
    public static function canNotify( $wpPath )
    {
        return !file_exists("$wpPath/" . self::BYPASS_FLAG);
    }

    /**
     * Checks the current installation for existing LSCWP plugin files and
     * copies them to the installation's plugins directory if not found.
     * This function should only be run as the user.
     *
     * @return bool  True when new Dash Notifier plugin files are used.
     *
     * @throws LSCMException  Thrown when unable to copy Dash Notifier plugin
     *     files to WordPress plugin directory.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    public static function prepareUserInstall()
    {
        $pluginDir = WpConstants::getWpConstant('WP_PLUGIN_DIR');

        $dashNotifierPlugin = "$pluginDir/dash-notifier/dash-notifier.php";

        if ( file_exists($dashNotifierPlugin) ) {
            /**
             * Existing installation detected.
             */
            return false;
        }

        exec(
            '/bin/cp -rf ' . Context::LOCAL_PLUGIN_DIR . '/' . self::PLUGIN_NAME
            . " $pluginDir"
        );

        if ( !file_exists($dashNotifierPlugin) ) {
            throw new LSCMException(
                "Failed to copy Dash Notifier plugin files to $pluginDir.",
                LSCMException::E_NON_FATAL
            );
        }

        Logger::debug(
            'Copied Dash Notifier plugin files into plugins directory '
                . $pluginDir
        );

        return true;
    }

    /**
     * Activate Dash Notifier plugin if it is not already activated, and give
     * the plugin any notification data in the form of a JSON encoded array.
     *
     * @param string $jsonInfo  Dash Notifier plugin info.
     *
     * @return bool
     *
     * @throws LSCMException  Thrown when unable to find Dash Notifier plugin
     *     files.
     */
    public static function doNotify( $jsonInfo )
    {
        if (
            file_exists(
                WpConstants::getWpConstant('WP_PLUGIN_DIR')
                    . '/' . self::DASH_PLUGIN
            )
        ) {
            /**
             * Used to pass info to the Dash Notifier Plugin.
             */
            Util::define_wrapper( 'DASH_NOTIFIER_MSG', $jsonInfo);

            if ( !WpFuncs::isPluginActive(self::DASH_PLUGIN) ) {

                /**
                * Should not check directly, can error on success due to object
                * cache.
                */
               WpFuncs::activatePlugin(self::DASH_PLUGIN);

               if ( !WpFuncs::isPluginActive(self::DASH_PLUGIN) ) {
                   return false;
               }
            }
            else {
                include WpConstants::getWpConstant('WP_PLUGIN_DIR')
                    . '/' . self::DASH_PLUGIN;
            }
        }
        else {
            throw new LSCMException(
                'Dash Notifier plugin files are missing. Cannot notify.',
                LSCMException::E_NON_FATAL
            );
        }

        return true;
    }

    /**
     *
     * @param bool $uninstall
     */
    public static function deactivate( $uninstall )
    {
        WpFuncs::deactivatePlugins(self::DASH_PLUGIN);

        if ( $uninstall ) {
            //add some msg about having removed plugin files?
            WpFuncs::deletePlugins(array( self::DASH_PLUGIN ));
        }
    }

}
