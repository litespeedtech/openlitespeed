<?php

namespace LSWebAdmin\Config\IO;

use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Util\PathTool;

class ConfigPathResolver
{
    public static function resolve($type, $name = '', $serverData = null, $allowTemplatePaths = true)
    {
        if ($type == DInfo::CT_SERV) {
            return ConfigPathResolutionResult::resolved($type, SERVER_ROOT . 'conf/httpd_config.conf', $name);
        }

        if ($type == DInfo::CT_ADMIN) {
            $adminRoot = PathTool::GetAbsFile('$SERVER_ROOT/admin/', 'SR');
            if ($name === '') {
                return ConfigPathResolutionResult::resolved($type, $adminRoot . 'conf/admin_config.conf', $name);
            }

            return ConfigPathResolutionResult::unresolved($type, null, $name);
        }

        if ($type == DInfo::CT_VH) {
            return self::resolveNamedConfigPath($type, 'virtualhost', 'configFile', $name, $serverData, 'VR');
        }

        if ($type == DInfo::CT_TP) {
            if (!$allowTemplatePaths) {
                return ConfigPathResolutionResult::unresolved($type, 'template config is not supported in this product variant', $name);
            }

            return self::resolveNamedConfigPath($type, 'vhTemplate', 'templateFile', $name, $serverData, 'SR');
        }

        return ConfigPathResolutionResult::unresolved($type, null, $name);
    }

    public static function resolveNamedConfigPath($type, $nodeKey, $pathKey, $name, $serverData, $rootType)
    {
        $shortType = ($type == DInfo::CT_VH) ? 'vh' : 'tp';
        if (!is_string($name) || $name === '') {
            return ConfigPathResolutionResult::unresolved($type, "cannot find config file for $shortType", $name);
        }

        if ($serverData == null) {
            return ConfigPathResolutionResult::unresolved($type, "cannot find config file for $shortType $name", $name);
        }

        $node = $serverData->GetChildNodeById($nodeKey, $name);
        if ($node == null) {
            return ConfigPathResolutionResult::unresolved($type, "cannot find config file for $shortType $name", $name);
        }

        $path = $node->GetChildVal($pathKey);
        if (!is_string($path) || $path === '') {
            return ConfigPathResolutionResult::unresolved($type, "cannot find config file for $shortType $name", $name);
        }

        if ($rootType == 'VR') {
            $vhRootPath = $node->GetChildVal('vhRoot');
            $path = PathTool::GetAbsFile($path, $rootType, $name, $vhRootPath);
        } else {
            $path = PathTool::GetAbsFile($path, $rootType);
        }

        return ConfigPathResolutionResult::resolved($type, $path, $name);
    }
}
