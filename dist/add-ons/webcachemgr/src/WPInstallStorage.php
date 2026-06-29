<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2026 LiteSpeed Technologies, Inc.
 * *******************************************
 */

namespace Lsc\Wp;

use Lsc\Wp\Context\Context;
use Lsc\Wp\Panel\ControlPanel;
use Lsc\Wp\ThirdParty\Polyfill\Utf8;

/**
 * map to data file
 */
class WPInstallStorage
{

    /**
     * @var string
     */
    const CMD_ADD_CUST_WPINSTALLS     = 'addCustWPInstalls';

    /**
     * @since 1.13.3
     * @var string
     */
    const CMD_ADD_NEW_WPINSTALL       = 'addNewWPInstall';

    /**
     * @deprecated 1.13.3  Use CMD_DISCOVER_NEW2 instead.
     * @var string
     */
    const CMD_DISCOVER_NEW            = 'discoverNew';

    /**
     * @since 1.14
     * @var string
     */
    const CMD_DISCOVER_NEW_AND_ENABLE = 'discoverNewAndEnable';

    /**
     * @since 1.13.3
     * @var string
     */
    const CMD_DISCOVER_NEW2           = 'discoverNew2';

    /**
     * @var string
     */
    const CMD_FLAG                    = 'flag';

    /**
     * @var string
     */
    const CMD_MASS_FLAG               = 'mass_flag';

    /**
     * @var string
     */
    const CMD_MASS_UNFLAG             = 'mass_unflag';

    /**
     * @deprecated 1.13.3  Use CMD_SCAN2 instead for now.
     * @var string
     */
    const CMD_SCAN                    = 'scan';

    /**
     * @since 1.13.3
     * @var string
     */
    const CMD_SCAN2                   = 'scan2';

    /**
     * @var string
     */
    const CMD_UNFLAG                  = 'unflag';

    /**
     * @var string
     */
    const DATA_VERSION = '1.5';

    /**
     * @var int
     */
    const ERR_NOT_EXIST    = 1;

    /**
     * @var int
     */
    const ERR_CORRUPTED    = 2;

    /**
     * @var int
     */
    const ERR_VERSION_HIGH = 3;

    /**
     * @var int
     */
    const ERR_VERSION_LOW  = 4;

    /**
     * @var string
     */
    protected $dataFile;

    /**
     * @var string
     */
    protected $customDataFile;

    /**
     * @var null|WPInstall[]  Key is the path to a WordPress installation.
     */
    protected $wpInstalls = null;

    /**
     * @var null|WPInstall[]  Key is the path to a WordPress installation.
     */
    protected $custWpInstalls = null;

    /**
     * @var int
     */
    protected $error;

    /**
     * @var WPInstall[]
     */
    protected $workingQueue = [];

    /**
     *
     * @param string $dataFile
     * @param string $custDataFile
     *
     * @throws LSCMException  Thrown indirectly by $this->init() call.
     */
    public function __construct( $dataFile, $custDataFile = '' )
    {
        $this->dataFile       = $dataFile;
        $this->customDataFile = $custDataFile;
        $this->error          = $this->init();
    }

    /**
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    protected function init()
    {
        $dataExists = false;

        try {

            if ( file_exists($this->dataFile) ) {
                $dataExists       = true;
                $this->wpInstalls = $this->getDataFileData($this->dataFile);
            }

            if (
                    $this->customDataFile != ''
                    &&
                    file_exists($this->customDataFile)
            ) {
                $dataExists           = true;
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
     * @since 1.15
     *
     * @param string $dataFile
     *
     * @return false|string
     */
    protected static function getDataFileContents( $dataFile )
    {
        return file_get_contents($dataFile);
    }

