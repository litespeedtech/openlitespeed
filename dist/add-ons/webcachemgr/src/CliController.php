<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp;

use \Lsc\Wp\Context\Context;
use \Lsc\Wp\DashNotifier;
use \Lsc\Wp\LSCMException;
use \Lsc\Wp\Panel\ControlPanel;
use \Lsc\Wp\Panel\CPanel;
use \Lsc\Wp\PluginVersion;
use \Lsc\Wp\Util;
use \Lsc\Wp\WPInstall;
use \Lsc\Wp\WPInstallStorage;

class CliController
{

    /**
     * @var string[]
     */
    private $cacheRootCmds = array();

    /**
     * @var string[]
     */
    private $commands = array();

    /**
     * @var string
     */
    private $svrCacheRootParam = '';

    /**
     * @var string
     */
    private $vhCacheRootParam = '';

    /**
     * @var boolean
     */
    private $displayCacheRoots = false;

    /**
     * @var null|string
     */
    private $currWpPath;

    /**
     * @var null|string
     */
    private $versionCmd;

    /**
     * @var null|string
     */
    private $specialCmd;

    /**
     * Holds any additional command input for use after parseCommands().
     *
     * @var null|mixed[]
     */
    private $input;

    /**
     *
     * @throws LSCMException
     */
    public function __construct()
    {
        if ( empty($_SERVER['argv']) || $_SERVER['argc'] < 3 ) {
            throw new LSCMException('Cli Illegal entrance!');
        }

        $args = $_SERVER['argv'];

        /**
         * shift out script name
         */
        array_shift($args);

        $this->parseCommands($args);
    }

    /**
     * Checks if current $wpInstallStorage object is capable of having the
     * given non-scan action performed.
     *
     * @param string $action
     * @param WPInstallStorage $wpInstallStorage
     * @return null
     * @throws LSCMException
     */
    private function checkDataFile( $action,
            WPInstallStorage $wpInstallStorage) {

        if ( $action == 'scan' ) {
            /**
             * Always allowed.
             */
            return;
        }

        $msg = '';

        if ( ($err = $wpInstallStorage->getError()) ) {

            switch ($err) {
                case WPInstallStorage::ERR_NOT_EXIST:
                case WPInstallStorage::ERR_CORRUPTED:
                case WPInstallStorage::ERR_VERSION_HIGH:
                    $msg = 'Scan data could not be read! Please scan again (without the \'-n\' flag) '
                            . "before attempting any cache operations.\n";
                    break;
                case WPInstallStorage::ERR_VERSION_LOW:
                    $msg = 'Scan data file format has been changed for this version. Please scan '
                            . 'again (without the \'-n\' flag) before attempting any cache operations.'
                            . "\n";
                    break;
                //no default
            }
        }
        elseif ( strpos($action,'mass_') === 0
                && $wpInstallStorage->getCount() == 0 ) {

            $msg = 'No WordPress installations discovered in the previous scan. If you have any newly '
                    . "installed WordPress installations, please scan again or add them with"
                    . "command 'addinstalls'.\n";
        }

        if ( $msg != '' ) {
            throw new LSCMException($msg);
        }
    }

    /**
     *
     * @param WPInstall  $wpInstall
     */
    private function printStatusMsg( WPInstall $wpInstall )
    {
        $msg = "{$this->currWpPath} - Status: ";

        $status = $wpInstall->getStatus();

        if ( $status & WPInstall::ST_PLUGIN_ACTIVE ) {
            $msg .= 'enabled';

            if ( !$status & WPInstall::ST_LSC_ADVCACHE_DEFINED ) {
                $msg .= ', not caching';
            }
        }
        else {
            $msg .= 'disabled';
        }

        $msg .= ' | Flagged: ';
        $msg .= ($status & WPInstall::ST_FLAGGED) ? 'yes' : 'no';

        $msg .= ' | Error: ';

        if ( $status & WPInstall::ST_ERR_SITEURL ) {
            $msg .= 'Could not retrieve WordPress siteURL.';
        }
        elseif ( $status & WPInstall::ST_ERR_DOCROOT ) {
            $msg .= 'Could not match WordPress siteURL to a known control panel docroot.';
        }
        elseif ( $status & WPInstall::ST_ERR_EXECMD ) {
            $msg .= 'WordPress fatal error encountered during action execution. This is most likely '
                    . 'caused by custom code in this WordPress installation.';
        }
        elseif ( $status & WPInstall::ST_ERR_EXECMD_DB ) {
            $msg .= 'Error establishing WordPress database connection.';
        }
        elseif ( $status & WPInstall::ST_ERR_TIMEOUT ) {
            $msg .= 'Timeout occurred during action execution.';
        }
        elseif ( $status & WPInstall::ST_ERR_WPCONFIG ) {
            $msg .= 'Could not find a valid wp-config.php file.';
        }

        echo "{$msg}\n\n";
    }

