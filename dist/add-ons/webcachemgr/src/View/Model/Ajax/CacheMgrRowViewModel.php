<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp\View\Model\Ajax;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\PluginVersion;
use Lsc\Wp\Util;
use \Lsc\Wp\WPInstall;

class CacheMgrRowViewModel
{

    const FLD_LIST_DATA = 'listData';

    /**
     * @var WPInstall
     */
    protected $wpInstall;

    /**
     * @var mixed[]
     */
    protected $tplData = array();

    /**
     *
     * @param WPInstall  $wpInstall
     */
    public function __construct( WPInstall $wpInstall )
    {
        $this->wpInstall = $wpInstall;

        $this->init();
    }

    protected function init()
    {
        $this->getActiveVerData();
        $this->setListRowData();
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

    /**
     *
     * @param string  $type
     * @return string
     */
    public function getSortVal( $type )
    {
        $listData = $this->getTplData(self::FLD_LIST_DATA);

        return $listData[$this->wpInstall->getPath()]["{$type}Data"]['sort'];
    }

    protected function setListRowData()
    {
        $listData = array();

        $info = array(
            'statusData' =>
            $this->getStatusDisplayData(),
            'flagData' => $this->getFlagDisplayData(),
            'siteUrl' => Util::tryIdnToUtf8(
                $this->wpInstall->getData(WPInstall::FLD_SITEURL)
            )
        );

        $listData[$this->wpInstall->getPath()] = $info;

        $this->tplData[self::FLD_LIST_DATA] = $listData;
    }

    /**
     *
     * @return string[]
     */
    protected function getStatusDisplayData()
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
            ),
            'removed' => array(
                'sort' => 'removed',
                'state' => '<span class="status-removed" data-uk-tooltip '
                . 'title="Installation could not be found and has been removed."></span>',
                'btn_content' => '<span class="inactive-action-btn"></span>',
                'onclick' => '',
                'btn_attributes' => '',
                'btn_state' => 'disabled',
            )
        );


        $wpStatus = $this->wpInstall->getStatus();

        if ( $wpStatus & WPInstall::ST_ERR_REMOVE ) {
            $currStatusData = $statusInfo['removed'];
        }
        elseif ( $this->wpInstall->hasFatalError($wpStatus) ) {

            $link = 'https://docs.litespeedtech.com/cp/cpanel'
                . '/wp-cache-management/#whm-plugin-cache-manager-error-status';

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

            $stateMsg .= ' Click for more info.';

            $currStatusData = $statusInfo['error'];
            $currStatusData['state'] = "<a href=\"{$link}\" target=\"_blank\" rel=\"noopener\" "
                    . "data-uk-tooltip title =\"{$stateMsg}\" class=\"status-error\"></a>";
        }
        elseif ( ($wpStatus & WPInstall::ST_PLUGIN_INACTIVE ) ) {

            if ( $this->getActiveVerData() == false ) {
                $currStatusData = $statusInfo['disabled_no_active_ver'];
            }
            else {
                $currStatusData = $statusInfo['disabled'];
            }
        }
        elseif ( !($wpStatus & WPInstall::ST_LSC_ADVCACHE_DEFINED) ) {
            $currStatusData = $statusInfo['adv_cache'];
        }
        else {
            $currStatusData = $statusInfo['enabled'];
        }

        return $currStatusData;
    }

    /**
     *
     * @return string[]
     */
    protected function getFlagDisplayData()
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
            2 => array (
                'sort' => 'removed',
                'icon' => '<span class="glyphicon glyphicon-flag ls-flag ls-flag-removed"></span>',
                'btn_title' => '',
                'onclick' => '',
                'btn_attributes' => ''
            )
        );

        $wpStatus = $this->wpInstall->getStatus();

        if ( $wpStatus & WPInstall::ST_ERR_REMOVE ) {
            $currFlagData = $flagInfo[2];
        }
        elseif ( ($wpStatus & WPInstall::ST_FLAGGED ) ) {
            $currFlagData = $flagInfo[1];
        }
        else {
            $currFlagData = $flagInfo[0];
        }

        return $currFlagData;
    }

    /**
     *
     * @return boolean|string
     */
    protected function getActiveVerData()
    {
        try
        {
            $currVer = PluginVersion::getCurrentVersion();
        }
        catch ( LSCMException $e )
        {
            //don't care about the exception in ajax load.
            $currVer = false;
        }

        return $currVer;
    }

    /**
     *
     * @param string  $tplID
     * @return null|string
     */
    public function getTpl( $tplID )
    {
        $sharedTplDir = Context::getOption()->getSharedTplDir();

        switch ($tplID) {
            case 'actions_td':
                return "{$sharedTplDir}/Ajax/CacheMgrActionsCol.tpl";
            case 'status_td':
                return "{$sharedTplDir}/Ajax/CacheMgrStatusCol.tpl";
            case 'flag_td':
                return "{$sharedTplDir}/Ajax/CacheMgrFlagCol.tpl";
            //no default
        }
    }

}
