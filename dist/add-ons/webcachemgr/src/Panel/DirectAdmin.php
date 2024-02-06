<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2019-2023 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\Panel;

use DirectoryIterator;
use Lsc\Wp\Logger;
use Lsc\Wp\LSCMException;
use Lsc\Wp\WPInstall;

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
        $this->panelName           = 'DirectAdmin';
        $this->defaultSvrCacheRoot = '/home/lscache/';

        /** @noinspection PhpUnhandledExceptionInspection */
        parent::init2();
    }

    protected function initConfPaths()
    {
        $this->apacheConf   = '/etc/httpd/conf/extra/httpd-includes.conf';
        $this->apacheVHConf = '/usr/local/directadmin/data/templates/custom/'
            . 'cust_httpd.CUSTOM.2.pre';
    }

    /**
     *
     * @return string
     */
    protected function serverCacheRootSearch()
    {
        if ( !file_exists($this->apacheConf) ) {
            return '';
        }

        return $this->getCacheRootSetting($this->apacheConf);
    }

    /**
     *
     * @return string
     */
    protected function vhCacheRootSearch()
    {
        $apacheUserdataDir = dirname($this->apacheVHConf);

        if ( !file_exists($apacheUserdataDir) ) {
            return '';
        }

        return $this->daVhCacheRootSearch($apacheUserdataDir);
    }

    /**
     * Searches the given directories '.pre' and '.post' files for CacheRoot
     * setting.
     *
     * @param string $confDir  Directory to be searched.
     *
     * @return string
     */
    public function daVhCacheRootSearch( $confDir )
    {
        $files = new DirectoryIterator($confDir);

        foreach ( $files as $file ) {
            $filename = $file->getFilename();

            $isPreOrPostFile = (
                strlen($filename) > 4
                &&
                (
                    substr_compare($filename, '.pre', -4) === 0
                    ||
                    substr_compare($filename, '.post', -5) === 0
                )
            );

            if ( $isPreOrPostFile ) {
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
     * @param array  $file_contents
     * @param string $vhCacheRoot
     *
     * @return array
     */
    protected function addVHCacheRootSection(
        array $file_contents,
              $vhCacheRoot = 'lscache' )
    {
        array_unshift(
            $file_contents,
            "<IfModule LiteSpeed>\nCacheRoot $vhCacheRoot\n</IfModule>\n\n"
        );

        return $file_contents;
    }

    /**
     *
     * @param string $vhConf
     * @param string $vhCacheRoot
     *
     * @throws LSCMException  Thrown when mkdir() call fails to create virtual
     *     host conf directory.
     * @throws LSCMException  Thrown when file_put_contents() call fails to
     *     create virtual host conf file.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     */
    public function createVHConfAndSetCacheRoot(
        $vhConf,
        $vhCacheRoot = 'lscache' )
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

        $bytesWrittenToFile = file_put_contents(
            $vhConf,
            "<IfModule Litespeed>\nCacheRoot $vhCacheRoot\n</IfModule>"
        );

        if ( false === $bytesWrittenToFile ) {
            throw new LSCMException("Failed to create file $vhConf.");
        }

        $this->log("Created file $vhConf.", Logger::L_DEBUG);
    }

    public function applyVHConfChanges()
    {
        exec('/usr/local/directadmin/custombuild/build rewrite_confs');
    }

    /**
     *
     * @since 1.13.7
     *
     * @return array[]
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    private function getHttpdConfDocrootMapInfo()
    {
        exec(
            'grep -hros "DocumentRoot.*\|ServerAlias.*\|ServerName.*" '
                . '/usr/local/directadmin/data/users/*/httpd.conf',
            $lines
        );

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
         *
         * @noinspection SpellCheckingInspection
         */

        $docRoots      = array();
        $curServerName = $curServerAliases = '';

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
                $curDocRoot = trim(substr($line, 12), " \n\r\t\v\x00\"");

                /**
                 * Avoid possible duplicate detections due to
                 * public_html/private_html symlinks.
                 */
                if ( !isset($docRoots[$curDocRoot])
                        && is_dir($curDocRoot)
                        && !is_link($curDocRoot) ) {

                    $docRoots[$curDocRoot]   = explode(' ', $curServerAliases);
                    $docRoots[$curDocRoot][] = $curServerName;
                }

                /**
                 * Looking for the next data set
                 */
                $curServerName = $curServerAliases = '';
            }
            else {
                Logger::debug("Unused line when preparing docroot map: $line.");
            }
        }

        return $docRoots;
    }

    /**
     *
     * @since 1.13.7
     *
     * @return array[]
     *
     * @throws LSCMException Thrown indirectly by Logger::debug() call.
     */
    private function getOpenlitespeedConfDocrootMapInfo()
    {
        exec(
            'grep -hros "docRoot.*\|vhDomain.*\|vhAliases.*" '
                . '/usr/local/directadmin/data/users/*/openlitespeed.conf',
            $lines
        );

        /**
         * [0]=docroot, [1]=servername, [2]=serveraliases, [3]=docroot, etc.
         * Not unique & not sorted.
         *
         * Example:
         * docRoot                   /home/test/domains/test.com/public_html
         * vhDomain                  test.com
         * vhAliases                 www.test.com
         * docRoot                   /home/test/domains/test.com/public_html
         * vhDomain                  testalias.com
         * vhAliases                 www.testalias.com
         * docRoot                   /home/test/domains/test.com/private_html
         * vhDomain                  test.com
         * vhAliases                 www.test.com
         * docRoot                   /home/test/domains/test.com/private_html
         * vhDomain                  testalias.com
         * vhAliases                 www.testalias.com
         * docRoot                   /home/test_2/domains/test2.com/public_html
         * vhDomain                  test2.com
         * vhAliases                 www.test2.com
         * docRoot                   /home/test_2/domains/test2.com/private_html
         * vhDomain                  test2.com
         * vhAliases                 www.test2.com
         *
         * @noinspection SpellCheckingInspection
         */

        $docRoots      = array();
        $curServerName = $curDocRoot = '';

        foreach ( $lines as $line ) {

            if ( $curDocRoot == '' ) {

                if ( strpos($line, 'docRoot') === 0 ) {
                    /**
                     * 7 is strlen('docRoot')
                     */
                    $curDocRoot = trim(substr($line, 7));
                }
            }
            elseif ( strpos($line, 'vhDomain') === 0 ) {
                /**
                 * 8 is strlen('vhDomain')
                 */
                $curServerName = trim(substr($line, 8));
            }
            elseif ( strpos($line, 'vhAliases') === 0 ) {
                /**
                 * 9 is strlen('vhAliases')
                 */
                $curServerAlias = trim(substr($line, 9));

                /**
                 * Avoid possible duplicate detections due to
                 * public_html/private_html symlinks.
                 */
                if ( is_dir($curDocRoot) && !is_link($curDocRoot) ) {

                    if ( !isset($docRoots[$curDocRoot]) ) {
                        $docRoots[$curDocRoot] =
                            array( $curServerName, $curServerAlias );
                    }
                    else {

                        if ( !in_array($curServerName, $docRoots[$curDocRoot]) ) {
                            $docRoots[$curDocRoot][] = $curServerName;
                        }

                        if ( !in_array($curServerAlias, $docRoots[$curDocRoot]) ) {
                            $docRoots[$curDocRoot][] = $curServerAlias;
                        }
                    }
                }

                /**
                 * Looking for the next data set
                 */
                $curDocRoot = $curServerName = '';

            }
            else {
                Logger::debug("Unused line when preparing docroot map: $line.");
            }
        }

        return $docRoots;
    }

    /**
     * Gets a list of found docroots and associated server names.
     * Only needed for scan operation.
     *
     * @throws LSCMException  Thrown indirectly by
     *     $this->getHttpdConfDocrootMapInfo() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->getOpenlitespeedConfDocrootMapInfo() call.
     */
    protected function prepareDocrootMap()
    {
        $docRootMapInfo                  = $this->getHttpdConfDocrootMapInfo();
        $openlitespeedConfDocrootMapInfo =
            $this->getOpenlitespeedConfDocrootMapInfo();

        foreach ( $openlitespeedConfDocrootMapInfo as $oDocRoot => $oDomains ) {

            if ( !isset($docRootMapInfo[$oDocRoot]) ) {
                $docRootMapInfo[$oDocRoot] = $oDomains;
            }
            else {

                foreach ( $oDomains as $oDomain ) {

                    if ( !in_array($oDomain, $docRootMapInfo[$oDocRoot]) ) {
                        $docRootMapInfo[$oDocRoot][] = $oDomain;
                    }
                }
            }
        }

        $roots       = array();
        $servernames = array();
        $index       = 0;

        foreach ( $docRootMapInfo as $docRoot => $domains ) {
            $domains       = array_unique($domains);
            $roots[$index] = $docRoot;

            foreach ( $domains as $domain ) {
                $servernames[$domain] = $index;
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
     * @param WPInstall $wpInstall
     *
     * @return string
     *
     * @throws LSCMException  Thrown when a valid user data conf file could not
     *     be found.
     */
    protected function getCustomPhpHandlerVer( WPInstall $wpInstall )
    {
        if ( ($serverName = $wpInstall->getServerName()) == null
                || ($docroot = $wpInstall->getDocRoot()) == null ) {

            return '';
        }

        $escServerName = str_replace('.', '\.', $serverName);
        $escDocRoot    =
            str_replace(array('.', '/'), array('\.', '\/'), $docroot);

        $user = $wpInstall->getOwnerInfo('user_name');

        $httpdConfFile = "/usr/local/directadmin/data/users/$user/httpd.conf";
        $olsConfFile   =
            "/usr/local/directadmin/data/users/$user/openlitespeed.conf";

        if ( file_exists($httpdConfFile) ) {
            $confFile = $httpdConfFile;
            $pattern  = '/VirtualHost'
                . '(?:(?!<\/VirtualHost).)*'
                . "ServerName $escServerName"
                . '(?:(?!<\/VirtualHost).)*'
                . "DocumentRoot $escDocRoot"
                . '(?:(?!<\/VirtualHost).)*'
                . 'AddHandler.* \.php(\d\d)/sU';
        }
        elseif ( file_exists($olsConfFile) ) {
            $confFile = $olsConfFile;
            $pattern  = '/virtualHost\s'
                . '(?:(?!}\s*\n*virtualHost).)*?'
                . '{'
                . '(?:(?!}\s*\n*virtualHost).)*?'
                . "(?:\s|\n)docRoot\s+$escDocRoot(?:\s|\n)"
                . '(?:(?!}\s*\n*virtualHost).)*?'
                . "(?:\s|\n)vhDomain\s+$escServerName(?:\s|\n)"
                . '(?:(?!}\s*\n*virtualHost).)*?'
                . '(?:\s|\n)scripthandler(?:\s|\n)*{'
                . '(?:(?!}\s*\n*virtualHost).)*?'
                . '\sphp(\d\d)(?=\s|\n)/s';
        }
        else {
            throw new LSCMException('Could not find valid user data conf file');
        }

         if ( preg_match($pattern, file_get_contents($confFile), $m) ) {
            return $m[1];
         }

         return '';
    }

    /**
     *
     * @param WPInstall $wpInstall
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by
     *     $this->getCustomPhpHandlerVer() call.
     */
    public function getPhpBinary( WPInstall $wpInstall )
    {
        $phpBin = '/usr/local/bin/php';

        if ( ($ver = $this->getCustomPhpHandlerVer($wpInstall)) != '' ) {

            $customBin = "/usr/local/php$ver/bin/php";

            if ( file_exists($customBin) && is_executable($customBin) ) {
                $phpBin = $customBin;
            }
        }

        return "$phpBin $this->phpOptions";
    }

}