    /**
     *
     * @param string[]  $args
     * @return null
     * @throws LSCMException
     */
    private function handleSetCacheRootInput( &$args )
    {
        if ( empty($args) ) {
            $this->cacheRootCmds[] = 'listCacheRoots';
            return;
        }

        $controlPanel = ControlPanel::getClassInstance();

        if ( ($key = array_search('-svr', $args)) !== false ) {

            if ( empty($args[$key + 1])
                    || ($this->svrCacheRootParam = trim($args[$key + 1])) == '' ) {

                throw new LSCMException('Invalid Command, missing server cache root value.');
            }

            $currSvrCacheRoot = $controlPanel->getServerCacheRoot();

            if ( $this->svrCacheRootParam != $currSvrCacheRoot) {

                if ( !Util::is_dir_empty($this->svrCacheRootParam) ) {
                    throw new LSCMException(
                            'Provided server level cache root must be an empty directory.');
                }

                $this->cacheRootCmds[] = 'setSvrCacheRoot';
            }

            unset($args[$key], $args[$key + 1]);
        }

        if ( ($key = array_search('-vh', $args)) !== false ) {
            $setvhCacheRoot = false;

            if ( empty($args[$key + 1])
                    || ($this->vhCacheRootParam = trim($args[$key + 1])) == '' ) {

                throw new LSCMException(
                        'Invalid Command, missing virtual host cache root value.');
            }

            if ( strpos($this->vhCacheRootParam, '$') !== false ) {
                throw new LSCMException(
                    'Invalid Command, virtual host cache root value cannot contain any \'$\' '
                        . 'characters. \'$vh_user\' will be automatically added to the end of virtual '
                        . 'host cache root values starting with a \'/\'.');
            }

            $currVHCacheRoot = $controlPanel->getVHCacheRoot();

            if ( $this->vhCacheRootParam[0] == '/' ) {
                $updatedVhCacheRoot =
                        rtrim($this->vhCacheRootParam, '/') . '/$vh_user';

                if ( $updatedVhCacheRoot != $currVHCacheRoot
                        && ! Util::is_dir_empty($this->vhCacheRootParam) ) {

                    throw new LSCMException('Provided absolute path for virtual host level '
                            . 'cache root must be an empty directory.');
                }

                $this->vhCacheRootParam = $updatedVhCacheRoot;
                $setvhCacheRoot = true;
            }
            elseif ( $this->vhCacheRootParam != $currVHCacheRoot ) {
                $setvhCacheRoot = true;
            }

            unset($args[$key], $args[$key + 1]);

            if ( $setvhCacheRoot ) {
                $this->cacheRootCmds[] = 'setVHCacheRoot';
            }
        }
    }

    /**
     *
     * @param string[]  $args
     * @return null
     * @throws LSCMException
     */
    private function handleSetVersionInput( &$args )
    {
        if ( ($key = array_search('--list', $args)) !== false ) {
            unset($args[$key]);
            $this->versionCmd = 'list';
            return;
        }

        if ( ($key = array_search('--latest', $args)) !== false ) {
            unset($args[$key]);
            $this->versionCmd = 'latest';
            return;
        }

        if ( empty($args) ) {
            $this->versionCmd = 'active';
            return;
        }

        $v = array_shift($args);

        if ( preg_match('/[1-9]\.[0-9](?:\.[0-9])*(?:\.[0-9])*/', $v) !== 1 ) {
            throw new LSCMException("Invalid version number ({$v}).");
        }

        $this->versionCmd = $v;
    }

    /**
     *
     * @param string[]  $args
     */
    private function handleScanInput( &$args )
    {
        if ( ($key = array_search('-n', $args)) !== false ) {
            unset($args[$key]);
            $this->commands[] = 'discoverNew';
        }
        else {
            $this->commands[] = 'scan';
        }

        if ( ($key = array_search('-e', $args)) !== false ) {
            unset($args[$key]);
            $this->commands[] = 'mass_enable';
        }
    }

