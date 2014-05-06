<?php

class ConfValidation extends CValidation
{
	// to hold special validation
	protected function isValidAttr($attr, $cval)
	{
		$isValid = parent::isValidAttr($attr, $cval);

		if ($isValid == 1 && $attr->_type == 'modulename') {
			$res = $this->chkAttr_modulename($attr, $cval);
			$this->setValid($isValid, $res);
		}
		return $isValid;
	}

	protected function chkAttr_modulename($attr, $cval)
	{
		$name = $cval->GetVal();
		if ( preg_match( "/[<>&%\s]/", $name) ) {
			$cval->SetErr('invalid characters in name');
			return -1;
		}
		else
			return 1;
	}

	protected function validatePostTbl($tbl, &$d)
	{
		if ($tbl->_id == 'SERV_MODULE') {
			$isValid = $this->chkPostTbl_SERV_MODULE($d);
		}
		else {
			$isValid = parent::validatePostTbl($tbl, $d);
		}
		return $isValid;
	}

	protected function chkPostTbl_SERV_MODULE(&$d)
	{
		$isValid = 1;

		if ($d['internal']->GetVal() == 0) {
			$name = $d['name']->GetVal();
			$module = $_SERVER['LS_SERVER_ROOT'] . "modules/{$name}.so" ;
			if (!file_exists($module)) {
				$d['name']->SetErr("cannot find external module: $module");
				$isValid = -1;
			}
		}

		return $isValid;
	}

}
