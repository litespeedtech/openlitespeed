<?php

namespace LSWebAdmin\Config\IO;

use LSWebAdmin\Config\Parser\LsXmlParser;
use LSWebAdmin\Config\Parser\PlainConfParser;
use LSWebAdmin\Config\CNode;

class ConfigDataLoader
{
    public static function loadPlainRoot(PlainConfParser $parser, $path, &$conferr)
    {
        $root = $parser->Parse($path);
        if ($root->HasFatalErr()) {
            $conferr = $root->Get(CNode::FLD_ERR);
            error_log('fatal err ' . $root->Get(CNode::FLD_ERR));
            return false;
        }

        if ($parser->HasInclude() && !defined('_CONF_READONLY_')) {
            define('_CONF_READONLY_', true);
        }

        return $root;
    }

    public static function loadXmlRoot(LsXmlParser $parser, $path, &$conferr)
    {
        $root = $parser->Parse($path);
        if ($root->HasFatalErr()) {
            $conferr = $root->Get(CNode::FLD_ERR);
            error_log('fatal err ' . $root->Get(CNode::FLD_ERR));
            return false;
        }

        return $root;
    }

    public static function loadSpecialRoot($path, $id)
    {
        return SpecialFileCodec::load($path, $id);
    }

    public static function saveSpecialRoot($path, $id, $root)
    {
        return SpecialFileCodec::save($path, $id, $root);
    }
}
