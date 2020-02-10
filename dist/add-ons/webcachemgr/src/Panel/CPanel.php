<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2017-2020
 * ******************************************* */

namespace Lsc\Wp\Panel;

use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Util;
use \Lsc\Wp\WPInstall;

class CPanel extends ControlPanel
{

    const USER_PLUGIN_INSTALL_SCRIPT =
            '/usr/local/cpanel/whostmgr/docroot/cgi/lsws/res/ls_web_cache_mgr/install.sh';
    const USER_PLUGIN_UNINSTALL_SCRIPT =
            '/usr/local/cpanel/base/frontend/paper_lantern/ls_web_cache_manager/uninstall.sh';
    const USER_PLUGIN_CONF =
            '/usr/local/cpanel/base/frontend/paper_lantern/ls_web_cache_manager/lswcm.conf';
    const CPANEL_AUTOINSTALL_DISABLE_FLAG =
            '/usr/local/cpanel/whostmgr/docroot/cgi/lsws/cpanel_autoinstall_off';
    const USER_PLUGIN_SETTING_VHOST_CACHE_ROOT = 'vhost_cache_root';
    const USER_PLUGIN_SETTING_LSWS_DIR = 'lsws_dir';

    /**
     * @var bool
     */
    protected $isEA4;

    protected function __construct()
    {
        $this->panelName = 'cPanel/WHM';
        $this->defaultSvrCacheRoot = '/home/lscache/';
        $this->isEA4 = file_exists('/etc/cpanel/ea4/is_ea4');
        parent::__construct();
    }

