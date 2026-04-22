<?php

namespace LSWebAdmin\Config\IO;

use LSWebAdmin\Config\Migration\ConfigRootLifecycle;

class ConfigDataWriter
{
    const FORMAT_CONF = 'conf';
    const FORMAT_XML = 'xml';

    public static function extractMappedConfRoot($root, $filemap)
    {
        $sourceRoot = $root->DeepCopy();
        $convertedroot = $root->DupHolder();
        $filemap->Convert(1, $sourceRoot, 1, $convertedroot);

        return $convertedroot;
    }

    public static function extractMappedXmlRoot($type, $root, $filemap)
    {
        $sourceRoot = $root->DeepCopy();
        ConfigRootLifecycle::beforeWriteXml($type, $sourceRoot);

        $xmlroot = $root->DupHolder();
        $filemap->Convert(1, $sourceRoot, 0, $xmlroot);

        return $xmlroot;
    }

    public static function saveMappedRoot($format, $type, $root, $filemap, $filepath, $permissionSource)
    {
        if ($format === self::FORMAT_CONF) {
            return self::saveConfRoot($type, $root, $filemap, $filepath, $permissionSource);
        }

        if ($format === self::FORMAT_XML) {
            return self::saveXmlRoot($type, $root, $filemap, $filepath, $permissionSource);
        }

        trigger_error('Unsupported config write format ' . $format, E_USER_ERROR);
    }

    public static function writeRoot($format, $type, $root, $filepath, $permissionSource)
    {
        if ($format === self::FORMAT_CONF) {
            return self::writeConfRoot($type, $root, $filepath, $permissionSource);
        }

        if ($format === self::FORMAT_XML) {
            return self::writeXmlRoot($type, $root, $filepath, $permissionSource);
        }

        trigger_error('Unsupported config write format ' . $format, E_USER_ERROR);
    }

    public static function writeConfRoot($type, $root, $filepath, $permissionSource)
    {
        $confbuf = '';
        ConfigRootLifecycle::beforeWriteConf($type, $root);
        $root->PrintBuf($confbuf);
        if (!defined('_CONF_READONLY_')) {
            AtomicFileWriter::write($filepath, $confbuf, $permissionSource);
        }

        return $root;
    }

    public static function saveConfRoot($type, $root, $filemap, $filepath, $permissionSource)
    {
        $convertedroot = self::extractMappedConfRoot($root, $filemap);
        self::writeConfRoot($type, $convertedroot, $filepath, $permissionSource);

        return $convertedroot;
    }

    public static function writeXmlRoot($type, $root, $filepath, $permissionSource)
    {
        $xmlbuf = '';
        $root->PrintXmlBuf($xmlbuf);
        AtomicFileWriter::write($filepath, $xmlbuf, $permissionSource);

        return $root;
    }

    public static function saveXmlRoot($type, $root, $filemap, $filepath, $permissionSource)
    {
        $xmlroot = self::extractMappedXmlRoot($type, $root, $filemap);
        self::writeXmlRoot($type, $xmlroot, $filepath, $permissionSource);

        return $xmlroot;
    }
}
