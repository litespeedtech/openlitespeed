<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2023 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp;

use Lsc\Wp\Context\Context;
use Lsc\Wp\Panel\ControlPanel;
use Lsc\Wp\View\AjaxView;
use Lsc\Wp\View\Model\Ajax as AjaxViewModel;

class PanelController
{

    /**
     * @var int
     */
    const MGR_STEP_NONE = 0;

    /**
     * @var int
     */
    const MGR_STEP_SCAN = 1;

    /**
     * @var int
     */
    const MGR_STEP_DISCOVER_NEW = 2;

    /**
     * @var int
     */
    const MGR_STEP_REFRESH_STATUS = 3;

    /**
     * @var int
     */
    const MGR_STEP_MASS_UNFLAG = 4;

    /**
     * @var int
     */
    const DASH_STEP_NONE = 0;

    /**
     * @var int
     */
    const DASH_STEP_MASS_DASH_NOTIFY = 1;

    /**
     * @var int
     */
    const DASH_STEP_MASS_DASH_DISABLE = 2;

    /**
     * @var ControlPanel
     */
    protected $panelEnv;

    /**
     * @var WPInstallStorage
     */
    protected $wpInstallStorage;

    /**
     * @var int
     */
    protected $mgrStep;

    /**
     * @var int
     */
    protected $dashStep = self::DASH_STEP_NONE;

    /**
     *
     * @param ControlPanel     $panelEnv
     * @param WPInstallStorage $wpInstallStorage
     * @param int              $mgrStep
     */
    public function __construct(
        ControlPanel     $panelEnv,
        WPInstallStorage $wpInstallStorage,
                         $mgrStep = self::MGR_STEP_NONE )
    {
        $this->panelEnv         = $panelEnv;
        $this->wpInstallStorage = $wpInstallStorage;
        $this->mgrStep          = $mgrStep;
    }

    /**
     *
     * @param string $type
     *
     * @return void|string[]
     */
    protected function getCurrentAction( $type = 'cacheMgr' )
    {
        switch ( $type ) {

            case 'dashNotifier':
                $all_actions = array(
                    'notify_single'  => UserCommand::CMD_DASH_NOTIFY,
                    'disable_single' => UserCommand::CMD_DASH_DISABLE,
                    'get_msg'        => WPDashMsgs::ACTION_GET_MSG,
                    'add_msg'        => WPDashMsgs::ACTION_ADD_MSG,
                    'delete_msg'     => WPDashMsgs::ACTION_DELETE_MSG
                );

                break;

            case 'cacheMgr':
                $all_actions = array(
                    'enable_single'         => UserCommand::CMD_ENABLE,
                    'disable_single'        => UserCommand::CMD_DISABLE,
                    'flag_single'           => WPInstallStorage::CMD_FLAG,
                    'unflag_single'         => WPInstallStorage::CMD_UNFLAG,
                    'refresh_status_single' => UserCommand::CMD_STATUS,
                    'enable_sel'            => UserCommand::CMD_ENABLE,
                    'disable_sel'           => UserCommand::CMD_DISABLE,
                    'flag_sel'              => WPInstallStorage::CMD_FLAG,
                    'unflag_sel'            => WPInstallStorage::CMD_UNFLAG,
                );
                break;

            default:
                return;
        }

        foreach ( $all_actions as $act => $doAct ) {

            if ( Util::get_request_var($act) !== null ) {
                return array( 'act_key' => $act, 'action' => $doAct );
            }
        }
    }

    /**
     *
     * @deprecated
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by
     *     $this->manageCacheOperations() call.
     */
    public function manageOperationsSubController()
    {
        return $this->manageCacheOperations();
    }

    /**
     *
     * @deprecated 1.13.3  Use $this->manageCacheOperations2() instead.
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by $this->checkScanAction()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->checkRefreshAction()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->checkUnflagAction()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->doFormAction() call.
     * @throws LSCMException  Thrown indirectly by $this->doSingleAction() call.
     */
    public function manageCacheOperations()
    {
        if ( $this->checkScanAction()
                || $this->checkRefreshAction()
                || $this->checkUnflagAction()
                || ($actionInfo = $this->getCurrentAction()) == NULL ) {

            return $this->mgrStep;
        }

        $actKey = $actionInfo['act_key'];
        $action = $actionInfo['action'];

        if ( strcmp(substr($actKey, -3), 'sel') == 0 ) {
            $this->doFormAction($action);
        }
        else {
            $this->doSingleAction($action, Util::get_request_var($actKey));
        }

        return $this->mgrStep;
    }

