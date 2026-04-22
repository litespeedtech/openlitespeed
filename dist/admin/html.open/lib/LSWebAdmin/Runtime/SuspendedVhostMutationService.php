<?php

namespace LSWebAdmin\Runtime;

use LSWebAdmin\Config\CNode;

class SuspendedVhostMutationService
{
    public static function apply($serverData, $actionType, $virtualHostName)
    {
        if ($serverData == null || $virtualHostName == null || $virtualHostName === '') {
            return false;
        }

        $hasChanged = false;
        $currentDisabled = [];
        $key = 'suspendedVhosts';

        $currentNode = $serverData->GetRootNode()->GetChildren($key);
        if ($currentNode != null && $currentNode->Get(CNode::FLD_VAL) != null) {
            $currentDisabled = preg_split("/[,;]+/", $currentNode->Get(CNode::FLD_VAL), -1, PREG_SPLIT_NO_EMPTY);
        }

        $found = in_array($virtualHostName, $currentDisabled);
        if ($actionType == SInfo::SREQ_VH_DISABLE) {
            if (!$found) {
                $currentDisabled[] = $virtualHostName;
                $hasChanged = true;
            }
        } elseif ($actionType == SInfo::SREQ_VH_ENABLE) {
            if ($found) {
                $arrayKey = array_search($virtualHostName, $currentDisabled);
                unset($currentDisabled[$arrayKey]);
                $hasChanged = true;
            }
        }

        if ($hasChanged) {
            $values = implode(',', $currentDisabled);
            if ($currentNode == null) {
                $serverData->GetRootNode()->AddChild(new CNode($key, $values));
            } else {
                $currentNode->SetVal($values);
            }
            $serverData->SaveFile();
        }

        return $hasChanged;
    }
}
