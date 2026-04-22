<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\Config\CNode;

class DTbl
{

    const FLD_INDEX = 1;
    const FLD_SHOWPARENTREF = 2;
    const FLD_LINKEDTBL = 3;
    const FLD_DEFAULTEXTRACT = 4;
    const FLD_DATTRS = 5;
    const FLD_ID = 6;
    const FLD_SUBTBLS = 7;

    private $_id;
    private $_dattrs;
    private $_helpKey;
    private $_cols = 0;
    private $_isTop = false;
    private $_holderIndex = null;
    private $_showParentRef = false;
    private $_subTbls = null;
    private $_defaultExtract = null;
    private $_linkedTbls = null;
    private $_width = '100%';
    private $_align;
    private $_title;
    private $_addTbl;
    private $_icon;
    private $_hasNote;
    private $_clientFilter = false;
    private $_clientFilterThreshold = 30;
    private $_clientTableControls = false;
    private $_clientTableControlsThreshold = 10;

    public static function NewRegular($id, $title, $attrs, $helpkey = null, $cols = null)
    {
        $tbl = new DTbl($id, $title, $attrs, $helpkey);
        $tbl->_cols = ($cols > 0) ? $cols : 2;
        return $tbl;
    }

    public static function NewIndexed($id, $title, $attrs, $index, $helpkey = null, $defaultExtract = null)
    {
        $tbl = new DTbl($id, $title, $attrs, $helpkey);
        $tbl->_holderIndex = $index;
        $tbl->_cols = 2;
        if ($defaultExtract) {
            $tbl->_defaultExtract = $defaultExtract;
        }
        return $tbl;
    }

    public static function NewTop($id, $title, $attrs, $index, $addTbl, $align = 0, $helpkey = null, $icon = null, $hasNote = false)
    {
        $tbl = new DTbl($id, $title, $attrs, $helpkey);
        $cols = count($attrs);
        $tbl->_holderIndex = $index;
        $tbl->_addTbl = $addTbl;
        $tbl->_align = $align;
        $tbl->_isTop = true;
        if ($icon != null) {
            $tbl->_icon = $icon;
        }
        if ($hasNote) {
            $cols++;
            $tbl->_hasNote = $hasNote;
        }
        $tbl->_cols = $cols;
        return $tbl;
    }

    public static function NewSel($id, $title, $attrs, $subtbls, $helpkey = null)
    {
        $tbl = new DTbl($id, $title, $attrs, $helpkey);
        $tbl->_subTbls = $subtbls;
        $tbl->_cols = 3;
        return $tbl;
    }

    private function __construct($id, $title, $attrs, $helpKey = null)
    {
        $this->_id = $id;
        $this->_title = $title;
        $this->_dattrs = $attrs;
        $this->_helpKey = $helpKey;
    }

    public function Dup($newId, $title = null)
    {
        $d = new DTbl($newId, (($title == null) ? $this->_title : $title), $this->_dattrs, $this->_helpKey);
        $d->_addTbl = $this->_addTbl;
        $d->_align = $this->_align;
        $d->_icon = $this->_icon;
        $d->_width = $this->_width;
        $d->_cols = $this->_cols;
        $d->_hasNote = $this->_hasNote;
        $d->_holderIndex = $this->_holderIndex;
        $d->_subTbls = $this->_subTbls;
        $d->_linkedTbls = $this->_linkedTbls;
        $d->_defaultExtract = $this->_defaultExtract;
        $d->_showParentRef = $this->_showParentRef;
        $d->_clientFilter = $this->_clientFilter;
        $d->_clientFilterThreshold = $this->_clientFilterThreshold;
        $d->_clientTableControls = $this->_clientTableControls;
        $d->_clientTableControlsThreshold = $this->_clientTableControlsThreshold;

        return $d;
    }

    public function Get($field)
    {
        switch ($field) {
            case self::FLD_ID:
                return $this->_id;
            case self::FLD_LINKEDTBL:
                return $this->_linkedTbls;
            case self::FLD_INDEX:
                return $this->_holderIndex;
            case self::FLD_DATTRS:
                return $this->_dattrs;
            case self::FLD_DEFAULTEXTRACT:
                return $this->_defaultExtract;
            case self::FLD_SUBTBLS:
                return $this->_subTbls;
        }
        trigger_error("DTbl::Get field $field not supported", E_USER_ERROR);
    }