    /**
     *
     * @since 1.13.3
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by $this->checkScanAction2()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->checkRefreshAction()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->checkUnflagAction()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->doFormAction() call.
     * @throws LSCMException  Thrown indirectly by $this->doSingleAction() call.
     */
    public function manageCacheOperations2()
    {
        if ( $this->checkScanAction2()
            || $this->checkRefreshAction()
            || $this->checkUnflagAction()
            || ($actionInfo = $this->getCurrentAction()) == NULL ) {

            return $this->mgrStep;
        }

        $actKey = $actionInfo['act_key'];
        $action = $actionInfo['action'];

        if ( strcmp(substr($actKey, -3), 'sel') == 0 ) {
            $this->doFormAction($action);
        }
        else {
            $path = Util::get_request_var($actKey);
            $this->doSingleAction($action, $path);
        }

        return $this->mgrStep;
    }

    /**
     *
     * @return string
     */
    protected function getDashMsgInfoJSON()
    {
        $msgInfo = array( 'msg' => '', 'plugin' => '', 'plugin_name' => '' );

        if ( ($msg = Util::get_request_var('notify_msg')) !== NULL ) {
            $msgInfo['msg'] = trim($msg);
        }

        if ( ($slug = Util::get_request_var('notify_slug')) !== NULL ) {
            $msgInfo['plugin'] = trim($slug);
        }

        /**
         * plugin_name is not passed at this time.
         */

        return json_encode($msgInfo);
    }

    /**
     *
     * @param int        $dashStep
     * @param WPDashMsgs $wpDashMsgs
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by
     *     $this->checkDashMassNotifyAction() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->checkDashMassDisableAction() call.
     * @throws LSCMException  Thrown indirectly by $this->doWpDashMsgOperation()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->doSingleDashAction()
     *     call.
     */
    public function manageDashOperations( $dashStep, WPDashMsgs $wpDashMsgs )
    {
        $this->dashStep = $dashStep;

        if ( $this->checkDashMassNotifyAction()
                || $this->checkDashMassDisableAction()
                || ($actionInfo = $this->getCurrentAction('dashNotifier')) == NULL ) {

            return $this->dashStep;
        }

        $actKey      = $actionInfo['act_key'];
        $action      = $actionInfo['action'];
        $msgInfoJSON = '';

        if ( substr($action, 0, 3) == 'msg' ) {
            $this->doWpDashMsgOperation($wpDashMsgs, $action);
        }

        if ( $actKey == UserCommand::CMD_DASH_NOTIFY ) {
            $msgInfoJSON = base64_encode($this->getDashMsgInfoJSON());
        }

        $this->doSingleDashAction(
            $action,
            Util::get_request_var($actKey),
            $msgInfoJSON
        );

        return $this->dashStep;
    }

    /**
     *
     * @param array $array
     *
     * @return int
     *
     * @throws LSCMException  Rethrown when Context::getOption() throws an
     *     LSCMException exception.
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     */
    protected function getBatchSize( array $array )
    {
        try {
            $batchSize = Context::getOption()->getBatchSize();
        }
        catch ( LSCMException $e ) {
            $msg = "{$e->getMessage()} Could not get batch size.";
            Logger::error($msg);

            throw new LSCMException($msg);
        }

        $arrSize = count($array);

        if ( $batchSize > $arrSize ) {
            return $arrSize;
        }

        return $batchSize;
    }

    /**
     *
     * @param object $viewModel
     * @param string $tplID
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     */
    protected function getAjaxViewContent( $viewModel, $tplID = '' )
    {
        $view = new AjaxView($viewModel);

        try {
            return $view->getAjaxContent($tplID);
        }
        catch ( LSCMException $e ) {
            Logger::error($e->getMessage());
        }

        return '';
    }

    /**
     *
     * @deprecated 1.9 Moved to using AjaxResponse object for this.
     *
     * @param array $ajaxInfo
     */
    protected function ajaxReturn( array $ajaxInfo )
    {
        ob_clean();
        echo json_encode($ajaxInfo);
        exit;
    }

