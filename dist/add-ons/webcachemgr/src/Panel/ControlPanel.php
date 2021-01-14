<?php

/** ******************************************
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
    const PANEL_API_VERSION = '1.13.4.2';

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
    const PHP_TIMEOUT = 30;

    /**
     * @var string
     */
    const NOT_SET = '__LSC_NOTSET__';

    /**
     * @var string
     */
    protected $panelName = '';

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
     * @since 1.9.7
     * @var string
     */
    protected static $minAPIFilePath = '';

    /**
     * @var null|ControlPanel  Object that extends ControlPanel abstract class.
     */
    protected static $instance;

    /**
     *
     * @throws LSCMException  Thrown indirectly.
     */
    protected function __construct()
    {
        $this->init2();
    }

    /**
     * Temporary function name until existing deprecated public static init()
     * function is removed.
     *
     * @since 1.13.2
     *
     * @throws LSCMException  Thrown indirectly.
     */
    protected function init2()
    {
        /**
         * output_handler value cleared to avoid compressed output through
         * 'ob_gzhandler' etc.
         */
        $this->phpOptions = '-d disable_functions=ini_set -d opcache.enable=0 '
            . '-d max_execution_time=' . static::PHP_TIMEOUT
            . ' -d memory_limit=512M -d register_argc_argv=1 '
            . '-d zlib.output_compression=0 -d output_handler= '
            . '-d safe_mode=0 -d open_basedir=';

        $this->initConfPaths();
    }

    /**
     * Deprecated 02/04/19 as this function will be made private.
     * Use getClassInstance() with a fully qualified class name as a parameter
     * instead.
     *
     * Sets static::$instance with a new $className instance if it has not been
     * set already. An exception will be thrown if static::$instance has already
     * been set to a different class name than the one provided.
     *
     * @deprecated
     * @param string  $className  A fully qualified control panel class name
     * @return ControlPanel|null  Object that extends ControlPanel abstract
     *                            class.
     * @throws LSCMException
     */
    public static function initByClassName( $className )
    {
        if ( static::$instance == null ) {

            if ( $className == 'custom' ) {
                $lsws_home = realpath(__DIR__ . '/../../../../');
                $customPanelFile =
                    "{$lsws_home}/admin/lscdata/custom/CustomPanel.php";

                if ( ! file_exists($customPanelFile)
                        || ! include_once $customPanelFile ) {

                    throw new LSCMException(
                        "Unable to include file {$customPanelFile}"
                    );
                }

                $className = '\Lsc\Wp\Panel\CustomPanel';

                $isSubClass = is_subclass_of(
                    $className,
                    '\Lsc\Wp\Panel\CustomPanelBase'
                );

                if ( ! $isSubClass ) {
                    throw new LSCMException(
                        'Class CustomPanel must extend class '
                            . '\Lsc\Wp\Panel\CustomPanelBase'
                    );
                }
            }

            try{
                static::$instance = new $className();
            }
            catch ( \Exception $e ){
                throw new LSCMException(
                    "Could not create object with class name {$className}"
                );
            }
        }
        else {
            $instanceClassName = '\\' . get_class(static::$instance);

            if ( $instanceClassName != $className ) {
                throw new LSCMException(
                    "Could not initialize {$className} instance as an instance "
                        . "of another class ({$instanceClassName}) has already "
                        . 'been created.'
                );
            }
        }

        return static::$instance;
    }

    /**
     * Deprecated 01/14/19. Use initByClassName() instead.
     *
     * @deprecated
     *
     * @param string  $name
     * @return ControlPanel  Object that extends ControlPanel abstract class.
     * @throws LSCMException
     */
    public static function init( $name )
    {
        if ( static::$instance != null ) {
            throw new LSCMException(
                'ControlPanel cannot be initialized twice.'
            );
        }

        switch ($name) {
            case static::PANEL_CPANEL:
                $className = 'CPanel';
                break;
            case static::PANEL_PLESK:
                $className = 'Plesk';
                break;
            default:
                throw new LSCMException(
                    "Control panel '{$name}' is not supported."
                );
        }

        return static::initByClassName("\Lsc\Wp\Panel\\{$className}");
    }

    /**
     * Returns current ControlPanel instance when no $className is given. When
     * $className is provided, an instance of $className will also be
     * initialized if it has not yet been initialized already.
     *
     * @param string $className  Fully qualified class name.
     * @return ControlPanel  Object that extends ControlPanel abstract class.
     * @throws LSCMException  Thrown directly and indirectly.
     */
    public static function getClassInstance( $className = '' )
    {
        if ( $className != '' ) {
            static::initByClassName($className);
        }
        elseif ( static::$instance == null ) {
            throw new LSCMException(
                'Could not get instance, ControlPanel not initialized. '
            );
        }

        return static::$instance;
    }

    /**
     * Deprecated on 02/06/19. Use getClassInstance() instead.
     *
     * @deprecated
     * @return ControlPanel  Object that extends ControlPanel abstract class.
     */
    public static function getInstance()
    {
        return static::getClassInstance();
    }

    /**
     *
     * @param string  $serverName
     * @return string|null
     * @throws LSCMException  Thrown indirectly.
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

         if ( static::NOT_SET == $this->getServerCacheRoot() ) {
            $ret = false;
        }

        if ( static::NOT_SET == $this->getVHCacheRoot() ) {
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
            throw new LSCMException(
                'LSCache is not included in the current LiteSpeed license. '
                    . 'Please purchase the LSCache add-on or upgrade to a '
                    . 'license type that includes LSCache and try again.',
                LSCMException::E_PERMISSION
            );
        }

        $restartRequired = false;

        if ( static::NOT_SET == $this->getServerCacheRoot() ) {
            $this->setServerCacheRoot();
            $restartRequired = true;
        }

        if ( static::NOT_SET == $this->getVHCacheRoot() ) {
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
     * @throws LSCMException  Thrown indirectly.
     */
    public function setVHCacheRoot( $vhCacheRoot = 'lscache' )
    {
        $this->log('Attempting to set VH cache root...', Logger::L_VERBOSE);

        if ( !file_exists($this->apacheVHConf) ) {
            $this->createVHConfAndSetCacheRoot(
                $this->apacheVHConf,
                $vhCacheRoot
            );
        }
        else {
            $this->writeVHCacheRoot($this->apacheVHConf, $vhCacheRoot);
        }

        $this->vhCacheRoot = $vhCacheRoot;

        $this->log(
            "Virtual Host cache root set to {$vhCacheRoot}",
            Logger::L_INFO
        );

        if ( $this->vhCacheRoot[0] == '/'
                && !file_exists($this->vhCacheRoot) ) {

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
        $statusFile = '/tmp/lshttpd/.status';

        if ( !file_exists($statusFile) ) {
            throw new LSCMException(
                'Cannot determine LSCache availability. Please start/switch '
                    . 'to LiteSpeed Web Server before trying again.',
                LSCMException::E_PERMISSION
            );
        }

        if ( ($f = fopen($statusFile, 'r')) === false ) {
            throw new LSCMException(
                'Cannot determine LSCache availability.',
                LSCMException::E_PERMISSION
            );
        }

        fseek($f, -128, SEEK_END);
        $line = fread($f, 128);
        fclose($f);

        if ( preg_match('/FEATURES: ([0-9\.]+)/', $line, $m)
                && ($m[1] & 1) == 1 ) {

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
     * @throws LSCMException  Thrown indirectly.
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
     *
     * @noinspection PhpUnused
     * @noinspection PhpDocMissingThrowsInspection
     */
    public function getDocrootMap()
    {
        if ( $this->docRootMap == null ) {
            /**
             * LSCMException not thrown in Plesk implementation.
             * @noinspection PhpUnhandledExceptionInspection
             */
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

    /**
     *
     * @return void
     * @throws LSCMException  Thrown in some existing implementations.
     */
    abstract protected function initConfPaths();

    /**
     *
     * @throws LSCMException  Thrown in some existing implementations.
     */
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

        $serverConf = __DIR__ . '/../../../../conf/httpd_config.xml';

        if ( file_exists($serverConf) ) {
            $file_content = file_get_contents($serverConf);

            $pattern = '!<cacheStorePath>(.+)</cacheStorePath>!i';

            if ( preg_match($pattern, $file_content, $matches) ) {
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
            $this->log(
                "Server level cache root is {$svrCacheRoot}.",
                Logger::L_DEBUG
            );
        }
        else {
            $this->serverCacheRoot = static::NOT_SET;
            $this->log('Server level cache root is not set.', Logger::L_NOTICE);
        }

        if ( $vhCacheRoot ) {
            $this->vhCacheRoot = $vhCacheRoot;
            $this->log(
                "Virtual Host level cache root is {$vhCacheRoot}.",
                Logger::L_DEBUG
            );
        }
        else {
            $this->vhCacheRoot = static::NOT_SET;
            $this->log(
                'Virtual Host level cache root is not set.',
                Logger::L_INFO
            );
        }
    }

    /**
     *
     * @param string  $msg
     * @param int     $level
     * @throws LSCMException  Thrown indirectly.
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
     * @throws LSCMException  Thrown directly and indirectly.
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

        $cacheRootLine =
            "<IfModule LiteSpeed>\nCacheRoot {$cacheroot}\n</IfModule>\n\n";

        if ( !file_exists($this->apacheConf) ) {
            file_put_contents($this->apacheConf, $cacheRootLine);
            chmod($this->apacheConf, 0644);

            $this->log("Created file {$this->apacheConf}", Logger::L_VERBOSE);
        }
        else {

            if ( !is_writable($this->apacheConf) ) {
                throw new LSCMException(
                    'Apache Config is not writeable. No changes made.'
                );
            }

            if ( !Util::createBackup($this->apacheConf) ) {
                throw new LSCMException(
                    'Could not backup Apache config. No changes made.'
                );
            }
            else {
                $file_contents = file($this->apacheConf);

                $pattern = '/^\s*<IfModule +LiteSpeed *>/im';

                if ( preg_grep($pattern, $file_contents) ) {

                    if ( preg_grep('/^\s*CacheRoot +/im', $file_contents) ) {
                        $file_contents = preg_replace(
                            '/^\s*CacheRoot +.+/im',
                            "CacheRoot {$cacheroot}",
                            $file_contents
                        );
                    }
                    else {
                        $file_contents = preg_replace(
                            '/^\s*<IfModule +LiteSpeed *>/im',
                            "<IfModule LiteSpeed>\nCacheRoot {$cacheroot}",
                            $file_contents
                        );
                    }
                }
                else {
                    array_unshift($file_contents, $cacheRootLine);
                }

                file_put_contents($this->apacheConf, $file_contents);
            }
        }

        $this->serverCacheRoot = $cacheroot;

        $this->log(
            "Server level cache root set to {$cacheroot}",
            Logger::L_INFO
        );

        if ( file_exists($cacheroot) ) {
            exec("/bin/rm -rf {$cacheroot}");
            $this->log(
                'Server level cache root directory removed for proper '
                    . 'permission.',
                Logger::L_DEBUG
            );
        }
    }

    /**
     *
     * @param array   $file_contents
     * @param string  $vhCacheRoot
     * @return array
     */
    abstract protected function addVHCacheRootSection( $file_contents,
            $vhCacheRoot = 'lscache' );

    /**
     *
     * @param string  $vhConf
     * @throws LSCMException  Thrown directly and indirectly.
     */
    public function writeVHCacheRoot( $vhConf, $vhCacheRoot = 'lscache' )
    {
        if ( !is_writable($vhConf) ) {
            throw new LSCMException(
                "Could not write to VH config {$vhConf}. No changes made.",
                LSCMException::E_PERMISSION
            );
        }
        if ( !Util::createBackup($vhConf) ) {
            throw new LSCMException(
                "Could not backup Virtual Host config file {$vhConf}. No "
                    . 'changes made.',
                LSCMException::E_PERMISSION
            );
        }

        $file_contents = file($vhConf);

        if ( preg_grep('/^\s*<IfModule +LiteSpeed *>/im', $file_contents) ) {

            if ( preg_grep('/^\s*CacheRoot +/im', $file_contents) ) {
                $modified_contents = preg_replace(
                    '/^\s*CacheRoot +.+/im',
                    "CacheRoot {$vhCacheRoot}",
                    $file_contents
                );
            }
            else {
                $modified_contents = preg_replace(
                    '/^\s*<IfModule +LiteSpeed *>/im',
                    "<IfModule LiteSpeed>\nCacheRoot {$vhCacheRoot}",
                    $file_contents
                );
            }
        }
        else {
            $modified_contents =
                $this->addVHCacheRootSection($file_contents, $vhCacheRoot);
        }

        if ( file_put_contents($vhConf, $modified_contents) === false ) {
            throw new LSCMException(
                "Failed to write to file {$vhConf}.",
                LSCMException::E_PERMISSION
            );
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
     * @since 1.9.7
     */
    protected static function setMinAPIFilePath()
    {
        static::$minAPIFilePath = realpath(__DIR__ . '/../..') . '/MIN_VER';
    }

    /**
     *
     * @since 1.9.7
     *
     * @return string
     */
    protected static function getMinAPIFilePath()
    {
        if ( static::$minAPIFilePath == '' ) {
            static::setMinAPIFilePath();
        }

        return static::$minAPIFilePath;
    }

    /**
     *
     * @since 1.9.7
     * @since 1.12  Changed visibility from protected to public.
     */
    public static function populateMinAPIVerFile()
    {
        $minVerFile = static::getMinAPIFilePath();

        $minVerURL = 'https://www.litespeed.sh/sub/shared/MIN_VER';
        $content = Util::get_url_contents($minVerURL);

        if ( !empty($content) ) {
            file_put_contents($minVerFile, $content);
        }
        else {
            touch($minVerFile);
        }
    }

    /**
     *
     * @since 1.9.7
     *
     * @return string
     */
    protected static function getMinAPIVer()
    {
        $minVerFile = static::getMinAPIFilePath();

        clearstatcache();

        if ( !file_exists($minVerFile)
                || (time() - filemtime($minVerFile)) > 86400 ) {

            static::populateMinAPIVerFile();
        }

        return trim(file_get_contents($minVerFile));
    }

    /**
     *
     * @since 1.9.7
     *
     * @return boolean
     */
    public static function meetsMinAPIVerRequirement()
    {
        $minAPIVer = static::getMinAPIVer();

        if ( $minAPIVer == ''
                || version_compare(static::PANEL_API_VERSION, $minAPIVer, '<') ) {

            return false;
        }

        return true;
    }

    /**
     *
     * @since 1.9
     *
     * @param string  $panelAPIVer  Shared code API version used by the panel
     *                              plugin.
     * @return int
     */
    public static function checkPanelAPICompatibility( $panelAPIVer )
    {
        $supportedAPIVers = array (
            '1.13.4.2',
            '1.13.4.1',
            '1.13.4',
            '1.13.3.1',
            '1.13.3',
            '1.13.2.2',
            '1.13.2.1',
            '1.13.2',
            '1.13.1',
            '1.13.0.3',
            '1.13.0.2',
            '1.13.0.1',
            '1.13',
            '1.12',
            '1.11',
            '1.10',
            '1.9.8',
            '1.9.7',
            '1.9.6.1',
            '1.9.6',
            '1.9.5',
            '1.9.4',
            '1.9.3',
            '1.9.2',
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

        $maxSupportedAPIVer = $supportedAPIVers[0];
        $minSupportedAPIVer = end($supportedAPIVers);

        if ( version_compare($panelAPIVer, $maxSupportedAPIVer, '>') ) {
            return static::PANEL_API_VERSION_TOO_HIGH;
        }
        elseif ( version_compare($panelAPIVer, $minSupportedAPIVer, '<') ) {
            return static::PANEL_API_VERSION_TOO_LOW;
        }
        elseif ( ! in_array($panelAPIVer, $supportedAPIVers) ) {
            return static::PANEL_API_VERSION_UNKNOWN;
        }
        else {
            return static::PANEL_API_VERSION_SUPPORTED;
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
        $apiCompatStatus = static::checkPanelAPICompatibility($panelAPIVer);

        if ( $apiCompatStatus != static::PANEL_API_VERSION_SUPPORTED ) {
            return false;
        }

        return true;
    }

}
