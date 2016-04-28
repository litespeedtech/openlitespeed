<?php

class Product
{
	const PROD_NAME = 'LITESPEED WEB SERVER';

	private $version;
	private $edition;
	private $new_version;

	private function __construct()
	{
		$matches = array();
		$str = $_SERVER['LSWS_EDITION'];
		if ( preg_match('/^(.*)\/(.*)\/(.*)$/i', $str, $matches ) ) {
			$this->edition = strtoupper(trim($matches[2]));
			$this->version = trim($matches[3]);
		}
		$releasefile = SERVER_ROOT . 'autoupdate/release' ;
		if ( file_exists($releasefile) ) {
			$this->new_version = trim(file_get_contents($releasefile)) ;
			if ($this->version == $this->new_version)
				$this->new_version = null;
		}
	}

	public static function GetInstance()
	{
        if ( !isset($GLOBALS['_PRODUCT_']) )
			$GLOBALS['_PRODUCT_'] = new Product();
		return $GLOBALS['_PRODUCT_'];
	}

	public function getEdition()
	{
		return $this->edition;
	}

	public function getVersion()
	{
		return $this->version;
	}

	public function refreshVersion() {
		$versionfile = SERVER_ROOT . 'VERSION';
		$this->version = trim( file_get_contents($versionfile) );
	}

	public function getNewVersion()
	{
		return $this->new_version;
	}

}
