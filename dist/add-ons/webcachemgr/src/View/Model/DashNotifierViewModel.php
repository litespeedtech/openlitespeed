<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2019
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\Logger;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\WPDashMsgs;
use \Lsc\Wp\WPInstallStorage;

class DashNotifierViewModel
{

    const STEP_CONFIRM = 0;
    const STEP_DO_ACTION = 1;
    const FLD_ICON_DIR = 'iconDir';
    const FLD_DISCOVERED_COUNT = 'discoveredCount';
    const FLD_RAP_MSG_IDS = 'rapMsgIds';
    const FLD_BAM_MSG_IDS = 'bamMsgIds';

    /**
     * @var WPInstallStorage
     */
    protected $wpInstallStorage;

    /**
     * @var WPDashMsgs
     */
    protected $wpDashMsgs;

    /**
     * @var mixed[]
     */
    protected $tplData = array();

    /**
     *
     * @param WPInstallStorage  $wpInstallStorage
     * @param WPDashMsgs        $wpDashMsgs
     */
    public function __construct( WPInstallStorage $wpInstallStorage,
            WPDashMsgs $wpDashMsgs )
    {
        $this->wpInstallStorage = $wpInstallStorage;
        $this->wpDashMsgs = $wpDashMsgs;

        $this->init();
    }

    protected function init()
    {
        $this->setIconDir();
        $this->setStoredMsgIds();
        $this->setDiscoveredCount();
    }

    /**
     *
     * @param string  $field
     * @return null|string
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

    protected function setStoredMsgIds()
    {
        $msgs = $this->wpDashMsgs->getMsgData();

        $this->tplData[self::FLD_RAP_MSG_IDS] =
                array_keys($msgs[WPDashMsgs::KEY_RAP_MSGS]);
        $this->tplData[self::FLD_BAM_MSG_IDS] =
                array_keys($msgs[WPDashMsgs::KEY_BAM_MSGS]);
    }

    protected function setDiscoveredCount()
    {
        $this->tplData[self::FLD_DISCOVERED_COUNT] =
                $this->wpInstallStorage->getCount();
    }

    /**
     *
     * @return string
     */
    public function getTpl()
    {
        return realpath(__DIR__ . '/../Tpl') . '/DashNotifier.tpl';
    }

}
