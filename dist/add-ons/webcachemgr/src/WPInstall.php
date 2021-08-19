<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2020
 * *******************************************
 */

namespace Lsc\Wp;

use \Lsc\Wp\Panel\ControlPanel;
use \Lsc\Wp\Context\Context;

class WPInstall
{

    /**
     * @var int
     */
    const ST_PLUGIN_ACTIVE = 1;

    /**
     * @var int
     */
    const ST_PLUGIN_INACTIVE = 2;

    /**
     * @var int
     */
    const ST_LSC_ADVCACHE_DEFINED = 4;

    /**
     * @var int
     */
    const ST_FLAGGED = 8;

    /**
     * @var int
     */
    const ST_ERR_SITEURL = 16;

    /**
     * @var int
     */
    const ST_ERR_DOCROOT = 32;

    /**
     * @var int
     */
    const ST_ERR_EXECMD = 64;

    /**
     * @var int
     */
    const ST_ERR_TIMEOUT = 128;

    /**
     * @var int
     */
    const ST_ERR_EXECMD_DB = 256;

    /**
     * @var int
     */
    const ST_ERR_WPCONFIG = 1024;

    /**
     * @var int
     */
    const ST_ERR_REMOVE = 2048;

    /**
     * @var string
     */
    const FLD_STATUS = 'status';

    /**
     * @var string
     */
    const FLD_DOCROOT = 'docroot';

    /**
     * @var string
     */
    const FLD_SERVERNAME = 'server_name';

    /**
     * @var string
     */
    const FLD_SITEURL = 'site_url';

    /**
     * @var string
     */
    const FLAG_FILE = '.litespeed_flag';

    /**
     * @var string
     */
    const FLAG_NEW_LSCWP = '.lscm_new_lscwp';

    /**
     * @var string
     */
    protected $path;

    /**
     * @var mixed[]
     */
    protected $data;

    /**
     * @var null|string
     */
    protected $phpBinary = null;

    /**
     * @var null|string
     */
    protected $suCmd = null;

    /**
     * @var null|mixed[]  Keys are 'user_id', 'user_name', 'group_id'
     */
    protected $ownerInfo = null;

    /**
     * @var bool
     */
    protected $changed = false;

    /**
     * @var bool
     */
    protected $refreshed = false;

    /**
     * @var null|string
     */
    protected $wpConfigFile = '';

    /**
     * @var int
     */
    protected $cmdStatus = 0;

    /**
     * @var string
     */
    protected $cmdMsg = '';

    /**
     *
     * @param string  $path
     */
    public function __construct( $path )
    {
        $this->init($path);
    }

    /**
     *
     * @param string  $path
     */
    protected function init( $path )
    {
        if ( ($realPath = realpath($path)) === false ) {
            $this->path = $path;
        }
        else {
            $this->path = $realPath;
        }

        $this->data = array(
            self::FLD_STATUS => 0,
            self::FLD_DOCROOT => null,
            self::FLD_SERVERNAME => null,
            self::FLD_SITEURL => null
        );
    }

    /**
     *
     * @return string
     */
    public function __toString()
    {
        return sprintf(
            "%s (status=%d docroot=%s siteurl=%s)",
            $this->path,
            $this->data[self::FLD_STATUS],
            $this->data[self::FLD_DOCROOT],
            $this->data[self::FLD_SITEURL]
        );
    }

    /**
     *
     * @return string|null
     */
    public function getDocRoot()
    {
        return $this->getData(self::FLD_DOCROOT);
    }

    /**
     *
     * @param string    $docRoot
     * @return boolean
     */
    public function setDocRoot( $docRoot )
    {
        return $this->setData(self::FLD_DOCROOT, $docRoot);
    }

    /**
     *
     * @return string|null
     */
    public function getServerName()
    {
        return $this->getData(self::FLD_SERVERNAME);
    }

    /**
     *
     * @param string  $serverName
     * @return boolean
     */
    public function setServerName( $serverName )
    {
        return $this->setData(self::FLD_SERVERNAME, $serverName);
    }

    /**
     * Note: Temporary function name until existing deprecated setSiteUrl()
     * function is removed.
     *
     * @param string  $siteUrl
     * @return boolean
     */
    public function setSiteUrlDirect( $siteUrl )
    {
        return $this->setData(self::FLD_SITEURL, $siteUrl);
    }

    /**
     *
     * @param int  $status
     * @return boolean
     */
    public function setStatus( $status )
    {
        return $this->setData(self::FLD_STATUS, $status);
    }

    /**
     *
     * @return int
     */
    public function getStatus()
    {
        return $this->getData(self::FLD_STATUS);
    }

    /**
     *
     * @param string  $field
     * @return mixed|null
     */
    public function getData( $field = '' )
    {
        if ( !$field ) {
            return $this->data;
        }

        if ( isset($this->data[$field]) ) {
            return $this->data[$field];
        }

        /**
         * Error out
         */
        return null;
    }

    /**
     *
     * @param string  $field
     * @param mixed   $value
     * @return boolean
     */
    protected function setData( $field, $value )
    {
        $updated = false;

        if ( $this->data[$field] !== $value ) {
            $this->changed = $updated = true;
            $this->data[$field] = $value;
        }

        return $updated;
    }

