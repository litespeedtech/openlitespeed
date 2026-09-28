<?php

namespace LSWebAdmin\Config\Migration;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\Product\Current\DTblDef;
use LSWebAdmin\UI\DTbl;

class ConfigXmlBotWhiteListAdapter
{
    public static function normalizeForConfigMap($root)
    {
        if (!self::activeProductUsesNestedListPath()) {
            return;
        }

        self::visit($root, function ($node) {
            if ($node->Get(CNode::FLD_KEY) !== 'lsrecaptcha') {
                return;
            }

            $botWhiteList = $node->GetChildren('botWhiteList');
            if (!($botWhiteList instanceof CNode)
                    || $botWhiteList->HasDirectChildren()) {
                return;
            }

            $value = $botWhiteList->Get(CNode::FLD_VAL);
            $botWhiteList->Set(CNode::FLD_TYPE, CNode::T_KB);
            $botWhiteList->SetVal(null);
            $botWhiteList->AddChild(new CNode('list', $value));
        });
    }

    private static function activeProductUsesNestedListPath()
    {
        $table = DTblDef::GetInstance()->GetTblDef('S_SEC_RECAP');
        $attrs = $table->Get(DTbl::FLD_DATTRS);
        foreach ($attrs as $attr) {
            if ($attr != null && $attr->GetKey() === 'botWhiteList:list') {
                return true;
            }
        }
        return false;
    }

    private static function visit($node, $callback)
    {
        if (!($node instanceof CNode)) {
            return;
        }

        $callback($node);
        foreach ($node->GetChildKeys() as $key) {
            $children = $node->GetChildren($key);
            if (is_array($children)) {
                foreach ($children as $child) {
                    self::visit($child, $callback);
                }
            } else {
                self::visit($children, $callback);
            }
        }
    }
}
