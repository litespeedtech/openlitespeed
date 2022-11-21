<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2017-2022
 * ******************************************* */

namespace Lsc\Wp\Panel;

use Lsc\Wp\Logger;
use Lsc\Wp\LSCMException;
use Lsc\Wp\Util;
use Lsc\Wp\WPInstall;

class CPanel extends ControlPanel
{

    /**
     * @var string
     */
    const USER_PLUGIN_INSTALL_SCRIPT = '/usr/local/cpanel/whostmgr/docroot/cgi/lsws/res/ls_web_cache_mgr/install.sh';

    /**
     * @deprecated 1.13.11  Split into paper_lantern and jupiter theme specific
     *     constants.
     * @since 1.13.2
     * @var string
     */
    const USER_PLUGIN_DIR = '/usr/local/cpanel/base/frontend/paper_lantern/ls_web_cache_manager';

    /**
     * @since 1.13.11
     * @var string
     */
    const THEME_JUPITER_USER_PLUGIN_DIR = '/usr/local/cpanel/base/frontend/jupiter/ls_web_cache_manager';

    /**
     * @since 1.13.11
     * @var string
     */
    const THEME_PAPER_LANTERN_USER_PLUGIN_DIR = '/usr/local/cpanel/base/frontend/paper_lantern/ls_web_cache_manager';

    /**
     * @deprecated 1.13.11 No longer used.
     * @var string
     */
    const USER_PLUGIN_UNINSTALL_SCRIPT = self::USER_PLUGIN_DIR . '/uninstall.sh';

    /**
     * @since 1.13.11
     * @var string
     */
    const USER_PLUGIN_RELATIVE_UNINSTALL_SCRIPT = 'uninstall.sh';

    /**
     * @since 1.13.2
     * @var string
     */
    const USER_PLUGIN_BACKUP_DIR = '/tmp/lscp-plugin-tmp';

    /**
     * @since 1.13.11
     * @var string
     */
    const USER_PLUGIN_RELATIVE_DATA_DIR = 'data';

    /**
     * @deprecated 1.13.11
     * @since 1.13.2
     * @var string  An old location for cPanel user-end plugin conf file.
     */
    const USER_PLUGIN_CONF_OLD = self::USER_PLUGIN_DIR . '/lswcm.conf';

    /**
     * @since 1.13.11
     * @var string
     */
    const USER_PLUGIN_RELATIVE_CONF_OLD = '/lswcm.conf';

    /**
     * @since 1.13.11
     * @var string
     */
    const USER_PLUGIN_RELATIVE_CONF_OLD_2 = self::USER_PLUGIN_RELATIVE_DATA_DIR . '/lswcm.conf';

    /**
     * @var string
     */
    const USER_PLUGIN_CONF = '/usr/local/cpanel/3rdparty/ls_webcache_mgr/lswcm.conf';

    /**
     * @var string
     */
    const CPANEL_AUTOINSTALL_DISABLE_FLAG = '/usr/local/cpanel/whostmgr/docroot/cgi/lsws/cpanel_autoinstall_off';

    /**
     * @var string
     */
    const USER_PLUGIN_SETTING_VHOST_CACHE_ROOT = 'vhost_cache_root';

    /**
     * @var string
     */
    const USER_PLUGIN_SETTING_LSWS_DIR = 'lsws_dir';

    /**
     * @deprecated 1.13.11  Never used.
     * @since 1.13.2
     * @var string
     */
    protected $cpanelPluginDataDir;

    /**
     * @deprecated 1.13.11  Never used.
     * @since 1.13.2
     * @var string
     */
    protected $cpanelPluginTplDir;

    /**
     * @deprecated 1.13.11  Never used.
     * @since 1.13.2
     * @var string
     */
    protected $cpanelPluginCustTransDir;

    /**
     * @deprecated 1.13.11  Never used.
     * @since 1.13.2
     * @var string
     */
    protected $tmpCpanelPluginDataDir;

    /**
     * @since 1.13.2
     * @var string
     */
    protected $tmpCpanelPluginTplDir;

    /**
     * @since 1.13.2
     * @var string
     */
    protected $tmpCpanelPluginCustTransDir;

