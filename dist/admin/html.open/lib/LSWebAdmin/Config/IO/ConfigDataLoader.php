<?php

namespace LSWebAdmin\Config\IO;

use LSWebAdmin\Config\Parser\LsXmlParser;
use LSWebAdmin\Config\Parser\PlainConfParser;
use LSWebAdmin\Config\CNode;

class ConfigDataLoader
{
    public static function loadPlainRoot(PlainConfParser $parser, $path, &$conferr, &$hasInclude = false)
    {
        $root = $parser->Parse($path);
        if ($root->HasFatalErr()) {
            $conferr = $root->Get(CNode::FLD_ERR);
            error_log('fatal err ' . $root->Get(CNode::FLD_ERR));
            return false;
        }

        $hasInclude = $parser->HasInclude();

        // Legacy process-global signal: kept for backward compatibility. Whether a
        // given config page is read-only is decided per config root (see
        // CData::IsReadOnly), because one request can load both an include-driven
        // server config and an include-free admin config.
        if ($hasInclude && !defined('_CONF_READONLY_')) {
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
