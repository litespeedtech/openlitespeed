<?php

namespace LSWebAdmin\Config\Service;

class ConfigDataSelectionService
{
    public static function resolve($request, $serverData, $adminData, $currentData, $specialData, $allowCurrentData = true)
    {
        if ($request->UsesSpecialData()) {
            return $specialData;
        }

        if ($allowCurrentData && $request->UsesCurrentData()) {
            return $currentData;
        }

        if ($request->IsAdminConf()) {
            return $adminData;
        }

        return $serverData;
    }
}
