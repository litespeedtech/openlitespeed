<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    LiteSpeed Technologies, Inc.
 * @copyright 2018-2025 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\Context;

use Lsc\Wp\LSCMException;
use Lsc\Wp\Logger;

/**
 * Context is a singleton
 */
class Context
{

    const LOCAL_PLUGIN_DIR = '/usr/src/litespeed-wp-plugin';

    /**
     * @var int
     */
    protected $isRoot;

    /**
     * @var ContextOption
     */
    protected $options;

    /**
     * @var string
     */
    protected $dataDir;

    /**
     * @var string
     */
    protected $dataFile;

    /**
     * @var string
     */
    protected $customDataFile;

    /**
     * @var null|string
     */
    protected $flagContent;

    /**
     *
     * @var null|Context
     */
    protected static $instance;

    /**
     *
     * @param ContextOption  $contextOption
     */
    protected function __construct( ContextOption $contextOption )
    {
        $this->options = $contextOption;

        $this->init();
    }

    protected function init()
    {
        $this->dataDir        =
            realpath(__DIR__ . '/../../../..') . '/admin/lscdata';
        $this->dataFile       = "$this->dataDir/lscm.data";
        $this->customDataFile = "$this->dataDir/lscm.data.cust";
        $this->isRoot         = $this->options->isRoot();
    }

    /**
     *
     * @since 1.9
     *
     * @return void
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::notice() call.
     * @throws LSCMException  Thrown indirectly by Logger::notice() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    protected function checkLocalPluginDirInCageFS()
    {
        $mpFile    = '/etc/cagefs/cagefs.mp';
        $mountFile = '/proc/mounts';

        if ( !file_exists($mpFile) ) {
            return;
        }

        Logger::debug('Detected cagefs.mp file.');

        $localPluginDir = self::LOCAL_PLUGIN_DIR;

        $pattern      = "=^((?!deleted).)*$localPluginDir((?!deleted).)*$=m";
        $result       = preg_grep($pattern, file($mountFile));
        $procMountSet = !empty($result);

        if ( ! $procMountSet ) {
            Logger::debug("Data dir not set in $mountFile.");

            $pattern     = "=^.*$localPluginDir.*$=m";
            $result      = preg_grep($pattern, file($mpFile));
            $setInMpFile = !empty($result);

            if ( ! $setInMpFile ) {
                file_put_contents(
                    $mpFile,
                    "\n$localPluginDir",
                    FILE_APPEND
                );

                Logger::notice('Added data dir to cagefs.mp.');
            }

            exec('/usr/sbin/cagefsctl --remount-all');

            Logger::notice('Remounted CageFS.');
        }
        else {
            Logger::debug('Data dir already added to CageFS.');
        }
    }

    /**
     *
     * @throws LSCMException  Thrown when unable to create data dir.
     */
    protected function createDataDir()
    {
        if ( !file_exists($this->dataDir) && !mkdir($this->dataDir, 0755) ) {
            throw new LSCMException(
                "Fail to create data directory $this->dataDir.",
                LSCMException::E_PERMISSION
            );
        }
    }

    /**
     *
     * @since 1.9
     *
     * @throws LSCMException  Thrown when unable to create local plugin dir.
     */
    protected function createLocalPluginDir()
    {
        if (
                !file_exists(Context::LOCAL_PLUGIN_DIR)
                &&
                !mkdir(Context::LOCAL_PLUGIN_DIR, 0755)
        ) {
            throw new LSCMException(
                "Fail to create local plugin directory "
                    . Context::LOCAL_PLUGIN_DIR . '.',
                LSCMException::E_PERMISSION
            );
        }
    }

    /**
     *
     * @return ContextOption
     *
     * @throws LSCMException  Thrown indirectly by self::me() call.
     */
    public static function getOption()
    {
        return self::me(true)->options;
    }