    /**
     *
     * @param string[]  $args
     * @return boolean
     */
    private function isMassOperation( &$args )
    {
        if ( ($key = array_search('-m', $args)) !== false ) {
            unset($args[$key]);
            return true;
        }

        return false;
    }

    /**
     *
     * @param string    $cmd
     * @param string[]  $args
     * @throws LSCMException
     */
    private function handleSingleOperationInput( $cmd, &$args )
    {
        $path = array_shift($args);

        if ( $path == null ) {
            throw new LSCMException('Invalid Command, missing WP path.');
        }

        $wpInstall = new WPInstall($path);

        if ( !$wpInstall->hasValidPath() ) {
            throw new LSCMException("Invalid WP Path: {$path}.");
        }

        $this->commands[] = $cmd;
        $this->currWpPath = rtrim($path, '/');
    }

    /**
     *
     * @param string    $cmdType
     * @param string[]  $args
     * @throws LSCMException
     */
    private function handleDashNotifyInput( $cmdType, &$args )
    {
        if ( $cmdType == 'notify' ) {

            if ( ($key = array_search('-m', $args)) !== false ) {
                unset($args[$key]);
                $this->commands[] = 'mass_dash_notify';
            }
            else {

                if ( ($key = array_search('-wppath', $args)) === false ) {
                    throw new LSCMException(
                            'Invalid Command, missing required \'-m\' or \'-wppath\'. parameter.');
                }

                if ( empty($args[$key + 1]) ) {
                    throw new LSCMException('Invalid Command, missing \'-wppath\' value.');
                }

                $path = $args[$key + 1];

                $wpInstall = new WPInstall($path);

                if ( !$wpInstall->hasValidPath() ) {
                    throw new LSCMException("Invalid WP Path: {$path}.");
                }

                $this->commands[] = 'dash_notify';
                $this->currWpPath = rtrim($path, '/');

                unset($args[$key], $args[$key + 1]);
            }

            $dashInput = array();

            if ( ($key = array_search('-msgfile', $args)) !== false ) {

                if ( empty($args[$key + 1]) ) {
                    throw new LSCMException(
                            'Invalid Command, missing \'-msgfile\' value.');
                }

                $msgFilePath = $args[$key + 1];

                if ( !file_exists($msgFilePath) ) {
                    throw new LSCMException('Provided message file does not exist.');
                }

                if ( ($message = file_get_contents($msgFilePath)) === false ) {
                    throw new LSCMException(
                            'Unable to retrieve provided message file content.');
                }
            }
            else {

                if ( ($key = array_search('-msg', $args)) === false ) {
                    throw new LSCMException(
                            'Invalid Command, missing required \'-msgfile\' or \'-msg\'. parameter.');
                }

                if ( empty($args[$key + 1]) ) {
                    throw new LSCMException('Invalid Command, missing \'-msg\' value.');
                }

                $message = $args[$key + 1];
            }

            $dashInput['msg'] = $message;

            unset($args[$key], $args[$key + 1]);

            if ( ($key = array_search('-plugin', $args)) !== false ) {

                if ( empty($args[$key + 1]) ) {
                    throw new LSCMException('Invalid Command, missing \'-plugin\' value.');
                }

                $slug = $args[$key + 1];

                $url = "https://api.wordpress.org/plugins/info/1.0/{$slug}.json";
                $pluginInfoJSON = Util::get_url_contents($url);

                $pluginInfo = json_decode($pluginInfoJSON, true);

                if ( empty($pluginInfo['name']) ) {
                    throw new LSCMException(
                            'Could not find a plugin mathcing the provided plugin slug.');
                }

                $dashInput['slug'] = $slug;

                unset($args[$key], $args[$key + 1]);
            }

            $this->input = $dashInput;
        }
        elseif ( $cmdType == 'remove' ) {
            $arg1 = array_shift($args);

            if ( $arg1 == '-m' ) {
                $this->commands[] = 'mass_dash_disable';
            }
            else {
                $path = $arg1;

                if ( $path == null ) {
                    throw new LSCMException('Invalid Command, missing WP path.');
                }

                $wpInstall = new WPInstall($path);

                if ( !$wpInstall->hasValidPath() ) {
                    throw new LSCMException("Invalid WP Path: {$path}.");
                }

                $this->commands[] = 'dash_disable';
                $this->currWpPath = rtrim($path, '/');
            }
        }
    }

