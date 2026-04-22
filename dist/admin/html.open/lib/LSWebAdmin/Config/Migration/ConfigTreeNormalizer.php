<?php

namespace LSWebAdmin\Config\Migration;

use LSWebAdmin\Config\CNode;

class ConfigTreeNormalizer
{
    public static function normalizeContextCharsets($root, $location)
    {
        $contexts = self::normalizeChildren($root->GetChildren($location));
        foreach ($contexts as $context) {
            self::normalizeCharsetSetting($context);
        }
    }

    public static function convertListenerVhMapsToMaps($root)
    {
        $listeners = self::normalizeChildren($root->GetChildren('listener'));
        foreach ($listeners as $listener) {
            $maps = self::normalizeChildren($listener->GetChildren('vhmap'));
            foreach ($maps as $map) {
                $vn = $map->Get(CNode::FLD_VAL);
                $domain = $map->GetChildVal('domain');
                $listener->AddChild(new CNode('map', "$vn $domain"));
            }
            if (!empty($maps)) {
                $listener->RemoveChild('vhmap');
            }
        }
    }

    public static function addListenerAddressFields($root)
    {
        $listeners = self::normalizeChildren($root->GetChildren('listener'));
        foreach ($listeners as $listener) {
            $addr = $listener->GetChildVal('address');
            if ($addr && ($pos = strrpos($addr, ':'))) {
                $ip = substr($addr, 0, $pos);
                if ($ip == '*') {
                    $ip = 'ANY';
                }
                $listener->AddChild(new CNode('ip', $ip));
                $listener->AddChild(new CNode('port', substr($addr, $pos + 1)));
            }
        }
    }

    public static function convertListenerMapsToVhMaps($root)
    {
        $listeners = self::normalizeChildren($root->GetChildren('listener'));
        foreach ($listeners as $listener) {
            $maps = self::normalizeChildren($listener->GetChildren('map'));
            foreach ($maps as $map) {
                $mapval = $map->Get(CNode::FLD_VAL);
                if (($pos = strpos($mapval, ' ')) > 0) {
                    $vn = substr($mapval, 0, $pos);
                    $domain = trim(substr($mapval, $pos + 1));
                    $anode = new CNode('vhmap', $vn);
                    $anode->AddChild(new CNode('vhost', $vn));
                    $anode->AddChild(new CNode('domain', $domain));
                    $listener->AddChild($anode);
                }
            }
            if (!empty($maps)) {
                $listener->RemoveChild('map');
            }
        }
    }

    public static function convertScriptHandlerAddSuffixToAdd($root, $location)
    {
        $sh = $root->GetChildren($location);
        if (!$sh) {
            return;
        }

        $items = self::normalizeChildren($sh->GetChildren('addsuffix'));
        foreach ($items as $item) {
            $suffix = $item->Get(CNode::FLD_VAL);
            $type = $item->GetChildVal('type');
            $handler = $item->GetChildVal('handler');
            $sh->AddChild(new CNode('add', "$type:$handler $suffix"));
        }
        if (!empty($items)) {
            $sh->RemoveChild('addsuffix');
        }
    }

    public static function convertScriptHandlerAddToAddSuffix($root, $location)
    {
        $sh = $root->GetChildren($location);
        if (!$sh) {
            return;
        }

        $items = self::normalizeChildren($sh->GetChildren('add'));
        foreach ($items as $item) {
            $typeval = $item->Get(CNode::FLD_VAL);
            $m = [];
            if (preg_match("/^(\\w+):(\\S+)\\s+(.+)$/", $typeval, $m)) {
                $anode = new CNode('addsuffix', $m[3]);
                $anode->AddChild(new CNode('suffix', $m[3]));
                $anode->AddChild(new CNode('type', $m[1]));
                $anode->AddChild(new CNode('handler', $m[2]));
                $sh->AddChild($anode);
            }
        }
        if (!empty($items)) {
            $sh->RemoveChild('add');
        }
    }

    public static function stripDefaultContextType($root, $location)
    {
        $contexts = self::normalizeChildren($root->GetChildren($location));
        foreach ($contexts as $context) {
            if ($context->GetChildVal('type') === 'null') {
                $context->RemoveChild('type');
            }
        }
    }

    public static function addDefaultContextTypeAndOrder($root, $location)
    {
        $contexts = self::normalizeChildren($root->GetChildren($location));
        $order = 1;
        foreach ($contexts as $context) {
            if ($context->GetChildren('type') == null) {
                $context->AddChild(new CNode('type', 'null'));
            }
            $context->AddChild(new CNode('order', $order++));
        }
    }

    private static function normalizeChildren($nodes)
    {
        if ($nodes == null) {
            return [];
        }

        if (is_array($nodes)) {
            return $nodes;
        }

        return [$nodes];
    }

    private static function normalizeCharsetSetting($node)
    {
        $charset = self::normalizeCharsetValue($node->GetChildVal('addDefaultCharset'));
        $customized = self::normalizeCharsetValue($node->GetChildVal('defaultCharsetCustomized'));

        if ($customized !== '' && $customized !== null) {
            if ($charset === '' || $charset === null || $charset === 'on') {
                $charset = $customized;
            }
        }

        if ($charset !== null) {
            if (!$node->SetChildVal('addDefaultCharset', $charset)) {
                $node->AddChild(new CNode('addDefaultCharset', $charset));
            }
        } elseif ($customized !== '' && $customized !== null) {
            $node->AddChild(new CNode('addDefaultCharset', $customized));
        }

        if ($node->GetChildren('defaultCharsetCustomized') != null) {
            $node->RemoveChild('defaultCharsetCustomized');
        }
    }

    private static function normalizeCharsetValue($value)
    {
        if (!is_string($value)) {
            return $value;
        }

        $value = trim($value);
        if ($value === '') {
            return '';
        }

        $lower = strtolower($value);
        if ($lower === 'on' || $lower === 'off') {
            return $lower;
        }

        return $value;
    }
}
