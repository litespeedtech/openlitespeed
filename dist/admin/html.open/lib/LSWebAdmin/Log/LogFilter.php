<?php

namespace LSWebAdmin\Log;

use LSWebAdmin\I18n\DMsg;

class LogFilter
{

    const LOGTYPE_ERRLOG = 1;
    const LOGTYPE_ACCESS = 2;
    const LOGTYPE_RESTART = 3;

    //'E' => 1, 'W' => 2, 'N' => 3, 'I' => 4, 'D' => 5

    private $LEVEL_DESCR = [1 => 'ERROR', 2 => 'WARNING', 3 => 'NOTICE', 4 => 'INFO', 5 => 'DEBUG'];

    const LEVEL_ERR = 1;
    const LEVEL_WARN = 2;
    const LEVEL_NOTICE = 3;
    const LEVEL_INFO = 4;
    const LEVEL_DEBUG = 5;
    const POS_FILEEND = -1;
    const FLD_LEVEL = 1;
    const FLD_LOGFILE = 2;
    const FLD_FROMINPUT = 3;
    const FLD_BLKSIZE = 4;
    const FLD_FROMPOS = 5;
    const FLD_TOTALSEARCHED = 6;
    const FLD_OUTMESG = 7;
    const FLD_TOTALFOUND = 8;
    const FLD_FILE_SIZE = 9;
    const FLD_SEARCH_LIMITED = 10;

    private $_logfile;
    private $_level;
    private $_frominput;
    private $_blksize;
    private $_frompos;
    private $_filesize;
    private $_output = '';
    private $_textOutput = '';
    private $_outlines = 0;
    private $_totallines = 0;
    private $_outmesg = '';
    private $_searchLimited = 0;

    public function __construct($filename)
    {
        $this->_logfile = $filename;
    }

    public function Get($field)
    {
        switch ($field) {
            case self::FLD_LEVEL: return $this->_level;
            case self::FLD_LOGFILE: return $this->_logfile;
            case self::FLD_FROMPOS: return number_format($this->_frompos / 1024, 2);
            case self::FLD_FROMINPUT: return $this->_frominput;
            case self::FLD_BLKSIZE: return $this->_blksize;
            case self::FLD_TOTALSEARCHED: return $this->_totallines;
            case self::FLD_OUTMESG:
                $repl = ['%%totallines%%' => number_format($this->_totallines),
                    '%%outlines%%'   => number_format($this->_outlines),
                    '%%level%%'      => $this->LEVEL_DESCR[$this->_level]
                ];
                return DMsg::UIStr('service_logresnote', $repl) . ' ' . $this->_outmesg;
            case self::FLD_TOTALFOUND: return $this->_outlines;
            case self::FLD_FILE_SIZE: return number_format($this->_filesize / 1024, 2);
            case self::FLD_SEARCH_LIMITED: return $this->_searchLimited;

            default: trigger_error("LogFilter: illegal field $field", E_USER_ERROR);
        }
    }

    public function GetLogOutput()
    {
        return $this->_output;
    }

    public function GetTextOutput()
    {
        return $this->_textOutput;
    }

    public function AddLogEntry($level, $time, $message)
    {
        $levelLabel = 'DEBUG';
        $this->_outlines ++;
        $buf = '<tr><td>' . $time . '</td><td class="lst-log-level-cell';
        switch ($level) {
            case self::LEVEL_ERR:
                $buf .= ' lst-log-level-cell--error">ERROR';
                $levelLabel = 'ERROR';
                break;
            case self::LEVEL_WARN:
                $buf .= ' lst-log-level-cell--warning">WARNING';
                $levelLabel = 'WARNING';
                break;
            case self::LEVEL_NOTICE:
                $buf .= ' lst-log-level-cell--notice">NOTICE';
                $levelLabel = 'NOTICE';
                break;
            case self::LEVEL_INFO:
                $buf .= ' lst-log-level-cell--info">INFO';
                $levelLabel = 'INFO';
                break;
            default:
                $buf .= ' lst-log-level-cell--debug">DEBUG';
        }
        $buf .= '</td><td>' . $message . '</td></tr>' . "\n";
        $this->_output .= $buf;

        $textMessage = preg_replace('/<br\s*\/?>/i', "\n", $message);
        $textMessage = html_entity_decode(strip_tags($textMessage), ENT_QUOTES | ENT_HTML5, 'UTF-8');
        $textMessage = preg_replace("/\R+/", "\n", $textMessage);
        $textMessage = trim($textMessage, "\r\n");
        $this->_textOutput .= $time . "\t" . $levelLabel . "\t" . $textMessage . "\n";
    }

    public function Set($field, $value)
    {
        switch ($field) {
            case self::FLD_LEVEL: $this->_level = $value;
                break;
            case self::FLD_FROMPOS: $this->_frompos = $value;
                break;
            case self::FLD_FROMINPUT: $this->_frominput = $value;
                break;
            case self::FLD_TOTALSEARCHED: $this->_totallines = $value;
                break;
            case self::FLD_FILE_SIZE: $this->_filesize = $value;
                break;
            case self::FLD_SEARCH_LIMITED: $this->_searchLimited = $value;
                break;
        }
    }

    public function SetRange($from, $size)
    {
        if ($from < 0 && $from !== self::POS_FILEEND) {
            $from = 0;
        }
        $this->_frominput = $from;
        $this->_blksize = $size;
        $this->_output = '';
        $this->_textOutput = '';
        $this->_searchLimited = 0;
    }

    public function SetMesg($mesg)
    {
        $this->_outmesg = $mesg;
    }
}