    /**
     *
     * @since 1.9.1  Added optional parameter $loggerObj.
     *
     * @param ContextOption $contextOption
     * @param object        $loggerObj      Object implementing all public
     *     Logger class functions.
     *
     * @throws LSCMException  Thrown when Context is already initialized.
     * @throws LSCMException  Thrown indirectly by Logger::setInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     self::$instance->createDataDir() call.
     * @throws LSCMException  Thrown indirectly by
     *     self::$instance->createLocalPluginDir() call.
     * @throws LSCMException  Thrown indirectly by
     *     self::$instance->checkLocalPluginDirInCageFS() call.
     */
    public static function initialize(
        ContextOption $contextOption,
                      $loggerObj = null
    )
    {
        if ( self::$instance != null ) {
            /**
             * Do not allow, must initialize first.
             */
            throw new LSCMException(
                'Context cannot be initialized twice.',
                LSCMException::E_PROGRAM
            );
        }

        self::$instance = new self($contextOption);

        if ( $loggerObj != null ) {
            Logger::setInstance($loggerObj);
        }
        else {
            $loggerClass = $contextOption->getLoggerClass();
            $loggerClass::Initialize($contextOption);
        }

        if ( self::$instance->isRoot == ContextOption::IS_ROOT ) {
            self::$instance->createDataDir();
            self::$instance->createLocalPluginDir();
            self::$instance->checkLocalPluginDirInCageFS();
        }
    }

    /**
     * Checks if the current instance is lacking the expected level of
     * permissions.
     *
     * @return bool
     */
    protected function hasInsufficentPermissions()
    {
        $expectedPermissions =
            self::$instance->options->getExpectedPermissions();

        return (self::$instance->isRoot < $expectedPermissions);
    }

    /**
     *
     * @param bool $checkPerms
     *
     * @return Context
     *
     * @throws LSCMException  Thrown when Context is not initialized.
     * @throws LSCMException  Thrown when required permissions not met.
     */
    protected static function me( $checkPerms = false )
    {
        if ( self::$instance == null ) {
            /**
             * Do not allow, must initialize first.
             */
            throw new LSCMException(
                'Uninitialized context.',
                LSCMException::E_NON_FATAL
            );
        }

        if ( $checkPerms && self::$instance->hasInsufficentPermissions() ) {
            throw new LSCMException(
                'Access denied: Insufficient permissions.',
                LSCMException::E_NON_FATAL
            );
        }

        return self::$instance;
    }

    /**
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by self::me() call.
     */
    public static function getLSCMDataDir()
    {
        return self::me()->dataDir;
    }


    /**
     * Deprecated 06/19/19. Shift to using getLSCMDataFiles() instead.
     *
     * @deprecated
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by self::getLSCMDataFiles()
     *     call.
     */
    public static function getLSCMDataFile()
    {
        $dataFiles = self::getLSCMDataFiles();

        return $dataFiles['dataFile'];
    }

    /**
     *
     * @return string[]
     *
     * @throws LSCMException  Re-thrown when self::me() calls throw an
     *     exception.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    public static function getLSCMDataFiles()
    {
        try {
            $dataFile     = self::me(true)->dataFile;
            $custDataFile = self::me(true)->customDataFile;
        }
        catch ( LSCMException $e ) {
            $msg = "{$e->getMessage()} Could not get data file paths.";
            Logger::debug($msg);

            throw new LSCMException($msg);
        }

        return array(
            'dataFile'     => $dataFile,
            'custDataFile' => $custDataFile
        );
    }

    /**
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by self::me() call.
     */
    public static function isRoot()
    {
        return self::me()->isRoot;
    }

    /**
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by self::me() call.
     */
    public static function isPrivileged()
    {
        return (self::me()->isRoot > ContextOption::IS_NOT_ROOT);
    }

    /**
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by self::me() call.
     */
    public static function getScanDepth()
    {
        return self::me()->options->getScanDepth();
    }

    /**
     *
     * @return int
     *
     * @throws LSCMException Thrown indirectly by self::me() call.
     */
    public static function getActionTimeout()
    {
        $timeout = self::me()->options->getBatchTimeout();

        if ( $timeout > 1 ) {
            return time() + $timeout;
        }

        /**
         * No timeout.
         */
        return 0;
    }

    /**
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by self::me() call.
     */
    public static function getFlagFileContent()
    {
        $m = self::me();

        if ( $m->flagContent == null ) {
            $m->flagContent = <<<CONTENT
This file was created by LiteSpeed Web Cache Manager

When this file exists, your LiteSpeed Cache plugin for WordPress will NOT be affected
by Mass Enable/Disable operations performed through LiteSpeed Web Cache Manager.

Please DO NOT ATTEMPT to remove this file unless you understand the above.

CONTENT;
        }

        return $m->flagContent;
    }

}
