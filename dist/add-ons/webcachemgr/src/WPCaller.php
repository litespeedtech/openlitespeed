<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2023
 * *******************************************
 */

namespace Lsc\Wp;

use Lsc\Wp\Context\Context;

/**
 * Calls WP internal functions in SHORTINIT mode.
 */
class WPCaller
{

    const LSCWP_PLUGIN = 'litespeed-cache/litespeed-cache.php';
    const LSCWP_HTTP_HOST_TEST = 'litespeed_something_is_wrong';

    /**
     * @var null|WPCaller
     */
    private static $instance = null;

    /**
     * @var WPInstall
     */
    private $currInstall;

    /**
     * @since 1.13.4.4
     * @var string
     */
    private $advancedCacheFile;

    /**
     * @var boolean
     */
    private $loadLscwp;

    /**
     * @var string
     */
    private $pluginEntry;

    /**
     * @since 1.13
     * @var null|string
     */
    private $installedLscwpVer = null;

    /**
     * @var string[]
     */
    private $outputResult = array();

    /**
     * @var string
     */
    private $massIncr = '';

    /**
     *
     * @param WPInstall $curInstall
     * @param boolean   $loadLscwp
     *
     * @throws LSCMException  Thrown indirectly.
     */
    private function __construct( WPInstall $curInstall, $loadLscwp )
    {
        $this->currInstall = $curInstall;
        $this->loadLscwp = $loadLscwp;

        $this->init();
    }

    /**
     *
     * @since 1.13.4.4
     *
     * @throws LSCMException  Thrown indirectly.
     */
    private function init()
    {
        $this->advancedCacheFile =
                "{$this->currInstall->getPath()}/wp-content/advanced-cache.php";

        $this->initWp();
    }

    /**
     *
     * @param WPInstall $currInstall
     * @param boolean   $loadLscwp
     *
     * @return WPCaller
     * @throws LSCMException  Thrown indirectly.
     */
    public static function getInstance( WPInstall $currInstall,
            $loadLscwp = true )
    {
        if ( self::$instance == null
                || self::$instance->currInstall !== $currInstall ) {

            self::$instance = new self($currInstall, $loadLscwp);
        }

        return self::$instance;
    }

    /**
     *
     * @param int    $errno   Not used at this time.
     * @param string $errstr
     *
     * @return boolean
     *
     * @noinspection PhpUnusedParameterInspection
     */
    public static function warning_handler( $errno, $errstr )
    {
        $strs = array(
            'ini_set() has been disabled for security reasons',
            'Constant FS_METHOD already defined'
        );

        if ( in_array($errstr, $strs) ) {
            /**
             * Throw this warning out.
             */
            return true;
        }

        return false;
    }

    /**
     * Deprecated 03/12/19 as this function is no longer used.
     *
     * Prevents database table options not in the $allowedOptions list
     * from having their value updated by others during execution.
     *
     * Run when WP function apply_filters('pre_update_option', ...) is called
     * in WP function update_option().
     *
     * @deprecated
     *
     * @param mixed  $value      New option value.
     * @param string $option     Option name in db.
     * @param mixed  $old_value  Old option value.
     *
     * @return mixed
     */
    public static function lock_database( $value, $option, $old_value )
    {
        $allowedOptions = array(
            'litespeed-cache-conf',
            '_transient_lscwp_whm_install',
            'active_plugins',
            '_transient_doing_cron',
            '_site_transient_update_plugins',
            'uninstall_plugins'
        );

        if ( in_array($option, $allowedOptions) ) {
            return $value;
        }
        else {
            return $old_value;
        }
    }

    /**
     * Redefine disabled PHP global/core functions that no longer exist
     * (PHP 8+).
     *
     * @since 1.13.10
     */
    private static function redefineDisabledFunctions()
    {
        if ( Util::betterVersionCompare(phpversion(), '8.0', '>=') ) {
            include_once(__DIR__ . '/RedefineGlobalFuncs.php');
        }
    }

    /**
     * @return string[]
     */
    public function getOutputResult()
    {
        return $this->outputResult;
    }

    /**
     * Adds key value pair to $this->outputResult to be grabbed later in
     * the $output variable of the UserCommand::issue() exec call.
     *
     * @param string $key
     * @param mixed  $value
     */
    private function outputResult( $key, $value )
    {
        $this->outputResult[$key] = $value;
    }

    /**
     *
     * @deprecated 1.9  Use Logger::getLogMsgQueue() to get these messages as
     *                  LogEntry objects.
     * @return string[]
     */
    public function getDebugMsgs()
    {
        $debugMsgs = array();

        $msgQueue = Logger::getLogMsgQueue();

        foreach ( $msgQueue as $logEntry ) {
            $label = Logger::getLvlDescr($logEntry->getLvl());
            $debugMsgs[] = "[$label] {$logEntry->getMsg()}";
        }

        return $debugMsgs;
    }

    /**
     * @deprecated 1.9  Deprecated 07/30/19. Use
     *                  Logger::getUiMsgs(Logger::UI_ERR) to get these messages.
     * @return string[]
     * @throws LSCMException  Thrown indirectly.
     */
    public function getErrMsgs()
    {
        return Logger::getUiMsgs(Logger::UI_ERR);
    }

    /**
     *
     * @deprecated 1.9  Deprecated 07/30/19. Use
     *                  Logger::getUiMsgs(Logger::UI_SUCC) to get these
     *                  messages.
     * @return string[]
     * @throws LSCMException  Thrown indirectly.
     */
    public function getSuccMsgs()
    {
        return Logger::getUiMsgs(Logger::UI_SUCC);
    }

