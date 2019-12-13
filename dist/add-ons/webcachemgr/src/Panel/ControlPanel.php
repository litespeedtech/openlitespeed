<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2017-2019
 * ******************************************* */

namespace Lsc\Wp\Panel;

use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Util;
use \Lsc\Wp\WPInstall;

abstract class ControlPanel
{

    /**
     * @deprecated
     *
     * @var string
     */
    const PANEL_CPANEL = 'whm';

    /**
     * @deprecated
     *
     * @var string
     */
    const PANEL_PLESK = 'plesk';

    /**
     * @var string
     */
    const PANEL_API_VERSION = '1.9.1';

    /**
     * @since 1.9
     * @var int
     */
    const PANEL_API_VERSION_SUPPORTED = 0;

    /**
     * @since 1.9
     * @var int
     */
    const PANEL_API_VERSION_TOO_LOW = 1;

    /**
     * @since 1.9
     * @var int
     */
    const PANEL_API_VERSION_TOO_HIGH = 2;

    /**
     * @since 1.9
     * @var int
     */
    const PANEL_API_VERSION_UNKNOWN = 3;

    /**
     * @var int
     */
    const PHP_TIMEOUT = 10;

    /**
     * @var string
     */
    const NOT_SET = '__LSC_NOTSET__';

    /**
     * @var string
     */
    protected $phpOptions;

    /**
     * @var null|string
     */
    protected $serverCacheRoot;

    /**
     * @var null|string
     */
    protected $vhCacheRoot;

    /**
     * @var string
     */
    protected $defaultSvrCacheRoot;

    /**
     * @var string
     */
    protected $apacheConf;

    /**
     * @var string
     */
    protected $apacheVHConf;

    /**
     * @var null|mixed[][]  'docroots' => (index => docroots),
     *                      'names' => (servername => index)
     */
    protected $docRootMap = null;

    /**
     * @var null|ControlPanel
     */
    protected static $instance;

    protected function __construct()
    {
        /**
         * output_handler value cleared to avoid compressed output through
         * 'ob_gzhandler' etc.
         */
        $this->phpOptions = '-d disable_functions=ini_set -d opcache.enable=0 '
                . '-d max_execution_time=' . self::PHP_TIMEOUT . ' -d memory_limit=512M '
                . '-d register_argc_argv=1 -d zlib.output_compression=0 -d output_handler= '
                . '-d safe_mode=0';

        $this->initConfPaths();
    }

    /**
     * Deprecated 02/04/19 as this function will be made private.
     * Use getClassInstance() with a fully qualified class name as a parameter
     * instead.
     *
     * Sets self::$instance with a new $className instance if it has not been
     * set already. An exception will be thrown if self::$instance has already
     * been set to a different class name than the one provided.
     *
     * @deprecated
     * @param string  $className  A fully qualified control panel class name
     * @throws LSCMException
     */
    public static function initByClassName( $className )
    {
        if ( self::$instance == null ) {

            try{
                self::$instance = new $className();
            }
            catch ( \Exception $e ){
                throw new LSCMException(
                        "Could not create object with class name {$className}");
            }
        }
        else {
            $instanceClassName = '\\' . get_class(self::$instance);

            if ( $instanceClassName != $className ) {
                throw new LSCMException(
                        "Could not initialize {$className} instance as an instance of another "
                        . "class ({$instanceClassName}) has already been created.");
            }
        }

        return self::$instance;
    }

    /**
     * Deprecated 01/14/19. Use initByClassName() instead.
     *
     * @deprecated
     *
     * @param string  $name
     * @return ControlPanel
     * @throws LSCMException
     */
    public static function init( $name )
    {
        if ( self::$instance != null ) {
            throw new LSCMException('ControlPanel cannot be initialized twice.');
        }

        switch ($name) {
            case self::PANEL_CPANEL:
                $className = 'CPanel';
                break;
            case self::PANEL_PLESK:
                $className = 'Plesk';
                break;
            default:
                throw new LSCMException("Control panel '{$name}' is not supported.");
        }

        return self::initByClassName("\Lsc\Wp\Panel\\{$className}");
    }

