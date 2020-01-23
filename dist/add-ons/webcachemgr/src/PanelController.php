<?php

/* * *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2018-2020
 * *******************************************
 */

namespace Lsc\Wp;

use \Lsc\Wp\AjaxResponse;
use \Lsc\Wp\Context\Context;
use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Panel\ControlPanel;
use \Lsc\Wp\View\AjaxView;
use \Lsc\Wp\View\Model\Ajax as AjaxViewModel;
use \Lsc\Wp\WPDashMsgs;
use \Lsc\Wp\WPInstallStorage;
use \Lsc\Wp\WPInstall;
use \Lsc\Wp\UserCommand;
use \Lsc\Wp\Util;

class PanelController
{

    const MGR_STEP_NONE = 0;
    const MGR_STEP_SCAN = 1;
    const MGR_STEP_DISCOVER_NEW = 2;
    const MGR_STEP_REFRESH_STATUS = 3;
    const MGR_STEP_MASS_UNFLAG = 4;
    const DASH_STEP_NONE = 0;
    const DASH_STEP_MASS_DASH_NOTIFY = 1;
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
     * @param ControlPanel      $panelEnv
     * @param WPInstallStorage  $wpInstallStorage
     * @param int               $mgrStep
     */
    public function __construct( ControlPanel $panelEnv,
            WPInstallStorage $wpInstallStorage, $mgrStep = self::MGR_STEP_NONE )
    {
        $this->panelEnv = $panelEnv;
        $this->wpInstallStorage = $wpInstallStorage;
        $this->mgrStep = $mgrStep;
    }

