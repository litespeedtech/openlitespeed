<?php

ob_start(); // just in case

header("Cache-Control: no-store, no-cache, private"); //HTTP/1.1
header("Expires: -1"); //ie busting
header("Pragma: no-cache");

define ('SERVER_ROOT', $_SERVER['LS_SERVER_ROOT']);
//set auto include path...get rid of all path headaches
ini_set('include_path',
SERVER_ROOT . 'admin/html/classes/:' .
SERVER_ROOT . 'admin/html/classes/ows/:' .
SERVER_ROOT. 'admin/html/includes/:.');

date_default_timezone_set('America/New_York');

spl_autoload_register( function ($class) {
	include $class . '.php';
});
