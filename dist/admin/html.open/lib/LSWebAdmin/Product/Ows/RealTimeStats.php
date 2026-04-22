<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\Product\Base\RealTimeStatsBase;

class RealTimeStats extends RealTimeStatsBase
{
    protected function getStatsDir()
    {
        return '/tmp/lshttpd';
    }

    protected function getReportProcessCount()
    {
        return isset($_SERVER['LSWS_CHILDREN']) ? (int) $_SERVER['LSWS_CHILDREN'] : 1;
    }
}
