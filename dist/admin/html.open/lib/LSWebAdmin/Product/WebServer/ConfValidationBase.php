<?php

namespace LSWebAdmin\Product\WebServer;

use LSWebAdmin\Config\Validation\ConfigDeleteValidationResult;
use LSWebAdmin\Product\Base\ConfValidationBase as ProductConfValidationBase;

class ConfValidationBase extends ProductConfValidationBase
{
    public function ValidateDelete($request)
    {
        $externalAppResult = $this->validateExternalAppDelete($request);
        if (!$externalAppResult->IsAllowed() || $externalAppResult->HasWarnings()) {
            return $externalAppResult;
        }

        return ConfigDeleteValidationResult::allow();
    }
}
