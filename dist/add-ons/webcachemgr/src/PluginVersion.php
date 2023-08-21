<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author Michael Alegre
 * @copyright (c) 2018-2023 LiteSpeed Technologies, Inc.
 * *******************************************
 */

namespace Lsc\Wp;

use Lsc\Wp\Context\Context;
use LsUserPanel\Lsc\UserLSCMException;

class PluginVersion
{

    /**
     * @var string
     */
    const DEFAULT_PLUGIN_PATH = '/wp-content/plugins/litespeed-cache';

    /**
     * @deprecated 1.9
     * @var string
     */
    const DOWNLOAD_DIR = '/usr/src/litespeed-wp-plugin';

    /**
     * @var string
     */
    const LSCWP_DEFAULTS_INI_FILE_NAME = 'const.default.ini';

    /**
     * @var string
     */
    const PLUGIN_NAME = 'litespeed-cache';

    /**
     * @var string
     */
    const TRANSLATION_CHECK_FLAG_BASE = '.ls_translation_check';

    /**
     * @var string
     */
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
     * @throws LSCMException  Thrown indirectly by $this->init() call.
     */
    protected function __construct()
    {
        $this->init();
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Context::isPrivileged() call.
     * @throws LSCMException  Thrown indirectly by $this->refreshVersionList()
     *     call.
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
     *
     * @throws LSCMException  Thrown indirectly by Context::getOption() call.
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
     *     $this->getShortVersions() instead. Un-formatted version of this list
     *     will no longer be available once this function is removed.
     *
     * @param bool $formatted
     *
     * @return string[]
     *
     * @throws LSCMException  Thrown indirectly by $this->setKnownVersions()
     *     call.
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
                        $ver = "$ver2[0].$ver2[1].x";
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
     *
     * @throws LSCMException  Thrown indirectly by $this->setAllowedVersions()
     *     call.
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
     *
     * @throws LSCMException  Thrown indirectly by $this->setShortVersions()
     *     call.
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
     *
     * @throws LSCMException  Thrown indirectly by $this->getAllowedVersions()
     *     call.
     */
    public function getLatestVersion()
    {
        $allowedVers = $this->getAllowedVersions();

        return (isset($allowedVers[0])) ? $allowedVers[0] : '';
    }

    /**
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by self::getInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     $instance->setCurrentVersion() call.
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
     * @throws LSCMException  Thrown indirectly by Context::getLSCMDataDir()
     *     call.
     */
    protected function checkOldVersionFiles()
    {
        $dataDir = Context::getLSCMDataDir();
        $oldActiveFile = "$dataDir/lscwp_active_version";

        if ( file_exists($oldActiveFile) ) {

            if ( !file_exists(Context::LOCAL_PLUGIN_DIR) ) {
                $this->createDownloadDir();
            }

            rename($oldActiveFile, $this->activeFile);
            unlink("$dataDir/lscwp_versions");
        }
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->checkOldVersionFiles()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
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
                $activeVersion = self::getLatestVersion();
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
     *     pre-formatted $this->setShortVersions().
     *
     * @throws LSCMException  Thrown when LSCWP version list cannot be found.
     * @throws LSCMException  Thrown when read LSCWP version list command fails.
     * @throws LSCMException  Thrown when match against LSCWP version list
     *     content fails.
     */
    protected function setKnownVersions()
    {
        if ( !file_exists($this->versionFile) ) {
            throw new LSCMException(
                'Cannot find LSCWP version list.',
                LSCMException::E_NON_FATAL
            );
        }

        if ( ($content = file_get_contents($this->versionFile)) === false ) {
            throw new LSCMException(
                'Failed to read LSCWP version list content.',
                LSCMException::E_NON_FATAL
            );
        }

        if ( preg_match('/old\s{(.*)}/sU', trim($content), $m) != 1 ) {
            throw new LSCMException(
                'Failed to get known versions from LSCWP version list content.',
                LSCMException::E_NON_FATAL
            );
        }

        $this->knownVersions = explode("\n", trim($m[1]));
    }

    /**
     *
     * @throws LSCMException  Thrown when LSCWP version list cannot be found.
     * @throws LSCMException  Thrown when read LSCWP version list command fails.
     * @throws LSCMException  Thrown when LSCWP version list content is empty.
     */
    protected function setAllowedVersions()
    {
        if ( !file_exists($this->versionFile) ) {
            throw new LSCMException(
                'Cannot find LSCWP version list.',
                LSCMException::E_NON_FATAL
            );
        }

        if ( ($content = file_get_contents($this->versionFile)) === false ) {
            throw new LSCMException(
                'Failed to read LSCWP version list content.',
                LSCMException::E_NON_FATAL
            );
        }

        $matchFound = preg_match(
            '/allowed\s{(.*)}/sU',
            trim($content),
            $m
        );

        if ( !$matchFound || ($list = trim($m[1])) == '' ) {
            throw new LSCMException(
                'LSCWP version list is empty.',
                LSCMException::E_NON_FATAL
            );
        }

        $this->allowedVersions = explode("\n", $list);
    }

    /**
     *
     * @since 4.1.3
     *
     * @throws LSCMException  Thrown when LSCWP version list cannot be found.
     * @throws LSCMException  Thrown when read LSCWP version list command fails.
     * @throws LSCMException  Thrown when LSCWP version list content is empty.
     */
    protected function setShortVersions()
    {
        if ( !file_exists($this->versionFile) ) {
            throw new LSCMException(
                'Cannot find LSCWP version list.',
                LSCMException::E_NON_FATAL
            );
        }

        if ( ($content = file_get_contents($this->versionFile)) === false ) {
            throw new LSCMException(
                'Failed to read LSCWP version list content.',
                LSCMException::E_NON_FATAL
            );
        }

        $matchFound = preg_match(
            '/short\s{(.*)}/sU',
            trim($content),
            $m
        );

        if ( !$matchFound || ($list = trim($m[1])) == '' ) {
            throw new LSCMException(
                'LSCWP version list is empty.',
                LSCMException::E_NON_FATAL
            );
        }

        $this->shortVersions = explode("\n", $list);

    }

    /**
     *
     * @param string $version  Valid LSCWP version.
     * @param bool   $init     True when trying to set initial active version.
     *
     * @throws LSCMException  Thrown indirectly by $this->getAllowedVersions()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     * @throws LSCMException  Thrown indirectly by $this->downloadVersion()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::notice() call.
     */
    public function setActiveVersion( $version, $init = false )
    {
        $allowedVers = $this->getAllowedVersions();

        if ( in_array($version, $allowedVers) ) {
            $activeVer = $version;
        }
        else {

            try {
                $currVer = ($init) ? '' : $this->getCurrentVersion();
            }
            catch ( LSCMException $e ) {
                $currVer = '';
            }

            if ( $currVer != '' ) {
                $activeVer = $currVer;
            }
            else {
                $activeVer = $allowedVers[0];
            }

            Logger::error(
                "Version $version not in allowed list, reset active "
                    . "version to $activeVer."
            );
        }

        if ( $activeVer != $this->getActiveVersion() ) {

            if ( !$this->hasDownloadedVersion($activeVer) ) {
                $this->downloadVersion($activeVer);
            }

            $this->currVersion = $activeVer;
            file_put_contents($this->activeFile, $activeVer);
            Logger::notice("Current active LSCWP version is now $activeVer.");
        }
    }

    /**
     *
     * @param bool $isforced
     *
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     */
    protected function refreshVersionList( $isforced = false )
    {
        clearstatcache();

        if ( $isforced || !file_exists($this->versionFile)
                || (time() - filemtime($this->versionFile)) > 86400 ) {

            if ( !file_exists(Context::LOCAL_PLUGIN_DIR) ) {
                $this->createDownloadDir();
            }

            $url = 'https://www.litespeedtech.com/packages/lswpcache'
                . '/version_list_v2';

            $content = Util::get_url_contents($url);

            if ( empty($content) || substr($content, 0, 7) != 'allowed' ) {
                /**
                 * Try again using cli curl directly to bypass potential
                 * reCAPTCHA issues.
                 */
                $content = Util::getUrlContentsUsingExecCurl($url);

                if ( empty($content) || substr($content, 0, 7) != 'allowed' ) {
                    touch($this->versionFile);
                    return;
                }
            }

            file_put_contents($this->versionFile, $content);
            Logger::info('LSCache for WordPress version list updated');
        }
    }

    /**
     * Filter out any versionList versions that do not meet specific criteria.
     *
     * @deprecated 4.1.3  No longer used.
     *
     * @param string $ver  Version string.
     *
     * @return bool
     */
    protected function filterVerList( $ver )
    {
        return Util::betterVersionCompare($ver, '1.2.2', '>');
    }

    /**
     * Checks the current installation for existing LSCWP plugin files and
     * copies them to the installation's plugins directory if not found.
     * This function should only be run as the user.
     *
     * @param string $pluginDir  The WordPress plugin directory.
     * @param string $version    The version of LSCWP to be used when copying
     *     over plugin files.
     *
     * @return bool  True when new LSCWP plugin files are used.
     *
     * @throws LSCMException  Thrown when LSCWP source package is not available
     *     for the provided version.
     * @throws LSCMException  Thrown when LSCWP plugin files could not be copied
     *     to plugin directory.
     * @throws LSCMException  Thrown indirectly by self::getInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     $instance->getCurrentVersion() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    public static function prepareUserInstall( $pluginDir, $version = '' )
    {
        $lscwp_plugin = "$pluginDir/litespeed-cache/litespeed-cache.php";

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
                "Source Package not available for version $version.",
                LSCMException::E_NON_FATAL
            );
        }

        exec(
            '/bin/cp --preserve=mode -rf '
                . Context::LOCAL_PLUGIN_DIR . "/$version/" . self::PLUGIN_NAME
                . " $pluginDir"
        );

        if ( !file_exists($lscwp_plugin) ) {
            throw new LSCMException(
                "Failed to copy plugin files to $pluginDir.",
                LSCMException::E_NON_FATAL
            );
        }

        $customIni = Context::LOCAL_PLUGIN_DIR . '/'
            . self::LSCWP_DEFAULTS_INI_FILE_NAME;

        if ( file_exists($customIni) ) {
            copy(
                $customIni,
                "$pluginDir/litespeed-cache/data/"
                    . self::LSCWP_DEFAULTS_INI_FILE_NAME
            );
        }

        Logger::debug(
            'Copied LSCache for WordPress plugin files into plugins directory '
                . $pluginDir
        );

        return true;
    }

    /**
     *
     * @param string $version
     *
     * @return bool
     */
    protected function hasDownloadedVersion( $version )
    {
        $dir = Context::LOCAL_PLUGIN_DIR . "/$version";
        $md5file = "$dir/" . self::VER_MD5;
        $plugin = "$dir/" . self::PLUGIN_NAME;

        if ( !file_exists($md5file) || !is_dir($plugin) ) {
            return false;
        }

        return ( file_get_contents($md5file) == Util::DirectoryMd5($plugin) );
    }

    /**
     *
     * @param string $version
     * @param string $dir
     * @param bool   $saveMD5
     *
     * @throws LSCMException  Thrown when wget command for downloaded LSCWP
     *     version fails.
     * @throws LSCMException  Thrown when unable to unzip LSCWP zip file.
     * @throws LSCMException  Thrown when unzipped LSCWP files do not contain
     *     expected test file.
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     * @throws LSCMException  Thrown indirectly by Util::unzipFile() call.
     */
    protected function wgetPlugin( $version, $dir, $saveMD5 = false )
    {
        Logger::info("Downloading LSCache for WordPress v$version...");

        $zipFile = self::PLUGIN_NAME . ".$version.zip";

        exec(
            'wget -q --tries=1 --no-check-certificate '
                . "https://downloads.wordpress.org/plugin/$zipFile -P $dir",
            $output,
            $return_var
        );

        if ( $return_var !== 0 ) {
            throw new LSCMException(
                "Failed to download LSCWP v$version with wget exit status "
                    . "$return_var.",
                LSCMException::E_NON_FATAL
            );
        }

        $localZipFile = "$dir/$zipFile";

        $extractedZip = Util::unzipFile($localZipFile, $dir);
        unlink($localZipFile);

        if ( !$extractedZip ) {
            throw new LSCMException(
                "Unable to unzip $localZipFile",
                LSCMException::E_NON_FATAL
            );
        }

        $plugin = "$dir/" . self::PLUGIN_NAME;

        if ( !file_exists("$plugin/" . self::PLUGIN_NAME . '.php') ) {
            throw new LSCMException(
                "Test file not found. Downloaded LSCWP v$version is invalid.",
                LSCMException::E_NON_FATAL
            );
        }

        if ( $saveMD5 ) {
            file_put_contents(
                "$dir/" . self::VER_MD5,
                Util::DirectoryMd5($plugin)
            );
        }
    }

    /**
     *
     * @param string $version
     *
     * @throws LSCMException  Thrown when download dir could not be created.
     * @throws LSCMException  Thrown indirectly by $this->wgetPlugin() call.
     */
    protected function downloadVersion( $version )
    {
        $dir = Context::LOCAL_PLUGIN_DIR . "/$version";

        if ( !file_exists($dir) ) {

            if ( !mkdir($dir, 0755, true) ) {
                throw new LSCMException(
                    "Failed to create download dir $dir.",
                    LSCMException::E_NON_FATAL
                );
            }
        }
        else {
            exec("/bin/rm -rf $dir/*");
        }

        $this->wgetPlugin($version, $dir, true);
    }

    /**
     *
     * @param string $locale
     * @param string $pluginVer
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     * @throws LSCMException  Thrown indirectly by Util::unzipFile() call.
     */
    public static function retrieveTranslation( $locale, $pluginVer )
    {
        Logger::info(
            "Downloading LSCache for WordPress $locale translation..."
        );

        $translationDir =
            Context::LOCAL_PLUGIN_DIR . "/$pluginVer/translations";

        if ( !file_exists($translationDir) ) {
            mkdir($translationDir, 0755);
        }

        touch(
            "$translationDir/" . self::TRANSLATION_CHECK_FLAG_BASE . "_$locale"
        );

        /**
         * downloads.wordpress.org looks to always return a '200 OK' status,
         * even when serving a 404 page. As such invalid downloads can only be
         * checked through user failure to unzip through WP func unzip_file()
         * as we do not assume that root has the ability to unzip.
         */
        exec(
            'wget -q --tries=1 --no-check-certificate '
                . 'https://downloads.wordpress.org/translation/plugin/'
                . "litespeed-cache/$pluginVer/$locale.zip "
                . "-P $translationDir",
            $output,
            $return_var
        );

        if ( $return_var !== 0 ) {
            return false;
        }

        /**
         * The WordPress user can unzip for us if this call fails.
         */
        Util::unzipFile("$translationDir/$locale.zip", $translationDir);

        return true;
    }

    /**
     *
     * @param string $locale
     * @param string $pluginVer
     *
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     */
    public static function removeTranslationZip( $locale, $pluginVer )
    {
        Logger::info("Removing LSCache for WordPress $locale translation...");

        $zipFile = realpath(
            Context::LOCAL_PLUGIN_DIR . "/$pluginVer/translations/$locale.zip"
        );

        $realPathStart = substr($zipFile, 0, strlen(Context::LOCAL_PLUGIN_DIR));

        if ( $realPathStart === Context::LOCAL_PLUGIN_DIR ) {
            unlink($zipFile);
        }
    }

}
