<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2025 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\View\Model\Ajax;

use Lsc\Wp\Context\Context;
use Lsc\Wp\LSCMException;
use Lsc\Wp\PluginVersion;
use Lsc\Wp\Util;
use Lsc\Wp\WPInstall;

class CacheMgrRowViewModel
{

    const FLD_LIST_DATA = 'listData';

    /**
     * @var WPInstall
     */
    protected $wpInstall;

    /**
     * @var array
     */
    protected $tplData = array();

    /**
     *
     * @param WPInstall $wpInstall
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
     * @param string $type
     *
     * @return string
     */
    public function getSortVal( $type )
    {
        $listData = $this->getTplData(self::FLD_LIST_DATA);

        return $listData[$this->wpInstall->getPath()]["{$type}Data"]['sort'];
    }

    protected function setListRowData()
    {
        $this->tplData[self::FLD_LIST_DATA] = [
            $this->wpInstall->getPath() => [
                'statusData' => $this->getStatusDisplayData(),
                'flagData'   => $this->getFlagDisplayData(),
                'siteUrl'    => Util::tryIdnToUtf8(
                    $this->wpInstall->getData(WPInstall::FLD_SITEURL)
                )
            ]
        ];
    }

    /**
     *
     * @return string[]
     */
    protected function getStatusDisplayData()
    {
        $statusInfo = [
            'disabled'               => [
                'sort'           => 'disabled',
                'state'          => '<span '
                    . 'class="glyphicon glyphicon-flash status-disabled" '
                    . 'data-uk-tooltip title="LSCWP is disabled."></span>',
                'btn_content'    => '<span class="enable_btn"></span>',
                'btn_title'      => 'Click to enable LSCache',
                'onclick'        =>
                    'onclick="javascript:lscwpEnableSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip',
                'btn_state'      => ''
            ],
            'enabled'                => [
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
            ],
            'adv_cache'              => [
                'sort'           => 'warning',
                'state'          => '<span class="status-warning" '
                    . 'data-uk-tooltip '
                    . 'title="LSCache is enabled but not caching. Please visit '
                    . 'the WordPress Dashboard for more information."></span>',
                'btn_content'    => '<span class="disable_btn"></span>',
                'btn_title'      => 'Click to disable LSCache',
                'onclick'        =>
                    'onclick="javascript:lscwpDisableSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip',
                'btn_state'      => ''
            ],
            'disabled_no_active_ver' => [
                'sort'           => 'disabled',
                'state'          => '<span '
                    . 'class="glyphicon glyphicon-flash status-disabled" '
                    . 'data-uk-tooltip title="LSCWP is disabled."></span>',
                'btn_content'    => '<span class="inactive-action-btn" '
                    . 'data-uk-tooltip '
                    . 'title="No active LSCWP version set! Cannot enable '
                    . 'LSCache."></span>',
                'onclick'        => '',
                'btn_attributes' => '',
                'btn_state'      => 'disabled'
            ],
            'error'                  => [
                'sort'           => 'error',
                /**
                 * 'state' added individually later.
                 */
                'btn_title'      => '',
                'btn_content'    => '<span class="inactive-action-btn"></span>',
                'onclick'        => '',
                'btn_attributes' => '',
                'btn_state'      => 'disabled'
            ],
            'removed'                => [
                'sort'           => 'removed',
                'state'          => '<span class="status-removed" '
                    . 'data-uk-tooltip '
                    . 'title="Installation could not be found and has been '
                    . 'removed."></span>',
                'btn_content'    => '<span class="inactive-action-btn"></span>',
                'onclick'        => '',
                'btn_attributes' => '',
                'btn_state'      => 'disabled'
            ]
        ];

        $wpStatus = $this->wpInstall->getStatus();

        if ( $wpStatus & WPInstall::ST_ERR_REMOVE ) {
            $currStatusData = $statusInfo['removed'];
        }
        elseif ( $this->wpInstall->hasFatalError($wpStatus) ) {
            $fatalErrStateInfo =
                Util::getFatalErrorStateMessageAndLink($wpStatus);

            $currStatusData          = $statusInfo['error'];
            $currStatusData['state'] = '<a '
                . "href=\"{$fatalErrStateInfo['link']}\" "
                . 'target="_blank" rel="noopener" data-uk-tooltip '
                . "title =\"{$fatalErrStateInfo['stateMsg']}\" "
                . 'class="status-error"></a>';
        }
        elseif ( ($wpStatus & WPInstall::ST_PLUGIN_INACTIVE ) ) {

            if ( !$this->getActiveVerData() ) {
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
        $flagInfo = [
            0 => [
                'sort' => 'unflagged',
                'icon'           => '<span '
                    . 'class="glyphicon glyphicon-flag ls-flag ls-flag-unset"'
                    . '></span>',
                'btn_title'      => 'Click to set flag',
                'onclick'        => 'onclick="lscwpFlagSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip'
            ],
            1 => [
                'sort'           => 'flagged',
                'icon'           => '<span '
                    . 'class="glyphicon glyphicon-flag ls-flag ls-flag-set"'
                    . '></span>',
                'btn_title'      => 'Click to unset flag',
                'onclick'        => 'onclick="lscwpUnflagSingle(this);"',
                'btn_attributes' => 'data-uk-tooltip'
            ],
            2 => [
                'sort'           => 'removed',
                'icon'           => '<span '
                    . 'class="glyphicon glyphicon-flag ls-flag ls-flag-removed"'
                    . '></span>',
                'btn_title'      => '',
                'onclick'        => '',
                'btn_attributes' => ''
            ]
        ];

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
     * @return bool|string
     */
    protected function getActiveVerData()
    {
        try {
            return PluginVersion::getCurrentVersion();
        }
        catch ( LSCMException $e ) {
            //don't care about the exception in ajax load.
            return false;
        }

    }

    /**
     *
     * @param string $tplID
     *
     * @return null|string
     *
     * @throws LSCMException  Thrown indirectly by Context::getOption() call.
     */
    public function getTpl( $tplID )
    {
        $sharedTplDir = Context::getOption()->getSharedTplDir();

        switch ( $tplID ) {

            case 'actions_td':
                return "$sharedTplDir/Ajax/CacheMgrActionsCol.tpl";

            case 'status_td':
                return "$sharedTplDir/Ajax/CacheMgrStatusCol.tpl";

            case 'flag_td':
                return "$sharedTplDir/Ajax/CacheMgrFlagCol.tpl";

            //no default
        }
    }

}