    /**
     *
     * @param string $dataFile
     *
     * @return WPInstall[]
     *
     * @throws LSCMException  Thrown when data file is corrupt.
     * @throws LSCMException  Thrown when there is a data file version issue.
     * @throws LSCMException  Thrown indirectly by $this->verifyDataFileVer()
     *     call.
     */
    protected function getDataFileData( $dataFile )
    {
        $content = static::getDataFileContents($dataFile);

        if ( ($data = json_decode($content, true)) === null ) {
            /**
             * Data file may be in the legacy serialized format (pre-v1.15).
             *
             * I-7: every cPanel-supported runtime is PHP 7+, so the previous
             * PHP_VERSION < 7.0 LSCMException branch was dead code. Calling
             * unserialize() with `'allowed_classes' => false` blocks object
             * instantiation, which is the only gadget path of concern here.
             */
            $data = unserialize($content, ['allowed_classes' => false]);
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

        $wpInstalls = [];

        foreach ( $data as $utf8Path => $idata ) {
            $path = Utf8::decode($utf8Path);
            $i    = new WPInstall($path);

            $siteUrl = isset($idata[WPInstall::FLD_SITEURL]) ? $idata[WPInstall::FLD_SITEURL] : null;
            $idata[WPInstall::FLD_SITEURL] =
                ($siteUrl !== null) ? urldecode($siteUrl) : null;
            $serverName = isset($idata[WPInstall::FLD_SERVERNAME]) ? $idata[WPInstall::FLD_SERVERNAME] : null;
            $idata[WPInstall::FLD_SERVERNAME] =
                ($serverName !== null) ? urldecode($serverName) : null;

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
     * @param bool $nonFatalOnly
     *
     * @return int
     */
    public function getCount( $nonFatalOnly = false )
    {
        $count = 0;

        if ( $this->wpInstalls != null ) {

            if ( $nonFatalOnly ) {

                foreach ( $this->wpInstalls as $install ) {

                    if ( !$install->hasFatalError() ) {
                        $count++;
                    }
                }
            }
            else {
                $count += count($this->wpInstalls);
            }
        }

        if ( $this->custWpInstalls != null ) {

            if ( $nonFatalOnly ) {

                foreach ( $this->custWpInstalls as $custInstall ) {

                    if ( !$custInstall->hasFatalError() ) {
                        $count++;
                    }
                }
            }
            else {
                $count += count($this->custWpInstalls);
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
        $paths = [];

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
     * @param string $path
     *
     * @return WPInstall|null
     */
    public function getWPInstall( $path )
    {
        /**
         * V9.5 — match the literal path against the canonical keyset first.
         * Stored keys are already canonical (WPInstall::init() canonicalises
         * and addWPInstall() keys by getPath()), so a caller passing a
         * canonical path (the flag-action case) resolves to its bound install
         * even if an intermediate component was swapped after the path was
         * enqueued. Resolving via realpath() first would follow such a swap to
         * a victim tree and miss the binding, yielding an unbound WPInstall
         * downstream whose null expectedOwnerUid would skip Layer 2 of
         * addUserFlagFile().
         */
        if ( isset($this->wpInstalls[$path]) ) {
            return $this->wpInstalls[$path];
        }
        elseif ( isset($this->custWpInstalls[$path]) ) {
            return $this->custWpInstalls[$path];
        }

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
     * @param WPInstall $wpInstall
     */
    public function addWPInstall( WPInstall $wpInstall )
    {
        $this->wpInstalls[$wpInstall->getPath()] = $wpInstall;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->saveDataFile() call.
     * @throws LSCMException  Thrown indirectly by $this->saveDataFile() call.
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
     * @param string           $dataFile
     * @param WPInstall[]|null $wpInstalls
     *
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     */
    protected function saveDataFile( $dataFile, $wpInstalls )
    {
        $data = [ '__VER__' => self::DATA_VERSION ];

        if ( !empty($wpInstalls) ) {

            foreach ( $wpInstalls as $path => $install ) {

                if ( !$install->shouldRemove() ) {
                    $utf8Path = Utf8::encode($path);

                    $data[$utf8Path] = $install->getData();

                    $siteUrl = &$data[$utf8Path][WPInstall::FLD_SITEURL];

                    if ( $siteUrl != null ) {
                        $siteUrl = urlencode($siteUrl);
                    }

                    $serverName = &$data[$utf8Path][WPInstall::FLD_SERVERNAME];

                    if ( $serverName != null ) {
                        $serverName = urlencode($serverName);
                    }
                }
            }

            ksort($data);
        }

        file_put_contents($dataFile, json_encode($data), LOCK_EX);
        chmod($dataFile, 0600);

        $this->log("Data file saved $dataFile", Logger::L_DEBUG);
    }

    /**
     *
     * @param string $dataFile
     * @param string $dataFileVer
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     * @throws LSCMException  Thrown indirectly by $this->updateDataFile() call.
     */
    protected function verifyDataFileVer( $dataFile, $dataFileVer )
    {
        $res = Util::betterVersionCompare($dataFileVer, self::DATA_VERSION);

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
     * @param string $dataFile
     * @param string $dataFileVer
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     * @throws LSCMException  Thrown indirectly by Util::createBackup() call.
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
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
        $updatableVersions = [];

        if (
                !in_array($dataFileVer, $updatableVersions)
                ||
                ! Util::createBackup($dataFile)
        ) {
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
     * @param string $action
     *
     * @return string[]
     *
     * @throws LSCMException  Thrown when "get docroots" command fails.
     * @throws LSCMException  Thrown when $action value is unsupported.
     */
    protected function prepareActionItems( $action )
    {
        switch ( $action ) {

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
     * @param string   $action
     * @param string   $path
     * @param string[] $extraArgs
     *
     * @throws LSCMException  Thrown when an invalid LSCWP version is selected
     *     in action UserCommand::CMD_MASS_UPGRADE.
     * @throws LSCMException  Thrown when LSCWP version fails to download in
     *     action UserCommand::CMD_MASS_UPGRADE.
     * @throws LSCMException  Thrown when LSCWP source package is not available
     *     in action UserCommand::CMD_MASS_UPGRADE.
     * @throws LSCMException  Thrown indirectly by $wpInstall->hasValidPath()
     *     call.
     * @throws LSCMException  Thrown indirectly by $wpInstall->addUserFlagFile()
     *     call.
     * @throws LSCMException  Thrown indirectly by $wpInstall->hasValidPath()
     *     call.
     * @throws LSCMException  Thrown indirectly by $wpInstall->refreshStatus()
     *     call.
     * @throws LSCMException  Thrown indirectly by $wpInstall->addUserFlagFile()
     *     call.
     * @throws LSCMException  Thrown indirectly by PluginVersion::getInstance()
     *     call.
     * @throws LSCMException  Thrown indirectly by
     *     PluginVersion::getInstance()->getAllowedVersions() call.
     * @throws LSCMException  Thrown indirectly by UserCommand::issue() call.
     * @throws LSCMException  Thrown indirectly by $this->syncToDisk() call.
     * @throws LSCMException  Thrown indirectly by $this->syncToDisk() call.
     */
    protected function doWPInstallAction( $action, $path, array $extraArgs )
    {
        if ( ($wpInstall = $this->getWPInstall($path)) == null ) {
            /**
             * V9.5 — for flag actions the path always originates from the
             * existing keyset (getPaths() canonical keys, or the single-flag
             * pre-validation in doSingleAction()). A miss here means the path
             * drifted between the time the list was built and this lookup —
             * indicative of a cross-tenant TOCTOU intermediate-component swap.
             * Constructing an unbound WPInstall would carry null
             * expectedOwnerUid, silently disabling Layer 2 of
             * addUserFlagFile() and allowing the privileged write to proceed
             * against an unverified (possibly swapped) path. Fail closed
             * instead.
             *
             * V9.6 — extend the fail-closed guard to every action that can
             * reach a root-context addUserFlagFile(false) on the
             * dispatcher-resolved object:
             *   - CMD_ENABLE/CMD_DISABLE/CMD_DASH_NOTIFY/CMD_DASH_DISABLE can
             *     call addUserFlagFile(false) directly when refreshStatus()
             *     still shows a fatal error.
             *   - All actions fed through UserCommand::issue() can call
             *     addUserFlagFile(false) in root context via its error handler.
             * These paths have the same unbound-object exposure as the explicit
             * flag actions (§13/§14 of the fix plan). The fallback bare
             * construct is kept for discovery/custom install actions whose
             * paths do not originate from the existing keyset.
             *
             * V9.7 — the V9.6 explicit OR-list was incomplete: CMD_STATUS
             * (and all other issue-able commands) share the same
             * addUserFlagFile(false) exposure via UserCommand::issue()'s error
             * handler, and CMD_STATUS is reachable via the single-action UI
             * path (refresh_status_single). Centralise: guard every command
             * accepted by UserCommand::isSupportedIssueCmd() plus the explicit
             * flag commands so any future addition to the issue() command set
             * is protected automatically.
             *
             * V9.8 — CMD_UNFLAG/CMD_MASS_UNFLAG added to the guard.  These
             * are not in isSupportedIssueCmd() (unflag is handled inline below,
             * not via UserCommand::issue()), so they must be listed explicitly.
             * An unbound WPInstall for an unflag miss would carry null
             * expectedOwnerUid, silently disabling the Layer 2 owner check in
             * the hardened removeFlagFile(false) path added in V9.8.
             */
            if (
                    $action === self::CMD_FLAG
                    || $action === self::CMD_MASS_FLAG
                    || $action === self::CMD_UNFLAG
                    || $action === self::CMD_MASS_UNFLAG
                    || UserCommand::isSupportedIssueCmd($action)
            ) {
                $this->log(
                    "Skipping $action: install path $path is not a known "
                        . 'install (possible cross-tenant TOCTOU — '
                        . 'intermediate component swapped after the action '
                        . 'list was built).',
                    Logger::L_INFO
                );
                return;
            }

            $wpInstall = new WPInstall($path);
            $this->addWPInstall($wpInstall);
        }

        switch ( $action ) {

            case self::CMD_FLAG:
            case self::CMD_MASS_FLAG:

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

                if ( $wpInstall->removeFlagFile(false) ) {
                    $wpInstall->setCmdStatusAndMsg(
                        UserCommand::EXIT_SUCC,
                        'Flag file unset'
                    );
                }
                else {
                    $wpInstall->setCmdStatusAndMsg(
                        UserCommand::EXIT_FAIL,
                        'Could not remove flag file'
                    );
                }

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
                $lscwpVer     = $extraArgs[1];
                $isAllowedVer = in_array(
                    $lscwpVer,
                    PluginVersion::getInstance()->getAllowedVersions()
                );

                if ( !$isAllowedVer ) {
                    throw new LSCMException(
                        'Selected LSCWP version ('
                            . htmlspecialchars($lscwpVer)
                            . ') is invalid.'
                    );
                }
                break;

            //no default
        }

        if ( UserCommand::issue($action, $wpInstall, $extraArgs) ) {

            if (
                    $action == UserCommand::CMD_MASS_UPGRADE
                    &&
                    ($wpInstall->getCmdStatus() & UserCommand::EXIT_FAIL)
                    &&
                    preg_match(
                        '/Download failed. Not Found/',
                        $wpInstall->getCmdMsg()
                    )
            ) {
                $this->syncToDisk();

                throw new LSCMException(
                    'Could not download version '
                        . htmlspecialchars($extraArgs[1])
                        . '.'
                );
            }

            if (
                    $action == UserCommand::CMD_MASS_ENABLE
                    &&
                    ($wpInstall->getCmdStatus() & UserCommand::EXIT_FAIL)
                    &&
                    preg_match(
                        '/Source Package not available/',
                        $wpInstall->getCmdMsg()
                    )
            ) {
                $this->syncToDisk();

                throw new LSCMException($wpInstall->getCmdMsg());
            }

            $this->workingQueue[$path] = $wpInstall;
        }
    }

    /**
     *
     * @param string              $action
     * @param null|string[]       $list
     * @param string[]|string[][] $extraArgs
     *
     * @return string[]
     *
     * @throws LSCMException  Thrown indirectly by $this->prepareActionItems()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by Context::getActionTimeout()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->scan() call.
     * @throws LSCMException  Thrown indirectly by $this->addNewWPInstall()
     *     call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->addCustomInstallations() call.
     * @throws LSCMException  Thrown indirectly by
     *     PluginVersion::getCurrentVersion() call.
     * @throws LSCMException  Thrown indirectly by PluginVersion::getInstance()
     *     call.
     * @throws LSCMException  Thrown indirectly by
     *     PluginVersion::getInstance()->setActiveVersion() call.
     * @throws LSCMException  Thrown indirectly by $this->doWPInstallAction()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->syncToDisk() call.
     */
    public function doAction( $action, $list, array $extraArgs = [] )
    {
        if ( $list === null ) {
            $list = $this->prepareActionItems($action);
        }

        $count = count($list);
        $this->log("doAction $action for $count items", Logger::L_VERBOSE);

        $endTime      = ($count > 1) ? Context::getActionTimeout() : 0;
        $finishedList = [];

        switch ( $action ) {

            case self::CMD_SCAN:
            case self::CMD_DISCOVER_NEW:

                foreach ( $list as $path ) {
                    $this->scan($path, ($action == self::CMD_SCAN));

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
                $this->addCustomInstallations($extraArgs[0]);
                break;

            default:

                if (
                        $action == UserCommand::CMD_ENABLE
                        ||
                        $action == UserCommand::CMD_MASS_ENABLE
                ) {
                    /**
                     * Ensure that the current version is locally downloaded.
                     */
                    PluginVersion::getInstance()
                    ->setActiveVersion(PluginVersion::getCurrentVersion())
                    ;
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
     * @since 1.17.10  Added 'find -- ' end-of-options guard.
     *
     * @param string $docroot
     * @param bool   $forceRefresh
     *
     * @return void
     *
     * @throws LSCMException  Thrown indirectly by Context::getScanDepth() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstalls[$wp_path]->refreshStatus() call.
     */
    protected function scan( $docroot, $forceRefresh = false )
    {
        /**
         * V6 (CWE-59 SSRF/symlink traversal) — use 'find -P' (the default; NOT
         * '-L') so find does not descend into symlinked directories. As root,
         * scanning a tenant-owned docroot with '-L' let a planted symlink (e.g.
         * public_html/x -> /root) surface a wp-admin that resolves outside the
         * tenant tree, steering subsequent root operations off-tree.
         * The '--' prevents a docroot starting with '-' from being misread as a
         * find option (escapeshellarg protects the shell, not find's own parser).
         */
        $directories = shell_exec(
            'find -P -- ' . escapeshellarg($docroot) . ' -maxdepth '
                . (int) Context::getScanDepth()
                . ' -name wp-admin -print'
        );

        $hasMatches = false;

        if ( $directories ) {

            /**
             * Example:
             * /home/user/public_html/wordpress/wp-admin
             * /home/user/public_html/blog/wp-admin
             * /home/user/public_html/wp/wp-admin
             */
            $hasMatches = preg_match_all(
                '|' . preg_quote($docroot, '|') . '(.*)(?=/wp-admin)|',
                $directories,
                $matches
            );
        }

        if ( ! $hasMatches ) {
            /**
             *  Nothing found.
             */
            return;
        }

        $realDocroot = realpath($docroot);

        /**
         * V9/V9.2 (CWE-59/CWE-367 cross-tenant TOCTOU) — capture the
         * panel-assigned docroot owner once before the loop.  The docroot is
         * invariant across iterations; reading it here avoids repeated lstat()
         * calls and makes the binding available to both new and existing
         * installs (V9.2 fix: previously only new installs were rebound,
         * leaving upgraded existing installs with null Layer 2 bindings).
         */
        $rootStat = @lstat($realDocroot);

        /** @noinspection PhpUndefinedVariableInspection */
        foreach ( $matches[1] as $path ) {
            $wp_path = realpath($docroot . $path);

            /**
             * V6 — defence-in-depth: drop any match that does not canonically
             * resolve to a location contained under the docroot, so a symlinked
             * leaf component cannot redirect a root operation off-tree.
             *
             * Note: this realpath() is a snapshot, not a lock. A path component
             * could be swapped for a symlink after this check. That residual
             * race is intentionally handled at the consumer, not here:
             *   - per-install enable/disable/upgrade/status run privilege-
             *     dropped as the install owner (UserCommand::runAsUser), where
             *     following an owner-planted symlink crosses no trust boundary;
             *   - the one root-context write into the untrusted tree,
             *     WPInstall::addUserFlagFile(), drops to install-owner
             *     credentials (V8) before the unlink and fopen so an
             *     intermediate-directory swap redirecting the path off-tree
             *     fails with EACCES (CWE-59/CWE-367 closed at the consumer).
             */
            if ( $wp_path === false || $realDocroot === false
                    || strpos($wp_path . '/', $realDocroot . '/') !== 0 ) {
                $this->log(
                    "Scan match not contained under docroot $docroot. Skipping.",
                    Logger::L_INFO
                );

                continue;
            }

            $refresh = $forceRefresh;

            if ( !isset($this->wpInstalls[$wp_path]) ) {
                $this->wpInstalls[$wp_path] = new WPInstall($wp_path);

                $refresh = true;
                $this->log(
                    "New installation found: $wp_path",
                    Logger::L_INFO
                );

                if (
                        $this->custWpInstalls != null
                        &&
                        isset($this->custWpInstalls[$wp_path])
                ) {
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

            /**
             * V9/V9.2 — bind expected owner unconditionally on every scan
             * match, not only newly discovered installs.  Existing installs
             * (loaded from the data file) must be rebound on each scan so
             * that Layer 2 (expected-owner equality check in addUserFlagFile)
             * fires for the root-context flag path after an upgrade or after
             * the first scan that discovered the install.
             */
            if ( $rootStat !== false ) {
                $this->wpInstalls[$wp_path]->setExpectedOwner(
                    $rootStat['uid'],
                    $rootStat['gid']
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
     * @since 1.15    Changed function visibility from 'public' to
     *     'public static'.
     * @since 1.17.10  Added 'find -- ' end-of-options guard.
     *
     * @param string $docroot
     *
     * @return string[]
     *
     * @throws LSCMException  Thrown indirectly by Context::getScanDepth() call.
     */
    public static function scan2( $docroot )
    {
        /**
         * V6 (CWE-59 SSRF/symlink traversal) — use 'find -P' (the default; NOT
         * '-L') so find does not descend into symlinked directories. As root,
         * scanning a tenant-owned docroot with '-L' let a planted symlink (e.g.
         * public_html/x -> /root) surface a wp-admin that resolves outside the
         * tenant tree, steering subsequent root operations off-tree.
         * The '--' prevents a docroot starting with '-' from being misread as a
         * find option (escapeshellarg protects the shell, not find's own parser).
         */
        $directories = shell_exec(
            'find -P -- ' . escapeshellarg($docroot) . ' -maxdepth '
                . (int) Context::getScanDepth()
                . ' -name wp-admin -print'
        );

        $hasMatches = false;

        if ( $directories ) {
            /**
             * Example:
             * /home/user/public_html/wordpress/wp-admin
             * /home/user/public_html/blog/wp-admin
             * /home/user/public_html/wp/wp-admin
             */
            $hasMatches = preg_match_all(
                '|' . preg_quote($docroot, '|') . '(.*)(?=/wp-admin)|',
                $directories,
                $matches
            );
        }

        if ( ! $hasMatches ) {
            /**
             *  Nothing found.
             */
            return [];
        }

        $wpPaths     = [];
        $realDocroot = realpath($docroot);

        /** @noinspection PhpUndefinedVariableInspection */
        foreach ( $matches[1] as $path ) {
            $wpPath = realpath($docroot . $path);

            /**
             * V6 — defence-in-depth: only return matches that canonically
             * resolve to a location contained under the docroot, so a symlinked
             * leaf component cannot redirect a root operation off-tree.
             *
             * Note: this realpath() is a snapshot, not a lock. A path component
             * could be swapped for a symlink after this check. That residual
             * race is intentionally handled at the consumer, not here:
             *   - per-install enable/disable/upgrade/status run privilege-
             *     dropped as the install owner (UserCommand::runAsUser), where
             *     following an owner-planted symlink crosses no trust boundary;
             *   - the one root-context write into the untrusted tree,
             *     WPInstall::addUserFlagFile(), drops to install-owner
             *     credentials (V8) before the unlink and fopen so an
             *     intermediate-directory swap redirecting the path off-tree
             *     fails with EACCES (CWE-59/CWE-367 closed at the consumer).
             */
            if ( $wpPath === false || $realDocroot === false
                    || strpos($wpPath . '/', $realDocroot . '/') !== 0 ) {
                continue;
            }

            $wpPaths[] = $wpPath;
        }

        return $wpPaths;
    }

    /**
     * Add a new WPInstall object to WPInstallStorage's $wpInstalls[] given a
     * path to a WordPress installation and refresh its status. If a WPInstall
     * object already exists for the given path, refresh its status.
     *
     * @since 1.13.3
     *
     * @param string $wpPath
     *
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstalls[$wpPath]->refreshStatus() call.
     */
    protected function addNewWPInstall( $wpPath )
    {
        /**
         * V9.4 (CWE-59/CWE-367 cross-tenant TOCTOU) — drift-rejection guard.
         *
         * scan2() returns canonical, docroot-contained paths (produced via
         * realpath() + containment check).  If realpath() here no longer
         * resolves to the same value, an intermediate directory component was
         * swapped (symlinked) between scan2's containment validation and this
         * invocation — the TOCTOU window that spans the WHM UI's multi-request
         * handoff (scan request stores paths in $_SESSION; discovery request
         * re-resolves them here).
         *
         * Fail closed: skip the add entirely so the swapped-in (victim) path
         * is never registered, never bound an owner from, and never flagged.
         * This prevents a compromised realpath() result from poisoning the V9
         * Layer 2 owner binding captured two lines below.
         *
         * A legitimate scan2 path is already canonical, so realpath() is
         * idempotent and the equality holds with no false positives.
         */
        $realPath = realpath($wpPath);

        if ( $realPath === false || $realPath !== $wpPath ) {
            $this->log(
                "Skipping add: install path $wpPath no longer canonicalises to "
                    . 'itself (possible cross-tenant TOCTOU — intermediate '
                    . 'component swapped between scan2 and addNewWPInstall).',
                Logger::L_INFO
            );

            return;
        }

        if ( !isset($this->wpInstalls[$wpPath]) ) {
            $this->wpInstalls[$wpPath] = new WPInstall($wpPath);
            $this->log("New installation found: $wpPath", Logger::L_INFO);

            if (
                    $this->custWpInstalls != null
                    &&
                    isset($this->custWpInstalls[$wpPath])
            ) {
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

        /**
         * V9.3 (CWE-59/CWE-367 cross-tenant TOCTOU) — bind the install
         * directory's owner observed in root context at this scan-time
         * snapshot.  scan2() already canonicalised $wpPath via realpath() and
         * validated containment under the panel-assigned docroot before
         * returning it.  Persisting the lstat() uid/gid here lets the later
         * root-context flag write (addUserFlagFile(false), in a separate
         * process / request) compare against a known-good snapshot rather
         * than a same-instant read of the very directory it is about to
         * modify — closing the race that Layers 1/3 leave open.
         *
         * Applied unconditionally to both newly-discovered and already-known
         * installs so that pre-V9.3 records hydrated with a null binding are
         * rebound on the next scan, mirroring §9 (V9.2)'s treatment in
         * scan().  lstat() is used (not stat()) so a symlinked leaf reports
         * the link's own inode owner rather than its target's, preventing
         * a same-instant final-component symlink swap from binding a
         * cross-tenant uid.  If lstat() fails (path vanished between scan2
         * and here), binding is skipped silently; the later flag write will
         * itself refuse on its own lstat($this->path) failure.
         */
        $installStat = @lstat($wpPath);

        if ( $installStat !== false ) {
            $this->wpInstalls[$wpPath]->setExpectedOwner(
                $installStat['uid'],
                $installStat['gid']
            );
        }

        $this->wpInstalls[$wpPath]->refreshStatus();
        $this->workingQueue[$wpPath] = $this->wpInstalls[$wpPath];
    }

    /**
     *
     * @param string[] $wpInstallsInfo
     *
     * @return void
     *
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->custWpInstalls[$wpPath]->refreshStatus() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     * @throws LSCMException  Thrown indirectly by $this->log() call.
     */
    protected function addCustomInstallations( array $wpInstallsInfo )
    {
        if ( $this->customDataFile == '' ) {
            $this->log(
                'No custom data file set, could not add custom Installation.',
                Logger::L_INFO
            );

            return;
        }

        if ( $this->custWpInstalls == null ) {
            $this->custWpInstalls = [];
        }

        for ( $i = 0; $i < count($wpInstallsInfo); $i++ ) {
            $info = preg_split('/\s+/', trim($wpInstallsInfo[$i]));

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

            if ( !Util::isSafeAbsPath($wpPath) ) {
                $this->log(
                    "Unsafe wpPath value on line $line. Skipping.",
                    Logger::L_INFO
                );

                continue;
            }

            if ( !file_exists("$wpPath/wp-admin") ) {
                $this->log(
                    "No 'wp-admin' directory found for $wpPath on line "
                        . "$line. Skipping.",
                    Logger::L_INFO
                );

                continue;
            }

            $docroot = $info[1];

            if ( !Util::isSafeAbsPath($docroot) ) {
                $this->log(
                    "Unsafe docroot value on line $line. Skipping.",
                    Logger::L_INFO
                );

                continue;
            }

            $realWpPath  = realpath($wpPath);
            $realDocroot = realpath($docroot);

            if ( $realWpPath === false || $realDocroot === false
                    || strpos($realWpPath . '/', $realDocroot . '/') !== 0 ) {
                $this->log(
                    "wpPath $wpPath not contained under docroot $docroot on "
                        . "line $line. Skipping.",
                    Logger::L_INFO
                );

                continue;
            }

            if ( !isset($this->wpInstalls[$wpPath]) ) {
                $this->custWpInstalls[$wpPath] = new WPInstall($wpPath);
                $this->custWpInstalls[$wpPath]->setDocRoot($docroot);
                $this->custWpInstalls[$wpPath]->setServerName($info[2]);
                $this->custWpInstalls[$wpPath]->setSiteUrlDirect($info[3]);
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
        $succ = $fail = $err = [];

        foreach ( $this->workingQueue as $wpInstall ) {

            if ( ($msg = $wpInstall->getCmdMsg()) ) {
                $cmdStatus = $wpInstall->getCmdStatus();

                switch( true ) {

                    case $cmdStatus & UserCommand::EXIT_SUCC:
                        $msgType = &$succ;
                        break;

                    case $cmdStatus & UserCommand::EXIT_FAIL:
                        $msgType = &$fail;
                        break;

                    case $cmdStatus & UserCommand::EXIT_ERROR:
                        $msgType = &$err;
                        break;

                    default:
                        continue 2;
                }

                $msgType[] = "{$wpInstall->getPath()} - $msg";
            }
        }

        return [ 'succ' => $succ, 'fail' => $fail, 'err' => $err ];
    }

    /**
     *
     * @param string $msg
     * @param int    $level
     *
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     * @throws LSCMException  Thrown indirectly by Logger::warn() call.
     * @throws LSCMException  Thrown indirectly by Logger::notice() call.
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     * @throws LSCMException  Thrown indirectly by Logger::verbose() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    protected function log( $msg, $level )
    {
        $msg = "WPInstallStorage - $msg";

        switch ( $level ) {

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
