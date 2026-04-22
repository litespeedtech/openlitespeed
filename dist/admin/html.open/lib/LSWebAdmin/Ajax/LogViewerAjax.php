<?php

namespace LSWebAdmin\Ajax;

use LSWebAdmin\Log\LogFilter;
use LSWebAdmin\Log\LogViewer;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Runtime\SInfo;
use LSWebAdmin\UI\UIBase;

class LogViewerAjax
{
    public static function viewLog()
    {
        $logfilter = Service::ServiceData(SInfo::DATA_VIEW_LOG);
        echo json_encode(self::buildResponse($logfilter, 'browse'));
    }

    public static function searchLog()
    {
        $browseFilter = Service::ServiceData(SInfo::DATA_VIEW_LOG);
        $defaultFile = (string) $browseFilter->Get(LogFilter::FLD_LOGFILE);
        $requestedFile = UIBase::GrabGoodInput('any', 'filename');
        $searchTerm = UIBase::GrabInput('any', 'searchterm');
        $level = UIBase::GrabGoodInput('any', 'sellevel', 'int');
        if ($level <= 0) {
            $level = $browseFilter->Get(LogFilter::FLD_LEVEL);
        }

        $logfilter = LogViewer::SearchLog($defaultFile, $requestedFile, $searchTerm, $level, LogViewer::SEARCH_MAX_RESULTS);
        echo json_encode(self::buildResponse($logfilter, 'search', (string) $searchTerm, $browseFilter));
    }

    public static function downloadLog()
    {
        $file = UIBase::GrabGoodInput('get', 'filename');
        $scope = trim((string) UIBase::GrabGoodInput('get', 'scope'));
        $searchTerm = UIBase::GrabInput('get', 'searchterm');
        $timestamp = date('Ymd_His');

        if ($scope === 'current') {
            $browseFilter = Service::ServiceData(SInfo::DATA_VIEW_LOG);
            $currentFile = (string) $browseFilter->Get(LogFilter::FLD_LOGFILE);
            $baseName = pathinfo($currentFile, PATHINFO_FILENAME);

            if ($baseName === '') {
                $baseName = 'server_log';
            }

            if (trim((string) $searchTerm) !== '') {
                $level = UIBase::GrabGoodInput('get', 'sellevel', 'int');
                if ($level <= 0) {
                    $level = $browseFilter->Get(LogFilter::FLD_LEVEL);
                }

                $logfilter = LogViewer::SearchLog($currentFile, $file, $searchTerm, $level, LogViewer::SEARCH_MAX_RESULTS);
                $downloadName = $baseName . '_search_' . $timestamp . '.txt';
            } else {
                $logfilter = $browseFilter;
                $downloadName = $baseName . '_selection_' . $timestamp . '.txt';
            }

            self::prepareDownloadResponse();
            self::sendAttachmentHeaders($downloadName, 'text/plain; charset=UTF-8');
            echo $logfilter->GetTextOutput();
            exit;
        }

        $browseFilter = Service::ServiceData(SInfo::DATA_VIEW_LOG);
        $allowedFile = LogViewer::ResolveAllowedLogFile((string) $browseFilter->Get(LogFilter::FLD_LOGFILE), $file);

        if ($allowedFile !== '') {
            $file = $allowedFile;
            $baseName = pathinfo($file, PATHINFO_FILENAME);
            $extension = pathinfo($file, PATHINFO_EXTENSION);
            $downloadName = ($baseName !== '' ? $baseName : basename($file));

            $downloadName .= '_' . $timestamp;
            if ($extension !== '') {
                $downloadName .= '.' . $extension;
            }

            if (self::streamFileSnapshot($file, $downloadName)) {
                exit;
            }

            error_log('download log ' . $file . ' failed to stream');
        } else {
            error_log("download log $file not exist");
        }
    }

    private static function buildCurrentSelectionRows($logHtml)
    {
        $rows = [];

        if (!is_string($logHtml) || trim($logHtml) === '') {
            return $rows;
        }

        if (!class_exists('DOMDocument')) {
            return $rows;
        }

        $dom = new \DOMDocument();
        $wrappedHtml = '<table><tbody>' . $logHtml . '</tbody></table>';

        if (!@$dom->loadHTML('<?xml encoding="UTF-8">' . $wrappedHtml, LIBXML_HTML_NOIMPLIED | LIBXML_HTML_NODEFDTD)) {
            return $rows;
        }

        foreach ($dom->getElementsByTagName('tr') as $tr) {
            $cells = $tr->getElementsByTagName('td');
            if ($cells->length < 3) {
                continue;
            }

            $messageHtml = '';
            for ($i = 0, $childCount = $cells->item(2)->childNodes->length; $i < $childCount; ++$i) {
                $messageHtml .= $dom->saveHTML($cells->item(2)->childNodes->item($i));
            }

            $messageText = preg_replace('/<br\s*\/?>/i', "\n", $messageHtml);
            $messageText = html_entity_decode(strip_tags((string) $messageText), ENT_QUOTES | ENT_HTML5, 'UTF-8');
            $messageText = preg_replace("/\R+/", "\n", $messageText);
            $messageText = trim((string) $messageText, "\r\n");

            $rows[] = [
                'time' => trim($cells->item(0)->textContent),
                'level' => trim($cells->item(1)->textContent),
                'message' => $messageText,
            ];
        }

        return $rows;
    }

