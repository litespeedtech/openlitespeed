<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Util\PathTool;

class ConfDerivedOptionsResolver
{
    public static function resolve($routeState, $servData, $confData, $tid, $loc, $node)
    {
        if (substr($loc, 0, 13) == 'extprocessor:') {
            return self::resolveExtprocessor($routeState, $servData, $confData, substr($loc, 13), $node);
        }

        $names = [];
        $resolvedLoc = $loc;
        if ($loc == 'cluster') {
            $resolvedLoc = 'loadbalancer';
        }
        if (in_array($resolvedLoc, ['virtualhost', 'listener', 'module', 'loadbalancer'])) {
            if ($servData != null) {
                $names = $servData->GetChildrenValues($resolvedLoc);
            }
        } elseif ($loc == 'realm') {
            if ($routeState->GetView() == DInfo::CT_TP) {
                $loc = 'virtualHostConfig:' . $loc;
            }
            if ($confData != null) {
                $names = $confData->GetChildrenValues($loc);
            }
        }

        if (!is_array($names)) {
            $names = [];
        }

        sort($names);
        $options = [];
        foreach ($names as $name) {
            $options[$name] = $name;
        }

        return $options;
    }

    public static function resolveVHRoot($routeState, $servData)
    {
        if ($routeState->GetView() != DInfo::CT_VH || $servData == null) {
            return null;
        }

        $viewName = $routeState->GetViewName();
        $vh = $servData->GetChildNodeById('virtualhost', $viewName);
        if ($vh == null) {
            return null;
        }

        return PathTool::GetAbsFile($vh->GetChildVal('vhRoot'), 'SR', $viewName);
    }

    private static function resolveExtprocessor($routeState, $servData, $confData, $type, $node)
    {
        $options = [];

        if ($type == '$$type') {
            if ($node != null) {
                $type = $node->GetChildVal('type');
            }
            if ($type == null) {
                $type = 'fcgi';
            }
        }

        if ($type == 'cgi') {
            $options['cgi'] = 'CGI Daemon';
            return $options;
        }

        if ($type == 'module') {
            if ($servData != null) {
                $modules = $servData->GetChildrenValues('module');
                if (is_array($modules)) {
                    foreach ($modules as $moduleName) {
                        $options[$moduleName] = $moduleName;
                    }
                }
            }

            return $options;
        }

        $serverLevel = self::collectExternalApps($servData, 'extprocessor', $type);
        if ($routeState->GetView() == DInfo::CT_SERV) {
            foreach ($serverLevel as $name) {
                $options[$name] = $name;
            }
            return $options;
        }

        foreach ($serverLevel as $name) {
            $options[$name] = '[' . DMsg::UIStr('note_serv_level') . "]: $name";
        }

        $vhLocation = ($routeState->GetView() == DInfo::CT_TP) ? 'virtualHostConfig:extprocessor' : 'extprocessor';
        $vhLevel = self::collectExternalApps($confData, $vhLocation, $type);
        foreach ($vhLevel as $name) {
            $options[$name] = '[' . DMsg::UIStr('note_vh_level') . "]: $name";
        }

        return $options;
    }

    private static function collectExternalApps($data, $location, $type)
    {
        $names = [];
        if ($data == null || $data->GetRootNode() == null) {
            return $names;
        }

        $externalApps = $data->GetRootNode()->GetChildren($location);
        if ($externalApps == null) {
            return $names;
        }

        if (is_array($externalApps)) {
            foreach ($externalApps as $name => $externalApp) {
                if ($externalApp->GetChildVal('type') == $type) {
                    $names[] = $name;
                }
            }
            return $names;
        }

        if ($externalApps->GetChildVal('type') == $type) {
            $names[] = $externalApps->Get(CNode::FLD_VAL);
        }

        return $names;
    }
}
