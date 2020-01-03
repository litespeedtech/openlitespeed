<?php

/* * *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2019
 * *******************************************
 */

namespace Lsc\Wp;

use \Lsc\Wp\Context\ContextOption;
use \Lsc\Wp\LogEntry;
use \Lsc\Wp\LSCMException;

/**
 * Logger is a pseudo singleton.
 *
 * Public Logger functions starting with 'p_' are intended for internal use to
 * account for $instance sometimes being of a different non-extending logger
 * class.
 */
class Logger
{

    /**
     * @var int
     */
    const L_NONE = 0;

    /**
     * @var int
     */
    const L_ERROR = 1;

    /**
     * @var int
     */
    const L_WARN = 2;

    /**
     * @var int
     */
    const L_NOTICE = 3;

    /**
     * @var int
     */
    const L_INFO = 4;

    /**
     * @var int
     */
    const L_VERBOSE = 5;

    /**
     * @var int
     */
    const L_DEBUG = 9;

    /**
     * @var int
     */
    const UI_INFO = 0;

    /**
     * @var int
     */
    const UI_SUCC = 1;

    /**
     * @var int
     */
    const UI_ERR = 2;

    /**
     * @var int
     */
    const UI_WARN = 3;

    /**
     * @var null|Logger|object  Object that implements all Logger class
     *                          public functions (minus setInstance()). Caution,
     *                          this requirement is not enforced in the code.
     */
    protected static $instance;

    /**
     * @var int  Highest log message level allowed to be logged. Set to the
     *            higher value between $this->logFileLvl and $this->logEchoLvl.
     */
    protected $logLvl;

    /**
     * @var string  File that log messages will be written to (if writable).
     */
    protected $logFile;

    /**
     * @var int  Highest log message level allowed to be written to the log
     *           file.
     */
    protected $logFileLvl;

    /**
     *
     * @var string  Additional tag to be added at the start of any log
     *              messages.
     */
    protected $addTagInfo = '';

    /**
     * @var boolean  When set to true, log messages will not be written to the
     *               log file until this logger object is destroyed.
     */
    protected $bufferedWrite;

    /**
     * @var LogEntry[]|object[]  Stores created objects that implement all
     *                           LogEntry class public functions.
     */
    protected $msgQueue = array();

    /**
     * @var int  Highest log message level allowed to echoed.
     */
    protected $logEchoLvl;

    /**
     * @var boolean  When set to true, echoing of log messages is suppressed.
     */
    protected $bufferedEcho;

    /**
     * @var string[][]  Leveraged by control panel GUI to store and retrieve
     *                  display messages. Also used as temporary storage for
     *                  display only messages by UserCommand.
     */
    protected $uiMsgs = array(
        self::UI_INFO => array(),
        self::UI_SUCC => array(),
        self::UI_ERR => array(),
        self::UI_WARN => array()
    );

    /**
     *
     * @param ContextOption  $ctxOption
     */
    final protected function __construct( ContextOption $ctxOption )
    {
        $this->logFile = $ctxOption->getDefaultLogFile();
        $this->logFileLvl = $ctxOption->getLogFileLvl();
        $this->bufferedWrite = $ctxOption->isBufferedWrite();
        $this->logEchoLvl = $ctxOption->getLogEchoLvl();
        $this->bufferedEcho = $ctxOption->isBufferedEcho();

        if ( $this->logEchoLvl >= $this->logFileLvl ) {
            $logLvl = $this->logEchoLvl;
        }
        else {
            $logLvl = $this->logFileLvl;
        }

        $this->logLvl = $logLvl;
    }

    public function __destruct()
    {
        if ( $this->bufferedWrite ) {
            $this->writeToFile($this->msgQueue);
        }
    }

    /**
     *
     * @param ContextOption  $contextOption
     * @throws LSCMException
     */
    public static function Initialize( ContextOption $contextOption )
    {
        if ( self::$instance != null ) {
            throw new LSCMException('Logger cannot be initialized twice.',
                    LSCMException::E_PROGRAM);
        }

        self::$instance = new static($contextOption);
    }

    /**
     * Set self::$instance to a pre-created logger object.
     *
     * This function is intended as an alternative to Initialize() and will
     * throw an exception if self::$instance is already set.
     *
     * @since 1.9.1
     *
     * @param object  $loggerObj
     * @throws LSCMException
     */
    public static function setInstance( $loggerObj )
    {
        if ( self::$instance != null ) {
            throw new LSCMException('Logger instance already set.',
                    LSCMException::E_PROGRAM);
        }

        self::$instance = $loggerObj;
    }

    /**
     *
     * @since 1.9
     *
     * @param string  $logFile
     */
    public static function changeLogFileUsed( $logFile )
    {
        self::me()->p_setLogFile($logFile);
    }

    /**
     *
     * @since 1.9
     *
     * @param string  $addInfo
     */
    public static function setAdditionalTagInfo( $addInfo )
    {
        self::me()->p_setAddTagInfo($addInfo);
    }

