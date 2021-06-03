<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2017-2021
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
     * @since 1.13.2
     * @var string
     */
    const USER_PLUGIN_DIR = '/usr/local/cpanel/base/frontend/paper_lantern/ls_web_cache_manager';

    /**
     * @var string
     */
    const USER_PLUGIN_UNINSTALL_SCRIPT = self::USER_PLUGIN_DIR . '/uninstall.sh';

    /**
     * @since 1.13.2
     * @var string
     */
    const USER_PLUGIN_BACKUP_DIR = '/tmp/lscp-plugin-tmp';

    /**
     * @since 1.13.5
     * @var string
     */
    const USER_PLUGIN_DATA_DIR = self::USER_PLUGIN_DIR . '/data';

    /**
     * @since 1.13.2
     * @var string  Old location for cPanel user-end plugin conf file.
     */
    const USER_PLUGIN_CONF_OLD = self::USER_PLUGIN_DIR . '/lswcm.conf';

    /**
     * @var string
     */
    const USER_PLUGIN_CONF = self::USER_PLUGIN_DATA_DIR . '/lswcm.conf';

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
     * @since 1.13.2
     * @var string
     */
    protected $cpanelPluginDataDir;

    /**
     * @since 1.13.2
     * @var string
     */
    protected $cpanelPluginTplDir;

    /**
     * @since 1.13.2
     * @var string
     */
    protected $cpanelPluginCustTransDir;

    /**
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
        $this->cpanelPluginDataDir = self::USER_PLUGIN_DATA_DIR;
        $this->cpanelPluginTplDir = self::USER_PLUGIN_DIR . '/landing';
        $this->cpanelPluginCustTransDir = self::USER_PLUGIN_DIR . '/lang/cust';
        $this->tmpCpanelPluginDataDir = self::USER_PLUGIN_BACKUP_DIR . '/data';
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

    public function applyVHConfChanges()
    {
        exec('/scripts/ensure_vhost_includes --all-users');

        if ( file_exists(self::USER_PLUGIN_DIR) ) {
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
     * @throws LSCMException  Thrown directly and indirectly.
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

        $existingInstall = true;

        if ( !file_exists(self::USER_PLUGIN_CONF)
                && !file_exists(self::USER_PLUGIN_CONF_OLD) ) {

            $existingInstall = false;
        }

        if ( $existingInstall ) {

            if ( self::backupCpanelPluginDataFiles() == false ) {
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
     * @since ???       Removed optional param $oldLogic.
     *
     * @return bool
     * @throws LSCMException
     */
    protected static function backupCpanelPluginDataFiles()
    {
        if ( !file_exists(self::USER_PLUGIN_DIR) ) {
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

        $cpanelPluginDataDir = self::USER_PLUGIN_DATA_DIR;
        $cpanelPluginTplDir = self::USER_PLUGIN_DIR . '/landing';
        $cpanelPluginCustTransDir = self::USER_PLUGIN_DIR . '/lang/cust';

        $tmpCpanelPluginTplDir = self::USER_PLUGIN_BACKUP_DIR . '/landing';
        $tmpCpanelPluginCustTransDir = self::USER_PLUGIN_BACKUP_DIR . '/cust';

        /**
         * Move existing conf file, templates, and custom translations to
         * temp directory and remove default template dir to prevent
         * overwrite when moving back.
         */

        if ( file_exists(self::USER_PLUGIN_CONF) ) {
            $backupCmds = "/bin/mv $cpanelPluginDataDir "
                    . self::USER_PLUGIN_BACKUP_DIR . '/;';
        }
        elseif ( file_exists(self::USER_PLUGIN_CONF_OLD) ) {
            mkdir(self::USER_PLUGIN_BACKUP_DIR . '/data');

            $backupCmds = '/bin/mv ' . self::USER_PLUGIN_CONF_OLD . " "
                . self::USER_PLUGIN_BACKUP_DIR . '/data/;';
        }
        else {
            return false;
        }

        $backupCmds .= ' /bin/mv '
            . "$cpanelPluginTplDir " . self::USER_PLUGIN_BACKUP_DIR . '/; '
            . "/bin/rm -rf $tmpCpanelPluginTplDir/default; "
            . '/bin/mv '
            . "$cpanelPluginCustTransDir $tmpCpanelPluginCustTransDir; "
            . "/bin/rm -rf $tmpCpanelPluginCustTransDir/README";

        exec($backupCmds);

        return true;
    }

    /**
     *
     * @since 1.13.2
     * @since 1.13.2.2  Made function static.
     * @since ???       Removed optional param $oldLogic.
     *
     * @return bool
     */
    protected static function restoreCpanelPluginDataFiles()
    {
        if ( !file_exists(self::USER_PLUGIN_BACKUP_DIR)
                || !file_exists(self::USER_PLUGIN_DIR) ) {

            return false;
        }

        $tmpCpanelPluginDataDir = self::USER_PLUGIN_BACKUP_DIR . '/data';
        $tmpCpanelPluginTplDir = self::USER_PLUGIN_BACKUP_DIR . '/landing';
        $tmpCpanelPluginCustTransDir = self::USER_PLUGIN_BACKUP_DIR . '/cust';

        $cpanelPluginLangDir = self::USER_PLUGIN_DIR . '/lang';

        if ( !file_exists($cpanelPluginLangDir) ) {
            mkdir($cpanelPluginLangDir, 0755);
        }

        if ( !file_exists(self::USER_PLUGIN_DATA_DIR) ) {
            /**
             * Cover old conf file location
             */
            copy(
                "$tmpCpanelPluginDataDir/lswcm.conf",
                self::USER_PLUGIN_DIR . '/lswcm.conf'
            );
        }

        $restoreCmds = '/bin/cp -prf '
            . "$tmpCpanelPluginDataDir " . self::USER_PLUGIN_DIR . "/;";

        /**
         * Replace cPanel plugin conf file, templates, and custom
         * translations and remove temp directory.
         */
        $restoreCmds .=
            " /bin/cp -prf "
            . "$tmpCpanelPluginTplDir " . self::USER_PLUGIN_DIR . "/; "
            . "/bin/cp -prf "
            . "$tmpCpanelPluginCustTransDir $cpanelPluginLangDir/; "
            . '/bin/rm -rf ' . self::USER_PLUGIN_BACKUP_DIR;

        exec($restoreCmds);

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
        if ( !file_exists(self::USER_PLUGIN_UNINSTALL_SCRIPT) ) {
            throw new LSCMException(
                'Unable to find cPanel user-end plugin uninstallation script.'
                    . ' Plugin may already be uninstalled.'
            );
        }

        exec(self::USER_PLUGIN_UNINSTALL_SCRIPT);

        self::turnOffCpanelPluginAutoInstall();
    }

    /**
     * Attempt to update core cPanel plugin settings used for basic plugin
     * operation to the currently discovered values.
     *
     * @since 1.13.2.2
     * @since 1.13.5  Changed function visibility to public.
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
     */
    public static function UpdateCpanelPluginConf( $setting, $value )
    {
        if ( !file_exists(self::USER_PLUGIN_CONF)
                && file_exists(self::USER_PLUGIN_CONF_OLD) ) {

            if ( !file_exists(self::USER_PLUGIN_DATA_DIR) ) {
                mkdir(self::USER_PLUGIN_DATA_DIR);
            }

            copy(self::USER_PLUGIN_CONF_OLD, self::USER_PLUGIN_CONF);
        }

        if ( file_exists(self::USER_PLUGIN_CONF) ) {

            switch( $setting ) {

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

            $content = file_get_contents(self::USER_PLUGIN_CONF);

            if ( preg_match($pattern, $content) ) {
                $content = preg_replace($pattern, $replacement, $content);

                file_put_contents(self::USER_PLUGIN_CONF, $content);
            }
            else {
                file_put_contents(self::USER_PLUGIN_CONF, $replacement, FILE_APPEND);
            }
        }
    }

}
