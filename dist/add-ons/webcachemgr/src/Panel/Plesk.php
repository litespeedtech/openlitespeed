<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2023
 * ******************************************* */

namespace Lsc\Wp\Panel;

use Lsc\Wp\Logger;
use Lsc\Wp\LSCMException;
use Lsc\Wp\Util;
use Lsc\Wp\WPInstall;

class Plesk extends ControlPanel
{

    /**
     *
     * @throws LSCMException Thrown indirectly by parent::__construct() call.
     */
    public function __construct()
    {
        parent::__construct();
    }

    /**
     *
     * @since 1.13.2
     *
     * @throws LSCMException  Thrown indirectly by parent::init2() call.
     */
    protected function init2()
    {
        $this->panelName = 'Plesk';
        $this->defaultSvrCacheRoot = '/var/www/vhosts/lscache/';
        parent::init2();
    }

    /**
     * More reliable than php_uname('s')
     *
     * @return string
     *
     * @throws LSCMException  Thrown when supported Plesk OS detection command
     *     fails.
     * @throws LSCMException  Thrown when supported OS is not detected.
     */
    public function getPleskOS()
    {
        $supportedOsList = array(
            'centos',
            'virtuozzo',
            'cloudlinux',
            'redhat',
            'rhel',
            'ubuntu',
            'debian',
            'almalinux',
            'rocky'
        );

        $cmds = array();

        if ( file_exists('/etc/debian_version') ) {
            return 'debian';
        }

        if ( is_readable('/etc/os-release') ) {
            $cmds[] = 'grep ^ID= /etc/os-release | cut -d "=" -f2 | xargs';
        }

        if ( is_readable('/etc/lsb-release') ) {
            $cmds[] =
                'grep ^DISTRIB_ID= /etc/lsb-release | cut -d "=" -f2 | xargs';
        }

        if ( is_readable('/etc/redhat-release') ) {
            $cmds[] = 'cat /etc/redhat-release | awk \'{print $1}\'';
        }

        foreach ( $cmds as $cmd ) {

            if ( !($output = shell_exec($cmd)) ) {
                throw new LSCMException(
                    'Supported Plesk OS detection command failed.',
                    LSCMException::E_UNSUPPORTED
                );
            }

            $OS = trim($output);

            foreach ( $supportedOsList as $supportedOs ) {

                if ( stripos($OS, $supportedOs) !== false ) {
                    return $supportedOs;
                }
            }
        }

        throw new LSCMException(
            'Plesk detected with unsupported OS. '
                . '(Not CentOS/Virtuozzo/Cloudlinux/RedHat/Ubuntu/Debian/'
                . 'AlmaLinux/Rocky)',
            LSCMException::E_UNSUPPORTED
        );
    }

