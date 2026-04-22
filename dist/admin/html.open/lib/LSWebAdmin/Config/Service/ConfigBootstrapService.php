<?php

namespace LSWebAdmin\Config\Service;

use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Config\Load\ConfigLoadPlanner;
use LSWebAdmin\Config\Load\ConfigLoadTarget;
use LSWebAdmin\Config\IO\ConfigPathResolutionResult;

class ConfigBootstrapService
{
    public static function execute($request)
    {
        $dataClass = $request->GetDataClass();
        $loadRequest = $request->GetLoadRequest();
        $routeState = $request->GetRouteState();
        $result = new ConfigBootstrapResult();

        $serverResolution = $request->GetServerResolution();
        self::recordResolutionError($result, $serverResolution);
        $serverData = self::loadResolvedData($dataClass, DInfo::CT_SERV, $serverResolution, null, $routeState);
        if ($serverData != null) {
            $result->SetServerData($serverData);
        }

        if ($request->ShouldLoadAdmin()) {
            $adminResolution = $request->GetAdminResolution();
            self::recordResolutionError($result, $adminResolution);
            $adminData = self::loadResolvedData($dataClass, DInfo::CT_ADMIN, $adminResolution, null, $routeState);
            if ($adminData != null) {
                $result->SetAdminData($adminData);
            }
        }

        $currentTarget = ConfigLoadPlanner::resolveCurrentTarget($loadRequest, $request->AllowCurrentData());
        if ($currentTarget instanceof ConfigLoadTarget) {
            $currentResolution = $request->ResolveCurrentPathResult(
                $currentTarget->GetType(),
                $currentTarget->GetName(),
                $serverData
            );
            self::recordResolutionError($result, $currentResolution);

            if ($currentResolution->HasPath()) {
                $result->SetCurrentData(self::loadData(
                    $dataClass,
                    $currentTarget->GetType(),
                    $currentResolution->GetPath(),
                    $currentTarget->GetId(),
                    $routeState
                ));
            }
        }

        $specialTarget = ConfigLoadPlanner::resolveSpecialTarget($loadRequest, $serverData, $result->GetCurrentData());
        if ($specialTarget instanceof ConfigLoadTarget) {
            $result->SetSpecialData(self::loadData(
                $dataClass,
                $specialTarget->GetType(),
                $specialTarget->GetPath(),
                $specialTarget->GetId(),
                $routeState
            ));
        }

        return $result;
    }

    private static function loadData($dataClass, $type, $path, $id = null, $routeState = null)
    {
        return new $dataClass($type, $path, $id, $routeState);
    }

    private static function loadResolvedData($dataClass, $type, $resolution, $id = null, $routeState = null)
    {
        if (!($resolution instanceof ConfigPathResolutionResult) || !$resolution->HasPath()) {
            return null;
        }

        return self::loadData($dataClass, $type, $resolution->GetPath(), $id, $routeState);
    }

    private static function recordResolutionError($result, $resolution)
    {
        if ($resolution instanceof ConfigPathResolutionResult && $resolution->HasError()) {
            $result->AddMessage($resolution->GetError());
        }
    }
}