    /**
     *
     * @param CPanel $controlPanel
     */
    private function initNewCpanelPluginConf( CPanel $controlPanel )
    {
        $lswsHome = realpath(__DIR__ . '/../../..');

        $controlPanel->UpdateCpanelPluginConf(
                CPanel::USER_PLUGIN_SETTING_LSWS_DIR, $lswsHome);

        $vhCacheRoot = $controlPanel->getVHCacheRoot();

        $controlPanel->UpdateCpanelPluginConf(
                CPanel::USER_PLUGIN_SETTING_VHOST_CACHE_ROOT, $vhCacheRoot);
    }

    /**
     *
     * @param string[]  $args
     * @throws LSCMException
     */
    private function handleCpanelPluginInput( &$args )
    {
        $controlPanel = ControlPanel::getClassInstance();

        if ( !($controlPanel instanceof CPanel) ) {
            throw new LSCMException('Command \'cpanelplugin\' cannot be used in a non-cPanel '
                    . 'environment.');
        }

        if ( $args[0] == '--install' ) {
            $this->specialCmd = 'cpanelPluginInstall';
            unset($args[0]);
        }
        elseif ( $args[0] == '--uninstall' ) {
            $this->specialCmd = 'cpanelPluginUninstall';
            unset($args[0]);
        }
        elseif ( $args[0] == '-autoinstall' ) {

            if ( !isset($args[1]) ) {
                $this->specialCmd = 'cpanelPluginAutoInstallStatus';
            }
            elseif ( "{$args[1]}" === '1' ) {
                $this->specialCmd = 'cpanelPluginAutoInstallOn';
                unset($args[1]);
            }
            elseif ( "{$args[1]}" === '0' ) {
                $this->specialCmd = 'cpanelPluginAutoInstallOff';
                unset($args[1]);
            }

            unset($args[0]);
        }
    }

    /**
     *
     * @param string[]  $args
     * @throws LSCMException
     */
    private function handleAddInstallsInput( &$args )
    {
        switch ($args[0]) {

            case '-wpinstall':

                if ( empty($args[1]) ) {
                    throw new LSCMException('Invalid Command, missing \'<wp path>\' value.');
                }

                $wpInstallsInfo = "{$args[1]}";

                if ( empty($args[2]) ) {
                    throw new LSCMException('Invalid Command, missing \'<docroot>\' value.');
                }

                $wpInstallsInfo .= " {$args[2]}";

                if ( empty($args[3]) ) {
                    throw new LSCMException(
                            'Invalid Command, missing \'<server name>\' value.');
                }

                $wpInstallsInfo .= " {$args[3]}";

                if ( empty($args[4]) ) {
                    throw new LSCMException('Invalid Command, missing \'<site url>\' value.');
                }

                $wpInstallsInfo .= " {$args[4]}";

                $this->commands[] = WPInstallStorage::CMD_ADD_CUST_WPINSTALLS;

                $this->input = array( 'addInstallsInfo' => array($wpInstallsInfo) );

                unset($args[0], $args[1], $args[2], $args[3], $args[4]);
                break;

            case '-wpinstallsfile':

                if ( empty($args[1]) ) {
                    throw new LSCMException(
                            'Invalid Command, missing \'-wpinstallsfile\' value.');
                }

                $wpInstallsFile = $args[1];

                if ( !file_exists($wpInstallsFile) ) {
                    throw new LSCMException('Provided wpinstalls file does not exist.');
                }

                $fileContent = file_get_contents($wpInstallsFile);

                if ( $fileContent === false ) {
                    throw new LSCMException("Could not read wpinstalls file content.");
                }

                $wpInstallsInfo = explode("\n", trim($fileContent));

                $this->commands[] = WPInstallStorage::CMD_ADD_CUST_WPINSTALLS;

                $input = array( 'addInstallsInfo' => $wpInstallsInfo );
                $this->input = $input;

                unset($args[0], $args[1]);
                break;

            // no default case
        }
    }

