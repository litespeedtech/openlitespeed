<?php

namespace LSWebAdmin\Log;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\UI\UIBase;

class LogViewer
{
    const SEARCH_MAX_RESULTS = 500;
    const SEARCH_MIN_LENGTH = 3;
    const SEARCH_MAX_LENGTH = 50;
    const BROWSE_DEFAULT_BLOCK_KB = 20;
    const BROWSE_MAX_BLOCK_KB = 2048;

    public static function GetDashErrLog($errorlogfile, $level = LogFilter::LEVEL_NOTICE)
    {
        $filter = new LogFilter($errorlogfile);
        $filter->Set(LogFilter::FLD_LEVEL, $level);
        $filter->SetRange(LogFilter::POS_FILEEND, self::BROWSE_DEFAULT_BLOCK_KB);
        self::loadErrorLog($filter);
        return $filter;
    }

    public static function NormalizeBrowseBlockSize($value)
    {
        $block = (float) $value;
        if ($block <= 0) {
            return self::BROWSE_DEFAULT_BLOCK_KB;
        }

        return min($block, self::BROWSE_MAX_BLOCK_KB);
    }

    public static function GetErrLog($errorlogfile)
    {
        // get from input
        $requestedFile = UIBase::GrabGoodInput('any', 'filename');
        $filename = self::ResolveAllowedLogFile($errorlogfile, $requestedFile);
        if ($filename == '') {
            $filename = self::ResolveAllowedLogFile($errorlogfile);
        }
        if ($requestedFile == '') {
            return self::GetDashErrLog($filename);
        }

        $level = UIBase::GrabGoodInput('ANY', 'sellevel', 'int');
        $startinput = UIBase::GrabGoodInput('any', 'startpos', 'float');
        $block = self::NormalizeBrowseBlockSize(UIBase::GrabGoodInput('any', 'blksize', 'float'));
        $act = UIBase::GrabGoodInput('any', 'act');

        switch ($act) {
            case 'begin':
                $startinput = 0;
                break;
            case 'end':
                $startinput = LogFilter::POS_FILEEND;
                break;
            case 'prev':
                $startinput -= $block;
                break;
            case 'next':
                $startinput += $block;
                break;
        }
        $filter = new LogFilter($filename);

        $filter->Set(LogFilter::FLD_LEVEL, $level);
        $filter->SetRange($startinput, $block);
        $filter->SetMesg('');
        self::loadErrorLog($filter);
        return $filter;
    }

    public static function SearchLog($defaultLogFile, $requestedFile, $searchTerm, $level, $maxResults = self::SEARCH_MAX_RESULTS)
    {
        $filename = self::ResolveAllowedLogFile($defaultLogFile, $requestedFile);
        if ($filename == '') {
            $filename = self::ResolveAllowedLogFile($defaultLogFile);
        }

        $filter = new LogFilter($filename);
        $filter->Set(LogFilter::FLD_LEVEL, $level);
        $filter->Set(LogFilter::FLD_FILE_SIZE, @filesize($filename));
        $filter->SetMesg('');

        $searchTerm = self::NormalizeSearchTerm($searchTerm);
        $validationError = self::ValidateSearchTerm($searchTerm);
        if ($validationError !== '') {
            $filter->SetMesg(DMsg::UIStr($validationError));
            return $filter;
        }

        $matchedLines = self::GetMatchedLineNumbers($filename, $searchTerm);
        if ($matchedLines === null) {
            $filter->SetMesg(DMsg::Err('err_failreadfile') . ': ' . $filename);
            return $filter;
        }

        if (count($matchedLines) == 0) {
            return $filter;
        }

        if (($fd = fopen($filename, 'r')) == false) {
            $filter->SetMesg(DMsg::Err('err_failreadfile') . ': ' . $filename);
            return $filter;
        }

        $maxResults = max(1, (int) $maxResults);
        $currentLine = 0;
        $matchCount = 0;
        $limited = false;
        $currentEntry = null;

        while (($buffer = fgets($fd)) !== false) {
            ++$currentLine;
            $entryData = self::ParseErrorLogHeader($buffer);
            if ($entryData !== null) {
                self::FinalizeSearchEntry($filter, $currentEntry, $level, $maxResults, $matchCount, $limited);
                $currentEntry = $entryData;
                $currentEntry['matched'] = isset($matchedLines[$currentLine]);
            } elseif ($currentEntry !== null) {
                if (isset($matchedLines[$currentLine])) {
                    $currentEntry['matched'] = true;
                }
                $currentEntry['message'] .= '<br>' . htmlspecialchars($buffer);
            }
        }

        fclose($fd);
        self::FinalizeSearchEntry($filter, $currentEntry, $level, $maxResults, $matchCount, $limited);
        $filter->Set(LogFilter::FLD_TOTALSEARCHED, $matchCount);
        $filter->Set(LogFilter::FLD_SEARCH_LIMITED, $limited ? 1 : 0);

        return $filter;
    }

