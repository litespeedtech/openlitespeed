<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2018
 * ******************************************* */

namespace Lsc\Wp;

class LSCMException extends \Exception
{

    const E_ERROR = 0;

    /**
     * show trace msg
     */
    const E_PROGRAM = 100;

    /**
     * error shown to user
     */
    const E_PERMISSION = 101;
    const E_UNSUPPORTED = 102;

    /**
     * Exception is considered non-fatal. Used to determine
     * UserCommand->runAsUser() return status.
     */
    const E_NON_FATAL = 103;

}
