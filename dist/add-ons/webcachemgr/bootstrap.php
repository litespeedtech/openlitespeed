<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2025 LiteSpeed Technologies, Inc.
 * *******************************************
 */

use Lsc\Wp\Context\Context;
use Lsc\Wp\Context\RootPanelContextOption;

require_once __DIR__ . '/autoloader.php';

/**
 * @noinspection PhpUnhandledExceptionInspection
 * @noinspection PhpUndefinedVariableInspection
 */
Context::initialize(new RootPanelContextOption($panelName));