    /**
     *
     * @deprecated 1.13.3  Use $this->checkScanAction2() instead.
     *
     * @return bool|void  Function outputs ajax and exits without returning a
     *     value when $init is evaluated to be false.
     *
     * @throws LSCMException  Thrown indirectly by
     *     $this->panelEnv->getDocroots() call.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstallStorage->doAction() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function checkScanAction()
    {
        $init   = false;
        $action = '';

        if ( $this->mgrStep == self::MGR_STEP_SCAN ) {
            $action = WPInstallStorage::CMD_SCAN;
        }
        elseif ( $this->mgrStep == self::MGR_STEP_DISCOVER_NEW ) {
            $action = WPInstallStorage::CMD_DISCOVER_NEW;
        }
        elseif ( Util::get_request_var('re-scan') ) {
            $this->mgrStep = self::MGR_STEP_SCAN;
            $init = true;
        }
        elseif ( Util::get_request_var('scan_more') ) {
            $this->mgrStep = self::MGR_STEP_DISCOVER_NEW;
            $init = true;
        }
        else {
            return false;
        }

        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $info = &$_SESSION['scanInfo'];

        if ( $init ) {
            $info = array( 'homeDirs' => $this->panelEnv->getDocroots() );
            return true;
        }

        $completedCount = -1;

        if ( !isset($info['homeDirs']) ) {
            Logger::uiError(
                'Expected session data missing! Stopping mass operation.'
            );
        }
        else {
            $batchSize = $this->getBatchSize($info['homeDirs']);
            $batch     = array_splice($info['homeDirs'], 0, $batchSize);

            $completedCount = count(
                $this->wpInstallStorage->doAction(
                    $action,
                    $batch
                )
            );

            if ( $completedCount != $batchSize ) {
                $info['homeDirs'] = array_merge(
                    array_slice($batch, $completedCount),
                    $info['homeDirs']
                );
            }
        }

        if ( empty($info['homeDirs']) ) {
            unset($_SESSION['scanInfo']);
        }

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        AjaxResponse::setAjaxContent(
            json_encode(
                array(
                    'completed' => $completedCount,
                    'errMsgs'   => array_merge(
                        $msgs['fail'],
                        $msgs['err'],
                        Logger::getUiMsgs(Logger::UI_ERR)
                    )
                )
            )
        );
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @since 1.15
     *
     * @param string[] $docroots
     * @param string[] $wpPaths  Array of discovered WordPress installations
     *     (passed by address).
     *
     * @return int  Completed count.
     *
     * @throws LSCMException  Thrown indirectly by WPInstallStorage->scan2()
     *     call.
     */
    protected function runScan2Logic( array $docroots, array &$wpPaths )
    {
        foreach ( $docroots as $docroot ) {
            $wpPaths = array_merge(
                $wpPaths,
                WPInstallStorage::scan2($docroot)
            );
        }

        return count($docroots);
    }

