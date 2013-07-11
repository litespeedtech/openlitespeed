<?php

class CLIENT
{

	var $id = "";
	var $id_field = "lslb_uid";
	var $pass= "";
	var $pass_field = "lslb_pass";
	var $type = "LSLB";

	var $secret = NULL;
	var $token = "";
	
	var $timeout = 0;

	var $valid = FALSE;
	var $changed = FALSE;

	//limit array size per stat..
	var $stat_limit = 60;

	private static $_instance = NULL;
	
	// prevent an object from being constructed
	private function __construct() {}
	
	public static function singleton() {

		if (!isset(self::$_instance)) {
			$c = __CLASS__;
			self::$_instance = new $c;
			self::$_instance->init(); 
		}

		return self::$_instance;
	}
	
	function init() {
		global $_SESSION;

		session_name($this->$type . 'WEBUI'); // to prevent conflicts with other app sessions
		session_start();


		if(!array_key_exists('lbsecret',$_SESSION)) {
			$_SESSION['lbsecret'] = 'litespeedlbrocks';
		}

		if(!array_key_exists('lbchanged',$_SESSION)) {
			$_SESSION['lbchanged'] = FALSE;
		}


		if(!array_key_exists('lbvalid',$_SESSION)) {
			$_SESSION['lbsecret'] = FALSE;
		}

		if(!array_key_exists('lbtimeout',$_SESSION)) {
			$_SESSION['lbtimeout'] = 0;
		}

		if(!array_key_exists('lbtoken',$_SESSION)) {
			$_SESSION['lbtoken'] = microtime();
		}
		
		$this->valid = &$_SESSION['lbvalid'];
		$this->changed = &$_SESSION['lbchanged'];
		$this->secret = &$_SESSION['lbsecret'];
		$this->timeout = &$_SESSION['lbtimeout'];
		$this->token = $_SESSION['lbtoken'];

		if($this->valid == TRUE) {
			if (array_key_exists('lblastaccess',$_SESSION)) {

				if($this->timeout > 0 && time() - $_SESSION['lblastaccess'] > $this->timeout ) {
					$this->clear();
					header("location:/login.php?timedout=1");
					die();
				}

				$this->id = DUtil::grab_input('cookie',$this->id_field);
				$this->pass = DUtil::grab_input('cookie',$this->pass_field);
			}

			$this->updateAccessTime();
			
		}

		
	}

	function isValid() {
		return $this->valid;

	}

	function store($uid, $pass) {
		setcookie($this->id_field, $uid, 0, "/");
		setcookie($this->pass_field, $pass, 0, "/");

	}

	function setTimeout($timeout) {
		$this->timeout = (int) $timeout;
	}

	function updateAccessTime() {
		global $_SESSION;
		$_SESSION['lblastaccess'] = time();
	}

	function clear() {

		$this->valid = FALSE;
		session_destroy();
		session_unset();
		$outdated = time() - 3600*24*30;
		setcookie($this->id_field, "a", $outdated, "/");
		setcookie($this->pass_field, "b", $outdated, "/");
		setcookie(session_name(), "b", $outdated, "/");		
	}


	function authenticate($authUser, $authPass)
	{
		$auth = FALSE;

		if (strlen($authUser) && strlen($authPass) )
		{
			$filename = DUtil::grab_input("server","LS_SERVER_ROOT") . 'admin/conf/htpasswd';

			$fd = fopen( $filename, 'r');
			if ( !$fd) {
				return FALSE;
			}

			$all = trim(fread($fd, filesize($filename)));
			fclose($fd);

			$lines = explode("\n", $all);
			foreach( $lines as $line )
			{
				list($user, $pass) = explode(':', $line);
				if ( $user == $authUser )
				{
					$encypt = crypt($authPass, $pass);
					if ( $pass == $encypt )
					{
						$auth = TRUE;
						break;
					}
				}
			}
		}
		return $auth;

	}
	
	//persistent stats
	function &getStat($key) {
		global $_SESSION;
		
		$key = "lbstat_" . $key;
		
		if(isset($_SESSION[$key])) {
			return $_SESSION[$key];
		}
		else {
			$temp = NULL;
			return $temp;
		}
	}
	
	function addStat($key, &$data) {
		global $_SESSION;
		
		$result = &$this->getStat($key);
		
		$key = "lbstat_" . $key;

		if( $result != NULL) {
			//multiple stats windows open...check locks
			if(isset($_SESSION[$key . "_lock_"]) && time() - $_SESSION[$key . "_lock_"] <= 1) {
				return FALSE;
			}
			
			//data is stale
			if(isset($_SESSION[$key . "_lock_"]) && time() - $_SESSION[$key . "_lock_"] > 70) {
				$_SESSION[$key] = NULL;
				return FALSE;
			}
			
			//incoming data's column set does not match that of store data
			if(count($data) != count($result)) {
				return FALSE;	
			}
			
			//max item 30 reached...shorten array by 1 from head
			if( count($result[0]) >= $this->stat_limit) {
				foreach($result as $index => $set) {
					while(count($result[$index]) >= $this->stat_limit) {
						array_shift($result[$index]);
					}
				}
			}
			
			//add data
			foreach($result as $index => $set) {
				$tt = 1;
				$result[$index][] = $data[$index];
			}
			
			$_SESSION[$key . "_lock_"] = time();
			//$_SESSION[$key] = $result;
			return TRUE;
				
		}
		else {
			$result = array();
			
			//init data
			foreach($data as $index => $value) {
				$result[$index] = array();
				$result[$index][] = $value;
			}
			
			$_SESSION[$key] = &$result;
			$_SESSION[$key . "_lock_"] = time();
			return TRUE;
			
		}
	}



}
