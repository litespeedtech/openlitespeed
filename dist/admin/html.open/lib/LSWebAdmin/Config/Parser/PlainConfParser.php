<?php

namespace LSWebAdmin\Config\Parser;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\Product\Current\DTblDef;

class PlainConfParser
{
    private $_hasInclude;
    private $_bypassIncludes = [];

    public function SetBypassInclude($bypassPatterns)
    {
        $this->_bypassIncludes = $bypassPatterns;
    }

    public function __construct()
    {
    }

    public function Parse($filename)
    {
        $this->_hasInclude = false;
        $root = new CNode(CNode::K_ROOT, $filename, CNode::T_ROOT);
        $rawfiles = new RawFiles();

        $this->parse_raw($rawfiles, $root);

        return $root;
    }

    public function HasInclude()
    {
        return $this->_hasInclude;
    }

    private function parse_raw($rawfiles, $root)
    {
        $fid = $rawfiles->AddRawFile($root);

        $filename = $root->Get(CNode::FLD_VAL);
        if (!empty($this->_bypassIncludes)) {
            foreach ($this->_bypassIncludes as $pattern) {
                if (preg_match($pattern, $filename)) {
                    return;
                }
            }
        }
        $fullpath = $rawfiles->GetFullFileName($fid);

        $rawlines = file($fullpath);

        if ($rawlines === false) {
            $errlevel = ($root->Get(CNode::FLD_KEY) == CNode::K_ROOT) ? CNode::E_FATAL : CNode::E_WARN;
            $errmsg = "Failed to read file $filename, abspath = $fullpath";
            $rawfiles->MarkError($root, $errlevel, $errmsg);
            return;
        }

        $root->SetRawMap($fid, 1, count($rawlines), '');

        $stack = [];
        $cur_node = $root;
        $prev_node = null;
        $cur_val = $cur_comment = '';
        $from_line = $to_line = 0;

        $sticky = false;
        $multiline_tag = '';

        foreach ($rawlines as $line_num => $data) {

            $line_num++;

            if ($sticky || ($multiline_tag != '')) {
                $d = rtrim($data, "\r\n");
            } else {
                $d = trim($data);
                if ($d == '') {
                    $cur_comment .= "\n";
                    continue;
                }

                if ($d[0] == '#') {
                    $cur_comment .= $d . "\n";
                    continue;
                }
                $from_line = $line_num;
            }

            if (strlen($d) > 0) {
                $end_char = $d[strlen($d) - 1];
            } else {
                $end_char = '';
            }

            $cur_val .= $d;

            if ($end_char == '\\') {
                $sticky = true;
                $cur_val .= "\n";
                continue;
            } else {
                $sticky = false;
            }

            if ($multiline_tag != '') {
                if (trim($d) == $multiline_tag) {
                    $multiline_tag = '';
                } else {
                    $cur_val .= "\n";
                    continue;
                }
            } elseif (($pos = strpos($d, '<<<')) > 0) {
                $multiline_tag = trim(substr($d, $pos + 3));
                $cur_val .= "\n";
                continue;
            }

            $to_line = $line_num;

            if ($d[0] == '}') {
                $cur_node->EndBlock($cur_comment);

                if (strlen($cur_val) > 1) {
                    $rawfiles->MarkError($cur_node, CNode::E_WARN, 'No other characters allowed at the end of closing }');
                }

                if (count($stack) > 0) {
                    $prev_node = $cur_node;
                    $prev_node->Set(CNode::FLD_FLTO, $line_num);
                    $cur_node = array_pop($stack);
                } else {
                    $rawfiles->MarkError(($prev_node == null) ? $cur_node : $prev_node, CNode::E_FATAL, 'Mismatched blocks, may due to extra closing }');
                }
            } else {

                $is_block = false;
                if ($end_char == '{') {
                    $cur_val = rtrim(substr($cur_val, 0, (strlen($cur_val) - 1)));
                    $is_block = true;
                }

                if (preg_match('/^([\S]+)\s/', $cur_val, $m)) {
                    $key = $m[1];
                    $val = trim(substr($cur_val, strlen($m[0])));
                    if (substr($val, 0, 3) == '<<<') {
                        $posv0 = strpos($val, "\n");
                        $posv1 = strrpos($val, "\n");
                        $val = trim(substr($val, $posv0 + 1, $posv1 - $posv0));
                    }
                } else {
                    $key = $cur_val;
                    $val = null;
                }

                if ($cur_node->HasFlag(CNode::BM_HAS_RAW)) {
                    if (DTblDef::GetInstance()->IsSpecialBlockRawContent($cur_node, $key)) {
                        $cur_node->AddRawContent($d, $cur_comment);
                        $cur_val = '';
                        continue;
                    }
                }

                $type = CNode::T_KV;
                if ($is_block) {
                    $type = ($val == null) ? CNode::T_KB : CNode::T_KVB;
                } elseif (strcasecmp($key, 'include') == 0) {
                    $type = CNode::T_INC;
                }

                $newnode = new CNode($key, $val, $type);
                $newnode->SetRawMap($fid, $from_line, $to_line, $cur_comment);
                if (!preg_match('/^([a-zA-Z_0-9:])+$/', $key)) {
                    $rawfiles->MarkError($newnode, CNode::E_WARN, "Invalid char in keyword $key");
                }

                $cur_node->AddChild($newnode);

                if ($newnode->HasFlag(CNode::BM_BLK)) {
                    DTblDef::GetInstance()->MarkSpecialBlock($newnode);
                    $stack[] = $cur_node;
                    $prev_node = $cur_node;
                    $cur_node = $newnode;
                } elseif ($newnode->HasFlag(CNode::BM_INC)) {
                    $this->parse_raw($rawfiles, $newnode);
                    $cur_node->AddIncludeChildren($newnode);
                    $this->_hasInclude = true;
                }
            }

            $cur_val = '';
            $cur_comment = '';
        }

        $cur_node->EndBlock($cur_comment);

        while (count($stack) > 0) {
            $rawfiles->MarkError($cur_node, CNode::E_FATAL, 'Mismatched blocks at end of the file, may due to extra openning { or missing closing }.');

            $prev_node = $cur_node;
            $cur_node = array_pop($stack);
        }
    }
}
