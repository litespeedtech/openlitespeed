<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2022
 * *******************************************
 */

namespace Lsc\Wp;

use Lsc\Wp\Context\Context;
use Lsc\Wp\Panel\ControlPanel;

/**
 * map to data file
 */
class WPInstallStorage
{

    /**
     * @var string
     */
    const CMD_ADD_CUST_WPINSTALLS = 'addCustWPInstalls';

    /**
     * @since 1.13.3
     * @var string
     */
    const CMD_ADD_NEW_WPINSTALL = 'addNewWPInstall';

    /**
     * @deprecated 1.13.3  Use CMD_DISCOVER_NEW2 instead.
     * @var string
     */
    const CMD_DISCOVER_NEW = 'discoverNew';

    /**
     * @since 1.13.3
     * @var string
     */
    const CMD_DISCOVER_NEW2 = 'discoverNew2';

    /**
     * @var string
     */
    const CMD_FLAG = 'flag';

    /**
     * @var string
     */
    const CMD_MASS_UNFLAG = 'mass_unflag';

    /**
     * @deprecated 1.13.3  Use CMD_SCAN2 instead for now.
     * @var string
     */
    const CMD_SCAN = 'scan';

    /**
     * @since 1.13.3
     * @var string
     */
    const CMD_SCAN2 = 'scan2';

    /**
     * @var string
     */
    const CMD_UNFLAG = 'unflag';

    /**
     * @var string
     */
    const DATA_VERSION = '1.5';

    /**
     * @var int
     */
    const ERR_NOT_EXIST = 1;

    /**
     * @var int
     */
    const ERR_CORRUPTED = 2;

    /**
     * @var int
     */
    const ERR_VERSION_HIGH = 3;

    /**
     * @var int
     */
    const ERR_VERSION_LOW = 4;

    /**
     * @var string
     */
    protected $dataFile;

    /**
     * @var string
     */
    protected $customDataFile;

    /**
     * @var null|WPInstall[]  Key is path
     */
    protected $wpInstalls = null;

    /**
     * @var null|WPInstall[]  Key is path
     */
    protected $custWpInstalls = null;

    /**
     * @var int
     */
    protected $error;

    /**
     * @var WPInstall[]
     */
    protected $workingQueue = array();

    /**
     *
     * @param string  $dataFile
     * @param string  $custDataFile
     * @throws LSCMException  Thrown indirectly.
     */
    public function __construct( $dataFile, $custDataFile = '' )
    {
        $this->dataFile = $dataFile;
        $this->customDataFile = $custDataFile;
        $this->error = $this->init();
    }

    /**
     *
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    protected function init()
    {
        $dataExists = false;

        try {
            if ( file_exists($this->dataFile) ) {
                $dataExists = true;
                $this->wpInstalls = $this->getDataFileData($this->dataFile);
            }

            if ( $this->customDataFile != ''
                    && file_exists($this->customDataFile) ) {

                $dataExists = true;
                $this->custWpInstalls =
                    $this->getDataFileData($this->customDataFile);
            }
        }
        catch ( LSCMException $e ) {
            Logger::debug($e->getMessage());
            return $e->getCode();
        }

        if ( !$dataExists ) {
            return self::ERR_NOT_EXIST;
        }

        return 0;
    }

    /**
     *
     * @param string  $dataFile
     * @return WPInstall[]
     * @throws LSCMException  Thrown indirectly.
     */
    protected function getDataFileData( $dataFile )
    {
        $content = file_get_contents($dataFile);

        if ( ($data = json_decode($content, true)) === null ) {
            /*
             * Data file may be in old serialized format. Try unserializing.
             */
             $data = unserialize($content);
        }

        if ( $data === false || !is_array($data) || !isset($data['__VER__']) ) {
            throw new LSCMException(
                "$dataFile - Data is corrupt.",
                self::ERR_CORRUPTED
            );
        }

        if ( ($err = $this->verifyDataFileVer($dataFile, $data['__VER__'])) ) {
            throw new LSCMException(
                "$dataFile - Data file version issue.",
                $err
            );
        }

        unset($data['__VER__']);

        $wpInstalls = array();

        foreach ( $data as $utf8Path => $idata ) {
            $path = utf8_decode($utf8Path);
            $i = new WPInstall($path);

            $idata[WPInstall::FLD_SITEURL] =
                    urldecode($idata[WPInstall::FLD_SITEURL]);
            $idata[WPInstall::FLD_SERVERNAME] =
                    urldecode($idata[WPInstall::FLD_SERVERNAME]);

            $i->initData($idata);
            $wpInstalls[$path] = $i;
        }

        return $wpInstalls;
    }

