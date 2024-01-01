<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2023 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use Lsc\Wp\Context\Context;
use Lsc\Wp\Util;
use Lsc\Wp\WPInstallStorage;
use Lsc\Wp\WPInstall;
use Lsc\Wp\PluginVersion;
use Lsc\Wp\Logger;
use Lsc\Wp\LSCMException;

class ManageViewModel
{

    /**
     * @var string
     */
    const FLD_ICON_DIR = 'iconDir';

    /**
     * @var string
     */
    const FLD_SCAN_BTN_NAME = 'scanBtnName';

    /**
     * @var string
     */
    const FLD_BTN_STATE = 'btnState';

    /**
     * @var string
     */
    const FLD_ACTIVE_VER = 'activeVer';

    /**
     * @var string
     */
    const FLD_SHOW_LIST = 'showList';

    /**
     * @var string
     */
    const FLD_LIST_DATA = 'listData';

    /**
     * @var string
     */
    const FLD_COUNT_DATA = 'countData';

    /**
     * @var string
     */
    const FLD_INFO_MSGS = 'infoMsgs';

    /**
     * @var string
     */
    const FLD_SUCC_MSGS = 'succMsgs';

    /**
     * @var string
     */
    const FLD_ERR_MSGS = 'errMsgs';

    /**
     * @var string
     */
    const FLD_WARN_MSGS = 'warnMsgs';

    /**
     * @var string
     */
    const COUNT_DATA_INSTALLS = 'installs';

    /**
     * @var string
     */
    const COUNT_DATA_ENABLED = 'enabled';

    /**
     * @var string
     */
    const COUNT_DATA_DISABLED = 'disabled';

    /**
     * @var string
     */
    const COUNT_DATA_WARN = 'warn';

    /**
     * @var string
     */
    const COUNT_DATA_ERROR = 'err';

    /**
     * @var string
     */
    const COUNT_DATA_FLAGGED = 'flagged';

    /**
     * @var string
     */
    const COUNT_DATA_UNFLAGGED = 'unflagged';

    /**
     * @var WPInstallStorage
     */
    protected $wpInstallStorage;

    /**
     * @var array
     */
    protected $tplData = array();

    /**
     * @var string
     */
    protected $iconDir = '';

    /**
     * @since 1.13.3
     * @var string[][]
     */
    protected $statusInfo = array(
        'disabled'               => array(
            'sort'           => 'disabled',
            'state'          => '<span '
                . 'class="glyphicon glyphicon-flash status-disabled" '
                . 'data-uk-tooltip title="LSCWP is disabled."></span>',
            'btn_content'    => '<span class="enable_btn"></span>',
            'btn_title'      => 'Click to enable LSCache',
            'onclick'        => 'onclick="javascript:lscwpEnableSingle(this);"',
            'btn_attributes' => 'data-uk-tooltip',
            'btn_state'      => ''
        ),
        'enabled'                => array(
            'sort'           => 'enabled',
            'state'          => '<span '
                . 'class="glyphicon glyphicon-flash status-enabled" '
                . 'data-uk-tooltip title="LSCWP is enabled."></span>',
            'btn_content'    => '<span class="disable_btn"></span>',
            'btn_title'      => 'Click to disable LSCache',
            'onclick'        =>
                'onclick="javascript:lscwpDisableSingle(this);"',
            'btn_attributes' => 'data-uk-tooltip',
            'btn_state'      => ''
        ),
        'adv_cache'              => array(
            'sort'           => 'warning',
            'state'          => '<span class="status-warning" data-uk-tooltip '
                . 'title="LSCache is enabled but not caching. Please visit the '
                . 'WordPress Dashboard for more information."></span>',
            'btn_content'    => '<span class="disable_btn"></span>',
            'btn_title'      => 'Click to disable LSCache',
            'onclick'        =>
                'onclick="javascript:lscwpDisableSingle(this);"',
            'btn_attributes' => 'data-uk-tooltip',
            'btn_state'      => ''
        ),
        'disabled_no_active_ver' => array(
            'sort'           => 'disabled',
            'state'          => '<span '
                . 'class="glyphicon glyphicon-flash status-disabled" '
                . 'data-uk-tooltip title="LSCWP is disabled."></span>',
            'btn_content'    => '<span class="inactive-action-btn" '
                . 'data-uk-tooltip '
                . 'title="No active LSCWP version set! Cannot enable LSCache.">'
                . '</span>',
            'btn_title'      => '',
            'onclick'        => '',
            'btn_attributes' => '',
            'btn_state'      => 'disabled',
        ),
        'error'                  => array(
            'sort'           => 'error',
            /**
             * 'state' added individually later.
             */
            'btn_title'      => '',
            'btn_content'    => '<span class="inactive-action-btn"></span>',
            'onclick'        => '',
            'btn_attributes' => '',
            'btn_state'      => 'disabled'
        )
    );

