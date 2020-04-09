<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2020
 * *******************************************
 */

namespace Lsc\Wp;

use \Exception;
use \Lsc\Wp\Context\Context;
use \Lsc\Wp\Context\ContextOption;
use \Lsc\Wp\Context\UserCLIContextOption;
use \Lsc\Wp\Panel\ControlPanel;

/**
 * Running as user - suexec
 */
class UserCommand
{

    const EXIT_ERROR = 1;
    const EXIT_SUCC = 2;
    const EXIT_FAIL = 4;
    const EXIT_INCR_SUCC = 8;
    const EXIT_INCR_FAIL = 16;
    const EXIT_INCR_BYPASS = 32;
    const RETURN_CODE_TIMEOUT = 124;
    const CMD_STATUS = 'status';
    const CMD_ENABLE = 'enable';
    const CMD_DIRECT_ENABLE = 'direct_enable';
    const CMD_MASS_ENABLE = 'mass_enable';
    const CMD_DISABLE = 'disable';
    const CMD_MASS_DISABLE = 'mass_disable';
    const CMD_UPGRADE = 'upgrade';
    const CMD_MASS_UPGRADE = 'mass_upgrade';
    const CMD_UPDATE_TRANSLATION = 'update_translation';
    const CMD_REMOVE_LSCWP_PLUGIN_FILES = 'remove_lscwp_plugin_files';
    const CMD_DASH_NOTIFY = 'dash_notify';
    const CMD_MASS_DASH_NOTIFY = 'mass_dash_notify';
    const CMD_DASH_DISABLE = 'dash_disable';
    const CMD_MASS_DASH_DISABLE = 'mass_dash_disable';
    const CMD_DASH_GET_MSG = 'dash_get_msg';
    const CMD_DASH_ADD_MSG = 'dash_add_msg';
    const CMD_DASH_DELETE_MSG = 'dash_delete_msg';

    /**
     * @since 1.12
     * @var string
     */
    const CMD_GET_QUICCLOUD_API_KEY = 'getQuicCloudApiKey';

    /**
     * @var bool
     */
    private $asUser = false;

    /**
     * @var WPInstall
     */
    private $currInstall;

    /**
     * @var string
     */
    private $action;

    /**
     * @var string[]
     */
    private $extraArgs;

    /**
     *
     * @param boolean $asUser
     * @throws LSCMException  Thrown indirectly.
     */
    private function __construct( $asUser = false )
    {
        $this->asUser = $asUser;

        if ( $asUser ) {
            require_once __DIR__ . '/../autoloader.php';
            date_default_timezone_set('UTC');
            Context::initialize(new UserCLIContextOption('userCommand'));
        }
    }

    /**
     * Handles logging unexpected error output (or not if too long) and returns
     * a crafted message to be displayed instead.
     *
     * @param WPInstall  $wpInstall  WordPress Installation object.
     * @param string     $err        Compiled error message.
     * @param int        $lines      Number of $output lines read into the
     *                               error msg.
     * @return string                Message to be displayed instead.
     * @throws LSCMException  Indirectly thrown by Logger::error().
     */
    private static function handleUnexpectedError( $wpInstall, &$err, $lines )
    {
        $msg = 'Unexpected Error Encountered!';
        $path = $wpInstall->getPath();

        /**
         * $lines > 500 are likely some custom code triggering a page render.
         * Throw out actual message in this case.
         */
        if ( $lines < 500 ) {
            $match = false;
            $commonErrs = array(
                WPInstall::ST_ERR_EXECMD_DB => 'Error establishing a database connection'
            );

            foreach ( $commonErrs as $statusBit => $commonErr ) {

                if ( strpos($err, $commonErr) !== false ) {

                    $wpInstall->unsetStatusBit(WPInstall::ST_ERR_EXECMD);
                    $wpInstall->setStatusBit($statusBit);

                    $msg .= " {$commonErr}.";
                    $match = true;
                    break;
                }
            }

            if ( !$match ) {
                Logger::error("{$path} - {$err}");
                return "{$msg} See " . ContextOption::LOG_FILE_NAME
                        . " for more information.";
            }
        }

        Logger::error("{$path} - {$msg}");
        return $msg;
    }

