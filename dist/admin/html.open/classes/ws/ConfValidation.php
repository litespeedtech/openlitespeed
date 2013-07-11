<?php

class ConfValidation extends CValidation
{
	protected function validatePostTbl($tbl, &$d)
	{
		$isValid = parent::validatePostTbl($tbl, $d);
		
		if ($tbl->_id == 'VH_SECHL') {
			$res = $this->chkPostTbl_VH_SECHL($d);
			$this->setValid($isValid, $res);
		}
		return $isValid;
	}
	
	protected function chkPostTbl_VH_SECHL(&$d)
	{
		$isValid = 1;
		if ( $d['enableHotlinkCtrl']->GetVal() == '0' ) {
			if ( isset($d['suffixes']) && !$d['suffixes']->HasVal() ) {
				$d['suffixes']->SetErr(NULL);
			}
			if ( isset($d['allowDirectAccess']) && !$d['allowDirectAccess']->HasVal() ) {
				$d['allowDirectAccess']->SetErr(NULL);
			}
			if ( isset($d['onlySelf']) && !$d['onlySelf']->HasVal() ) {
				$d['onlySelf']->SetErr(NULL);
			}
			$isValid = 2;
		} else {
			if ( $d['onlySelf']->GetVal() == '0'
			&&  !$d['allowedHosts']->HasVal() ) {
				$d['allowedHosts']->SetErr('Must be specified if "Only Self Reference" is set to No');
				$isValid = -1;
			}
		}
		return $isValid;
	}

	
}
