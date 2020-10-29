<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author  LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright  (c) 2018-2020
 * ******************************************* */

namespace Lsc\Wp\View;

use \Lsc\Wp\LSCMException;

class AjaxView
{

    /**
     * @var object
     */
    private $viewModel;

    /**
     * @var string
     */
    private $sharedTplDir = __DIR__;

    /**
     *
     * @param object  $viewModel
     */
    public function __construct( $viewModel )
    {
        $this->viewModel = $viewModel;
    }

    /**
     *
     * @param string  $tplID
     * @return string
     * @throws LSCMException  Thrown directly and indirectly.
     */
    public function getAjaxContent( $tplID = '' )
    {
        ob_start();
        try
        {
            $this->loadAjaxTpl($this->viewModel->getTpl($tplID));
        }
        catch ( LSCMException $e )
        {
            ob_clean();
            throw $e;
        }

        return ob_get_clean();
    }

    /**
     *
     * @param string  $tplPath
     * @throws LSCMException
     */
    private function loadAjaxTpl( $tplPath )
    {
        $tplFile = basename($tplPath);
        $custTpl = "{$this->sharedTplDir}/Cust/{$tplFile}";

        if ( file_exists($custTpl) ) {
            include $custTpl;
        }
        elseif ( file_exists($tplPath) ) {
            include $tplPath;
        }
        else {
            throw new LSCMException("Could not load ajax template {$tplPath}.");
        }
    }

}
