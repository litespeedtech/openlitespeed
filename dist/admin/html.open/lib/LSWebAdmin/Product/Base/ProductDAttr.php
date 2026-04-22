<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\UI\DAttrBase;

class ProductDAttr extends DAttrBase
{
    public $_version = 0;
    public $_feature = 0;

    public function dup($key, $label, $helpkey)
    {
        $d = parent::dup($key, $label, $helpkey);
        $d->_version = $this->_version;
        $d->_feature = $this->_feature;
        return $d;
    }

    public function blockedReason()
    {
        return '';
    }

    public function blockedVersion()
    {
        return ($this->blockedReason() !== '');
    }

    public function bypassSavePost()
    {
        return ($this->IsFlagOn(self::BM_NOEDIT) || $this->blockedReason() !== '');
    }
}
