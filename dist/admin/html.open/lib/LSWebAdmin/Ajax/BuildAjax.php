<?php

namespace LSWebAdmin\Ajax;

class BuildAjax
{
    public static function buildProgress()
    {
        $progressFile = isset($_SESSION['progress_file']) ? $_SESSION['progress_file'] : '';
        $logFile = isset($_SESSION['log_file']) ? $_SESSION['log_file'] : '';

        if ($progressFile === '' || $logFile === '' || !is_readable($progressFile) || !is_readable($logFile)) {
            return;
        }

        echo file_get_contents($progressFile);
        echo "\n**LOG_DETAIL** retrieved from $logFile\n";
        echo file_get_contents($logFile);
    }
}
