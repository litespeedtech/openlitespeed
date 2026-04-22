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

        $fd = fopen("{$filepath}.new", 'w');
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

        if ($permissionSource !== null && file_exists($permissionSource)) {
            self::copyPermission($permissionSource, "{$filepath}.new");
        }

        @unlink("{$filepath}.bak");
        if (file_exists($filepath) && !rename($filepath, "{$filepath}.bak")) {
            error_log("failed to rename {$filepath} to {$filepath}.bak");
            return false;
        }

        if (!rename("{$filepath}.new", $filepath)) {
            error_log("failed to rename {$filepath}.new to {$filepath}");
            return false;
        }

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
