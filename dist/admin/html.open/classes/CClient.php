<?php

class CClient
{
	//limit array size per stat..
	private $stat_limit = 60;

	private static $_instance = NULL;

	// prevent an object from being constructed
	private function __construct()
	{
		global $_SESSION;

		if(!array_key_exists('changed',$_SESSION)) {
			$_SESSION['changed'] = FALSE;
		}

		$this->changed = &$_SESSION['changed'];
	}

	public static function singleton() {

		if (!isset(self::$_instance)) {
			$c = __CLASS__;
			self::$_instance = new $c;
		}

		return self::$_instance;
	}

	public static function HasChanged()
	{
		global $_SESSION;
		return $_SESSION['changed'];
	}

	public static function SetChanged($changed)
	{
		global $_SESSION;
		$_SESSION['changed'] = $changed;
	}

	//persistent stats
	function &getStat($key) {
		global $_SESSION;

		$key = "stat_$key";

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
		$sess_key = "stat_$key";
		$sess_keylock = "{$key}_lock_";

		if( $result != NULL) {
			$curtime = time();
			$time_span = $curtime - $_SESSION[$sess_keylock];
			if(isset($_SESSION[$sess_keylock])) {
				if ($time_span <= 1) {
					//multiple stats windows open...check locks
					echo("multiple stats windows open\n");
					return FALSE;
				}
				elseif ($time_span > 70) {
					//data is stale
					$_SESSION[$sess_key] = NULL;
					echo ("data is stale\n");
					return FALSE;
				}
			}

			//incoming data's column set does not match that of store data
			if(count($data) != count($result)) {
				echo("incoming data's column set does not match that of the stored data.\n");
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
				$result[$index][] = $data[$index];
			}

			$_SESSION[$sess_keylock] = $curtime;
			return TRUE;

		}
		else {
			$result = array();

			//init data
			foreach($data as $index => $value) {
				$result[$index] = array();
				$result[$index][] = $value;
			}

			$_SESSION[$sess_key] = &$result;
			$_SESSION[$sess_keylock] = $curtime;
			return TRUE;

		}
	}


}