    /**
     *
     * @since 1.9
     *
     * @param WPInstall  $wpInstall
     * @param string     $output
     * @throws LSCMException  Indirectly thrown by
     *                        PluginVersion::retrieveTranslation(),
     *                        self::getIssueCmd(), Logger::debug(), and
     *                        PluginVersion::removeTranslationZip().
     */
    private static function handleGetTranslationOutput( WPInstall $wpInstall,
            $output )
    {
        $translationInfo = explode(' ', $output);
        $locale = $translationInfo[0];
        $lscwpVer = $translationInfo[1];

        if ( PluginVersion::retrieveTranslation($locale, $lscwpVer) ) {

            $subAction = self::CMD_UPDATE_TRANSLATION;
            $subCmd = self::getIssueCmd($subAction, $wpInstall);

            exec($subCmd, $subOutput, $subReturn_var);

            Logger::debug(
                    "Issue sub command {$subAction}={$subReturn_var} {$wpInstall}\n"
                    . "{$subCmd}");
            Logger::debug('sub output = ' . var_export($subOutput, true));

            foreach ( $subOutput as $subLine ) {

                if ( preg_match('/BAD_TRANSLATION=(.+)/', $subLine,
                        $m) ) {

                    $translationInfo = explode(' ', $m[1]);
                    $locale = $translationInfo[0];
                    $lscwpVer = $translationInfo[1];

                    PluginVersion::removeTranslationZip($locale,
                            $lscwpVer);
                }
            }
        }
    }

    /**
     *
     * @since 1.9
     *
     * @param WPInstall  $wpInstall
     * @param string     $line
     * @param int        $retStatus
     * @param int        $cmdStatus
     * @param string     $err
     * @return boolean
     * @throws LSCMException  Indirectly thrown by
     *                        $wpInstall->populateDataFromUrl() and
     *                        self::handleGetTranslationOutput().
     */
    private static function handleResultOutput( WPInstall $wpInstall, $line,
            &$retStatus, &$cmdStatus, &$err )
    {
        if ( preg_match('/SITEURL=(.+)/', $line, $m) ) {

            if ( !$wpInstall->populateDataFromUrl($m[1]) ) {
                /**
                 * Matching docroot could not be found, ignore other
                 * output. setCmdStatusAndMsg() etc already handled in
                 * setSiteUrl().
                 */
                return false;
            }
        }
        elseif ( preg_match('/STATUS=(.+)/', $line, $m) ) {
            $retStatus = (int)$m[1];
        }
        elseif ( preg_match('/MASS_INCR=(.+)/', $line, $m) ) {

            if ( $m[1] == 'SUCC' ) {
                $cmdStatus |= UserCommand::EXIT_INCR_SUCC;
            }
            elseif ( $m[1] == 'FAIL' ) {
                $cmdStatus |= UserCommand::EXIT_INCR_FAIL;
            }
            elseif( $m[1] = 'BYPASS' ) {
                $cmdStatus |= UserCommand::EXIT_INCR_BYPASS;
            }
        }
        elseif ( preg_match('/GET_TRANSLATION=(.+)/', $line, $m) ) {
            self::handleGetTranslationOutput($wpInstall, $m[1]);
        }
        else {
            $err .= "Unexpected result line: {$line}\n";
        }

        return true;
    }

    /**
     * @since 1.9
     *
     * @param WPInstall  $wpInstall
     * @throws LSCMException  Indirectly thrown by self::getIssueCmd() and
     *                        Logger::debug().
     */
    private static function removeLeftoverLscwpFiles( $wpInstall )
    {
        $subAction = self::CMD_REMOVE_LSCWP_PLUGIN_FILES;
        $subCmd = self::getIssueCmd($subAction, $wpInstall);

        exec($subCmd, $subOutput, $subReturn_var);

        Logger::debug("Issue sub command "
                . "{$subAction}={$subReturn_var} {$wpInstall}\n"
                . "{$subCmd}");
        Logger::debug('sub output = ' . var_export($subOutput, true));

        $wpInstall->removeNewLscwpFlagFile();
    }

