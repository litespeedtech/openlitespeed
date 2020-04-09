<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 * @Author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @Copyright: (c) 2018
 * *******************************************
 */

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\Context\RootCLIContextOption;
use \Lsc\Wp\CliController;

require_once __DIR__ . '/autoloader.php';

date_default_timezone_set('UTC');

/** @noinspection PhpUnhandledExceptionInspection */
Context::initialize(new RootCLIContextOption());
CliController::run();
