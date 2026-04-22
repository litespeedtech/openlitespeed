<?php

namespace LSWebAdmin\Config\IO;

use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Config\CNode;
use LSWebAdmin\Config\Service\ConfigMutationTarget;

class ConfigDataEditor
{
    public static function savePost($type, $root, $extractData, $disp, $pageDefClass)
    {
        $target = self::resolveTarget($type, $disp, $pageDefClass);
        self::saveToTarget($root, $extractData, $target);

        return [
            'tid' => $target->GetTid(),
            'newref' => $extractData->Get(CNode::FLD_VAL),
        ];
    }

    public static function deleteEntry($type, $root, $disp, $pageDefClass)
    {
        $target = self::resolveTarget($type, $disp, $pageDefClass);
        if (self::deleteTarget($root, $target)) {
            return $target->GetTid();
        }

        error_log('cannot find delete entry');
        return false;
    }

    public static function resolveTarget($type, $disp, $pageDefClass)
    {
        $tid = $disp->GetLastTid();
        $ref = self::resolveRef($type, $disp);
        $location = self::resolveLocation($disp, $tid, $pageDefClass);

        return new ConfigMutationTarget($tid, $location, $ref);
    }

    public static function saveToTarget($root, $extractData, $target)
    {
        $root->UpdateChildren($target->GetLocation(), $target->GetRef(), $extractData);
    }

    public static function deleteTarget($root, $target)
    {
        $location = $target->GetLocation();
        $ref = $target->GetRef();
        $layer = $root->GetChildrenByLoc($location, $ref);
        if ($layer) {
            $layer->RemoveFromParent();
            return true;
        }

        return false;
    }

    private static function resolveRef($type, $disp)
    {
        if ($type == DInfo::CT_EX) {
            return $disp->GetLastRef();
        }

        return $disp->GetRef();
    }

    private static function resolveLocation($disp, $tid, $pageDefClass)
    {
        $page = call_user_func([$pageDefClass, 'GetPage'], $disp);
        $tblmap = $page->GetTblMap();
        return $tblmap->FindTblLoc($tid);
    }
}
