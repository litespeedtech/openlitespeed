<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2020
 * *******************************************
 */

namespace Lsc\Wp;

use \Lsc\Wp\Context\Context;

class PluginVersion
{

    const DEFAULT_PLUGIN_PATH = '/wp-content/plugins/litespeed-cache';

    /**
     * @deprecated 1.9
     */
    const DOWNLOAD_DIR = '/usr/src/litespeed-wp-plugin';

    const LSCWP_DEFAULTS_INI_FILE_NAME = 'const.default.ini';
    const PLUGIN_NAME = 'litespeed-cache';
    const TRANSLATION_CHECK_FLAG_BASE = '.ls_translation_check';
    const VER_MD5 = 'lscwp_md5';

    /**
     * @deprecated 4.1.3  Will be removed in favor of $this->shortVersions.
     * @var string[]
     */
    protected $knownVersions = array();

    /**
     * @since 4.1.3
     * @var string[]
     */
    protected $shortVersions = array();

    /**
     * @var string[]
     */
    protected $allowedVersions = array();

    /**
     * @var string
     */
    protected $currVersion = '';

    /**
     * @var string
     */
    protected $versionFile;

    /**
     * @var string
     */
    protected $activeFile;

    /**
     * @var null|PluginVersion
     */
    protected static $instance;

    /**
     *
     * @throws LSCMException  Thrown indirectly.
     */
    protected function __construct()
    {
        $this->init();
    }

    /**
     * @throws LSCMException  Thrown indirectly.
     */
    protected function init()
    {
        $this->setVersionFiles();

        if ( Context::isPrivileged() ) {
            $this->refreshVersionList();
        }
    }

    /**
     *
     * @return PluginVersion
     * @throws LSCMException  Thrown indirectly.
     */
    public static function getInstance()
    {
        if ( self::$instance == null ) {
            $className = Context::getOption()->getLscwpVerClass();

            self::$instance = new $className();
        }

        return self::$instance;
    }

    /**
     *
     * @deprecated 4.1.3  Use "$formatted = true" equivalent function
     *                    $this->getShortVersions() instead. Un-formatted
     *                    version of this list will no longer be available once
     *                    this function is removed.
     *
     * @param bool  $formatted
     * @return string[]
     * @throws LSCMException  Thrown indirectly.
     */
    public function getKnownVersions( $formatted = false )
    {
        if ( empty($this->knownVersions) ) {
            $this->setKnownVersions();
        }

        if ( $formatted ) {
            $knownVers = $this->knownVersions;

            $prevVer = '';

            foreach ( $knownVers as &$ver ) {

                if ( $prevVer !== '' ) {
                    $ver1 = explode('.', $prevVer);
                    $ver2 = explode('.', $ver);

                    if ( $ver1[0] !== $ver2[0] || $ver1[1] !== $ver2[1] ) {
                        $ver = "{$ver2[0]}.{$ver2[1]}.x";
                    }
                }

                $prevVer = $ver;
            }

            return $knownVers;
        }

        return $this->knownVersions;
    }

    /**
     *
     * @return string[]
     * @throws LSCMException  Thrown indirectly.
     */
    public function getAllowedVersions()
    {
        if ( empty($this->allowedVersions) ) {
            $this->setAllowedVersions();
        }

        return $this->allowedVersions;
    }

    /**
     *
     * @since 4.1.3
     *
     * @return string[]
     * @throws LSCMException  Thrown indirectly.
     */
    public function getShortVersions()
    {
        if ( empty($this->shortVersions) ) {
            $this->setShortVersions();
        }

        return $this->shortVersions;
    }

    /**
     *
     * @return string
     * @throws LSCMException  Thrown indirectly.
     */
    public function getLatestVersion()
    {
        $allowedVers = $this->getAllowedVersions();


        return (isset($allowedVers[0])) ? $allowedVers[0] : '';
    }

    /**
     *
     * @return string
     * @throws LSCMException  Thrown indirectly.
     */
    public static function getCurrentVersion()
    {
        $instance = self::getInstance();

        if ( $instance->currVersion == '' ) {
            $instance->setCurrentVersion();
        }

        return $instance->currVersion;
    }

    /**
     *
     * @since 1.9
     */
    protected function createDownloadDir()
    {
        mkdir(Context::LOCAL_PLUGIN_DIR, 0755);
    }

