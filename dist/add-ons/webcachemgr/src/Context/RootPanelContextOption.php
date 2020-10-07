<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2020
 * ******************************************* */

namespace Lsc\Wp\Context;

use \Lsc\Wp\Logger;

class RootPanelContextOption extends ContextOption
{

    /**
     *
     * @param string  $panelName
     */
    public function __construct( $panelName )
    {
        $invokerName = $panelName;
        $invokerType = parent::FROM_CONTROL_PANEL;
        $isRoot = parent::IS_ROOT;
        $logFileLvl = Logger::L_INFO;
        $logEchoLvl = Logger::L_NONE;
        $bufferedWrite = true;
        $bufferedEcho = true;
        parent::__construct($invokerName, $invokerType, $isRoot, $logFileLvl,
                $logEchoLvl, $bufferedWrite, $bufferedEcho);
        $this->scanDepth = 2;
        $this->batchTimeout = 60;
        $this->batchSize = 10;

        $sharedTplDir = realpath(__DIR__ . '/../View/Tpl');

        if ( !is_string($sharedTplDir) ) {
            $sharedTplDir = '/usr/local/lsws/add-ons/webcachemgr/src/View/Tpl';
        }

        $this->sharedTplDir = $sharedTplDir;
    }

}