    private function displayCacheRoots()
    {
       $controlPanel = ControlPanel::getClassInstance();

       $svrCacheRoot = $controlPanel->getServerCacheRoot();
       $vhCacheRoot = $controlPanel->getVHCacheRoot();

       if ( $svrCacheRoot == ControlPanel::NOT_SET ) {
           $svrCacheRoot = 'Not Set';
       }

       if ( $vhCacheRoot == ControlPanel::NOT_SET) {
           $vhCacheRoot = 'Not Set';
       }

       echo <<<EOF

Server Cache Root:        {$svrCacheRoot}
Virtual Host Cache Root:  {$vhCacheRoot}


EOF;
    }

    /**
     *
     * @param string[]  $args
     * @throws LSCMException
     */
    private function parseCommands( $args )
    {
        $panelClassName = array_shift($args);

        $controlPanel = ControlPanel::getClassInstance($panelClassName);

        $cmd = array_shift($args);

        switch ($cmd) {
            case 'setcacheroot':
                if ( $panelClassName == 'custom' ) {
                    $msg = 'Command \'setcacheroot\' cannot be used in the CustomPanel context.';
                    throw new LSCMException($msg);
                }

                $this->handleSetCacheRootInput($args);
                break;

            case 'setversion':
                $this->handleSetVersionInput($args);
                break;

            case 'scan':
                $this->handleScanInput($args);
                break;

            case 'enable':
            case 'disable':
            case 'upgrade':
            case 'unflag':

                if ( $this->isMassOperation($args) ) {
                    $this->commands[] = "mass_{$cmd}";
                    break;
                }

            //fall through

            case 'flag':
            case 'status':
                $this->handleSingleOperationInput($cmd, $args);
                break;
            case 'dashnotify':
                $this->handleDashNotifyInput('notify', $args);
                break;
            case 'dashnotifyremove':
                $this->handleDashNotifyInput('remove', $args);
                break;
            case 'cpanelplugin':
                $this->handleCpanelPluginInput($args);
                break;
            case 'addinstalls':
                $this->handleAddInstallsInput($args);
                break;
            default:
                throw new LSCMException('Invalid Command, Try --help.');
        }

        if ( !empty($args) ) {
            throw new LSCMException('Invalid Command, Try --help.');
        }
    }

    /**
     *
     * @param string        $action
     */
    private function doCacheRootCommand( $action )
    {
        $restartRequired = false;

        $controlPanel = ControlPanel::getClassInstance();

        switch ( $action ) {
            case 'setSvrCacheRoot':
                $controlPanel->setServerCacheRoot($this->svrCacheRootParam);
                $restartRequired = true;
                break;

            case 'setVHCacheRoot':
                $controlPanel->setVHCacheRoot($this->vhCacheRootParam);
                $restartRequired = true;
                break;

            case 'listCacheRoots':
                /**
                 * Wait until after verifyCacheSetup() has been run.
                 */
                $this->displayCacheRoots = true;
                break;

            //no default
        }

        if ( $restartRequired ) {
            Util::restartLsws();
        }

        $vhCacheRoot = $controlPanel->getVHCacheRoot();
        Util::ensureVHCacheRootInCage($vhCacheRoot);
    }

    private function doSpecialCommand()
    {
        $controlPanel = ControlPanel::getClassInstance();

        switch($this->specialCmd) {

            case 'cpanelPluginInstall':
                /* @var $controlPanel CPanel */

                switch ( $controlPanel->installCpanelPlugin() ) {

                    case 'update':
                        echo "Updated LiteSpeed cPanel plugin to current version\n\n";
                        break;

                    case 'new':
                        $this->initNewCpanelPluginConf($controlPanel);
                        echo "LiteSpeed cPanel plugin installed, auto install turned on.\n\n";
                        break;

                    //no default
                }

                break;

            case 'cpanelPluginUninstall':
                /* @var $controlPanel CPanel */
                $controlPanel->uninstallCpanelPlugin();

                echo "LiteSpeed cPanel plugin uninstalled successfully, auto install turned off.\n\n";
                break;

            case 'cpanelPluginAutoInstallStatus':
                $state = (CPanel::isCpanelPluginAutoInstallOn()) ? 'On' : 'Off';

                echo "Auto install is currently {$state} for the LiteSpeed cPanel plugin.\n";
                echo "Use command 'cpanelplugin -autoinstall {0 | 1}' to turn auto install off/on respectively.\n\n";
                break;

            case 'cpanelPluginAutoInstallOn':

                if ( CPanel::turnOnCpanelPluginAutoInstall() ) {
                    echo "Auto install is now On for LiteSpeed cPanel plugin.\n\n";
                }
                else {
                    echo "Failed to turn off auto install for LiteSpeed cPanel plugin.\n\n";
                }

                break;

            case 'cpanelPluginAutoInstallOff':

                if ( CPanel::turnOffCpanelPluginAutoInstall() ) {
                    echo "Auto install is now Off for LiteSpeed cPanel plugin.\n\n";
                }
                else {
                    echo "Failed to turn on auto install for LiteSpeed cPanel plugin.\n\n";
                }

                break;

            //no default
        }
    }

