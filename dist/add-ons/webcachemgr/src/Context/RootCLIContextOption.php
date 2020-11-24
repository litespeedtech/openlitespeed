<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018
 * ******************************************* */

namespace Lsc\Wp\Context;

use \Lsc\Wp\Context\ContextOption;
use \Lsc\Wp\Logger;

class RootCLIContextOption extends ContextOption
{

    public function __construct()
    {
        $invokerName = 'lscmctl';
        $invokerType = parent::FROM_CLI;
        $isRoot = parent::IS_ROOT;
        $logFileLvl = Logger::L_INFO;
        $logEchoLvl = Logger::L_INFO;
        $bufferedWrite = true;
        $bufferedEcho = false;
        parent::__construct($invokerName, $invokerType, $isRoot, $logFileLvl,
                $logEchoLvl, $bufferedWrite, $bufferedEcho);
        $this->scanDepth = 2;
        $this->batchTimeout = 0;
        $this->batchSize = 0;
    }

}
