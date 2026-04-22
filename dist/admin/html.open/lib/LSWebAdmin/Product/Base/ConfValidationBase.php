<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\Config\Validation\CValidation;
use LSWebAdmin\UI\DTbl;

class ConfValidationBase extends CValidation
{
    protected function validateManagedConfigFilePolicy($attr, $path, &$err)
    {
        $policy = $this->getManagedConfigFilePolicy($attr);
        if (!is_array($policy)) {
            return 1;
        }

        $directory = isset($policy['directory']) ? $policy['directory'] : '';
        $suffix = isset($policy['suffix']) ? $policy['suffix'] : '';
        $locationMessage = isset($policy['location_message']) ? $policy['location_message'] : '';
        $suffixMessage = isset($policy['suffix_message']) ? $policy['suffix_message'] : '';

        if ($directory !== '' && strpos($path, SERVER_ROOT . $directory) !== 0) {
            $err = $locationMessage;
            return -1;
        }

        if ($suffix !== '' && !$this->endsWith($path, $suffix)) {
            $err = $suffixMessage;
            return -1;
        }

        return 1;
    }

    protected function getManagedConfigFilePolicy($attr)
    {
        return null;
    }

    protected function isValidAttr($attr, $cval)
    {
        $isValid = parent::isValidAttr($attr, $cval);

        if ($isValid == 1 && $attr->_type == 'modulename') {
            $res = $this->chkAttrModuleName($cval);
            if ($res != 1) {
                return $res;
            }
        }

        return $isValid;
    }

    protected function chkAttrModuleName($cval)
    {
        $name = $cval->Get(CNode::FLD_VAL);
        if (preg_match("/[<>&%\s]/", $name)) {
            $cval->SetErr('invalid characters in name');
            return -1;
        }

        return 1;
    }

    protected function validatePostTbl($tbl, $extracted)
    {
        if ($tbl->Get(DTbl::FLD_ID) == 'S_MOD') {
            return $this->chkPostTblServModule($extracted);
        }

        return parent::validatePostTbl($tbl, $extracted);
    }

    protected function chkPostTblServModule($extracted)
    {
        $isValid = 1;

        $name = $extracted->GetChildVal('name');
        if ($extracted->GetChildVal('internal') == 0) {
            if ($name != 'cache') {
                $module = SERVER_ROOT . "modules/{$name}.so";
                if (!file_exists($module)) {
                    $extracted->SetChildErr('name', "cannot find external module: $module");
                    $isValid = -1;
                }
            } else {
                $extracted->SetChildErr('internal', 'This is a built-in internal module');
                $isValid = -1;
            }
        }

        return $isValid;
    }

    protected function endsWith($value, $suffix)
    {
        $suffixLength = strlen($suffix);
        if ($suffixLength === 0) {
            return true;
        }

        if (!is_string($value) || strlen($value) < $suffixLength) {
            return false;
        }

        return substr_compare($value, $suffix, -$suffixLength, $suffixLength) === 0;
    }
}