    /**
     *
     * @param string     $action
     * @param WPInstall  $wpInstall
     * @param mixed[]    $extraArgs
     * @return string
     * @throws LSCMException  Indirectly thrown by
     *                        $wpInstall->getPhpBinary() and
     *                        Context::getOption().
     */
    protected static function getIssueCmd( $action, WPInstall $wpInstall,
            $extraArgs = array() )
    {
        $su = $wpInstall->getSuCmd();
        $timeout = ControlPanel::PHP_TIMEOUT;
        $phpBin = $wpInstall->getPhpBinary();
        $path = $wpInstall->getPath();
        $serverName = $wpInstall->getData(WPInstall::FLD_SERVERNAME);
        $env = Context::getOption()->getInvokerName();

        if ( $serverName === null ) {
            $serverName = $docRoot = 'x';
        }
        else {
            $docRoot = $wpInstall->getData(WPInstall::FLD_DOCROOT);

            if ( $docRoot === null ) {
                $docRoot = 'x';
            }
        }

        $modifier = implode(' ', $extraArgs);
        $file = __FILE__;

        return "{$su} -c \"cd {$path}/wp-admin && timeout {$timeout} {$phpBin} "
                . "{$file} {$action} {$path} {$docRoot} {$serverName} {$env}"
                . (($modifier !== '') ? " {$modifier}\"" : '"');
    }

    /**
     *
     * @since 1.12
     *
     * @param string     $action
     * @param WPInstall  $wpInstall
     * @param string[]   $extraArgs
     * @return null|mixed
     * @throws LSCMException  Thrown indirectly.
     */
    public static function getValueFromWordPress( $action,
            WPInstall $wpInstall, $extraArgs = array() )
    {
        $ret = null;

        if ( !self::preIssueValidation($action, $wpInstall, $extraArgs) ) {
            return $ret;
        }

        $cmd = self::getIssueCmd($action, $wpInstall, $extraArgs);

        exec($cmd, $output, $return_var);

        Logger::debug("getValueFromWordPress command "
            . "{$action}={$return_var} {$wpInstall}\n{$cmd}");
        Logger::debug('output = ' . var_export($output, true));

        $debug = $upgrade = $err = '';
        $curr = &$err;

        foreach ( $output as $line ) {

            /**
             * If this line is not present in output, did not return normally.
             * This line will appear after any [UPGRADE] output.
             */
            if ( strpos($line, 'LS UserCommand Output Start') !== false ) {
                continue;
            }
            elseif ( strpos($line, '[RESULT]') !== false ) {

                if ( preg_match('/API_KEY=(.+)/', $line, $m) ) {
                    $ret = $m[1];
                }
                else {
                    $err .= "Unexpected result line {$line}\n";
                }
            }
            elseif ( ($pos = strpos($line, '[DEBUG]')) !== false ) {
                $debug .= substr($line, $pos + 7) . "\n";
                $curr = &$debug;
            }
            elseif ( strpos($line, '[UPGRADE]') !== false ) {
                //Ignore this output
                $curr = &$upgrade;
            }
            else {
                $curr .= "{$line}\n";
            }
        }

        $path = $wpInstall->getPath();

        if ( $debug ) {
            Logger::logMsg("{$path} - {$debug}", Logger::L_DEBUG);
        }

        if ( $err ) {
            Logger::logMsg("{$path} - {$err}", Logger::L_ERROR);
        }

        return $ret;
    }

