<?php

class DAttr extends DAttrBase
{
	public $_version = 0; // 0: no restriction; 1: LSWS ENTERPRISE; 2: LSWS 2CPU +; 3: LSLB 2CPU +

	public function blockedVersion()
	{
		switch ($this->_version) {
			case 0: // no restriction
				return FALSE;
				
			case 3: // LSLB 2CPU +
				$processes = $_SERVER['LSLB_CHILDREN'];
				if ( !$processes ) {
					$processes = 1;
				}
				return ($processes < 2);
				
		}
		return TRUE; // not supported
	}

	public function bypassSavePost()
	{
		return ($this->_FDE[2] == 'N' || $this->blockedVersion());
	}
}
