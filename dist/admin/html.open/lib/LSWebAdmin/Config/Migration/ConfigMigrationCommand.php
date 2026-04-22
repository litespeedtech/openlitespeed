<?php

namespace LSWebAdmin\Config\Migration;

use LSWebAdmin\Product\Current\DPageDef;
use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Config\IO\AtomicFileWriter;
use LSWebAdmin\Config\IO\ConfigDataWriter;
use LSWebAdmin\Product\Current\ConfigData;

class ConfigMigrationCommand
{
    public static function migrateAllConfToXml($serverRoot)
    {
        $serverRoot = self::normalizeServerRoot($serverRoot);
        error_log("Migrate plain conf to xml from $serverRoot\n");

        define('SERVER_ROOT', $serverRoot);

        $servdata = new ConfigData(DInfo::CT_SERV, SERVER_ROOT . 'conf/httpd_config.conf');
        ConfigXmlMigrator::saveLinkedConfChildrenToXml($servdata->GetRootNode());

        $filemap = DPageDef::GetInstance()->GetFileMap(DInfo::CT_SERV);
        ConfigDataWriter::saveXmlRoot(
            DInfo::CT_SERV,
            $servdata->GetRootNode(),
            $filemap,
            $servdata->GetXmlPath(),
            $servdata->GetPath()
        );
        AtomicFileWriter::copyPermission($servdata->GetPath(), $servdata->GetXmlPath());
        error_log("  converted " . $servdata->GetPath() . " to " . $servdata->GetXmlPath() . "\n");

        $admindata = new ConfigData(DInfo::CT_ADMIN, SERVER_ROOT . 'admin/conf/admin_config.conf');
        $filemap = DPageDef::GetInstance()->GetFileMap(DInfo::CT_ADMIN);
        ConfigDataWriter::saveXmlRoot(
            DInfo::CT_ADMIN,
            $admindata->GetRootNode(),
            $filemap,
            $admindata->GetXmlPath(),
            $admindata->GetPath()
        );
        AtomicFileWriter::copyPermission($admindata->GetPath(), $admindata->GetXmlPath());
        error_log("Migration done.\n");
    }

    public static function migrateAllXmlToConf($serverRoot, $recoverScript, $removeXml)
    {
        $serverRoot = self::normalizeServerRoot($serverRoot);
        self::validateRecoverScript($recoverScript);
        self::validateRemoveXml($removeXml);

        error_log("Migrate xml to plain conf under server root $serverRoot\n");
        define('SERVER_ROOT', $serverRoot);

        $servconf = SERVER_ROOT . 'conf/httpd_config.xml';
        if (!file_exists($servconf)) {
            trigger_error("Cannot find xml config file $servconf under server root $serverRoot", E_USER_ERROR);
        }

        self::initializeRecoverScript($recoverScript);
        if ($removeXml == 0)
            define('SAVE_XML', 1);

        new ConfigData(DInfo::CT_SERV, $servconf);
        new ConfigData(DInfo::CT_ADMIN, SERVER_ROOT . 'admin/conf/admin_config.xml');

        if (defined('RECOVER_SCRIPT')) {
            chmod(RECOVER_SCRIPT, 0700);
            error_log("You can recover the migrated xml configuration files from this script " . RECOVER_SCRIPT . "\n");
        }

        error_log("Migration done.\n");
    }

    private static function normalizeServerRoot($serverRoot)
    {
        if ($serverRoot == '' || trim($serverRoot) == '')
            trigger_error('ConfigMigrationCommand: Require SERVER_ROOT as input param', E_USER_ERROR);

        if (substr($serverRoot, -1) != '/')
            $serverRoot .= '/';

        return $serverRoot;
    }

    private static function validateRecoverScript($recoverScript)
    {
        if ($recoverScript == '' || trim($recoverScript) == '')
            trigger_error('ConfigMigrationCommand: Require recover script as input param', E_USER_ERROR);
    }

    private static function validateRemoveXml($removeXml)
    {
        if ($removeXml != 1 && $removeXml != 0)
            trigger_error('ConfigMigrationCommand: Require removexml as input param with value 1 or 0', E_USER_ERROR);
    }

    private static function initializeRecoverScript($recoverScript)
    {
        $timestamp = date(DATE_RFC2822);
        $script = "#!/bin/sh

########################################################################################
#  xml configuration files (.xml) were migrated to plain configuration (.conf) on $timestamp
#  If you need to revert back to older versions that based on xml configuration, please manually
#  run this script to restore the original files.
######################################################################################## \n\n";

        if (file_put_contents($recoverScript, $script) === false) {
            trigger_error("ConfigMigrationCommand: Failed to write to recover script $recoverScript, abort", E_USER_ERROR);
        }

        define('RECOVER_SCRIPT', $recoverScript);
    }
}
