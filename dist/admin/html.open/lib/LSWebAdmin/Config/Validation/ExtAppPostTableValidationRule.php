<?php

namespace LSWebAdmin\Config\Validation;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\UI\DTbl;

class ExtAppPostTableValidationRule implements PostTableValidationRuleInterface
{
    private static $supportedTables = [
        'SV_EXT_FCGI' => true,
        'SV_EXT_FCGIAUTH' => true,
        'SV_EXT_LSAPI' => true,
        'T_EXT_FCGI' => true,
        'T_EXT_FCGIAUTH' => true,
        'T_EXT_LSAPI' => true,
    ];

    public function Supports($request, $table)
    {
        return isset(self::$supportedTables[$table->Get(DTbl::FLD_ID)]);
    }

    public function Validate($request, $extracted)
    {
        $autoStart = $extracted->GetChildVal('autoStart');
        if ($autoStart > 0 && empty($extracted->GetChildVal('path'))) {
            if ($extracted->GetChildren('path') == null) {
                $extracted->AddChild(new CNode('path', ''));
            }

            $extracted->SetChildErr('path', 'Required field when auto start by Server is enabled.');
            return -1;
        }

        return 1;
    }
}
