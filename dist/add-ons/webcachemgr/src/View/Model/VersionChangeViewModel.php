<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Logger;

class VersionChangeViewModel
{

    const FLD_ICON = 'icon';
    const FLD_INSTALLS_COUNT = 'installsCount';
    const FLD_VER_NUM = 'verNum';

    /**
     * @var (int|string)[]
     */
    protected $tplData = array();

    public function __construct()
    {
        $this->init();
    }

    protected function init()
    {
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

    protected function setIconPath()
    {
        $iconPath = '';

        try
        {
            $iconDir = Context::getOption()->getIconDir();
            $iconPath = "{$iconDir}/lscwpCurrentVersion.svg";
        }
        catch ( LSCMException $e )
        {
            Logger::debug($e->getMessage() . ' Could not get icon directory.');
        }

        $this->tplData[self::FLD_ICON] = $iconPath;
    }

    protected function grabSessionData()
    {
        $info = $_SESSION['verInfo'];

        $this->tplData[self::FLD_INSTALLS_COUNT] = count($info['installs']);
        $this->tplData[self::FLD_VER_NUM] = $info['verNum'];
    }

    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/VersionChange.tpl';
    }

}