    /**
     * Returns current ControlPanel instance when no $className is given. When
     * $className is provided, an instance of $className will also be
     * initialized if it has not yet been initialized already.
     *
     * @param string $className  Fully qualified class name.
     * @return ControlPanel
     * @throws LSCMException
     */
    public static function getClassInstance( $className = '' )
    {
        if ( $className != '' ) {
            self::initByClassName($className);
        }
        elseif ( self::$instance == null ) {
            throw new LSCMException('Could not get instance, ControlPanel not initialized. ');
        }

        return self::$instance;
    }

    /**
     * Deprecated on 02/06/19. Use getClassInstance() instead.
     *
     * @deprecated
     * @return ControlPanel
     */
    public static function getInstance()
    {
        return $this->getClassInstance();
    }

    /**
     *
     * @param string  $serverName
     * @return string|null
     */
    public function mapDocRoot( $serverName )
    {
        if ( $this->docRootMap == null ) {
            $this->prepareDocrootMap();
        }

        if ( isset($this->docRootMap['names'][$serverName]) ) {
            $index = $this->docRootMap['names'][$serverName];

            return $this->docRootMap['docroots'][$index];
        }

        // error out
        return null;
    }

    /**
     *
     * @return boolean
     */
    public function areCacheRootsSet()
    {
        $ret = true;

         if ( self::NOT_SET == $this->getServerCacheRoot() ) {
            $ret = false;
        }

        if ( self::NOT_SET == $this->getVHCacheRoot() ) {
            $ret = false;
        }

        return $ret;
    }

    /**
     *
     * @throws LSCMException
     */
    public function verifyCacheSetup()
    {
        if ( !$this->isCacheEnabled() ) {
            $msg = 'LSCache is not included in the current LiteSpeed license. Please purchase the '
                    . 'LSCache add-on or upgrade to a license type that includes LSCache and try '
                    . 'again.';

            throw new LSCMException($msg, LSCMException::E_PERMISSION);
        }

        $restartRequired = false;

        if ( self::NOT_SET == $this->getServerCacheRoot() ) {
            $this->setServerCacheRoot();
            $restartRequired = true;
        }

        if ( self::NOT_SET == $this->getVHCacheRoot() ) {
            $this->setVHCacheRoot();
            $restartRequired = true;
        }

        if ( $restartRequired ) {
            Util::restartLsws();
        }
    }

    /**
     *
     * @param string  $vhCacheRoot
     */
    public function setVHCacheRoot( $vhCacheRoot = 'lscache' )
    {
        $this->log('Attempting to set VH cache root...', Logger::L_VERBOSE);

        if ( !file_exists($this->apacheVHConf) ) {
            $this->createVHConfAndSetCacheRoot($this->apacheVHConf,
                    $vhCacheRoot);
        }
        else {
            $this->writeVHCacheRoot($this->apacheVHConf, $vhCacheRoot);
        }

        $this->vhCacheRoot = $vhCacheRoot;

        $this->log("Virtual Host cache root set to {$vhCacheRoot}", Logger::L_INFO);

        if ( $this->vhCacheRoot[0] == '/' && !file_exists($this->vhCacheRoot) ) {
            /**
             * 01/29/19: Temporarily create top virtual host cache root
             * directory to avoid LSWS setting incorrect owner/group and
             * permissions for the directory outside of the cage.
             */

            $topVHCacheRoot = str_replace('/$vh_user', '', $vhCacheRoot);
            mkdir($topVHCacheRoot, 0755, true);
        }

        $this->applyVHConfChanges();
    }

