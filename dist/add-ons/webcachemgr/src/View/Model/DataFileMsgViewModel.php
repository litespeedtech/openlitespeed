<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2025 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use Lsc\Wp\Context\Context;
use Lsc\Wp\LSCMException;
use Lsc\Wp\WPInstallStorage;

class DataFileMsgViewModel
{

    const FLD_TITLE    = 'title';
    const FLD_DISCOVER = 'discover';

    /**
     * @var WPInstallStorage
     */
    private $wpInstallStorage;

    /**
     * @var string[]
     */
    private $tplData = array();

    /**
     *
     * @param WPInstallStorage  $wpInstallStorage
     */
    public function __construct( WPInstallStorage $wpInstallStorage )
    {
        $this->wpInstallStorage = $wpInstallStorage;

        $this->init();
    }

    private function init()
    {
        $this->setTitleAndDiscover();
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

    private function setTitleAndDiscover()
    {
        switch ( $this->wpInstallStorage->getError() ) {

            case WPInstallStorage::ERR_NOT_EXIST:
                $title    = 'No Scan Data Found';
                $discover = 'discover';
                break;

            case WPInstallStorage::ERR_CORRUPTED:
                $title    = 'Scan Data Corrupted';
                $discover = 're-discover';
                break;

            default:
                $title    = 'Scan Data Needs To Be Updated';
                $discover = 're-discover';
        }

        $this->tplData[self::FLD_TITLE]    = $title;
        $this->tplData[self::FLD_DISCOVER] = $discover;
    }

    /**
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by Context::getOption() call.
     */
    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/DataFileMsg.tpl';
    }

}
