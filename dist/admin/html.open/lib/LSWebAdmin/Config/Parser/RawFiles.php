<?php

namespace LSWebAdmin\Config\Parser;

use LSWebAdmin\Config\CNode;

class RawFiles
{
    private $_list = [];
    private $_errs = [];
    private $_fatal = 0;

    public function GetFullFileName($fid)
    {
        return $this->_list[$fid][4];
    }

    public function AddRawFile($node)
    {
        $filename = $node->Get(CNode::FLD_VAL);
        $index = count($this->_list);
        $parentid = $index - 1;
        $level = ($index > 0) ? $this->_list[$parentid][1] + 1 : 0;
        $fullpath = $filename;
        if (substr($filename, 0, 1) != '/') {
            if ($index > 0) {
                $fullpath = $this->_list[$parentid][3] . '/' . $filename;
            } else {
                $fullpath = SERVER_ROOT . '/conf/' . $fullpath;
            }
        }
        $dir = dirname($fullpath);

        $this->_list[$index] = [$filename, $level, $index, $dir, $fullpath];

        return $index;
    }

    public function MarkError($node, $errlevel, $errmsg)
    {
        $node->SetErr($errmsg, $errlevel);
        $this->_errs[] = [
            $errlevel,
            $errmsg,
            $node->Get(CNode::FLD_FID),
            $node->Get(CNode::FLD_FLFROM),
            $node->Get(CNode::FLD_FLTO)
        ];
        if ($errlevel == CNode::E_FATAL) {
            $this->_fatal++;
        }
    }

    public function HasFatalErr()
    {
        return ($this->_fatal > 0);
    }

    public function HasErr()
    {
        return (count($this->_errs) > 0);
    }

    public function GetAllErrors()
    {
        $level = [CNode::E_WARN => 'WARN', CNode::E_FATAL => 'FATAL'];

        $buf = "\nShow Errors: \n";
        foreach ($this->_errs as $e) {
            $errlevel = $level[$e[0]];
            $filename = $this->_list[$e[2]][0];
            $buf .= "$errlevel $filename line $e[3]";
            if ($e[3] != $e[4]) {
                $buf .= " ~ $e[4]";
            }
            $buf .= ": $e[1]\n";
        }
        return $buf;
    }

}
