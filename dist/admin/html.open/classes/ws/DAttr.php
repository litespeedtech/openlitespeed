<?php

class DAttr extends DAttrBase
{
	public $_version = 0; // 0: no restriction; 1: LSWS ENTERPRISE; 2: LSWS 2CPU +; 

	public function blockedVersion()
	{
		switch ($this->_version) {
			case 0: // no restriction
				return FALSE;
				
			case 1:  // LSWS ENTERPRISE; 
				$edition = strtoupper($_SERVER['LSWS_EDITION']);
				return ( strpos($edition, "ENTERPRISE" ) === FALSE );

			case 2: // LSWS 2CPU +
				$processes = $_SERVER['LSWS_CHILDREN'];
				if ( !$processes) {
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
