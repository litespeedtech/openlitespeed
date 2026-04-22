<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\Product\Base\ConfValidationBase;

class ConfValidation extends ConfValidationBase
{
    protected function getManagedConfigFilePolicy($attr)
    {
        if ($attr->_type == 'filetp') {
            return [
                'directory' => 'conf/templates/',
                'suffix' => '.conf',
                'location_message' => 'Template file must locate within $SERVER_ROOT/conf/templates/',
                'suffix_message' => 'Template file name needs to be ".conf"',
            ];
        }

        if ($attr->_type == 'filevh') {
            return [
                'directory' => 'conf/vhosts/',
                'suffix' => '.conf',
                'location_message' => 'VHost config file must locate within $SERVER_ROOT/conf/vhosts/, suggested value is $SERVER_ROOT/conf/vhosts/$VH_NAME/vhconf.conf',
                'suffix_message' => 'VHost config file name needs to be ".conf"',
            ];
        }

        return parent::getManagedConfigFilePolicy($attr);
    }
}
