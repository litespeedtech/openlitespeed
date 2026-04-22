<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\Config\CData;
use LSWebAdmin\Config\CNode;
use LSWebAdmin\Config\IO\ConfigDataLoader;
use LSWebAdmin\Config\IO\ConfigDataWriter;
use LSWebAdmin\Config\IO\ConfigFormatPathPair;
use LSWebAdmin\Config\IO\ConfigWritePlan;
use LSWebAdmin\Config\IO\ConfigWriteTask;
use LSWebAdmin\Config\Migration\ConfigLegacyFormatConverter;
use LSWebAdmin\Config\Parser\PlainConfParser;
use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Util\PathTool;

class ConfigData extends CData
{
    public function __construct($type, $path, $id = null, $routeState = null)
    {
        $this->_type = $type;
        $this->_id = $id;
        $isnew = (is_string($id) && isset($id[0]) && $id[0] === '`');

        if ($type == DInfo::CT_EX) {
            $this->_path = $path;
            $this->init_special();
        } else {
            $pathPair = ConfigFormatPathPair::fromPath($path);
            $this->_path = $pathPair->GetConfPath();
            $this->_xmlpath = $pathPair->GetXmlPath();
            $this->init($isnew);
        }
    }

    public function GetWritePlan()
    {
        $filemap = DPageDef::GetInstance()->GetFileMap($this->_type);
        $plan = new ConfigWritePlan();
        $plan->AddTask(ConfigWriteTask::saveRoot(
            ConfigDataWriter::FORMAT_CONF,
            $this->_type,
            $this->_root,
            $filemap,
            $this->_path,
            $this->_path
        ));

        if (defined('SAVE_XML')) {
            $plan->AddTask(ConfigWriteTask::saveRootFromPreviousRoot(
                ConfigDataWriter::FORMAT_XML,
                $this->_type,
                $filemap,
                $this->_xmlpath,
                $this->_path
            ));
        }

        return $plan;
    }

    protected function init($isnew)
    {
        if ($isnew) {
            if (!file_exists($this->_path) && !PathTool::createFile($this->_path, $err)) {
                $this->_conferr = 'Failed to create config file at ' . $this->_path;
                return false;
            }

            $this->_root = new CNode(CNode::K_ROOT, $this->_path, CNode::T_ROOT);
            return true;
        }

        if (!file_exists($this->_path) || filesize($this->_path) < 10) {
            if ($this->_type == DInfo::CT_SERV) {
                if (file_exists($this->_xmlpath) && !$this->migrate_allxml2conf()) {
                    return false;
                }
                $this->_conferr = 'Failed to find config file at ' . $this->_path;
                return false;
            }

            if (file_exists($this->_xmlpath)) {
                if (!$this->migrate_xml2conf()) {
                    return false;
                }
            } else {
                $this->_root = new CNode(CNode::K_ROOT, $this->_path, CNode::T_ROOT);
                return true;
            }
        }

        $parser = new PlainConfParser();
        $this->_root = ConfigDataLoader::loadPlainRoot($parser, $this->_path, $this->_conferr);
        if ($this->_root === false) {
            return false;
        }

        $this->after_read();
        return true;
    }

    protected function migrate_xml2conf()
    {
        return ConfigLegacyFormatConverter::migrateXmlToConf(
            $this->_type,
            $this->_xmlpath,
            $this->_path,
            $this->_conferr
        );
    }

    protected function migrate_allxml2conf()
    {
        return ConfigLegacyFormatConverter::migrateServerXmlTreeToConf(
            $this->_xmlpath,
            $this->_path,
            $this->_conferr
        );
    }
}
