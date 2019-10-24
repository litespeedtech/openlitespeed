<?php

/* * ******************************************
 * LiteSpeed Web Server Cache Manager
 * @author: LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright: (c) 2018-2019
 * ******************************************* */

namespace Lsc\Wp\View\Model;

use \Lsc\Wp\Context\Context;

class MissingTplViewModel
{

    const FLD_MSG = 'msg';

    /**
     * @var string[]
     */
    private $tplData = array();

    /**
     *
     * @param string  $msg
     */
    public function __construct( $msg )
    {
        $this->init($msg);
    }

    private function init( $msg )
    {
        $this->tplData[self::FLD_MSG] = $msg;
    }

    /**
     *
     * @param string  $field
     * @return null|string
     */
    public function getTplData( $field )
    {
        if ( !isset($this->tplData[$field]) ) {
            return null;
        }

        return $this->tplData[$field];
    }

    public function getTpl()
    {
        return Context::getOption()->getSharedTplDir() . '/MissingTpl.tpl';
    }

}