    /**
     *
     * @since 1.13.3
     *
     * @return bool|void  Function outputs ajax and exits without returning a
     *     value when $init is evaluated to be false.
     *
     * @throws LSCMException  Thrown indirectly by
     *     $this->panelEnv->getDocroots() call.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by $this->runScan2Logic() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstallStorage->doAction() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function checkScanAction2()
    {
        $init   = false;
        $action = '';

        if ( $this->mgrStep == self::MGR_STEP_SCAN ) {
            $action = WPInstallStorage::CMD_SCAN2;
        }
        elseif ( $this->mgrStep == self::MGR_STEP_DISCOVER_NEW ) {
            $action = WPInstallStorage::CMD_DISCOVER_NEW2;
        }
        elseif ( Util::get_request_var('re-scan') ) {
            $this->mgrStep = self::MGR_STEP_SCAN;
            $init = true;
        }
        elseif ( Util::get_request_var('scan_more') ) {
            $this->mgrStep = self::MGR_STEP_DISCOVER_NEW;
            $init = true;
        }
        else {
            return false;
        }

        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        if ( Util::get_request_var('go_to_next') == 1 ){
            return true;
        }

        $info = &$_SESSION['scanInfo'];

        if ( $init ) {
            $info = array(
                'homeDirs' => $this->panelEnv->getDocroots(),
                'installs' => array()
            );
            return true;
        }

        $scanStep       = self::MGR_STEP_NONE;
        $errMsgs        = array();
        $completedCount = -1;

        if ( !is_array($info)
                || !array_key_exists('homeDirs', $info)
                || !array_key_exists('installs', $info) ) {

            Logger::uiError(
                'Expected session data missing! Stopping mass operation.'
            );
        }
        else {

            if ( $info['homeDirs'] !== null ) {
                $scanStep  = self::MGR_STEP_SCAN;
                $batchSize = $this->getBatchSize($info['homeDirs']);
                $batch     = array_splice($info['homeDirs'], 0, $batchSize);

                $wpPaths = array();
                $completedCount = $this->runScan2Logic($batch, $wpPaths);

                if ( $completedCount != $batchSize ) {
                    $info['homeDirs'] = array_merge(
                        array_slice($batch, $completedCount),
                        $info['homeDirs']
                    );
                }

                if ( $action == WPInstallStorage::CMD_DISCOVER_NEW2 ) {

                    foreach( $wpPaths as $wpPath ) {

                        if ( $this->wpInstallStorage->getWPInstall($wpPath) == null) {
                            $info['installs'][] = $wpPath;
                        }
                    }
                }
                else {
                    $info['installs'] =
                        array_merge($info['installs'], $wpPaths);
                }

                if ( empty($info['homeDirs']) ) {
                    $info['homeDirs'] = null;

                    if ( $action == WPInstallStorage::CMD_SCAN2 ) {
                        $flippedInstalls = array_flip($info['installs']);

                        $wpInstalls = $this->wpInstallStorage->getWPInstalls();

                        if ( $wpInstalls === null ) {
                            $wpInstalls = array();
                        }

                        foreach ( $wpInstalls as $wpInstall ) {
                            $wpInstallPath = $wpInstall->getPath();

                            if ( !isset($flippedInstalls[$wpInstallPath]) ) {
                                $info['installs'][] = $wpInstallPath;
                            }
                        }
                    }
                }
            }
            elseif ( $info['installs'] !== null ) {
                $scanStep  = self::MGR_STEP_DISCOVER_NEW;
                $batchSize = $this->getBatchSize($info['installs']);
                $batch     = array_splice($info['installs'], 0, $batchSize);

                $completedCount = count(
                    $this->wpInstallStorage->doAction(
                        WPInstallStorage::CMD_ADD_NEW_WPINSTALL,
                        $batch
                    )
                );

                if ( $completedCount != $batchSize ) {
                    $info['installs'] = array_merge(
                        array_slice($batch, $completedCount),
                        $info['installs']
                    );
                }

                $msgs    = $this->wpInstallStorage->getAllCmdMsgs();
                $errMsgs = array_merge($msgs['fail'], $msgs['err']);

                if ( empty($info['installs']) ) {
                    $info['installs'] = null;
                }
            }
        }

        if ( $info['homeDirs'] === null && $info['installs'] === null ) {
            unset($_SESSION['scanInfo']);
        }

        AjaxResponse::setAjaxContent(
            json_encode(
                array(
                    'scanStep'  => $scanStep,
                    'completed' => $completedCount,
                    'errMsgs'   => array_merge(
                        $errMsgs,
                        Logger::getUiMsgs(Logger::UI_ERR)
                    )
                )
            )
        );
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @return bool|void  Function outputs ajax and exits without returning a
     *     value when $init is evaluated to be false.
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstallStorage->doAction() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function checkRefreshAction()
    {
        if ( $this->mgrStep == self::MGR_STEP_REFRESH_STATUS ) {
            $init = false;
        }
        elseif ( Util::get_request_var('refresh_status') ) {
            $init = true;
            $this->mgrStep = self::MGR_STEP_REFRESH_STATUS;
        }
        else {
            return false;
        }

        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $info = &$_SESSION['refreshInfo'];

        if ( $init ) {
            $info = array( 'installs' => $this->wpInstallStorage->getPaths() );
            return true;
        }

        $completedCount = -1;

        if ( !isset($info['installs']) ) {
            Logger::uiError(
                'Expected session data missing! Stopping mass operation.'
            );
        }
        else {
            $batchSize = $this->getBatchSize($info['installs']);
            $batch     = array_splice($info['installs'], 0, $batchSize);

            $completedCount = count(
                $this->wpInstallStorage->doAction(
                    UserCommand::CMD_STATUS,
                    $batch
                )
            );

            if ( $completedCount != $batchSize ) {
                $info['installs'] = array_merge(
                    array_slice($batch, $completedCount),
                    $info['installs']
                );
            }
        }

        if ( empty($info['installs']) ) {
            unset($_SESSION['refreshInfo']);
        }

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        AjaxResponse::setAjaxContent(
            json_encode(
                array(
                    'completed' => $completedCount,
                    'errMsgs'   => array_merge(
                        $msgs['fail'],
                        $msgs['err'],
                        Logger::getUiMsgs(Logger::UI_ERR)
                    )
                )
            )
        );
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @return bool|void  Function outputs ajax and exits without returning a
     *     value when $init is evaluated to be false.
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstallStorage->doAction() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function checkUnflagAction()
    {
        if ( $this->mgrStep == self::MGR_STEP_MASS_UNFLAG ) {
            $init = false;
        }
        elseif ( Util::get_request_var('mass_unflag') ) {
            $init = true;
            $this->mgrStep = self::MGR_STEP_MASS_UNFLAG;
        }
        else {
            return false;
        }

        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $info = &$_SESSION['unflagInfo'];

        if ( $init ) {
            $info = array( 'installs' => $this->wpInstallStorage->getPaths() );
            return true;
        }

        $completedCount = -1;

        if ( !isset($info['installs']) ) {
            Logger::uiError(
                'Expected session data missing! Stopping mass operation.'
            );
        }
        else {
            $batchSize = $this->getBatchSize($info['installs']);
            $batch     = array_splice($info['installs'], 0, $batchSize);

            $completedCount = count(
                $this->wpInstallStorage->doAction(
                    WPInstallStorage::CMD_MASS_UNFLAG,
                    $batch
                )
            );

            if ( $completedCount != $batchSize ) {
                $info['installs'] = array_merge(
                    array_slice($batch, $completedCount),
                    $info['installs']
                );
            }
        }

        if ( empty($info['installs']) ) {
            unset($_SESSION['unflagInfo']);
        }

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        AjaxResponse::setAjaxContent(
            json_encode(
                array(
                    'completed' => $completedCount,
                    'errMsgs'   => array_merge(
                        $msgs['fail'],
                        $msgs['err'],
                        Logger::getUiMsgs(Logger::UI_ERR)
                    ),
                    'succMsgs'  => array_merge(
                        $msgs['succ'],
                        Logger::getUiMsgs(Logger::UI_SUCC)
                    )
                )
            )
        );
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @param string $action
     *
     * @return void
     *
     * @throws LSCMException  Rethrown when Context::getOption() throws an
     *     LSCMException exception.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstallStorage->doAction() call.
     */
    protected function doFormAction( $action )
    {
        $list = Util::get_request_list('installations');

        /**
         * Empty list also checked earlier using JS.
         */
        if ( $list == NULL ) {
            Logger::uiError('Please select at least one checkbox.');
            return;
        }

        foreach ( $list as $wpPath ) {

            if ( $this->wpInstallStorage->getWPInstall($wpPath) === null ) {
                Logger::uiError(
                    'Invalid input value detected -  No Action Taken'
                );
                return;
            }
        }

        try {
            Context::getOption()->setBatchTimeout(0);
        }
        catch ( LSCMException $e ) {
            $msg = "{$e->getMessage()} Could not set batch timeout.";
            Logger::error($msg);

            throw new LSCMException($msg);
        }

        $this->wpInstallStorage->doAction($action, $list);
    }