    /**
     *
     * @return boolean
     * @throws LSCMException
     */
    public function isCacheEnabled()
    {
        if ( ($f = fopen('/tmp/lshttpd/.status', 'r')) === false ) {
            throw new LSCMException('Cannot determine LSCache availability.',
                    LSCMException::E_PERMISSION);
        }

        fseek($f, -128, SEEK_END);
        $line = fread($f, 128);
        fclose($f);

        if ( preg_match('/FEATURES: ([0-9\.]+)/', $line, $m) && ($m[1] & 1) == 1 ) {
            return true;
        }

        return false;
    }

    /**
     * return array of docroots, can set index from and batch
     *
     * @param int       $offset
     * @param null|int  $length
     * @return string[]
     */
    public function getDocRoots( $offset = 0, $length = null )
    {
        if ( $this->docRootMap == null ) {
            $this->prepareDocrootMap();
        }

        return array_slice($this->docRootMap['docroots'], $offset, $length);
    }

    /**
     * Used in PleskEscalate.
     *
     * @return mixed[][]
     */
    public function getDocrootMap()
    {
        if ( $this->docRootMap == null ) {
            $this->prepareDocrootMap();
        }

        return $this->docRootMap;
    }

    /**
     *
     * @return string
     */
    public function getDefaultSvrCacheRoot()
    {
        return $this->defaultSvrCacheRoot;
    }

    /**
     *
     * @return string
     */
    public function getServerCacheRoot()
    {
        if ( $this->serverCacheRoot == null ) {
            $this->initCacheRoots();
        }

        return $this->serverCacheRoot;
    }

    /**
     *
     * @return string
     */
    public function getVHCacheRoot()
    {
        if ( $this->vhCacheRoot == null ) {
            $this->initCacheRoots();
        }

        return $this->vhCacheRoot;
    }

    abstract protected function initConfPaths();

    abstract protected function prepareDocrootMap();

    abstract public function getPhpBinary( WPInstall $wpInstall );

    /**
     * Searches the given directories '.conf' files for CacheRoot setting.
     *
     * Note: Visibility is public to better accommodate escalation functions.
     *
     * @param string  $confDir  Directory to be searched.
     * @return string
     */
    public function cacheRootSearch( $confDir )
    {
        $files = new \DirectoryIterator($confDir);

        foreach ( $files as $file ) {
            $filename = $file->getFilename();

            if ( strlen($filename) > 5
                    && substr_compare($filename, '.conf', -5) === 0 ) {

                $cacheRoot = $this->getCacheRootSetting($file->getPathname());

                if ( $cacheRoot != '' ) {
                    return $cacheRoot;
                }
            }
        }

        return '';
    }

    /**
     * Note: Visibility is public to better accommodate escalation functions.
     *
     * @param string  $file
     * @return string
     */
    public function getCacheRootSetting( $file )
    {
        $cacheRoot = '';

        if ( file_exists($file) ) {
            $file_content = file_get_contents($file);

            if ( preg_match('/^\s*CacheRoot (.+)/im', $file_content, $matches) ) {
                $cacheRoot = trim($matches[1]);
            }
        }

        return $cacheRoot;
    }

    /**
     * Note: Visibility is public to better accommodate escalation functions.
     *
     * @return string
     */
    public function getLSWSCacheRootSetting()
    {
        $svrCacheRoot = '';

        $serverConf = dirname(__FILE__) . '/../../../../conf/httpd_config.xml';

        if ( file_exists($serverConf) ) {
            $file_content = file_get_contents($serverConf);

            if ( preg_match('!<cacheStorePath>(.+)</cacheStorePath>!i',
                            $file_content, $matches) ) {

                $svrCacheRoot = trim($matches[1]);
            }
        }

        return $svrCacheRoot;
    }

    abstract protected function serverCacheRootSearch();

    abstract protected function vhCacheRootSearch();