    /**
     *
     * @since 1.9.1
     *
     * @param string  $msg
     * @param int     $type
     */
    public function p_addUiMsg( $msg, $type )
    {
        switch ($type) {
            case self::UI_INFO:
            case self::UI_SUCC:
            case self::UI_ERR:
            case self::UI_WARN:
                $this->uiMsgs[$type][] = $msg;
                break;
            //no default
        }
    }

    /**
     *
     * @since 1.9.1
     *
     * @param LogEntry[]|object[]  $entries  Array of objects that implement
     *                                       all LogEntry class public
     *                                       functions.
     */
    public function p_echoEntries( $entries )
    {
        $this->echoEntries($entries);
    }

    /**
     *
     * @since 1.9.1
     *
     * @return string
     */
    public function p_getAddTagInfo()
    {
        return $this->addTagInfo;
    }

    /**
     *
     * @since 1.9.1
     *
     * @return boolean
     */
    public function p_getBufferedEcho()
    {
        return $this->bufferedEcho;
    }

    /**
     *
     * @since 1.9.1
     *
     * @return boolean
     */
    public function p_getBufferedWrite()
    {
        return $this->bufferedWrite;
    }

    /**
     *
     * @since 1.9.1
     *
     * @return string
     */
    public function p_getLogFile()
    {
        return $this->logFile;
    }

