<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\I18n\DMsg;

class ConfActionDataBuilder
{
    public static function build($actions, $routeState, $ctrlUrl, $isPrintingLinkedTbl, $editTid = '', $editRef = '', $addTid = '')
    {
        $actionData = [];
        $definitions = self::getDefinitions();
        $chars = preg_split('//', $actions, -1, PREG_SPLIT_NO_EMPTY);
        $currentTid = $routeState->GetLastTid();

        foreach ($chars as $act) {
            if (!isset($definitions[$act])) {
                continue;
            }

            $name = $definitions[$act][0];
            $icon = $definitions[$act][1];
            if ($act == 'C') {
                $act = 'B';
            }

            if ($act == 'B' && $isPrintingLinkedTbl) {
                continue;
            }

            $submitName = null;
            $submitValue = null;

            if (self::isSubmitAction($act)) {
                list($submitName, $submitValue) = self::buildSubmitTransport($routeState, $currentTid, $act, $editTid, $editRef, $addTid);
                $href = 'index.php?view=confMgr';
            } elseif ($act == 'X') {
                $href = self::getControlBaseUrl($ctrlUrl) . 'm=' . urlencode($routeState->GetView() . '_' . $editRef);
                $act = 'v';
            } else {
                list($t, $r) = self::resolveActionRoute($routeState, $currentTid, $act, $editTid, $editRef, $addTid);
                $href = self::buildDirectRouteUrl($routeState, $t, $r, $act);
            }

            $actionData[$act] = [
                'label' => $name,
                'href' => $href,
                'ico' => $icon,
                'submit' => self::isSubmitAction($act),
                'submit_name' => $submitName,
                'submit_value' => $submitValue,
            ];
        }

        return $actionData;
    }

    private static function getDefinitions()
    {
        return [
            'a' => [DMsg::UIStr('btn_add'), 'plus'],
            'v' => [DMsg::UIStr('btn_view'), 'zoom-in'],
            'E' => [DMsg::UIStr('btn_edit'), 'edit'],
            's' => [DMsg::UIStr('btn_save'), 'save'],
            'B' => [DMsg::UIStr('btn_back'), 'reply'],
            'n' => [DMsg::UIStr('btn_next'), 'step-forward'],
            'd' => [DMsg::UIStr('btn_delete'), 'trash-2'],
            'D' => [DMsg::UIStr('btn_delete'), 'trash-2'],
            'C' => [DMsg::UIStr('btn_cancel'), 'chevrons-left'],
            'i' => [DMsg::UIStr('btn_instantiate'), 'copy'],
            'I' => [DMsg::UIStr('btn_instantiate'), 'copy'],
            'X' => [DMsg::UIStr('btn_view'), 'zoom-in'],
        ];
    }

    private static function isSubmitAction($act)
    {
        return in_array($act, ['s', 'n', 'd', 'D', 'i', 'I'], true);
    }

    private static function buildSubmitTransport($routeState, $currentTid, $act, $editTid, $editRef, $addTid)
    {
        if (in_array($act, ['d', 'D', 'i', 'I'], true)) {
            list($t, $r) = self::resolveActionRoute($routeState, $currentTid, $act, $editTid, $editRef, $addTid);
            return ['lst_action', self::encodeSubmitPayload($act, stripslashes((string) $t), stripslashes((string) $r))];
        }

        return ['a', $act];
    }

    private static function encodeSubmitPayload($act, $t, $r)
    {
        return $act . '|' . rawurlencode($t) . '|' . rawurlencode($r);
    }

    private static function resolveActionRoute($routeState, $currentTid, $act, $editTid, $editRef, $addTid)
    {
        if ($act == 'a') {
            $resolvedEditTid = $addTid;
            $resolvedEditRef = '~';
        } else {
            $resolvedEditTid = $editTid;
            $resolvedEditRef = $editRef;
        }

        if ($resolvedEditTid == '' || $resolvedEditTid == $currentTid) {
            $t = $routeState->GetTid();
        } elseif ($currentTid != null && $currentTid != $resolvedEditTid) {
            $t = $routeState->GetTid() . '`' . $resolvedEditTid;
        } else {
            $t = $resolvedEditTid;
        }

        if ($resolvedEditRef == '') {
            $r = $routeState->GetRef();
        } elseif ($routeState->GetRef() != null && $routeState->GetRef() != $resolvedEditRef) {
            $r = $routeState->GetRef() . '`' . $resolvedEditRef;
        } else {
            $r = $resolvedEditRef;
        }

        if ($t) {
            $t = addslashes($t);
        }
        if ($r) {
            $r = addslashes($r);
        }

        return [$t, $r];
    }

    private static function buildDirectRouteUrl($routeState, $t, $r, $act)
    {
        $params = [
            'view' => 'confMgr',
            'm' => $routeState->GetMid(),
            'p' => $routeState->GetPid()
        ];

        if ($act !== 'v') {
            $params['a'] = $act;
        }

        if ($t) {
            $params['t'] = stripslashes($t);
        }

        if ($r) {
            $params['r'] = stripslashes($r);
        }

        return 'index.php?' . http_build_query($params);
    }

    private static function getControlBaseUrl($ctrlUrl)
    {
        if (strpos($ctrlUrl, 'index.php?view=confMgr') === 0) {
            return 'index.php?view=confMgr&';
        }

        $pos = strpos($ctrlUrl, '?');
        if ($pos === false) {
            return $ctrlUrl;
        }

        return substr($ctrlUrl, 0, $pos + 1);
    }
}
