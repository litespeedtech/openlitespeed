<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author Michael Alegre
 * @copyright (c) 2018-2022 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use Lsc\Wp\WPInstallStorage;
use Lsc\Wp\Context\Context;
use Lsc\Wp\PluginVersion;
use Lsc\Wp\Logger;
use Lsc\Wp\LSCMException;

class VersionManageViewModel
{

    /**
     * @var string
     */
    const FLD_ICON = 'icon';

    /**
     * @var string
     */
    const FLD_VERSION_LIST = 'versionList';

    /**
     * @var string
     */
    const FLD_ALLOWED_VER_LIST = 'allowedList';

    /**
     * @var string
     */
    const FLD_ACTIVE_VER = 'activeVer';

    /**
     * @var string
     */
    const FLD_ERR_MSGS = 'errMsgs';

    /**
     * @var string
     */
    const FLD_STATE = 'state';

    /**
     * @var int
     */
    const ST_INSTALLS_DISCOVERED = 2;

    /**
     * @deprecated 1.13.4.1  Added back as a deprecated constant after
     *     accidental removal in v1.13.4. Use
     *     self::ST_NO_NON_ERROR_INSTALLS_DISCOVERED instead.
     *
     * @var int
     */
    const ST_NO_INSTALLS_DISCOVERED = 1;

    /**
     * @var int
     */
    const ST_NO_NON_ERROR_INSTALLS_DISCOVERED = 1;

    /**
     * @var int
     */
    const ST_SCAN_NEEDED = 0;

    /**
     * @var WPInstallStorage
     */
    protected $wpInstallStorage;

    /**
     * @var (boolean|string|string[])[]
     */
    protected $tplData = array();

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
     * @throws LSCMException  Thrown indirectly by $this->setIconPath() call.
     * @throws LSCMException  Thrown indirectly by $this->setActiveVerData()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->setVerListData() call.
     * @throws LSCMException  Thrown indirectly by $this->setMsgData() call.
     */
    protected function init()
    {
        $this->setIconPath();
        $this->setActiveVerData();
        $this->setVerListData();
        $this->setStateData();
        $this->setMsgData();
    }

    /**
     *
     * @param string $field
     *
     * @return null|boolean|string|string[]
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
    protected function setIconPath()
    {
        $iconPath = '';

        try {
            $iconDir = Context::getOption()->getIconDir();
            $iconPath = "$iconDir/lscwpCurrentVersion.svg";
        }
        catch ( LSCMException $e ) {
            Logger::debug($e->getMessage() . ' Could not get icon directory.');
        }

        $this->tplData[self::FLD_ICON] = $iconPath;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    protected function setActiveVerData()
    {
        try {
            $currVer = PluginVersion::getCurrentVersion();
        }
        catch ( LSCMException $e ) {
            Logger::debug(
                $e->getMessage() . ' Could not get active LSCWP version.'
            );

            $currVer = false;
        }

        $this->tplData[self::FLD_ACTIVE_VER] = $currVer;
    }

    protected function setStateData()
    {
        if ( $this->wpInstallStorage->getError() == 0 ) {

            if ( $this->wpInstallStorage->getCount(true) > 0 ) {
                $this->tplData[self::FLD_STATE] = self::ST_INSTALLS_DISCOVERED;
            }
            else {
                $this->tplData[self::FLD_STATE] =
                    self::ST_NO_NON_ERROR_INSTALLS_DISCOVERED;
            }
        }
        else {
            $this->tplData[self::FLD_STATE] = self::ST_SCAN_NEEDED;
        }
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by PluginVersion::getInstance()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    protected function setVerListData()
    {
        $vermgr = PluginVersion::getInstance();

        try
        {
            $verList = $vermgr->getShortVersions();
            $allowedList = $vermgr->getAllowedVersions();
        }
        catch ( LSCMException $e )
        {
            Logger::debug(
                $e->getMessage() . ' Could not retrieve version list.'
            );

            $verList = $allowedList = array();
        }

        $this->tplData[self::FLD_VERSION_LIST] = $verList;
        $this->tplData[self::FLD_ALLOWED_VER_LIST] = $allowedList;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Logger::getUiMsgs() call.
     */
    protected function setMsgData()
    {
        $this->tplData[self::FLD_ERR_MSGS] = Logger::getUiMsgs(Logger::UI_ERR);
    }

    /**
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by Context::getOption() call.
     */
    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/VersionManage.tpl';
    }

}
