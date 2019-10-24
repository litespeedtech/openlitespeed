<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @Author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @Copyright: (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp\Context;

use \Lsc\Wp\Context\ContextOption;
use \Lsc\Wp\Logger;

class UserCLIContextOption extends ContextOption
{

    public function __construct( $invokerName )
    {
        $invokerType = parent::FROM_CLI;
        $isRoot = parent::IS_NOT_ROOT;

        /**
         * Do not change log levels as it will break UserCommand output format.
         */
        $logFileLvl = Logger::L_NONE;
        $logEchoLvl = Logger::L_NONE;

        $bufferedWrite = false;
        $bufferedEcho = false;
        parent::__construct($invokerName, $invokerType, $isRoot, $logFileLvl,
                $logEchoLvl, $bufferedWrite, $bufferedEcho);

        $this->expectedPermissions = self::IS_NOT_ROOT;
        $this->scanDepth = 2;
        $this->batchTimeout = 0;
        $this->batchSize = 0;
        $this->logFile = '';
    }

}
