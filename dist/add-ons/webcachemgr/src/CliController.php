<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author Michael Alegre
 * @copyright (c) 2018-2023 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp;

use Exception;
use Lsc\Wp\Context\Context;
use Lsc\Wp\Panel\ControlPanel;
use Lsc\Wp\Panel\CPanel;

class CliController
{

    /**
     * @since 1.13.5
     * @var string
     */
    const SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_OFF = 'cpanelPluginAutoInstallOff';

    /**
     * @since 1.13.5
     * @var string
     */
    const SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_ON = 'cpanelPluginAutoInstallOn';

    /**
     * @since 1.13.5
     * @var string
     */
    const SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_STATUS = 'cpanelPluginAutoInstallStatus';

    /**
     * @since 1.13.5
     * @var string
     */
    const SPECIAL_CMD_CPANEL_PLUGIN_FIX_CONF = 'cpanelPluginFixConf';

    /**
     * @since 1.13.5
     * @var string
     */
    const SPECIAL_CMD_CPANEL_PLUGIN_INSTALL = 'cpanelPluginInstall';

    /**
     * @since 1.13.5
     * @var string
     */
    const SPECIAL_CMD_CPANEL_PLUGIN_UNINSTALL = 'cpanelPluginUninstall';

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
     * @var bool
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
     * @var null|array
     */
    private $input;

    /**
     *
     * @throws LSCMException  Thrown when script argument count is less than
     *     expected.
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
     * @param string           $action
     * @param WPInstallStorage $wpInstallStorage
     *
     * @return void
     *
     * @throws LSCMException  Thrown when scan data cannot be read.
     * @throws LSCMException  Thrown when expected scan data format has changed.
     * @throws LSCMException  Thrown when attempting to execute a "mass"
     *     operation without any discovered installations.
     */
    private function checkDataFile(
                         $action,
        WPInstallStorage $wpInstallStorage )
    {
        if ( $action == WPInstallStorage::CMD_SCAN2 ) {

            /**
             * Always allowed.
             */
            return;
        }

        if ( ($err = $wpInstallStorage->getError()) ) {

            switch ($err) {

                case WPInstallStorage::ERR_NOT_EXIST:
                case WPInstallStorage::ERR_CORRUPTED:
                case WPInstallStorage::ERR_VERSION_HIGH:
                    throw new LSCMException(
                        'Scan data could not be read! Please scan again '
                            . '(without the \'-n\' flag) before attempting any '
                            . "cache operations.\n"
                    );

                case WPInstallStorage::ERR_VERSION_LOW:
                    throw new LSCMException(
                        'Scan data file format has been changed for this '
                            . 'version. Please scan again (without the \'-n\' '
                            . "flag) before attempting any cache operations.\n"
                    );

                //no default
            }
        }
        elseif ( strpos($action,'mass_') === 0
                && $wpInstallStorage->getCount() == 0 ) {

            throw new LSCMException(
                'No WordPress installations discovered in the previous scan. '
                    . 'If you have any newly installed WordPress '
                    . 'installations, please scan again or add them with '
                    . "command 'addinstalls'.\n"
            );
        }
    }

    /**
     *
     * @param WPInstall $wpInstall
     */
    private function printStatusMsg( WPInstall $wpInstall )
    {
        $msg = "$this->currWpPath - Status: ";

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
            $msg .= 'Could not match WordPress siteURL to a known control '
                . 'panel docroot.';
        }
        elseif ( $status & WPInstall::ST_ERR_EXECMD ) {
            $msg .= 'WordPress fatal error encountered during action '
                . 'execution. This is most likely caused by custom code in '
                . 'this WordPress installation.';
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

        echo "$msg\n\n";
    }