    /**
     *
     * @param string     $action
     * @param WPInstall  $wpInstall
     * @param string[]   $extraArgs
     * @return boolean
     * @throws LSCMException  Thrown indirectly.
     */
    public static function issue( $action, WPInstall $wpInstall,
            $extraArgs = array() )
    {
        if ( !self::preIssueValidation($action, $wpInstall, $extraArgs) ) {
            return false;
        }

        $cmd = self::getIssueCmd($action, $wpInstall, $extraArgs);

        exec($cmd, $output, $return_var);

        Logger::debug(
                "Issue command {$action}={$return_var} {$wpInstall}\n{$cmd}");
        Logger::debug('output = ' . var_export($output, true));

        if ( $wpInstall->hasNewLscwpFlagFile() ) {
            self::removeLeftoverLscwpFiles($wpInstall);
        }

        $errorStatus = $retStatus = $cmdStatus = 0;

        switch ( $return_var ) {
            case UserCommand::RETURN_CODE_TIMEOUT:
                $errorStatus |= WPInstall::ST_ERR_TIMEOUT;
                break;
            case UserCommand::EXIT_ERROR:
            case 255:
                $errorStatus |= WPInstall::ST_ERR_EXECMD;
                break;
            //no default
        }

        $isExpectedOutput = false;
        $unexpectedLines = 0;
        $succ = $upgrade = $err = $msg = $logMsg = '';
        $logLvl = -1;
        $curr = &$err;

        foreach ( $output as $line ) {

            /**
             * If this line is not present in output, did not return normally.
             * This line will appear after any [UPGRADE] output.
             */
            if ( strpos($line, 'LS UserCommand Output Start') !== false ) {
                $isExpectedOutput = true;
            }
            elseif ( strpos($line, '[RESULT]') !== false ) {

                if ( !self::handleResultOutput($wpInstall, $line, $retStatus,
                        $cmdStatus, $err) ) {

                    /**
                     * Problem handling RESULT output, ignore other output.
                     */
                    return false;
                }
            }
            elseif ( $pos = (strpos($line, '[SUCCESS]')) !== false ) {
                $succ .= substr($line, $pos + 9) . "\n";
                $curr = &$succ;
            }
            elseif ( ($pos = strpos($line, '[ERROR]')) !== false ) {
                $err .= substr($line, $pos + 7) . "\n";
                $curr = &$err;
            }
            elseif ( ($pos = strpos($line, '[LOG]')) !== false ) {

                if ( $logMsg != '' ) {
                    Logger::logMsg(trim($logMsg), $logLvl);
                    $logMsg = '';
                }

                if ( preg_match('/\[(\d+)\] (.+)/', $line, $m) ) {
                    $logLvl = $m[1];
                    $logMsg = "{$wpInstall->getPath()} - {$m[2]}\n";
                }

                $curr = &$logMsg;
            }
            elseif ( strpos($line, '[UPGRADE]') !== false ) {
                /**
                 * Ignore this output
                 */
                $curr = &$upgrade;
            }
            else {

                if ( !$isExpectedOutput ) {
                    $line = htmlentities($line);
                    $unexpectedLines++;
                }

                $curr .= "{$line}\n";
            }
        }

        if ( $logMsg != '' ) {
            Logger::logMsg(trim($logMsg), $logLvl);
        }

        if ( !$isExpectedOutput && !$errorStatus ) {
            $errorStatus |= WPInstall::ST_ERR_EXECMD;
        }

        if ( $errorStatus ) {
            $wpInstall->addUserFlagFile(false);
            $errorStatus |= WPInstall::ST_FLAGGED;

            $cmdStatus |= UserCommand::EXIT_INCR_FAIL;
        }

        $newStatus = ($errorStatus | $retStatus);

        if ( $newStatus != 0 ) {
            $wpInstall->setStatus($newStatus);
        }

        if ( $succ ) {
            $cmdStatus |= UserCommand::EXIT_SUCC;
            $msg = $succ;
        }

        if ( $err ) {

            if ( $return_var == UserCommand::EXIT_FAIL ) {
                $cmdStatus |= UserCommand::EXIT_FAIL;
            }
            else {
                $cmdStatus |= UserCommand::EXIT_ERROR;
            }

            if ( $isExpectedOutput ) {
                $msg = $err;
                Logger::error("{$wpInstall->getPath()} - {$err}");
            }
            else {
                $msg = self::handleUnexpectedError($wpInstall, $err,
                        $unexpectedLines);
            }
        }

        $wpInstall->setCmdStatusAndMsg($cmdStatus, $msg);

        return true;
    }

    /**
     *
     * @param string[]  $args
     * @return null|WPInstall
     */
    public static function newFromCmdArgs( &$args )
    {
        $wpPath = array_shift($args);

        if ( !$wpPath || !is_dir($wpPath) ) {
            return null;
        }

        $docRoot = array_shift($args);

        if ( !$docRoot ) {
            return null;
        }

        $serverName = array_shift($args);

        if ( !$serverName ) {
            return null;
        }

        /**
         * LSCWP_REF used by LSCWP plugin.
         */
        $env = array_shift($args);
        define('LSCWP_REF', $env);

        $install = new WPInstall($wpPath);

        if ( $docRoot != 'x' ) {
            $install->setDocRoot($docRoot);
        }

        if ( $serverName != 'x' ) {
            $install->setServerName($serverName);
        }

        return $install;
    }