    /**
     *
     * @return int
     */
    public function getError()
    {
        return $this->error;
    }

    /**
     *
     * @param bool  $nonFatalOnly
     * @return int
     */
    public function getCount( $nonFatalOnly = false )
    {
        $count = 0;

        if ( !$nonFatalOnly ) {

            if ( $this->wpInstalls != null ) {
                $count += count($this->wpInstalls);
            }

            if ( $this->custWpInstalls != null ) {
                $count += count($this->custWpInstalls);
            }
        }
        else {

            foreach ( $this->wpInstalls as $install ) {

                if ( !$install->hasFatalError() ) {
                    $count++;
                }
            }

            foreach ( $this->custWpInstalls as $custInstall ) {

                if ( !$custInstall->hasFatalError() ) {
                    $count++;
                }
            }
        }

        return $count;
    }

    /**
     *
     * @return null|WPInstall[]
     *
     * @noinspection PhpUnused
     */
    public function getWPInstalls()
    {
        return $this->wpInstalls;
    }

    /**
     *
     * @return null|WPInstall[]
     *
     * @noinspection PhpUnused
     */
    public function getCustWPInstalls()
    {
        return $this->custWpInstalls;
    }

    /**
     *
     * @return null|WPInstall[]
     */
    public function getAllWPInstalls()
    {
        if ( $this->wpInstalls != null ) {

            if ( $this->custWpInstalls != null ) {
                return array_merge($this->wpInstalls, $this->custWpInstalls);
            }
            else {
                return $this->wpInstalls;
            }
        }
        elseif ( $this->custWpInstalls != null ) {
            return $this->custWpInstalls;
        }
        else {
            return null;
        }
    }

    /**
     * Get all known WPInstall paths.
     *
     * @return string[]
     */
    public function getPaths()
    {
        $paths = array();

        if ( $this->wpInstalls != null ) {
            $paths = array_keys($this->wpInstalls);
        }

        if ( $this->custWpInstalls != null ) {
            $paths = array_merge($paths, array_keys($this->custWpInstalls));
        }

        return $paths;
    }

    /**
     *
     * @param string  $path
     * @return WPInstall|null
     */
    public function getWPInstall( $path )
    {
        if ( ($realPath = realpath($path)) === false ) {
            $index = $path;
        }
        else {
            $index = $realPath;
        }

        if ( isset($this->wpInstalls[$index]) ) {
            return $this->wpInstalls[$index];
        }
        elseif ( isset($this->custWpInstalls[$index]) ) {
            return $this->custWpInstalls[$index];
        }

        return null;
    }

    /**
     *
     * @return WPInstall[]
     */
    public function getWorkingQueue()
    {
        return $this->workingQueue;
    }

