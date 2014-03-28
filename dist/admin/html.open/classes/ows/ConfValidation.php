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
		if ( preg_match( "/[<>&%]/", $name) ) {
			$err = 'invalid characters in name';
		}
		else {		
			$module = $_SERVER['LS_SERVER_ROOT'] . "modules/{$name}.so" ;
			if (!file_exists($module)) {
				$err = "cannot find module: $module";
			}
		}
		if ($err != '') {
			$cval->SetErr($err);
			return -1;
		}
		else 
			return 1;
	}
}
