<?php

define ('LEGAL', 1);
define ('TITLE', 'Compile PHP with LSAPI');
define ('OPTION_VERSION', 3);

define ('BUILD_DIR', $_SERVER['LS_SERVER_ROOT'] . 'phpbuild');
define ('LAST_CONF', BUILD_DIR . '/savedconfig.'); // actual file will include . php base version.

define ('DEFAULT_INSTALL_DIR', $_SERVER['LS_SERVER_ROOT'].'lsphp'); // actual dir will include . php base version.

$PHP_VER = array('5'=>
		 array(
		 	'5.6.9',
		 	'5.5.25',
		 	'5.4.41',
		 	'5.3.29',
			'5.2.17'),
		 '4'=>
		 array('4.4.9'));

define ('LSAPI_VERSION', '6.7');
define ('SUHOSIN_VERSION', '0.9.37.1');
define ('APC_VERSION', '3.1.9');
define ('XCACHE_VERSION', '3.2.0');
define ('MEMCACHE_VERSION', '2.2.7');
//define ('MEMCACHED_VERSION', '1.0.2');
define ('OPCACHE_VERSION', '7.0.5');

$DEFAULT_PHP_PARAMS = array(
	'5' => '--with-mysqli --with-zlib --with-gd --enable-shmop --enable-sockets --enable-sysvsem --enable-sysvshm --enable-mbstring --with-iconv --with-mcrypt',
	'4' => '--with-mysql  --with-zlib --with-gd --enable-shmop --enable-sockets --enable-sysvsem --enable-sysvshm --enable-magic-quotes --enable-mbstring');


include_once( 'buildfunc.inc.php' );
