<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\WPInstallStorage;
use \Lsc\Wp\WPInstall;
use \Lsc\Wp\PluginVersion;
use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;

class ManageViewModel
{

    const FLD_ICON_DIR = 'iconDir';
    const FLD_SCAN_BTN_NAME = 'scanBtnName';
    const FLD_BTN_STATE = 'btnState';
    const FLD_ACTIVE_VER = 'activeVer';
    const FLD_SHOW_LIST = 'showList';
    const FLD_LIST_DATA = 'listData';
    const FLD_COUNT_DATA = 'countData';
    const FLD_INFO_MSGS = 'infoMsgs';
    const FLD_SUCC_MSGS = 'succMsgs';
    const FLD_ERR_MSGS = 'errMsgs';
    const FLD_WARN_MSGS = 'warnMsgs';
    const COUNT_DATA_INSTALLS = 'installs';
    const COUNT_DATA_ENABLED = 'enabled';
    const COUNT_DATA_DISABLED = 'disabled';
    const COUNT_DATA_WARN = 'warn';
    const COUNT_DATA_ERROR = 'err';
    const COUNT_DATA_FLAGGED = 'flagged';
    const COUNT_DATA_UNFLAGGED = 'unflagged';

    /**
     * @var WPInstallStorage
     */
    protected $wpInstallStorage;

    /**
     * @var mixed[]
     */
    protected $tplData = array();

    /**
     * @var string
     */
    protected $iconDir = '';

    /**
     *
     * @param WPInstallStorage  $wpInstallStorage
     */
    public function __construct( WPInstallStorage $wpInstallStorage )
    {
        $this->wpInstallStorage = $wpInstallStorage;

        $this->init();
    }

    protected function init()
    {
        $this->setIconDir();
        $this->setBtnDataAndListVisibility();
        $this->setActiveVerData();
        $this->setListAndCountData();
        $this->setMsgData();
    }

    /**
     *
     * @param string  $field
     * @return null|mixed
     */
    public function getTplData( $field )
    {
        if ( !isset($this->tplData[$field]) ) {
            return null;
        }

        return $this->tplData[$field];
    }

    protected function setIconDir()
    {
        $iconDir = '';

        try
        {
            $iconDir = Context::getOption()->getIconDir();
        }
        catch ( LSCMException $e )
        {
            Logger::debug($e->getMessage() . ' Could not get icon directory.');
        }

        $this->tplData[self::FLD_ICON_DIR] = $iconDir;
    }

    protected function setBtnDataAndListVisibility()
    {
        $scanBtnName = 'Re-scan';
        $btnState = 'disabled';

        if ( ($errStatus = $this->wpInstallStorage->getError()) !== 0 ) {
            $this->tplData[self::FLD_SHOW_LIST] = false;

            if ( $errStatus == WPInstallStorage::ERR_NOT_EXIST ) {
                $scanBtnName = 'Scan';
                $msg = 'Start by clicking Scan. This will discover all active WordPress '
                        . 'installations and add them to a list below.';
            }
            elseif ( $errStatus == WPInstallStorage::ERR_VERSION_LOW ) {
                $scanBtnName = 'Scan';
                $msg = 'To further improve Cache Management features in this version, current '
                        . 'installations must be re-discovered. Please perform a Scan now.';
            }
            else {
                $msg = 'Scan data could not be read. Please perform a Re-scan.';
            }

            Logger::uiInfo($msg);
        }
        else {
            $this->tplData[self::FLD_SHOW_LIST] = true;
            $discoveredCount = $this->wpInstallStorage->getCount();

            if ( $discoveredCount > 0 ) {
                $btnState = '';
            }
        }

        $this->tplData[self::FLD_SCAN_BTN_NAME] = $scanBtnName;
        $this->tplData[self::FLD_BTN_STATE] = $btnState;
    }