    /**
     * Get simple string representations of current installation status.
     *
     * @param WPInstall $wpInstall
     *
     * @return string[]
     */
    protected function getStatusStrings( WPInstall $wpInstall )
    {
        $status = $wpInstall->getStatus();

        if ( $status & WPInstall::ST_ERR_REMOVE ) {
            $statusString = 'removed';
        }
        elseif ( $wpInstall->hasFatalError() ) {
            $statusString = 'error';
        }
        elseif ( ($status & WPInstall::ST_PLUGIN_INACTIVE ) ) {
            $statusString = 'disabled';
        }
        elseif ( !($status & WPInstall::ST_LSC_ADVCACHE_DEFINED) ) {
            $statusString = 'warning';
        }
        else {
            $statusString = 'enabled';
        }

        if ( $wpInstall->isFlagBitSet() ) {
            $flagString = 'flagged';
        }
        else {
            $flagString = 'unflagged';
        }

        return array(
            'status'      => $statusString,
            'flag_status' => $flagString
        );
    }

    /**
     *
     * @param string $action
     * @param string $path
     *
     * @return void
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstallStorage->doAction() call.
     * @throws LSCMException  Thrown indirectly by $this->getAjaxViewContent()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->getAjaxViewContent()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->getAjaxViewContent()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function doSingleAction( $action, $path )
    {
        $wpInstall = $this->wpInstallStorage->getWPInstall($path);

        if ( $wpInstall === null ) {
            Logger::uiError('Invalid input value detected - No Action Taken');
            return;
        }

        $preStatusInfo = $this->getStatusStrings($wpInstall);

        $this->wpInstallStorage->doAction($action, array( $path ));

        $postStatusInfo = $this->getStatusStrings($wpInstall);

        $badgesDown = $badgesUp = array();
        $removed = false;

        if ( $postStatusInfo['status'] == 'removed' ) {
            $removed = true;

            $badgesDown[] = $preStatusInfo['status'];
            $badgesDown[] = 'flag';
        }
        else {

            if ( $preStatusInfo['status'] != $postStatusInfo['status'] ) {
                $badgesDown[] = $preStatusInfo['status'];
                $badgesUp[]   = $postStatusInfo['status'];
            }

            if ( $preStatusInfo['flag_status'] != $postStatusInfo['flag_status'] ) {

                if ( $postStatusInfo['flag_status'] == 'flagged' ) {
                    $badgesUp[] = 'flag';
                }
                else {
                    $badgesDown[] = 'flag';
                }
            }
        }

        $viewModel = new AjaxViewModel\CacheMgrRowViewModel($wpInstall);

        $statusSort = $viewModel->getsortVal('status');
        $flagSort   = $viewModel->getsortVal('flag');

        switch ( $flagSort ) {

            case 'removed':
                $flagSearch = $flagSort;
                break;

            case 'flagged':
                $flagSearch = 'f';
                break;

            default:
                $flagSearch = 'u';
        }

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        AjaxResponse::setAjaxContent(
            json_encode(
                array(
                    'displayData' => array(
                        'actions_td'    =>
                            $this->getAjaxViewContent($viewModel, 'actions_td'),
                        'status_search' => $statusSort,
                        'status_sort'   => $statusSort,
                        'status_td'     =>
                            $this->getAjaxViewContent($viewModel, 'status_td'),
                        'flag_search'   => $flagSearch,
                        'flag_sort'     => $flagSort,
                        'flag_td'       =>
                            $this->getAjaxViewContent($viewModel, 'flag_td')
                    ),
                    'badgesDown'  => $badgesDown,
                    'badgesUp'    => $badgesUp,
                    'removed'     => $removed,
                    'errMsgs'     => array_merge(
                        $msgs['fail'],
                        $msgs['err'],
                        Logger::getUiMsgs(Logger::UI_ERR)
                    ),
                    'succMsgs'    => array_merge(
                        $msgs['succ'],
                        Logger::getUiMsgs(Logger::UI_SUCC)
                    )
                )
            )
        );
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @param WPDashMsgs $wpDashMsgs
     * @param string     $action
     *
     * @throws LSCMException  Thrown when parameter $action value is
     *     unrecognized.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function doWpDashMsgOperation(
        WPDashMsgs $wpDashMsgs,
                   $action )
    {
        if ( ($type = Util::get_request_var('msg_type')) !== NULL ) {
            $type = trim($type);
        }

        if ( ($msgId = Util::get_request_var('msg_id')) !== NULL ) {
            $msgId = trim($msgId);
        }

        switch ( $action ) {

            case WPDashMsgs::ACTION_GET_MSG:
                $msgs = $wpDashMsgs->getMsgData($type);

                $msgInfo = array();

                if ( $msgId !== '' && $msgId !== NULL && isset($msgs[$msgId]) ) {
                    $msgInfo = $msgs[$msgId];
                }

                $ajaxReturn = array( 'msgInfo' => $msgInfo );
                break;

            case WPDashMsgs::ACTION_ADD_MSG:

                if ( ($msg = Util::get_request_var('notify_msg')) !== NULL ) {
                    $msg = trim($msg);
                }

                if ( ($slug = Util::get_request_var('notify_slug')) !== NULL ) {
                    $slug = trim($slug);
                }

                $ret = 0;

                if ( $wpDashMsgs->addMsg($type, $msgId, $msg, $slug) ) {
                    $ret = 1;
                }

                $ajaxReturn = array( 'ret' => $ret );

                break;

            case WPDashMsgs::ACTION_DELETE_MSG:
                $ajaxReturn = array(
                    'ret' => ($wpDashMsgs->deleteMsg($type, $msgId)) ? 1 : 0
                );

                break;

            default:
                throw new LSCMException(
                    'PanelController->doWpDashMsgOperation(): Unrecognized '
                        . '$action value.'
                );
        }

        AjaxResponse::setAjaxContent(json_encode($ajaxReturn));
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @param string $action
     * @param string $path
     * @param string $msgInfoJSON
     *
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->wpInstallStorage->doAction() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function doSingleDashAction( $action, $path, $msgInfoJSON = '' )
    {
        if ( $this->wpInstallStorage->getWPInstall($path) === null ) {
            AjaxResponse::setAjaxContent(
                json_encode(
                    array(
                        'errMsgs'  => array(
                            'Invalid input value detected - Please use the '
                                . 'exact path displayed in the '
                                . '"Manage Cache Installations" list when '
                                . 'testing.'
                        ),
                        'succMsgs' => array()
                    )
                )
            );
            AjaxResponse::outputAndExit();
        }

        $this->wpInstallStorage->doAction(
            $action,
            array( $path ),
            array( $msgInfoJSON )
        );

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        AjaxResponse::setAjaxContent(
            json_encode(
                array(
                    'errMsgs'  => array_merge(
                        $msgs['fail'],
                        $msgs['err'],
                        Logger::getUiMsgs(Logger::UI_ERR)
                    ),
                    'succMsgs' => array_merge(
                        $msgs['succ'],
                        Logger::getUiMsgs(Logger::UI_SUCC)
                    )
                )
            )
        );
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @param string $action
     * @param int    $step
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    public function massEnableDisable( $action, $step )
    {
        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $actionUpper = ucfirst($action);

        $info = &$_SESSION["mass{$actionUpper}Info"];

        if ( $step == 1 ) {
            $info = array('installs' => $this->wpInstallStorage->getPaths() );
        }
        elseif ( $step == 2 ) {
            $completedCount = -1;
            $succCount = $failCount = $bypassedCount = 0;

            if ( !isset($info['installs']) ) {
                Logger::uiError(
                    'Expected session data missing! Stopping mass operation.'
                );
            }
            else {
                $batchSize = $this->getBatchSize($info['installs']);
                $batch     = array_splice($info['installs'], 0, $batchSize);

                try
                {
                    $finishedList = $this->wpInstallStorage->doAction(
                        "mass_$action",
                        $batch
                    );

                    $completedCount = count($finishedList);

                    if ( $completedCount != $batchSize ) {
                        $info['installs'] = array_merge(
                            array_slice($batch, $completedCount),
                            $info['installs']
                        );
                    }
                }
                catch ( LSCMException $e )
                {
                    Logger::uiError(
                        "{$e->getMessage()} Stopping mass operation."
                    );
                    $finishedList     = array();
                    $info['installs'] = array();
                }

                $workingQueue = $this->wpInstallStorage->getWorkingQueue();

                foreach ( $finishedList as $path ) {
                    $wpInstall = $this->wpInstallStorage->getWPInstall($path);

                    if ( isset($workingQueue[$path]) ) {
                        $cmdStatus = $workingQueue[$path]->getCmdStatus();
                    }
                    else {
                        $cmdStatus = $wpInstall->getCmdStatus();
                    }

                    if ( $cmdStatus & UserCommand::EXIT_INCR_SUCC ) {
                        $succCount++;
                    }
                    elseif ( $cmdStatus & UserCommand::EXIT_INCR_FAIL ) {
                        $failCount++;
                    }
                    elseif ( $cmdStatus & UserCommand::EXIT_INCR_BYPASS
                            || $wpInstall->isFlagBitSet() ) {

                        $bypassedCount++;
                    }
                }
            }

            if ( empty($info['installs']) ) {
                unset($_SESSION["mass{$actionUpper}Info"]);
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();

            AjaxResponse::setAjaxContent(
                json_encode(
                    array(
                        'completed'     => $completedCount,
                        'bypassedCount' => $bypassedCount,
                        'failCount'     => $failCount,
                        'succCount'     => $succCount,
                        'errMsgs'       => array_merge(
                            $msgs['fail'],
                            $msgs['err'],
                            Logger::getUiMsgs(Logger::UI_ERR)
                        ),
                        'succMsgs'      => array_merge(
                            $msgs['succ'],
                            Logger::getUiMsgs(Logger::UI_SUCC)
                        )
                    )
                )
            );
            AjaxResponse::outputAndExit();
        }
    }

    /**
     * Force updates LiteSpeed Cache Plugins whose versions
     * match those in $verList and refreshes status as well. Flagged
     * installations are bypassed.
     *
     * @param int $step  Value of 2 will prep session info, 3 will begin
     *     changing installed LSCWP versions in batches.
     *
     * @return void
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    public function prepVersionChange( $step )
    {
        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $info = &$_SESSION['verInfo'];

        if ( $step == 2 ) {
            /**
             * init upgrade
             */

