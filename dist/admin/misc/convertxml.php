<?php
/**
 * convertxml.php -- convert LiteSpeed/OpenLiteSpeed configuration between the
 * XML format (.xml) and the plain-text conf format (.conf).
 *
 * This script ships in the OpenLiteSpeed-family (OWS/OwsDa) webadmin package
 * and is copied to $SERVER_ROOT/admin/misc/ at install time. The OWS package
 * stores configuration as plain conf; XML is supported only as a migration
 * source (older versions) and as a downgrade/recovery target.
 *
 * Usage:
 *   php convertxml.php <SERVER_ROOT> 2conf [<recover_script>]   # XML -> conf
 *   php convertxml.php <SERVER_ROOT> 2xml                       # conf -> XML
 *
 * Back-compat: any second argument other than "2xml"/"2conf" is treated as a
 * recover-script path and runs the XML -> conf direction, matching the legacy
 * `php convertxml.php $SERVER_ROOT $SERVER_ROOT/backup/recover_xml.sh` form.
 */

date_default_timezone_set('America/New_York');

// admin_php (lsphp, litespeed SAPI) does not define the CLI stream constants.
if (!defined('STDERR')) {
    define('STDERR', fopen('php://stderr', 'w'));
}

function convertxml_usage($exitCode)
{
    fwrite(STDERR, "Usage:\n");
    fwrite(STDERR, "  php convertxml.php <SERVER_ROOT> 2conf [<recover_script>]   convert XML to conf\n");
    fwrite(STDERR, "  php convertxml.php <SERVER_ROOT> 2xml                       convert conf to XML\n");
    exit($exitCode);
}

/**
 * Locate the deployed webadmin lib/ directory.
 *
 * This script lives in $SERVER_ROOT/admin/misc/. The webadmin runtime (with
 * its lib/ autoload tree) is installed alongside as admin/html/ (active) and,
 * on OLS, may also appear as admin/html.open/. Prefer an explicit override via
 * the LSWS_WEBADMIN_LIB environment variable for non-standard layouts.
 */
function convertxml_find_lib($serverRoot)
{
    $candidates = [];
    if (($override = getenv('LSWS_WEBADMIN_LIB')) !== false && $override !== '') {
        $candidates[] = rtrim($override, '/');
    }

    $adminBase = rtrim($serverRoot, '/') . '/admin';
    foreach (['html', 'html.open'] as $dir) {
        $candidates[] = $adminBase . '/' . $dir . '/lib';
    }

    // Fall back to a lib/ sibling of this script's directory.
    $candidates[] = dirname(__DIR__) . '/lib';

    foreach ($candidates as $candidate) {
        if (is_file($candidate . '/LSWebAdmin/Support/Bootstrap.php')) {
            return $candidate;
        }
    }

    return null;
}

if ($argc < 3) {
    convertxml_usage(1);
}

$serverRoot = $argv[1];
$mode = $argv[2];

if ($serverRoot === '' || !is_dir($serverRoot)) {
    fwrite(STDERR, "ERROR: SERVER_ROOT '$serverRoot' is not a directory\n");
    convertxml_usage(1);
}

$libDir = convertxml_find_lib($serverRoot);
if ($libDir === null) {
    fwrite(STDERR, "ERROR: unable to locate the webadmin lib/ directory under $serverRoot/admin/.\n");
    fwrite(STDERR, "       Set LSWS_WEBADMIN_LIB to the absolute path of the webadmin lib/ directory.\n");
    exit(1);
}

// The packaged product Resolver bakes in its variant, but the development tree
// resolves the active product from the PRODUCT constant. Define it (defaulting
// to ows, the only family that ships this converter) so the script runs in both
// layouts. An explicit LSWS_WEBADMIN_PRODUCT override wins if set.
if (!defined('PRODUCT')) {
    $product = getenv('LSWS_WEBADMIN_PRODUCT');
    if ($product === false || $product === '') {
        $product = 'ows';
    }
    define('PRODUCT', $product);
}

set_include_path($libDir . PATH_SEPARATOR . get_include_path());
require_once $libDir . '/LSWebAdmin/Support/Bootstrap.php';
\LSWebAdmin\Support\Bootstrap::registerDefaultAutoload();

use LSWebAdmin\Config\Migration\ConfigMigrationCommand;

if ($mode === '2xml') {
    echo "Converting plain conf configuration files to XML under $serverRoot ...\n";
    ConfigMigrationCommand::migrateAllConfToXml($serverRoot);
    echo "Conversion to XML finished.\n";
    exit(0);
}

// Default and "2conf": XML -> conf. A third argument (or a legacy non-mode
// second argument) names the recover script that restores the archived XML.
if ($mode === '2conf') {
    $recoverScript = ($argc >= 4 && $argv[3] !== '')
        ? $argv[3]
        : rtrim($serverRoot, '/') . '/backup/recover_xml.sh';
} else {
    // Legacy form: second argument is the recover script path.
    $recoverScript = $mode;
}

$recoverDir = dirname($recoverScript);
if (!is_dir($recoverDir) && !@mkdir($recoverDir, 0755, true) && !is_dir($recoverDir)) {
    fwrite(STDERR, "ERROR: unable to create recover-script directory $recoverDir\n");
    exit(1);
}

echo "Converting XML configuration files to plain conf under $serverRoot ...\n";
ConfigMigrationCommand::migrateAllXmlToConf($serverRoot, $recoverScript, 1);
echo "Conversion to conf finished. Recover script: $recoverScript\n";
exit(0);
