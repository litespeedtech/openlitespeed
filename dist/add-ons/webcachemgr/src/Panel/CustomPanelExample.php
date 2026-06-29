<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright 2026 LiteSpeed Technologies, Inc.
 * @since 1.17.8
 * ******************************************* */

namespace Lsc\Wp\Panel;

use Lsc\Wp\WPInstall;

/**
 * Example CustomPanel implementation for third-party integrators.
 *
 * To use this template:
 * 1. Copy this file to $lsws_home/admin/lscdata/custom/CustomPanel.php
 * 2. Rename both the file and the class: file must be CustomPanel.php and
 *    the class declaration must read "class CustomPanel"
 * 3. Customize the method implementations for your panel
 *
 * The framework expects the deployed file to declare class
 * \Lsc\Wp\Panel\CustomPanel extending \Lsc\Wp\Panel\CustomPanelBase.
 *
 * SECURITY CONTRACT FOR INTEGRATORS
 * -----------------------------------
 * Before constructing \Lsc\Wp\PanelController or calling any of its
 * manage*Operations*() methods, your HTTP handler MUST:
 *
 *   1. Verify that the incoming request is from an authenticated admin session.
 *   2. Verify a per-session or per-request anti-CSRF token to prevent
 *      cross-site request forgery.
 *
 * The library itself does not perform either check; they belong to the outer
 * panel/framework layer.  Omitting them exposes destructive operations
 * (cache enable/disable, plugin file removal, cache-root deletion) to
 * unauthenticated or CSRF-driven requests.
 *
 * @see \Lsc\Wp\Panel\CustomPanelBase
 * @see \Lsc\Wp\Panel\ControlPanel::initByClassName()
 * @see \Lsc\Wp\PanelController::__construct()
 * @since 1.17.8
 */
class CustomPanelExample
extends CustomPanelBase
{
    /**
     *
     * @since 1.17.8
     */
    protected function __construct()
    {
        /** @noinspection PhpUnhandledExceptionInspection */
        parent::__construct();
    }

    /**
     *
     * @since 1.17.8
     */
    protected function init2()
    {
        /**
         * Panel name can be set to whatever you'd like.
         */
        $this->panelName = 'customPanel';

        /**
         * Optionally set a default server-level cache root directory.
         * This is used when no explicit cache root has been configured.
         * Example: $this->defaultSvrCacheRoot = '/var/lscache/';
         */

        /** @noinspection PhpUnhandledExceptionInspection */
        parent::init2();
    }

    /**
     * Gets a list of found docroots and associated server names.
     * Only needed for lscmctl 'scan' command.
     *
     * @since 1.17.8
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
     * Returns the PHP binary split into a path token and an options string.
     *
     * Override this method to detect the correct PHP binary for the given
     * WordPress installation. The $wpInstall object provides context such as
     * the install's document root and owner information, e.g.:
     *   $ownerInfo = $wpInstall->getOwnerInfo();
     *   $user = $ownerInfo['user_name'];
     *
     * SECURITY NOTE
     * -----------------------------------------------------------------
     * The framework will escapeshellarg() the binPath token but will
     * interpolate the optionsString RAW into the inner shell command.
     * Treat optionsString as a shell-active string:
     *   - Only include flags you have composed in source code, or that
     *     come from $this->phpOptions (library-controlled).
     *   - If any options-fragment value originates from configuration or
     *     external data, escapeshellarg() that value before appending it.
     *
     * @since 1.17.10
     *
     * @param WPInstall $wpInstall
     *
     * @return PhpBinaryParts
     */
    public function getPhpBinaryParts( WPInstall $wpInstall )
    {
        $phpBin = 'php';

        return new PhpBinaryParts($phpBin, $this->phpOptions);
    }

    /**
     * @deprecated since 1.17.10  Override getPhpBinaryParts() instead.
     *
     * @since 1.17.8
     *
     * @param WPInstall $wpInstall
     *
     * @return string
     */
    public function getPhpBinary( WPInstall $wpInstall )
    {
        $parts   = $this->getPhpBinaryParts($wpInstall);
        $options = $parts->getOptionsString();

        return $options === ''
            ? $parts->getBinPath()
            : $parts->getBinPath() . ' ' . $options;
    }

}