            if ( empty(($verList = Util::get_request_list('selectedVers'))) ) {
                /**
                 * Version list should not be empty
                 */
                return;
            }

            $info = array(
                'installs' => $this->wpInstallStorage->getPaths(),
                'verNum'   => Util::get_request_var('version_num'),
                'verList'  => $verList,
            );
        }
        elseif ( $step == 3 ) {
            $completedCount = -1;

            if ( !isset($info['installs']) ) {
                Logger::uiError(
                    'Expected session data missing! Stopping mass operation.'
                );
            }
            else {
                $batchSize = $this->getBatchSize($info['installs']);
                $batch     = array_splice($info['installs'], 0, $batchSize);

                try {
                    $completedCount = count(
                        $this->wpInstallStorage->doAction(
                            UserCommand::CMD_MASS_UPGRADE,
                            $batch,
                            /**
                             * 1 => "from versions" (comma seperated),
                             * 2 => "to version"
                             */
                            array(
                                implode(',', $info['verList']),
                                $info['verNum']
                            )
                        )
                    );

                    if ( $completedCount != $batchSize ) {
                        $info['installs'] = array_merge(
                            array_slice($batch, $completedCount),
                            $info['installs']
                        );
                    }
                }
                catch ( LSCMException $e ) {
                    Logger::uiError(
                        "{$e->getMessage()} Stopping mass operation."
                    );

                    $info['installs'] = array();
                }
            }

            if ( empty($info['installs']) || $completedCount == -1 ) {
                unset($_SESSION['verInfo']);
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();

            AjaxResponse::setAjaxContent(
                json_encode(
                    array(
                        'completed' => $completedCount,
                        'errMsgs'   => array_merge(
                            $msgs['fail'],
                            $msgs['err'],
                            Logger::getUiMsgs(Logger::UI_ERR)
                        )
                    )
                )
            );
            AjaxResponse::outputAndExit();
        }
    }

    /**
     *
     * @return bool|void  Function outputs ajax and exits without returning a
     *     value when $init is evaluated to be false.
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function checkDashMassNotifyAction()
    {
        if ( $this->dashStep == self::DASH_STEP_MASS_DASH_NOTIFY ) {
            $init = false;
        }
        elseif ( Util::get_request_var('mass-notify') ) {
            $init = true;
            $this->dashStep = self::DASH_STEP_MASS_DASH_NOTIFY;
        }
        else {
            return false;
        }

        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $info = &$_SESSION['massDashNotifyInfo'];

        if ( $init ) {
            $info = array(
                'installs' => $this->wpInstallStorage->getPaths(),
                'msgInfo'  => base64_encode($this->getDashMsgInfoJSON())
            );

            return true;
        }

        $completedCount = -1;
        $succCount = $failCount = $bypassedCount = 0;

        if ( !isset($info['installs']) ) {
            Logger::uiError(
                'Expected session data missing! Stopping mass operation.'
            );
        }
        else {
            $batchSize = $this->getBatchSize($info['installs']);
            $batch     = array_splice($info['installs'], 0, $batchSize);

            try {
                $finishedList = $this->wpInstallStorage->doAction(
                    UserCommand::CMD_MASS_DASH_NOTIFY,
                    $batch,
                    array($info['msgInfo'])
                );

                $completedCount = count($finishedList);

                if ( $completedCount != $batchSize ) {
                    $info['installs'] = array_merge(
                        array_slice($batch, $completedCount),
                        $info['installs']
                    );
                }
            }
            catch ( LSCMException $e ) {
                Logger::uiError("{$e->getMessage()} Stopping mass operation.");
                $finishedList     = array();
                $info['installs'] = array();
            }

            $workingQueue = $this->wpInstallStorage->getWorkingQueue();

            foreach ( $finishedList as $path ) {
                $wpInstall = $this->wpInstallStorage->getWPInstall($path);

                if ( isset($workingQueue[$path]) ) {
                    $cmdStatus = $workingQueue[$path]->getCmdStatus();
                }
                else {
                    $cmdStatus = $wpInstall->getCmdStatus();
                }

                if ( $cmdStatus & UserCommand::EXIT_INCR_SUCC ) {
                    $succCount++;
                }
                elseif ( $cmdStatus & UserCommand::EXIT_INCR_FAIL ) {
                    $failCount++;
                }
                elseif ( $cmdStatus & UserCommand::EXIT_INCR_BYPASS
                        || $wpInstall->isFlagBitSet() ) {

                    $bypassedCount++;
                }
            }
        }

        if ( empty($info['installs']) ) {
            unset($_SESSION['massDashNotifyInfo']);
        }

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        AjaxResponse::setAjaxContent(
            json_encode(
                array(
                    'completed'     => $completedCount,
                    'bypassedCount' => $bypassedCount,
                    'failCount'     => $failCount,
                    'succCount'     => $succCount,
                    'errMsgs'       => array_merge(
                        $msgs['fail'],
                        $msgs['err'],
                        Logger::getUiMsgs(Logger::UI_ERR)
                    ),
                    'succMsgs'      => array_merge(
                        $msgs['succ'],
                        Logger::getUiMsgs(Logger::UI_SUCC)
                    )
                )
            )
        );
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @return bool|void  Function outputs ajax and exits without returning a
     *     value when $init is evaluated to be false.
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by $this->getBatchSize() call.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by AjaxResponse::outputAndExit()
     *     call.
     */
    protected function checkDashMassDisableAction()
    {
        if ( $this->dashStep == self::DASH_STEP_MASS_DASH_DISABLE ) {
            $init = false;
        }
        elseif ( Util::get_request_var('mass-disable-dash-notifier') ) {
            $init = true;
            $this->dashStep = self::DASH_STEP_MASS_DASH_DISABLE;
        }
        else {
            return false;
        }

        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $info = &$_SESSION['massDashDisableInfo'];

        if ( $init ) {
            $info = array( 'installs' => $this->wpInstallStorage->getPaths() );
            return true;
        }

        $completedCount = -1;
        $succCount = $failCount = $bypassedCount = 0;

        if ( !isset($info['installs']) ) {
            Logger::uiError(
                'Expected session data missing! Stopping mass operation.'
            );
        }
        else {
            $batchSize = $this->getBatchSize($info['installs']);
            $batch     = array_splice($info['installs'], 0, $batchSize);

            try {
                $finishedList = $this->wpInstallStorage->doAction(
                    UserCommand::CMD_MASS_DASH_DISABLE,
                    $batch
                );

                $completedCount = count($finishedList);

                if ( $completedCount != $batchSize ) {
                    $info['installs'] = array_merge(
                        array_slice($batch, $completedCount),
                        $info['installs']
                    );
                }
            }
            catch ( LSCMException $e ) {
                Logger::uiError("{$e->getMessage()} Stopping mass operation.");
                $finishedList     = array();
                $info['installs'] = array();
            }

            $workingQueue = $this->wpInstallStorage->getWorkingQueue();

            foreach ( $finishedList as $path ) {
                $wpInstall = $this->wpInstallStorage->getWPInstall($path);

                if ( isset($workingQueue[$path]) ) {
                    $cmdStatus = $workingQueue[$path]->getCmdStatus();
                }
                else {
                    $cmdStatus = $wpInstall->getCmdStatus();
                }

                if ( $cmdStatus & UserCommand::EXIT_INCR_SUCC ) {
                    $succCount++;
                }
                elseif ( $cmdStatus & UserCommand::EXIT_INCR_FAIL ) {
                    $failCount++;
                }
                elseif ( $cmdStatus & UserCommand::EXIT_INCR_BYPASS
                        || $wpInstall->isFlagBitSet() ) {

                    $bypassedCount++;
                }
            }
        }

        if ( empty($info['installs']) ) {
            unset($_SESSION['massDashDisableInfo']);
        }

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        AjaxResponse::setAjaxContent(
            json_encode(
                array(
                    'completed'     => $completedCount,
                    'bypassedCount' => $bypassedCount,
                    'failCount'     => $failCount,
                    'succCount'     => $succCount,
                    'errMsgs'       => array_merge(
                        $msgs['fail'],
                        $msgs['err'],
                        Logger::getUiMsgs(Logger::UI_ERR)
                    ),
                    'succMsgs'      => array_merge(
                        $msgs['succ'],
                        Logger::getUiMsgs(Logger::UI_SUCC)
                    )
                )
            )
        );
        AjaxResponse::outputAndExit();
    }

}