    public function Set($field, $fieldval)
    {
        switch ($field) {
            case self::FLD_SHOWPARENTREF:
                $this->_showParentRef = $fieldval;
                break;
            case self::FLD_LINKEDTBL:
                $this->_linkedTbls = $fieldval;
                break;
            case self::FLD_DEFAULTEXTRACT:
                $this->_defaultExtract = $fieldval;
                break;
            case self::FLD_INDEX:
                $this->_holderIndex = $fieldval;
                break;
            default:
                trigger_error("DTbl::Set field $field not supported", E_USER_ERROR);
        }
    }

    public function ResetAttrEntry($index, $newAttr)
    {
        $this->_dattrs[$index] = $newAttr;
    }

    public function getId()
    {
        return $this->_id;
    }

    public function getAttrs()
    {
        return $this->_dattrs;
    }

    public function getHelpKey()
    {
        return $this->_helpKey;
    }

    public function getCols()
    {
        return $this->_cols;
    }

    public function isTop()
    {
        return $this->_isTop;
    }

    public function getHolderIndex()
    {
        return $this->_holderIndex;
    }

    public function shouldShowParentRef()
    {
        return $this->_showParentRef;
    }

    public function getSubTbls()
    {
        return $this->_subTbls;
    }

    public function getDefaultExtract()
    {
        return $this->_defaultExtract;
    }

    public function getLinkedTbls()
    {
        return $this->_linkedTbls;
    }

    public function getWidth()
    {
        return $this->_width;
    }

    public function getAlign()
    {
        return $this->_align;
    }

    public function getTitle()
    {
        return $this->_title;
    }

    public function getAddTbl()
    {
        return $this->_addTbl;
    }

    public function getIcon()
    {
        return $this->_icon;
    }

    public function enableClientFilter($enabled = true)
    {
        if (is_bool($enabled)) {
            $this->_clientFilter = $enabled;
            if ($enabled && $this->_clientFilterThreshold < 1) {
                $this->_clientFilterThreshold = 30;
            }
        } elseif (is_numeric($enabled)) {
            $this->_clientFilter = true;
            $this->_clientFilterThreshold = max(1, (int) $enabled);
        } else {
            $this->_clientFilter = (bool) $enabled;
        }
        return $this;
    }

    public function hasClientFilter()
    {
        return $this->_clientFilter;
    }

    public function getClientFilterThreshold()
    {
        return $this->_clientFilterThreshold;
    }

    /**
     * Enable client-side table controls (inline filter, rows-per-page dropdown, pagination).
     * These controls auto-show when the row count exceeds the given threshold.
     *
     * @param bool|int $enabled  Pass true (uses default threshold of 10), a numeric threshold, or false to disable.
     * @return $this  Fluent interface.
     */
    public function enableTableControls($enabled = true)
    {
        if (is_bool($enabled)) {
            $this->_clientTableControls = $enabled;
            if ($enabled && $this->_clientTableControlsThreshold < 1) {
                $this->_clientTableControlsThreshold = 10;
            }
        } elseif (is_numeric($enabled)) {
            $this->_clientTableControls = true;
            $this->_clientTableControlsThreshold = max(1, (int) $enabled);
        } else {
            $this->_clientTableControls = (bool) $enabled;
        }
        return $this;
    }

    public function hasTableControls()
    {
        return $this->_clientTableControls;
    }

    public function getTableControlsThreshold()
    {
        return $this->_clientTableControlsThreshold;
    }

    public function hasNote()
    {
        return $this->_hasNote;
    }

    public function GetSubTid($node)
    {
        if ($this->_subTbls == '') {
            return null;
        }

        $keynode = $node->GetChildren($this->_subTbls[0]);
        if ($keynode == null) {
            return null;
        }
        $newkey = $keynode->Get(CNode::FLD_VAL);
        if (($newkey == '0') || !isset($this->_subTbls[$newkey])) {
            return $this->_subTbls[1];
        }
        return $this->_subTbls[$newkey];
    }

    public function PrintHtml($dlayer, $disp)
    {
        $renderer = new DTblRenderer($this);
        $renderer->PrintHtml($dlayer, $disp);
    }
}
