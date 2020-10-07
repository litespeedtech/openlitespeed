<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2020
 * @since 1.13.3
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Logger;

/**
 *
 * @since 1.13.3
 */
class ScanProgressStepViewModel
{

    /**
     * @since 1.13.3
     * @var string
     */
    const FLD_TITLE = 'title';

    /**
     * @since 1.13.3
     * @var string
     */
    const FLD_ICON = 'icon';

    /**
     * @since 1.13.3
     * @var string
     */
    const FLD_MGR_STEP = 'mgrStep';

    /**
     * @since 1.13.3
     * @var string
     */
    const FLD_TOTAL_COUNT = 'totalCount';

    /**
     * @since 1.13.3
     * @var string
     */
    const FLD_INSTALLS_COUNT = 'installsCount';

    /**
     * @since 1.13.3
     * @var int
     */
    const OP_SCAN = 1;

    /**
     * @since 1.13.3
     * @var int
     */
    protected $mgrStep;

    /**
     * @since 1.13.3
     * @var (int|string)[]
     */
    protected $tplData = array();

    /**
     *
     * @since 1.13.3
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
     * @since 1.13.3
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
     * @since 1.13.3
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

    /**
     *
     * @since 1.13.3
     */
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
     * @since 1.13.3
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

    /**
     *
     * @since 1.13.3
     */
    protected function grabSessionData()
    {
        $info = $_SESSION['scanInfo'];

        if ( !empty($info['homeDirs']) ) {
            $total = count($info['homeDirs']);
        }
        else {
            $total = count($info['installs']);
        }

        $this->tplData[self::FLD_TOTAL_COUNT] = $total;
    }

    /**
     *
     * @since 1.13.3
     *
     * @return string
     * @throws LSCMException  Thrown indirectly.
     */
    public function getTpl()
    {
        $info = $_SESSION['scanInfo'];

        if ( !empty($info['homeDirs']) ) {
            return Context::getOption()->getSharedTplDir()
                . '/ScanProgressStep1.tpl';
        }
        else {
            return Context::getOption()->getSharedTplDir()
                . '/ScanProgressStep2.tpl';
        }
    }

}
