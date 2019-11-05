<?php

/* * *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2019
 * *******************************************
 */

namespace Lsc\Wp;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\DashNotifier;
use \Lsc\Wp\WPInstall;
use \Lsc\Wp\UserCommand;
Use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;

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
     * @var boolean
     */
    private $loadLscwp;

    /**
     * @var string
     */
    private $pluginEntry;

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
     * @param WPInstall  $curInstall
     */
    private function __construct( WPInstall $curInstall, $loadLscwp )
    {
        $this->currInstall = $curInstall;
        $this->loadLscwp = $loadLscwp;
        $this->initWp();
    }

    /**
     *
     * @param WPInstall  $currInstall
     * @return WPCaller
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
     * @param mixed   $value      New option value.
     * @param string  $option     Option name in db.
     * @param mixed   $old_value  Old option value.
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
     * @param string  $key
     * @param mixed   $value
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
            $debugMsgs[] = "[{$label}] {$logEntry->getMsg()}";
        }

        return $debugMsgs;
    }

    /**
     * @deprecated 1.9  Deprecated 07/30/19. Use
     *                  Logger::getUiMsgs(Logger::UI_ERR) to get these messages.
     * @return string[]
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
     */
    public function getSuccMsgs()
    {
        return Logger::getUiMsgs(Logger::UI_SUCC);
    }

    /**
     *
     * WP Functions: get_option()
     *
     * @global wpdb    $wpdb
     * @global string  $table_prefix
     * @return string
     */
    private function getSiteURL()
    {
        global $wpdb;
        global $table_prefix;

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
     * WP Functions: is_plugin_active()
     *
     * @param WPInstall  $install
     * @return int
     */
    private function checkStatus( WPInstall $install )
    {
        if ( $install->getDocRoot() == null ) {

            if ( ($siteUrl = $this->getSiteURL()) === '' ) {
                Logger::uiError('Could not retrieve siteURL to match against known docroots.');
                $install->addUserFlagFile();

                return (WPInstall::ST_ERR_SITEURL | WPInstall::ST_FLAGGED);
            }

            $this->outputResult('SITEURL', $siteUrl);
        }

        $status = 0;

        /**
         * Check if plugin files exist first, as status in db could be stale if
         * LSCWP was removed manually.
         */
        if ( file_exists($this->pluginEntry)
                && is_plugin_active(self::LSCWP_PLUGIN) ) {

            $status |= WPInstall::ST_PLUGIN_ACTIVE;

            if ( defined('LSCACHE_ADV_CACHE') && LSCACHE_ADV_CACHE === true ) {
                $status |= WPInstall::ST_LSC_ADVCACHE_DEFINED;
            }
            else {
                $status |= WPInstall::ST_FLAGGED;
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
     * @param boolean  $output
     * @return int
     */
    public function updateStatus( $output = false )
    {
        $status = $this->checkStatus($this->currInstall);
        $this->currInstall->setStatus($status);

        if ( $output ) {
            $this->outputResult('STATUS', $status);
        }

        if ( $status & WPInstall::ST_ERR_SITEURL ) {
            return UserCommand::EXIT_FAIL;
        }

        return UserCommand::EXIT_SUCC;
    }

    private function getCurrStatus()
    {
        $this->updateStatus();
        $status = $this->currInstall->getStatus();

        return $status;
    }

    /**
     *
     * WP Constants: WP_PLUGIN_DIR
     * WP Functions: get_option()
     *
     * @param boolean   $isMassAction
     * @return boolean
     */
    private function canEnable( $isMassAction, $isNewInstall )
    {
        $status = $this->getCurrStatus();

        if ( $status & WPInstall::ST_PLUGIN_INACTIVE ) {

            if ( !$isNewInstall && $isMassAction ) {
                $this->currInstall->addUserFlagFile();

                Logger::uiSuccess('LSCWP Detected As Manually Disabled - Flag Set');
                Logger::notice('Ignore - Previously disabled, flag it from mass operation');

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

                $msg = 'LSCWP Already Enabled But Not Caching - Detected another active cache'
                        . ' plugin. Please visit the WordPress Dashboard for further instructions.';

                Logger::uiError($msg);
                Logger::notice('Ignore - Existing install but advanced cache not set');
            }

            return false;
        }

        return true;
    }

    /**
     *
     * @return int
     * @throws LSCMException
     */
    public function directEnable()
    {
        $isNewInstall = (file_exists($this->pluginEntry)) ? false : true;

        if ( !$this->canEnable(false, $isNewInstall) ) {
            $this->outputResult('STATUS', $this->currInstall->getStatus());
            return UserCommand::EXIT_FAIL;
        }

        if (!$isNewInstall) {
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
     * @return int
     * @throws LSCMException
     */
    private function directEnableNewInstall()
    {
        $pluginDir = WP_PLUGIN_DIR;

        $lscwpZip = "{$pluginDir}/litespeed-cache.latest-stable.zip";

        $this->downloadLSCWPZip($lscwpZip);

        WP_Filesystem();
        $unzipRet = unzip_file($lscwpZip, $pluginDir);
        unlink($lscwpZip);

        if ( $unzipRet !== true ) {
            throw new LSCMException("Unable to extract downloaded LSCWP files.",
                    LSCMException::E_NON_FATAL);
        }

        $this->currInstall->addNewLscwpFlagFile();

        $customIni = Context::LOCAL_PLUGIN_DIR . '/'
                . PluginVersion::LSCWP_DEFAULTS_INI_FILE_NAME;
        $defaultIni = "{$pluginDir}/litespeed-cache/data/"
                . PluginVersion::LSCWP_DEFAULTS_INI_FILE_NAME;

        if ( file_exists($customIni) ) {
            copy($customIni, $defaultIni);
        }

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
     * @param string  $lscwpZip
     * @return null
     * @throws LSCMException
     */
    private function downloadLscwpZip( $lscwpZip )
    {
        $pluginDir = WP_PLUGIN_DIR;
        $url = 'https://downloads.wordpress.org/plugin/litespeed-cache.latest-stable.zip';

        $wget_command = "wget -q --tries=1 --no-check-certificate {$url} -P {$pluginDir}";
        exec($wget_command, $output, $return_var);

        if ( $return_var === 0 && file_exists($lscwpZip) ) {
            return;
        }

        $exMsg = "Failed to download LSCWP with wget exit status {$return_var}";

        /**
         * Fall back to curl incase wget is disabled for user.
         */
        $curl_command = "cd {$pluginDir} && curl -O -s --retry 1 --insecure {$url}";
        exec($curl_command, $output, $return_var);

        if ( $return_var === 0 && file_exists($lscwpZip) ) {
            return;
        }

        $exMsg .= " and curl exit status {$return_var}.";

        throw new LSCMException($exMsg, LSCMException::E_NON_FATAL);
    }

    /**
     *
     * @param array    $extraArgs  Not used at this time.
     * @param boolean  $massOp     True when called from massEnable().
     * @return int
     */
    public function enable( $extraArgs, $massOp = false )
    {
        $isNewInstall =
                PluginVersion::getInstance()->prepareUserInstall(WP_PLUGIN_DIR);

        if ( $isNewInstall ) {
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

            if ( $status &= WPInstall::ST_PLUGIN_INACTIVE ) {
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
     * @param array  $extraArgs
     * @return int
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
     * @param boolean   $isMassAction
     * @return boolean
     */
    private function canDisable( $isMassAction )
    {
        $status = $this->getCurrStatus();

        if ( $status & WPInstall::ST_PLUGIN_INACTIVE ) {
            Logger::notice('Ignore - Already disabled');
            Logger::uiSuccess('LiteSpeed Cache Already Disabled - No Action Taken');

            return false;
        }

        if ( $isMassAction ) {

            if ( !($status & WPInstall::ST_LSC_ADVCACHE_DEFINED) ) {
                $this->currInstall->addUserFlagFile();

                $msg = 'LSCWP Detected As Manually Enabled But Not Caching - Flag Set. If '
                        . 'desired, this installation can be disabled from the '
                        . 'Manage Cache Installations screen.';

                Logger::uiSuccess($msg);
                Logger::notice(
                        'Ignore for mass disable - Installed manually as advanced cache not set.');

                return false;
            }

            if ( is_plugin_active_for_network(self::LSCWP_PLUGIN) ) {
                $this->currInstall->addUserFlagFile();

                $msg = 'LiteSpeed Cache Detected As Network Activated - '
                        . 'Flag Set & No Action Taken';
                Logger::uiSuccess($msg);

                return false;
            }
        }

        return true;
    }

    /**
     *
     * @param array    $extraArgs  Not used at this time.
     * @param boolean  $massOp     True when called from MassDisable().
     * @return int
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
     * @param array  $extraArgs
     * @return int
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
     * @param string  $lscwpVer  Current LSCWP version in the WP plugins
     *                            directory.
     */
    private function includeDisableRequiredFiles( $lscwpVer )
    {
        $dir = WP_PLUGIN_DIR . '/litespeed-cache/admin';

        if ( version_compare($lscwpVer, '1.1.2.2', '>') ) {
            require_once "{$dir}/litespeed-cache-admin.class.php";
        }
        else {
            require_once "{$dir}/class-litespeed-cache-admin.php";
        }

        if ( version_compare($lscwpVer, '1.1.0', '<')
                && version_compare($lscwpVer, '1.0.6', '>') ) {

            require_once "{$dir}/class-litespeed-cache-admin-rules.php";
        }
    }

    /**
     *
     * @param boolean  $uninstall
     * @return int
     */
    private function performDisable( $uninstall )
    {
        $this->includeDisableRequiredFiles($this->getPluginVersionFromFile());
        $status = $this->disable_lscwp($uninstall);

        return $status;
    }

    /**
     *
     * @param string[]  $fromVersions
     * @param string    $toVersion
     * @param boolean   $massOp        Not used at this time.
     * @return boolean
     */
    private function canUpgrade( $fromVersions, $toVersion, $massOp )
    {
        if ( !file_exists($this->pluginEntry) ) {
            return false;
        }

        $currVersion = $this->getPluginVersionFromFile();

        if ( $toVersion == $currVersion ) {
            return false;
        }

        $match = false;

        foreach ( $fromVersions as $ver ) {

            if ( strpos($ver, '.x') !== false ) {
                $ver1 = explode('.', $ver);
                $ver2 = explode('.', $currVersion);

                if ( $ver1[0] === $ver2[0] && $ver1[1] === $ver2[1] ) {
                    $match = true;
                    break;
                }
            }
            elseif ( $ver === $currVersion ) {
                $match = true;
                break;
            }
        }

        return $match;
    }

    /**
     *
     * @param string[] $extraArgs
     * @param boolean $massOp
     * @return int
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
     * @return int
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
     * @param boolean  $uninstall
     */
    private function deactivate_lscwp( $uninstall )
    {
        deactivate_plugins(self::LSCWP_PLUGIN);

        if ( $uninstall ) {
            //add some msg about having removed plugin files?
            delete_plugins(array( self::LSCWP_PLUGIN ));
        }
    }

    /**
     *
     * WP Constants: MULTISITE
     * WP Variables: $wpdb
     * WP Functions: switch_to_blog(), restore_current_blog()
     *
     * @global wpdb    $wpdb
     * @param boolean  $uninstall
     * @return int
     */
    private function disable_lscwp( $uninstall )
    {
        if ( MULTISITE ) {
            global $wpdb;

            $blogs = $wpdb->get_col("SELECT blog_id FROM {$wpdb->blogs};");

            foreach ( $blogs as $id ) {
                switch_to_blog($id);
                $this->deactivate_lscwp($uninstall);
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
     */
    private function enable_lscwp()
    {
        /**
         * Should not check directly, can error on success due to object cache.
         */
        activate_plugin(self::LSCWP_PLUGIN, '', false, false);

        $status = $this->getCurrStatus();

        if ( !($status & WPInstall::ST_LSC_ADVCACHE_DEFINED) ) {
            $status = $this->performDisable(true);

            $msg = 'Detected another active cache plugin. Please deactivate the detected plugin and '
                    . 'try again. You may also try manually installing through the WordPress '
                    . 'Dashboard and following the instructions given.';
            Logger::uiError($msg);

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

                $msg = 'LSCWP Enabled But Not Caching - Please visit the WordPress Dashboard '
                        . 'for further instructions.';
                Logger::uiError($msg);

                $this->massIncr = 'SUCC';
            }
            else {
                Logger::uiSuccess('LSCWP Enabled');

                $this->massIncr = 'SUCC';
            }
        }

        return $status;
    }

    /**
     *
     * WP Functions: add_filter(), remove_filter(), is_wp_error(),
     *               wp_clean_plugins_cache()
     * WP Classes: Plugin_Upgrader
     *
     * @param string   $ver
     * @param boolean  $runHooks
     * @throws LSCMException
     */
    private function upgrade_lscwp( $ver, $runHooks = true )
    {
        /**
         * Label the following $upgrader output (Cannot be buffered).
         */
        echo "[UPGRADE]\n";

        $upgrader = new \Plugin_Upgrader;

        $upgrader->init();
        $upgrader->upgrade_strings();

        $lscwpPackageURL = "https://downloads.wordpress.org/plugin/litespeed-cache.{$ver}.zip";

        if ( $runHooks ) {
            add_filter('upgrader_pre_install',
                    array( $upgrader, 'deactivate_plugin_before_upgrade' ), 10, 2);
            add_filter('upgrader_clear_destination',
                    array( $upgrader, 'delete_old_plugin' ), 10, 4);
        }

        $upgrader->run(array(
            'package' => $lscwpPackageURL,
            'destination' => WP_PLUGIN_DIR,
            'clear_destination' => true,
            'clear_working' => true,
            'hook_extra' => array(
                'plugin' => $this->pluginEntry,
                'type' => 'plugin',
                'action' => 'update',
            )
        ));

        /**
         * Start new messages on a new line
         */
        echo "\n";

        if ( $runHooks ) {
            /**
             * Cleanup our hooks, in case something else does a upgrade on
             * this connection.
             */
            remove_filter('upgrader_pre_install',
                    array( $upgrader, 'deactivate_plugin_before_upgrade' ));
            remove_filter('upgrader_clear_destination',
                    array( $upgrader, 'delete_old_plugin' ));
        }

        if ( !$upgrader->result || is_wp_error($upgrader->result) ) {
            throw new LSCMException("Failed to upgrade to v{$ver}.",
            LSCMException::E_NON_FATAL);
        }

        $this->updateTranslationFiles();

        /**
         * Force refresh of plugin update information
         */
        wp_clean_plugins_cache(true);
    }

    /**
     * Gets LSCWP version from the litespeed-cache.php file.
     *
     * WP Constants: WP_PLUGIN_DIR
     * WP Functions: get_plugin_data()
     *
     * @return string
     */
    private function getPluginVersionFromFile()
    {
        $lscwp_data = get_plugin_data($this->pluginEntry, false, false);
        $ver = $lscwp_data['Version'];

        return $ver;
    }

    /**
     * Checks for local plugin translation files and copies them to the plugin
     * languages directory if able. This function will also attempt to inform
     * the root user when a locales translation should be retrieved or removed.
     *
     * WP Functions: get_locale()
     * WP Classes: WP_Filesystem
     */
    public function updateTranslationFiles()
    {
        $locale = get_locale();

        if ( $locale == 'en_US' ) {
            return;
        }

        $pluginVersion = $this->getPluginVersionFromFile();

        $localTranslationDir = Context::LOCAL_PLUGIN_DIR
                . "/{$pluginVersion}/translations";

        $moFileName = "litespeed-cache-{$locale}.mo";
        $poFileName = "litespeed-cache-{$locale}.po";
        $localMoFile = "{$localTranslationDir}/{$moFileName}";
        $localPoFile = "{$localTranslationDir}/{$poFileName}";
        $zipFile = "{$localTranslationDir}/{$locale}.zip";
        $translationFlag = "{$localTranslationDir}/"
                . PluginVersion::TRANSLATION_CHECK_FLAG_BASE . "_{$locale}";

        $langDir = $this->currInstall->getPath() . '/wp-content/languages/plugins';

        if ( !file_exists($langDir) ) {
            mkdir($langDir, 0755);
        }

        if ( file_exists($localMoFile) && file_exists($localPoFile) ) {
            copy($localMoFile, "{$langDir}/{$moFileName}");
            copy($localPoFile, "{$langDir}/{$poFileName}");
        }
        elseif ( file_exists($zipFile) ) {
            \WP_Filesystem();

            if ( unzip_file($zipFile, $langDir) !== true ) {
                $this->outputResult('BAD_TRANSLATION',
                        "{$locale} {$pluginVersion}");
            }
        }
        elseif ( !file_exists($translationFlag) ||
                (time() - filemtime($translationFlag)) > 86400  ) {

            $this->outputResult('GET_TRANSLATION', "{$locale} {$pluginVersion}");
        }

        return;
    }

    private function includeLSCWPAdvancedCacheFile()
    {
        $advCacheFile = $this->currInstall->getPath()
                . '/wp-content/advanced-cache.php';

        if ( file_exists($advCacheFile) ) {
            $content = file_get_contents($advCacheFile);

            if ( strpos($content, 'LSCACHE_ADV_CACHE') !== false ) {
                include_once $advCacheFile;
            }
        }
    }

    public function removeLscwpPluginFiles()
    {
        $this->currInstall->removePluginFiles(dirname($this->pluginEntry));
    }

    public function dashNotify( $extraArgs, $massOp = false )
    {
        $jsonInfo = base64_decode($extraArgs[0]);

        if ( DashNotifier::canNotify($this->currInstall->getPath()) ) {
            DashNotifier::prepareUserInstall();

            if ( DashNotifier::doNotify($jsonInfo) ) {
                Logger::uiSuccess('Notfied Successfully');
                $this->massIncr = 'SUCC';
                $ret = UserCommand::EXIT_SUCC;
            }
            else {
                Logger::uiError('Failed to Notify');
                $this->massIncr = 'FAIL';
                $ret = UserCommand::EXIT_FAIL;
            }
        }
        else {
            $this->massIncr = 'BYPASS';
        }

        return $ret;
    }

    /**
     *
     * @param array  $extraArgs
     * @return int
     */
    public function massDashNotify( $extraArgs )
    {
        $ret = $this->dashNotify($extraArgs, true);

        if ( $this->massIncr != '' ) {
            $this->outputResult('MASS_INCR', $this->massIncr);
        }

        return $ret;
    }

    public function dashDisable( $extraArgs, $massOp = false )
    {
        if ( MULTISITE ) {
            global $wpdb;

            $blogs = $wpdb->get_col("SELECT blog_id FROM {$wpdb->blogs};");

            foreach ( $blogs as $id ) {
                switch_to_blog($id);
                DashNotifier::deactivate(true);
                restore_current_blog();
            }
        }
        else {
            DashNotifier::deactivate(true);
        }

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
     * @param array  $extraArgs
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

    private function initWp()
    {
        /**
         * Declared for use in included files.
         */
        global $wpdb;

        error_reporting(E_ALL);

        /**
         * Attempt to override any WordPress memory limits.
         */
        define('WP_MEMORY_LIMIT', '512M');
        define('WP_MAX_MEMORY_LIMIT', '512M');

        /**
         * Only load core WordPress functionality.
         */
        define('SHORTINIT', true);

        $wpPath = $this->currInstall->getPath();

        /**
         * Set WP version data global variables, including $wp_version.
         */
        include_once "{$wpPath}/wp-includes/version.php";

        if ( version_compare($wp_version, '4.0', '<') ) {
            throw new LSCMException("Detected WordPress version as {$wp_version}. "
            . 'Version 4.0 required at minimum.');
        }

        /**
         * Set needed server variables.
         */
        $_SERVER['SCRIPT_FILENAME'] = "{$wpPath}/wp-admin/plugins.php";

        $config_content =
                file_get_contents($this->currInstall->getWpConfigFile());
        $uri = '';

        if ( preg_match('/define\(\s*[\'"]MULTISITE[\'"]\s*,\s*true\s*\)\s*;/', $config_content) ) {

            if ( !preg_match('/define\(\s*[\'"]DOMAIN_CURRENT_SITE[\'"]\s*,\s*[\'"](.+)[\'"]\s*\)\s*;/',
                    $config_content, $m) ) {

                throw new LSCMException(
                'Cannot find DOMAIN_CURRENT_SITE with MULTISITE defined.');
            }

            $this->currInstall->setServerName($m[1]);

            if ( !preg_match('/define\(\s*[\'"]PATH_CURRENT_SITE[\'"]\s*,\s*[\'"](.+)[\'"]\s*\)\s*;/',
                    $config_content, $m2) ) {

                throw new LSCMException(
                'Cannot find PATH_CURRENT_SITE with MULTISITE defined.');
            }

            $uri = $m2[1];

            define('WP_NETWORK_ADMIN', true);
        }

        $_SERVER['REQUEST_URI'] = $uri;
        putenv("REQUEST_URI={$uri}");

        /**
         * Set for LSCWP v1.1.5.1+ plugin logic.
         */
        if ( $docRoot = $this->currInstall->getDocRoot() ) {

            /**
             * For enable/disable.
             */
            $_SERVER['DOCUMENT_ROOT'] = $docRoot;
            putenv("DOCUMENT_ROOT={$docRoot}");
        }

        if ( $serverName = $this->currInstall->getServerName() ) {

            /**
             * For security plugins.
             */
            $_SERVER['HTTP_HOST'] = $serverName;
            putenv("HTTP_HOST={$serverName}");
        }
        else {
            $_SERVER['HTTP_HOST'] = self::LSCWP_HTTP_HOST_TEST;
            putenv('HTTP_HOST=' . self::LSCWP_HTTP_HOST_TEST);
        }

        $includeFiles = array(
            '/wp-load.php',
            '/wp-admin/includes/plugin.php',
            '/wp-includes/l10n.php',
            '/wp-admin/includes/file.php',
            '/wp-admin/includes/class-wp-upgrader.php',
            '/wp-admin/includes/misc.php',
            '/wp-includes/formatting.php',
            '/wp-includes/theme.php',
            '/wp-includes/link-template.php',
            '/wp-includes/class-wp-theme.php',
            '/wp-includes/kses.php',
            '/wp-includes/cron.php',
            '/wp-includes/pluggable.php',
            '/wp-includes/http.php',
            '/wp-includes/class-http.php',
            '/wp-includes/general-template.php',
            '/wp-includes/ms-functions.php',
            '/wp-includes/ms-deprecated.php',
            '/wp-includes/shortcodes.php',
            '/wp-includes/user.php',
            '/wp-includes/capabilities.php',
            '/wp-includes/default-constants.php',
            '/wp-includes/meta.php',
            '/wp-includes/update.php',
            '/wp-includes/query.php',
            '/wp-includes/post.php'
        );

        /**
         * The following version specific includes may fail on RC releases.
         */
        if ( version_compare($wp_version, '4.4.0', '>=') ) {
            $includeFiles[] = '/wp-includes/class-wp-user.php';
            $includeFiles[] = '/wp-includes/rest-api.php';
            $includeFiles[] = '/wp-includes/class-wp-http-encoding.php';
            $includeFiles[] = '/wp-includes/class-wp-http-proxy.php';
            $includeFiles[] = '/wp-includes/class-wp-http-response.php';
            $includeFiles[] = '/wp-includes/class-wp-http-curl.php';
            $includeFiles[] = '/wp-includes/class-wp-http-cookie.php';
        }

        if ( version_compare($wp_version, '4.6.0', '>=') ) {
            $includeFiles[] = '/wp-includes/class-wp-http-requests-response.php';
        }

        if ( version_compare($wp_version, '4.7.0', '>=') ) {
            $includeFiles[] = '/wp-includes/class-wp-http-requests-hooks.php';

            /**
             * Content contained in /wp-includes/query.php for earlier versions.
             */
            $includeFiles[] = '/wp-includes/class-wp-query.php';
        }

        set_error_handler('\Lsc\Wp\WPCaller::warning_handler');

        /**
         * Force WP to use PHP I/O file handling.
         */
        define('FS_METHOD', 'direct');

        /**
         * Trigger an early return from WP Rocket advanced-cache.php to prevent
         * WP Rocket from serving a cached copy and killing the process. This
         * occurs when calling an action from our control panel plugins and is
         * fixed by more closely matching the environment of a direct cli call.
         */
        unset($_SERVER['REQUEST_METHOD']);

        foreach ( $includeFiles as $file ) {
            $file = $wpPath . $file;

            if ( !file_exists($file) ) {
                throw new LSCMException("Could not include missing file {$file}.");
            }

            include_once $file;
        }

        restore_error_handler();

        /**
         * Needs to be defined after including files.
         */
        define('WP_ADMIN', true);

        /**
         * Define common WP constants and set 'wp_plugin_paths' array.
         */
        wp_plugin_directory_constants();

        /**
         * Do not load other plugins.
         */
        $GLOBALS['wp_plugin_paths'] = array();

        wp_cookie_constants();

        /**
         * Create global wp_query (WordPress) object entry. Needed during
         * LSCWP uninstall.
         */
        $GLOBALS['wp_the_query'] = new \WP_Query();
        $GLOBALS['wp_query'] = $GLOBALS['wp_the_query'];

        $this->pluginEntry = WP_PLUGIN_DIR . '/' . self::LSCWP_PLUGIN;

        if ( $this->loadLscwp && file_exists($this->pluginEntry) ) {
            include $this->pluginEntry;
            $this->includeLSCWPAdvancedCacheFile();
        }
    }

}
