<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\Product\Current\DTblDef;
use LSWebAdmin\UI\DTbl;

class DTblSelectorMap
{
    private $_tid;
    private $_overrides;
    private $_includeSelector;
    private $_includeTids;

    public function __construct($tid, $overrides = [], $includeSelector = false, $includeTids = null)
    {
        $this->_tid = $tid;
        $this->_overrides = is_array($overrides) ? $overrides : [];
        $this->_includeSelector = (bool) $includeSelector;
        $this->_includeTids = ($includeTids == null) ? null : array_flip((array) $includeTids);
    }

    public function Resolve($node)
    {
        $maps = $this->_includeSelector ? [$this->_tid] : [];

        if (!($node instanceof CNode)) {
            return $maps;
        }

        $maps = array_merge($maps, $this->GetSelectedMaps($node));

        return DTblMap::ResolveMaps($maps, $node);
    }

    public function GetTid()
    {
        return $this->_tid;
    }

    public function ShouldIncludeSelector()
    {
        return $this->_includeSelector;
    }

    public function GetSelectedMaps($node)
    {
        $maps = [];

        if (!($node instanceof CNode)) {
            return $maps;
        }

        $tbl = DTblDef::GetInstance()->GetTblDef($this->_tid);
        $subtid = $tbl->GetSubTid($node);
        if ($subtid == null) {
            return $maps;
        }

        foreach ((array) $subtid as $tid) {
            if ($this->_includeTids !== null && !isset($this->_includeTids[$tid])) {
                continue;
            }
            $maps[] = isset($this->_overrides[$tid]) ? $this->_overrides[$tid] : $tid;
        }

        return $maps;
    }

    public function GetStaticMaps()
    {
        $maps = $this->_includeSelector ? [$this->_tid] : [];
        $tbl = DTblDef::GetInstance()->GetTblDef($this->_tid);
        $subtbls = $tbl->Get(DTbl::FLD_SUBTBLS);

        if (is_array($subtbls)) {
            foreach ($subtbls as $key => $subtid) {
                if ($key === 0 || $subtid == null) {
                    continue;
                }
                foreach ((array) $subtid as $tid) {
                    if ($this->_includeTids !== null && !isset($this->_includeTids[$tid])) {
                        continue;
                    }
                    $maps[] = isset($this->_overrides[$tid]) ? $this->_overrides[$tid] : $tid;
                }
            }
        }

        return DTblMap::ResolveMaps($maps, null);
    }
}
