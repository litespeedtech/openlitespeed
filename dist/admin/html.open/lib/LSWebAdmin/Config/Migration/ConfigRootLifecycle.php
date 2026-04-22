<?php

namespace LSWebAdmin\Config\Migration;

use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Config\CNode;

class ConfigRootLifecycle
{
    public static function beforeWriteConf($type, $root)
    {
        if ($type == DInfo::CT_SERV) {
            ConfigTreeNormalizer::convertListenerVhMapsToMaps($root);
            self::stripNonInternalModuleFlags($root);
        }

        $loc = ($type == DInfo::CT_TP) ? 'virtualHostConfig:scripthandler' : 'scripthandler';
        ConfigTreeNormalizer::convertScriptHandlerAddSuffixToAdd($root, $loc);

        if ($type == DInfo::CT_VH || $type == DInfo::CT_TP) {
            $loc = ($type == DInfo::CT_VH) ? 'context' : 'virtualHostConfig:context';
            // Canonicalize legacy split charset fields into the single AddDefaultCharset directive.
            ConfigTreeNormalizer::normalizeContextCharsets($root, $loc);
            ConfigTreeNormalizer::stripDefaultContextType($root, $loc);
        }

        if ($type == DInfo::CT_TP) {
            self::rewriteRootChildSuffix($root, 'configFile', '.xml', '.conf');
        }
    }

    public static function beforeWriteXml($type, $root)
    {
        if ($type == DInfo::CT_SERV) {
            ConfigTreeNormalizer::convertListenerMapsToVhMaps($root);
            self::rewriteChildrenSuffix($root, 'virtualhost', 'configFile', '.conf', '.xml');
            self::rewriteChildrenSuffix($root, 'vhTemplate', 'templateFile', '.conf', '.xml');
        }

        $loc = ($type == DInfo::CT_TP) ? 'virtualHostConfig:scripthandler' : 'scripthandler';
        ConfigTreeNormalizer::convertScriptHandlerAddToAddSuffix($root, $loc);

        if ($type == DInfo::CT_VH || $type == DInfo::CT_TP) {
            $loc = ($type == DInfo::CT_VH) ? 'context' : 'virtualHostConfig:context';
            ConfigTreeNormalizer::normalizeContextCharsets($root, $loc);
            ConfigTreeNormalizer::stripDefaultContextType($root, $loc);
        }

        if ($type == DInfo::CT_TP) {
            self::rewriteRootChildSuffix($root, 'configFile', '.conf', '.xml');
        }
    }

    public static function afterRead($type, $root, $currentId)
    {
        if ($type == DInfo::CT_SERV) {
            $currentId = self::applyServerReadDefaults($root, $currentId);
        }

        if ($type == DInfo::CT_SERV || $type == DInfo::CT_ADMIN) {
            ConfigTreeNormalizer::addListenerAddressFields($root);
            ConfigTreeNormalizer::convertListenerMapsToVhMaps($root);
        }

        if ($type == DInfo::CT_VH) {
            self::afterReadContextOwner($root, 'context', 'scripthandler');
        } elseif ($type == DInfo::CT_TP) {
            self::afterReadContextOwner($root, 'virtualHostConfig:context', 'virtualHostConfig:scripthandler');
        } else {
            $loc = ($type == DInfo::CT_TP) ? 'virtualHostConfig:scripthandler' : 'scripthandler';
            ConfigTreeNormalizer::convertScriptHandlerAddToAddSuffix($root, $loc);
        }

        return $currentId;
    }

    public static function afterReadVirtualHostNode($root)
    {
        self::afterReadContextOwner($root, 'context', 'scripthandler');
    }

    private static function applyServerReadDefaults($root, $currentId)
    {
        $serverName = $root->GetChildVal('serverName');
        if ($serverName == '$HOSTNAME' || $serverName == '') {
            $serverName = php_uname('n');
        }

        $runningAs = 'user(' . $root->GetChildVal('user')
            . ') : group(' . $root->GetChildVal('group') . ')';
        $root->AddChild(new CNode('runningAs', $runningAs));

        $mods = $root->GetChildren('module');
        if ($mods != null) {
            if (!is_array($mods)) {
                $mods = [$mods];
            }
            foreach ($mods as $mod) {
                if ($mod->GetChildVal('internal') === null) {
                    if ($mod->Get(CNode::FLD_VAL) == 'cache') {
                        $mod->AddChild(new CNode('internal', '1'));
                    } else {
                        $mod->AddChild(new CNode('internal', '0'));
                    }
                }
            }
        }

        return $serverName;
    }

    private static function afterReadContextOwner($root, $contextLoc, $scriptHandlerLoc)
    {
        ConfigTreeNormalizer::normalizeContextCharsets($root, $contextLoc);
        ConfigTreeNormalizer::addDefaultContextTypeAndOrder($root, $contextLoc);
        ConfigTreeNormalizer::convertScriptHandlerAddToAddSuffix($root, $scriptHandlerLoc);
    }

    private static function stripNonInternalModuleFlags($root)
    {
        $mods = $root->GetChildren('module');
        if ($mods == null) {
            return;
        }

        if (!is_array($mods)) {
            $mods = [$mods];
        }

        foreach ($mods as $mod) {
            if ($mod->GetChildVal('internal') != 1) {
                $mod->RemoveChild('internal');
            }
        }
    }

    private static function rewriteChildrenSuffix($root, $childKey, $fieldKey, $fromSuffix, $toSuffix)
    {
        $children = $root->GetChildren($childKey);
        if ($children == null) {
            return;
        }

        if (!is_array($children)) {
            $children = [$children];
        }

        foreach ($children as $child) {
            $value = $child->GetChildVal($fieldKey);
            if (self::endsWith($value, $fromSuffix)) {
                $child->SetChildVal($fieldKey, substr($value, 0, -strlen($fromSuffix)) . $toSuffix);
            }
        }
    }

    private static function rewriteRootChildSuffix($root, $fieldKey, $fromSuffix, $toSuffix)
    {
        $value = $root->GetChildVal($fieldKey);
        if (self::endsWith($value, $fromSuffix)) {
            $root->SetChildVal($fieldKey, substr($value, 0, -strlen($fromSuffix)) . $toSuffix);
        }
    }

    private static function endsWith($value, $suffix)
    {
        $suffixLength = strlen($suffix);
        if ($suffixLength === 0) {
            return true;
        }

        if (!is_string($value) || strlen($value) < $suffixLength) {
            return false;
        }

        return substr_compare($value, $suffix, -$suffixLength, $suffixLength) === 0;
    }
}
