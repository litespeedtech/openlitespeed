<?php

namespace LSWebAdmin\Config\Validation;

use LSWebAdmin\UI\DTbl;

class ThrottlePostTableValidationRule implements PostTableValidationRuleInterface
{
    public function Supports($request, $table)
    {
        return $table->Get(DTbl::FLD_ID) === 'ADM_THROTTLE';
    }

    public function Validate($request, $extracted)
    {
        $blockWindow = $extracted->GetChildVal('throttleBlockWindow');
        $maxBackoff = $extracted->GetChildVal('throttleMaxBackoff');

        if ($blockWindow !== null && $maxBackoff !== null) {
            $blockWindow = (int) $blockWindow;
            $maxBackoff = (int) $maxBackoff;

            if ($blockWindow > $maxBackoff) {
                $extracted->SetChildErr(
                    'throttleBlockWindow',
                    'Initial Block Duration must not exceed Maximum Block Duration.'
                );
                return -1;
            }
        }

        return 1;
    }
}