    protected function setVersionFiles()
    {
        $this->versionFile = Context::LOCAL_PLUGIN_DIR . '/lscwp_versions_v2';
        $this->activeFile = Context::LOCAL_PLUGIN_DIR . '/lscwp_active_version';
    }

    /**
     * Temporary function for handling the active version file and versions
     * file move in the short term.
     *
     * @deprecated 1.9
     * @since 1.9
     *
     * @throws LSCMException
     */
    protected function checkOldVersionFiles()
    {
        $dataDir = Context::getLSCMDataDir();
        $oldVersionsFile = "{$dataDir}/lscwp_versions";
        $oldActiveFile = "{$dataDir}/lscwp_active_version";

        if ( file_exists($oldActiveFile) ) {

            if ( !file_exists(Context::LOCAL_PLUGIN_DIR) ) {
                $this->createDownloadDir();
            }

            rename($oldActiveFile, $this->activeFile);
            unlink($oldVersionsFile);
        }
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly.
     */
    public function setCurrentVersion()
    {
        $this->checkOldVersionFiles();

        if ( ($activeVersion = $this->getActiveVersion()) == '' ) {
            Logger::debug('Active LSCWP version not found.');
        }
        elseif ( ! $this->hasDownloadedVersion($activeVersion) ) {
            $activeVersion = '';
            unlink($this->activeFile);
            Logger::debug('Valid LSCWP download not found.');
        }

        if ( $activeVersion == '' ) {

            try {
                $latestVer = self::getLatestVersion();
                $activeVersion = $latestVer;
            }
            catch ( LSCMException $e ) {
                Logger::debug($e->getMessage());
            }
        }

        $this->currVersion = $activeVersion;
    }

    /**
     *
     * @since 1.11
     *
     * @return string
     */
    private function getActiveVersion()
    {
        if ( file_exists($this->activeFile)
                && ($content = file_get_contents($this->activeFile)) ) {

            return trim($content);
        }

        return '';
    }

    /**
     *
     * @deprecated 4.1.3  This function will be removed in favor of
     *                    pre-formatted $this->setShortVersions().
     *
     * @throws LSCMException
     */
    protected function setKnownVersions()
    {
        if ( !file_exists($this->versionFile) ) {
            throw new LSCMException(
                'Cannot find LSCWP version list.',
                LSCMException::E_NON_FATAL
            );
        }

        $content = trim(file_get_contents($this->versionFile));

        preg_match('/old\s{(.*)}/sU', $content, $m);

        $this->knownVersions = explode("\n", trim($m[1]));
    }

    /**
     *
     * @throws LSCMException
     */
    protected function setAllowedVersions()
    {
        if ( !file_exists($this->versionFile) ) {
            throw new LSCMException(
                'Cannot find LSCWP version list.',
                LSCMException::E_NON_FATAL
            );
        }

        $content = trim(file_get_contents($this->versionFile));

        preg_match('/allowed\s{(.*)}/sU', $content, $m);

        $this->allowedVersions = explode("\n", trim($m[1]));
    }

    /**
     *
     * @since 4.1.3
     *
     * @throws LSCMException
     */
    protected function setShortVersions()
    {
        if ( !file_exists($this->versionFile) ) {
            throw new LSCMException(
                'Cannot find LSCWP version list.',
                LSCMException::E_NON_FATAL
            );
        }

        $content = trim(file_get_contents($this->versionFile));

        preg_match('/short\s{(.*)}/sU', $content, $m);

        $this->shortVersions = explode("\n", trim($m[1]));
    }

    /**
     *
     * @param string   $version  Valid LSCWP version.
     * @param boolean  $init     True when trying to set initial active
     *                           version.
     * @throws LSCMException  Thrown indirectly.
     */
    public function setActiveVersion( $version, $init = false )
    {
        try {
            $currVer = ($init) ? '' : $this->getCurrentVersion();
        }
        catch ( LSCMException $e ) {
            $currVer = '';
        }

        $allowedVers = $this->getAllowedVersions();

        if ( !in_array($version, $allowedVers) ) {
            $badVer = $version;

            if ( $currVer != '' ) {
                $version = $currVer;
            }
            else {
                $version = $allowedVers[0];
            }

            Logger::error(
                "Version {$badVer} not in allowed list, reset active "
                    . "version to {$version}."
            );
        }

        if ( $version != $this->getActiveVersion() ) {

            if ( !$this->hasDownloadedVersion($version) ) {
                $this->downloadVersion($version);
            }

            $this->currVersion = $version;
            file_put_contents($this->activeFile, $version);
            Logger::notice("Current active LSCWP version is now {$version}.");
        }
    }

    /**
     *
     * @param boolean  $isforced
     * @throws LSCMException  Thrown indirectly.
     */
    protected function refreshVersionList( $isforced = false )
    {
        clearstatcache();
        $versionsUrl =
            'https://www.litespeedtech.com/packages/lswpcache/version_list_v2';

        if ( $isforced || !file_exists($this->versionFile)
                || (time() - filemtime($this->versionFile)) > 86400 ) {

            $content = Util::get_url_contents($versionsUrl);

            if ( !empty($content) ) {

                if ( !file_exists(Context::LOCAL_PLUGIN_DIR) ) {
                    $this->createDownloadDir();
                }

                file_put_contents($this->versionFile, $content);
                Logger::info('LSCache for WordPress version list updated');
            }
        }
    }

    /**
     * Filter out any versionList versions that do not meet specific criteria.
     *
     * @deprecated 4.1.3  No longer used.
     *
     * @param string  $ver  Version string.
     * @return boolean
     */
    protected function filterVerList( $ver )
    {
        return version_compare($ver, '1.2.2', '>');
    }

    /**
     * Checks the current installation for existing LSCWP plugin files and
     * copies them to the installation's plugins directory if not found.
     * This function should only be run as the user.
     *
     * @param string  $pluginDir  The WordPress plugin directory.
     * @param string  $version    The version of LSCWP to be used when
     *                             copying over plugin files.
     * @return boolean            True when new LSCWP plugin files are used.
     * @throws LSCMException
     */
    public static function prepareUserInstall( $pluginDir, $version = '' )
    {
        $lscwp_plugin = "{$pluginDir}/litespeed-cache/litespeed-cache.php";

        if ( file_exists($lscwp_plugin) ) {
            /**
             * Existing installation detected.
             */
            return false;
        }

        $instance = self::getInstance();

        if ( $version == '' ) {
            $version = $instance->getCurrentVersion();
        }

        if ( !$instance->hasDownloadedVersion($version) ) {
            throw new LSCMException(
                "Source Package not available for version {$version}.",
                LSCMException::E_NON_FATAL
            );
        }

        $pluginSrc =
                Context::LOCAL_PLUGIN_DIR . "/{$version}/" . self::PLUGIN_NAME;
        exec("/bin/cp -rf {$pluginSrc} {$pluginDir}");

        if ( !file_exists($lscwp_plugin) ) {
            throw new LSCMException(
                "Failed to copy plugin files to {$pluginDir}.",
                LSCMException::E_NON_FATAL
            );
        }

        $customIni = Context::LOCAL_PLUGIN_DIR . '/'
            . self::LSCWP_DEFAULTS_INI_FILE_NAME;
        $defaultIni = "{$pluginDir}/litespeed-cache/data/"
            . self::LSCWP_DEFAULTS_INI_FILE_NAME;

        if ( file_exists($customIni) ) {
            copy($customIni, $defaultIni);
        }

        Logger::debug(
            'Copied LSCache for WordPress plugin files into plugins directory '
                . $pluginDir
        );

        return true;
    }

    /**
     *
     * @param string  $version
     * @return boolean
     */
    protected function hasDownloadedVersion( $version )
    {
        $dir = Context::LOCAL_PLUGIN_DIR . "/{$version}";
        $md5file = "{$dir}/" . self::VER_MD5;
        $plugin = "{$dir}/" . self::PLUGIN_NAME;

        if ( !file_exists($md5file) || !is_dir($plugin) ) {
            return false;
        }

        $md5_stored_val = file_get_contents($md5file);
        $md5_val = Util::DirectoryMd5($plugin);

        return ( $md5_stored_val == $md5_val );
    }

    /**
     *
     * @param string   $version
     * @param string   $dir
     * @param boolean  $saveMD5
     * @throws LSCMException
     */
    protected function wgetPlugin( $version, $dir, $saveMD5 = false )
    {
        Logger::info("Downloading LSCache for WordPress v{$version}...");

        $plugin = "{$dir}/" . self::PLUGIN_NAME;
        $zipFile = self::PLUGIN_NAME . ".{$version}.zip";
        $localZipFile = "{$dir}/{$zipFile}";

        $url = "https://downloads.wordpress.org/plugin/{$zipFile}";
        $wget_command =
            "wget -q --tries=1 --no-check-certificate {$url} -P {$dir}";

        exec($wget_command, $output, $return_var);

        if ( $return_var !== 0 ) {
            throw new LSCMException(
                "Failed to download LSCWP v{$version} with wget "
                    . "exit status {$return_var}.",
                LSCMException::E_NON_FATAL
            );
        }

        $extracted = Util::unzipFile($localZipFile, $dir);
        unlink($localZipFile);

        if ( !$extracted ) {
            throw new LSCMException(
                "Unable to unzip {$localZipFile}",
                LSCMException::E_NON_FATAL
            );
        }

        $testfile = "{$plugin}/" . self::PLUGIN_NAME . '.php';

        if ( !file_exists($testfile) ) {
            throw new LSCMException(
                "Unable to download LSCWP v{$version}.",
                LSCMException::E_NON_FATAL
            );
        }

        if ( $saveMD5 ) {
            $md5_val = Util::DirectoryMd5($plugin);
            file_put_contents("{$dir}/" . self::VER_MD5, $md5_val);
        }
    }

    /**
     *
     * @param string  $version
     * @throws LSCMException
     */
    protected function downloadVersion( $version )
    {
        $dir = Context::LOCAL_PLUGIN_DIR . "/{$version}";

        if ( !file_exists($dir) ) {

            if ( !mkdir($dir, 0755, true) ) {
                throw new LSCMException(
                    "Failed to create download dir {$dir}.",
                    LSCMException::E_NON_FATAL
                );
            }
        }
        else {
            exec("/bin/rm -rf {$dir}/*");
        }

        $this->wgetPlugin($version, $dir, true);
    }

    /**
     *
     * @param string  $locale
     * @param string  $pluginVer
     * @return boolean
     * @throws LSCMException  Thrown indirectly.
     */
    public static function retrieveTranslation( $locale, $pluginVer )
    {
        Logger::info(
            "Downloading LSCache for WordPress {$locale} translation..."
        );

        $translationDir =
            Context::LOCAL_PLUGIN_DIR . "/{$pluginVer}/translations";
        $zipFile = "{$locale}.zip";
        $localZipFile = "{$translationDir}/{$zipFile}";

        if ( !file_exists($translationDir) ) {
            mkdir($translationDir, 0755);
        }

        touch(
            "$translationDir/" . self::TRANSLATION_CHECK_FLAG_BASE
                . "_{$locale}"
        );

        /**
         * downloads.wordpress.org looks to always return a '200 OK' status,
         * even when serving a 404 page. As such invalid downloads can only be
         * checked through user failure to unzip through WP func unzip_file()
         * as we do not assume that root has the ability to unzip.
         */
        $url = 'https://downloads.wordpress.org/translation/plugin/'
            . "litespeed-cache/{$pluginVer}/{$locale}.zip";
        $wget_command = 'wget -q --tries=1 --no-check-certificate '
            . "{$url} -P {$translationDir}";

        exec($wget_command, $output, $return_var);

        if ( $return_var !== 0 ) {
            return false;
        }

        /**
         * WordPress user can unzip for us if this call fails.
         */
        Util::unzipFile($localZipFile, $translationDir);

        return true;
    }

    /**
     *
     * @param string  $locale
     * @param string  $pluginVer
     * @throws LSCMException  Thrown indirectly.
     */
    public static function removeTranslationZip( $locale, $pluginVer )
    {
        Logger::info("Removing LSCache for WordPress {$locale} translation...");

        $zipFile = realpath(
            Context::LOCAL_PLUGIN_DIR
                . "/{$pluginVer}/translations/{$locale}.zip"
        );

        $realPathStart = substr($zipFile, 0, strlen(Context::LOCAL_PLUGIN_DIR));

        if ( $realPathStart === Context::LOCAL_PLUGIN_DIR ) {
            unlink($zipFile);
        }
    }

}
