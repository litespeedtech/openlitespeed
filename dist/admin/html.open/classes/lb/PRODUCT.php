<?php

class PRODUCT
{
	var $product = NULL;
	var $version = NULL;
	var $edition = '';
	var $type = 'LSLB';
	
	var $tmp_path = NULL;
	var $processes = 1;
	
	var $new_release = NULL;
	var $new_version = NULL;
	
	var $installed_releases = array();	
	
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

		$this->processes = DUtil::grab_input("server", 'LSLB_CHILDREN','int');
		
		$matches = array();
		$this->product = 'LITESPEED LOAD BALANCER';
		$str = DUtil::grab_input("server",'LSLB_RELEASE');
		if ( preg_match('/^(.*)\/(.*)$/i', $str, $matches ) ) {
			$this->version = trim($matches[2]);
		}
		$this->tmp_path = "/tmp/lslbd/";
	}
	
	function getInstalled() {

		$dir = DUtil::grab_input("server",'LS_SERVER_ROOT'). 'bin';
		$dh = @opendir($dir);
		if ($dh) {
			while (($fname = readdir($dh)) !== false) {
				$matches = array();
				if (preg_match('/^lslbctrl\.(.*)$/', $fname, $matches)) {
					$this->installed_releases[] = $matches[1];
				}
			}
			closedir($dh);
		}
	}
	
	function isInstalled($version) {
		$state = FALSE;
		
		foreach($this->installed_releases as $value) {
			if($version == $value) {
				return TRUE;
			}
		}
		
		return $state;
	}
	
	function refreshVersion() {
		$versionfile = DUtil::grab_input("server",'LS_SERVER_ROOT'). 'VERSION';
		$this->version = trim( file_get_contents($versionfile) );		
	}
	
	function getNewVersion() {

		$dir = DUtil::grab_input("server",'LS_SERVER_ROOT'). 'autoupdate';
		$releasefile = $dir.'/release';
		if ( file_exists($releasefile) ) {
			$this->new_release = trim( file_get_contents($releasefile) );
			$matches = array();
			if (strpos($this->new_release, '-')) {
				if ( preg_match( '/^(.*)-/', $this->new_release, $matches ) ) {
					$this->new_version = $matches[1];
				}
			}
			else {
				$this->new_version = $this->new_release;
			}
		}
	}

}
