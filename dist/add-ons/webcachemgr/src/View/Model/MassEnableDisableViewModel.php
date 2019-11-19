<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\PluginVersion;
use \Lsc\Wp\WPInstallStorage;

class MassEnableDisableViewModel
{

    const FLD_ICON = 'icon';
    const FLD_ACTIVE_VER = 'activeVer';
    const FLD_STATE = 'allowCacheOp';
    const ST_INSTALLS_DISCOVERED = 2;
    const ST_NO_INSTALLS_DISCOVERED = 1;
    const ST_SCAN_NEEDED = 0;

    /**
     * @var WPInstallStorage
     */
    protected $wpInstallStorage;

    /**
     * @var mixed[]
     */
    protected $tplData = array();

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
        $this->setIconPath();
        $this->setActiveVerData();
        $this->setStateData();
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

    protected function setIconPath()
    {
        $iconPath = '';

        try
        {
            $iconDir = Context::getOption()->getIconDir();
            $iconPath = "{$iconDir}/massEnableDisableCache.svg";
        }
        catch ( LSCMException $e )
        {
            Logger::debug($e->getMessage() . ' Could not get icon directory.');
        }

        $this->tplData[self::FLD_ICON] = $iconPath;
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
                $this->tplData[self::FLD_STATE] = self::ST_NO_INSTALLS_DISCOVERED;
            }
        }
        else {
            $this->tplData[self::FLD_STATE] = self::ST_SCAN_NEEDED;
        }
    }

    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/MassEnableDisable.tpl';
    }

}
