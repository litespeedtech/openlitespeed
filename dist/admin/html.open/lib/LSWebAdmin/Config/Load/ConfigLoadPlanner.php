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
            return self::resolveRealmDataTarget($request, $serverData, $currentData, $realmDataId);
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

    private static function resolveRealmDataTarget($request, $serverData, $currentData, $realmDataId)
    {
        if ($currentData != null) {
            // Detached vhost config file: realms are flattened to top-level `realm`.
            $realm = $currentData->GetChildNodeById('realm', $request->GetFirstRef());
        } else {
            // Embedded DirectAdmin vhost: the node lives inside the server config
            // tree, so realms keep their nested `security:realmList:realm` shape.
            $realm = self::resolveEmbeddedRealmNode($request, $serverData);
        }
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

    private static function resolveEmbeddedRealmNode($request, $serverData)
    {
        $vhnode = self::resolveEmbeddedVirtualHostTarget($request, $serverData);
        if ($vhnode == null || !method_exists($vhnode, 'GetChildNode')) {
            return null;
        }

        $location = 'security:realmList:*realm';
        $ref = $request->GetFirstRef();
        if (!is_string($ref) || $ref === '') {
            return null;
        }

        return $vhnode->GetChildNode($location, $ref);
    }

    private static function resolveEmbeddedVirtualHostTarget($request, $serverData)
    {
        if ($serverData == null || !method_exists($serverData, 'GetChildNodeById')) {
            return null;
        }

        $vhName = $request->GetViewName();
        if (!is_string($vhName) || $vhName === '') {
            return null;
        }

        return $serverData->GetChildNodeById('virtualhost', $vhName);
    }
}
