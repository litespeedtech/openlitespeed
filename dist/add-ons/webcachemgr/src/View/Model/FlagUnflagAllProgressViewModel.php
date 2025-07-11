<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2025 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use Lsc\Wp\Context\Context;
use Lsc\Wp\Logger;
use Lsc\Wp\LSCMException;

class FlagUnflagAllProgressViewModel
{

    const FLD_ACTION         = 'action';
    const FLD_ICON           = 'icon';
    const FLD_INSTALLS_COUNT = 'installsCount';

    /**
     * @var string  Should be either 'flag' or 'unflag'.
     */
    protected $action = '';

    /**
     * @var (int|string)[]
     */
    protected $tplData = [];

    /**
     *
     * @param string $action  Should be 'flag' or 'unflag'.
     *
     * @throws LSCMException  Thrown when $action value is unrecognized.
     * @throws LSCMException  Thrown indirectly by $this->init() call.
     */
    public function __construct( $action )
    {
        switch ( $action ) {

            case 'flag':
            case 'unflag':
                $this->action = $action;
                break;

            default:
                throw new LSCMException(
                    'Unrecognized $action value passed to FlagUnflagAllProgressViewModel constructor.'
                );
        }

        $this->init();
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->setIconPath() call.
     */
    protected function init()
    {
        $this->setIconPath();
        $this->setAction();
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

    protected function setAction()
    {
        $this->tplData[self::FLD_ACTION] = $this->action;
    }

    protected function grabSessionData()
    {
        $this->tplData[self::FLD_INSTALLS_COUNT] =
            count($_SESSION['mass_' . $this->action . '_info']['installs']);
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
            . '/FlagUnflagAllProgress.tpl';
    }

}
