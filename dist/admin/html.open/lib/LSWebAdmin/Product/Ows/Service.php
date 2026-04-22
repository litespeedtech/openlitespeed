<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\Controller\ControllerBase;

class Service extends ControllerBase
{
    protected function getConfigDataClass()
    {
        return ConfigData::class;
    }

    protected function getPidFile()
    {
        return '/tmp/lshttpd/lshttpd.pid';
    }

    protected function getStatusFile()
    {
        return '/tmp/lshttpd/.status';
    }
}