    /**
     * @since 1.13.3
     * @var string[][]
     */
    protected $flagInfo = array(
        'unflagged' => array(
            'sort'           => 'unflagged',
            'icon'           => '<span '
                . 'class="glyphicon glyphicon-flag ls-flag ls-flag-unset">'
                . '</span>',
            'btn_title'      => 'Click to set flag',
            'onclick'        => 'onclick="javascript:lscwpFlagSingle(this);"',
            'btn_attributes' => 'data-uk-tooltip'
        ),
        'flagged'   => array(
            'sort'           => 'flagged',
            'icon'           => '<span '
                . 'class="glyphicon glyphicon-flag ls-flag ls-flag-set">'
                . '</span>',
            'btn_title'      => 'Click to unset flag',
            'onclick'        => 'onclick="javascript:lscwpUnflagSingle(this);"',
            'btn_attributes' => 'data-uk-tooltip'
        ),
    );

    /**
     *
     * @param WPInstallStorage $wpInstallStorage
     *
     * @throws LSCMException  Thrown indirectly by $this->init() call.
     */
    public function __construct( WPInstallStorage $wpInstallStorage )
    {
        $this->wpInstallStorage = $wpInstallStorage;

        $this->init();
    }

    /**
     *
     * @throws LSCMException  thrown indirectly by $this->setIconDir() call.
     * @throws LSCMException  thrown indirectly by $this->setActiveVerData()
     *     call.
     * @throws LSCMException  thrown indirectly by $this->setMsgData() call.
     */
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
     * @param string $field
     *
     * @return null|mixed
     */
    public function getTplData( $field )
    {
        if ( !isset($this->tplData[$field]) ) {
            return null;
        }

        return $this->tplData[$field];
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
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

    /**
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiInfo() call.
     */
    protected function setBtnDataAndListVisibility()
    {
        $scanBtnName = 'Re-scan';
        $btnState = 'disabled';

        if ( ($errStatus = $this->wpInstallStorage->getError()) !== 0 ) {
            $this->tplData[self::FLD_SHOW_LIST] = false;

            if ( $errStatus == WPInstallStorage::ERR_NOT_EXIST ) {
                $scanBtnName = 'Scan';
                $msg = 'Start by clicking Scan. This will discover all active '
                    . 'WordPress installations and add them to a list below.';
            }
            elseif ( $errStatus == WPInstallStorage::ERR_VERSION_LOW ) {
                $scanBtnName = 'Scan';
                $msg = 'To further improve Cache Management features in this '
                    . 'version, current installations must be re-discovered. '
                    . 'Please perform a Scan now.';
            }
            else {
                $msg = 'Scan data could not be read. Please perform a Re-scan.';
            }

            Logger::uiInfo($msg);
        }
        else {
            $this->tplData[self::FLD_SHOW_LIST] = true;

            if ( $this->wpInstallStorage->getCount() > 0 ) {
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
            self::COUNT_DATA_INSTALLS  => 0,
            self::COUNT_DATA_ENABLED   => 0,
            self::COUNT_DATA_DISABLED  => 0,
            self::COUNT_DATA_WARN      => 0,
            self::COUNT_DATA_ERROR     => 0,
            self::COUNT_DATA_FLAGGED   => 0,
            self::COUNT_DATA_UNFLAGGED => 0
        );

        $wpInstalls = $this->wpInstallStorage->getAllWPInstalls();

        if ( $wpInstalls !== null ) {
            $countData[self::COUNT_DATA_INSTALLS] = count($wpInstalls);

            foreach ( $wpInstalls as $wpInstall ) {
                $listData[$wpInstall->getPath()] = array(
                    'statusData' =>
                        $this->getStatusDisplayData($wpInstall, $countData),
                    'flagData'   =>
                        $this->getFlagDisplayData($wpInstall, $countData),
                    'siteUrl'    => Util::tryIdnToUtf8(
                        $wpInstall->getData(WPInstall::FLD_SITEURL)
                    )
                );

            }
        }

        $this->tplData[self::FLD_LIST_DATA] = $listData;
        $this->tplData[self::FLD_COUNT_DATA] = $countData;
    }

    /**
     *
     * @param WPInstall $wpInstall
     * @param int[]     $countData
     *
     * @return string[]
     */
    protected function getStatusDisplayData(
        WPInstall $wpInstall,
        array     &$countData )
    {
        $wpStatus = $wpInstall->getStatus();

        if ( $wpInstall->hasFatalError($wpStatus) ) {
            $countData[self::COUNT_DATA_ERROR]++;

            $link = 'https://docs.litespeedtech.com/cp/cpanel'
                . '/wp-cache-management/#whm-plugin-cache-manager-error-status';

            $stateMsg = '';

            if ( $wpStatus & WPInstall::ST_ERR_EXECMD ) {
                $stateMsg = 'WordPress fatal error encountered during action '
                    . 'execution. This is most likely caused by custom code in '
                    . 'this WordPress installation.';
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
                $stateMsg = 'Could not match WordPress siteURL to a known '
                    . 'control panel docroot.';
                $link .= '#could_not_match_wordpress_siteurl_to_a_known_'
                    . 'cpanel_docroot';
            }
            elseif ( $wpStatus & WPInstall::ST_ERR_WPCONFIG ) {
                $stateMsg = 'Could not find a valid wp-config.php file.';
                $link .= '#could_not_find_a_valid_wp-configphp_file';
            }

            $stateMsg .= ' Click for more information.';

            $currStatusData = $this->statusInfo['error'];
            $currStatusData['state'] = "<a href=\"$link\" target=\"_blank\" "
                . "rel=\"noopener\" data-uk-tooltip title =\"$stateMsg\" "
                . 'class="status-error"></a>';
        }
        elseif ( ($wpStatus & WPInstall::ST_PLUGIN_INACTIVE ) ) {
            $countData[self::COUNT_DATA_DISABLED]++;

            if ( !$this->getTplData(self::FLD_ACTIVE_VER) ) {
                $currStatusData = $this->statusInfo['disabled_no_active_ver'];
            }
            else {
                $currStatusData = $this->statusInfo['disabled'];
            }
        }
        elseif ( !($wpStatus & WPInstall::ST_LSC_ADVCACHE_DEFINED) ) {
            $countData[self::COUNT_DATA_WARN]++;
            $currStatusData = $this->statusInfo['adv_cache'];
        }
        else {
            $countData[self::COUNT_DATA_ENABLED]++;
            $currStatusData = $this->statusInfo['enabled'];
        }

        return $currStatusData;
    }

    /**
     *
     * @param WPInstall $wpInstall
     * @param int[]     $countData
     *
     * @return string[]
     */
    protected function getFlagDisplayData(
        WPInstall $wpInstall,
        array     &$countData )
    {
        if ( ($wpInstall->getStatus() & WPInstall::ST_FLAGGED ) ) {
            $countData[self::COUNT_DATA_FLAGGED]++;
            $currFlagData = $this->flagInfo['flagged'];
        }
        else {
            $countData[self::COUNT_DATA_UNFLAGGED]++;
            $currFlagData = $this->flagInfo['unflagged'];
        }

        return $currFlagData;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::uiWarning() call.
     */
    protected function setActiveVerData()
    {
        try
        {
            $currVer = PluginVersion::getCurrentVersion();
        }
        catch ( LSCMException $e )
        {
            Logger::debug(
                $e->getMessage() . ' Could not get active LSCWP version.'
            );

            Logger::uiWarning(
                'Active LiteSpeed Cache Plugin version is not set. Enable '
                    . 'operations cannot be performed. Please go to '
                    . '<a href="?do=lscwpVersionManager" '
                    . 'title="Go to Version Manager">Version Manager</a> to '
                    . 'select a version.'
                );

            $currVer = false;
        }

        $this->tplData[self::FLD_ACTIVE_VER] = $currVer;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     */
    protected function setMsgData()
    {
        $this->tplData[self::FLD_INFO_MSGS] =
            Logger::getUiMsgs(Logger::UI_INFO);

        $this->tplData[self::FLD_WARN_MSGS] =
            Logger::getUiMsgs(Logger::UI_WARN);

        $msgs = $this->wpInstallStorage->getAllCmdMsgs();

        $this->tplData[self::FLD_SUCC_MSGS] = array_merge(
            $msgs['succ'],
            Logger::getUiMsgs(Logger::UI_SUCC)
        );

        $this->tplData[self::FLD_ERR_MSGS] = array_merge(
            $msgs['fail'],
            $msgs['err'],
            Logger::getUiMsgs(Logger::UI_ERR)
        );
    }

    /**
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by Context::getOption() call.
     */
    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/Manage.tpl';
    }

}
