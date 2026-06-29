<?php

namespace LSWebAdmin\Product\WebServer\Ols;

use LSWebAdmin\Product\Base\RealTimeStatsBase as ProductRealTimeStatsBase;

class RealTimeStatsBase extends ProductRealTimeStatsBase
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