    private function doVersionCommand()
    {
        $pluginVerInstance = PluginVersion::getInstance();

        switch ($this->versionCmd) {

            case 'list':
                $allowedVers = $pluginVerInstance->getAllowedVersions();

                echo "Available versions are: \n" . implode("\n",$allowedVers) . "\n";
                break;

            case 'latest':
                $latest = $pluginVerInstance->getLatestVersion();

                try {
                    $currVer = $pluginVerInstance->getCurrentVersion();
                }
                catch ( LSCMException $e ) {
                    $currVer = '';
                }

                if ( $latest == $currVer ) {
                    echo "Current version, {$latest}, is already the latest version.\n";
                }
                else {
                    $pluginVerInstance->setActiveVersion($latest);
                }

                break;

            case 'active':
                $currVer = $pluginVerInstance->getCurrentVersion();

                echo "Current active version is {$currVer}.\n";
                break;

            default:
                $pluginVerInstance->setActiveVersion($this->versionCmd);
        }
    }

    private function doWPInstallStorageAction()
    {
        $extraArgs = array();
        $list = null;

        $lscmDataFiles = Context::getLSCMDataFiles();

        $wpInstallStorage =
                new WPInstallStorage($lscmDataFiles['dataFile'],
                        $lscmDataFiles['custDataFile']);

        if ($this->currWpPath) {
            $list = array( $this->currWpPath );
        }

        foreach ( $this->commands as $action ) {
            $this->checkDataFile($action , $wpInstallStorage);

            echo "\nPerforming {$action} operation. Please be patient...\n\n";

            switch ( $action ) {

                case 'upgrade':
                case 'mass_upgrade':
                    $pluginVerInstance = PluginVersion::getInstance();

                    $extraArgs[] = implode(',',
                            $pluginVerInstance->getKnownVersions(true));
                    $extraArgs[] = $pluginVerInstance->getCurrentVersion();
                    break;

                case 'dash_notify':
                case 'mass_dash_notify':
                    DashNotifier::prepLocalDashPluginFiles();

                    $slug = (isset($this->input['slug'])) ? $this->input['slug'] : '';

                    $msgInfoJSON = json_encode(
                            array(
                                'msg' => $this->input['msg'],
                                'plugin' => $slug,
                                'plugin_name' => ''
                            )
                    );

                    $extraArgs[] = base64_encode($msgInfoJSON);
                    break;

                case WPInstallStorage::CMD_ADD_CUST_WPINSTALLS:
                    $list = array();
                    $extraArgs[] = $this->input['addInstallsInfo'];
                    break;

                // no default case
            }

            $wpInstallStorage->doAction($action, $list, $extraArgs);

            if ( $action == 'status' ) {
                $wpInstall =
                        $wpInstallStorage->getWPInstall($this->currWpPath);

                $this->printStatusMsg($wpInstall);
            }

            echo "\n{$action} complete!\n\n";
        }
    }

    private function runCommand()
    {
        foreach ( $this->cacheRootCmds as $action ) {
            $this->doCacheRootCommand($action);
        }

        $controlPanel = ControlPanel::getClassInstance();
        $controlPanel->verifyCacheSetup();

        if ( $this->displayCacheRoots ) {
            $this->displayCacheRoots();
        }

        if ( $this->specialCmd ) {
            $this->doSpecialCommand();
        }
        elseif ( $this->versionCmd ) {
            $this->doVersionCommand();
        }
        else {
            $this->doWPInstallStorageAction();
        }
    }

    public static function run()
    {
        try
        {
            $cli = new self();
            $cli->runCommand();
        }
        catch ( \Exception $e )
        {
            echo "[ERROR] {$e->getMessage()}\n\n";
            exit(1);
        }
    }

}