    protected function __construct()
    {
        /** @noinspection PhpUnhandledExceptionInspection */
        parent::__construct();
    }

    /**
     *
     * @since 1.13.2
     */
    protected function init2()
    {
        $this->panelName = 'cPanel/WHM';
        $this->defaultSvrCacheRoot = '/home/lscache/';
        $this->tmpCpanelPluginTplDir =
            self::USER_PLUGIN_BACKUP_DIR . '/landing';
        $this->tmpCpanelPluginCustTransDir =
            self::USER_PLUGIN_BACKUP_DIR . '/cust';

        /** @noinspection PhpUnhandledExceptionInspection */
        parent::init2();
    }

    protected function initConfPaths()
    {
        $this->apacheConf = '/etc/apache2/conf.d/includes/pre_main_global.conf';
        $this->apacheVHConf = '/etc/apache2/conf.d/userdata/lscache_vhosts.conf';
    }

    /**
     *
     * @return string
     */
    protected function serverCacheRootSearch()
    {
        if ( file_exists($this->apacheConf) ) {
            return $this->getCacheRootSetting($this->apacheConf);
        }

        return '';
    }

    /**
     *
     * @return string
     */
    protected function vhCacheRootSearch()
    {
        $apacheUserdataDir = dirname($this->apacheVHConf);

        if ( file_exists($apacheUserdataDir) ) {
            return $this->cacheRootSearch($apacheUserdataDir);
        }

        return '';
    }

    /**
     *
     * @param array   $file_contents
     * @param string  $vhCacheRoot
     * @return array
     */
    protected function addVHCacheRootSection(
            $file_contents, $vhCacheRoot = 'lscache' )
    {
        array_unshift(
            $file_contents,
            "<IfModule LiteSpeed>\nCacheRoot $vhCacheRoot\n</IfModule>\n\n"
        );

        return $file_contents;
    }

