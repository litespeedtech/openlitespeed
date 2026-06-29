<?php

namespace LSWebAdmin\Config\IO;

use LSWebAdmin\Util\PathTool;

class ConfigAutoBackupService
{
    const DEFAULT_RETENTION_DAYS = 90;
    const MIN_RETENTION_DAYS = 3;
    const MAX_RETENTION_DAYS = 3650;

    private static $_retentionDays = self::DEFAULT_RETENTION_DAYS;

    public static function setRetentionDays($days)
    {
        $days = (int) $days;
        if ($days < self::MIN_RETENTION_DAYS || $days > self::MAX_RETENTION_DAYS) {
            $days = self::DEFAULT_RETENTION_DAYS;
        }

        self::$_retentionDays = $days;
    }

    public static function getRetentionDays()
    {
        return self::$_retentionDays;
    }

    public static function backupExistingConfig($filepath)
    {
        if (!is_string($filepath) || $filepath === '' || !file_exists($filepath)) {
            return true;
        }

        $paths = self::getBackupPathParts($filepath);
        if ($paths === null) {
            error_log('config autobackup skipped: path not under conf/ or admin/conf/: ' . $filepath);
            return true;
        }

        $backupPath = self::nextBackupPath($filepath);
        if ($backupPath === null) {
            error_log('config autobackup skipped: collision counter exhausted for ' . $filepath);
            return true;
        }

        $dir = dirname($backupPath);
        if (!is_dir($dir)) {
            if (!self::createBackupDir($dir)) {
                error_log('failed to create config autobackup directory ' . $dir);
                return false;
            }
        }

        if (!@copy($filepath, $backupPath)) {
            error_log('failed to copy config autobackup from ' . $filepath . ' to ' . $backupPath);
            return false;
        }

        // Backups are kept private; do not inherit potentially world-readable
        // permissions from the source config.
        @chmod($backupPath, 0600);

        return true;
    }

    public static function pruneExpiredBackups()
    {
        $backupRoot = self::getBackupRoot();
        if ($backupRoot === null || !is_dir($backupRoot)) {
            return true;
        }

        $cutoff = time() - (self::$_retentionDays * 86400);
        self::pruneDir($backupRoot, $cutoff, true);

        return true;
    }

    public static function nextBackupPath($filepath)
    {
        $base = self::backupPathBase($filepath);
        if ($base === null) {
            return null;
        }

        $prefix = $base . '.' . date('Ymd-His');
        for ($i = 1; $i < 10000; ++$i) {
            $candidate = $prefix . '.' . $i;
            if (!file_exists($candidate)) {
                return $candidate;
            }
        }

        return null;
    }

    public static function backupPathBase($filepath)
    {
        $paths = self::getBackupPathParts($filepath);
        if ($paths === null) {
            return null;
        }

        return $paths['backupRoot'] . $paths['relativePath'];
    }

    public static function getBackupRoot()
    {
        if (!defined('SERVER_ROOT') || SERVER_ROOT === '') {
            return null;
        }

        return rtrim(PathTool::clean(SERVER_ROOT), '/') . '/conf/autobackup/';
    }

    private static function getBackupPathParts($filepath)
    {
        if (!defined('SERVER_ROOT') || SERVER_ROOT === '') {
            return null;
        }

        $serverRoot = rtrim(PathTool::clean(SERVER_ROOT), '/') . '/';
        $path = PathTool::clean($filepath);
        $confRoot = $serverRoot . 'conf/';
        $adminConfRoot = $serverRoot . 'admin/conf/';
        $backupRoot = $confRoot . 'autobackup/';

        if (strpos($path, $backupRoot) === 0) {
            return null;
        }

        if (strpos($path, $confRoot) === 0) {
            return [
                'backupRoot' => $backupRoot,
                'relativePath' => substr($path, strlen($confRoot)),
            ];
        }

        if (strpos($path, $adminConfRoot) === 0) {
            return [
                'backupRoot' => $backupRoot,
                'relativePath' => 'admin/' . substr($path, strlen($adminConfRoot)),
            ];
        }

        return null;
    }

    private static function createBackupDir($dir)
    {
        $err = '';
        return PathTool::createDir($dir, 0700, $err);
    }

    private static function pruneDir($dir, $cutoff, $isRoot)
    {
        if (is_link($dir)) {
            return;
        }

        $items = @scandir($dir);
        if (!is_array($items)) {
            error_log('failed to scan config autobackup directory ' . $dir);
            return;
        }

        foreach ($items as $item) {
            if ($item === '.' || $item === '..') {
                continue;
            }

            $path = $dir . '/' . $item;
            if (is_link($path)) {
                continue;
            }

            if (is_dir($path)) {
                self::pruneDir($path, $cutoff, false);
                self::removeEmptyDir($path);
                continue;
            }

            if (!is_file($path)) {
                continue;
            }

            $timestamp = self::extractBackupTimestamp($item);
            if ($timestamp !== null && $timestamp < $cutoff && !@unlink($path)) {
                error_log('failed to prune config autobackup ' . $path);
            }
        }

        if (!$isRoot) {
            self::removeEmptyDir($dir);
        }
    }

    private static function removeEmptyDir($dir)
    {
        $items = @scandir($dir);
        if (is_array($items) && count($items) === 2) {
            @rmdir($dir);
        }
    }

    private static function extractBackupTimestamp($filename)
    {
        if (!preg_match('/\.(\d{8})-(\d{6})\.\d+$/', $filename, $matches)) {
            return null;
        }

        $date = $matches[1];
        $time = $matches[2];
        $timestamp = mktime(
            (int) substr($time, 0, 2),
            (int) substr($time, 2, 2),
            (int) substr($time, 4, 2),
            (int) substr($date, 4, 2),
            (int) substr($date, 6, 2),
            (int) substr($date, 0, 4)
        );

        return ($timestamp === false) ? null : $timestamp;
    }
}