    protected function setListAndCountData()
    {
        $listData = array();

        $countData = array(
            self::COUNT_DATA_INSTALLS => 0,
            self::COUNT_DATA_ENABLED => 0,
            self::COUNT_DATA_DISABLED => 0,
            self::COUNT_DATA_WARN => 0,
            self::COUNT_DATA_ERROR => 0,
            self::COUNT_DATA_FLAGGED => 0,
            self::COUNT_DATA_UNFLAGGED => 0
        );

        $wpInstalls = $this->wpInstallStorage->getAllWPInstalls();

        if ( $wpInstalls !== null ) {
            $countData[self::COUNT_DATA_INSTALLS] = count($wpInstalls);

            foreach ( $wpInstalls as $wpInstall ) {
                $info = array(
                    'statusData' =>
                            $this->getStatusDisplayData($wpInstall, $countData),
                    'flagData' => $this->getFlagDisplayData($wpInstall, $countData),
                    'siteUrl' => $wpInstall->getData(WPInstall::FLD_SITEURL)
                );

                $listData[$wpInstall->getPath()] = $info;
            }
        }

        $this->tplData[self::FLD_LIST_DATA] = $listData;
        $this->tplData[self::FLD_COUNT_DATA] = $countData;
    }

    /**
     *
     * @param WPInstall  $wpInstall
     * @param int[]      $countData
     * @return string[]
     */
    protected function getStatusDisplayData( WPInstall $wpInstall,
            &$countData )
    {
        $statusInfo = array(
            'disabled' => array(
                'sort' => 'disabled',
                'state' => '<span class="glyphicon glyphicon-flash status-disabled" data-uk-tooltip '
                        . 'title="LSCWP is disabled."></span>',
                'btn_content' => '<span class="enable_btn"></span>',
                'btn_title' => 'Click to enable LSCache',
                'onclick' => 'onclick="javascript:lscwpEnableSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip',
                'btn_state' => ''
            ),
            'enabled' => array(
                'sort' => 'enabled',
                'state' => '<span class="glyphicon glyphicon-flash status-enabled" data-uk-tooltip '
                        . 'title="LSCWP is enabled."></span>',
                'btn_content' => '<span class="disable_btn"></span>',
                'btn_title' => 'Click to disable LSCache',
                'onclick' => 'onclick="javascript:lscwpDisableSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip',
                'btn_state' => ''
            ),
            'adv_cache' => array(
                'sort' => 'warning',
                'state' => '<span class="status-warning" data-uk-tooltip '
                        . 'title="LSCache is enabled but not caching. Please visit the '
                        . 'WordPress Dashboard for more information."></span>',
                'btn_content' => '<span class="disable_btn"></span>',
                'btn_title' => 'Click to disable LSCache',
                'onclick' => 'onclick="javascript:lscwpDisableSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip',
                'btn_state' => ''
            ),
            'disabled_no_active_ver' => array(
                'sort' => 'disabled',
                'state' => '<span class="glyphicon glyphicon-flash status-disabled" data-uk-tooltip '
                . 'title="LSCWP is disabled."></span>',
                'btn_content' => '<span class="inactive-action-btn" data-uk-tooltip '
                        . 'title="No active LSCWP version set! Cannot enable LSCache."></span>',
                'onclick' => '',
                'btn_attributes' => '',
                'btn_state' => 'disabled',
            ),
            'error' => array(
                'sort' => 'error',
                /**
                 * 'state' added individually later.
                 */
                'btn_title' => '',
                'btn_content' => '<span class="inactive-action-btn"></span>',
                'onclick' => '',
                'btn_attributes' => '',
                'btn_state' => 'disabled'
            )
        );

        $wpStatus = $wpInstall->getStatus();

        if ( $wpInstall->hasFatalError($wpStatus) ) {
            $countData[self::COUNT_DATA_ERROR]++;

            $link = 'https://www.litespeedtech.com/support/wiki/doku.php/litespeed_wiki:cpanel:whm-plugin-cache-manager-error-status';

            if ( $wpStatus & WPInstall::ST_ERR_EXECMD ) {
                $stateMsg = 'WordPress fatal error encountered during action execution. This is '
                        . 'most likely caused by custom code in this WordPress installation.';
                $link .= '#fatal_error_encountered_during_action_execution';
            }
            if ( $wpStatus & WPInstall::ST_ERR_EXECMD_DB ) {
                $stateMsg = 'Error establishing WordPress database connection.';
                $link .= '#';
            }
            elseif ( $wpStatus & WPInstall::ST_ERR_TIMEOUT ) {
                $stateMsg = 'Timeout occurred during action execution.';
                $link .= '#timeout_occurred_during_action_execution';
            }
            elseif ( $wpStatus & WPInstall::ST_ERR_SITEURL ) {
                $stateMsg = 'Could not retrieve WordPress siteURL.';
                $link .= '#could_not_retrieve_wordpress_siteurl';
            }
            elseif ( $wpStatus & WPInstall::ST_ERR_DOCROOT ) {
                $stateMsg = 'Could not match WordPress siteURL to a known control panel '
                        . 'docroot.';
                $link .= '#could_not_match_wordpress_siteurl_to_a_known_cpanel_docroot';
            }
            elseif ( $wpStatus & WPInstall::ST_ERR_WPCONFIG ) {
                $stateMsg = 'Could not find a valid wp-config.php file.';
                $link .= '#could_not_find_a_valid_wp-configphp_file';
            }

            $stateMsg .= ' Click for more information.';

            $currStatusData = $statusInfo['error'];
            $currStatusData['state'] =
                    "<a href=\"{$link}\" target=\"_blank\" rel=\"noopener\" "
                    . "data-uk-tooltip title =\"{$stateMsg}\" class=\"status-error\"></a>";
        }
        elseif ( ($wpStatus & WPInstall::ST_PLUGIN_INACTIVE ) ) {
            $countData[self::COUNT_DATA_DISABLED]++;

            $currVer = $this->getTplData(self::FLD_ACTIVE_VER);

            if ( $currVer == false ) {
                $currStatusData = $statusInfo['disabled_no_active_ver'];
            }
            else {
                $currStatusData = $statusInfo['disabled'];
            }
        }
        elseif ( !($wpStatus & WPInstall::ST_LSC_ADVCACHE_DEFINED) ) {
            $countData[self::COUNT_DATA_WARN]++;
            $currStatusData = $statusInfo['adv_cache'];
        }
        else {
            $countData[self::COUNT_DATA_ENABLED]++;
            $currStatusData = $statusInfo['enabled'];
        }

        return $currStatusData;
    }

