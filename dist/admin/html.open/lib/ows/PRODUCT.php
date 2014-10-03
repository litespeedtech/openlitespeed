<?php

class PRODUCT
{
	var $product = NULL;
	var $version = NULL;
	var $edition = '';
	var $type = 'LSWS';

	var $tmp_path = NULL;
	var $processes = 1;


	private function __construct()
	{
		$this->init();
	}

	public static function GetInstance()
	{
        if ( !isset($GLOBALS['_PRODUCT_']) )
			$GLOBALS['_PRODUCT_'] = new PRODUCT();
		return $GLOBALS['_PRODUCT_'];
	}

	function init() {

		$this->processes = $_SERVER['LSWS_CHILDREN'];

		$matches = array();
		$this->product = 'LITESPEED WEB SERVER';
		$str = $_SERVER['LSWS_EDITION'];
		if ( preg_match('/^(.*)\/(.*)\/(.*)$/i', $str, $matches ) ) {
			$this->edition = strtoupper(trim($matches[2]));
			$this->version = trim($matches[3]);
		}
		$this->tmp_path = "/tmp/lshttpd/";
	}


	function refreshVersion() {
		$versionfile = SERVER_ROOT . 'VERSION';
		$this->version = trim( file_get_contents($versionfile) );
	}


}