    /**
     * Checks server and VH conf files for cacheroot settings and populates in
     * object if found.
     */
    protected function initCacheRoots()
    {
        $svrCacheRoot = $this->serverCacheRootSearch();

        if ( $svrCacheRoot == '' ) {
            $svrCacheRoot = $this->getLSWSCacheRootSetting();
        }

        $vhCacheRoot = $this->vhCacheRootSearch();

        if ( $svrCacheRoot ) {
            $this->serverCacheRoot = $svrCacheRoot;
            $this->log("Server level cache root is {$svrCacheRoot}.", Logger::L_DEBUG);
        }
        else {
            $this->serverCacheRoot = self::NOT_SET;
            $this->log('Server level cache root is not set.', Logger::L_NOTICE);
        }

        if ( $vhCacheRoot ) {
            $this->vhCacheRoot = $vhCacheRoot;
            $this->log("Virtual Host level cache root is {$vhCacheRoot}.", Logger::L_DEBUG);
        }
        else {
            $this->vhCacheRoot = self::NOT_SET;
            $this->log('Virtual Host level cache root is not set.', Logger::L_INFO);
        }
    }

    /**
     *
     * @param string  $msg
     * @param int     $level
     */
    protected function log( $msg, $level )
    {
        $msg = "{$this->panelName} - {$msg}";

        switch ($level) {

            case Logger::L_ERROR:
                Logger::error($msg);
                break;

            case Logger::L_WARN:
                Logger::warn($msg);
                break;

            case Logger::L_NOTICE:
                Logger::notice($msg);
                break;

            case Logger::L_INFO:
                Logger::info($msg);
                break;

            case Logger::L_VERBOSE:
                Logger::verbose($msg);
                break;

            case Logger::L_DEBUG:
                Logger::debug($msg);
                break;

            //no default
        }
    }

    /**
     *
     * @param string  $svrCacheRoot
     * @throws LSCMException
     */
    public function setServerCacheRoot( $svrCacheRoot = '' )
    {
        $this->log('Attempting to set server cache root...', Logger::L_VERBOSE);

        if ( $svrCacheRoot != '' ) {
            $cacheroot = $svrCacheRoot;
        }
        else {
            $cacheroot = $this->defaultSvrCacheRoot;
        }

        $cacheRootLine = "<IfModule LiteSpeed>\nCacheRoot {$cacheroot}\n</IfModule>\n\n";

        if ( !file_exists($this->apacheConf) ) {
            file_put_contents($this->apacheConf, $cacheRootLine);
            chmod($this->apacheConf, 0644);

            $this->log("Created file {$this->apacheConf}", Logger::L_VERBOSE);
        }
        else {

            if ( !is_writable($this->apacheConf) ) {
                throw new LSCMException('Apache Config is not writeable. No changes made.');
            }

            if ( !Util::createBackup($this->apacheConf) ) {
                throw new LSCMException('Could not backup Apache config. No changes made.');
            }
            else {
                $file_contents = file($this->apacheConf);

                if ( preg_grep('/^\s*<IfModule +LiteSpeed *>/im', $file_contents) ) {

                    if ( preg_grep('/^\s*CacheRoot +/im', $file_contents) ) {
                        $file_contents = preg_replace('/^\s*CacheRoot +.+/im',
                                "CacheRoot {$cacheroot}", $file_contents);
                    }
                    else {
                        $file_contents = preg_replace('/^\s*<IfModule +LiteSpeed *>/im',
                                "<IfModule LiteSpeed>\nCacheRoot {$cacheroot}",
                                $file_contents);
                    }
                }
                else {
                    array_unshift($file_contents, $cacheRootLine);
                }

                file_put_contents($this->apacheConf, $file_contents);
            }
        }

        $this->serverCacheRoot = $cacheroot;

        $this->log("Server level cache root set to {$cacheroot}", Logger::L_INFO);

        if ( file_exists($cacheroot) ) {
            exec("/bin/rm -rf {$cacheroot}");
            $this->log('Server level cache root directory removed for proper permission.',
                    Logger::L_DEBUG);
        }
    }

