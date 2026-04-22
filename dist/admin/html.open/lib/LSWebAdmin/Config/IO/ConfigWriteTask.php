<?php

namespace LSWebAdmin\Config\IO;

class ConfigWriteTask
{
    const MODE_SAVE_ROOT = 'save_root';
    const MODE_WRITE_ROOT = 'write_root';
    const MODE_SAVE_CONF_ROOT = self::MODE_SAVE_ROOT;
    const MODE_SAVE_XML_ROOT = self::MODE_SAVE_ROOT;
    const MODE_WRITE_CONF_ROOT = self::MODE_WRITE_ROOT;

    private $_mode;
    private $_format;
    private $_type;
    private $_root;
    private $_usePreviousRoot;
    private $_filemap;
    private $_filepath;
    private $_permissionSource;

    private function __construct($mode, $format, $type, $root, $usePreviousRoot, $filemap, $filepath, $permissionSource)
    {
        $this->_mode = $mode;
        $this->_format = $format;
        $this->_type = $type;
        $this->_root = $root;
        $this->_usePreviousRoot = $usePreviousRoot;
        $this->_filemap = $filemap;
        $this->_filepath = $filepath;
        $this->_permissionSource = $permissionSource;
    }

    public static function saveRoot($format, $type, $root, $filemap, $filepath, $permissionSource)
    {
        return new self(self::MODE_SAVE_ROOT, $format, $type, $root, false, $filemap, $filepath, $permissionSource);
    }

    public static function saveRootFromPreviousRoot($format, $type, $filemap, $filepath, $permissionSource)
    {
        return new self(self::MODE_SAVE_ROOT, $format, $type, null, true, $filemap, $filepath, $permissionSource);
    }

    public static function writeRoot($format, $type, $root, $filepath, $permissionSource)
    {
        return new self(self::MODE_WRITE_ROOT, $format, $type, $root, false, null, $filepath, $permissionSource);
    }

    public static function saveConfRoot($type, $root, $filemap, $filepath, $permissionSource)
    {
        return self::saveRoot(ConfigDataWriter::FORMAT_CONF, $type, $root, $filemap, $filepath, $permissionSource);
    }

    public static function saveXmlFromPreviousRoot($type, $filemap, $filepath, $permissionSource)
    {
        return self::saveRootFromPreviousRoot(ConfigDataWriter::FORMAT_XML, $type, $filemap, $filepath, $permissionSource);
    }

    public static function writeConfRoot($type, $root, $filepath, $permissionSource)
    {
        return self::writeRoot(ConfigDataWriter::FORMAT_CONF, $type, $root, $filepath, $permissionSource);
    }

    public function GetMode()
    {
        return $this->_mode;
    }

    public function GetFormat()
    {
        return $this->_format;
    }

    public function GetType()
    {
        return $this->_type;
    }

    public function GetRoot()
    {
        return $this->_root;
    }

    public function UsesPreviousRoot()
    {
        return $this->_usePreviousRoot;
    }

    public function GetFileMap()
    {
        return $this->_filemap;
    }

    public function GetFilePath()
    {
        return $this->_filepath;
    }

    public function GetPermissionSource()
    {
        return $this->_permissionSource;
    }
}
