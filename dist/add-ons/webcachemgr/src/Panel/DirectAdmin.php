<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2019-2020
 * ******************************************* */

namespace Lsc\Wp\Panel;

use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\WPInstall;

class DirectAdmin extends ControlPanel
{

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
        $this->panelName = 'DirectAdmin';
        $this->defaultSvrCacheRoot = '/home/lscache/';

        /** @noinspection PhpUnhandledExceptionInspection */
        parent::init2();
    }

    protected function initConfPaths()
    {
        $this->apacheConf = '/etc/httpd/conf/extra/httpd-includes.conf';
        $this->apacheVHConf = '/usr/local/directadmin/data/templates/custom/'
                . 'cust_httpd.CUSTOM.2.pre';
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
            return $this->daVhCacheRootSearch($apacheUserdataDir);
        }

        return '';
    }

    /**
     * Searches the given directories '.pre' and '.post' files for CacheRoot
     * setting.
     *
     * @param string  $confDir  Directory to be searched.
     * @return string
     */
    public function daVhCacheRootSearch( $confDir )
    {
        $files = new \DirectoryIterator($confDir);

        foreach ( $files as $file ) {
            $filename = $file->getFilename();

            if ( strlen($filename) > 4
                    && (substr_compare($filename, '.pre', -4) === 0
                        || substr_compare($filename, '.post', -5) === 0) ) {

                $cacheRoot = $this->getCacheRootSetting($file->getPathname());

                if ( $cacheRoot != '' ) {
                    return $cacheRoot;
                }
            }
        }

        return '';
    }

    /**
     *
     * @param array   $file_contents
     * @param string  $vhCacheRoot
     * @return array
     */
    protected function addVHCacheRootSection( $file_contents,
            $vhCacheRoot = 'lscache' )
    {
        array_unshift(
            $file_contents,
            "<IfModule LiteSpeed>\nCacheRoot {$vhCacheRoot}\n</IfModule>\n\n"
        );

        return $file_contents;
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
                throw new LSCMException(
                    "Failed to create directory {$vhConfDir}."
                );
            }

            $this->log("Created directory {$vhConfDir}", Logger::L_DEBUG);
        }

        $content =
            "<IfModule Litespeed>\nCacheRoot {$vhCacheRoot}\n</IfModule>";

        if ( false === file_put_contents($vhConf, $content) ) {
            throw new LSCMException("Failed to create file {$vhConf}.");
        }

        $this->log("Created file {$vhConf}.", Logger::L_DEBUG);
    }

    public function applyVHConfChanges()
    {
        exec('/usr/local/directadmin/custombuild/build rewrite_confs');
    }

    /**
     * Gets a list of found docroots and associated server names.
     * Only needed for scan.
     *
     * @throws LSCMException  Thrown indirectly.
     */
    protected function prepareDocrootMap()
    {
        $cmd = 'grep -hro "DocumentRoot.*\|ServerAlias.*\|ServerName.*" '
                . '/usr/local/directadmin/data/users/*/httpd.conf';
        exec($cmd, $lines);

        /**
         * [0]=servername, [1]=serveraliases, [2]=docroot, [3]=servername, etc.
         * Not unique & not sorted.
         *
         * Example:
         * ServerName www.daruby1.com
         * ServerAlias www.daruby1.com daruby1.com
         * DocumentRoot /home/daruby1/domains/daruby1.com/public_html
         * ServerName www.daruby1.com
         * ServerAlias www.daruby1.com daruby1.com
         * DocumentRoot /home/daruby1/domains/daruby1.com/private_html
         * ServerName www.dauser1.com
         * ServerAlias www.dauser1.com dauser1.com
         * DocumentRoot /home/dauser1/domains/dauser1.com/public_html
         * ServerName www.dauser1.com
         * ServerAlias www.dauser1.com dauser1.com
         * DocumentRoot /home/dauser1/domains/dauser1.com/private_html
         */

        $docRoots = array();
        $curServerName = $curServerAliases = $curDocRoot = '';

        foreach ( $lines as $line ) {

            if ( $curServerName == '' ) {

                if ( strpos($line, 'ServerName') === 0 ) {
                    /**
                     * 10 is strlen('ServerName')
                     */
                    $curServerName = trim(substr($line, 10));
                }
            }
            elseif ( strpos($line, 'ServerAlias') === 0 ) {
                /**
                 * 11 is strlen('ServerAlias')
                 */
                $curServerAliases = trim(substr($line, 11));
            }
            elseif ( strpos($line, 'DocumentRoot') === 0 ) {
                /**
                 * 12 is strlen('DocumentRoot')
                 */
                $curDocRoot = trim(substr($line, 12));

                /**
                 * Avoid possible duplicate detections due to
                 * public_html/private_html symlinks.
                 */
                if ( !isset($docRoots[$curDocRoot])
                        && is_dir($curDocRoot)
                        && !is_link($curDocRoot) ) {

                    $docRoots[$curDocRoot] =
                            "{$curServerName} {$curServerAliases}";
                }

                /**
                 * Looking for next data set
                 */
                $curServerName = $curServerAliases = $curDocRoot = '';
            }
            else {
                Logger::debug(
                    "Unused line when preparing docroot map: {$line}."
                );
            }
        }

        $roots = array();
        $servernames = array();
        $index = 0;

        foreach ( $docRoots as $docRoot => $line ) {
            $names = preg_split('/\s+/', trim($line), -1, PREG_SPLIT_NO_EMPTY);
            $names = array_unique($names);
            $roots[$index] = $docRoot;

            foreach ( $names as $n ) {
                $servernames[$n] = $index;
            }

            $index++;
        }

        $this->docRootMap =
            array( 'docroots' => $roots, 'names' => $servernames );
    }

    /**
     * Check the user's httpd.conf file for a VirtualHost entry containing a
     * given servername/docroot combination and return the PHP handler version
     * if set.
     *
     * @param WPInstall  $wpInstall
     * @return string
     */
    protected function getCustomPhpHandlerVer( $wpInstall )
    {
        $ver = '';

        $user = $wpInstall->getOwnerInfo('user_name');
        $confFile = "/usr/local/directadmin/data/users/{$user}/httpd.conf";

        $serverName = $wpInstall->getServerName();
        $docRoot = $wpInstall->getDocRoot();

        $escServerName = str_replace('.', '\.', $serverName);
        $escDocRoot = str_replace(array('.', '/'), array('\.', '\/'), $docRoot);

        $pattern = "/VirtualHost(?:(?!<\/VirtualHost).)*"
                . "ServerName {$escServerName}(?:(?!<\/VirtualHost).)*"
                . "DocumentRoot {$escDocRoot}(?:(?!<\/VirtualHost).)*"
                . 'AddHandler ([^\s]*)/s';

         if ( preg_match($pattern, file_get_contents($confFile), $m) ) {
            $ver = substr(trim($m[1]), -2);
         }

         return $ver;
    }

    /**
     *
     * @param WPInstall  $wpInstall
     * @return string
     */
    public function getPhpBinary( WPInstall $wpInstall )
    {
        $phpBin = '/usr/local/bin/php';

        if ( ($ver = $this->getCustomPhpHandlerVer($wpInstall)) != '' ) {

            $customBin = "/usr/local/php{$ver}/bin/php";

            if ( file_exists($customBin) && is_executable($customBin) ) {
                $phpBin = $customBin;
            }
        }

        return "{$phpBin} {$this->phpOptions}";
    }

}
