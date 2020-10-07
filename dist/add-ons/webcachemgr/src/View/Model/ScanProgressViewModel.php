<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2020
 * @deprecated 1.13.3  This file will be removed in a future release.
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Logger;

/**
 *
 * @deprecated 1.13.3
 */
class ScanProgressViewModel
{

    const FLD_TITLE = 'title';
    const FLD_ICON = 'icon';
    const FLD_MGR_STEP = 'mgrStep';
    const FLD_HOME_DIR_COUNT = 'homeDirCount';
    const OP_SCAN = 1;
    const OP_DISCOVER_NEW = 2;

    /**
     * @var int
     */
    protected $mgrStep;

    /**
     * @var (int|string)[]
     */
    protected $tplData = array();

    /**
     *
     * @param int  $mgrStep
     * @throws LSCMException  Thrown indirectly.
     */
    public function __construct( $mgrStep )
    {
        $this->mgrStep = $this->tplData[self::FLD_MGR_STEP] = $mgrStep;

        $this->init();
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly.
     */
    protected function init()
    {
        $this->setTitle();
        $this->setIconPath();
        $this->grabSessionData();
    }

    /**
     *
     * @param string  $field
     * @return null|int|string
     */
    public function getTplData( $field )
    {
        if ( !isset($this->tplData[$field]) ) {
            return null;
        }

        return $this->tplData[$field];
    }

    protected function setTitle()
    {

        if ( $this->mgrStep == self::OP_SCAN ) {
            $title = 'Scanning/Re-scanning For All WordPress Installations...';
        }
        else {
            $title = 'Discovering New WordPress Installations...';
        }

        $this->tplData[self::FLD_TITLE] = $title;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly.
     */
    protected function setIconPath()
    {
        $iconPath = '';

        try
        {
            $iconDir = Context::getOption()->getIconDir();
            $iconPath = "{$iconDir}/manageCacheInstallations.svg";
        }
        catch ( LSCMException $e )
        {
            Logger::debug($e->getMessage() . ' Could not get icon directory.');
        }

        $this->tplData[self::FLD_ICON] = $iconPath;
    }

    protected function grabSessionData()
    {
        $info = $_SESSION['scanInfo'];

        $this->tplData[self::FLD_HOME_DIR_COUNT] = count($info['homeDirs']);
    }

    /**
     *
     * @return string
     * @throws LSCMException  Thrown indirectly.
     */
    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/ScanProgress.tpl';
    }

}