    public static function ResolveAllowedLogFile($defaultLogFile, $requestedFile = '')
    {
        $defaultReal = realpath($defaultLogFile);
        if ($defaultReal === false || !is_file($defaultReal) || !is_readable($defaultReal)) {
            return '';
        }

        if ($requestedFile == '') {
            return $defaultReal;
        }

        $requestedReal = realpath($requestedFile);
        if ($requestedReal === false || $requestedReal !== $defaultReal || !is_file($requestedReal) || !is_readable($requestedReal)) {
            return '';
        }

        return $requestedReal;
    }

    public static function GetBrowseWindow($filter)
    {
        $startKb = self::FilterNumber($filter->Get(LogFilter::FLD_FROMPOS));
        $blockKb = self::NormalizeBrowseBlockSize($filter->Get(LogFilter::FLD_BLKSIZE));
        $totalKb = self::FilterNumber($filter->Get(LogFilter::FLD_FILE_SIZE));

        if ($totalKb < 0) {
            $totalKb = 0;
        }

        $endKb = $startKb + $blockKb;
        if ($blockKb <= 0 || $endKb > $totalKb) {
            $endKb = $totalKb;
        }
        if ($endKb < $startKb) {
            $endKb = $startKb;
        }

        return [
            'start_kb' => round($startKb, 2),
            'end_kb' => round($endKb, 2),
            'total_kb' => round($totalKb, 2),
            'at_start' => ($startKb <= 0.00001),
            'at_end' => ($totalKb <= 0.00001 || $endKb >= ($totalKb - 0.00001)),
        ];
    }

    private static function loadErrorLog($filter)
    {
        $logfile = $filter->Get(LogFilter::FLD_LOGFILE);
        if (($fd = fopen($logfile, 'r')) == false) {
            $filter->SetMesg(DMsg::Err('err_failreadfile') . ': ' . $filter->Get(LogFilter::FLD_LOGFILE));
            return;
        }

        $frominput = $filter->Get(LogFilter::FLD_FROMINPUT);

        $blockKb = self::NormalizeBrowseBlockSize($filter->Get(LogFilter::FLD_BLKSIZE));
        $block = $blockKb * 1024;
        $file_size = filesize($logfile);
        $filter->Set(LogFilter::FLD_FILE_SIZE, $file_size);
        $filter->Set(LogFilter::FLD_BLKSIZE, $blockKb);

        $frompos = (int)(($frominput == LogFilter::POS_FILEEND) ? ($file_size - $block) : ($frominput * 1024));
        if ($frompos < 0) {
            $frompos = 0;
            $endpos = $file_size;
            } else {
                $endpos = $frompos + $block;
        }

        fseek($fd, $frompos);
        $filter->Set(LogFilter::FLD_FROMPOS, $frompos);

        $found = false;
        $totalLine = 0;

        $newlineTag = '[ERR[WAR[NOT[INF[DEB';
        $levels = ['E' => 1, 'W' => 2, 'N' => 3, 'I' => 4, 'D' => 5];
        $filterlevel = $filter->Get(LogFilter::FLD_LEVEL);

        $cur_level = '';
        $cur_time = '';
        $cur_mesg = '';

        while ($buffer = fgets($fd)) {
            // check if new line
            $c28 = substr($buffer, 28, 3);
            if ($c28 && strstr($newlineTag, $c28)) {
                // is new line
                $totalLine ++;
                if ($found) { // finish prior line
                    $filter->AddLogEntry($cur_level, $cur_time, $cur_mesg);
                    $found = false;
                }
                $cur_level = $levels[substr($c28, 0, 1)];
                if ($cur_level <= $filterlevel && preg_match("/^\d{4}-\d{2}-\d{2} /", $buffer)) {
                    // start a new line
                    $found = true;
                    $cur_time = substr($buffer, 0, 26);
                    $cur_mesg = htmlspecialchars(substr($buffer, strpos($buffer, ']', 27) + 2));
                }
            } elseif ($found) { // multi-line output
                $cur_mesg .= '<br>' . htmlspecialchars($buffer);
            }

            $curpos = ftell($fd);
            if ($curpos >= $endpos)
                break;
        }

        fclose($fd);
        if ($found)
            $filter->AddLogEntry($cur_level, $cur_time, $cur_mesg);

        $filter->Set(LogFilter::FLD_TOTALSEARCHED, $totalLine);
    }

