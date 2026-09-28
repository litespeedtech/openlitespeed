<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2026 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use Lsc\Wp\Context\Context;
use Lsc\Wp\Logger;
use Lsc\Wp\LSCMException;
use Lsc\Wp\Util;

class UnflagAllProgressViewModel
{

    const FLD_ICON           = 'icon';
    const FLD_INSTALLS_COUNT = 'installsCount';

    /**
     * @var (int|string)[]
     */
    protected $tplData = [];

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
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    protected function setIconPath()
    {
        $iconPath = '';

        try
        {
            $iconPath = Context::getOption()->getIconDir()
                . '/manageCacheInstallations.svg';
        }
        catch ( LSCMException $e )
        {
            Logger::debug("{$e->getMessage()}. Could not get icon directory.");
        }

        $this->tplData[self::FLD_ICON] = $iconPath;
    }

    protected function grabSessionData()
    {
        $this->tplData[self::FLD_INSTALLS_COUNT] =
            count($_SESSION[Util::sessionKey('unflagInfo')]['installs']);
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
            . '/UnflagAllProgress.tpl';
    }

}
