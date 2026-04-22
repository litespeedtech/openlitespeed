<?php

namespace LSWebAdmin\Product\Base;

class BlockedIpReasonMap
{
    public static function getMap()
    {
        return [
            'A' => 'BOT_UNKNOWN',
            'B' => 'BOT_OVER_SOFT',
            'C' => 'BOT_OVER_HARD',
            'D' => 'BOT_TOO_MANY_BAD_REQ',
            'E' => 'BOT_CAPTCHA',
            'F' => 'BOT_FLOOD',
            'G' => 'BOT_REWRITE_RULE',
            'H' => 'BOT_TOO_MANY_BAD_STATUS',
            'I' => 'BOT_BRUTE_FORCE',
        ];
    }

    public static function getReason($code)
    {
        $normalizedCode = strtoupper(trim((string) $code));
        $map = static::getMap();

        return isset($map[$normalizedCode]) ? $map[$normalizedCode] : 'UNKNOWN';
    }
}
