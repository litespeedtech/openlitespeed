<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author Michael Alegre
 * @copyright (c) 2018-2022 LiteSpeed Technologies, Inc.
 * *******************************************
 */

if ( version_compare(PHP_VERSION, '5.6.0', '<') ) {
    fwrite(
        STDERR,
        "ERROR: PHP 5.6.0 or later is required. Current version: "
            . PHP_VERSION
            . "\n"
    );
    exit(1);
}

use Lsc\Wp\Context\Context;
use Lsc\Wp\Context\RootCLIContextOption;
use Lsc\Wp\CliController;

require_once __DIR__ . '/autoloader.php';

date_default_timezone_set('UTC');

/** @noinspection PhpUnhandledExceptionInspection */
Context::initialize(new RootCLIContextOption());
CliController::run();
