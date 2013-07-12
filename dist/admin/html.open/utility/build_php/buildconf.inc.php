<?php

define ('LEGAL', 1);
define ('TITLE', 'Compile PHP with LSAPI');
define ('OPTION_VERSION', 2);

define ('BUILD_DIR', $_SERVER['LS_SERVER_ROOT'] . 'phpbuild');
define ('LAST_CONF', BUILD_DIR . '/savedconfig.'); // actual file will include . php base version.

define ('DEFAULT_INSTALL_DIR', $_SERVER['LS_SERVER_ROOT'].'lsphp'); // actual dir will include . php base version.

$PHP_VER = array('5'=>
		 array(
		 	'5.5.0',
		 	'5.4.17',
		 	'5.4.16',
		 	'5.4.15',
			'5.4.14',
		 	'5.4.13',
			'5.4.12',
			'5.4.11',
			'5.4.10',	
			'5.4.9',
			'5.4.8',
			'5.4.7',
			'5.4.6',
			'5.4.5',
		 	'5.3.26',
		 	'5.3.25',
		 	'5.3.24',
			'5.3.23',
			'5.3.22',
			'5.3.21',
			'5.3.20',
			'5.3.19',
			'5.3.18',
			'5.3.17',
			'5.3.16',
			'5.3.15',
			'5.3.8',
			'5.3.6',
			'5.2.17',
			'5.2.9',
			'5.1.6'),
		 '4'=>
		 array('4.4.9',
		       '4.4.8'));

define ('LSAPI_VERSION', '6.2');
define ('SUHOSIN_VERSION', '0.9.33');
define ('APC_VERSION', '3.1.9');
define ('XCACHE_VERSION', '3.0.3');
define ('MEMCACHE_VERSION', '2.2.7');
//define ('MEMCACHED_VERSION', '1.0.2');

				

$DEFAULT_PHP_PARAMS = array(
	'5' => '--with-mysqli --with-zlib --with-gd --enable-shmop --enable-track-vars --enable-sockets --enable-sysvsem --enable-sysvshm --enable-magic-quotes --enable-mbstring --with-iconv',
	'4' => '--with-mysql  --with-zlib --with-gd --enable-shmop --enable-track-vars --enable-sockets --enable-sysvsem --enable-sysvshm --enable-magic-quotes --enable-mbstring');


include_once( 'buildfunc.inc.php' );

?>
