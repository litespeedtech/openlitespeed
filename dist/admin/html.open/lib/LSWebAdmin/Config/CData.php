<?php

namespace LSWebAdmin\Config;

use LSWebAdmin\Config\IO\ConfigDataLoader;
use LSWebAdmin\Config\IO\ConfigWriteService;
use LSWebAdmin\Config\Service\ConfigMutationService;
use LSWebAdmin\Config\Migration\ConfigRootLifecycle;
use LSWebAdmin\Product\Current\DPageDef;
use LSWebAdmin\UI\DInfo;

abstract class CData
{
    protected $_type; //{'serv','admin','vh','tp','special'}
    protected $_id;
    protected $_root;
    protected $_path;
    protected $_xmlpath;
    protected $_conferr;

    public function GetRootNode()
    {
        return $this->_root;
    }

    public function GetId()
    {
        return $this->_id;
    }

    public function GetType()
    {
        return $this->_type;
    }

    public function GetPath()
    {
        return $this->_path;
    }

    public function GetXmlPath()
    {
        return $this->_xmlpath;
    }

    public function GetConfErr()
    {
        return $this->_conferr;
    }

    public function GetChildrenValues($location, $ref = '')
    {
        $vals = [];
        $layer = $this->_root->GetChildrenByLoc($location, $ref);
        if ($layer != null) {
            if (is_array($layer)) {
                $vals = array_map('strval', array_keys($layer));
            } else {
                $vals[] = $layer->Get(CNode::FLD_VAL);
            }
        }

        return $vals;
    }

    public function GetChildVal($location, $ref = '')
    {
        $layer = $this->_root->GetChildrenByLoc($location, $ref);
        if ($layer instanceof CNode) {
            return $layer->Get(CNode::FLD_VAL);
        }

        return null;
    }

    public function GetChildNodeById($key, $id)
    {
        return $this->_root->GetChildNodeById($key, $id);
    }

    public function SetRootNode($nd)
    {
        $this->_root = $nd;
        $this->_root->SetVal($this->_path);
        $this->_root->Set(CNode::FLD_TYPE, CNode::T_ROOT);
    }

    public function SavePost($extractData, $disp)
    {
        $result = ConfigMutationService::savePost($this->_type, $this->_root, $extractData, $disp, DPageDef::class);
        ConfigMutationService::applyIntegrity($result, $disp);
        $this->SaveFile();
    }

    public function ChangeContextSeq($seq)
    {
        if (($ctxs = $this->getContextNodes()) == null) {
            return false;
        }

        if (!is_array($ctxs) || $seq == -1 || $seq == count($ctxs)) {
            return false;
        }

        if ($seq > 0) {
            $index = $seq - 1;
            $switched = $seq;
        } else {
            $index = -$seq - 1;
            $switched = $index - 1;
        }

        $uris = array_keys($ctxs);
        $temp = $uris[$switched];
        $uris[$switched] = $uris[$index];
        $uris[$index] = $temp;

        return $this->applyContextOrder($ctxs, $uris);
    }

    public function ChangeContextOrder($orderedUris)
    {
        if (($ctxs = $this->getContextNodes()) == null || !is_array($ctxs) || !is_array($orderedUris)) {
            return false;
        }

        $expectedUris = array_keys($ctxs);
        if (count($orderedUris) != count($expectedUris)) {
            return false;
        }

        if ($orderedUris === $expectedUris) {
            return false;
        }

        $remaining = array_fill_keys($expectedUris, true);
        foreach ($orderedUris as $uri) {
            if (!is_string($uri) || $uri === '' || !array_key_exists($uri, $remaining)) {
                return false;
            }

            unset($remaining[$uri]);
        }

        if (!empty($remaining)) {
            return false;
        }

        return $this->applyContextOrder($ctxs, $orderedUris);
    }

    public function DeleteEntry($disp)
    {
        $result = ConfigMutationService::deleteEntry($this->_type, $this->_root, $disp, DPageDef::class);
        if ($result !== false) {
            ConfigMutationService::applyIntegrity($result, $disp);
            $this->SaveFile();
        }
    }

    public function SaveFile()
    {
        if ($this->_type == DInfo::CT_EX) {
            return $this->save_special();
        }

        ConfigWriteService::execute($this->GetWritePlan());
    }

    abstract public function GetWritePlan();

    protected function after_read()
    {
        $this->_id = ConfigRootLifecycle::afterRead($this->_type, $this->_root, $this->_id);
    }

    protected function init_special()
    {
        $root = ConfigDataLoader::loadSpecialRoot($this->_path, $this->_id);
        if ($root === false) {
            return false;
        }

        $this->_root = $root;
        return true;
    }

    protected function save_special()
    {
        return ConfigDataLoader::saveSpecialRoot($this->_path, $this->_id, $this->_root);
    }

    private function getContextNodes()
    {
        $loc = ($this->_type == DInfo::CT_VH) ? 'context' : 'virtualHostConfig:context';
        return $this->_root->GetChildren($loc);
    }

    private function applyContextOrder($ctxs, $uris)
    {
        $parent = null;
        foreach ($uris as $uri) {
            $ctx = $ctxs[$uri];
            if ($parent == null) {
                $parent = $ctx->Get(CNode::FLD_PARENT);
                $parent->RemoveChild('context');
            }

            $parent->AddChild($ctx);
        }

        $this->SaveFile();
        return true;
    }
}
