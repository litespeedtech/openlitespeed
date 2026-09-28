<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2019-2026 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use Lsc\Wp\Context\Context;
use Lsc\Wp\LSCMException;
use Lsc\Wp\Logger;
use Lsc\Wp\Util;

class MassDashNotifyProgressViewModel
{

    const FLD_ICON           = 'icon';
    const FLD_INSTALLS_COUNT = 'installsCount';
    const FLD_ACTIVE_VER     = 'activeVer';

    /**
     * Un-prefixed session key name. Pass it through Util::sessionKey() to get
     * the actual $_SESSION key.
     *
     * @var string
     */
    protected $sessionKey = 'massDashNotifyInfo';

    /**
     * @var array
     */
    protected $tplData = array();

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->init() call.
     */
    public function __construct()
    {
        $this->init();
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->setIconPath() call.
     */
    protected function init()
    {
        $this->setIconPath();
        $this->grabSessionData();
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
    protected function setIconPath()
    {
        $iconPath = '';

        try
        {
            $iconPath = Context::getOption()->getIconDir() . '/wpNotifier.svg';
        }
        catch ( LSCMException $e )
        {
            Logger::debug("{$e->getMessage()} Could not get icon directory.");
        }

        $this->tplData[self::FLD_ICON] = $iconPath;
    }

    protected function grabSessionData()
    {
        $info = $_SESSION[Util::sessionKey($this->sessionKey)];

        $this->tplData[self::FLD_INSTALLS_COUNT] = count($info['installs']);
    }

    /**
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by Context::getOption() call.
     */
    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir()
            . '/MassDashNotifyProgress.tpl';
    }

}
