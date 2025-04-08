<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2025 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use Lsc\Wp\Context\Context;
use Lsc\Wp\PluginVersion;
use Lsc\Wp\LSCMException;
use Lsc\Wp\Logger;

class MassEnableDisableProgressViewModel
{

    const FLD_ICON           = 'icon';
    const FLD_ACTION         = 'action';
    const FLD_INSTALLS_COUNT = 'installsCount';
    const FLD_ACTIVE_VER     = 'activeVer';

    /**
     * @var string
     */
    protected $action;

    /**
     * @var string
     */
    protected $sessionKey;

    /**
     * @var array
     */
    protected $tplData = array();

    /**
     *
     * @param string $action
     *
     * @throws LSCMException  Thrown indirectly by $this->init() call.
     */
    public function __construct( $action )
    {
        $this->action     = $action;
        $this->sessionKey = 'mass' . ucfirst($this->action) . 'Info';

        $this->init();
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->setIconPath().
     * @throws LSCMException  Thrown indirectly by $this->setActiveVerData()
     *     call.
     */
    protected function init()
    {
        $this->setIconPath();

        $this->tplData[self::FLD_ACTION] = $this->action;

        $this->grabSessionData();
        $this->setActiveVerData();
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
            $iconPath = Context::getOption()->getIconDir()
                . '/massEnableDisableCache.svg';
        }
        catch ( LSCMException $e )
        {
            Logger::debug("{$e->getMessage()} Could not get icon directory.");
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
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    protected function setActiveVerData()
    {
        try {
            $activeVer = PluginVersion::getCurrentVersion();
        }
        catch ( LSCMException $e ) {
            Logger::debug(
                "{$e->getMessage()} Could not get active LSCWP version."
            );

            $activeVer = false;

            /**
             * Unset session data early.
             */
            if ( $this->tplData[self::FLD_ACTION] == 'enable' ) {
                unset($_SESSION[$this->sessionKey]);
            }
        }

        $this->tplData[self::FLD_ACTIVE_VER] = $activeVer;
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
            . '/MassEnableDisableProgress.tpl';
    }

}