    /**
     *
     * WP Variables $wpdb, $table_prefix
     * WP Functions: get_option()
     *
     * @global \wpdb  $wpdb
     * @global string $table_prefix
     *
     * @return string
     *
     * @noinspection PhpUndefinedClassInspection
     */
    private function getSiteURL()
    {
        global $wpdb, $table_prefix;

        /** @noinspection PhpUndefinedFunctionInspection */
        $siteURL = get_option('siteurl');

        if ( !$siteURL ) {
            return '';
        }


        if ( strpos($siteURL, self::LSCWP_HTTP_HOST_TEST) !== false ) {
            /**
             * User is setting WP_SITEURL using fake $_SERVER['HTTP_HOST'].
             * Get siteurl value from DB directly.
             */
            $query = "SELECT option_value FROM {$table_prefix}options "
                . "WHERE option_name = 'siteurl'";
            $siteURL = $wpdb->get_var($query);
        }

        return $siteURL;
    }

    /**
     *
     * @since 1.13.4.3
     *
     * @return bool
     */
    private function generic3rdPartyAdvCachePluginExists()
    {
        if ( Util::betterVersionCompare($this->installedLscwpVer, '3.0.4', '>=') ) {

            /**
             * Old LSCWP advanced-cache.php file is no longer used in these
             * versions but is also never cleaned up. As a result, any existing
             * advanced-cache.php files need to have their contents checked to
             * avoid detecting an old LSCWP advanced-cache.php file as a generic
             * 3rd-party advanced-cache plugin.
             */

            if ( file_exists($this->advancedCacheFile)
                    && file_get_contents($this->advancedCacheFile) !== ''
                    && !$this->advancedCacheFileHasLscacheDefine() ) {

                return true;
            }
        }
        /** @noinspection PhpUndefinedConstantInspection */
        elseif ( !defined('LSCACHE_ADV_CACHE') || LSCACHE_ADV_CACHE !== true ) {
            return true;
        }

        return false;
    }

    /**
     *
     * WP Functions: is_plugin_active()
     *
     * @since 1.13.8  Added optional parameter $output.
     *
     * @param WPInstall $install
     * @param bool      $output
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    private function checkStatus( WPInstall $install, $output = false )
    {
        if ( $output == true ) {

            if ( ($siteUrl = $this->getSiteURL()) === '' ) {
                Logger::uiError(
                    'Could not retrieve siteURL to match against known '
                        . 'docroots.'
                );
                $install->addUserFlagFile();

                return (WPInstall::ST_ERR_SITEURL | WPInstall::ST_FLAGGED);
            }

            $this->outputResult('SITEURL', $siteUrl);
        }

        $status = 0;

        /**
         * Check if plugin files exist first, as status in db could be stale if
         * LSCWP was removed manually.
         *
         * @noinspection PhpUndefinedFunctionInspection
         */
        if ( file_exists($this->pluginEntry)
                && is_plugin_active(self::LSCWP_PLUGIN) ) {

            $status |= WPInstall::ST_PLUGIN_ACTIVE;

            //TODO: Get rid of ST_LSC_ADVCACHE_DEFINED status or replace with
            //      new "is caching enabled" define check.

            if ( $this->generic3rdPartyAdvCachePluginExists() ) {
                $status |= WPInstall::ST_FLAGGED;
            }
            else {
                $status |= WPInstall::ST_LSC_ADVCACHE_DEFINED;
            }
        }
        else {
            $status |= WPInstall::ST_PLUGIN_INACTIVE;
        }

        if ( $install->hasFlagFile() ) {
            $status |= WPInstall::ST_FLAGGED;
        }

        if ( $status & WPInstall::ST_FLAGGED ) {

            if ( $install->addUserFlagFile() ) {
                Logger::notice('Install is flagged');
            }
            else {
                Logger::notice('Install is not flagged');
            }
        }

