<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2020
 * ******************************************* */

namespace Lsc\Wp\Panel;

use Lsc\Wp\WPInstall;

class CustomPanel
extends CustomPanelBase
{
    protected function __construct()
    {
        /**
         * Panel name can be set to whatever you'd like.
         */
        $this->panelName = 'customPanel';

        /** @noinspection PhpUnhandledExceptionInspection */
        parent::__construct();
    }

    /**
     *
     * @since 1.13.2
     */
    protected function init2()
    {
        $this->panelName = 'customPanel';

        /** @noinspection PhpUnhandledExceptionInspection */
        parent::init2();
    }

    /**
     * Gets a list of found docroots and associated server names.
     * Only needed for lscmctl 'scan' command.
     */
    protected function prepareDocrootMap()
    {
        /**
         * This function can be left as is if you do not intend to use
         * the lscmctl 'scan' command. In this case lscmctl command
         * 'addinstalls' can be used to add WordPress installations to the
         * custom data file instead.
         *
         * If you would like to add support for the lscmctl 'scan' command,
         * implement this function so that it searches for all document root,
         * server name, and server alias groups and uses this information to
         * populate $this->docRootMap as follows:
         *
         * array(
         *     'docroots' => array(index => docroot),
         *     'names' => array("server name/alias" => index)
         * );
         *
         * Where the value of each discovered servername/alias in the 'names'
         * array matches the index of the related document root in the
         * 'docroots' array.
         */
        $this->docRootMap = array('docroots' => array(), 'names' => array());
    }

    /**
     * This function returns the PHP binary to be used when performing
     * WordPress related actions for the WordPress installation associated with
     * the passed in WPInstall object.
     *
     * @param WPInstall  $wpInstall
     * @return string
     */
    public function getPhpBinary( WPInstall $wpInstall )
    {
        /**
         * If PHP binary $phpBin can be more accurately detected for the given
         * installation, do so here.
         */
        $phpBin = 'php';

        return "{$phpBin} {$this->phpOptions}";
    }

}
