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
 * Logger is a singleton
 */
class Logger
{

    const L_NONE = 0;
    const L_ERROR = 1;
    const L_WARN = 2;
    const L_NOTICE = 3;
    const L_INFO = 4;
    const L_VERBOSE = 5;
    const L_DEBUG = 9;
    const UI_INFO = 0;
    const UI_SUCC = 1;
    const UI_ERR = 2;
    const UI_WARN = 3;

    /**
     * @var null|Logger
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
     *               log file until this Logger object is destroyed.
     */
    protected $bufferedWrite;

    /**
     * @var LogEntry[]  Stores created LogEntry objects.
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
     *
     * @since 1.9
     *
     * @param string  $logFile
     */
    public static function changeLogFileUsed( $logFile )
    {
        self::me()->logFile = $logFile;
    }

    /**
     *
     * @since 1.9
     *
     * @param string  $addInfo
     */
    public static function setAdditionalTagInfo( $addInfo )
    {
        self::me()->addTagInfo = $addInfo;
    }

    /**
     *
     * @since 1.9
     *
     * @return string
     */
    public static function getAdditionalTagInfo()
    {
        return self::me()->addTagInfo;
    }

    /**
     *
     * @since 1.9
     *
     * @return string
     */
    public static function getLogFilePath()
    {
        return self::me()->logFile;
    }

    /**
     *
     * @since 1.9
     *
     * @return LogEntry[]
     */
    public static function getLogMsgQueue()
    {
        return self::me()->msgQueue;
    }

    /**
     *
     * @param int  $type
     * @return string[]
     */
    public static function getUiMsgs( $type )
    {
        $ret = array();

        switch ($type) {
            case self::UI_INFO:
            case self::UI_SUCC:
            case self::UI_ERR:
            case self::UI_WARN:
                $ret = self::me()->uiMsgs[$type];
                break;
            //no default
        }

        return $ret;
    }

    /**
     * Processes any buffered output, writing it to the log file, echoing it
     * out, or both.
     */
    public static function processBuffer()
    {
        $clear = false;

        $m = self::me();

        if ( $m->bufferedWrite ) {
            $m->writeToFile($m->msgQueue);
            $clear = true;
        }

        if ( $m->bufferedEcho ) {
            $m->echoEntries($m->msgQueue);
            $clear = true;
        }

        if ( $clear ) {
            $m->msgQueue = array();
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
        switch ( $type ) {
            case self::UI_INFO:
            case self::UI_SUCC:
            case self::UI_ERR:
            case self::UI_WARN:
                self::me()->uiMsgs[$type][] = $msg;
                break;
            //no default
        }
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
        self::me()->log($msg, $lvl);
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
     * @return Logger
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
     * @param string  $mesg
     * @param int     $lvl
     */
    protected function log( $mesg, $lvl )
    {
        $entry = new LogEntry($mesg, $lvl);

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
     * @param LogEntry[]  $entries
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
     * @param LogEntry[]  $entries
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

            self::me()->logFileLvl = $lvl;

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