    /**
     *
     * @param WPInstall  $wpInstall
     * @param int[]      $countData
     * @return string[]
     */
    protected function getFlagDisplayData( WPInstall $wpInstall, &$countData )
    {
        $flagInfo = array(
            0 => array(
                'sort' => 'unflagged',
                'icon' => '<span class="glyphicon glyphicon-flag ls-flag ls-flag-unset"></span>',
                'btn_title' => 'Click to set flag',
                'onclick' => 'onclick="javascript:lscwpFlagSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip'
            ),
            1 => array(
                'sort' => 'flagged',
                'icon' => '<span class="glyphicon glyphicon-flag ls-flag ls-flag-set"></span>',
                'btn_title' => 'Click to unset flag',
                'onclick' => 'onclick="javascript:lscwpUnflagSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip'
            ),
        );

        $wpStatus = $wpInstall->getStatus();

        if ( ($wpStatus & WPInstall::ST_FLAGGED ) ) {
            $countData[self::COUNT_DATA_FLAGGED]++;
            $currFlagData = $flagInfo[1];
        }
        else {
            $countData[self::COUNT_DATA_UNFLAGGED]++;
            $currFlagData = $flagInfo[0];
        }

        return $currFlagData;
    }

    protected function setActiveVerData()
    {
        try
        {
            $currVer = PluginVersion::getCurrentVersion();
        }
        catch ( LSCMException $e )
        {
            Logger::debug($e->getMessage() . ' Could not get active LSCWP version.');

            $displayWarning = 'Active LiteSpeed Cache Plugin version is not set. Enable operations '
                    . 'cannot be performed. Please got to '
                    . '<a href="?do=lscwpVersionManager" title="Go to Version Manager">'
                    . 'Version Manager</a> to select a version.';
            Logger::uiWarning($displayWarning);

            $currVer = false;
        }

        $this->tplData[self::FLD_ACTIVE_VER] = $currVer;
    }

    protected function setMsgData()
    {
        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        $infoMsgs = Logger::getUiMsgs(Logger::UI_INFO);
        $succMsgs = array_merge($msgs['succ'],
                Logger::getUiMsgs(Logger::UI_SUCC));
        $errMsgs = array_merge($msgs['fail'], $msgs['err'],
                Logger::getUiMsgs(Logger::UI_ERR));
        $warnMsgs = Logger::getUiMsgs(Logger::UI_WARN);

        $this->tplData[self::FLD_INFO_MSGS] = $infoMsgs;
        $this->tplData[self::FLD_SUCC_MSGS] = $succMsgs;
        $this->tplData[self::FLD_ERR_MSGS] = $errMsgs;
        $this->tplData[self::FLD_WARN_MSGS] = $warnMsgs;
    }

    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/Manage.tpl';
    }

}
