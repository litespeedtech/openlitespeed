<?php

namespace LSWebAdmin\Config\Service;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\Config\IO\ConfigDataEditor;
use LSWebAdmin\Config\Migration\ConfigIntegrityUpdater;

class ConfigMutationService
{
    public static function savePost($type, $root, $extractData, $disp, $pageDefClass)
    {
        $target = ConfigDataEditor::resolveTarget($type, $disp, $pageDefClass);
        return self::saveToTarget($root, $extractData, $target);
    }

    public static function deleteEntry($type, $root, $disp, $pageDefClass)
    {
        $target = ConfigDataEditor::resolveTarget($type, $disp, $pageDefClass);
        return self::deleteTarget($root, $target);
    }

    public static function saveToTarget($root, $extractData, $target)
    {
        ConfigDataEditor::saveToTarget($root, $extractData, $target);

        return new ConfigMutationResult($target, $extractData->Get(CNode::FLD_VAL));
    }

    public static function deleteTarget($root, $target)
    {
        if (!ConfigDataEditor::deleteTarget($root, $target)) {
            return false;
        }

        return new ConfigMutationResult($target, null, true);
    }

    public static function applyIntegrity($result, $disp)
    {
        if (!($result instanceof ConfigMutationResult)) {
            return;
        }

        if ($result->IsDelete()) {
            ConfigIntegrityUpdater::update($result->GetTid(), null, $disp);
            return;
        }

        if ($result->GetNewRef() != null) {
            ConfigIntegrityUpdater::update($result->GetTid(), $result->GetNewRef(), $disp);
        }
    }
}
