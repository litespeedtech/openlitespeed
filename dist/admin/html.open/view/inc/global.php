<?php

ob_start(); // just in case


header("Expires: -1");

header("Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
header("Pragma: no-cache");
header("X-Frame-Options: SAMEORIGIN");
header("Content-Security-Policy: frame-ancestors 'self'");
header("Referrer-Policy: same-origin");
header("X-Content-Type-Options: nosniff");

if (!function_exists('lstResolveServerRoot')) {
    function lstResolveServerRoot()
    {
        if (isset($_SERVER['LS_SERVER_ROOT']) && is_string($_SERVER['LS_SERVER_ROOT']) && $_SERVER['LS_SERVER_ROOT'] !== '') {
            return rtrim($_SERVER['LS_SERVER_ROOT'], '/') . '/';
        }

        $envRoot = getenv('LS_SERVER_ROOT');
        if (is_string($envRoot) && $envRoot !== '') {
            return rtrim($envRoot, '/') . '/';
        }

        if (is_dir('/usr/local/lsws/admin')) {
            return '/usr/local/lsws/';
        }

        $repoRoot = realpath(__DIR__ . '/../../');
        if ($repoRoot !== false) {
            return rtrim($repoRoot, '/') . '/';
        }

        return '';
    }
}

if (!defined('SERVER_ROOT')) {
    define('SERVER_ROOT', lstResolveServerRoot());
}

if (!defined('PRODUCT')) {
	define('PRODUCT', 'ows');
}

$libRoot = realpath(__DIR__ . '/../../lib');
$viewRoot = realpath(__DIR__ . '/..');
$includePath = [];
if ($libRoot !== false) {
    $includePath[] = $libRoot;
}
if ($viewRoot !== false) {
    $includePath[] = $viewRoot;
}
$includePath[] = '.';
ini_set('include_path', implode(PATH_SEPARATOR, $includePath));

// **PREVENTING SESSION HIJACKING**
// Prevents javascript XSS attacks aimed to steal the session ID
ini_set('session.cookie_httponly', 1);

// **PREVENTING SESSION FIXATION**
// Session ID cannot be passed through URLs
ini_set('session.use_only_cookies', 1);

// Uses a secure connection (HTTPS) if possible
if (isset($_SERVER['HTTPS']) && ($_SERVER['HTTPS'] == 'on')) {
	ini_set('session.cookie_secure', 1);
}

date_default_timezone_set('UTC');

require_once __DIR__ . '/../../lib/LSWebAdmin/Support/Bootstrap.php';

\LSWebAdmin\Support\Bootstrap::registerDefaultAutoload();
