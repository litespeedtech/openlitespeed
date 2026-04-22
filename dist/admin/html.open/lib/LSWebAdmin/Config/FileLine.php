<?php

namespace LSWebAdmin\Config;

class FileLine
{
    public $_note0;  // begin notes
    public $_note1 = '';  // end notes
    public $_fid;
    public $_fline0;
    public $_fline1;

    public function __construct($file_id, $from_line, $to_line, $comment)
    {
        $this->_fid = $file_id;
        $this->_fline0 = $from_line;
        $this->_fline1 = $to_line;
        $this->_note0 = $comment;
    }

    public function AddEndComment($note1)
    {
        $this->_note1 .= "$note1\n";
    }

    public function debug_str()
    {
        return sprintf("fid=%s from line %s to %s", $this->_fid, $this->_fline0, $this->_fline1);
    }
}
