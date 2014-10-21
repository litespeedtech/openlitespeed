<?php

ob_start(); // just in case

define ('SERVER_ROOT', $_SERVER['LS_SERVER_ROOT']);

ini_set('include_path',
SERVER_ROOT . 'admin/html/lib/:' .
SERVER_ROOT . 'admin/html/lib/ows/:' .
SERVER_ROOT . 'admin/html/view/:.');

date_default_timezone_set('America/New_York');

spl_autoload_register( function ($class) {
	include $class . '.php';
});
