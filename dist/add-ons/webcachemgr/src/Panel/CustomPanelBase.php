<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2020-2023
 * @since 1.10
 * ******************************************* */

namespace Lsc\Wp\Panel;

abstract class CustomPanelBase
extends ControlPanel
{
    /**
     * The following functions deal with apache configuration files and
     * server/virtual host cache roots and will never be called in the
     * CustomPanel context. They are included here to meet abstract function
     * requirements in parent class ControlPanel.
     *
     */
    protected function initConfPaths()
    {}
    protected function serverCacheRootSearch()
    {}
    protected function vhCacheRootSearch()
    {}
    protected function addVHCacheRootSection(
        array $file_contents,
              $vhCacheRoot = 'lscache' )
    {}
    public function verifyCacheSetup()
    {}
    public function createVHConfAndSetCacheRoot(
        $vhConf,
        $vhCacheRoot = 'lscache' )
    {}
    public function applyVHConfChanges()
    {}

}