    /**
     *
     * @param string     $action
     * @param WPInstall  $wpInstall
     * @param string[]   $extraArgs  Not used at the moment.
     * @return boolean
     * @throws LSCMException  Thrown directly and indirectly.
     *
     * @noinspection PhpUnusedParameterInspection
     */
    private static function preIssueValidation( $action, WPInstall $wpInstall,
            $extraArgs )
    {
        if ( !self::isSupportedIssueCmd($action) ) {
            throw new LSCMException("Illegal action {$action}.",
                    LSCMException::E_PROGRAM);
        }

        if ( !$wpInstall->hasValidPath() ) {
            return false;
        }

        switch ($action) {
            case self::CMD_MASS_ENABLE:
            case self::CMD_MASS_DISABLE:
            /** @noinspection PhpMissingBreakStatementInspection */
            case self::CMD_MASS_UPGRADE:

                if ( $wpInstall->hasFlagFile() ) {
                    Logger::debug(
                            'Bypass mass operation for flagged install '
                            . $wpInstall);
                    return false;
                }

                //fallthrough
            case self::CMD_MASS_DASH_NOTIFY:
            case self::CMD_MASS_DASH_DISABLE:

                if ( $wpInstall->hasFatalError() ) {
                    $wpInstall->refreshStatus();

                    if ( $wpInstall->hasFatalError() ) {
                        $wpInstall->addUserFlagFile(false);

                        $msg = 'Bypassed mass operation for error install and '
                                . "flagged {$wpInstall}";
                        Logger::debug($msg);
                        return false;
                    }
                }

            //no default
        }

        if ( $action == self::CMD_DASH_NOTIFY
                || $action == self::CMD_MASS_DASH_NOTIFY ) {

            DashNotifier::prepLocalDashPluginFiles();
        }

        return true;
    }

    /**
     *
     * @since 1.9  Changed echo'd output format to include "[LOG][$lvl] $msg".
     *             Stopped echo'ing "[DEBUG] $msg" output.
     *
     * @return int
     */
    private function runAsUser()
    {
        try
        {
            $ret = 0;

            if ( $this->action == self::CMD_REMOVE_LSCWP_PLUGIN_FILES ) {
                $proc = WPCaller::getInstance($this->currInstall, false);

                $proc->removeLscwpPluginFiles();
                $ret = self::EXIT_SUCC;
            }
            else {
                $proc = WPCaller::getInstance($this->currInstall, true);

                switch ($this->action) {
                    case self::CMD_STATUS:
                        $ret = $proc->updateStatus(true);
                        break;

                    case self::CMD_ENABLE:
                        $ret = $proc->enable($this->extraArgs);
                        $this->currInstall->removeNewLscwpFlagFile();
                        break;

                    case self::CMD_DIRECT_ENABLE:
                        $ret = $proc->directEnable();
                        $this->currInstall->removeNewLscwpFlagFile();
                        break;

                    case self::CMD_MASS_ENABLE:
                        $ret = $proc->massEnable($this->extraArgs);
                        $this->currInstall->removeNewLscwpFlagFile();
                        break;

                    case self::CMD_DISABLE:
                        $ret = $proc->disable($this->extraArgs);
                        break;

                    case self::CMD_MASS_DISABLE:
                        $ret = $proc->massDisable($this->extraArgs);
                        break;

                    case self::CMD_UPGRADE:
                        $ret = $proc->upgrade($this->extraArgs);
                        break;

                    case self::CMD_MASS_UPGRADE:
                        $ret = $proc->massUpgrade($this->extraArgs);
                        break;

                    case self::CMD_UPDATE_TRANSLATION:
                        $proc->updateTranslationFiles();
                        $ret = self::EXIT_SUCC;
                        break;

                    case self::CMD_DASH_NOTIFY:
                        $ret = $proc->dashNotify($this->extraArgs);
                        break;

                    case self::CMD_MASS_DASH_NOTIFY:
                        $ret = $proc->massDashNotify($this->extraArgs);
                        break;

                    case self::CMD_DASH_DISABLE:
                        $ret = $proc->dashDisable($this->extraArgs);
                        break;

                    case self::CMD_MASS_DASH_DISABLE:
                        $ret = $proc->massDashDisable($this->extraArgs);
                        break;

                    case self::CMD_GET_QUICCLOUD_API_KEY:
                        $proc->getQuicCloudAPIKey(true);
                        $ret = self::EXIT_SUCC;
                        break;

                    //no default
                }
            }

            echo "LS UserCommand Output Start\n";

            foreach ( $proc->getOutputResult() as $key => $value ) {
                echo "[RESULT] {$key}={$value}\n";
            }

            foreach ( Logger::getUiMsgs(Logger::UI_SUCC) as $msg ) {
                echo "[SUCCESS] {$msg}\n";
            }

            foreach ( Logger::getUiMsgs(Logger::UI_ERR) as $msg ) {
                echo "[ERROR] {$msg}\n";
            }

            foreach ( Logger::getLogMsgQueue() as $logEntry ) {
                $lvl = $logEntry->getLvl();
                $msg = $logEntry->getMsg();

                echo "[LOG][{$lvl}] {$msg}\n";
            }
        }
        catch ( Exception $e )
        {
            $ret = UserCommand::EXIT_ERROR;

            if ( $e instanceof LSCMException
                    && $e->getCode() == LSCMException::E_NON_FATAL ) {

                $ret = UserCommand::EXIT_FAIL;
            }

            echo "LS UserCommand Output Start\n";
            echo "[ERROR] {$e->getMessage()}\n";
        }

        return $ret;
    }