    /**
     *
     * @param string[] $args
     *
     * @return void
     *
     * @throws LSCMException  Thrown when -svr parameter is passed without a
     *     value.
     * @throws LSCMException  Thrown when -svr parameter value points to a
     *     non-empty directory.
     * @throws LSCMException  Thrown when -vh parameter is passed without a
     *     value.
     * @throws LSCMException  Thrown when -vh parameter value contains invalid
     *     character '$'.
     * @throws LSCMException  Thrown when -vh parameter value points to a
     *     non-empty directory.
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->getServerCacheRoot() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->getVHCacheRoot() call.
     */
    private function handleSetCacheRootInput( array &$args )
    {
        if ( empty($args) ) {
            $this->cacheRootCmds[] = 'listCacheRoots';
            return;
        }

        $controlPanel = ControlPanel::getClassInstance();

        if ( ($key = array_search('-svr', $args)) !== false ) {

            if ( empty($args[$key + 1])
                    || ($this->svrCacheRootParam = trim($args[$key + 1])) == '' ) {

                throw new LSCMException(
                    'Invalid Command, missing server cache root value.'
                );
            }

            $currSvrCacheRoot = $controlPanel->getServerCacheRoot();

            if ( $this->svrCacheRootParam != $currSvrCacheRoot ) {

                if ( !Util::is_dir_empty($this->svrCacheRootParam) ) {
                    throw new LSCMException(
                        'Provided server level cache root must be an empty '
                            . 'directory.'
                    );
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
                    'Invalid Command, missing virtual host cache root value.'
                );
            }

            if ( strpos($this->vhCacheRootParam, '$') !== false ) {
                throw new LSCMException(
                    'Invalid Command, virtual host cache root value cannot '
                        . 'contain any \'$\' characters. \'$vh_user\' will be '
                        . 'automatically added to the end of virtual host '
                        . 'cache root values starting with a \'/\'.'
                );
            }

            $currVHCacheRoot = $controlPanel->getVHCacheRoot();

            if ( $this->vhCacheRootParam[0] == '/' ) {
                $updatedVhCacheRoot =
                    rtrim($this->vhCacheRootParam, '/') . '/$vh_user';

                if ( $updatedVhCacheRoot != $currVHCacheRoot
                        && ! Util::is_dir_empty($this->vhCacheRootParam) ) {

                    throw new LSCMException(
                        'Provided absolute path for virtual host level cache '
                            . 'root must be an empty directory.'
                    );
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
     * @param string[] $args
     *
     * @return void
     *
     * @throws LSCMException  Thrown when passed version set value is invalid.
     */
    private function handleSetVersionInput( array &$args )
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

        if ( preg_match('/[1-9]\.\d(?:\.\d)*(?:\.\d)*/', $v) !== 1 ) {
            throw new LSCMException("Invalid version number ($v).");
        }

        $this->versionCmd = $v;
    }

    /**
     *
     * @param string[] $args
     */
    private function handleScanInput( array &$args )
    {
        if ( ($key = array_search('-n', $args)) !== false ) {
            unset($args[$key]);
            $this->commands[] = WPInstallStorage::CMD_DISCOVER_NEW2;
        }
        else {
            $this->commands[] = WPInstallStorage::CMD_SCAN2;
        }

        if ( ($key = array_search('-e', $args)) !== false ) {
            unset($args[$key]);
            $this->commands[] = UserCommand::CMD_MASS_ENABLE;
        }
    }

    /**
     *
     * @param string[] $args
     */
    private function handleScanNewInput( array &$args )
    {
        if ( ($key = array_search('-en', $args)) !== false ) {
            unset($args[$key]);
            $this->commands[] = WPInstallStorage::CMD_DISCOVER_NEW_AND_ENABLE;
        }
        else {
            $this->commands[] = WPInstallStorage::CMD_DISCOVER_NEW2;
        }
    }

    /**
     *
     * @param string[] $args
     *
     * @return bool
     */
    private function isMassOperation( array &$args )
    {
        if ( ($key = array_search('-m', $args)) !== false ) {
            unset($args[$key]);
            return true;
        }

        return false;
    }

    /**
     *
     * @param string   $cmd
     * @param string[] $args
     *
     * @throws LSCMException  Thrown when expected WP path value is not
     *     provided.
     * @throws LSCMException  Thrown when provided an invalid WP path value.
     */
    private function handleSingleOperationInput( $cmd, array &$args )
    {
        $path = array_shift($args);

        if ( $path == null ) {
            throw new LSCMException('Invalid Command, missing WP path.');
        }

        $wpInstall = new WPInstall($path);

        if ( !$wpInstall->hasValidPath() ) {
            throw new LSCMException("Invalid WP Path: $path.");
        }

        $this->commands[] = $cmd;
        $this->currWpPath = rtrim($path, '/');
    }

    /**
     *
     * @param string   $cmdType
     * @param string[] $args
     *
     * @throws LSCMException  Thrown when neither -m nor -wppath parameters are
     *     provided in notify command.
     * @throws LSCMException  Thrown when -wppath parameter is passed without a
     *     value in notify command.
     * @throws LSCMException  Thrown when provided -wppath parameter value is
     *     invalid in notify command.
     * @throws LSCMException  Thrown when -msgfile parameter is passed without a
     *     value in notify command.
     * @throws LSCMException  Thrown when provided -msgfile parameter value
     *     points to a non-existent file in notify command.
     * @throws LSCMException  Thrown when unable to get file contents of the
     *     file pointed to by provided -msgfile value in notify command.
     * @throws LSCMException  Thrown when neither -msgfile nor -msg parameters
     *     are provided in notify command.
     * @throws LSCMException  Thrown when parameter -msg is passed without a
     *     value in notify command.
     * @throws LSCMException  Thrown when expected -plugin parameter is not
     *     provided in notify command.
     * @throws LSCMException  Thrown when provided -plugin parameter value does
     *     not a known plugin slug in notify command.
     * @throws LSCMException  Thrown when neither -m flag nor WP path value are
     *     provided in remove command.
     * @throws LSCMException  Thrown when provided WP path value is invalid in
     *     remove command.
     */
    private function handleDashNotifyInput( $cmdType, array &$args )
    {
        if ( $cmdType == 'notify' ) {

            if ( ($key = array_search('-m', $args)) !== false ) {
                unset($args[$key]);
                $this->commands[] = UserCommand::CMD_MASS_DASH_NOTIFY;
            }
            else {

                if ( ($key = array_search('-wppath', $args)) === false ) {
                    throw new LSCMException(
                        'Invalid Command, missing required \'-m\' or '
                            . '\'-wppath\' parameter.'
                    );
                }

                if ( empty($args[$key + 1]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'-wppath\' value.'
                    );
                }

                $path = $args[$key + 1];

                $wpInstall = new WPInstall($path);

                if ( !$wpInstall->hasValidPath() ) {
                    throw new LSCMException("Invalid WP Path: $path.");
                }

                $this->commands[] = UserCommand::CMD_DASH_NOTIFY;
                $this->currWpPath = rtrim($path, '/');

                unset($args[$key], $args[$key + 1]);
            }

            if ( ($key = array_search('-msgfile', $args)) !== false ) {

                if ( empty($args[$key + 1]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'-msgfile\' value.'
                    );
                }

                $msgFilePath = $args[$key + 1];

                if ( !file_exists($msgFilePath) ) {
                    throw new LSCMException(
                        'Provided message file does not exist.'
                    );
                }

                if ( ($message = file_get_contents($msgFilePath)) === false ) {
                    throw new LSCMException(
                        'Unable to retrieve provided message file content.'
                    );
                }
            }
            else {

                if ( ($key = array_search('-msg', $args)) === false ) {
                    throw new LSCMException(
                        'Invalid Command, missing required \'-msgfile\' or '
                            . '\'-msg\' parameter.'
                    );
                }

                if ( empty($args[$key + 1]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'-msg\' value.'
                    );
                }

                $message = $args[$key + 1];
            }

            $dashInput = array( 'msg' => $message );

            unset($args[$key], $args[$key + 1]);

            if ( ($key = array_search('-plugin', $args)) !== false ) {

                if ( empty($args[$key + 1]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'-plugin\' value.'
                    );
                }

                $slug = $args[$key + 1];

                $pluginInfoJSON = Util::get_url_contents(
                    "https://api.wordpress.org/plugins/info/1.0/$slug.json"
                );

                $pluginInfo = json_decode($pluginInfoJSON, true);

                if ( empty($pluginInfo['name']) ) {
                    throw new LSCMException(
                        'Could not find a plugin matching the provided plugin '
                            . 'slug.'
                    );
                }

                $dashInput['slug'] = $slug;

                unset($args[$key], $args[$key + 1]);
            }

            $this->input = $dashInput;
        }
        elseif ( $cmdType == 'remove' ) {
            $arg1 = array_shift($args);

            if ( $arg1 == '-m' ) {
                $this->commands[] = UserCommand::CMD_MASS_DASH_DISABLE;
            }
            else {
                $path = $arg1;

                if ( $path == null ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'-m\' flag or WP path value.'
                    );
                }

                $wpInstall = new WPInstall($path);

                if ( !$wpInstall->hasValidPath() ) {
                    throw new LSCMException("Invalid WP Path: $path.");
                }

                $this->commands[] = UserCommand::CMD_DASH_DISABLE;
                $this->currWpPath = rtrim($path, '/');
            }
        }
    }

    /**
     *
     * @param string[] $args
     *
     * @throws LSCMException  Thrown when command 'cpanelplugin' is used in a
     *     non-cPanel environment.
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     */
    private function handleCpanelPluginInput( array &$args )
    {
        $controlPanel = ControlPanel::getClassInstance();

        if ( !($controlPanel instanceof CPanel) ) {
            throw new LSCMException(
                'Command \'cpanelplugin\' cannot be used in a non-cPanel '
                    . 'environment.'
            );
        }

        switch ($args[0]) {

            case '--install':
                $this->specialCmd = self::SPECIAL_CMD_CPANEL_PLUGIN_INSTALL;
                unset($args[0]);
                break;

            case '--uninstall':
                $this->specialCmd = self::SPECIAL_CMD_CPANEL_PLUGIN_UNINSTALL;
                unset($args[0]);
                break;

            case '-autoinstall':

                if ( !isset($args[1]) ) {
                    $this->specialCmd =
                        self::SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_STATUS;
                }
                elseif ( "$args[1]" === '1' ) {
                    $this->specialCmd =
                        self::SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_ON;
                    unset($args[1]);
                }
                elseif ( "$args[1]" === '0' ) {
                    $this->specialCmd =
                        self::SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_OFF;
                    unset($args[1]);
                }

                unset($args[0]);
                break;

            case '--fixconf':
                $this->specialCmd = self::SPECIAL_CMD_CPANEL_PLUGIN_FIX_CONF;
                unset($args[0]);
                break;

            //no default
        }
    }

    /**
     *
     * @param string[] $args
     *
     * @throws LSCMException  Thrown when -wpinstall parameter is passed without
     *     a value.
     * @throws LSCMException  Thrown when expected 'docroot' value is not
     *     provided when using parameter -wpinstall.
     * @throws LSCMException  Thrown when expected 'server name' value is not
     *     provided when using parameter -wpinstall.
     * @throws LSCMException  Thrown when expected 'site url' value is not
     *     provided when using parameter -wpinstall.
     * @throws LSCMException  Thrown when -wpinstallsfile parameter is passed
     *     without a value.
     * @throws LSCMException  Thrown when provided -wpinstallsfile parameter
     *     value points to a non-existent file.
     * @throws LSCMException  Thrown when unable to get file contents of the
     *     file pointed to by provided -wpinstallsfile parameter value.
     */
    private function handleAddInstallsInput( array &$args )
    {
        switch ($args[0]) {

            case '-wpinstall':

                if ( empty($args[1]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'<wp path>\' value.'
                    );
                }

                $wpInstallsInfo = "$args[1]";

                if ( empty($args[2]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'<docroot>\' value.'
                    );
                }

                $wpInstallsInfo .= " $args[2]";

                if ( empty($args[3]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'<server name>\' value.'
                    );
                }

                $wpInstallsInfo .= " $args[3]";

                if ( empty($args[4]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'<site url>\' value.'
                    );
                }

                $wpInstallsInfo .= " $args[4]";

                $this->commands[] = WPInstallStorage::CMD_ADD_CUST_WPINSTALLS;

                $this->input =
                    array( 'addInstallsInfo' => array($wpInstallsInfo) );

                unset($args[0], $args[1], $args[2], $args[3], $args[4]);
                break;

            case '-wpinstallsfile':

                if ( empty($args[1]) ) {
                    throw new LSCMException(
                        'Invalid Command, missing \'-wpinstallsfile\' value.'
                    );
                }

                $wpInstallsFile = $args[1];

                if ( !file_exists($wpInstallsFile) ) {
                    throw new LSCMException(
                        'Provided wpinstalls file does not exist.'
                    );
                }

                $fileContent = file_get_contents($wpInstallsFile);

                if ( $fileContent === false ) {
                    throw new LSCMException(
                        'Could not read wpinstalls file content.'
                    );
                }

                $this->commands[] = WPInstallStorage::CMD_ADD_CUST_WPINSTALLS;

                $this->input = array(
                    'addInstallsInfo' => explode("\n", trim($fileContent))
                );

                unset($args[0], $args[1]);
                break;

            // no default
        }
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->getServerCacheRoot() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->getVHCacheRoot() call.
     */
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

Server Cache Root:        $svrCacheRoot
Virtual Host Cache Root:  $vhCacheRoot


EOF;
    }

    /**
     *
     * @param string[] $args
     *
     * @throws LSCMException  Thrown when command setcacheroot is used in the
     *     CustomPanel context.
     * @throws LSCMException  Thrown when an unrecognized command is provided.
     * @throws LSCMException  Thrown when not all provided command arguments are
     *     recognized or used when provided with a valid command.
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->handleSetCacheRootInput() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->handleSetVersionInput() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->handleSingleOperationInput() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->handleDashNotifyInput() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->handleDashNotifyInput() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->handleCpanelPluginInput() call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->handleAddInstallsInput() call.
     */
    private function parseCommands( array $args )
    {
        $panelClassName = array_shift($args);

        /**
         * Initialize ControlPanel instance.
         */
        ControlPanel::getClassInstance($panelClassName);

        $cmd = array_shift($args);

        switch ( $cmd ) {

            case 'setcacheroot':

                if ( $panelClassName == 'custom' ) {
                    throw new LSCMException(
                        'Command \'setcacheroot\' cannot be used in the '
                            . 'CustomPanel context.'
                    );
                }

                $this->handleSetCacheRootInput($args);
                break;

            case 'setversion':
                $this->handleSetVersionInput($args);
                break;

            case 'scan':
                $this->handleScanInput($args);
                break;

            case 'scannew':
                $this->handleScanNewInput($args);
                break;

            case 'enable':
            case 'disable':
            case 'upgrade':
            /** @noinspection PhpMissingBreakStatementInspection */
            case 'unflag':

                if ( $this->isMassOperation($args) ) {
                    $this->commands[] = "mass_$cmd";
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
     * @param string $action
     *
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->setServerCacheRoot() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->setVHCacheRoot() call.
     * @throws LSCMException  Thrown indirectly by Util::restartLsws() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->getVHCacheRoot() call.
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

        Util::ensureVHCacheRootInCage($controlPanel->getVHCacheRoot());
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->installCpanelPlugin() call.
     * @throws LSCMException  Thrown indirectly by
     *     CPanel::uninstallCpanelPlugin() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->updateCoreCpanelPluginConfSettings() call.
     */
    private function doSpecialCommand()
    {
        $controlPanel = ControlPanel::getClassInstance();

        switch($this->specialCmd) {

            case self::SPECIAL_CMD_CPANEL_PLUGIN_INSTALL:
                /* @var $controlPanel CPanel */

                switch ( $controlPanel->installCpanelPlugin() ) {

                    case 'update':
                        echo 'Updated LiteSpeed cPanel plugin to current '
                            . "version\n\n";
                        break;

                    case 'new':
                        echo 'LiteSpeed cPanel plugin installed, auto install '
                            . "turned on.\n\n";
                        break;

                    //no default
                }

                break;

            case self::SPECIAL_CMD_CPANEL_PLUGIN_UNINSTALL:
                CPanel::uninstallCpanelPlugin();

                echo 'LiteSpeed cPanel plugin uninstalled successfully, auto '
                    . "install turned off.\n\n";
                break;

            case self::SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_STATUS:
                $state = (CPanel::isCpanelPluginAutoInstallOn()) ? 'On' : 'Off';

                echo "Auto install is currently $state for the LiteSpeed "
                    . "cPanel plugin.\n";
                echo 'Use command \'cpanelplugin -autoinstall {0 | 1}\' to '
                    . "turn auto install off/on respectively.\n\n";
                break;

            case self::SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_ON:

                if ( CPanel::turnOnCpanelPluginAutoInstall() ) {
                    echo 'Auto install is now On for LiteSpeed cPanel plugin.'
                        . "\n\n";
                }
                else {
                    echo 'Failed to turn off auto install for LiteSpeed cPanel '
                        . "plugin.\n\n";
                }

                break;

            case self::SPECIAL_CMD_CPANEL_PLUGIN_AUTOINSTALL_OFF:

                if ( CPanel::turnOffCpanelPluginAutoInstall() ) {
                    echo 'Auto install is now Off for LiteSpeed cPanel plugin.'
                        . "\n\n";
                }
                else {
                    echo 'Failed to turn on auto install for LiteSpeed cPanel '
                        . "plugin.\n\n";
                }

                break;

            case self::SPECIAL_CMD_CPANEL_PLUGIN_FIX_CONF:
                /* @var $controlPanel CPanel */
                $controlPanel->updateCoreCpanelPluginConfSettings();

                echo "Attempted to fix user-end cPanel Plugin conf.\n\n";
                break;

            //no default
        }
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by PluginVersion::getInstance()
     *     call.
     * @throws LSCMException  Thrown indirectly by
     *     $pluginVerInstance->getAllowedVersions() call.
     * @throws LSCMException  Thrown indirectly by
     *     $pluginVerInstance->getLatestVersion() call.
     * @throws LSCMException  Thrown indirectly by
     *     $pluginVerInstance->setActiveVersion() call.
     * @throws LSCMException  Thrown indirectly by
     *     $pluginVerInstance->getCurrentVersion() call.
     * @throws LSCMException  Thrown indirectly by
     *     $pluginVerInstance->setActiveVersion() call.
     */
    private function doVersionCommand()
    {
        $pluginVerInstance = PluginVersion::getInstance();

        switch ( $this->versionCmd ) {

            case 'list':

                echo "Available versions are: \n"
                    . implode("\n",$pluginVerInstance->getAllowedVersions())
                    . "\n";
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
                    echo "Current version, $latest, is already the latest "
                        . "version.\n";
                }
                else {
                    $pluginVerInstance->setActiveVersion($latest);
                }

                break;

            case 'active':

                echo "Current active version is "
                    . $pluginVerInstance->getCurrentVersion()
                    .  ".\n";
                break;

            default:
                $pluginVerInstance->setActiveVersion($this->versionCmd);
        }
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Context::getLSCMDataFiles()
     *     call.
     * @throws LSCMException  Thrown indirectly by "new WPInstallStorage()"
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->checkDataFile() call.
     * @throws LSCMException  Thrown indirectly by PluginVersion::getInstance()
     *     call.
     * @throws LSCMException  Thrown indirectly by
     *     $pluginVerInstance->getShortVersions() call.
     * @throws LSCMException  Thrown indirectly by
     *     $pluginVerInstance->getCurrentVersion() call.
     * @throws LSCMException  Thrown indirectly by
     *     DashNotifier::prepLocalDashPluginFiles() call.
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance()->getDocRoots() call.
     * @throws LSCMException  Thrown indirectly by $wpInstallStorage->scan2()
     *     call.
     * @throws LSCMException  Thrown indirectly by $wpInstallStorage->doAction()
     *     call.
     */
    private function doWPInstallStorageAction()
    {
        $extraArgs = array();
        $list = null;
        $originalAction = null;

        $lscmDataFiles = Context::getLSCMDataFiles();

        $wpInstallStorage = new WPInstallStorage(
            $lscmDataFiles['dataFile'],
            $lscmDataFiles['custDataFile']
        );

        if ( $this->currWpPath ) {
            $list = array( $this->currWpPath );
        }

        foreach ( $this->commands as $action ) {
            $this->checkDataFile($action, $wpInstallStorage);

            echo "\nPerforming $action operation. Please be patient...\n\n";

            switch ( $action ) {

                case UserCommand::CMD_UPGRADE:
                case UserCommand::CMD_MASS_UPGRADE:
                    $pluginVerInstance = PluginVersion::getInstance();

                    $extraArgs[] = implode(
                        ',',
                        $pluginVerInstance->getShortVersions()
                    );
                    $extraArgs[] = $pluginVerInstance->getCurrentVersion();

                    break;

                case UserCommand::CMD_DASH_NOTIFY:
                case UserCommand::CMD_MASS_DASH_NOTIFY:
                    DashNotifier::prepLocalDashPluginFiles();

                    $slug = '';

                    if ( isset($this->input['slug']) ) {
                        $slug = $this->input['slug'];
                    }

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

                case WPInstallStorage::CMD_SCAN2:
                case WPInstallStorage::CMD_DISCOVER_NEW2:
                case WPInstallStorage::CMD_DISCOVER_NEW_AND_ENABLE:
                    $wpPaths = array();
                    $docroots = ControlPanel::getClassInstance()->getDocRoots();

                    foreach ( $docroots as $docroot ) {
                        $wpPaths = array_merge(
                            $wpPaths,
                            WPInstallStorage::scan2($docroot)
                        );
                    }

                    $list = array();

                    if ( $action == WPInstallStorage::CMD_DISCOVER_NEW2
                            || $action == WPInstallStorage::CMD_DISCOVER_NEW_AND_ENABLE ) {

                        foreach( $wpPaths as $wpPath ) {

                            if ( $wpInstallStorage->getWPInstall($wpPath) == null ) {
                                $list[] = $wpPath;
                            }
                        }
                    }
                    else {
                        $list = array_merge($list, $wpPaths);
                    }

                    $originalAction = $action;
                    $action = WPInstallStorage::CMD_ADD_NEW_WPINSTALL;

                    break;

                // no default case
            }

            $wpInstallStorage->doAction($action, $list, $extraArgs);

            if ( $originalAction == WPInstallStorage::CMD_DISCOVER_NEW_AND_ENABLE ) {
                $wpInstallStorage->doAction(
                    UserCommand::CMD_MASS_ENABLE,
                    $list
                );
            }

            if ( $action == UserCommand::CMD_STATUS ) {
                $wpInstall = $wpInstallStorage->getWPInstall($this->currWpPath);

                $this->printStatusMsg($wpInstall);
            }

            if ( $originalAction != null ) {
                $action = $originalAction;
            }

            echo "\n$action complete!\n\n";
        }
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by $this->doCacheRootCommand()
     *     call.
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     $controlPanel->verifyCacheSetup() call.
     * @throws LSCMException  Thrown indirectly by $this->displayCacheRoots()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->doSpecialCommand()
     *     call.
     * @throws LSCMException  Thrown indirectly by $this->doVersionCommand()
     *     call.
     * @throws LSCMException  Thrown indirectly by
     *     $this->doWPInstallStorageAction() call.
     */
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
        try {
            $cli = new self();
            $cli->runCommand();
        }
        catch ( Exception $e ) {
            echo "[ERROR] {$e->getMessage()}\n\n";
            exit(1);
        }
    }

}
