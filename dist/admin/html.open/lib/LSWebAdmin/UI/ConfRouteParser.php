<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\Product\Current\DPageDef;

class ConfRouteParser
{
    public static function parse($inputSource = null, $pageDefClass = null, $sessionToken = null)
    {
        if ($inputSource == null) {
            $inputSource = UIBase::GetInputSource();
        }
        if ($pageDefClass == null) {
            $pageDefClass = DPageDef::class;
        }

        $submitAction = self::parseSubmitActionPayload($inputSource->GrabGoodInput('POST', 'lst_action'));
        $hasPid = false;
        $mid = $inputSource->GrabGoodInput('REQUEST', 'm');
        if ($mid == null) {
            $mid = 'serv';
        }

        $pid = null;
        $view = null;
        $viewName = null;
        $tid = null;
        $ref = null;

        $pidInput = $inputSource->GrabGoodInput('REQUEST', 'p');
        if ($pidInput != null) {
            $pid = $pidInput;
            $hasPid = true;
        }

        if (($pos = strpos($mid, '_')) > 0) {
            $view = substr($mid, 0, $pos + 1);
            $viewName = substr($mid, $pos + 1);
            if ($pid == '' || $view == 'sl' || $view == 'sl_' || $view == 'al' || $view == 'al_' || $view == 'lb' || $view == 'lb_' || $view == 'lb4_' || $pid == 'base' || $pid == 'mbr') {
                $ref = $viewName;
            }
        } else {
            $view = $mid;
        }

        $confType = ($mid[0] == 'a') ? DInfo::CT_ADMIN : DInfo::CT_SERV;
        $tabs = call_user_func([$pageDefClass, 'GetInstance'])->GetTabDef($view);

        if ($hasPid) {
            if (!array_key_exists($pid, $tabs)) {
                throw new \RuntimeException("Invalid pid - {$pid} \n");
            }
        } else {
            $pid = key($tabs);
        }

        if ($submitAction != null) {
            $tid = $submitAction['t'];
            $ref = $submitAction['r'];
        } elseif ($hasPid && !$inputSource->HasInput('REQUEST', 't0') && $inputSource->HasInput('REQUEST', 't')) {
            $t = $inputSource->GrabGoodInput('REQUEST', 't');
            if ($t != null) {
                $tid = $t;
                $t1 = $inputSource->GrabGoodInputWithReset('REQUEST', 't1');
                if ($t1 != null && self::getLastPart($tid) != $t1) {
                    $tid .= '`' . $t1;
                }

                $r = $inputSource->GrabGoodInputWithReset('REQUEST', 'r');
                if ($r != null) {
                    $ref = $r;
                }
                $r1 = $inputSource->GrabGoodInputWithReset('REQUEST', 'r1');
                if ($r1 != null) {
                    $ref .= '`' . $r1;
                }
            }
        }

        $act = ($submitAction != null) ? $submitAction['a'] : $inputSource->GrabGoodInput('REQUEST', 'a');
        if ($act == null) {
            $act = 'v';
        }

        $token = $sessionToken;
        $tokenInput = $inputSource->GrabGoodInput('REQUEST', 'tk');
        if (self::requiresMutationToken($act) && $token != $tokenInput) {
            throw new \RuntimeException('Illegal entry point!');
        }

        if ($act == 'B') {
            $tid = self::trimLastIdPart($tid);
            $ref = self::trimLastRefPart($ref, $view, $pid, $viewName);
            $act = 'v';
        }

        return new ConfRouteContext($confType, $view, $viewName, $mid, $pid, $tid, $ref, $act, $token, $tabs);
    }

    private static function getLastPart($value)
    {
        if ($value != null && ($pos = strrpos($value, '`')) !== false) {
            if (strlen($value) > $pos + 1) {
                return substr($value, $pos + 1);
            }
            return '';
        }

        return $value;
    }

    private static function trimLastIdPart($tid)
    {
        if ($tid && ($pos = strrpos($tid, '`')) !== false) {
            return substr($tid, 0, $pos);
        }

        return null;
    }

    private static function trimLastRefPart($ref, $view, $pid, $viewName)
    {
        if ($ref && ($pos = strrpos($ref, '`')) !== false) {
            return substr($ref, 0, $pos);
        }

        if ($view == 'sl_' || $view == 'al_' || $view == 'lb_' || $view == 'lb4_' || $pid == 'base' || $pid == 'mbr') {
            return $viewName;
        }

        return null;
    }

    private static function requiresMutationToken($act)
    {
        return in_array($act, ['s', 'S', 'd', 'D', 'I', 'i', 'n', 'c', 'o'], true);
    }

    private static function parseSubmitActionPayload($payload)
    {
        if ($payload == null || $payload === '') {
            return null;
        }

        $parts = explode('|', $payload, 3);
        if (count($parts) !== 3) {
            throw new \RuntimeException('Illegal submit action payload!');
        }

        $act = $parts[0];
        if (!in_array($act, ['d', 'D', 'i', 'I'], true)) {
            throw new \RuntimeException('Illegal submit action payload!');
        }

        return [
            'a' => $act,
            't' => self::decodeSubmitPayloadPart($parts[1]),
            'r' => self::decodeSubmitPayloadPart($parts[2]),
        ];
    }

    private static function decodeSubmitPayloadPart($value)
    {
        if ($value === '') {
            return null;
        }

        return rawurldecode($value);
    }
}