    /**
     * Calling from unserialized data.
     *
     * @param mixed[]  $data
     */
    public function initData( $data )
    {
        $this->data = $data;
    }

    /**
     *
     * @return string
     */
    public function getPath()
    {
        return $this->path;
    }

    /**
     *
     * @return boolean
     */
    public function shouldRemove()
    {
        return ($this->getStatus() & self::ST_ERR_REMOVE) ? true : false;
    }

    /**
     *
     * @return boolean
     */
    public function hasFlagFile()
    {
        return file_exists("{$this->path}/" . self::FLAG_FILE);
    }

    public function hasNewLscwpFlagFile()
    {
        return file_exists("{$this->path}/" . self::FLAG_NEW_LSCWP);
    }

    /**
     *
     * @return boolean
     * @throws LSCMException  Thrown indirectly.
     */
    public function hasValidPath()
    {
        if ( !is_dir($this->path) || !is_dir("{$this->path}/wp-admin") ) {
            $this->setStatusBit(self::ST_ERR_REMOVE);

            $msg = "{$this->path} - Could not be found and has been removed "
                . 'from Cache Manager list.';
            Logger::uiError($msg);
            Logger::notice($msg);
        }
        elseif ( $this->getWpConfigFile() == null ) {
            $this->setStatusBit(self::ST_ERR_WPCONFIG);
            $this->addUserFlagFile(false);

            $msg = "{$this->path} - Could not find a valid wp-config.php file. "
                . 'Install has been flagged.';
            Logger::uiError($msg);
            Logger::error($msg);
        }
        else {
            return true;
        }

        return false;
    }

    /**
     * Set the provided status bit.
     *
     * @param int  $bit
     */
    public function setStatusBit( $bit )
    {
        $status = $this->getStatus();
        $status |= $bit;
        $this->setStatus($status);
    }

    /**
     * Unset the provided status bit.
     *
     * @param int  $bit
     */
    public function unsetStatusBit( $bit )
    {
        $status = $this->getStatus();
        $status &= ~$bit;
        $this->setStatus($status);
    }

    /**
     *
     * @deprecated 1.9.5  Deprecated to avoid confusion with $this->cmdStatus
     *                    and $this->cmdMsg related functions. Use
     *                    $this->setStatus() instead.
     * @param int  $newStatus
     */
    public function updateCommandStatus( $newStatus )
    {
        $this->setData(self::FLD_STATUS, $newStatus);
    }

    /**
     *
     * @return null|string
     */
    public function getWpConfigFile()
    {
        if ( $this->wpConfigFile === '' ) {
            $file = "{$this->path}/wp-config.php";

            if ( !file_exists($file) ) {
                /**
                 *  check parent dir
                 */
                $parentDir = dirname($this->path);
                $file = "{$parentDir}/wp-config.php";

                if ( !file_exists($file)
                        || file_exists("{$parentDir}/wp-settings.php") ) {

                    /**
                     * If wp-config moved up, in same dir should NOT have
                     * wp-settings
                     */
                    $file = null;
                }
            }

            $this->wpConfigFile = $file;
        }

        return $this->wpConfigFile;
    }

    /**
     * Takes a WordPress site URL and uses it to populate serverName, siteUrl,
     * and docRoot data. If a matching docRoot cannot be found using the
     * serverName, the install will be flagged and an ST_ERR_DOCROOT status set.
     *
     * @param string $url
     *
     * @return boolean
     * @throws LSCMException  Thrown indirectly.
     */
    public function populateDataFromUrl( $url )
    {
        /** @noinspection HttpUrlsUsage */
        $parseSafeUrl =
            (preg_match('#^https?://#', $url)) ? $url : "http://$url";

        $info = parse_url($parseSafeUrl);

        $serverName = strtolower($info['host']);
        $this->setData(self::FLD_SERVERNAME, $serverName);

        $siteUrlTrim = $info['host'];

        if ( isset($info['path']) ) {
            $siteUrlTrim .= $info['path'];
        }

        $this->setData(self::FLD_SITEURL, $siteUrlTrim);

        $docRoot = ControlPanel::getClassInstance()->mapDocRoot($serverName);
        $this->setData(self::FLD_DOCROOT, $docRoot);

        if ( $docRoot === null ) {
            $this->setStatus(self::ST_ERR_DOCROOT);
            $this->addUserFlagFile(false);

            $msg = "{$this->path} - Could not find matching document root for "
                . "WP siteurl/servername {$serverName}.";

            $this->setCmdStatusAndMsg(UserCommand::EXIT_ERROR, $msg);
            Logger::error($msg);
            return false;
        }

        return true;
    }

    /**
     * Deprecated 06/18/19. Renamed to populateDataFromUrl().
     *
     * @deprecated
     * @param string  $siteUrl
     * @return boolean
     * @throws LSCMException  Thrown indirectly.
     */
    public function setSiteUrl( $siteUrl )
    {
        return $this->populateDataFromUrl($siteUrl);
    }