    /**
     *
     * @param string  $type
     * @return null|string[]
     */
    protected function getCurrentAction( $type = 'cacheMgr' )
    {
        switch ( $type ) {
            case 'dashNotifier':
                $all_actions = array(
                    'notify_single' => UserCommand::CMD_DASH_NOTIFY,
                    'disable_single' => UserCommand::CMD_DASH_DISABLE,
                    'get_msg' => WPDashMsgs::ACTION_GET_MSG,
                    'add_msg' => WPDashMsgs::ACTION_ADD_MSG,
                    'delete_msg' => WPDashMsgs::ACTION_DELETE_MSG
                );

                break;
            case 'cacheMgr':
                $all_actions = array(
                    'enable_single' => UserCommand::CMD_ENABLE,
                    'disable_single' => UserCommand::CMD_DISABLE,
                    'flag_single' => WPInstallStorage::CMD_FLAG,
                    'unflag_single' => WPInstallStorage::CMD_UNFLAG,
                    'refresh_status_single' => UserCommand::CMD_STATUS,
                    'enable_sel' => UserCommand::CMD_ENABLE,
                    'disable_sel' => UserCommand::CMD_DISABLE,
                    'flag_sel' => WPInstallStorage::CMD_FLAG,
                    'unflag_sel' => WPInstallStorage::CMD_UNFLAG,
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
     * @deprecated
     * @return int
     */
    public function manageOperationsSubController()
    {
        return $this->manageCacheOperations();
    }

    /**
     *
     * @return int
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
        $msgInfo = array(
            'msg' => '',
            'plugin' => '',
            'plugin_name' => ''
        );

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
     * @return int
     */
    public function manageDashOperations( $dashStep, WPDashMsgs $wpDashMsgs )
    {
        $this->dashStep = $dashStep;

        if ( $this->checkDashMassNotifyAction()
                || $this->checkDashMassDisableAction()
                || ($actionInfo = $this->getCurrentAction('dashNotifier')) == NULL ) {

            return $this->dashStep;
        }

        $actKey = $actionInfo['act_key'];
        $action = $actionInfo['action'];
        $msgInfoJSON = '';

        if ( substr($action, 0, 3) == 'msg' ) {
            $this->doWpDashMsgOperation($wpDashMsgs, $action);
        }

        $path = Util::get_request_var($actKey);

        if ( $actKey = UserCommand::CMD_DASH_NOTIFY ) {
            $msgInfoJSON = base64_encode($this->getDashMsgInfoJSON());
        }

        $this->doSingleDashAction($action, $path, $msgInfoJSON);

        return $this->dashStep;
    }

    /**
     *
     * @param array  $array
     * @return int
     * @throws LSCMException
     */
    protected function getBatchSize( &$array )
    {
        try
        {
            $batchSize = Context::getOption()->getBatchSize();
        }
        catch ( LSCMException $e )
        {
            $msg = $e->getMessage() . ' Could not get batch size.';
            Logger::error($msg);

            throw new LSCMException($msg);
        }

        $arrSize = count($array);

        if ( $batchSize > $arrSize ) {
            $batchSize = $arrSize;
        }

        return $batchSize;
    }

    /**
     *
     * @param object  $viewModel
     * @param string  $tplID
     * @return type
     */
    protected function getAjaxViewContent( $viewModel, $tplID = '' )
    {
        $view = new AjaxView($viewModel);

        $content = '';

        try
        {
            $content = $view->getAjaxContent($tplID);
        }
        catch ( LSCMException $e )
        {
            Logger::error($e->getMessage());
        }

        return $content;
    }

    /**
     *
     * @deprecated 1.9 Moved to using AjaxResponse object for this.
     *
     * @param mixed[]  $ajaxInfo
     */
    protected function ajaxReturn( $ajaxInfo )
    {
        ob_clean();
        echo json_encode($ajaxInfo);
        exit;
    }

    /**
     *
     * @return boolean
     */
    protected function checkScanAction()
    {
        $init = false;

        if ( $this->mgrStep == self::MGR_STEP_SCAN ) {
            $action = WPInstallStorage::CMD_SCAN;
        }
        elseif ( $this->mgrStep == self::MGR_STEP_DISCOVER_NEW ) {
            $action = WPInstallStorage::CMD_DISCOVER_NEW;
        }
        else {

            if ( Util::get_request_var('re-scan') ) {
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
        }

        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $info = &$_SESSION['scanInfo'];

        if ( $init ) {
            $docrootInfo = $this->panelEnv->getDocroots();

            $info = array(
                'homeDirs' => $docrootInfo,
            );
        }
        else {
            $completedCount = -1;

            if ( !isset($info['homeDirs']) ) {
                Logger::uiError('Expected session data missing! Stopping mass operation.');
            }
            else {
                $batchSize = $this->getBatchSize($info['homeDirs']);

                $batch = array_splice($info['homeDirs'], 0, $batchSize);

                $finishedList = $this->wpInstallStorage->doAction($action,
                        $batch);
                $completedCount = count($finishedList);

                if ( $completedCount != $batchSize ) {
                    $info['homeDirs'] = array_merge(
                            array_slice($batch, $completedCount), $info['homeDirs']);
                }
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();
            $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                    Logger::getUiMsgs(Logger::UI_ERR));

            $ajaxInfo = array(
                'completed' => $completedCount,
                'errMsgs' => $errMsgs
            );

            if ( empty($info['homeDirs']) ) {
                unset($_SESSION['scanInfo']);
            }

            AjaxResponse::setAjaxContent(json_encode($ajaxInfo));
            AjaxResponse::outputAndExit();
        }

        return true;
    }

    /**
     *
     * @return boolean
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
            $wpInstallPaths = $this->wpInstallStorage->getPaths();

            $info = array( 'installs' => $wpInstallPaths );
        }
        else {
            $completedCount = -1;

            if ( !isset($info['installs']) ) {
                Logger::uiError('Expected session data missing! Stopping mass operation.');
            }
            else {
                $batchSize = $this->getBatchSize($info['installs']);

                $batch = array_splice($info['installs'], 0, $batchSize);

                $finishedList = $this->wpInstallStorage->doAction(
                        UserCommand::CMD_STATUS, $batch);

                $completedCount = count($finishedList);

                if ( $completedCount != $batchSize ) {
                    $info['installs'] = array_merge(
                            array_slice($batch, $completedCount), $info['installs']);
                }
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();
            $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                    Logger::getUiMsgs(Logger::UI_ERR));

            $ajaxInfo = array(
                'completed' => $completedCount,
                'errMsgs' => $errMsgs
            );

            if ( empty($info['installs']) ) {
                unset($_SESSION['refreshInfo']);
            }

            AjaxResponse::setAjaxContent(json_encode($ajaxInfo));
            AjaxResponse::outputAndExit();
        }

        return true;
    }

    /**
     *
     * @return boolean
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
            $wpInstallPaths = $this->wpInstallStorage->getPaths();

            $info = array( 'installs' => $wpInstallPaths );
        }
        else {
            $completedCount = -1;

            if ( !isset($info['installs']) ) {
                Logger::uiError('Expected session data missing! Stopping mass operation.');
            }
            else {
                $batchSize = $this->getBatchSize($info['installs']);

                $batch = array_splice($info['installs'], 0, $batchSize);

                $finishedList = $this->wpInstallStorage->doAction(
                        WPInstallStorage::CMD_MASS_UNFLAG, $batch);

                $completedCount = count($finishedList);

                if ( $completedCount != $batchSize ) {
                    $info['installs'] = array_merge(
                            array_slice($batch, $completedCount), $info['installs']);
                }
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();
            $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                    Logger::getUiMsgs(Logger::UI_ERR));
            $succMsgs = array_merge($msgs['succ'],
                    Logger::getUiMsgs(Logger::UI_SUCC));


            $ajaxInfo = array(
                'completed' => $completedCount,
                'errMsgs' => $errMsgs,
                'succMsgs' => $succMsgs
            );

            if ( empty($info['installs']) ) {
                unset($_SESSION['unflagInfo']);
            }

            AjaxResponse::setAjaxContent(json_encode($ajaxInfo));
            AjaxResponse::outputAndExit();
        }

        return true;
    }

    /**
     *
     * @param string  $action
     * @return null
     * @throws LSCMException
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
                Logger::uiError('Invalid input value detected -  No Action Taken');
                return;
            }
        }

        try
        {
            Context::getOption()->setBatchTimeout(0);
        }
        catch ( LSCMException $e )
        {
            $msg = "{$e->getMessage()} Could not set batch timeout.";
            Logger::error($msg);

            throw new LSCMException($msg);
        }

        $this->wpInstallStorage->doAction($action, $list);
    }

    /**
     * Get simple string representations of current installation status.
     *
     * @param WPInstall  $wpInstall
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

        $statusInfo = array( 'status' => $statusString, 'flag_status' => $flagString );

        return $statusInfo;
    }

    /**
     *
     * @param string  $action
     * @param string  $path
     * @return null
     */
    protected function doSingleAction( $action, $path )
    {
        $wpInstall = $this->wpInstallStorage->getWPInstall($path);

        if ( $wpInstall === null ) {
            Logger::uiError('Invalid input value detected - No Action Taken');
            return;
        }

        $badgesDown = $badgesUp = array();
        $removed = false;

        $preStatusInfo = $this->getStatusStrings($wpInstall);

        $this->wpInstallStorage->doAction($action, array( $path ));

        $postStatusInfo = $this->getStatusStrings($wpInstall);

        if ( $postStatusInfo['status'] == 'removed' ) {
            $removed = true;

            $badgesDown[] = $preStatusInfo['status'];
            $badgesDown[] = 'flag';
        }
        else {

            if ( $preStatusInfo['status'] != $postStatusInfo['status'] ) {
                $badgesDown[] = $preStatusInfo['status'];
                $badgesUp[] = $postStatusInfo['status'];
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
        $flagSort = $viewModel->getsortVal('flag');

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

        $actions_td = $this->getAjaxViewContent($viewModel, 'actions_td');
        $status_td = $this->getAjaxViewContent($viewModel, 'status_td');
        $flag_td = $this->getAjaxViewContent($viewModel, 'flag_td');

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();
        $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                Logger::getUiMsgs(Logger::UI_ERR));
        $succMsgs = array_merge($msgs['succ'],
                Logger::getUiMsgs(Logger::UI_SUCC));

        $ajaxReturn = array(
            'displayData' => array(
                'actions_td' => $actions_td,
                'status_search' => $statusSort,
                'status_sort' => $statusSort,
                'status_td' => $status_td,
                'flag_search' => $flagSearch,
                'flag_sort' => $flagSort,
                'flag_td' => $flag_td
            ),
            'badgesDown' => $badgesDown,
            'badgesUp' => $badgesUp,
            'removed' => $removed,
            'errMsgs' => $errMsgs,
            'succMsgs' => $succMsgs
        );

        AjaxResponse::setAjaxContent(json_encode($ajaxReturn));
        AjaxResponse::outputAndExit();
    }

    protected function doWpDashMsgOperation( WPDashMsgs $wpDashMsgs,
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

                if ( $msgId !== '' && $msgId !== NULL && isset($msgs[$msgId]) ) {
                    $msgInfo = $msgs[$msgId];
                }
                else {
                    $msgInfo = array();
                }

                $ajaxReturn = array(
                    'msgInfo' => $msgInfo
                );

                break;
            case WPDashMsgs::ACTION_ADD_MSG:
                $slug = '';

                if ( ($msg = Util::get_request_var('notify_msg')) !== NULL ) {
                    $msg = trim($msg);
                }

                if ( ($slug = Util::get_request_var('notify_slug')) !== NULL ) {
                    $slug = trim($slug);
                }

                if ( $wpDashMsgs->addMsg($type, $msgId, $msg, $slug) ) {
                    $ret = 1;
                }
                else {
                    $ret = 0;
                }

                $ajaxReturn = array(
                    'ret' => $ret
                );

                break;
            case WPDashMsgs::ACTION_DELETE_MSG:

                if ( $wpDashMsgs->deleteMsg($type, $msgId) ) {
                    $ret = 1;
                }
                else {
                    $ret = 0;
                }

                $ajaxReturn = array(
                    'ret' => $ret
                );

                break;
        }

        AjaxResponse::setAjaxContent(json_encode($ajaxReturn));
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @param string  $action
     * @param string  $path
     * @return null
     */
    protected function doSingleDashAction( $action, $path,
            $msgInfoJSON = array() )
    {
        $wpInstall = $this->wpInstallStorage->getWPInstall($path);

        if ( $wpInstall === null ) {
            $ajaxReturn = array(
                'errMsgs' => array('Invalid input value detected - Please use the exact path '
                    . 'displayed in the "Manage Cache Installations" list when testing.'),
                'succMsgs' => array()
            );

            AjaxResponse::setAjaxContent(json_encode($ajaxReturn));
            AjaxResponse::outputAndExit();
        }

        $this->wpInstallStorage->doAction($action, array( $path ),
                array($msgInfoJSON));

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();
        $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                Logger::getUiMsgs(Logger::UI_ERR));
        $succMsgs = array_merge($msgs['succ'],
                Logger::getUiMsgs(Logger::UI_SUCC));

        $ajaxReturn = array(
            'errMsgs' => $errMsgs,
            'succMsgs' => $succMsgs
        );

        AjaxResponse::setAjaxContent(json_encode($ajaxReturn));
        AjaxResponse::outputAndExit();
    }

    /**
     *
     * @param string  $action
     * @param int     $step
     * @return null
     */
    public function massEnableDisable( $action, $step )
    {
        if ( session_status() !== PHP_SESSION_ACTIVE ) {
            session_start();
        }

        $actionUpper = ucfirst($action);

        $info = &$_SESSION["mass{$actionUpper}Info"];

        if ( $step == 1 ) {
            $wpInstallPaths = $this->wpInstallStorage->getPaths();

            $info = array(
                'installs' => $wpInstallPaths,
            );
        }
        elseif ( $step == 2 ) {
            $completedCount = -1;

            if ( !isset($info['installs']) ) {
                Logger::uiError('Expected session data missing! Stopping mass operation.');
            }
            else {
                $batchSize = $this->getBatchSize($info['installs']);

                $batch = array_splice($info['installs'], 0, $batchSize);

                try
                {
                    $finishedList =
                            $this->wpInstallStorage->doAction("mass_{$action}",
                            $batch);

                    $completedCount = count($finishedList);

                    if ( $completedCount != $batchSize ) {
                        $info['installs'] = array_merge(
                                array_slice($batch, $completedCount),
                                $info['installs']);
                    }
                }
                catch ( LSCMException $e )
                {
                    Logger::uiError($e->getMessage() . ' Stopping mass operation.');

                    $info['installs'] = array();
                }

                $succCount = $failCount = $bypassedCount = 0;

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
                            || $wpInstall->hasFatalError()
                            || $wpInstall->isFlagBitSet() ) {

                        $bypassedCount++;
                    }
                }
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();
            $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                    Logger::getUiMsgs(Logger::UI_ERR));
            $succMsgs = array_merge($msgs['succ'],
                    Logger::getUiMsgs(Logger::UI_SUCC));

            $ajaxReturn = array(
                'completed' => $completedCount,
                'bypassedCount' => $bypassedCount,
                'failCount' => $failCount,
                'succCount' => $succCount,
                'errMsgs' => $errMsgs,
                'succMsgs' => $succMsgs
            );

            if ( empty($info['installs']) ) {
                unset($_SESSION["mass{$actionUpper}Info"]);
            }

            AjaxResponse::setAjaxContent(json_encode($ajaxReturn));
            AjaxResponse::outputAndExit();
        }
    }

    /**
     * Force updates LiteSpeed Cache Plugins whose versions
     * match those in $verList and refreshes status as well. Flagged
     * installations are bypassed.
     *
     * @param int  $step  Value of 2 will prep session info, 3 will begin
     * 					   changing installed LSCWP versions in batches.
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
            $verNum = Util::get_request_var('version_num');
            $verList = Util::get_request_list('selectedVers');
            $wpInstallPaths = $this->wpInstallStorage->getPaths();

            if ( empty($verList) ) {
                /**
                 * Version list should not be empty
                 */
                return 0;
            }

            $info = array(
                'installs' => $wpInstallPaths,
                'verNum' => $verNum,
                'verList' => $verList,
            );
        }
        elseif ( $step == 3 ) {
            $completedCount = -1;

            if ( !isset($info['installs']) ) {
                Logger::uiError('Expected session data missing! Stopping mass operation.');
            }
            else {
                $batchSize = $this->getBatchSize($info['installs']);

                $batch = array_splice($info['installs'], 0, $batchSize);

                $fromVersions = $info['verList'];
                $toVersion = $info['verNum'];

                $extraArgs = array( implode(',', $fromVersions), $toVersion );

                try
                {
                    $finishedList = $this->wpInstallStorage->doAction(
                            UserCommand::CMD_MASS_UPGRADE, $batch, $extraArgs);

                    $completedCount = count($finishedList);

                    if ( $completedCount != $batchSize ) {
                        $info['installs'] = array_merge(
                                array_slice($batch, $completedCount),
                                $info['installs']);
                    }
                }
                catch ( LSCMException $e )
                {
                    Logger::uiError($e->getMessage() . ' Stopping mass operation.');

                    $info['installs'] = array();
                }
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();
            $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                    Logger::getUiMsgs(Logger::UI_ERR));

            $ajaxReturn = array(
                'completed' => $completedCount,
                'errMsgs' => $errMsgs,
            );

            if ( empty($info['installs']) || $completedCount == -1 ) {
                unset($_SESSION['verInfo']);
            }

            AjaxResponse::setAjaxContent(json_encode($ajaxReturn));
            AjaxResponse::outputAndExit();
        }
    }

    /**
     *
     * @return boolean
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
            $wpInstallPaths = $this->wpInstallStorage->getPaths();

            $info = array(
                'installs' => $wpInstallPaths,
                'msgInfo' => base64_encode($this->getDashMsgInfoJSON())
            );
        }
        else {
            $completedCount = -1;

            if ( !isset($info['installs']) ) {
                Logger::uiError('Expected session data missing! Stopping mass operation.');
            }
            else {
                $batchSize = $this->getBatchSize($info['installs']);

                $batch = array_splice($info['installs'], 0, $batchSize);
                $msgInfoJSON = $info['msgInfo'];

                try
                {
                    $finishedList = $this->wpInstallStorage->doAction(
                            UserCommand::CMD_MASS_DASH_NOTIFY, $batch,
                            array($msgInfoJSON));

                    $completedCount = count($finishedList);

                    if ( $completedCount != $batchSize ) {
                        $info['installs'] = array_merge(
                                array_slice($batch, $completedCount), $info['installs']);
                    }
                }
                catch ( LSCMException $e )
                {
                    Logger::uiError($e->getMessage() . ' Stopping mass operation.');

                    $info['installs'] = array();
                }

                $succCount = $failCount = $bypassedCount = 0;

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
                            || $wpInstall->hasFatalError() ) {

                        $bypassedCount++;
                    }
                }
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();
            $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                    Logger::getUiMsgs(Logger::UI_ERR));
            $succMsgs = array_merge($msgs['succ'],
                    Logger::getUiMsgs(Logger::UI_SUCC));

            $ajaxInfo = array(
                'completed' => $completedCount,
                'bypassedCount' => $bypassedCount,
                'failCount' => $failCount,
                'succCount' => $succCount,
                'errMsgs' => $errMsgs,
                'succMsgs' => $succMsgs
            );

            if ( empty($info['installs']) ) {
                unset($_SESSION['massDashNotifyInfo']);
            }

            AjaxResponse::setAjaxContent(json_encode($ajaxInfo));
            AjaxResponse::outputAndExit();
        }

        return true;
    }

    /**
     *
     * @return boolean
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
            $wpInstallPaths = $this->wpInstallStorage->getPaths();

            $info = array( 'installs' => $wpInstallPaths );
        }
        else {
            $completedCount = -1;

            if ( !isset($info['installs']) ) {
                Logger::uiError('Expected session data missing! Stopping mass operation.');
            }
            else {
                $batchSize = $this->getBatchSize($info['installs']);

                $batch = array_splice($info['installs'], 0, $batchSize);

                try
                {
                    $finishedList = $this->wpInstallStorage->doAction(
                            UserCommand::CMD_MASS_DASH_DISABLE, $batch);

                    $completedCount = count($finishedList);

                    if ( $completedCount != $batchSize ) {
                        $info['installs'] = array_merge(
                                array_slice($batch, $completedCount), $info['installs']);
                    }
                }
                catch ( LSCMException $e )
                {
                    Logger::uiError($e->getMessage() . ' Stopping mass operation.');

                    $info['installs'] = array();
                }

                $succCount = $failCount = $bypassedCount = 0;

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
                            || $wpInstall->hasFatalError() ) {

                        $bypassedCount++;
                    }
                }
            }

            $msgs = $this->wpInstallStorage->getAllCmdMsgs();
            $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                    Logger::getUiMsgs(Logger::UI_ERR));
            $succMsgs = array_merge($msgs['succ'],
                    Logger::getUiMsgs(Logger::UI_SUCC));

            $ajaxInfo = array(
                'completed' => $completedCount,
                'bypassedCount' => $bypassedCount,
                'failCount' => $failCount,
                'succCount' => $succCount,
                'errMsgs' => $errMsgs,
                'succMsgs' => $succMsgs
            );

            if ( empty($info['installs']) ) {
                unset($_SESSION['massDashDisableInfo']);
            }

            AjaxResponse::setAjaxContent(json_encode($ajaxInfo));
            AjaxResponse::outputAndExit();
        }

        return true;
    }

}