    /**
     *
     * @param string  $vhConf
     * @param string  $vhCacheRoot
     * @throws LSCMException  Thrown directly and indirectly.
     */
    public function createVHConfAndSetCacheRoot(
            $vhConf, $vhCacheRoot = 'lscache' )
    {
        $vhConfDir = dirname($vhConf);

        if ( !file_exists($vhConfDir) ) {

            if ( !mkdir($vhConfDir, 0755) ) {
                throw new LSCMException(
                    "Failed to create directory $vhConfDir."
                );
            }

            $this->log("Created directory $vhConfDir", Logger::L_DEBUG);
        }

        $content =
            "<IfModule Litespeed>\nCacheRoot $vhCacheRoot\n</IfModule>";

        if ( false === file_put_contents($vhConf, $content) ) {
            throw new LSCMException("Failed to create file $vhConf.");
        }

        $this->log("Created file $vhConf.", Logger::L_DEBUG);
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by
     *     self::UpdateCpanelPluginConf() call.
     */
    public function applyVHConfChanges()
    {
        exec('/scripts/ensure_vhost_includes --all-users');

        if ( file_exists(self::THEME_JUPITER_USER_PLUGIN_DIR)
                || file_exists(self::THEME_PAPER_LANTERN_USER_PLUGIN_DIR) ) {

            self::UpdateCpanelPluginConf(
                    self::USER_PLUGIN_SETTING_VHOST_CACHE_ROOT,
                    $this->vhCacheRoot
            );
        }
    }

    /**
     * Gets a list of found docroots and associated server names.
     * Only needed for scan.
     *
     * @throws LSCMException  Thrown indirectly.
     */
    protected function prepareDocrootMap()
    {
        $cmd =
            'grep -hro --exclude="cache" --exclude="main" --exclude="*.cache" '
            . '"documentroot.*\|serveralias.*\|servername.*" '
            . '/var/cpanel/userdata/*';
        exec($cmd, $lines);

        /**
         * [0]=docroot, [1]=serveraliases, [2]=servername, [3]=docroot, etc.
         * Not unique & not sorted.
         *
         * Example:
         * documentroot: /home/user1/finches
         * serveralias: finches.com mail.finches.com www.finches.com www.finches.user1.com cpanel.finches.com autodiscover.finches.com whm.finches.com webmail.finches.com webdisk.finches.com
         * servername: finches.user1.com
         * documentroot: /home/user1/public_html/dookoo
         * serveralias: www.dookoo.user1.com
         * servername: dookoo.user1.com
         * documentroot: /home/user1/public_html/doo/doo2
         * serveralias: www.doo2.user1.com
         * servername: doo2.user1.com
         * documentroot: /home/user1/finches
         * serveralias: finches.com mail.finches.com www.finches.com www.finches.user1.com
         * servername: finches.user1.com
         */

        $cur = '';
        $docroots = array();

        foreach ( $lines as $line ) {

            if ( $cur == '' ) {

                if ( strpos($line, 'documentroot:') === 0 ) {
                    /**
                     * 13 is strlen('documentroot:')
                     */
                    $cur = trim(substr($line, 13));

                    if ( !isset($docroots[$cur]) ) {

                        if ( is_dir($cur) ) {
                            $docroots[$cur] = '';
                        }
                        else {
                            /**
                             * bad entry ignore
                             */
                            $cur = '';
                        }
                    }
                }
            }
            elseif ( strpos($line, 'serveralias:') === 0 ) {
                /**
                 * 12 is strlen('serveralias:')
                 */
                $docroots[$cur] .= substr($line, 12);
            }
            elseif ( strpos($line, 'servername:') === 0 ) {
                /**
                 * 11 is strlen('servername:')
                 */
                $docroots[$cur] .= substr($line, 11);

                /**
                 * looking for next docroot
                 */
                $cur = '';
            }
            else {
                Logger::debug(
                    "Unused line when preparing docroot map: $line."
                );
            }
        }

        $roots = array();
        $servernames = array();
        $index = 0;

        foreach ( $docroots as $docroot => $line ) {
            $names = preg_split('/\s+/', trim($line), -1, PREG_SPLIT_NO_EMPTY);
            $names = array_unique($names);
            $roots[$index] = $docroot;

            foreach ( $names as $n ) {
                $servernames[$n] = $index;
            }

            $index++;
        }

        $this->docRootMap =
            array( 'docroots' => $roots, 'names' => $servernames );
    }

    /**
     *
     * @param WPInstall  $wpInstall
     * @return string
     */
    public function getPhpBinary( WPInstall $wpInstall )
    {
        /**
         * cPanel php wrapper should accurately detect the correct binary in
         * EA4 when EA4 only directive '--ea-reference-dir' is provided.
         */
        $phpBin = '/usr/local/bin/php '
            . "--ea-reference-dir={$wpInstall->getPath()}/wp-admin";

        return "$phpBin $this->phpOptions";
    }

    /**
     *
     * @return boolean
     */
    public static function isCpanelPluginAutoInstallOn()
    {
        if ( file_exists(self::CPANEL_AUTOINSTALL_DISABLE_FLAG) ) {
            return false;
        }

        return true;
    }

    /**
     *
     * @return boolean
     */
    public static function turnOnCpanelPluginAutoInstall()
    {
        if ( !file_exists(self::CPANEL_AUTOINSTALL_DISABLE_FLAG) ) {
            return true;
        }

        return unlink(self::CPANEL_AUTOINSTALL_DISABLE_FLAG);
    }

    /**
     *
     * @return boolean
     */
    public static function turnOffCpanelPluginAutoInstall()
    {
        return touch(self::CPANEL_AUTOINSTALL_DISABLE_FLAG);
    }

    /**
     *
     * @return string
     *
     * @throws LSCMException  Thrown when unable to find cPanel user-end plugin
     *     installation script.
     * @throws LSCMException Thrown when failing to backup cPanel user-end
     *     plugin data files.
     * @throws LSCMException  Thrown indirectly by
     *     self::backupCpanelPluginDataFiles() call.
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     */
    public function installCpanelPlugin()
    {
        if ( !file_exists(self::USER_PLUGIN_INSTALL_SCRIPT) ) {
            throw new LSCMException(
                'Unable to find cPanel user-end plugin installation script.'
                    . ' Please ensure that the LiteSpeed WHM plugin is already '
                    . 'installed.'
            );
        }

        $existingInstall = (
            file_exists(self::THEME_JUPITER_USER_PLUGIN_DIR)
                || file_exists(self::THEME_PAPER_LANTERN_USER_PLUGIN_DIR)
        );

        if ( $existingInstall ) {

            if ( !self::backupCpanelPluginDataFiles() ) {
                throw new LSCMException(
                    'Failed to backup cPanel user-end plugin data files. '
                        . 'Aborting install/update operation.'
                );
            }

            exec(self::USER_PLUGIN_INSTALL_SCRIPT);

            if ( self::restoreCpanelPluginDataFiles() == false ) {
                Logger::error(
                    'Failed to restore cPanel user-end plugin data files.'
                );
            }
        }
        else {
            exec(self::USER_PLUGIN_INSTALL_SCRIPT);
            self::turnOnCpanelPluginAutoInstall();
        }

        $this->updateCoreCpanelPluginConfSettings();

        return ($existingInstall) ? 'update' : 'new';
    }

    /**
     *
     * @since 1.13.2
     * @since 1.13.2.2  Made function static.
     * @since 1.13.5.2  Removed optional param $oldLogic.
     *
     * @return bool
     *
     * @throws LSCMException  Thrown when failing to create a temporary backup
     *     directory.
     */
    protected static function backupCpanelPluginDataFiles()
    {
        if ( file_exists(self::THEME_JUPITER_USER_PLUGIN_DIR) ) {
            $pluginDir = self::THEME_JUPITER_USER_PLUGIN_DIR;
        }
        elseif ( file_exists(self::THEME_PAPER_LANTERN_USER_PLUGIN_DIR) ) {
            $pluginDir = self::THEME_PAPER_LANTERN_USER_PLUGIN_DIR;
        }
        else {
            return false;
        }

        if ( file_exists(self::USER_PLUGIN_BACKUP_DIR) ) {
            Util::rrmdir(self::USER_PLUGIN_BACKUP_DIR);
        }

        if ( !mkdir(self::USER_PLUGIN_BACKUP_DIR, 0755) ) {
            throw new LSCMException(
                'Failed to make temporary directory '
                    . self::USER_PLUGIN_BACKUP_DIR
            );
        }

        $cpanelPluginTplDir = "$pluginDir/landing";
        $cpanelPluginCustTransDir = "$pluginDir/lang/cust";

        $tmpCpanelPluginTplDir = self::USER_PLUGIN_BACKUP_DIR . '/landing';
        $tmpCpanelPluginCustTransDir = self::USER_PLUGIN_BACKUP_DIR . '/cust';

        /**
         * Move existing conf file (if needed), templates, and custom
         * translations to temp directory and remove default template dir to
         * prevent overwrite when moving back.
         */

        $backupCmds = '';

        $activeConfFile =
            self::getInstalledCpanelPluginActiveConfFileLocation($pluginDir);

        if ( $activeConfFile == '' || !file_exists($activeConfFile) ) {
            return false;
        }

        if ( $activeConfFile != self::USER_PLUGIN_CONF ) {
            $backupCmds .= "/bin/mv $activeConfFile "
                . self::USER_PLUGIN_BACKUP_DIR . '/;';
        }

        $backupCmds .= '/bin/mv '
            . "$cpanelPluginTplDir " . self::USER_PLUGIN_BACKUP_DIR . '/;'
            . "/bin/rm -rf $tmpCpanelPluginTplDir/default;"
            . '/bin/mv '
            . "$cpanelPluginCustTransDir $tmpCpanelPluginCustTransDir;"
            . "/bin/rm -rf $tmpCpanelPluginCustTransDir/README";

        exec($backupCmds);

        return true;
    }

    /**
     *
     * @since 1.13.2
     * @since 1.13.2.2  Made function static.
     * @since 1.13.5.2  Removed optional param $oldLogic.
     *
     * @return bool
     */
    protected static function restoreCpanelPluginDataFiles()
    {
        $pluginInstalls = array();

        if ( file_exists(self::THEME_JUPITER_USER_PLUGIN_DIR) ) {
            $pluginInstalls[] = self::THEME_JUPITER_USER_PLUGIN_DIR;
        }

        if ( file_exists(self::THEME_PAPER_LANTERN_USER_PLUGIN_DIR) ) {
            $pluginInstalls[] = self::THEME_PAPER_LANTERN_USER_PLUGIN_DIR;
        }


        if ( !file_exists(self::USER_PLUGIN_BACKUP_DIR)
                || empty($pluginInstalls) ) {

            return false;
        }

        $tmpCpanelPluginConfFile = self::USER_PLUGIN_BACKUP_DIR . '/lswcm.conf';
        $tmpCpanelPluginTplDir = self::USER_PLUGIN_BACKUP_DIR . '/landing';
        $tmpCpanelPluginCustTransDir = self::USER_PLUGIN_BACKUP_DIR . '/cust';

        foreach ( $pluginInstalls as $pluginInstall ) {

            $cpanelPluginLangDir = "$pluginInstall/lang";

            if (!file_exists($cpanelPluginLangDir)) {
                mkdir($cpanelPluginLangDir, 0755);
            }

            if ( file_exists($tmpCpanelPluginConfFile) ) {
                $activeConfFile = self::getInstalledCpanelPluginActiveConfFileLocation(
                    $pluginInstall
                );

                if ($activeConfFile == '') {
                    return false;
                }

                copy(
                    $tmpCpanelPluginConfFile,
                    $activeConfFile
                );
                chmod($activeConfFile, 0644);
            }

            /**
             * Replace cPanel plugin templates, custom translations.
             */
            $restoreCmds =
                " /bin/cp -prf "
                . "$tmpCpanelPluginTplDir " . "$pluginInstall/; "
                . "/bin/cp -prf "
                . "$tmpCpanelPluginCustTransDir $cpanelPluginLangDir/";

            exec($restoreCmds);
        }

        exec('/bin/rm -rf ' . self::USER_PLUGIN_BACKUP_DIR);
        return true;
    }

    /**
     *
     * @since 1.13.2.2  Made function static.
     *
     * @throws LSCMException
     */
    public static function uninstallCpanelPlugin()
    {
        $jupiterUninstallFile = self::THEME_JUPITER_USER_PLUGIN_DIR . '/'
            . self::USER_PLUGIN_RELATIVE_UNINSTALL_SCRIPT;
        $paperLanternUninstallFile = self::THEME_PAPER_LANTERN_USER_PLUGIN_DIR
            . '/' . self::USER_PLUGIN_RELATIVE_UNINSTALL_SCRIPT;

        if ( file_exists($jupiterUninstallFile) ) {
            $uninstallFile = $jupiterUninstallFile;
        }
        elseif ( file_exists($paperLanternUninstallFile) ) {
            $uninstallFile = $paperLanternUninstallFile;
        }
        else {
            throw new LSCMException(
                'Unable to find cPanel user-end plugin uninstallation script.'
                    . ' Plugin may already be uninstalled.'
            );
        }

        exec($uninstallFile);

        self::turnOffCpanelPluginAutoInstall();
    }

    /**
     * Attempt to update core cPanel plugin settings used for basic plugin
     * operation to the currently discovered values.
     *
     * @since 1.13.2.2
     * @since 1.13.5  Changed function visibility to public.
     *
     * @throws LSCMException  Thrown indirectly by
     *     self::UpdateCpanelPluginConf() call.
     * @throws LSCMException  Thrown indirectly by
     *     self::UpdateCpanelPluginConf() call.
     */
    public function updateCoreCpanelPluginConfSettings()
    {
        $lswsHome = realpath(__DIR__ . '/../../../..');

        self::UpdateCpanelPluginConf(
            self::USER_PLUGIN_SETTING_LSWS_DIR,
            $lswsHome
        );

        $vhCacheRoot = $this->getVHCacheRoot();

        self::UpdateCpanelPluginConf(
            self::USER_PLUGIN_SETTING_VHOST_CACHE_ROOT,
            $vhCacheRoot
        );
    }

    /**
     *
     * @since 1.13.2.2  Made function static.
     *
     * @param string  $setting
     * @param mixed   $value
     *
     * @throws LSCMException  Thrown when unable to determine active cPanel
     *     user-end plugin conf file location usually indicating that the
     *     cPanel user-end plugin is not currently installed.
     * @throws LSCMException  Thrown when unable to create cPanel user-end
     *     plugin "data" directory for older versions of the cPanel user-end
     *     plugin that require this directory.
     */
    public static function UpdateCpanelPluginConf( $setting, $value )
    {
        $pluginInstalls = array();

        if ( file_exists(self::THEME_JUPITER_USER_PLUGIN_DIR) ) {
            $pluginInstalls[] = self::THEME_JUPITER_USER_PLUGIN_DIR;
        }
        else {
            $pluginInstalls[] = self::THEME_PAPER_LANTERN_USER_PLUGIN_DIR;
        }

        foreach ( $pluginInstalls as $pluginInstall ) {
            $activeConfFile = self::getInstalledCpanelPluginActiveConfFileLocation(
                $pluginInstall
            );

            if ( $activeConfFile == '' ) {
                throw new LSCMException(
                    'Unable to determine active conf file location for cPanel '
                    . 'user-end plugin. cPanel user-end plugin is likely not '
                    . 'installed.'
                );
            }

            if ( !file_exists($activeConfFile) ) {
                $oldConf = '';

                if ( file_exists("$pluginInstall/" . self::USER_PLUGIN_RELATIVE_CONF_OLD_2) ) {
                    $oldConf = "$pluginInstall/" . self::USER_PLUGIN_RELATIVE_CONF_OLD_2;
                }
                elseif ( file_exists("$pluginInstall/" . self::USER_PLUGIN_RELATIVE_CONF_OLD) ) {
                    $oldConf = "$pluginInstall/" . self::USER_PLUGIN_RELATIVE_CONF_OLD;
                }

                if ( $oldConf != '' ) {
                    $dataDir =
                        "$pluginInstall/" . self::USER_PLUGIN_RELATIVE_DATA_DIR;

                    if ( $activeConfFile == "$pluginInstall/" . self::USER_PLUGIN_RELATIVE_CONF_OLD_2
                        && !file_exists($dataDir)
                        && !mkdir($dataDir)) {

                        throw new LSCMException(
                            "Failed to create directory $dataDir."
                        );
                    }

                    copy($oldConf, $activeConfFile);
                }
            }

            if ( file_exists($activeConfFile) ) {
                chmod($activeConfFile, 0644);

                switch ($setting) {

                    case self::USER_PLUGIN_SETTING_LSWS_DIR:
                        $pattern = '/LSWS_HOME_DIR = ".*"/';
                        $replacement = "LSWS_HOME_DIR = \"$value\"";
                        break;

                    case self::USER_PLUGIN_SETTING_VHOST_CACHE_ROOT:
                        $pattern = '/VHOST_CACHE_ROOT = ".*"/';
                        $replacement = "VHOST_CACHE_ROOT = \"$value\"";
                        break;

                    default:
                        return;
                }

                $content = file_get_contents($activeConfFile);

                if ( preg_match($pattern, $content) ) {
                    file_put_contents(
                        $activeConfFile,
                        preg_replace($pattern, $replacement, $content)
                    );
                }
                else {
                    file_put_contents(
                        $activeConfFile,
                        $replacement,
                        FILE_APPEND
                    );
                }
            }
        }
    }

    /**
     *
     * @since 1.13.11
     *
     * @return string
     */
    protected static function getInstalledCpanelPluginActiveConfFileLocation(
        $pluginDir )
    {
        $versionFile = "$pluginDir/VERSION";

        if ( file_exists($versionFile) ) {
            $versionNum = file_get_contents($versionFile);

            if ( Util::betterVersionCompare($versionNum, '2.1.2.2','>') ) {
                return self::USER_PLUGIN_CONF;
            }
        }

        if ( file_exists("$pluginDir/" . self::USER_PLUGIN_RELATIVE_DATA_DIR) ) {
            return "$pluginDir/" . self::USER_PLUGIN_RELATIVE_CONF_OLD_2;
        }

        if ( file_exists($pluginDir) ) {
            return "$pluginDir/" . self::USER_PLUGIN_RELATIVE_CONF_OLD;
        }

        return '';
    }

}