    /**
     *
     * @since 1.13.3
     *
     * @return string
     */
    protected function getVhDir()
    {
        $vhDir = '/var/www/vhosts';

        $psaConfFile = '/etc/psa/psa.conf';

        if ( file_exists($psaConfFile) ) {
            $ret = preg_match(
                '/HTTPD_VHOSTS_D\s+(\S+)/',
                file_get_contents($psaConfFile),
                $m
            );

            if ( $ret == 1 ) {
                $vhDir = $m[1];
            }
        }

        return $vhDir;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->getPleskOS() call.
     */
    protected function initConfPaths()
    {
        $OS = $this->getPleskOS();

        switch ( $OS ) {

            case 'centos':
            case 'virtuozzo':
            case 'cloudlinux':
            case 'redhat':
            case 'rhel':
            case 'almalinux':
            case 'rocky':
                $this->apacheConf = '/etc/httpd/conf.d/lscache.conf';
                break;

            case 'ubuntu':
                $this->apacheConf = '/etc/apache2/conf-enabled/lscache.conf';
                break;

            case 'debian':

                if ( is_dir('/etc/apache2/conf-enabled') ) {
                    $this->apacheConf =
                        '/etc/apache2/conf-enabled/lscache.conf';
                }
                else {
                    /**
                     * Old location.
                     */
                    $this->apacheConf = '/etc/apache2/conf.d/lscache.conf';
                }

                break;

            //no default case
        }

        $this->apacheVHConf = '/usr/local/psa/admin/conf/templates'
            . '/custom/domain/domainVirtualHost.php';
    }

    /**
     *
     * @return string
     */
    protected function serverCacheRootSearch()
    {
        $apacheConfDir = dirname($this->apacheConf);

        if ( file_exists($apacheConfDir) ) {
            return $this->cacheRootSearch($apacheConfDir);
        }

        return '';
    }

    /**
     *
     * @return string
     */
    protected function vhCacheRootSearch()
    {
        if ( file_exists($this->apacheVHConf) ) {
            return $this->getCacheRootSetting($this->apacheVHConf);
        }

        return '';
    }

    /**
     *
     * @param array  $file_contents
     * @param string $vhCacheRoot
     *
     * @return array
     */
    protected function addVHCacheRootSection(
        array $file_contents,
              $vhCacheRoot = 'lscache' )
    {
        return preg_replace(
            '!^\s*</VirtualHost>!im',
            "<IfModule Litespeed>\nCacheRoot $vhCacheRoot\n</IfModule>\n"
                . '</VirtualHost>',
            $file_contents
        );
    }

    /**
     *
     * @param string $vhConf
     * @param string $vhCacheRoot
     *
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     */
    public function createVHConfAndSetCacheRoot(
        $vhConf,
        $vhCacheRoot = 'lscache' )
    {
        $vhConfTmpl = '/usr/local/psa/admin/conf/templates/default/domain/'
            . 'domainVirtualHost.php';
        $vhConfDir = dirname($vhConf);

        if ( !file_exists($vhConfDir) ) {
            mkdir($vhConfDir, 0755, true);

            $this->log("Created directory $vhConfDir", Logger::L_DEBUG);
        }

        copy($vhConfTmpl, $vhConf);
        Util::matchPermissions($vhConfTmpl, $vhConf);

        $this->log(
            "Copied Virtual Host conf template file $vhConfTmpl to $vhConf",
            Logger::L_DEBUG
        );

        file_put_contents(
            $vhConf,
            preg_replace(
                '!^\s*</VirtualHost>!im',
                "<IfModule Litespeed>\nCacheRoot $vhCacheRoot\n</IfModule>"
                    . "\n</VirtualHost>",
                file($vhConf)
            )
        );

        $this->log(
            "Virtual Host cache root set to $vhCacheRoot",
            Logger::L_INFO
        );
    }

    public function applyVHConfChanges()
    {
        exec('/usr/local/psa/admin/bin/httpdmng --reconfigure-all');
    }

    /**
     * Gets a list of found docroots and associated server names.
     *
     * Note: This function is repeated in Plesk plugin files to avoid extra
     * serialize ops etc. This copy is for cli only.
     */
    protected function prepareDocrootMap()
    {
        exec(
            'grep -hro --exclude="stat_ttl.conf" --exclude="*.bak" '
                . '--exclude="last_httpd.conf" '
                . '"DocumentRoot.*\|ServerName.*\|ServerAlias.*" '
                . "{$this->getVhDir()}/system/*/conf/*",
            $lines
        );

        /**
         * [0]=servername, [1]=serveralias, [2]=serveralias, [3]=docroot, etc.
         * Not unique & not sorted.
         *
         * Example:
         * ServerName "pltest1.com:443"
         * ServerAlias "www.pltest1.com"
         * ServerAlias "ipv4.pltest1.com"
         * DocumentRoot "/var/www/vhosts/pltest1.com/httpdocs"
         * ServerName "pltest1.com:80"
         * ServerAlias "www.pltest1.com"
         * ServerAlias "ipv4.pltest1.com"
         * DocumentRoot "/var/www/vhosts/pltest1.com/httpdocs"
         * ServerName "pltest2.com:443"
         * ServerAlias "www.pltest2.com"
         * ServerAlias "ipv4.pltest2.com"
         * DocumentRoot "/var/www/vhosts/pltest2.com/httpdocs"
         * ServerName "pltest2.com:80"
         * ServerAlias "www.pltest2.com"
         * ServerAlias "ipv4.pltest2.com"
         * DocumentRoot "/var/www/vhosts/pltest2.com/httpdocs"
         *
         * @noinspection SpellCheckingInspection
         */

        $x = 0;
        $names = $tmpDocrootMap = array();

        $lineCount = count($lines);

        while ( $x <  $lineCount ) {
            $matchFound =
                preg_match('/ServerName\s+"([^"]+)"/', $lines[$x], $m1);

            if ( !$matchFound ) {
                /**
                 * Invalid start of group, skip.
                 */
                $x++;
                continue;
            }

            $UrlInfo = parse_url(
                (preg_match('#^https?://#', $m1[1])) ? $m1[1] : "http://$m1[1]"
            );

            $names[] = $UrlInfo['host'];
            $x++;

            $pattern = '/ServerAlias\s+"([^"]+)"/';

            while ( isset($lines[$x])
                    && preg_match($pattern, $lines[$x], $m2) ) {

                $names[] = $m2[1];
                $x++;
            }

            $pattern = '/DocumentRoot\s+"([^"]+)"/';

            if ( isset($lines[$x])
                    && preg_match($pattern, $lines[$x], $m3) == 1
                    && is_dir($m3[1]) ) {

                $docroot = $m3[1];

                if ( !isset($tmpDocrootMap[$docroot]) ) {
                    $tmpDocrootMap[$docroot] = $names;
                }
                else {
                    $tmpDocrootMap[$docroot] =
                        array_merge($tmpDocrootMap[$docroot], $names);
                }

                $x++;
            }

            $names = array();
        }

        $index = 0;
        $roots = $serverNames = array();

        foreach ( $tmpDocrootMap as $docroot => $names ) {
            $roots[$index] = $docroot;

            $names = array_unique($names);

            foreach ( $names as $n ) {
                $serverNames[$n] = $index;
            }

            $index++;
        }

        $this->docRootMap =
            array( 'docroots' => $roots, 'names' => $serverNames );
    }

    /**
     * Check for known Plesk PHP binaries and return the newest available
     * version among them.
     *
     * @since 1.9.6
     *
     * @return string
     */
    protected function getDefaultPhpBinary()
    {
        $binaryList = array (
            '/opt/plesk/php/8.2/bin/php',
            '/opt/plesk/php/8.1/bin/php',
            '/opt/plesk/php/8.0/bin/php',
            '/opt/plesk/php/7.4/bin/php',
            '/opt/plesk/php/7.3/bin/php',
            '/opt/plesk/php/7.2/bin/php',
            '/opt/plesk/php/7.1/bin/php',
            '/opt/plesk/php/7.0/bin/php',
            '/opt/plesk/php/5.6/bin/php',
        );

        foreach ( $binaryList as $binary ) {

            if ( file_exists($binary)) {
                return $binary;
            }
        }

        return '';
    }

    /**
     *
     * @param WPInstall $wpInstall
     *
     * @return string
     */
    public function getPhpBinary( WPInstall $wpInstall )
    {
        $serverName = $wpInstall->getData(WPInstall::FLD_SERVERNAME);

        if ( $serverName != null ) {
            $output = shell_exec(
                'plesk db -Ne "SELECT s.value '
                    . 'FROM ((domains d INNER JOIN hosting h ON h.dom_id=d.id) '
                    . 'INNER JOIN ServiceNodeEnvironment s '
                    . 'ON h.php_handler_id=s.name) '
                    . 'WHERE d.name=' . escapeshellarg($serverName)
                    . ' AND s.section=\'phphandlers\'" '
                    . '| sed -n \'s:.*<clipath>\(.*\)</clipath>.*:\1:p\''
            );

            if ( $output ) {
                $binPath = trim($output);
            }
        }

        if ( !empty($binPath) ) {
            $phpBin = $binPath;
        }
        elseif ( ($defaultBinary = $this->getDefaultPHPBinary()) != '' ) {
            $phpBin = $defaultBinary;
        }
        else {
            $phpBin = 'php';
        }

        return "$phpBin $this->phpOptions";
    }

}
