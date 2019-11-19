<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2019
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Logger;

class MassDashDisableProgressViewModel
{

    const FLD_ICON = 'icon';
    const FLD_INSTALLS_COUNT = 'installsCount';

    /**
     * @var string
     */
    protected $sessionKey = 'massDashDisableInfo';

    /**
     * @var mixed[]
     */
    protected $tplData = array();

    /**
     *
     * @param string  $action
     */
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
            $iconPath = "{$iconDir}/wpNotifier.svg";
        }
        catch ( LSCMException $e )
        {
            Logger::debug($e->getMessage() . ' Could not get icon directory.');
        }

        $this->tplData[self::FLD_ICON] = $iconPath;
    }

    protected function grabSessionData()
    {
        $info = $_SESSION[$this->sessionKey];

        $this->tplData[self::FLD_INSTALLS_COUNT] = count($info['installs']);
    }

    /**
     *
     * @return string
     */
    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir()
                . '/MassDashDisableProgress.tpl';
    }

}
