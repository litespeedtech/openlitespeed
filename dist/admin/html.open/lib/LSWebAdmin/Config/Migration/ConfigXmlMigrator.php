<?php

namespace LSWebAdmin\Config\Migration;

use LSWebAdmin\Product\Current\DPageDef;
use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Util\PathTool;
use LSWebAdmin\Config\IO\AtomicFileWriter;
use LSWebAdmin\Config\CNode;
use LSWebAdmin\Config\IO\ConfigDataWriter;
use LSWebAdmin\Product\Current\ConfigData;

class ConfigXmlMigrator
{
    public static function convertXmlRootToConfRoot($xmlroot, $filemap)
    {
        $root = $xmlroot->DupHolder();
        $filemap->Convert(0, $xmlroot, 1, $root);
        return $root;
    }

    public static function writeMigratedConfRoot($type, $root, $filepath, $permissionSource)
    {
        $buf = '';
        ConfigRootLifecycle::beforeWriteConf($type, $root);
        $root->PrintBuf($buf);
        touch($filepath);

        AtomicFileWriter::write($filepath, $buf);
        if ($permissionSource !== null) {
            AtomicFileWriter::copyPermission($permissionSource, $filepath);
        }
    }

    public static function archiveSourceXml($xmlpath)
    {
        $migrated = $xmlpath . '.migrated.' . time();
        if (defined('SAVE_XML')) {
            copy($xmlpath, $migrated);
        } else {
            rename($xmlpath, $migrated);
        }

        if (defined('RECOVER_SCRIPT')) {
            file_put_contents(RECOVER_SCRIPT, "mv $migrated $xmlpath\n", FILE_APPEND);
        }

        return $migrated;
    }

    public static function migrateLinkedXmlChildrenToConf($root)
    {
        self::migrateLinkedVhostsToConf($root);
        self::migrateLinkedTemplatesToConf($root);
    }

    public static function saveLinkedConfChildrenToXml($root)
    {
        self::saveLinkedVhostsToXml($root);
        self::saveLinkedTemplatesToXml($root);
    }

    private static function migrateLinkedVhostsToConf($root)
    {
        $vhosts = self::normalizeChildren($root->GetChildren('virtualhost'));
        foreach ($vhosts as $vh) {
            $vhname = $vh->Get(CNode::FLD_VAL);
            $vhroot = $vh->GetChildVal('vhRoot');
            $vhconf = $vh->GetChildVal('configFile');
            $conffile = PathTool::GetAbsFile($vhconf, 'VR', $vhname, $vhroot);
            new ConfigData(DInfo::CT_VH, $conffile);
            if (self::endsWith($vhconf, '.xml')) {
                $vh->SetChildVal('configFile', substr($vhconf, 0, -4) . '.conf');
            }
        }
    }

    private static function migrateLinkedTemplatesToConf($root)
    {
        $templates = self::normalizeChildren($root->GetChildren('vhTemplate'));
        foreach ($templates as $template) {
            $tpconf = $template->GetChildVal('templateFile');
            $conffile = PathTool::GetAbsFile($tpconf, 'SR');
            new ConfigData(DInfo::CT_TP, $conffile);
            if (self::endsWith($tpconf, '.xml')) {
                $template->SetChildVal('templateFile', substr($tpconf, 0, -4) . '.conf');
            }
        }
    }

    private static function saveLinkedVhostsToXml($root)
    {
        $vhosts = self::normalizeChildren($root->GetChildren('virtualhost'));
        $filemap = DPageDef::GetInstance()->GetFileMap(DInfo::CT_VH);
        foreach ($vhosts as $vh) {
            $vhname = $vh->Get(CNode::FLD_VAL);
            $vhroot = $vh->GetChildVal('vhRoot');
            $vhconf = $vh->GetChildVal('configFile');
            $conffile = PathTool::GetAbsFile($vhconf, 'VR', $vhname, $vhroot);
            $vhdata = new ConfigData(DInfo::CT_VH, $conffile);
            ConfigDataWriter::saveXmlRoot(
                DInfo::CT_VH,
                $vhdata->GetRootNode(),
                $filemap,
                $vhdata->GetXmlPath(),
                $vhdata->GetPath()
            );
            AtomicFileWriter::copyPermission($vhdata->GetPath(), $vhdata->GetXmlPath());
            error_log("  converted " . $vhdata->GetPath() . " to " . $vhdata->GetXmlPath() . "\n");
        }
    }

    private static function saveLinkedTemplatesToXml($root)
    {
        $templates = self::normalizeChildren($root->GetChildren('vhTemplate'));
        $filemap = DPageDef::GetInstance()->GetFileMap(DInfo::CT_TP);
        foreach ($templates as $template) {
            $tpconf = $template->GetChildVal('templateFile');
            $conffile = PathTool::GetAbsFile($tpconf, 'SR');
            $tpdata = new ConfigData(DInfo::CT_TP, $conffile);
            ConfigDataWriter::saveXmlRoot(
                DInfo::CT_TP,
                $tpdata->GetRootNode(),
                $filemap,
                $tpdata->GetXmlPath(),
                $tpdata->GetPath()
            );
            AtomicFileWriter::copyPermission($tpdata->GetPath(), $tpdata->GetXmlPath());
            error_log("  converted " . $tpdata->GetPath() . " to " . $tpdata->GetXmlPath() . "\n");
        }
    }

    private static function normalizeChildren($nodes)
    {
        if ($nodes == null) {
            return [];
        }

        if (is_array($nodes)) {
            return $nodes;
        }

        return [$nodes];
    }

    private static function endsWith($value, $suffix)
    {
        $suffixLength = strlen($suffix);
        if ($suffixLength === 0) {
            return true;
        }

        if (!is_string($value) || strlen($value) < $suffixLength) {
            return false;
        }

        return substr_compare($value, $suffix, -$suffixLength, $suffixLength) === 0;
    }
}