    private static function buildResponse($logfilter, $mode = 'browse', $searchTerm = '', $browseFilter = null)
    {
        if ($browseFilter === null) {
            $browseFilter = $logfilter;
        }

        $logBody = $logfilter->GetLogOutput();
        $browseWindow = LogViewer::GetBrowseWindow($browseFilter);

        $res = [
            'mode' => $mode,
            'logfound' => $logfilter->Get(LogFilter::FLD_TOTALFOUND),
            'logsearched' => $logfilter->Get(LogFilter::FLD_TOTALSEARCHED),
            'loglevel' => $logfilter->Get(LogFilter::FLD_LEVEL),
            'logfoundmesg' => $logfilter->Get(LogFilter::FLD_OUTMESG),
            'cur_log_file' => $logfilter->Get(LogFilter::FLD_LOGFILE),
            'cur_log_size' => $logfilter->Get(LogFilter::FLD_FILE_SIZE),
            'sellevel' => $logfilter->Get(LogFilter::FLD_LEVEL),
            'startpos' => $browseFilter->Get(LogFilter::FLD_FROMPOS),
            'blksize' => $browseFilter->Get(LogFilter::FLD_BLKSIZE),
            'browse_start_kb' => $browseWindow['start_kb'],
            'browse_end_kb' => $browseWindow['end_kb'],
            'browse_total_kb' => $browseWindow['total_kb'],
            'browse_at_start' => $browseWindow['at_start'],
            'browse_at_end' => $browseWindow['at_end'],
            'log_body' => $logBody,
            'download_rows' => self::buildCurrentSelectionRows($logBody),
        ];

        if ($mode == 'search') {
            $res['searchterm'] = $searchTerm;
            $res['search_total'] = $logfilter->Get(LogFilter::FLD_TOTALSEARCHED);
            $res['search_displayed'] = $logfilter->Get(LogFilter::FLD_TOTALFOUND);
            $res['search_limited'] = $logfilter->Get(LogFilter::FLD_SEARCH_LIMITED);
            $res['search_max_results'] = LogViewer::SEARCH_MAX_RESULTS;
        }

        return $res;
    }

    private static function prepareDownloadResponse()
    {
        while (ob_get_level() > 0) {
            ob_end_clean();
        }

        if (function_exists('apache_setenv')) {
            @apache_setenv('no-gzip', '1');
        }

        @ini_set('zlib.output_compression', 'Off');
        @ini_set('output_buffering', 'Off');
        @set_time_limit(0);
        ignore_user_abort(true);
    }

    private static function sanitizeDownloadName($downloadName)
    {
        $downloadName = str_replace(["\r", "\n"], '', (string) $downloadName);
        $downloadName = preg_replace('/[^A-Za-z0-9._-]+/', '_', $downloadName);

        if ($downloadName === null || $downloadName === '' || $downloadName === '.' || $downloadName === '..') {
            return 'server_log.txt';
        }

        return $downloadName;
    }

    private static function sendAttachmentHeaders($downloadName, $contentType)
    {
        $safeName = self::sanitizeDownloadName($downloadName);

        header('Content-Description: File Transfer');
        header('Content-Type: ' . $contentType);
        header('Content-Disposition: attachment; filename="' . addcslashes($safeName, '\\"') . '"');
        header('X-Content-Type-Options: nosniff');
        header('Expires: 0');
        header('Cache-Control: private, no-store, no-cache, must-revalidate');
        header('Pragma: public');
        header('Accept-Ranges: none');

        if (function_exists('header_remove')) {
            header_remove('Content-Length');
        }
    }

    private static function streamFileSnapshot($file, $downloadName)
    {
        $handle = @fopen($file, 'rb');
        if ($handle === false) {
            return false;
        }

        $fileStat = @fstat($handle);
        $remaining = (is_array($fileStat) && isset($fileStat['size'])) ? max(0, (int) $fileStat['size']) : null;
        $chunkSize = 1048576;

        self::prepareDownloadResponse();
        self::sendAttachmentHeaders($downloadName, 'application/octet-stream');

        while (!feof($handle) && ($remaining === null || $remaining > 0)) {
            if (connection_aborted()) {
                break;
            }

            $readLength = ($remaining === null) ? $chunkSize : min($chunkSize, $remaining);
            $buffer = fread($handle, $readLength);

            if ($buffer === false || $buffer === '') {
                break;
            }

            echo $buffer;

            if ($remaining !== null) {
                $remaining -= strlen($buffer);
            }

            flush();
        }

        fclose($handle);

        if ($remaining !== null && $remaining > 0 && !connection_aborted()) {
            error_log('logviewer snapshot download ended early for ' . $file);
        }

        return true;
    }
}
