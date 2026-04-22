<?php

namespace LSWebAdmin\Config;

use LSWebAdmin\Config\IO\ConfigDataLoader;
use LSWebAdmin\Config\IO\ConfigDataWriter;
use LSWebAdmin\Config\IO\ConfigFormatPathPair;
use LSWebAdmin\Config\IO\ConfigWritePlan;
use LSWebAdmin\Config\IO\ConfigWriteTask;
use LSWebAdmin\Config\Migration\ConfigXmlMigrator;
use LSWebAdmin\Config\Parser\LsXmlParser;
use LSWebAdmin\Product\Current\DPageDef;
use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Util\PathTool;

class XmlConfigData extends CData
{
    public function __construct($type, $path, $id = null, $routeState = null)
    {
        $this->_type = $type;
        $this->_id = $id;
        $isnew = (is_string($id) && isset($id[0]) && $id[0] === '`');

        if ($type == DInfo::CT_EX) {
            $this->_path = $path;
            $this->init_special();
            return;
        }

        $this->initXmlPaths($path);
        $this->initXml($isnew);
    }

    public function GetWritePlan()
    {
        $filemap = DPageDef::GetInstance()->GetFileMap($this->_type);
        $plan = new ConfigWritePlan();
        $plan->AddTask(ConfigWriteTask::saveRoot(
            ConfigDataWriter::FORMAT_XML,
            $this->_type,
            $this->_root,
            $filemap,
            $this->_xmlpath,
            $this->_xmlpath
        ));

        return $plan;
    }

    protected function initXmlPaths($path)
    {
        $pathPair = ConfigFormatPathPair::fromPath($path);
        $this->_xmlpath = $pathPair->GetXmlPath();

        $this->_path = $this->_xmlpath;
    }

    protected function initXml($isnew)
    {
        if ($isnew) {
            if (!file_exists($this->_xmlpath) && !PathTool::createFile($this->_xmlpath, $err)) {
                $this->_conferr = 'Failed to create config file at ' . $this->_xmlpath;
                return false;
            }

            $this->_root = new CNode(CNode::K_ROOT, $this->_path, CNode::T_ROOT);
            return true;
        }

        if (!file_exists($this->_xmlpath) || filesize($this->_xmlpath) < 10) {
            if ($this->_type == DInfo::CT_SERV || $this->_type == DInfo::CT_ADMIN) {
                $this->_conferr = 'Failed to find config file at ' . $this->_xmlpath;
                return false;
            }

            // Treat missing vhost/template XML as a new linked config.
            $this->_root = new CNode(CNode::K_ROOT, $this->_path, CNode::T_ROOT);
            return true;
        }

        $xmlparser = new LsXmlParser();
        $xmlroot = ConfigDataLoader::loadXmlRoot($xmlparser, $this->_xmlpath, $this->_conferr);
        if ($xmlroot === false) {
            return false;
        }

        $filemap = DPageDef::GetInstance()->GetFileMap($this->_type);
        $this->_root = ConfigXmlMigrator::convertXmlRootToConfRoot($xmlroot, $filemap);
        $this->_root->SetVal($this->_path);
        $this->_root->Set(CNode::FLD_TYPE, CNode::T_ROOT);
        $this->after_read();

        return true;
    }
}
