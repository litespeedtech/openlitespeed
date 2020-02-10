<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2020
 * ******************************************* */

namespace Lsc\Wp\Panel;

use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Panel\ControlPanel;
use \Lsc\Wp\Util;
use \Lsc\Wp\WPInstall;

class Plesk extends ControlPanel
{

    public function __construct()
    {
        $this->panelName = 'Plesk';
        $this->defaultSvrCacheRoot = '/var/www/vhosts/lscache/';
        parent::__construct();
    }

    /**
     * More reliable than php_uname('s')
     *
     * @return string
     * @throws LSCMException
     */
    public function getPleskOS()
    {
        $supportedOS = array( 'centos', 'cloudlinux', 'redhat', 'ubuntu', 'debian' );
        $cmd = '';

        if ( is_readable('/etc/os-release') ) {
            $cmd = 'grep ^ID= /etc/os-release | cut -d"=" -f2 | xargs';
        }
        elseif ( is_readable('/etc/lsb-release') ) {
            $cmd = 'grep ^DISTRIB_ID= /etc/lsb-release | cut -d"=" -f2 | xargs';
        }
        elseif ( is_readable('/etc/redhat-release') ) {
            $cmd = 'cat /etc/redhat-release | awk \'{print $1}\'';
        }
        elseif ( file_exists('/etc/debian_version') ) {
            return 'debian';
        }

        if ( $cmd ) {
            $OS = strtolower(trim(shell_exec($cmd)));

            if ( in_array($OS, $supportedOS) ) {
                return $OS;
            }
        }

        throw new LSCMException(
                'Plesk detected with unsupported OS. (Not CentOS/Cloudlinux/RedHat/Ubuntu/Debian)',
                LSCMException::E_UNSUPPORTED);
    }

    protected function initConfPaths()
    {
        $OS = $this->getPleskOS();

        switch ($OS) {
            case 'centos':
            case 'cloudlinux':
            case 'redhat':
                $this->apacheConf = '/etc/httpd/conf.d/lscache.conf';
                break;

            case 'ubuntu':
                $this->apacheConf = '/etc/apache2/conf-enabled/lscache.conf';
                break;

            case 'debian':

                if ( is_dir('/etc/apache2/conf-enabled') ) {
                    $this->apacheConf = '/etc/apache2/conf-enabled/lscache.conf';
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

        $this->apacheVHConf =
                '/usr/local/psa/admin/conf/templates/custom/domain/domainVirtualHost.php';
    }

    protected function serverCacheRootSearch()
    {
        $apacheConfDir = dirname($this->apacheConf);

        if ( file_exists($apacheConfDir) ) {
            return $this->cacheRootSearch($apacheConfDir);
        }

        return '';
    }

    protected function vhCacheRootSearch()
    {
        if ( file_exists($this->apacheVHConf) ) {
            return $this->getCacheRootSetting($this->apacheVHConf);
        }

        return '';
    }

    protected function addVHCacheRootSection( $file_contents,
            $vhCacheRoot = 'lscache' )
    {
        $modified_contents = preg_replace('!^\s*</VirtualHost>!im',
                "<IfModule Litespeed>\nCacheRoot {$vhCacheRoot}\n</IfModule>\n</VirtualHost>",
                $file_contents);

        return $modified_contents;
    }

    public function createVHConfAndSetCacheRoot( $vhConf,
            $vhCacheRoot = 'lscache' )
    {
        $vhConfTmpl = '/usr/local/psa/admin/conf/templates/default/domain/domainVirtualHost.php';
        $vhConfDir = dirname($vhConf);

        if ( !file_exists($vhConfDir) ) {
            mkdir($vhConfDir, 0755, true);

            $this->log("Created directory {$vhConfDir}", Logger::L_DEBUG);
        }

        copy($vhConfTmpl, $vhConf);
        Util::matchPermissions($vhConfTmpl, $vhConf);

        $this->log("Copied Virtual Host conf template file {$vhConfTmpl} to {$vhConf}",
                Logger::L_DEBUG);

        $file_contents = file($vhConf);

        $replaced_content = preg_replace('!^\s*</VirtualHost>!im',
                "<IfModule Litespeed>\nCacheRoot {$vhCacheRoot}\n</IfModule>\n</VirtualHost>",
                $file_contents);

        file_put_contents($vhConf, $replaced_content);

        $this->log("Virutal Host cache root set to {$vhCacheRoot}", Logger::L_INFO);
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
     *
     * @return string[][]  Array of docroot keys and their server name(s).
     */
    protected function prepareDocrootMap()
    {
        $cmd = 'grep -hro --exclude="stat_ttl.conf" --exclude="*.bak" --exclude="last_httpd.conf" '
                . '"DocumentRoot.*\|ServerName.*\|ServerAlias.*" /var/www/vhosts/system/*/conf/*';
        exec( $cmd, $lines);

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
         */

        $x = 0;
        $names = $tmpDocrootMap = array();

        $lineCount = count($lines);

        while ( $x <  $lineCount ) {
            $pattern = '/ServerName\s+"([^"]+)"/';

            if ( preg_match($pattern, $lines[$x], $m1) != 1 ) {
                /**
                 * Invalid start of group, skip.
                 */
                $x++;
                continue;
            }

            $serverNameAsUrl =
                    (preg_match('#^https?://#', $m1[1])) ? $m1[1] : "http://{$m1[1]}";

            $UrlInfo = parse_url($serverNameAsUrl);
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

        $this->docRootMap = array( 'docroots' => $roots, 'names' => $serverNames );
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
     * @param WPInstall $wpInstall  Not used
     * @return string
     */
    public function getPhpBinary( WPInstall $wpInstall )
    {
        $phpBin = 'php';

        $serverName = $wpInstall->getData(WPInstall::FLD_SERVERNAME);

        if ( $serverName != null ) {
            $escapedServerName = escapeshellarg($serverName);

            $cmd = 'plesk db -Ne "SELECT s.value '
                    . 'FROM ((domains d INNER JOIN hosting h ON h.dom_id=d.id) '
                    . 'INNER JOIN ServiceNodeEnvironment s ON h.php_handler_id=s.name) '
                    . "WHERE d.name={$escapedServerName} AND s.section='phphandlers'\" "
                    . '| sed -n \'s:.*<clipath>\(.*\)</clipath>.*:\1:p\'';

            $binPath = trim(shell_exec($cmd));
        }

        if ( !empty($binPath) ) {
            $phpBin = $binPath;
        }
        elseif ( ($defaultBinary = $this->getDefaultPHPBinary()) != '' ) {
            $phpBin = $defaultBinary;
        }

        return "{$phpBin} {$this->phpOptions}";
    }

}
