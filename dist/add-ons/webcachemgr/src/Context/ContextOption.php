<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp\Context;

class ContextOption
{

    const FROM_CONTROL_PANEL = 'panel';
    const FROM_CLI = 'cli';
    const LOG_FILE_NAME = 'webcachemgr.log';
    const IS_NOT_ROOT = 0;

    /**
     * IS_ROOT_ESCALATABLE depends on panel implemented escalation.
     */
    const IS_ROOT_ESCALATABLE = 1;
    const IS_ROOT = 2;

    /**
     * @var string
     */
    protected $invokerName;

    /**
     * @var string
     */
    protected $invokerType;

    /**
     * @var int
     */
    protected $isRoot;

    /**
     * @var int
     */
    protected $expectedPermissions;

    /**
     * @var string  Logger class name used during initialization. Child classes
     *              that need escalation should override this class name.
     */
    protected $loggerClass;

    /**
     * @var string  If set, must be writable.
     */
    protected $logFile;

    /**
     * @var int  Log to file level.
     */
    protected $logFileLvl;

    /**
     * @var int  Echo to user interface level.
     */
    protected $logEchoLvl;

    /**
     * @var boolean
     */
    protected $bufferedWrite;

    /**
     * @var boolean
     */
    protected $bufferedEcho;

    /**
     * @var string  PluginVersion class name used during initialization. Child
     *              classes that need escalation should override this class
     *              name.
     */
    protected $lscwpVerClass;

    /**
     * @var int
     */
    protected $scanDepth = 2;

    /**
     * @var int
     */
    protected $batchTimeout = 0;

    /**
     * @var int
     */
    protected $batchSize = 0;

    /**
     * @var string
     */
    protected $iconDir = '';

    /**
     * @var string
     */
    protected $sharedTplDir = '';

    /**
     *
     * @param string   $invokerName
     * @param string   $invokerType
     * @param int      $isRoot
     * @param int      $logFileLvl
     * @param int      $logEchoLvl
     * @param boolean  $bufferedWrite
     * @param boolean  $bufferedEcho
     */
    protected function __construct( $invokerName, $invokerType, $isRoot,
            $logFileLvl, $logEchoLvl, $bufferedWrite, $bufferedEcho )
    {
        $this->invokerName = $invokerName;
        $this->invokerType = $invokerType;
        $this->isRoot = $isRoot;
        $this->expectedPermissions = self::IS_ROOT;

        if ( $isRoot == self::IS_ROOT ) {
            $this->logFile = realpath(__DIR__ . '/../../../..') . '/logs/'
                    . self::LOG_FILE_NAME;
        }

        $this->loggerClass = '\Lsc\Wp\Logger';
        $this->lscwpVerClass ='\Lsc\Wp\PluginVersion';
        $this->logFileLvl = $logFileLvl;
        $this->logEchoLvl = $logEchoLvl;
        $this->bufferedWrite = $bufferedWrite;
        $this->bufferedEcho = $bufferedEcho;
    }

    /**
     *
     * @return int
     */
    public function isRoot()
    {
        return $this->isRoot;
    }

    /**
     *
     * @return int
     */
    public function getExpectedPermissions()
    {
        return $this->expectedPermissions;
    }

    /**
     *
     * @return string
     */
    public function getLoggerClass()
    {
        return $this->loggerClass;
    }

    /**
     * Returns the default log file path for this ContextOption. This can be
     * changed in the Logger class itself later on.
     *
     * @return string
     */
    public function getDefaultLogFile()
    {
        return $this->logFile;
    }

    /**
     *
     * @return int
     */
    public function getLogFileLvl()
    {
        return $this->logFileLvl;
    }

    /**
     *
     * @return int
     */
    public function getLogEchoLvl()
    {
        return $this->logEchoLvl;
    }

    /**
     *
     * @return boolean
     */
    public function isBufferedWrite()
    {
        return $this->bufferedWrite;
    }

    /**
     *
     * @return boolean
     */
    public function isBufferedEcho()
    {
        return $this->bufferedEcho;
    }

    /**
     *
     * @return string
     */
    public function getLscwpVerClass()
    {
        return $this->lscwpVerClass;
    }

    /**
     *
     * @return int
     */
    public function getScanDepth()
    {
        return $this->scanDepth;
    }

    /**
     *
     * @return string
     */
    public function getInvokerType()
    {
        return $this->invokerType;
    }

    /**
     *
     * @return string
     */
    public function getInvokerName()
    {
        return $this->invokerName;
    }

    /**
     *
     * @return int
     */
    public function getBatchSize()
    {
        return $this->batchSize;
    }

    /**
     *
     * @return int
     */
    public function getBatchTimeout()
    {
        return $this->batchTimeout;
    }

    /**
     *
     * @param int   $timeout
     * @return int
     */
    public function setBatchTimeout( $timeout )
    {
        if ( is_int($timeout) ) {
            $this->batchTimeout = $timeout;
        }

        return $this->batchTimeout;
    }

    /**
     *
     * @param string $iconDir
     */
    public function setIconDir( $iconDir )
    {
        $this->iconDir = $iconDir;
    }

    /**
     *
     * @return string
     */
    public function getIconDir()
    {
        return $this->iconDir;
    }

    /**
     *
     * @return string
     */
    public function getSharedTplDir()
    {
        return $this->sharedTplDir;
    }

}
