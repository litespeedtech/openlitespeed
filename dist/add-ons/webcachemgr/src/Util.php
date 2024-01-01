<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2023 LiteSpeed Technologies, Inc.
 * *******************************************
 */

namespace Lsc\Wp;


use ZipArchive;

class Util
{

    /**
     *
     * @param string $tag
     *
     * @return string
     */
    public static function get_request_var( $tag )
    {
        if ( isset($_REQUEST[$tag]) ) {
            return trim($_REQUEST[$tag]);
        }

        /**
         * Request var not found in $_REQUEST, try checking POST and
         * QUERY_STRING environment variables.
         */
        if ( $_SERVER['REQUEST_METHOD'] === 'POST' ) {
            $querystring = urldecode(getenv('POST'));
        }
        else {
            $querystring = urldecode(getenv('QUERY_STRING'));
        }

        if ( $querystring != ''
                && preg_match("/(?:^|\?|&)$tag=([^&]+)/", $querystring, $m) ) {

            return trim($m[1]);
        }

        return null;
    }

    /**
     *
     * @param string $tag
     *
     * @return array
     */
    public static function get_request_list( $tag )
    {
        $varValue = null;

        if ( isset($_REQUEST[$tag]) ) {
            $varValue = $_REQUEST[$tag];
        }
        else {
            /**
             * Request var not found in $_REQUEST, try checking POST and
             * QUERY_STRING environment variables.
             */
            if ( $_SERVER['REQUEST_METHOD'] === 'POST' ) {
                $querystring = urldecode(getenv('POST'));
            }
            else {
                $querystring = urldecode(getenv('QUERY_STRING'));
            }

            if ( $querystring != ''
                    && preg_match_all("/(?:^|\?|&)$tag\[]=([^&]+)/", $querystring, $m) ) {

                $varValue = $m[1];
            }
        }

        return (is_array($varValue)) ? $varValue : null;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     */
    public static function restartLsws()
    {
        Logger::info('Performing a Graceful Restart to apply changes...');

        /**
         * @noinspection PhpMethodParametersCountMismatchInspection  Suppress
         *     for PHP 5.x.
         */
        if ( php_uname('s') == 'FreeBSD' ) {
            $lswsCtl = '/usr/local/etc/rc.d/lsws.sh';
        }
        else {
            $lswsCtl = '/sbin/service lsws';
        }

        exec("$lswsCtl restart");
    }

    /**
     * @since 2.1.15
     *
     * @param int $startTime
     * @param int $timeout
     *
     * @return bool
     */
    public static function timedOut( $startTime, $timeout )
    {
        return ((time() - $startTime) > $timeout);
    }

    /**
     * This function is used to get the file owner by name. Useful in cases
     * where UID is not accepted or setting a files group to match its owner
     * (It is not safe to assume UID == GID or GID exists for username 'x').
     *
     * @since 2.2.0
     *
     * @param string $filepath
     *
     * @return array  Keys are id, name, group_id
     */
    public static function populateOwnerInfo( $filepath )
    {
        clearstatcache();
        $ownerID = fileowner($filepath);
        $ownerInfo = posix_getpwuid($ownerID);

        return array(
            'user_id'   => $ownerID,
            'user_name' => $ownerInfo['name'],
            'group_id'  => filegroup($filepath)
        );
    }

    /**
     *
     * @param string $file
     * @param string $owner
     * @param string $group
     */
    public static function changeUserGroup( $file, $owner, $group )
    {
        chown($file, $owner);
        chgrp($file, $group);
    }

    /**
     * Set file permissions of $file2 to match those of $file1.
     *
     * @since 2.2.0
     *
     * @param string $file1
     * @param string $file2
     */
    public static function matchPermissions( $file1, $file2 )
    {
        /**
         * convert fileperms() returned dec to oct
         */
        chmod($file2, (fileperms($file1) & 0777));
    }

    /**
     *
     * @since 1.14.3
     *
     * @param string $url
     * @param bool   $headerOnly
     *
     * @return string
     */
    public static function getUrlContentsUsingFileGetContents(
        $url,
        $headerOnly = false )
    {
        if ( ini_get('allow_url_fopen') ) {
            /**
             * silence warning when OpenSSL missing while getting LSCWP ver
             * file.
             */
            $url_content = @file_get_contents($url);

            if ( $url_content !== false ) {

                if ( $headerOnly ) {
                    return implode("\n", $http_response_header);
                }

                return $url_content;
            }
        }

        return '';
    }

    /**
     *
     * @since 1.14.3
     *
     * @param string $url
     * @param bool   $headerOnly
     *
     * @return string
     */
    public static function getUrlContentsUsingPhpCurl(
        $url,
        $headerOnly = false )
    {
        if ( function_exists('curl_version') ) {
            $ch = curl_init();

            curl_setopt_array(
                $ch,
                array(
                    CURLOPT_URL            => $url,
                    CURLOPT_RETURNTRANSFER => true,
                    CURLOPT_HEADER         => $headerOnly,
                    CURLOPT_NOBODY         => $headerOnly,
                    CURLOPT_HTTP_VERSION   => CURL_HTTP_VERSION_1_1
                )
            );

            $url_content = curl_exec($ch);
            curl_close($ch);

            if ( $url_content !== false ) {
                return $url_content;
            }
        }

        return '';
    }

    /**
     *
     * @since 1.14.3
     *
     * @param string $url
     * @param string $headerOnly
     *
     * @return string
     */
    public static function getUrlContentsUsingExecCurl(
        $url,
        $headerOnly = false )
    {
        $cmd = 'curl -s';

        if ( $headerOnly ) {
            $cmd .= ' -I';
        }

        exec("$cmd $url", $output, $ret);

        if ( $ret === 0 ) {
            return implode("\n", $output);
        }

        return '';
    }

    /**
     *
     * @param string $url
     * @param bool   $headerOnly
     *
     * @return string
     */
    public static function get_url_contents( $url, $headerOnly = false )
    {
        $content = self::getUrlContentsUsingFileGetContents($url, $headerOnly);

        if ( $content != '' ) {
            return $content;
        }

        $content = self::getUrlContentsUsingPhpCurl($url, $headerOnly);

        if ( $content != '' ) {
            return $content;
        }

        return self::getUrlContentsUsingExecCurl($url, $headerOnly);
    }

    /**
     *
     * @param string $dir
     *
     * @return false|string
     */
    public static function DirectoryMd5( $dir )
    {
        if ( !is_dir($dir) ) {
            return false;
        }

        $fileMd5s = array();
        $d = dir($dir);

        while ( ($entry = $d->read()) !== false ) {

            if ( $entry != '.' && $entry != '..' ) {
                $currEntry = "$dir/$entry";

                if ( is_dir($currEntry) ) {
                    $fileMd5s[] = self::DirectoryMd5($currEntry);
                }
                else {
                    $fileMd5s[] = md5_file($currEntry);
                }
            }
        }

        $d->close();
        return md5(implode('', $fileMd5s));
    }

    /**
     *
     * @param string $file
     * @param string $backup
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    private static function matchFileSettings( $file, $backup )
    {
        clearstatcache();
        $ownerID = fileowner($file);
        $groupID = filegroup($file);

        if ( $ownerID === false || $groupID === false ) {
            Logger::debug("Could not get owner/group of file $file");

            unlink($backup);

            Logger::debug("Removed file $backup");
            return false;
        }

        self::changeUserGroup($backup, $ownerID, $groupID);
        self::matchPermissions($file, $backup);

        return true;
    }

    /**
     *
     * @param string $filepath
     * @param string $bak
     *
     * @return string
     */
    private static function getBackupSuffix(
        $filepath,
        $bak = '_lscachebak_orig' )
    {
        $i = 1;

        if ( file_exists($filepath . $bak) ) {
            $bak = sprintf("_lscachebak_%02d", $i);

            while ( file_exists($filepath . $bak) ) {
                $i++;
                $bak = sprintf("_lscachebak_%02d", $i);
            }
        }

        return $bak;
    }

    /**
     *
     * @param string $filepath
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::verbose() call.
     * @throws LSCMException  Thrown indirectly by self::matchFileSettings()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     */
    public static function createBackup( $filepath )
    {
        $backup = $filepath . self::getBackupSuffix($filepath);

        if ( !copy($filepath, $backup) ) {
            Logger::debug(
                "Could not backup file $filepath to location $backup"
            );

            return false;
        }

        Logger::verbose("Created file $backup");

        if ( !self::matchFileSettings($filepath, $backup) ) {
            Logger::debug(
                "Could not backup file $filepath to location $backup"
            );

            return false;
        }

        Logger::debug('Matched owner/group setting for both files');
        Logger::info(
            "Successfully backed up file $filepath to location $backup"
        );

        return true;
    }

    /**
     *
     * @param string $zipFile
     * @param string $dest
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    public static function unzipFile( $zipFile, $dest )
    {
        if ( class_exists('\ZipArchive') ) {
            $zipArchive = new ZipArchive();

            if ( $zipArchive->open($zipFile) === true ) {
                $extracted = $zipArchive->extractTo($dest);
                $zipArchive->close();

                if ( $extracted ) {
                    return true;
                }
            }

            Logger::debug("Could not unzip $zipFile using ZipArchive.");
        }

        $output = array();

        exec(
            "unzip $zipFile -d $dest > /dev/null 2>&1",
            $output,
            $return_var
        );

        if ( $return_var == 0 ) {
            return true;
        }
        else {
            Logger::debug("Could not unzip $zipFile from cli.");
        }

        return false;
    }

    /**
     * Check if a given directory is empty.
     *
     * @param string $dir
     *
     * @return bool
     */
    public static function is_dir_empty( $dir )
    {
        if ( !($handle = @opendir($dir)) ) {
            return true;
        }

        while ( ($entry = readdir($handle)) !== false ) {

            if ( $entry != '.' && $entry != '..' ) {
                return false;
            }
        }

        return true;
    }

    /**
     *
     * @param string $vhCacheRoot
     */
    public static function ensureVHCacheRootInCage( $vhCacheRoot )
    {
        $cageFsFile = '/etc/cagefs/cagefs.mp';

        if ( file_exists($cageFsFile) ) {

            if ( $vhCacheRoot[0] == '/' ) {
                $cageVhCacheRoot =
                    '%' . str_replace('/$vh_user', '', $vhCacheRoot);

                $matchFound = preg_grep(
                    "!^\s*" . str_replace('!', '\!', $cageVhCacheRoot) . "!im",
                    file($cageFsFile)
                );

                if ( !$matchFound ) {
                    file_put_contents(
                        $cageFsFile,
                        "\n$cageVhCacheRoot",
                        FILE_APPEND
                    );

                    exec('/usr/sbin/cagefsctl --remount-all');
                }
            }
        }
    }

    /**
     * Recursively a directory's contents and optionally the directory itself.
     *
     * @param string $dir        Directory path
     * @param bool   $keepParent Only remove directory contents when true.
     *
     * @return bool
     */
    public static function rrmdir( $dir, $keepParent = false )
    {
        if ( $dir != '' && is_dir($dir) ) {

            if ( ($matches = glob("$dir/*")) === false ) {
                return false;
            }

            foreach ( $matches as $file ) {

                if ( is_dir($file) ) {
                    self::rrmdir($file);
                }
                else {
                    unlink($file);
                }
            }

            if ( !$keepParent ) {
                rmdir($dir);
            }

            return true;
        }

        return false;
    }

    /**
     * Wrapper for idn_to_utf8() function call to avoid "undefined" exceptions
     * when PHP intl module is not installed and enabled.
     *
     * @since 1.13.13.1
     *
     * @param string     $domain
     * @param int        $flags
     * @param int|null   $variant
     * @param array|null $idna_info
     *
     * @return false|string
     */
    public static function tryIdnToUtf8(
              $domain,
              $flags = 0,
              $variant = null,
        array &$idna_info = null )
    {
        if ( function_exists('idn_to_utf8') ) {

            if ( $variant == null ) {
                $variant = INTL_IDNA_VARIANT_UTS46;
            }

            return idn_to_utf8($domain, $flags, $variant, $idna_info);
        }

        return $domain;
    }

    /**
     * Wrapper for idn_to_ascii() function call to avoid "undefined" exceptions
     * when PHP intl module is not installed and enabled.
     *
     * @since 1.13.13.1
     *
     * @param string     $domain
     * @param int|null   $flags
     * @param int|null   $variant
     * @param array|null $idna_info
     *
     * @return false|string
     */
   public static function tryIdnToAscii(
             $domain,
             $flags = null,
             $variant = null,
       array &$idna_info = null )
   {
       if ( function_exists('idn_to_ascii') ) {

           if ( $flags == null ) {
               $flags = IDNA_DEFAULT;
           }

           if ( $variant == null ) {
               $variant = INTL_IDNA_VARIANT_UTS46;
           }

           return idn_to_ascii($domain, $flags, $variant, $idna_info);
       }

       return $domain;
   }

    /**
     * Version comparison function capable of properly comparing versions with
     * trailing ".0" groups such as '6.1' which is equal to '6.1.0' which is
     * equal to '6.1.000.0' etc.
     *
     * @since 1.14.2
     *
     * @param string      $ver1
     * @param string      $ver2
     * @param string|null $operator
     *
     * @return bool|int
     */
    public static function betterVersionCompare(
        $ver1,
        $ver2,
        $operator = null )
    {
        $pattern = '/(\.0+)+($|-)/';

        return version_compare(
            preg_replace($pattern, '', $ver1),
            preg_replace($pattern, '', $ver2),
            $operator
        );
    }

    /**
     *
     * @since 1.15.0.1
     *
     * @param string                           $constantName
     * @param array|bool|float|int|null|string $value
     * @param bool                             $caseInsensitive  Optional
     *     parameter used for define calls in PHP versions below 7.3.
     *
     * @return bool
     *
     * @noinspection PhpDeprecationInspection  Ignore deprecation of define()
     *     parameter $case_insensitive for PHP versions below 7.3.
     * @noinspection RedundantSuppression
     */
    public static function define_wrapper(
        $constantName,
        $value,
        $caseInsensitive = false )
    {
        if ( PHP_VERSION_ID < 70300 ) {
            return define($constantName, $value, $caseInsensitive);
        }
        else {
            return define($constantName, $value);
        }
    }
}