        return $status;
    }

    /**
     *
     * @param boolean $output
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    public function updateStatus( $output = false )
    {
        $status = $this->checkStatus($this->currInstall, $output);
        $this->currInstall->setStatus($status);

        if ( $output ) {
            $this->outputResult('STATUS', $status);
        }

        if ( $status & WPInstall::ST_ERR_SITEURL ) {
            return UserCommand::EXIT_FAIL;
        }

        return UserCommand::EXIT_SUCC;
    }

    /**
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    private function getCurrStatus()
    {
        $this->updateStatus();

        return $this->currInstall->getStatus();
    }

    /**
     * Check if any known cache plugins that do not use an advanced-cache.php
     * file are active for this WordPress installation. If any of these
     * plugins are found, that plugin's slug is returned.
     *
     * @since 1.9.1
     *
     * @return string  Empty string or detected active cache plugin slug.
     */
    private function checkForKnownNonAdvCachePlugins()
    {
        $knownPlugins = array(
            'wp-fastest-cache' => 'wp-fastest-cache/wpFastestCache.php'
        );


        foreach ( $knownPlugins as $slug => $plugin ) {
            /**
             * Check if plugin files exist first, as status in db could be
             * stale if plugin files were removed manually.
             *
             * @noinspection PhpUndefinedConstantInspection
             * @noinspection PhpUndefinedFunctionInspection
             */
            if ( file_exists(WP_PLUGIN_DIR . "/$plugin")
                    && is_plugin_active($plugin) ) {

                return $slug;
            }
        }

        return '';
    }

    /**
     *
     * WP Constants: WP_PLUGIN_DIR
     * WP Functions: get_option()
     *
     * @param boolean $isMassAction
     * @param boolean $isNewInstall
     *
     * @return boolean
     * @throws LSCMException  Thrown indirectly.
     */
    private function canEnable( $isMassAction, $isNewInstall )
    {
        $status = $this->getCurrStatus();

        if ( $status & WPInstall::ST_PLUGIN_INACTIVE ) {

            if ( !$isNewInstall && $isMassAction ) {
                $this->currInstall->addUserFlagFile();

                Logger::uiSuccess(
                    'LSCWP Detected As Manually Disabled - Flag Set'
                );
                Logger::notice(
                    'Ignore - Previously disabled, flag it from mass operation'
                );

                return false;
            }

            $thirdPartyCachePluginSlug =
                $this->checkForKnownNonAdvCachePlugins();

            if ( $thirdPartyCachePluginSlug != '' ) {
                $this->currInstall->addUserFlagFile();

                Logger::uiError(
                    'Cannot Enable LSCWP - Detected another active cache '
                        . "plugin \"$thirdPartyCachePluginSlug\". Flag set."
                );

                Logger::notice(
                    'Ignore - Detected another active cache plugin '
                        . "\"$thirdPartyCachePluginSlug\". Flagged."
                );

                return false;
            }
        }
        elseif ( !$isNewInstall ) {
            /**
             * already active
             */
            if ( $status & WPInstall::ST_LSC_ADVCACHE_DEFINED ) {
                Logger::uiSuccess('LSCWP Already Enabled - No Action Taken');
                Logger::notice('Ignore - Already enabled');
            }
            else {
                $this->currInstall->addUserFlagFile();

                Logger::uiError(
                    'LSCWP Already Enabled But Not Caching - Detected another '
                        . 'active cache plugin. Please visit the WordPress '
                        . 'Dashboard for further instructions.'
                );
                Logger::notice(
                    'Ignore - Existing install but advanced cache not set'
                );
            }

            return false;
        }

        return true;
    }

    /**
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    public function directEnable()
    {
        $isNewInstall = !file_exists($this->pluginEntry);

        if ( !$this->canEnable(false, $isNewInstall) ) {
            $this->outputResult('STATUS', $this->currInstall->getStatus());
            return UserCommand::EXIT_FAIL;
        }

        if ( !$isNewInstall ) {
            $status = $this->enable_lscwp();
        }
        else {
            $status = $this->directEnableNewInstall();
        }

        $this->outputResult('STATUS', $status);
        return UserCommand::EXIT_SUCC;
    }

    /**
     *
     * WP Constants: WP_PLUGIN_DIR
     * WP Functions: unzip_file()
     * WP Classes: WP_Filesystem
     *
     * @return int
     * @throws LSCMException  Thrown directly and indirectly.
     */
    private function directEnableNewInstall()
    {
        /** @noinspection PhpUndefinedConstantInspection */
        $pluginDir = WP_PLUGIN_DIR;

        $lscwpZip = "$pluginDir/litespeed-cache.latest-stable.zip";

        $this->downloadLSCWPZip($lscwpZip);

        /** @noinspection PhpUndefinedFunctionInspection */
        WP_Filesystem();

        /** @noinspection PhpUndefinedFunctionInspection */
        $unzipRet = unzip_file($lscwpZip, $pluginDir);
        unlink($lscwpZip);

        if ( $unzipRet !== true ) {
            throw new LSCMException(
                "Unable to extract downloaded LSCWP files.",
                LSCMException::E_NON_FATAL
            );
        }

        $this->currInstall->addNewLscwpFlagFile();

        $customIni = Context::LOCAL_PLUGIN_DIR . '/'
            . PluginVersion::LSCWP_DEFAULTS_INI_FILE_NAME;
        $defaultIni = "$pluginDir/litespeed-cache/data/"
            . PluginVersion::LSCWP_DEFAULTS_INI_FILE_NAME;

        if ( file_exists($customIni) ) {
            copy($customIni, $defaultIni);
        }

        $this->installedLscwpVer = $this->getPluginVersionFromFile();

        $status = $this->enable_lscwp();

        if ( $status & WPInstall::ST_PLUGIN_INACTIVE) {
            $this->removeLscwpPluginFiles();
        }
        else {
            $this->updateTranslationFiles();
        }

        return $status;
    }

    /**
     *
     * WP Constants: WP_PLUGIN_DIR
     *
     * @param string $lscwpZip
     *
     * @return null
     * @throws LSCMException
     */
    private function downloadLscwpZip( $lscwpZip )
    {
        /** @noinspection PhpUndefinedConstantInspection */
        $pluginDir = WP_PLUGIN_DIR;
        $url = 'https://downloads.wordpress.org/plugin/'
            . 'litespeed-cache.latest-stable.zip';

        $wget_command =
            "wget -q --tries=1 --no-check-certificate $url -P $pluginDir";
        exec($wget_command, $output, $return_var);

        if ( $return_var === 0 && file_exists($lscwpZip) ) {
            return;
        }

        $exMsg = "Failed to download LSCWP with wget exit status $return_var";

        /**
         * Fall back to curl in case wget is disabled for user.
         */
        $curl_command =
            "cd $pluginDir && curl -O -s --retry 1 --insecure $url";
        exec($curl_command, $output, $return_var);

        if ( $return_var === 0 && file_exists($lscwpZip) ) {
            return;
        }

        $exMsg .= " and curl exit status $return_var.";

        throw new LSCMException($exMsg, LSCMException::E_NON_FATAL);
    }

    /**
     *
     * WP Constants: WP_PLUGIN_DIR
     *
     * @param array   $extraArgs  Not used at this time.
     * @param boolean $massOp     True when called from massEnable().
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     *
     * @noinspection PhpUnusedParameterInspection
     */
    public function enable( $extraArgs, $massOp = false )
    {
        /** @noinspection PhpUndefinedConstantInspection */
        $isNewInstall =
            PluginVersion::getInstance()->prepareUserInstall(WP_PLUGIN_DIR);

        if ( $isNewInstall ) {
            $this->installedLscwpVer = $this->getPluginVersionFromFile();
            $this->currInstall->addNewLscwpFlagFile();
        }

        if ( !$this->canEnable($massOp, $isNewInstall) ) {
            $status = $this->currInstall->getStatus();
            $ret = UserCommand::EXIT_FAIL;
        }
        else {
            $status = $this->enable_lscwp();
            $ret = UserCommand::EXIT_SUCC;
        }

        if ( $isNewInstall ) {

            if ( $status & WPInstall::ST_PLUGIN_INACTIVE ) {
                $this->removeLscwpPluginFiles();
            }
            else {
                $this->updateTranslationFiles();
            }
        }

        $this->outputResult('STATUS', $this->currInstall->getStatus());
        return $ret;
    }

    /**
     *
     * @param string[] $extraArgs
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    public function massEnable( $extraArgs )
    {
        $ret = $this->enable($extraArgs, true);

        if ( $this->massIncr != '' ) {
            $this->outputResult('MASS_INCR', $this->massIncr);
        }

        return $ret;
    }

    /**
     *
     * WP Functions: is_plugin_active_for_network()
     *
     * @param boolean $isMassAction
     *
     * @return boolean
     * @throws LSCMException  Thrown indirectly.
     */
    private function canDisable( $isMassAction )
    {
        $status = $this->getCurrStatus();

        if ( $status & WPInstall::ST_PLUGIN_INACTIVE ) {
            Logger::notice('Ignore - Already disabled');
            Logger::uiSuccess(
                'LiteSpeed Cache Already Disabled - No Action Taken'
            );

            return false;
        }

        if ( $isMassAction ) {

            if ( !($status & WPInstall::ST_LSC_ADVCACHE_DEFINED) ) {
                $this->currInstall->addUserFlagFile();

                Logger::uiSuccess(
                    'LSCWP Detected As Manually Enabled But Not Caching - Flag '
                        . 'Set. If desired, this installation can be disabled '
                        . 'from the Manage Cache Installations screen.'
                );
                Logger::notice(
                    'Ignore for mass disable - Installed manually as advanced '
                        . 'cache not set.'
                );

                return false;
            }

            /** @noinspection PhpUndefinedFunctionInspection */
            if ( is_plugin_active_for_network(self::LSCWP_PLUGIN) ) {
                $this->currInstall->addUserFlagFile();

                Logger::uiSuccess(
                    'LiteSpeed Cache Detected As Network Activated - Flag Set '
                        . '& No Action Taken'
                );

                return false;
            }
        }

        return true;
    }

    /**
     *
     * @param string[] $extraArgs  Not used at this time.
     * @param boolean  $massOp     True when called from MassDisable().
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     *
     * @noinspection PhpUnusedParameterInspection
     */
    public function disable( $extraArgs, $massOp = false )
    {
        if ( !$this->canDisable($massOp) ) {
            $ret = UserCommand::EXIT_FAIL;
        }
        else {
            $status = $this->performDisable(true);

            if ( $status & WPInstall::ST_PLUGIN_ACTIVE ) {
                $ret = UserCommand::EXIT_FAIL;
            }
            else {
                Logger::uiSuccess('LiteSpeed Cache Disabled');
                $this->massIncr = 'SUCC';
                $ret = UserCommand::EXIT_SUCC;
            }
        }

        $this->outputResult('STATUS', $this->currInstall->getStatus());

        return $ret;
    }

    /**
     *
     * @param string[] $extraArgs
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    public function massDisable( $extraArgs )
    {
        $ret = $this->disable($extraArgs, true);

        if ( $this->massIncr != '' ) {
            $this->outputResult('MASS_INCR', $this->massIncr);
        }

        return $ret;
    }

    /**
     * Includes LSCWP files needed to properly disable the LSCWP plugin.
     *
     * @since 1.13  Removed param $lscwpVer.
     *
     * @noinspection PhpIncludeInspection
     */
    private function includeDisableRequiredFiles()
    {
        /** @noinspection PhpUndefinedConstantInspection */
        $dir = WP_PLUGIN_DIR . '/litespeed-cache';

        if ( Util::betterVersionCompare($this->installedLscwpVer, '3.0', '>=') ) {
            require_once "$dir/src/admin.cls.php";
        }
        elseif ( Util::betterVersionCompare($this->installedLscwpVer, '1.1.2.2', '>') ) {
            require_once "$dir/admin/litespeed-cache-admin.class.php";
        }
        else {
            require_once "$dir/admin/class-litespeed-cache-admin.php";
        }

        if ( Util::betterVersionCompare($this->installedLscwpVer, '1.1.0', '<')
                && Util::betterVersionCompare($this->installedLscwpVer, '1.0.6', '>') ) {

            require_once "$dir/admin/class-litespeed-cache-admin-rules.php";
        }
    }

    /**
     *
     * @param boolean $uninstall
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    private function performDisable( $uninstall )
    {
        $this->includeDisableRequiredFiles();
        return $this->disable_lscwp($uninstall);
    }

    /**
     *
     * @param string[] $fromVersions
     * @param string   $toVersion
     * @param boolean  $massOp        Not used at this time.
     *
     * @return boolean
     *
     * @noinspection PhpUnusedParameterInspection
     */
    private function canUpgrade( $fromVersions, $toVersion, $massOp )
    {
        if ( !file_exists($this->pluginEntry) ) {
            return false;
        }

        if ( $toVersion == $this->installedLscwpVer ) {
            return false;
        }

        foreach ( $fromVersions as $fromVer ) {
            $fromVerParts = explode('.', $fromVer);
            $installedVerParts = explode('.', $this->installedLscwpVer);

            $i = 0;

            while ( isset($fromVerParts[$i]) ) {
                $fromVerPart = $fromVerParts[$i];

                if ( $fromVerPart == 'x' ) {
                    return true;
                }

                if ( !isset($installedVerParts[$i])
                        || $installedVerParts[$i] != $fromVerPart ) {

                    continue 2;
                }

                $i++;
            }

            if ( !isset($installedVerParts[$i]) ) {
                return true;
            }
        }

        return false;
    }

    /**
     *
     * @param string[] $extraArgs
     * @param boolean  $massOp
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    public function upgrade( $extraArgs, $massOp = false )
    {
        $fromVersions = explode(',', $extraArgs[0]);
        $toVersion = $extraArgs[1];

        if ( !$this->canUpgrade($fromVersions, $toVersion, $massOp) ) {
            $this->updateStatus(true);
            $ret = UserCommand::EXIT_FAIL;
        }
        else {
            $this->upgrade_lscwp($toVersion);
            $ret = UserCommand::EXIT_SUCC;
        }

        return $ret;
    }

    /**
     *
     * @param string[] $extraArgs
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    public function massUpgrade( $extraArgs )
    {
        $ret = $this->upgrade($extraArgs, true);

        if ( $this->massIncr != '' ) {
            $this->outputResult('MASS_INCR', $this->massIncr);
        }

        return $ret;
    }

    /**
     *
     * WP Functions: deactivate_plugins(), delete_plugins()
     *
     * @param boolean $uninstall
     */
    private function deactivate_lscwp( $uninstall )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        deactivate_plugins(self::LSCWP_PLUGIN);

        if ( $uninstall ) {
            //Todo: add some msg about having removed plugin files?
            /** @noinspection PhpUndefinedFunctionInspection */
            delete_plugins(array( self::LSCWP_PLUGIN ));
        }
    }

    /**
     *
     * WP Constants: MULTISITE
     * WP Variables: $wpdb
     * WP Functions: switch_to_blog(), restore_current_blog()
     *
     * @global \wpdb $wpdb
     *
     * @param boolean $uninstall
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     *
     * @noinspection PhpUndefinedClassInspection
     */
    private function disable_lscwp( $uninstall )
    {
        /** @noinspection PhpUndefinedConstantInspection */
        if ( MULTISITE ) {
            global $wpdb;

            $blogs = $wpdb->get_col("SELECT blog_id FROM $wpdb->blogs;");

            foreach ( $blogs as $id ) {
                /** @noinspection PhpUndefinedFunctionInspection */
                switch_to_blog($id);

                $this->deactivate_lscwp($uninstall);

                /** @noinspection PhpUndefinedFunctionInspection */
                restore_current_blog();
            }
        }
        else {
            $this->deactivate_lscwp($uninstall);
        }

        return $this->getCurrStatus();
    }

    /**
     *
     * WP Constants: WP_CACHE
     * WP Functions: activate_plugin()
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    private function enable_lscwp()
    {
        /**
         * Should not check directly, can error on success due to object cache.
         *
         * @noinspection PhpUndefinedFunctionInspection
         */
        activate_plugin(self::LSCWP_PLUGIN, '', false, false);

        $status = $this->getCurrStatus();

        if ( !($status & WPInstall::ST_LSC_ADVCACHE_DEFINED) ) {
            $status = $this->performDisable(true);

            Logger::uiError(
                'Detected '
                    . "{$this->currInstall->getPath()}/wp-content/advanced-cache.php "
                    . 'as belonging to another cache plugin. Please deactivate '
                    . 'the related cache plugin and try again. You may also '
                    . 'try manually installing through the WordPress Dashboard '
                    . 'and following the instructions given.'
            );

            $this->massIncr = 'FAIL';
        }
        elseif ( $status & WPInstall::ST_PLUGIN_ACTIVE ) {

            if ( !is_writable($this->currInstall->getWpConfigFile())
                    && (!defined('WP_CACHE') || WP_CACHE !== true) ) {

                /**
                 * LSCACHE_ADV_CACHE is incorrectly defined true at this point.
                 * Detected status must be manually corrected.
                 */
                $status &= ~WPInstall::ST_LSC_ADVCACHE_DEFINED;
                $this->currInstall->setStatus($status);

                $this->currInstall->addUserFlagFile();
                $status = $this->currInstall->getStatus();

                Logger::uiError(
                    'LSCWP Enabled But Not Caching - Please visit the '
                        . 'WordPress Dashboard for further instructions.'
                );
            }
            else {
                Logger::uiSuccess('LSCWP Enabled');
            }

            $this->massIncr = 'SUCC';
        }

        return $status;
    }

    /**
     *
     * WP Constants: WP_PLUGIN_DIR
     * WP Functions: add_filter(), remove_filter(), is_wp_error(),
     *               wp_clean_plugins_cache()
     * WP Classes: Plugin_Upgrader
     *
     * @param string  $ver
     * @param boolean $runHooks
     *
     * @throws LSCMException
     */
    private function upgrade_lscwp( $ver, $runHooks = true )
    {
        /**
         * Label the following $upgrader output (Cannot be buffered).
         */
        echo "[UPGRADE]\n";

        /**
         * @noinspection PhpUndefinedClassInspection
         * @noinspection PhpFullyQualifiedNameUsageInspection
         */
        $upgrader = new \Plugin_Upgrader;

        $upgrader->init();
        $upgrader->upgrade_strings();

        $lscwpPackageURL =
            "https://downloads.wordpress.org/plugin/litespeed-cache.$ver.zip";

        if ( $runHooks ) {
            /** @noinspection PhpUndefinedFunctionInspection */
            add_filter(
                'upgrader_pre_install',
                array( $upgrader, 'deactivate_plugin_before_upgrade' ),
                10,
                2
            );

            /** @noinspection PhpUndefinedFunctionInspection */
            add_filter(
                'upgrader_clear_destination',
                array( $upgrader, 'delete_old_plugin' ),
                10,
                4
            );
        }

        /**
         * @noinspection PhpUndefinedConstantInspection
         */
        $upgrader->run(
            array(
                'package' => $lscwpPackageURL,
                'destination' => WP_PLUGIN_DIR,
                'clear_destination' => true,
                'clear_working' => true,
                'hook_extra' => array(
                    'plugin' => $this->pluginEntry,
                    'type' => 'plugin',
                    'action' => 'update',
                )
            )
        );

        /**
         * Start new messages on a new line
         */
        echo "\n";

        if ( $runHooks ) {
            /**
             * Cleanup our hooks, in case something else does a upgrade on
             * this connection.
             *
             * @noinspection PhpUndefinedFunctionInspection
             */
            remove_filter(
                'upgrader_pre_install',
                array( $upgrader, 'deactivate_plugin_before_upgrade' )
            );

            /** @noinspection PhpUndefinedFunctionInspection */
            remove_filter(
                'upgrader_clear_destination',
                array( $upgrader, 'delete_old_plugin' )
            );
        }

        /**
         * @noinspection PhpUndefinedFunctionInspection
         */
        if ( !$upgrader->result || is_wp_error($upgrader->result) ) {
            throw new LSCMException(
                "Failed to upgrade to v$ver.",
                LSCMException::E_NON_FATAL
            );
        }

        $this->updateTranslationFiles();

        /**
         * Force refresh of plugin update information
         *
         * @noinspection PhpUndefinedFunctionInspection
         */
        wp_clean_plugins_cache(true);
    }

    /**
     * Gets LSCWP version from the litespeed-cache.php file.
     *
     * WP Functions: get_plugin_data()
     *
     * @return string
     */
    private function getPluginVersionFromFile()
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        $lscwp_data = get_plugin_data($this->pluginEntry, false, false);

        return $lscwp_data['Version'];
    }

    /**
     * Checks for local plugin translation files and copies them to the plugin
     * languages directory if able. This function will also attempt to inform
     * the root user when a locale's translation should be retrieved or removed.
     *
     * WP Functions: get_locale(), unzip_file()
     * WP Classes: WP_Filesystem
     */
    public function updateTranslationFiles()
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        $locale = get_locale();

        if ( $locale == 'en_US' ) {
            return;
        }

        $localTranslationDir = Context::LOCAL_PLUGIN_DIR
            . "/$this->installedLscwpVer/translations";

        $moFileName = "litespeed-cache-$locale.mo";
        $poFileName = "litespeed-cache-$locale.po";
        $localMoFile = "$localTranslationDir/$moFileName";
        $localPoFile = "$localTranslationDir/$poFileName";
        $zipFile = "$localTranslationDir/$locale.zip";
        $translationFlag = "$localTranslationDir/"
            . PluginVersion::TRANSLATION_CHECK_FLAG_BASE . "_$locale";

        $langDir =
            $this->currInstall->getPath() . '/wp-content/languages/plugins';

        if ( !file_exists($langDir) ) {
            mkdir($langDir, 0755);
        }

        if ( file_exists($localMoFile) && file_exists($localPoFile) ) {
            copy($localMoFile, "$langDir/$moFileName");
            copy($localPoFile, "$langDir/$poFileName");
        }
        elseif ( file_exists($zipFile) ) {
            /**
             * @noinspection PhpUndefinedFunctionInspection
             * @noinspection PhpFullyQualifiedNameUsageInspection
             */
            \WP_Filesystem();

            /** @noinspection PhpUndefinedFunctionInspection */
            if ( unzip_file($zipFile, $langDir) !== true ) {
                $this->outputResult(
                    'BAD_TRANSLATION',
                    "$locale $this->installedLscwpVer"
                );
            }
        }
        elseif ( !file_exists($translationFlag) ||
                (time() - filemtime($translationFlag)) > 86400  ) {

            $this->outputResult(
                'GET_TRANSLATION',
                "$locale $this->installedLscwpVer"
            );
        }
    }

    /**
     *
     * @since 1.13.4.4
     *
     * @return bool
     */
    private function advancedCacheFileHasLscacheDefine()
    {
        $content = file_get_contents($this->advancedCacheFile);

        if ( strpos($content, 'LSCACHE_ADV_CACHE') !== false ) {
            return true;
        }

        return false;
    }

    private function includeLSCWPAdvancedCacheFile()
    {
        if ( file_exists($this->advancedCacheFile) ) {

            if ( $this->advancedCacheFileHasLscacheDefine() ) {
                /** @noinspection PhpIncludeInspection */
                include_once $this->advancedCacheFile;
            }
        }
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly.
     */
    public function removeLscwpPluginFiles()
    {
        $this->currInstall->removePluginFiles(dirname($this->pluginEntry));
    }

    /**
     *
     * @param string[] $extraArgs
     * @param boolean  $massOp     Not used at this time.
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     *
     * @noinspection PhpUnusedParameterInspection
     */
    public function dashNotify( $extraArgs, $massOp = false )
    {
        $jsonInfo = base64_decode($extraArgs[0]);

        $ret = UserCommand::EXIT_FAIL;

        if ( DashNotifier::canNotify($this->currInstall->getPath()) ) {
            DashNotifier::prepareUserInstall();

            if ( DashNotifier::doNotify($jsonInfo) ) {
                Logger::uiSuccess('Notified Successfully');
                $this->massIncr = 'SUCC';
                $ret = UserCommand::EXIT_SUCC;
            }
            else {
                Logger::uiError('Failed to Notify');
                $this->massIncr = 'FAIL';
            }
        }
        else {
            $this->massIncr = 'BYPASS';
        }

        return $ret;
    }

    /**
     *
     * @param string[] $extraArgs
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    public function massDashNotify( $extraArgs )
    {
        $ret = $this->dashNotify($extraArgs, true);

        if ( $this->massIncr != '' ) {
            $this->outputResult('MASS_INCR', $this->massIncr);
        }

        return $ret;
    }

    /**
     *
     * WP Constants: MULTISITE
     * WP Variables: $wpdb
     * WP Functions: switch_to_blog(), restore_current_blog(),
     *               is_plugin_active()
     *
     * @global \wpdb $wpdb
     *
     * @param string[] $extraArgs  Unused for now.
     * @param boolean  $massOp
     *
     * @return int
     *
     * @noinspection PhpUndefinedClassInspection
     * @noinspection PhpUnusedParameterInspection
     */
    public function dashDisable( $extraArgs, $massOp = false )
    {
        /** @noinspection PhpUndefinedConstantInspection */
        if ( MULTISITE ) {
            global $wpdb;

            $blogs = $wpdb->get_col("SELECT blog_id FROM $wpdb->blogs;");

            foreach ( $blogs as $id ) {
                /** @noinspection PhpUndefinedFunctionInspection */
                switch_to_blog($id);

                DashNotifier::deactivate(true);

                /** @noinspection PhpUndefinedFunctionInspection */
                restore_current_blog();
            }
        }
        else {
            DashNotifier::deactivate(true);
        }

        /** @noinspection PhpUndefinedFunctionInspection */
        if ( is_plugin_active(DashNotifier::DASH_PLUGIN) ) {
            $this->massIncr = 'FAIL';
            $ret = UserCommand::EXIT_FAIL;
        }
        else {
            Logger::uiSuccess('Dash Notifier Disabled');
            $this->massIncr = 'SUCC';
            $ret = UserCommand::EXIT_SUCC;
        }

        return $ret;
    }

    /**
     *
     * @param array $extraArgs
     *
     * @return int
     */
    public function massDashDisable( $extraArgs )
    {
        $ret = $this->dashDisable($extraArgs, true);

        if ( $this->massIncr != '' ) {
            $this->outputResult('MASS_INCR', $this->massIncr);
        }

        return $ret;
    }

    /**
     *
     * WP Functions: apply_filters().
     *
     * @since 1.12
     *
     * @param boolean $setOutputResult
     *
     * @return string
     */
    public function getQuicCloudAPIKey( $setOutputResult = false )
    {
        /** @noinspection PhpUndefinedFunctionInspection */
        $key = apply_filters('litespeed_conf', 'api_key');

        if ( $key == 'api_key' || $key == null ) {
            $key = '';
        }

        if ( $setOutputResult ) {
            $this->outputResult('API_KEY', $key);
        }

        return $key;
    }

    /**
     * Set global server and environment variables.
     *
     * @since 1.9.8
     *
     * @param string $key
     * @param mixed  $val
     */
    private function setEnvVar( $key, $val )
    {
        $_SERVER[$key] = $val;
        putenv("$key=$val");
    }

    /**
     * Checks if the current WordPress installation is a multisite install and
     * does some pre-load setup if so.
     *
     * Patterns and multisite check logic based on WordPress function
     * is_multisite().
     *
     * @since 1.9.8
     *
     * @return boolean
     * @throws LSCMException
     */
    private function isMultisite()
    {
        $isMultiSite = false;

        $pattern1 = '/define\(\s*[\'"]MULTISITE[\'"]\s*,[^;]*;/';
        $pattern2 = '/define\(\s*[\'"]MULTISITE[\'"]\s*,\s*true\s*\)\s*;/';
        $pattern3 = '/define\(\s*[\'"]SUBDOMAIN_INSTALL[\'"]\s*,[^;]*;/';
        $pattern4 = '/define\(\s*[\'"]VHOST[\'"]\s*,[^;]*;/';
        $pattern5 = '/define\(\s*[\'"]SUNRISE[\'"]\s*,[^;]*;/';

        $config_content =
            file_get_contents($this->currInstall->getWpConfigFile());

        if ( preg_match($pattern1, $config_content, $m1)
                && preg_match($pattern2, $m1[0]) ) {

            $isMultiSite = true;
        }
        elseif ( preg_match($pattern3, $config_content)
                || preg_match($pattern4, $config_content)
                || preg_match($pattern5, $config_content) ) {

            $isMultiSite = true;
        }

        if ( $isMultiSite ) {

            $domainPattern = '/define\(\s*[\'"]DOMAIN_CURRENT_SITE[\'"]\s*,'
                . '\s*[\'"](.+)[\'"]\s*\)\s*;/';

            if ( !preg_match($domainPattern, $config_content, $m2) ) {
                throw new LSCMException(
                    'Cannot find DOMAIN_CURRENT_SITE with MULTISITE defined.'
                );
            }

            $this->currInstall->setServerName($m2[1]);

            $pathPattern = '/define\(\s*[\'"]PATH_CURRENT_SITE[\'"]\s*,'
                . '\s*[\'"](.+)[\'"]\s*\)\s*;/';

            if ( !preg_match($pathPattern, $config_content, $m3) ) {
                throw new LSCMException(
                    'Cannot find PATH_CURRENT_SITE with MULTISITE defined.'
                );
            }

            $this->setEnvVar('REQUEST_URI', $m3[1]);

            Util::define_wrapper('WP_NETWORK_ADMIN', true);
        }

        return $isMultiSite;
    }

    /**
     *
     * WP Constants: WP_PLUGIN_DIR
     * WP Variables: $wpdb, $shortcode_tags
     * WP Functions: wp_plugin_directory_constants(), wp_cookie_constants()
     * WP Classes: WP_Query
     *
     * @global \wpdb $wpdb
     * @global array $shortcode_tags
     *
     * @throws LSCMException  Thrown directly and indirectly.
     *
     * @noinspection PhpUndefinedClassInspection
     * @noinspection PhpFullyQualifiedNameUsageInspection
     */
    private function initWp()
    {
        /**
         * Declared global variables for use in included files.
         *
         * @noinspection PhpUnusedLocalVariableInspection
         */
        global $wpdb, $shortcode_tags;

        error_reporting(E_ALL);

        /**
         * Attempt to override any WordPress memory limits.
         */
        Util::define_wrapper('WP_MEMORY_LIMIT', '512M');
        Util::define_wrapper('WP_MAX_MEMORY_LIMIT', '512M');

        /**
         * Only load core WordPress functionality.
         */
        Util::define_wrapper('SHORTINIT', true);

        $wpPath = $this->currInstall->getPath();

        /**
         * Set WP version data global variables, including $wp_version.
         *
         * @noinspection PhpIncludeInspection
         */
        include_once "$wpPath/wp-includes/version.php";

        /** @noinspection PhpUndefinedVariableInspection */
        if ( Util::betterVersionCompare($wp_version, '4.0', '<') ) {
            throw new LSCMException(
                "Detected WordPress version as $wp_version. Version 4.0 '
                    . 'required at minimum."
            );
        }

        /**
         * Set needed server variables.
         */
        $_SERVER['SCRIPT_FILENAME'] = "$wpPath/wp-admin/plugins.php";

        if ( ! $this->isMultisite() ) {
            $this->setEnvVar('REQUEST_URI', '');
        }

        /**
         * Set for LSCWP v1.1.5.1+ plugin logic.
         */
        if ( $docRoot = $this->currInstall->getDocRoot() ) {

            /**
             * For enable/disable.
             */
            $this->setEnvVar('DOCUMENT_ROOT', $docRoot);
        }

        $serverName = $this->currInstall->getServerName();

        if ( empty($serverName) ) {
            $serverName = self::LSCWP_HTTP_HOST_TEST;
        }

        /**
         * For security plugins.
         */
        $this->setEnvVar('HTTP_HOST', $serverName);

        /**
         * Version specific includes may fail on RC releases.
         */

        $includeFiles = array();
        $includeFiles[] = '/wp-load.php';
        $includeFiles[] = '/wp-includes/default-constants.php';
        $includeFiles[] = '/wp-includes/formatting.php';
        $includeFiles[] = '/wp-includes/meta.php';
        $includeFiles[] = '/wp-includes/l10n.php';
        $includeFiles[] = '/wp-includes/class-wp-walker.php';
        $includeFiles[] = '/wp-includes/capabilities.php';

        if ( Util::betterVersionCompare($wp_version, '4.4.0', '>=') ) {
            $includeFiles[] = '/wp-includes/class-wp-roles.php';
            $includeFiles[] = '/wp-includes/class-wp-role.php';
            $includeFiles[] = '/wp-includes/class-wp-user.php';
            $includeFiles[] = '/wp-includes/rest-api.php';
            $includeFiles[] = '/wp-includes/class-wp-http-encoding.php';
            $includeFiles[] = '/wp-includes/class-wp-http-proxy.php';
            $includeFiles[] = '/wp-includes/class-wp-http-response.php';
            $includeFiles[] = '/wp-includes/class-wp-http-curl.php';
            $includeFiles[] = '/wp-includes/class-wp-http-cookie.php';
        }

        $includeFiles[] = '/wp-includes/query.php';
        $includeFiles[] = '/wp-includes/theme.php';
        $includeFiles[] = '/wp-includes/class-wp-theme.php';
        $includeFiles[] = '/wp-includes/user.php';
        $includeFiles[] = '/wp-includes/general-template.php';
        $includeFiles[] = '/wp-includes/link-template.php';
        $includeFiles[] = '/wp-includes/post.php';
        $includeFiles[] = '/wp-includes/kses.php';
        $includeFiles[] = '/wp-includes/cron.php';
        $includeFiles[] = '/wp-includes/update.php';
        $includeFiles[] = '/wp-includes/shortcodes.php';
        $includeFiles[] = '/wp-includes/http.php';
        $includeFiles[] = '/wp-includes/class-http.php';

        if ( Util::betterVersionCompare($wp_version, '4.6.0', '>=') ) {
            $includeFiles[] = '/wp-includes/class-wp-http-requests-response.php';
        }

        if ( Util::betterVersionCompare($wp_version, '4.7.0', '>=') ) {
            $includeFiles[] = '/wp-includes/class-wp-http-requests-hooks.php';

            /**
             * Content contained in /wp-includes/query.php for earlier versions.
             */
            $includeFiles[] = '/wp-includes/class-wp-query.php';
        }

        if ( Util::betterVersionCompare($wp_version, '5.0.0', '>=') ) {
            $includeFiles[] = '/wp-includes/blocks.php';
            $includeFiles[] = '/wp-includes/class-wp-block-parser.php';
        }

        if ( Util::betterVersionCompare($wp_version, '6.1.0', '>=') ) {
            $includeFiles[] = '/wp-includes/class-wp-textdomain-registry.php';
        }

        $includeFiles[] = '/wp-includes/ms-functions.php';
        $includeFiles[] = '/wp-includes/ms-deprecated.php';
        $includeFiles[] = '/wp-includes/pluggable.php';
        $includeFiles[] = '/wp-admin/includes/plugin.php';
        $includeFiles[] = '/wp-admin/includes/file.php';
        $includeFiles[] = '/wp-admin/includes/class-wp-upgrader.php';
        $includeFiles[] = '/wp-admin/includes/misc.php';
        $includeFiles[] = '/wp-admin/includes/template.php';

        set_error_handler('\Lsc\Wp\WPCaller::warning_handler');

        /**
         * Force WP to use PHP I/O file handling.
         */
        Util::define_wrapper('FS_METHOD', 'direct');

        /**
         * Trigger an early return from WP Rocket advanced-cache.php to prevent
         * WP Rocket from serving a cached copy and killing the process. This
         * occurs when calling an action from our control panel plugins and is
         * fixed by more closely matching the environment of a direct cli call.
         */
        unset($_SERVER['REQUEST_METHOD']);

        self::redefineDisabledFunctions();

        foreach ( $includeFiles as $file ) {
            $file = $wpPath . $file;

            if ( !file_exists($file) ) {
                throw new LSCMException(
                    "Could not include missing file $file."
                );
            }

            /** @noinspection PhpIncludeInspection */
            include_once $file;
        }

        restore_error_handler();

        /**
         * Needs to be defined after including files.
         */
        Util::define_wrapper('WP_ADMIN', true);

        /**
         * Define common WP constants and set 'wp_plugin_paths' array.
         */
        /** @noinspection PhpUndefinedFunctionInspection */
        wp_plugin_directory_constants();

        /**
         * Do not load other plugins.
         */
        $GLOBALS['wp_plugin_paths'] = array();

        /** @noinspection PhpUndefinedFunctionInspection */
        wp_cookie_constants();

        /**
         * Create global wp_query (WordPress) object entry. Needed during
         * LSCWP uninstall.
         *
         * @noinspection PhpUndefinedClassInspection
         * @noinspection PhpFullyQualifiedNameUsageInspection
         */
        $GLOBALS['wp_the_query'] = new \WP_Query();
        $GLOBALS['wp_query'] = $GLOBALS['wp_the_query'];

        if ( Util::betterVersionCompare($wp_version, '6.1.0', '>=') ) {
            $GLOBALS['wp_textdomain_registry'] = new \WP_Textdomain_Registry();
        }

        /** @noinspection PhpUndefinedConstantInspection */
        $this->pluginEntry = WP_PLUGIN_DIR . '/' . self::LSCWP_PLUGIN;

        if ( $this->loadLscwp && file_exists($this->pluginEntry) ) {
            /** @noinspection PhpIncludeInspection */
            include $this->pluginEntry;

            $this->installedLscwpVer = $this->getPluginVersionFromFile();

            if ( Util::betterVersionCompare($this->installedLscwpVer, '3.0.4', '<') ) {
                $this->includeLSCWPAdvancedCacheFile();
            }
        }
    }

}
