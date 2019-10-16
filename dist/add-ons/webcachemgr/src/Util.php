<?php

/* * *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2019
 * *******************************************
 */

namespace Lsc\Wp;

use \Lsc\Wp\Logger;

class Util
{

    /**
     *
     * @param string  $tag
     * @return string
     */
    public static function get_request_var( $tag )
    {
        $varValue = null;

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
                && preg_match("/(?:^|\?|&){$tag}=([^&]+)/", $querystring, $m) ) {

            $varValue = $m[1];
        }

         if ( $varValue != null ) {
            $varValue = trim($varValue);
        }

        return $varValue;
    }

    /**
     *
     * @param string  $tag
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
                    && preg_match_all("/(?:^|\?|&){$tag}\[\]=([^&]+)/", $querystring, $m) ) {

                $varValue = $m[1];
            }
        }

        return (is_array($varValue)) ? $varValue : null;
    }

    public static function restartLsws()
    {
        Logger::info('Performing a Graceful Restart to apply changes...');

        $OS = php_uname('s');

        if ( $OS == 'FreeBSD' ) {
            $lswsCtl = '/usr/local/etc/rc.d/lsws.sh';
        }
        else {
            $lswsCtl = '/sbin/service lsws';
        }

        $cmd = "{$lswsCtl} restart";
        exec($cmd);
    }

    /**
     * @since 2.1.15
     *
     * @param int   $startTime
     * @param int   $timeout
     * @return bool
     */
    public static function timedOut( $startTime, $timeout )
    {
        return ((time() - $startTime) > $timeout);
    }

    /**
     * This function is used to get the file owner by name. Useful in cases
     * where UID is not accepted or setting a files group to match owner
     * (It is not safe to assume UID == GID or GID exists for username 'x').
     *
     * @since 2.2.0
     *
     * @param string  $filepath
     * @return array             Keys are id, name, group_id
     */
    public static function populateOwnerInfo( $filepath )
    {
        clearstatcache();
        $ownerID = fileowner($filepath);
        $ownerInfo = posix_getpwuid($ownerID);
        $name = $ownerInfo['name'];
        $groupID = filegroup($filepath);

        $info = array(
            'user_id' => $ownerID,
            'user_name' => $name,
            'group_id' => $groupID
        );

        return $info;
    }

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
     * @param string  $file1
     * @param string  $file2
     */
    public static function matchPermissions( $file1, $file2 )
    {
        $perms = (fileperms($file1) & 0777); #convert dec to oct

        chmod($file2, $perms);
    }

    /**
     *
     * @param string   $url
     * @param boolean  $headerOnly
     * @return string
     */
    public static function get_url_contents( $url, $headerOnly = false )
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

        if ( function_exists('curl_version') ) {
            $ch = curl_init();

            curl_setopt_array(
                    $ch,
                    array(
                        CURLOPT_URL => $url,
                        CURLOPT_RETURNTRANSFER => true,
                        CURLOPT_HEADER => $headerOnly,
                        CURLOPT_NOBODY => $headerOnly,
                        CURLOPT_HTTP_VERSION => CURL_HTTP_VERSION_1_1
                    )
            );

            $url_content = curl_exec($ch);
            curl_close($ch);

            if ( $url_content !== false ) {
                return $url_content;
            }
        }

        $cmd = 'curl';

        if ( $headerOnly ) {
            $cmd .= ' -s -I';
        }

        $url_content = exec("{$cmd} {$url}", $output, $ret);

        if ( $ret === 0 ) {
            $url_content = implode("\n", $output);
            return $url_content;
        }

        return '';
    }

    public static function DirectoryMd5( $dir )
    {
        if ( !is_dir($dir) ) {
            return false;
        }

        $filemd5s = array();
        $d = dir($dir);

        while ( ($entry = $d->read()) !== false ) {

            if ( $entry != '.' && $entry != '..' ) {

                $currEntry = "{$dir}/{$entry}";

                if ( is_dir($currEntry) ) {
                    $filemd5s[] = self::DirectoryMd5($currEntry);
                }
                else {
                    $filemd5s[] = md5_file($currEntry);
                }
            }
        }

        $d->close();
        return md5(implode('', $filemd5s));
    }

    private static function matchFileSettings( $file, $backup )
    {
        clearstatcache();
        $ownerID = fileowner($file);
        $groupID = filegroup($file);

        if ( $ownerID === false || $groupID === false ) {
            Logger::debug("Could not get owner/group of file {$file}");

            unlink($backup);

            Logger::debug("Removed file {$backup}");
            return false;
        }

        self::changeUserGroup($backup, $ownerID, $groupID);
        self::matchPermissions($file, $backup);

        return true;
    }

    private static function getBackupSuffix( $filepath, $bak = '_lscachebak_orig' )
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

    public static function createBackup( $filepath )
    {
        $bak = self::getBackupSuffix($filepath);
        $backup = $filepath . $bak;

        if ( !copy($filepath, $backup) ) {
            Logger::debug("Could not backup file {$filepath} to location {$backup}");

            return false;
        }

        Logger::verbose("Created file{$backup}");

        if ( !self::matchFileSettings($filepath, $backup) ) {
            Logger::debug("Could not backup file {$filepath} to location {$backup}");

            return false;
        }

        Logger::debug('Matched owner/group setting for both files');
        Logger::info("Successfully backed up file {$filepath} to location {$backup}");

        return true;
    }

    public static function unzipFile( $zipFile, $dest )
    {
        if ( class_exists('\ZipArchive') ) {
            $zipArchive = new \ZipArchive();

            if ( $zipArchive->open($zipFile) === true ) {
                $extracted = $zipArchive->extractTo($dest);
                $zipArchive->close();

                if ( $extracted ) {
                    return true;
                }
            }

            Logger::debug("Could not unzip {$zipFile} using ZipArchive.");
        }

        $output = array();

        exec("unzip {$zipFile} -d {$dest} > /dev/null 2>&1", $output, $return_var);

        if ( $return_var == 0 ) {
            return true;
        }
        else {
            Logger::debug("Could not unzip {$zipFile} from cli.");
        }

        return false;
    }

    /**
     * Check if directory is empty.
     *
     * @param string  $dir
     * @return boolean
     */
    public static function is_dir_empty( $dir )
    {

        if ( ($handle = @opendir($dir)) == false )
        {
            return true;
        }

        while ( ($entry = readdir($handle)) !== true ) {

            if ( $entry != '.' && $entry != '..' ) {
                return false;
            }
        }

        return true;
    }

    /**
     *
     * @param string  $vhCacheRoot
     */
    public static function ensureVHCacheRootInCage( $vhCacheRoot )
    {
        $cageFsFile = '/etc/cagefs/cagefs.mp';

        $remount = false;

        if ( file_exists($cageFsFile) ) {

            $file_contents = file($cageFsFile);

            if ( $vhCacheRoot[0] == '/' ) {

                $vhCacheRoot = '%' . str_replace('/$vh_user', '', $vhCacheRoot);
                $escVHCacheRoot = str_replace('!', '\!', $vhCacheRoot);

                if ( !preg_grep('!^\s*' . $escVHCacheRoot . '!im', $file_contents) ) {
                    $remount = true;
                    file_put_contents($cageFsFile, "\n{$vhCacheRoot}",
                            FILE_APPEND);
                }
            }

            if ( $remount ) {
                exec('/usr/sbin/cagefsctl --remount-all');
            }
        }
    }

    /**
     * Recursively a directory's contents and optionally the directory itself.
     *
     * @param string   $dir         Directory path
     * @param boolean  $keepParent  Only remove directory contents when true.
     */
    public static function rrmdir( $dir, $keepParent = false )
    {
        if ( $dir != '' && is_dir($dir) ) {

            foreach ( glob("{$dir}/*") as $file ) {

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

    public static function writeToTestLog($msg)
    {
        file_put_contents('/tmp/test.log', "{$msg}\n", FILE_APPEND);
    }

}
