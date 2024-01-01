<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2023 LiteSpeed Technologies, Inc.
 * *******************************************
 */

namespace Lsc\Wp;

use Lsc\Wp\Context\ContextOption;

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
     * @var null|Logger|object  Object that implements all Logger class public
     *     functions (minus setInstance()). Caution, this requirement is not
     *     enforced in the code.
     */
    protected static $instance;

    /**
     * @var int  Highest log message level allowed to be logged. Set to the
     *     higher value between $this->logFileLvl and $this->logEchoLvl.
     */
    protected $logLvl;

    /**
     * @var string  File that log messages will be written to (if writable).
     */
    protected $logFile;

    /**
     * @var int  Highest log message level allowed to be written to the log
     *     file.
     */
    protected $logFileLvl;

    /**
     * @var string  Additional tag to be added at the start of any log messages.
     */
    protected $addTagInfo = '';

    /**
     * @var bool  When set to true, log messages will not be written to the log
     *     file until this logger object is destroyed.
     */
    protected $bufferedWrite;

    /**
     * @var LogEntry[]|object[]  Stores created objects that implement all
     *     LogEntry class public functions.
     */
    protected $msgQueue = array();

    /**
     * @var int  Highest log message level allowed to echo.
     */
    protected $logEchoLvl;

    /**
     * @var bool  When set to true, echoing of log messages is suppressed.
     */
    protected $bufferedEcho;

    /**
     * @var string[][]  Leveraged by control panel GUI to store and retrieve
     *     display messages. Also used as temporary storage for display only
     *     messages by UserCommand.
     */
    protected $uiMsgs = array(
        self::UI_INFO => array(),
        self::UI_SUCC => array(),
        self::UI_ERR  => array(),
        self::UI_WARN => array()
    );

    /**
     *
     * @param ContextOption $ctxOption
     */
    final protected function __construct( ContextOption $ctxOption )
    {
        $this->logFile       = $ctxOption->getDefaultLogFile();
        $this->logFileLvl    = $ctxOption->getLogFileLvl();
        $this->bufferedWrite = $ctxOption->isBufferedWrite();
        $this->logEchoLvl    = $ctxOption->getLogEchoLvl();
        $this->bufferedEcho  = $ctxOption->isBufferedEcho();
        $this->logLvl        = max($this->logEchoLvl, $this->logFileLvl);
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->writeToFile() call.
     */
    public function __destruct()
    {
        if ( $this->bufferedWrite ) {
            $this->writeToFile($this->msgQueue);
        }
    }

    /**
     *
     * @param ContextOption $contextOption
     *
     * @throws LSCMException  Thrown when a static Logger instance has already
     *     been initialized.
     */
    public static function Initialize( ContextOption $contextOption )
    {
        if ( static::$instance != null ) {
            throw new LSCMException(
                'Logger cannot be initialized twice.',
                LSCMException::E_PROGRAM
            );
        }

        static::$instance = new static($contextOption);
    }

    /**
     * Set static::$instance to a pre-created logger object.
     *
     * This function is intended as an alternative to Initialize() and will
     * throw an exception if static::$instance is already set.
     *
     * @since 1.9.1
     *
     * @param object $loggerObj
     *
     * @throws LSCMException  Thrown when Logger instance is already set.
     */
    public static function setInstance( $loggerObj )
    {
        if ( static::$instance != null ) {
            throw new LSCMException(
                'Logger instance already set.',
                LSCMException::E_PROGRAM
            );
        }

        static::$instance = $loggerObj;
    }

    /**
     *
     * @since 1.9
     *
     * @param string $logFile
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     */
    public static function changeLogFileUsed( $logFile )
    {
        static::me()->p_setLogFile($logFile);
    }

    /**
     *
     * @since 1.9
     *
     * @param string $addInfo
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     */
    public static function setAdditionalTagInfo( $addInfo )
    {
        static::me()->p_setAddTagInfo($addInfo);
    }

    /**
     *
     * @since 1.9.1
     *
     * @param string $msg
     * @param int    $type
     */
    public function p_addUiMsg( $msg, $type )
    {
        switch ($type) {
            case static::UI_INFO:
            case static::UI_SUCC:
            case static::UI_ERR:
            case static::UI_WARN:
                $this->uiMsgs[$type][] = $msg;
                break;
            //no default
        }
    }

    /**
     *
     * @since 1.9.1
     *
     * @param LogEntry[]|object[] $entries  Array of objects that implement all
     *     LogEntry class public functions.
     *
     * @throws LSCMException  Thrown indirectly by $this->echoEntries() call.
     */
    public function p_echoEntries( array $entries )
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
     * @return bool
     */
    public function p_getBufferedEcho()
    {
        return $this->bufferedEcho;
    }

    /**
     *
     * @since 1.9.1
     *
     * @return bool
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
     * @param int $type
     *
     * @return string[]
     */
    public function p_getUiMsgs( $type )
    {
        switch ($type) {
            case static::UI_INFO:
            case static::UI_SUCC:
            case static::UI_ERR:
            case static::UI_WARN:
                return $this->uiMsgs[$type];

            default:
                return array();
        }
    }

    /**
     *
     * @since 1.9.1
     *
     * @param string $msg
     * @param int    $lvl
     *
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     */
    public function p_log( $msg, $lvl )
    {
        $this->log($msg, $lvl);
    }

    /**
     *
     * @since 1.9.1
     *
     * @param string $addInfo
     */
    public function p_setAddTagInfo( $addInfo )
    {
        $this->addTagInfo = $addInfo;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param string $logFile
     */
    public function p_setLogFile( $logFile )
    {
        $this->logFile = $logFile;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param int $logFileLvl
     */
    public function p_setLogFileLvl( $logFileLvl )
    {
        $this->logFileLvl = $logFileLvl;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param LogEntry[]|object[] $msgQueue
     */
    public function p_setMsgQueue( array $msgQueue )
    {
        $this->msgQueue = $msgQueue;
    }

    /**
     *
     * @since 1.9.1
     *
     * @param LogEntry[]|object[] $entries  Array of objects that implement all
     *     LogEntry class public functions.
     *
     * @throws LSCMException  Thrown indirectly by $this->writeToFile() call.
     */
    public function p_writeToFile( array $entries )
    {
        $this->writeToFile($entries);
    }

    /**
     *
     * @since 1.9
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     */
    public static function getAdditionalTagInfo()
    {
        return static::me()->p_getAddTagInfo();
    }

    /**
     *
     * @since 1.9
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     */
    public static function getLogFilePath()
    {
        return static::me()->p_getLogFile();
    }

    /**
     *
     * @since 1.9
     *
     * @return LogEntry[]|object[]  Array of objects that implement all LogEntry
     *     class public functions.
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     */
    public static function getLogMsgQueue()
    {
        return static::me()->p_getMsgQueue();
    }

    /**
     *
     * @param int $type
     *
     * @return string[]
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     */
    public static function getUiMsgs( $type )
    {
        return static::me()->p_getUiMsgs($type);
    }

    /**
     * Processes any buffered output, writing it to the log file, echoing it
     * out, or both.
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     * @throws LSCMException  Thrown indirectly by $m->p_writeToFile() call.
     * @throws LSCMException  Thrown indirectly by $m->p_echoEntries() call.
     */
    public static function processBuffer()
    {
        $clear = false;

        $m = static::me();

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
     *
     * @param string $msg
     * @param int    $type
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     */
    public static function addUiMsg( $msg, $type )
    {
        static::me()->p_addUiMsg($msg, $type);
    }

    /**
     * Calls addUiMsg() with message level static::UI_INFO.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::addUiMsg() call.
     */
    public static function uiInfo( $msg )
    {
        static::addUiMsg($msg, static::UI_INFO);
    }

    /**
     * Calls addUiMsg() with message level static::UI_SUCC.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::addUiMsg() call.
     */
    public static function uiSuccess( $msg )
    {
        static::addUiMsg($msg, static::UI_SUCC);
    }

    /**
     * Calls addUiMsg() with message level static::UI_ERR.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::addUiMsg() call.
     */
    public static function uiError( $msg )
    {
        static::addUiMsg($msg, static::UI_ERR);
    }

    /**
     * Calls addUiMsg() with message level static::UI_WARN.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::addUiMsg() call.
     */
    public static function uiWarning( $msg )
    {
        static::addUiMsg($msg, static::UI_WARN);
    }

    /**
     *
     * @param string $msg
     * @param int    $lvl
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     * @throws LSCMException  Thrown indirectly by static::me()->p_log() call.
     */
    public static function logMsg( $msg, $lvl )
    {
        static::me()->p_log($msg, $lvl);
    }

    /**
     * Calls logMsg() with message level static::L_ERROR.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::logMsg() call.
     */
    public static function error( $msg )
    {
        static::logMsg($msg, static::L_ERROR);
    }

    /**
     * Calls logMsg() with message level static::L_WARN.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::logMsg() call.
     */
    public static function warn( $msg )
    {
        static::logMsg($msg, static::L_WARN);
    }

    /**
     * Calls logMsg() with message level static::L_NOTICE.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::logMsg() call.
     */
    public static function notice( $msg )
    {
        static::logMsg($msg, static::L_NOTICE);
    }

    /**
     * Calls logMsg() with message level static::L_INFO.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::logMsg() call.
     */
    public static function info( $msg )
    {
        static::logMsg($msg, static::L_INFO);
    }

    /**
     * Calls logMsg() with message level static::L_VERBOSE.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::logMsg() call.
     */
    public static function verbose( $msg )
    {
        static::logMsg($msg, static::L_VERBOSE);
    }

    /**
     * Calls logMsg() with message level static::L_DEBUG.
     *
     * @param string $msg
     *
     * @throws LSCMException  Thrown indirectly by static::logMsg() call.
     */
    public static function debug( $msg )
    {
        static::logMsg($msg, static::L_DEBUG);
    }

    /**
     *
     * @return Logger|object  Object that implements all Logger class public
     *     functions.
     *
     * @throws LSCMException  Thrown when Logger instance has not yet been
     *     initialized.
     */
    protected static function me()
    {
        if ( static::$instance == null ) {
            throw new LSCMException(
                'Logger Uninitialized.',
                LSCMException::E_PROGRAM
            );
        }

        return static::$instance;
    }

    /**
     *
     * @param string $msg
     * @param int    $lvl
     *
     * @throws LSCMException  Thrown indirectly by $this->writeToFile() call.
     * @throws LSCMException  Thrown indirectly by $this->echoEntries() call.
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
     * @param LogEntry[]|object[] $entries  Array of objects that implements all
     *     LogEntry class public functions.
     *
     * @throws LSCMException  Thrown indirectly by $e->getOutput() call.
     */
    protected function writeToFile( array $entries )
    {
        $content = '';

        foreach ( $entries as $e ) {
            $content .= $e->getOutput($this->logFileLvl);
        }

        if ( $content != '' ) {

            if ( $this->logFile ) {
                file_put_contents(
                    $this->logFile,
                    $content,
                    FILE_APPEND | LOCK_EX
                );
            }
            else {
                error_log($content);
            }
        }
    }

    /**
     *
     * @param LogEntry[]|object[] $entries  Array of objects that implement all
     *     LogEntry class public functions.
     *
     * @throws LSCMException  Thrown indirectly by $entry->getOutput() call.
     */
    protected function echoEntries( array $entries )
    {
        foreach ( $entries as $entry ) {

            if ( ($msg = $entry->getOutput($this->logEchoLvl)) !== '' ) {
                echo $msg;
            }
        }
    }

    /**
     *
     * @param int $lvl
     *
     * @return string
     */
    public static function getLvlDescr( $lvl )
    {
        switch ($lvl) {

            case static::L_ERROR:
                return 'ERROR';

            case static::L_WARN:
                return 'WARN';

            case static::L_NOTICE:
                return 'NOTICE';

            case static::L_INFO:
                return 'INFO';

            case static::L_VERBOSE:
                return 'DETAIL';

            case static::L_DEBUG:
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
     * @deprecated 1.9.1  Deprecated on 11/22/19. Function is likely not needed
     *     after recent logger changes.
     *
     * @param int $lvl
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by static::me() call.
     */
    public static function setLogFileLvl( $lvl )
    {
        $lvl = (int)$lvl;

        if ( static::isValidLogFileLvl($lvl) ) {

            if ( $lvl > static::L_DEBUG ) {
                $lvl = static::L_DEBUG;
            }

            static::me()->p_setLogFileLvl($lvl);

            return true;
        }

        return false;
    }

    /**
     *
     * @param int $lvl
     *
     * @return bool
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