    abstract protected function addVHCacheRootSection( $file_contents,
            $vhCacheRoot = 'lscache' );

    /**
     *
     * @param string  $vhConf
     * @throws LSCMException
     */
    public function writeVHCacheRoot( $vhConf, $vhCacheRoot = 'lscache' )
    {
        if ( !is_writable($vhConf) ) {
            throw new LSCMException(
                    "Could not write to VH config {$vhConf}. No changes made.",
                    LSCMException::E_PERMISSION);
        }
        if ( !Util::createBackup($vhConf) ) {
            throw new LSCMException(
                    "Could not backup Virtual Host config file {$vhConf}. No changes made.",
                    LSCMException::E_PERMISSION);
        }

        $file_contents = file($vhConf);

        if ( preg_grep('/^\s*<IfModule +LiteSpeed *>/im', $file_contents) ) {

            if ( preg_grep('/^\s*CacheRoot +/im', $file_contents) ) {
                $modified_contents = preg_replace('/^\s*CacheRoot +.+/im',
                        "CacheRoot {$vhCacheRoot}", $file_contents);
            }
            else {
                $modified_contents = preg_replace('/^\s*<IfModule +LiteSpeed *>/im',
                        "<IfModule LiteSpeed>\nCacheRoot {$vhCacheRoot}",
                        $file_contents);
            }
        }
        else {
            $modified_contents =
                    $this->addVHCacheRootSection($file_contents, $vhCacheRoot);
        }

        if ( file_put_contents($vhConf, $modified_contents) === false ) {
            throw new LSCMException("Failed to write to file {$vhConf}.",
                    LSCMException::E_PERMISSION);
        }

        $this->log("Updated file {$vhConf}.", Logger::L_DEBUG);
    }

    /**
     * Note: Visibility is public to better accommodate escalation functions.
     *
     * @param string  $vhConf
     * @param string  $vhCacheRoot
     */
    abstract public function createVHConfAndSetCacheRoot( $vhConf,
            $vhCacheRoot = 'lscache' );

    /**
     * Note: Visibility is public to better accommodate escalation functions.
     */
    public function applyVHConfChanges()
    {
        //override in child class when needed.
    }

    /**
     *
     * @since 1.9
     *
     * @param type $panelAPIVer  Shared code API version used by the panel
     *                           plugin.
     * @return int
     */
    public static function checkPanelAPICompatibility( $panelAPIVer )
    {
        $supportedAPIVers = array (
            '1.9.1',
            '1.9',
            '1.8',
            '1.7',
            '1.6.1',
            '1.6',
            '1.5',
            '1.4',
            '1.3',
            '1.2',
            '1.1',
            '1.0'
        );

        $maxAPIVer = $supportedAPIVers[0];
        $minAPIVer = end($supportedAPIVers);

        if ( version_compare($panelAPIVer, $maxAPIVer, '>') ) {
            return self::PANEL_API_VERSION_TOO_HIGH;
        }
        elseif ( version_compare($panelAPIVer, $minAPIVer, '<') ) {
            return self::PANEL_API_VERSION_TOO_LOW;
        }
        elseif ( ! in_array($panelAPIVer, $supportedAPIVers) ) {
            return self::PANEL_API_VERSION_UNKNOWN;
        }
        else {
            return self::PANEL_API_VERSION_SUPPORTED;
        }
    }

    /**
     *
     * @deprecated 1.9  Use checkPanelAPICompatibility() instead.
     *
     * @param string  $panelAPIVer  Shared code API version used by the panel
     *                               plugin.
     * @return boolean
     */
    public static function isPanelAPICompatible( $panelAPIVer )
    {
        $apiCompatStatus = self::checkPanelAPICompatibility($panelAPIVer);

        if ( $apiCompatStatus != self::PANEL_API_VERSION_SUPPORTED ) {
            return false;
        }

        return true;
    }

}