    /**
     *
     * @param string $action
     * @return boolean
     */
    private static function isSupportedIssueCmd( $action )
    {
        $supported = array(
            self::CMD_STATUS,
            self::CMD_ENABLE,
            self::CMD_DIRECT_ENABLE,
            self::CMD_MASS_ENABLE,
            self::CMD_DISABLE,
            self::CMD_MASS_DISABLE,
            self::CMD_UPGRADE,
            self::CMD_MASS_UPGRADE,
            self::CMD_UPDATE_TRANSLATION,
            self::CMD_REMOVE_LSCWP_PLUGIN_FILES,
            self::CMD_DASH_NOTIFY,
            self::CMD_MASS_DASH_NOTIFY,
            self::CMD_DASH_DISABLE,
            self::CMD_MASS_DASH_DISABLE,
            self::CMD_GET_QUICCLOUD_API_KEY
        );

        return in_array($action, $supported);
    }

    /**
     *
     * @param string[] $args
     * @return boolean
     */
    private function initArgs( $args )
    {
        $action = array_shift($args);

        if ( self::isSupportedIssueCmd($action) ) {
            $this->action = $action;

            if ( $install = self::newFromCmdArgs($args) ) {
                $this->currInstall = $install;
                $this->extraArgs = $args;
                return true;
            }
        }

        return false;
    }

    /**
     *
     * @return UserCommand
     * @throws LSCMException  Thrown indirectly.
     */
    private static function getUserCommand()
    {
        /**
         * Check if invoked from shell.
         */
        if ( empty($_SERVER['argv']) ) {
            return null;
        }

        $args = $_SERVER['argv'];
        $script = array_shift($args);

        if ( $script != __FILE__ ) {
            return null;
        }

        $instance = new self(true);

        if ( !$instance->initArgs($args) ) {
            echo 'illegal input ' . implode(' ', $_SERVER['argv']);
            exit(self::EXIT_ERROR);
        }

        return $instance;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly.
     */
    public static function run()
    {
        if ( $cmd = self::getUserCommand() ) {

            if ( !defined('LSCM_RUN_AS_USER') ) {
                define('LSCM_RUN_AS_USER', 1);
            }

            $ret = $cmd->runAsUser();
            exit($ret);
        }
    }

}

/**
 * This should only be invoked from command line.
 *
 * @noinspection PhpUnhandledExceptionInspection
 */
UserCommand::run();