    /**
     *
     * @since 1.9.1
     *
     * @return LogEntry[]|object[]
     */
    public function p_getMsgQueue()
    {
        return $this->msgQueue;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param int  $type
     * @return string[]
     */
    public function p_getUiMsgs( $type )
    {
        $ret = array();

        switch ($type) {
            case self::UI_INFO:
            case self::UI_SUCC:
            case self::UI_ERR:
            case self::UI_WARN:
                $ret = $this->uiMsgs[$type];
                break;
            //no default
        }

        return $ret;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param string  $msg
     * @param int     $lvl
     */
    public function p_log( $msg, $lvl )
    {
        $this->log($msg, $lvl);
    }

    /**
     *
     * @since 1.9.1
     *
     * @param string  $addInfo
     */
    public function p_setAddTagInfo( $addInfo )
    {
        $this->addTagInfo = $addInfo;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param string  $logFile
     */
    public function p_setLogFile( $logFile )
    {
        $this->logFile = $logFile;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param int  $logFileLvl
     */
    public function p_setLogFileLvl( $logFileLvl )
    {
        $this->logFileLvl = $logFileLvl;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param LogEntry[]|object[]  $msgQueue
     */
    public function p_setMsgQueue( $msgQueue )
    {
        $this->msgQueue = $msgQueue;
    }

    /**
     *
     * @since 1.9.1
     *
     *
     * @param LogEntry[]|object[]  $entries  Array of objects that implement
     *                                       all LogEntry class public
     *                                       functions.
     */
    public function p_writeToFile( $entries )
    {
        $this->writeToFile($entries);
    }

    /**
     *
     * @since 1.9
     *
     * @return string
     */
    public static function getAdditionalTagInfo()
    {
        return self::me()->p_getAddTagInfo();
    }

    /**
     *
     * @since 1.9
     *
     * @return string
     */
    public static function getLogFilePath()
    {
        return self::me()->p_getLogFile();
    }

    /**
     *
     * @since 1.9
     *
     * @return LogEntry[]|object[]  Array of objects that implement all
     *                              LogEntry class public functions.
     */
    public static function getLogMsgQueue()
    {
        return self::me()->p_getMsgQueue();
    }

    /**
     *
     * @param int  $type
     * @return string[]
     */
    public static function getUiMsgs( $type )
    {
        return self::me()->p_getUiMsgs($type);
    }

    /**
     * Processes any buffered output, writing it to the log file, echoing it
     * out, or both.
     */
    public static function processBuffer()
    {
        $clear = false;

        $m = self::me();

        if ( $m->p_getBufferedWrite() ) {
            $m->p_writeToFile($m->p_getMsgQueue());
            $clear = true;
        }

        if ( $m->p_getBufferedEcho() ) {
            $m->p_echoEntries($m->p_getMsgQueue());
            $clear = true;
        }

        if ( $clear ) {
            $m->p_setMsgQueue(array());
        }
    }

    /**
     * Deprecated 06/25/19. Visibility going to be changed to "protected".
     *
     * @deprecated
     * @param string  $msg
     * @param int     $type
     */
    public static function addUiMsg( $msg, $type )
    {
        self::me()->p_addUiMsg($msg, $type);
    }

    /**
     * Calls addUiMsg() with message level self::UI_INFO.
     *
     * @param string  $msg
     */
    public static function uiInfo( $msg )
    {
        self::addUiMsg($msg, self::UI_INFO);
    }

    /**
     * Calls addUiMsg() with message level self::UI_SUCC.
     *
     * @param string  $msg
     */
    public static function uiSuccess( $msg )
    {
        self::addUiMsg($msg, self::UI_SUCC);
    }

    /**
     * Calls addUiMsg() with message level self::UI_ERR.
     *
     * @param string  $msg
     */
    public static function uiError( $msg )
    {
        self::addUiMsg($msg, self::UI_ERR);
    }

    /**
     * Calls addUiMsg() with message level self::UI_WARN.
     *
     * @param string  $msg
     */
    public static function uiWarning( $msg )
    {
        self::addUiMsg($msg, self::UI_WARN);
    }

    /**
     *
     * @param string  $msg
     * @param int     $lvl
     */
    public static function logMsg( $msg, $lvl )
    {
        self::me()->p_log($msg, $lvl);
    }

    /**
     * Calls logMsg() with message level self::L_ERROR.
     *
     * @param string  $msg
     */
    public static function error( $msg )
    {
        self::logMsg($msg, self::L_ERROR);
    }

    /**
     * Calls logMsg() with message level self::L_WARN.
     *
     * @param string  $msg
     */
    public static function warn( $msg )
    {
        self::logMsg($msg, self::L_WARN);
    }

    /**
     * Calls logMsg() with message level self::L_NOTICE.
     *
     * @param string  $msg
     */
    public static function notice( $msg )
    {
        self::logMsg($msg, self::L_NOTICE);
    }

    /**
     * Calls logMsg() with message level self::L_INFO.
     *
     * @param string  $msg
     */
    public static function info( $msg )
    {
        self::logMsg($msg, self::L_INFO);
    }

    /**
     * Calls logMsg() with message level self::L_VERBOSE.
     *
     * @param string  $msg
     */
    public static function verbose( $msg )
    {
        self::logMsg($msg, self::L_VERBOSE);
    }

    /**
     * Calls logMsg() with message level self::L_DEBUG.
     *
     * @param string  $msg
     */
    public static function debug( $msg )
    {
        self::logMsg($msg, self::L_DEBUG);
    }

    /**
     *
     * @return Logger|object  Object that implements all Logger class public
     *                        functions.
     * @throws LSCMException
     */
    protected static function me()
    {
        if ( self::$instance == null ) {
            throw new LSCMException('Logger Uninitialized.', LSCMException::E_PROGRAM);
        }

        return self::$instance;
    }

    /**
     *
     * @param string  $msg
     * @param int     $lvl
     */
    protected function log( $msg, $lvl )
    {
        $entry = new LogEntry($msg, $lvl);

        $this->msgQueue[] = $entry;

        if ( !$this->bufferedWrite ) {
            $this->writeToFile(array( $entry ));
        }

        if ( !$this->bufferedEcho ) {
            $this->echoEntries(array( $entry ));
        }
    }

    /**
     *
     * @param LogEntry[]|object[]  $entries  Array of objects that implement
     *                                       all LogEntry class public
     *                                       functions.
     */
    protected function writeToFile( $entries )
    {
        $content = '';

        foreach ( $entries as $e ) {
            $content .= $e->getOutput($this->logFileLvl);
        }

        if ( $content != '' ) {

            if ( $this->logFile ) {
                file_put_contents($this->logFile, $content,
                        FILE_APPEND | LOCK_EX);
            }
            else {
                error_log($content);
            }
        }
    }

    /**
     *
     * @param LogEntry[]|object[]  $entries  Array of objects that implement
     *                                       all LogEntry class public
     *                                       functions.
     */
    protected function echoEntries( $entries )
    {
        foreach ( $entries as $entry ) {

            if ( ($msg = $entry->getOutput($this->logEchoLvl)) !== '' ) {
                echo $msg;
            }
        }
    }

    /**
     *
     * @param int      $lvl
     * @return string
     */
    public static function getLvlDescr( $lvl )
    {
        switch ($lvl) {

            case self::L_ERROR:
                return 'ERROR';

            case self::L_WARN:
                return 'WARN';

            case self::L_NOTICE:
                return 'NOTICE';

            case self::L_INFO:
                return 'INFO';

            case self::L_VERBOSE:
                return 'DETAIL';

            case self::L_DEBUG:
                return 'DEBUG';

            default:
                /**
                 * Do silently.
                 */
                return '';
        }
    }

    /**
     * Not used yet. Added for later cases where shared log level should be
     * changed to match panel log level.
     *
     * @deprecated 1.9.1  Deprecated on 11/22/19. Function is likely not
     *                    needed after recent logger changes.
     * @param int  $lvl
     * @return boolean
     */
    public static function setLogFileLvl( $lvl )
    {
        $lvl = (int)$lvl;

        if ( self::isValidLogFileLvl($lvl) ) {

            if ( $lvl > self::L_DEBUG ) {
                $lvl = self::L_DEBUG;
            }

            self::me()->p_setLogFileLvl($lvl);

            return true;
        }

        return false;
    }

    /**
     *
     * @param int  $lvl
     * @return boolean
     */
    protected static function isValidLogFileLvl( $lvl )
    {
        if ( is_int($lvl) && $lvl >= 0 ) {
            return true;
        }

        return false;
    }

    /**
     * Prevent cloning here and in extending classes.
     */
    final protected function __clone() {}

}