    /**
     * Adds the flag file to an installation.
     *
     * @param boolean  $runningAsUser
     * @return boolean  True when install has a flag file created/already.
     * @throws LSCMException  Thrown indirectly.
     */
    public function addUserFlagFile( $runningAsUser = true )
    {
        $file = "{$this->path}/" . self::FLAG_FILE;

        if ( !file_exists($file) ) {
            $content = Context::getFlagFileContent();

            if ( !file_put_contents($file, $content) ) {
                return false;
            }
        }

        if ( !$runningAsUser ) {
            $ownerInfo = $this->getOwnerInfo();

            chown($file, $ownerInfo['user_id']);
            chgrp($file, $ownerInfo['group_id']);
        }

        $this->setStatusBit(self::ST_FLAGGED);
        return true;
    }

    /**
     *
     * @return boolean
     */
    public function removeFlagFile()
    {
        $file = "{$this->path}/" . self::FLAG_FILE;

        if ( file_exists($file) ) {

            if ( !unlink($file) ) {
                return false;
            }
        }

        $this->unsetStatusBit(self::ST_FLAGGED);
        return true;
    }

    /**
     * Add a flag file to indicate that a new LSCWP plugin was added to this
     * installation.
     *
     * This function should only be called by the installation owner to avoid
     * permission problems involving this file.
     *
     * @return boolean
     */
    public function addNewLscwpFlagFile()
    {
        $file = "{$this->path}/" . self::FLAG_NEW_LSCWP;

        if ( !file_exists($file) ) {

            if ( !file_put_contents($file, '') ) {
                return false;
            }
        }

        return true;
    }

    /**
     * Remove "In Progress" flag file to indicate that a WPInstall action has
     * been completed.
     *
     * @return boolean
     */
    public function removeNewLscwpFlagFile()
    {
        $file = "{$this->path}/" . self::FLAG_NEW_LSCWP;

        if ( file_exists($file) ) {

            if ( !unlink($file) ) {
                return false;
            }
        }

        return true;
    }

    /**
     *
     * @param boolean  $forced
     * @return int
     * @throws LSCMException  Thrown indirectly.
     */
    public function refreshStatus( $forced = false )
    {
        if ( !$this->refreshed || $forced ) {
            UserCommand::issue(UserCommand::CMD_STATUS, $this);
            $this->refreshed = true;
        }

        return $this->getData(self::FLD_STATUS);
    }

    /**
     *
     * @param string  $pluginDir
     * @throws LSCMException  Thrown indirectly.
     */
    public function removePluginFiles( $pluginDir )
    {
        if ( file_exists(dirname($pluginDir)) && file_exists($pluginDir) ) {
            exec("rm -rf {$pluginDir}");

            $msg = "{$this->path} - Removed LSCache for WordPress plugin files "
                . 'from plugins directory';
            Logger::debug($msg);
        }
    }

    public function isFlagBitSet()
    {
        $status = $this->getStatus();

        if ( $status & self::ST_FLAGGED ) {
            return true;
        }

        return false;
    }

    /**
     *
     * @param null|int  $status
     * @return bool
     */
    public function hasFatalError( $status = null )
    {
        if ( $status === null ) {
            $status = $this->getData(self::FLD_STATUS);
        }

        $errMask = (self::ST_ERR_EXECMD
            | self::ST_ERR_EXECMD_DB
            | self::ST_ERR_TIMEOUT
            | self::ST_ERR_SITEURL
            | self::ST_ERR_DOCROOT
            | self::ST_ERR_WPCONFIG
        );

        return (($status & $errMask) > 0);
    }

    /**
     *
     * @return string
     * @throws LSCMException  Thrown indirectly.
     */
    public function getPhpBinary()
    {
        if ( $this->phpBinary == null ) {
            $this->phpBinary =
                    ControlPanel::getClassInstance()->getPhpBinary($this);
        }

        return $this->phpBinary;
    }

    /**
     * Returns requested owner info.
     *
     * @param string  $field  Key ('user_id', 'user_name', or 'group_id') in
     *                         the $ownerInfo array.
     * @return mixed[]|int|string|null
     */
    public function getOwnerInfo( $field = '' )
    {
        if ( $this->ownerInfo == null ) {
            $this->ownerInfo = Util::populateOwnerInfo($this->path);
        }

        if ( $field == '' ) {
            return $this->ownerInfo;
        }
        elseif ( isset($this->ownerInfo[$field]) ) {
            return $this->ownerInfo[$field];
        }

        return null;
    }

    /**
     *
     * @return string
     */
    public function getSuCmd()
    {
        if ( $this->suCmd == null ) {
            $owner = $this->getOwnerInfo('user_name');
            $this->suCmd = "su {$owner} -s /bin/bash";
        }

        return $this->suCmd;
    }

    /**
     *
     * @return int
     */
    public function getCmdStatus()
    {
        return $this->cmdStatus;
    }

    /**
     *
     * @return string
     */
    public function getCmdMsg()
    {
        return $this->cmdMsg;
    }

    public function setCmdStatusAndMsg( $status, $msg )
    {
        $this->cmdStatus = $status;
        $this->cmdMsg = $msg;
    }

}
