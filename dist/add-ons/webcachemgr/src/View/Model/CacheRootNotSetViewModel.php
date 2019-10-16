<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use Lsc\Wp\Context\Context;

class CacheRootNotSetViewModel
{

    public function __construct()
    {
        /**
         * Nothing to do
         */
    }

    /**
     *
     * @return string
     */
    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/CacheRootNotSet.tpl';
    }

}
