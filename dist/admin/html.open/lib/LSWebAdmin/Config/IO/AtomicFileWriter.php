<?php

namespace LSWebAdmin\Config\IO;

use LSWebAdmin\Util\PathTool;

class AtomicFileWriter
{
    public static function write($filepath, $buf, $permissionSource = null)
    {
        if (!file_exists($filepath)) {
            if (!PathTool::createFile("{$filepath}.new", $err)) {
                error_log("failed to create file $filepath : $err \n");
                return false;
            }
        }

        // Restrict umask so the fresh .new file is created with 0600
        // permissions, avoiding a brief window where a world-readable
        // .new file could expose freshly-written secrets (htpasswd
        // hashes, config values) before copyPermission() runs.
        $previousUmask = umask(0077);
        $fd = fopen("{$filepath}.new", 'w');
        umask($previousUmask);
        if (!$fd) {
            error_log("failed to open in write mode for {$filepath}.new");
            return false;
        }

        if (fwrite($fd, $buf) === false) {
            fclose($fd);
            error_log("failed to write temp config for {$filepath}.new");
            return false;
        }
        fclose($fd);

        $permissionSourcePath = $permissionSource;
        if ($permissionSourcePath === null && file_exists($filepath)) {
            $permissionSourcePath = $filepath;
        }

        if ($permissionSourcePath !== null && file_exists($permissionSourcePath)) {
            self::copyPermission($permissionSourcePath, "{$filepath}.new");
        }

        if (file_exists($filepath) && !ConfigAutoBackupService::backupExistingConfig($filepath)) {
            return false;
        }

        if (!rename("{$filepath}.new", $filepath)) {
            error_log("failed to rename {$filepath}.new to {$filepath}");
            return false;
        }

        ConfigAutoBackupService::pruneExpiredBackups();

        return true;
    }

    public static function copyPermission($fromfile, $tofile)
    {
        $owner = fileowner($fromfile);
        if (fileowner($tofile) != $owner) {
            chown($tofile, $owner);
        }
        $perm = fileperms($fromfile);
        if (fileperms($tofile) != $perm) {
            chmod($tofile, $perm);
        }
    }
}