    /**
     *
     * @param WPInstall  $wpInstall
     */
    public function addWPInstall( WPInstall $wpInstall )
    {
        $this->wpInstalls[$wpInstall->getPath()] = $wpInstall;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly.
     */
    public function syncToDisk()
    {
        $this->saveDataFile($this->dataFile, $this->wpInstalls);

        if ( $this->customDataFile != '' ) {
            $this->saveDataFile($this->customDataFile, $this->custWpInstalls);
        }
    }

    /**
     *
     * @param string       $dataFile
     * @param WPInstall[]  $wpInstalls
     * @throws LSCMException  Thrown indirectly.
     */
    protected function saveDataFile( $dataFile, $wpInstalls )
    {
        $data = array( '__VER__' => self::DATA_VERSION );

        if ( !empty($wpInstalls) ) {

            foreach ( $wpInstalls as $path => $install ) {

                if ( !$install->shouldRemove() ) {
                    $utf8Path = utf8_encode($path);

                    $data[$utf8Path] = $install->getData();

                    $siteUrl = &$data[$utf8Path][WPInstall::FLD_SITEURL];
                    $siteUrl = urlencode($siteUrl);

                    $serverName = &$data[$utf8Path][WPInstall::FLD_SERVERNAME];
                    $serverName = urlencode($serverName);
                }
            }

            ksort($data);
        }

        $file_str = json_encode($data);
        file_put_contents($dataFile, $file_str, LOCK_EX);
        chmod($dataFile, 0600);

        $this->log("Data file saved $dataFile", Logger::L_DEBUG);
    }

    /**
     *
     * @param string  $dataFile
     * @param string  $dataFileVer
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    protected function verifyDataFileVer( $dataFile, $dataFileVer )
    {
        $res = version_compare($dataFileVer, self::DATA_VERSION);

        if ( $res == 1 ) {
            Logger::info(
                'Data file version is higher than expected and cannot be used.'
            );

            return self::ERR_VERSION_HIGH;
        }

        if ( $res == -1 && !$this->updateDataFile($dataFile, $dataFileVer) ) {
            return self::ERR_VERSION_LOW;
        }

        return 0;
    }

    /**
     *
     * @param string  $dataFile
     * @param string  $dataFileVer
     * @return bool
     * @throws LSCMException  Thrown indirectly.
     */
    public static function updateDataFile( $dataFile, $dataFileVer )
    {
        Logger::info(
            "$dataFile - Old data file version detected. Attempting to "
                . 'update...'
        );

        /**
         * Currently no versions are upgradeable to 1.5
         */
        $updatableVersions = array();

        if ( ! in_array($dataFileVer, $updatableVersions)
                || ! Util::createBackup($dataFile) ) {

            Logger::error(
                "$dataFile - Data file could not be updated to version "
                    . self::DATA_VERSION
            );

            return false;
        }

        /**
         * Upgrade funcs will be called here.
         */

        return true;
    }

    /**
     *
     * @param string  $action
     * @return string[]
     * @throws LSCMException  Thrown indirectly.
     */
    protected function prepareActionItems( $action )
    {
        switch ($action) {
            case self::CMD_SCAN:
            case self::CMD_SCAN2:
            case self::CMD_DISCOVER_NEW:
            case self::CMD_DISCOVER_NEW2:

                try
                {
                    return ControlPanel::getClassInstance()->getDocRoots();
                }
                catch ( LSCMException $e )
                {
                    throw new LSCMException(
                        $e->getMessage()
                            . " Could not prepare $action action items."
                    );
                }

            case UserCommand::CMD_MASS_ENABLE:
            case UserCommand::CMD_MASS_DISABLE:
            case UserCommand::CMD_MASS_UPGRADE:
            case UserCommand::CMD_MASS_DASH_NOTIFY:
            case UserCommand::CMD_MASS_DASH_DISABLE:
            case self::CMD_MASS_UNFLAG:
                return $this->getPaths();
            default:
                throw new LSCMException('Missing parameter(s).');
        }
    }

    /**
     *
     * @param string    $action
     * @param string    $path
     * @param string[]  $extraArgs
     * @throws LSCMException  Thrown indirectly.
     */
    protected function doWPInstallAction( $action, $path, $extraArgs )
    {
        if ( ($wpInstall = $this->getWPInstall($path)) == null ) {
            $wpInstall = new WPInstall($path);
            $this->addWPInstall($wpInstall);
        }

        switch ($action) {

            case self::CMD_FLAG:

                if ( !$wpInstall->hasValidPath() ) {
                    return;
                }

                if ( $wpInstall->addUserFlagFile(false) ) {
                    $wpInstall->setCmdStatusAndMsg(
                        UserCommand::EXIT_SUCC,
                        'Flag file set'
                    );
                }
                else {
                    $wpInstall->setCmdStatusAndMsg(
                        UserCommand::EXIT_FAIL,
                        'Could not create flag file'
                    );
                }

                $this->workingQueue[$path] = $wpInstall;

                return;

            case self::CMD_UNFLAG:
            case self::CMD_MASS_UNFLAG:

                if ( !$wpInstall->hasValidPath() ) {
                    return;
                }

                $wpInstall->removeFlagFile();

                $wpInstall->setCmdStatusAndMsg(
                    UserCommand::EXIT_SUCC,
                    'Flag file unset'
                );

                $this->workingQueue[$path] = $wpInstall;

                return;

            case UserCommand::CMD_ENABLE:
            case UserCommand::CMD_DISABLE:
            case UserCommand::CMD_DASH_NOTIFY:
            case UserCommand::CMD_DASH_DISABLE:

                if ( $wpInstall->hasFatalError() ) {
                    $wpInstall->refreshStatus();

                    if ( $wpInstall->hasFatalError() ) {
                        $wpInstall->addUserFlagFile(false);

                        $wpInstall->setCmdStatusAndMsg(
                            UserCommand::EXIT_FAIL,
                            'Install skipped and flagged due to Error status.'
                        );

                        $this->workingQueue[$path] = $wpInstall;

                        return;
                    }
                }

                break;

            case UserCommand::CMD_MASS_UPGRADE:
                $allowedVers =
                    PluginVersion::getInstance()->getAllowedVersions();

                if ( !in_array($extraArgs[1], $allowedVers) ) {
                    throw new LSCMException(
                        'Selected LSCWP version ('
                            . htmlspecialchars($extraArgs[1]) . ') is invalid.'
                    );
                }
                break;

            //no default
        }

        if ( UserCommand::issue($action, $wpInstall, $extraArgs) ) {

            if ( $action == UserCommand::CMD_MASS_UPGRADE
                    && ($wpInstall->getCmdStatus() & UserCommand::EXIT_FAIL)
                    && preg_match('/Download failed. Not Found/', $wpInstall->getCmdMsg()) ) {

                $this->syncToDisk();

                throw new LSCMException(
                    'Could not download version '
                        . htmlspecialchars($extraArgs[1]) . '.'
                );
            }

            if ( $action == UserCommand::CMD_MASS_ENABLE
                    && ($wpInstall->getCmdStatus() & UserCommand::EXIT_FAIL)
                    && preg_match('/Source Package not available/', $wpInstall->getCmdMsg()) ) {

                $this->syncToDisk();

                throw new LSCMException($wpInstall->getCmdMsg());
            }

            $this->workingQueue[$path] = $wpInstall;
        }
    }

    /**
     *
     * @param string               $action
     * @param null|string[]        $list
     * @param string[]|string[][]  $extraArgs
     * @return string[]
     * @throws LSCMException  Thrown indirectly.
     */
    public function doAction( $action, $list, $extraArgs = array() )
    {
        if ( $list === null ) {
            $list = $this->prepareActionItems($action);
        }

        $count = count($list);
        $this->log("doAction {$action} for {$count} items", Logger::L_VERBOSE);
        $endTime = $count > 1 ? Context::getActionTimeout() : 0;
        $finishedList = array();

        switch ( $action ) {

            case self::CMD_SCAN:
            case self::CMD_DISCOVER_NEW:
                $forceRefresh = ($action == self::CMD_SCAN);

                foreach ( $list as $path ) {
                    $this->scan($path, $forceRefresh);

                    $finishedList[] = $path;

                    if ( $endTime && time() >= $endTime ) {
                        break;
                    }
                }

                break;

            case self::CMD_ADD_NEW_WPINSTALL:

                foreach ( $list as $path) {
                    $this->addNewWPInstall($path);

                    $finishedList[] = $path;

                    if ( $endTime && time() >= $endTime ) {
                        break;
                    }
                }

                break;

            case self::CMD_ADD_CUST_WPINSTALLS:
                $wpInstallsInfo = $extraArgs[0];

                $this->addCustomInstallations($wpInstallsInfo);
                break;

            default:

                if ( $action == UserCommand::CMD_ENABLE
                        || $action == UserCommand::CMD_MASS_ENABLE ) {

                    /**
                     * Ensure that current version is locally downloaded.
                     */
                    $currVer = PluginVersion::getCurrentVersion();
                    PluginVersion::getInstance()->setActiveVersion($currVer);
                }

                foreach ( $list as $path ) {
                    $this->doWPInstallAction($action, $path, $extraArgs);

                    $finishedList[] = $path;

                    if ( $endTime && time() >= $endTime ) {
                        break;
                    }
                }
        }

        $this->syncToDisk();

        if ( $action == self::CMD_SCAN || $action == self::CMD_SCAN2 ) {
            /**
             * Explicitly clear any data file errors after scanning in case of
             * multiple actions performed in the same process (cli).
             */
            $this->error = 0;
        }

        return $finishedList;
    }

    /**
     *
     * @deprecated 1.13.3  Use $this->scan2() instead.
     *
     * @param string  $docroot
     * @param bool    $forceRefresh
     * @return void
     * @throws LSCMException  Thrown indirectly.
     */
    protected function scan( $docroot, $forceRefresh = false )
    {
        $depth = Context::getScanDepth();
        $cmd = "find -L $docroot -maxdepth $depth -name wp-admin -print";
        $directories = shell_exec($cmd);

        /**
         * Example:
         * /home/user/public_html/wordpress/wp-admin
         * /home/user/public_html/blog/wp-admin
         * /home/user/public_html/wp/wp-admin
         */
        $hasMatches = preg_match_all(
            "|$docroot(.*)(?=/wp-admin)|",
            $directories,
            $matches
        );

        if ( ! $hasMatches ) {
            /**
             *  Nothing found.
             */
            return;
        }

        foreach ( $matches[1] as $path ) {
            $wp_path = realpath($docroot . $path);
            $refresh = $forceRefresh;

            if ( !isset($this->wpInstalls[$wp_path]) ) {
                $this->wpInstalls[$wp_path] = new WPInstall($wp_path);
                $refresh = true;
                $this->log(
                    "New installation found: $wp_path",
                    Logger::L_INFO
                );

                if ( $this->custWpInstalls != null &&
                        isset($this->custWpInstalls[$wp_path]) ) {

                    unset($this->custWpInstalls[$wp_path]);

                    $this->log(
                        "Installation removed from custom data file: $wp_path",
                        Logger::L_INFO
                    );
                }
            }
            else {
                $this->log(
                    "Installation already found: $wp_path",
                    Logger::L_DEBUG
                );
            }

            if ( $refresh ) {
                $this->wpInstalls[$wp_path]->refreshStatus();
                $this->workingQueue[$wp_path] = $this->wpInstalls[$wp_path];
            }
        }
    }

    /**
     *
     * @since 1.13.3
     *
     * @param string   $docroot
     * @return string[]
     * @throws LSCMException  Thrown indirectly.
     */
    public function scan2( $docroot )
    {
        $depth = Context::getScanDepth();
        $cmd = "find -L $docroot -maxdepth $depth -name wp-admin -print";
        $directories = shell_exec($cmd);

        /**
         * Example:
         * /home/user/public_html/wordpress/wp-admin
         * /home/user/public_html/blog/wp-admin
         * /home/user/public_html/wp/wp-admin
         */
        $hasMatches = preg_match_all(
            "|$docroot(.*)(?=/wp-admin)|",
            $directories,
            $matches
        );

        if ( ! $hasMatches ) {
            /**
             *  Nothing found.
             */
            return array();
        }

        $wpPaths = array();

        foreach ( $matches[1] as $path ) {
            $wpPath = realpath($docroot . $path);
            $wpPaths[] = $wpPath;
        }

        return $wpPaths;
    }

    /**
     * Add a new WPInstall object to WPInstallStorage's $wpInstalls[] given a
     * path to a WordPress installation and refresh it's status . If a WPInstall
     * object already exists for the given path, refresh it's status.
     *
     * @since 1.13.3
     *
     * @param string  $wpPath
     * @throws LSCMException  Thrown indirectly.
     */
    protected function addNewWPInstall( $wpPath )
    {
        if ( ($realPath = realpath($wpPath)) !== false ) {
            $wpPath = $realPath;
        }

        if ( !isset($this->wpInstalls[$wpPath]) ) {
            $this->wpInstalls[$wpPath] = new WPInstall($wpPath);
            $this->log("New installation found: $wpPath", Logger::L_INFO);

            if ( $this->custWpInstalls != null &&
                isset($this->custWpInstalls[$wpPath]) ) {

                unset($this->custWpInstalls[$wpPath]);

                $this->log(
                    "Installation removed from custom data file: $wpPath",
                    Logger::L_INFO
                );
            }
        }
        else {
            $this->log(
                "Installation already found: $wpPath",
                Logger::L_DEBUG
            );
        }

        $this->wpInstalls[$wpPath]->refreshStatus();
        $this->workingQueue[$wpPath] = $this->wpInstalls[$wpPath];
    }

    /**
     *
     * @param string[]  $wpInstallsInfo
     * @return void
     * @throws LSCMException  Thrown indirectly.
     */
    protected function addCustomInstallations( $wpInstallsInfo )
    {
        if ( $this->customDataFile == '' ) {
            $this->log(
                'No custom data file set, could not add custom Installation.',
                Logger::L_INFO
            );

            return;
        }

        if ( $this->custWpInstalls == null ) {
            $this->custWpInstalls = array();
        }

        $count = count($wpInstallsInfo);

        for ( $i = 0; $i < $count; $i++ ) {
            $infoString = $wpInstallsInfo[$i];
            $info = preg_split('/\s+/', trim($infoString));

            $line = $i + 1;

            if ( count($info) != 4 ) {
                $this->log(
                    'Incorrect number of values for custom installation input '
                        . "string on line $line. Skipping.",
                    Logger::L_INFO
                );

                continue;
            }

            $wpPath = $info[0];
            $docroot = $info[1];
            $serverName = $info[2];
            $siteUrl = $info[3];

            if ( !file_exists("$wpPath/wp-admin") ) {
                $this->log(
                    "No 'wp-admin' directory found for $wpPath on line "
                        . "$line. Skipping.",
                    Logger::L_INFO
                );

                continue;
            }

            if ( !(substr($wpPath, 0, strlen($docroot)) === $docroot) ) {
                $this->log(
                    "docroot not contained in $wpPath on line $line. "
                        . 'Skipping.',
                    Logger::L_INFO
                );

                continue;
            }

            if ( !isset($this->wpInstalls[$wpPath]) ) {
                $this->custWpInstalls[$wpPath] = new WPInstall($wpPath);
                $this->custWpInstalls[$wpPath]->setDocRoot($docroot);
                $this->custWpInstalls[$wpPath]->setServerName($serverName);
                $this->custWpInstalls[$wpPath]->setSiteUrlDirect($siteUrl);
                $this->custWpInstalls[$wpPath]->refreshStatus();

                $this->log(
                    "New installation added to custom data file: $wpPath",
                    Logger::L_INFO
                );
            }
            else {
                $this->log(
                    "Installation already found during scan: $wpPath. "
                        . 'Skipping.',
                    Logger::L_INFO
                );
            }
        }
    }

    /**
     * Get all WPInstall command messages as a key=>value array.
     *
     * @return string[][]
     */
    public function getAllCmdMsgs()
    {
        $succ = $fail = $err = array();

        foreach ( $this->workingQueue as $WPInstall ) {
            $cmdStatus = $WPInstall->getCmdStatus();

            if ( $cmdStatus & UserCommand::EXIT_SUCC ) {
                $msgType = &$succ;
            }
            elseif ( $cmdStatus & UserCommand::EXIT_FAIL ) {
                $msgType = &$fail;
            }
            elseif ( $cmdStatus & UserCommand::EXIT_ERROR ) {
                $msgType = &$err;
            }
            else {
                continue;
            }

            if ( $msg = $WPInstall->getCmdMsg() ) {
                $msgType[] = "{$WPInstall->getPath()} - $msg";
            }
        }

        return array( 'succ' => $succ, 'fail' => $fail, 'err' => $err );
    }

    /**
     *
     * @param string  $msg
     * @param int     $level
     * @throws LSCMException  Thrown indirectly.
     */
    protected function log( $msg, $level )
    {
        $msg = "WPInstallStorage - {$msg}";

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

}
