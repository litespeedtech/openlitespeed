<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\Product\Base\ProductBase;

class Product extends ProductBase
{
    public function getProductName()
    {
        return 'OpenLiteSpeed';
    }

    protected function getBuildProbeBinaries()
    {
        return [
            'bin/openlitespeed',
            'bin/lshttpd',
        ];
    }
}
