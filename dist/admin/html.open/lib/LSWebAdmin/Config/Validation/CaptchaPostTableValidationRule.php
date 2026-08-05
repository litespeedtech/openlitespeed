<?php

namespace LSWebAdmin\Config\Validation;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\UI\DTbl;

class CaptchaPostTableValidationRule implements PostTableValidationRuleInterface
{
    public function Supports($request, $table)
    {
        if ($table->Get(DTbl::FLD_ID) !== 'VT_SEC_RECAP') {
            return false;
        }

        $keys = [];
        foreach ($table->Get(DTbl::FLD_DATTRS) as $attr) {
            $keys[$attr->GetKey()] = true;
        }

        return isset($keys['enabled']) && isset($keys['verifyExpires']);
    }

    public function Validate($request, $extracted)
    {
        if ((string) $extracted->GetChildVal('enabled') !== '1') {
            return 1;
        }

        $verifyExpires = $extracted->GetChildVal('verifyExpires');
        if ($verifyExpires !== null && $verifyExpires !== '') {
            return 1;
        }

        if ($extracted->GetChildren('verifyExpires') == null) {
            $extracted->AddChild(new CNode('verifyExpires', ''));
        }

        $extracted->SetChildErr('verifyExpires', 'Required when Enable CAPTCHA is set to Yes.');
        return -1;
    }
}
