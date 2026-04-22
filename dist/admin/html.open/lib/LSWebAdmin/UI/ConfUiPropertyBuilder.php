<?php

namespace LSWebAdmin\UI;

class ConfUiPropertyBuilder
{
    public static function apply($props, $routeState, $ctrlUrl, $servData = null)
    {
        $hiddenVars = [
            'm'  => $routeState->GetMid(),
            'p'  => $routeState->GetPid(),
            't'  => $routeState->GetTid(),
            'r'  => $routeState->GetRef(),
            'tk' => $routeState->GetToken()
        ];

        $props->Set(UIProperty::FLD_FORM_HIDDENVARS, $hiddenVars);

        if ($servData != null) {
            $props->Set(UIProperty::FLD_SERVER_NAME, $servData->GetId());
        }

        $tabs = [];
        foreach ($routeState->GetTabs() as $pid => $tabName) {
            $prefix = ($pid == $routeState->GetPid()) ? '1' : '0';
            $tabs[$prefix . $tabName] = self::buildTabUrl($routeState, $pid);
        }

        $props->Set(UIProperty::FLD_TABS, $tabs);
    }

    private static function buildTabUrl($routeState, $pid)
    {
        $params = [
            'view' => 'confMgr',
            'm' => $routeState->GetMid(),
            'p' => $pid
        ];

        return 'index.php?' . http_build_query($params);
    }
}
