<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 * @Author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @Copyright: (c) 2018
 * *******************************************
 */

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\Context\RootPanelContextOption;

require_once __DIR__ . '/autoloader.php';

/**
 * @noinspection PhpUnhandledExceptionInspection
 * @noinspection PhpUndefinedVariableInspection
 */
Context::initialize(new RootPanelContextOption($panelName));