    private static function FilterNumber($value)
    {
        return (float) str_replace(',', '', (string) $value);
    }

    private static function NormalizeSearchTerm($searchTerm)
    {
        $searchTerm = (string) $searchTerm;
        $searchTerm = preg_replace('/[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]/', '', $searchTerm);
        if ($searchTerm === null) {
            return '';
        }

        return trim($searchTerm);
    }

    private static function ValidateSearchTerm($searchTerm)
    {
        $length = strlen($searchTerm);

        if ($length < self::SEARCH_MIN_LENGTH || $length > self::SEARCH_MAX_LENGTH) {
            return 'service_logsearchenterterm';
        }

        if (!preg_match('/^[\x20-\x7E]+$/', $searchTerm)) {
            return 'service_logsearchinvalidterm';
        }

        if (preg_match('/[;&|`${}]/', $searchTerm)) {
            return 'service_logsearchinvalidterm';
        }

        if (preg_match('/\\\\[xXuU][0-9a-fA-F]{2}|%[0-9a-fA-F]{2}|[A-Za-z0-9+\/=]{40,}/i', $searchTerm)) {
            return 'service_logsearchinvalidterm';
        }

        return '';
    }

    private static function GetMatchedLineNumbers($logfile, $searchTerm)
    {
        $cmd = 'grep -inF -- ' . escapeshellarg($searchTerm) . ' ' . escapeshellarg($logfile) . ' 2>/dev/null';
        $output = [];
        $status = 0;
        exec($cmd, $output, $status);

        if ($status === 1) {
            return [];
        }

        if ($status !== 0) {
            return null;
        }

        $matchedLines = [];
        foreach ($output as $line) {
            $pos = strpos($line, ':');
            if ($pos === false) {
                continue;
            }

            $lineNumber = (int) substr($line, 0, $pos);
            if ($lineNumber > 0) {
                $matchedLines[$lineNumber] = true;
            }
        }

        return $matchedLines;
    }

    private static function ParseErrorLogHeader($buffer)
    {
        $c28 = substr($buffer, 28, 3);
        if ($c28 == '' || !strstr('[ERR[WAR[NOT[INF[DEB', $c28) || !preg_match("/^\d{4}-\d{2}-\d{2} /", $buffer)) {
            return null;
        }

        $levels = ['E' => 1, 'W' => 2, 'N' => 3, 'I' => 4, 'D' => 5];
        $levelKey = substr($c28, 0, 1);
        if (!isset($levels[$levelKey])) {
            return null;
        }

        return [
            'level' => $levels[$levelKey],
            'time' => substr($buffer, 0, 26),
            'message' => htmlspecialchars(substr($buffer, strpos($buffer, ']', 27) + 2)),
            'matched' => false,
        ];
    }

    private static function FinalizeSearchEntry($filter, &$entry, $filterLevel, $maxResults, &$matchCount, &$limited)
    {
        if ($entry === null || !$entry['matched'] || $entry['level'] > $filterLevel) {
            $entry = null;
            return;
        }

        ++$matchCount;
        if ($matchCount <= $maxResults) {
            $filter->AddLogEntry($entry['level'], $entry['time'], $entry['message']);
        } else {
            $limited = true;
        }

        $entry = null;
    }

}
