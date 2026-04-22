<?php

namespace LSWebAdmin\Config\Load;

use LSWebAdmin\Util\PathTool;

class ConfigLoadPlanner
{
    public static function resolveCurrentTarget($request, $allowCurrentData)
    {
        if (!$allowCurrentData || !$request->NeedsCurrentData()) {
            return null;
        }

        $type = $request->GetCurrentDataType();
        $name = $request->GetCurrentDataName();
        if (!is_string($name) || $name === '') {
            return null;
        }

        return ConfigLoadTarget::currentData($type, $name);
    }

    public static function resolveSpecialTarget($request, $serverData, $currentData)
    {
        if ($request->IsMimeDataRequested()) {
            return self::resolveMimeDataTarget($serverData);
        }

        if ($request->IsAdminUsersDataRequested()) {
            return ConfigLoadTarget::specialData(SERVER_ROOT . 'admin/conf/htpasswd', 'ADMUSR');
        }

        $realmDataId = $request->GetRealmDataId();
        if ($realmDataId != null) {
            return self::resolveRealmDataTarget($request, $currentData, $realmDataId);
        }

        return null;
    }

    private static function resolveMimeDataTarget($serverData)
    {
        if ($serverData == null) {
            return null;
        }

        $mime = $serverData->GetChildrenValues('mime');
        if (!isset($mime[0]) || !is_string($mime[0]) || $mime[0] === '') {
            return null;
        }

        return ConfigLoadTarget::specialData(PathTool::GetAbsFile($mime[0], 'SR'), 'MIME');
    }

    private static function resolveRealmDataTarget($request, $currentData, $realmDataId)
    {
        if ($currentData == null) {
            return null;
        }

        $realm = $currentData->GetChildNodeById('realm', $request->GetFirstRef());
        if ($realm == null) {
            return null;
        }

        $locationKey = ($realmDataId == 'V_UDB') ? 'userDB:location' : 'groupDB:location';
        $file = $realm->GetChildVal($locationKey);
        if (!is_string($file) || $file === '') {
            return null;
        }

        $vhName = $request->GetViewName();
        $vhRoot = $request->GetVHRoot();
        if (!is_string($vhName) || $vhName === '' || !is_string($vhRoot) || $vhRoot === '') {
            return null;
        }

        return ConfigLoadTarget::specialData(PathTool::GetAbsFile($file, 'VR', $vhName, $vhRoot), $realmDataId);
    }
}
