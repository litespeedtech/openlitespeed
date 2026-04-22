<?php

namespace LSWebAdmin\Config\Migration;

use LSWebAdmin\Config\IO\ConfigDataLoader;
use LSWebAdmin\Config\Parser\LsXmlParser;
use LSWebAdmin\Product\Current\DPageDef;
use LSWebAdmin\UI\DInfo;

class ConfigLegacyFormatConverter
{
    public static function migrateXmlToConf($type, $xmlPath, $confPath, &$conferr)
    {
        error_log("Migrating $xmlPath \n");

        $xmlroot = self::loadXmlRoot($xmlPath, $conferr);
        if ($xmlroot === false) {
            return false;
        }

        $filemap = DPageDef::GetInstance()->GetFileMap($type);
        $root = ConfigXmlMigrator::convertXmlRootToConfRoot($xmlroot, $filemap);
        ConfigXmlMigrator::writeMigratedConfRoot($type, $root, $confPath, $xmlPath);
        ConfigXmlMigrator::archiveSourceXml($xmlPath);
        error_log("  converted $xmlPath to $confPath\n\n");

        return true;
    }

    public static function migrateServerXmlTreeToConf($xmlPath, $confPath, &$conferr)
    {
        error_log("Migrating all config from server xml config $xmlPath \n");

        $xmlroot = self::loadXmlRoot($xmlPath, $conferr);
        if ($xmlroot === false) {
            return false;
        }

        $filemap = DPageDef::GetInstance()->GetFileMap(DInfo::CT_SERV);
        $root = ConfigXmlMigrator::convertXmlRootToConfRoot($xmlroot, $filemap);
        ConfigXmlMigrator::migrateLinkedXmlChildrenToConf($root);
        ConfigXmlMigrator::writeMigratedConfRoot(DInfo::CT_SERV, $root, $confPath, $xmlPath);
        ConfigXmlMigrator::archiveSourceXml($xmlPath);
        error_log("  converted $xmlPath to $confPath\n\n");

        return true;
    }

    private static function loadXmlRoot($xmlPath, &$conferr)
    {
        $xmlparser = new LsXmlParser();
        return ConfigDataLoader::loadXmlRoot($xmlparser, $xmlPath, $conferr);
    }
}
