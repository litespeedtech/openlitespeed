<?php

class PathTool
{

    public static function getAbsolutePath($root, $path)
    {
        if (substr($path, -1) != '/')
            $path .= '/';

        $newPath = $this->getAbsoluteFile($root, $path);
        return $newPath;
    }

    public static function getAbsoluteFile($root, $path)
    {
        if (substr($path, 0, 1) != '/')
            $path = $root . '/' . $path;

        $newPath = $this->clean($path);
        return $newPath;
    }

    public static function hasSymbolLink($path)
    {
        if ($path != realpath($path))
            return true;
        else
            return false;
    }

    public static function clean($path)
    {
        do {
            $newS1 = $path;
            $newS = str_replace('//', '/', $path);
            $path = $newS;
        } while ($newS != $newS1);

        do {
            $newS1 = $path;
            $newS = str_replace('/./', '/', $path);
            $path = $newS;
        } while ($newS != $newS1);

        do {
            $newS1 = $path;
            $newS = preg_replace('/\/[^\/^\.]+\/\.\.\//', '/', $path);
            $path = $newS;
        } while ($newS != $newS1);

        return $path;
    }

    public static function createFile($path, &$err, $attrName='')
    {
        if (file_exists($path)) {
            $err = is_file($path) ? "Already exists $path" : "name conflicting with an existing directory $path";
            return false;
        }
        $dir = substr($path, 0, (strrpos($path, '/')));
        $special_note = '';
        $dirmode = 0700; // default
        $filemode = 0600;
        $specials = ['userDB:location', 'groupDB:location'];
        if (in_array($attrName, $specials)) {
            $special_note = 'WebAdmin user does not have permission to create this file. You can manually create it and populate the data. Make sure it is readable by the user that web server is running as (usually nobody).';
            $dirmode = 0755; 
            $filemode = 0644;
        }
        
        if (!PathTool::createDir($dir, $dirmode, $err)) {
            $err .= '. ' . $special_note;
            return false;
        }

        if (touch($path)) {
            chmod($path, $filemode);
            return true;
        }
        $err = 'failed to create file ' . $path . '. ' . $special_note;
        return false;
    }

    public static function createDir($path, $mode, &$err)
    {
        if (file_exists($path)) {
            if (is_dir($path))
                return true;
            else {
                $err = "$path is not a directory";
                return false;
            }
        }
        $parent = substr($path, 0, (strrpos($path, '/')));
        if (strlen($parent) <= 1) {
            $err = "invalid path: $path";
            return false;
        }
        if (!file_exists($parent) && !PathTool::createDir($parent, $mode, $err))
            return false;

        if (mkdir($path, $mode))
            return true;
        else {
            $err = "fail to create directory $path";
            return false;
        }
    }

    public static function isDenied($path)
    {
        $absname = realpath($path);
        if (strncmp($absname, '/etc/', 5) == 0)
            return true;
        else
            return false;
    }

    public static function GetAbsFile($filename, $type, $vhname = '', $vhroot = '')
    {
        // type = 'SR', 'VR'
        if (strpos($filename, '$VH_NAME') !== false)
            $filename = str_replace('$VH_NAME', $vhname, $filename);

        if ($filename[0] == '$') {
            if (strncasecmp('$SERVER_ROOT', $filename, 12) == 0) {
                $filename = SERVER_ROOT . substr($filename, 13);
            } elseif ($type == 'VR' && strncasecmp('$VH_ROOT', $filename, 8) == 0) {
                $vhrootf = PathTool::GetAbsFile($vhroot, 'SR', $vhname);
                if (substr($vhrootf, -1, 1) !== '/')
                    $vhrootf .= '/';
                $filename = $vhrootf . substr($filename, 9);
            }
        }elseif ($filename[0] == '/') {
            if (isset($_SERVER['LS_CHROOT'])) {
                $root = $_SERVER['LS_CHROOT'];
                $len = strlen($root);
                if (strncmp($filename, $root, $len) == 0)
                    $filename = substr($filename, $len);
            }
        } else { // treat relative path to SERVER_ROOT
            $filename = SERVER_ROOT . $filename;
        }
        return $filename;
    }

    public static function IsCyberPanel()
    {
        if (!defined('CYBERP')) {
            if (file_exists('/usr/local/CyberCP')) {
                define('CYBERP', 1);
            } else {
                define('CYBERP', 0);
            }
        }
        return CYBERP;
    }

}