    protected function initConfPaths()
    {
        if ( $this->isEA4 ) {
            $this->apacheConf = '/etc/apache2/conf.d/includes/pre_main_global.conf';
            $this->apacheVHConf = '/etc/apache2/conf.d/userdata/lscache_vhosts.conf';
        }
        else {
            $this->apacheConf = '/usr/local/apache/conf/includes/pre_main_global.conf';
            $this->apacheVHConf = '/usr/local/apache/conf/userdata/lscache_vhosts.conf';
        }
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
     * @param string   $file_contents
     * @return string
     */
    protected function addVHCacheRootSection( $file_contents,
            $vhCacheRoot = 'lscache' )
    {
        array_unshift($file_contents,
                "<IfModule LiteSpeed>\nCacheRoot {$vhCacheRoot}\n</IfModule>\n\n");
        $modified_contents = $file_contents;

        return $modified_contents;
    }

    /**
     *
     * @param string  $vhConf
     * @throws LSCMException
     */
    public function createVHConfAndSetCacheRoot( $vhConf,
            $vhCacheRoot = 'lscache' )
    {
        $vhConfDir = dirname($vhConf);

        if ( !file_exists($vhConfDir) ) {

            if ( !mkdir($vhConfDir, 0755) ) {
                throw new LSCMException("Failed to create directory {$vhConfDir}.");
            }

            $this->log("Created directory {$vhConfDir}", Logger::L_DEBUG);
        }

        $content = "<IfModule Litespeed>\nCacheRoot {$vhCacheRoot}\n</IfModule>";

        if ( false === file_put_contents($vhConf, $content) ) {
            throw new LSCMException("Failed to create file {$vhConf}.");
        }

        $this->log("Created file {$vhConf}.", Logger::L_DEBUG);
    }

    public function applyVHConfChanges()
    {
        exec('/scripts/ensure_vhost_includes --all-users');

        $this->UpdateCpanelPluginConf(
                self::USER_PLUGIN_SETTING_VHOST_CACHE_ROOT,
                $this->vhCacheRoot);
    }

    /**
     * Gets a list of found docroots and associated server names.
     * Only needed for scan.
     */
    protected function prepareDocrootMap()
    {
        $cmd = 'grep -hro --exclude="cache" --exclude="main" '
                . '--exclude="*.cache" "documentroot.*\|serveralias.*\|servername.*" /var/cpanel/userdata/*';
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
                Logger::debug("Unused line when preparing docroot map: {$line}.");
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

        $this->docRootMap = array( 'docroots' => $roots, 'names' => $servernames );
    }

    /**
     *
     * @param WPInstall  $wpInstall
     * @return string
     */
    public function getPhpBinary( WPInstall $wpInstall )
    {
        /**
         * Default PHP always works in EA3 as CloudLinux PHP Selector changes
         * this binary.
         */
        $phpBin = 'php';


        if ( $this->isEA4 ) {
            /**
             * cPanel php wrapper should accurately detect the correct binary in
             * EA4 when EA4 only directive '--ea-reference-dir' is provided.
             */
            $phpBin =
                    "/usr/local/bin/php --ea-reference-dir={$wpInstall->getPath()}/wp-admin";
        }

        return "{$phpBin} {$this->phpOptions}";
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
     * @return boolean
     * @throws LSCMException
     */
    public function installCpanelPlugin()
    {
        if ( !file_exists(self::USER_PLUGIN_INSTALL_SCRIPT) ) {
            throw new LSCMException('Unable to find cPanel user-end plugin installation script.'
                    . ' Please ensure that the LiteSpeed WHM plugin is already installed.');
        }

        $cpanelPluginDir = '/usr/local/cpanel/base/frontend/paper_lantern/ls_web_cache_manager';
        $cpanelPluginConfFile = "{$cpanelPluginDir}/lswcm.conf";
        $cpanelPluginTplDir = "{$cpanelPluginDir}/landing";
        $cpanelPluginCustTransDir = "{$cpanelPluginDir}/lang/cust";

        $existingInstall = false;

        if ( file_exists($cpanelPluginConfFile) ) {
            $existingInstall = true;

            $tmpCpanelDir = '/tmp/lscp-plugin-tmp';
            $tmpCpanelPluginConfFile = "{$tmpCpanelDir}/lswcm.conf";
            $tmpCpanelPluginTplDir = "{$tmpCpanelDir}/landing";
            $tmpCpanelPluginCustTransDir = "{$tmpCpanelDir}/cust";

            if ( file_exists($tmpCpanelDir) ) {
                Util::rrmdir($tmpCpanelDir);
            }

            if ( !mkdir($tmpCpanelDir, 0755) ) {
                throw new LSCMException(
                        "Failed to make temporary directory {$tmpCpanelDir}");
            }

            /**
             * Move existing conf file, templates, and custom translations to
             * temp directory and remove default template dir to prevent
             * overwrite when moving back.
             */
            $commands =
                    "/bin/mv {$cpanelPluginConfFile} {$tmpCpanelPluginConfFile}; "
                    . "/bin/mv {$cpanelPluginTplDir} {$tmpCpanelPluginTplDir}; "
                    . "/bin/rm -rf {$tmpCpanelPluginTplDir}/default; "
                    . "/bin/mv {$cpanelPluginCustTransDir} {$tmpCpanelPluginCustTransDir}; "
                    . "/bin/rm -rf {$tmpCpanelPluginCustTransDir}/README";

            exec($commands);
        }

        exec(self::USER_PLUGIN_INSTALL_SCRIPT);

        if ( $existingInstall ) {
            $cpanelPluginLangDir = dirname($cpanelPluginCustTransDir);

            if ( !file_exists($cpanelPluginLangDir) ) {
                mkdir($cpanelPluginLangDir, 0755);
            }

            /**
             * Replace cPanel plugin conf file, templates, and custom
             * translations and remove temp directory.
             */
            $commands =
                    "/bin/mv -f {$tmpCpanelPluginConfFile} {$cpanelPluginConfFile}; "
                    . "/bin/cp -prf {$tmpCpanelPluginTplDir} {$cpanelPluginDir}; "
                    . "/bin/cp -prf {$tmpCpanelPluginCustTransDir} {$cpanelPluginCustTransDir}; "
                    . "/bin/rm -rf {$tmpCpanelDir}";

            exec($commands);
        }
        else {
            self::turnOnCpanelPluginAutoInstall();
        }

        return ($existingInstall) ? 'update' : 'new';
    }

    public function uninstallCpanelPlugin()
    {
        if ( !file_exists(self::USER_PLUGIN_UNINSTALL_SCRIPT) ) {
            throw new LSCMException('Unable to find cPanel user-end plugin uninstallation script.'
                    . ' Plugin may already be uninstalled.');
        }

        exec(self::USER_PLUGIN_UNINSTALL_SCRIPT);

        self::turnOffCpanelPluginAutoInstall();
    }

    /**
     *
     * @param string  $setting
     * @param mixed   $value
     */
    public function UpdateCpanelPluginConf( $setting, $value )
    {
        if ( file_exists(self::USER_PLUGIN_CONF) ) {

            switch( $setting ) {

                case self::USER_PLUGIN_SETTING_LSWS_DIR:
                    $pattern = '/LSWS_HOME_DIR = ".*"/';
                    $replacement = "LSWS_HOME_DIR = \"{$value}\"";
                    break;

                case self::USER_PLUGIN_SETTING_VHOST_CACHE_ROOT:
                    $pattern = '/VHOST_CACHE_ROOT = ".*"/';
                    $replacement = "VHOST_CACHE_ROOT = \"{$value}\"";
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
                file_put_contents(self::USER_PLUGIN_CONF, $replacement,
                        FILE_APPEND);
            }
        }
    }

}
