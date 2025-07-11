<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2025 LiteSpeed Technologies, Inc.
 * ******************************************* */

namespace Lsc\Wp\Context;

use Lsc\Wp\Logger;

class RootPanelContextOption extends ContextOption
{

    /**
     *
     * @param string $panelName
     */
    public function __construct( $panelName )
    {
        $logFileLvl    = Logger::L_INFO;
        $logEchoLvl    = Logger::L_NONE;
        $bufferedWrite = true;
        $bufferedEcho  = true;

        parent::__construct(
            $panelName,
            parent::FROM_CONTROL_PANEL,
            parent::IS_ROOT,
            $logFileLvl,
            $logEchoLvl,
            $bufferedWrite,
            $bufferedEcho
        );

        $this->scanDepth    = 2;
        $this->batchTimeout = 60;
        $this->batchSize    = 10;

        $sharedTplDir = realpath(__DIR__ . '/../View/Tpl');

        if ( !is_string($sharedTplDir) ) {
            $sharedTplDir = '/usr/local/lsws/add-ons/webcachemgr/src/View/Tpl';
        }

        $this->sharedTplDir = $sharedTplDir;
    }

}
