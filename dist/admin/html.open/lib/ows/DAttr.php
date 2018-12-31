<?php

class DAttr extends DAttrBase
{
    public $cyberpanelBlocked = false;

	public function blockedVersion()
	{
		if ($this->cyberpanelBlocked) {
            if (PathTool::IsCyberPanel()) {
                return 'Locked due to CyberPanel';
            }
        }

        // no other block
        return false;
	}

	public function bypassSavePost()
	{
		return ($this->IsFlagOn(DAttr::BM_NOEDIT));
	}
}
