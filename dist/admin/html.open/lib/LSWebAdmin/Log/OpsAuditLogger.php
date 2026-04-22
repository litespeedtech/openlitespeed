<?php

namespace LSWebAdmin\Log;

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\Auth\FileThrottleStore;

/**
 * Operations audit logger.
 *
 * Writes one JSON line per auditable event to a dedicated log file under
 * the admin-owned base directory. Supports size-based and daily rotation.
 *
 * File: SERVER_ROOT/admin/tmp/ops_audit.log
 */
class OpsAuditLogger
{
    const FILENAME = 'ops_audit.log';
    const MAX_FILE_SIZE = 5242880; // 5 MB
    const MAX_ROTATED = 30;

    /** @var string|null Override for the log directory (testing). */
    private static $dirOverride = null;

    /** @var int Effective max rotated files. Defaults to MAX_ROTATED. */
    private static $maxRotated = self::MAX_ROTATED;

    // ── Convenience wrappers ────────────────────────────────────────

    public static function configSave($target, $detail)
    {
        self::log('config_save', $target, $detail);
    }

    public static function configDelete($target, $detail)
    {
        self::log('config_delete', $target, $detail);
    }

    public static function configAdd($target, $detail)
    {
        self::log('config_add', $target, $detail);
    }

    public static function configInstantiate($target, $detail)
    {
        self::log('config_instantiate', $target, $detail);
    }

    public static function restart()
    {
        self::log('restart', 'server', 'graceful restart');
    }

    public static function toggleDebug($state)
    {
        self::log('toggle_debug', 'server', $state ? 'debug on' : 'debug off');
    }

    public static function vhostEnable($vhname)
    {
        self::log('vhost_enable', $vhname, '');
    }

    public static function vhostDisable($vhname)
    {
        self::log('vhost_disable', $vhname, '');
    }

    public static function vhostReload($vhname)
    {
        self::log('vhost_reload', $vhname, '');
    }

    // ── Core writer ─────────────────────────────────────────────────

    /**
     * Write one JSON audit line.
     *
     * @param string $action One of the defined action constants.
     * @param string $target What was affected (config path, vhost name, etc.).
     * @param string $detail Human-readable description of the change.
     * @param string $source Where the action came from: ui, api, system.
     */
    public static function log($action, $target, $detail, $source = 'ui')
    {
        $entry = array(
            'ts'     => gmdate('Y-m-d\TH:i:s\Z'),
            'user'   => self::resolveUser(),
            'ip'     => self::resolveIp(),
            'action' => (string) $action,
            'target' => (string) $target,
            'detail' => (string) $detail,
            'source' => (string) $source,
        );

        $json = json_encode($entry, JSON_UNESCAPED_SLASHES);
        if ($json === false) {
            return;
        }

        $file = self::getLogPath();
        if ($file === '') {
            return;
        }

        self::rotateIfNeeded($file);

        $dir = dirname($file);
        if (!is_dir($dir)) {
            if (!@mkdir($dir, 0700, true)) {
                error_log('[WebAdmin Console] Failed to create ops audit directory: ' . $dir);
                return;
            }
        }

        if (@file_put_contents($file, $json . "\n", FILE_APPEND | LOCK_EX) === false) {
            error_log('[WebAdmin Console] Failed to write ops audit log: ' . $file);
        }
    }

    // ── Rotation ────────────────────────────────────────────────────

    /**
     * Rotate the log file if it exceeds the size threshold or if the
     * day has changed since the file was last modified.
     */
    private static function rotateIfNeeded($file)
    {
        if (!file_exists($file)) {
            return;
        }

        $size = @filesize($file);
        $mtime = @filemtime($file);

        $shouldRotate = false;

        if ($size !== false && $size >= self::MAX_FILE_SIZE) {
            $shouldRotate = true;
        }

        if ($mtime !== false && gmdate('Y-m-d', $mtime) !== gmdate('Y-m-d')) {
            $shouldRotate = true;
        }

        if (!$shouldRotate) {
            return;
        }

        self::rotate($file);
    }

    /**
     * Shift existing rotated files and move the current log to .1.
     */
    private static function rotate($file)
    {
        $max = self::$maxRotated;

        // Remove files beyond the configured max.
        // This also cleans up excess files if the admin lowered the setting.
        // Scan up to 110 to cover the full config range (1–100) plus margin.
        for ($i = $max; $i <= 110; $i++) {
            $f = $file . '.' . $i;
            if (file_exists($f)) {
                @unlink($f);
            } else {
                break;
            }
        }

        // Shift .N-1 → .N, .N-2 → .N-1, ... , .1 → .2
        for ($i = $max - 1; $i >= 1; $i--) {
            $src = $file . '.' . $i;
            $dst = $file . '.' . ($i + 1);
            if (file_exists($src)) {
                @rename($src, $dst);
            }
        }

        // Move current log to .1
        @rename($file, $file . '.1');
    }

    // ── Path resolution ─────────────────────────────────────────────

    /**
     * Get the full path to the ops audit log file.
     *
     * @return string
     */
    public static function getLogPath()
    {
        if (self::$dirOverride !== null) {
            return rtrim(self::$dirOverride, '/') . '/' . self::FILENAME;
        }

        $base = FileThrottleStore::getDefaultBaseDir();
        if ($base === '') {
            return '';
        }

        return $base . '/' . self::FILENAME;
    }

    /**
     * Set the max number of rotated files to retain.
     *
     * Called from ControllerBase after admin config is loaded.
     * Also persisted in the session for cross-page availability.
     *
     * @param int $count 1–100.
     */
    public static function setMaxRotatedFiles($count)
    {
        $count = (int) $count;
        if ($count >= 1 && $count <= 100) {
            $previous = self::$maxRotated;
            self::$maxRotated = $count;

            // If the limit was lowered, immediately delete excess files.
            if ($count < $previous) {
                self::pruneExcessFiles($count);
            }
        }
    }

    /**
     * Delete rotated files beyond the configured max.
     *
     * @param int $max The new max rotated file count.
     */
    private static function pruneExcessFiles($max)
    {
        $file = self::getLogPath();
        if ($file === '') {
            return;
        }

        for ($i = $max + 1; $i <= 110; $i++) {
            $f = $file . '.' . $i;
            if (file_exists($f)) {
                @unlink($f);
            } else {
                break;
            }
        }
    }

    /**
     * Get the effective max rotated files setting.
     *
     * @return int
     */
    public static function getMaxRotatedFiles()
    {
        return self::$maxRotated;
    }

    /**
     * Override the log directory (for testing).
     *
     * @param string|null $dir
     */
    public static function setDirOverride($dir)
    {
        self::$dirOverride = $dir;
    }

    // ── Reader ───────────────────────────────────────────────────────

    /**
     * Read recent entries from the current (and optionally rotated) log files.
     *
     * Returns an array of decoded associative arrays, newest first.
     *
     * @param int $maxEntries Maximum entries to return.
     * @param int $maxFiles   How many rotated files to scan (0 = current only).
     * @return array
     */
    public static function readRecent($maxEntries = 500, $maxFiles = 2)
    {
        $entries = [];
        $file = self::getLogPath();
        if ($file === '' || !file_exists($file)) {
            return $entries;
        }

        // Collect from current file first, then rotated files.
        $files = [$file];
        for ($i = 1; $i <= $maxFiles; $i++) {
            $rotated = $file . '.' . $i;
            if (file_exists($rotated)) {
                $files[] = $rotated;
            }
        }

        foreach ($files as $f) {
            $lines = @file($f, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
            if (!is_array($lines)) {
                continue;
            }
            foreach ($lines as $line) {
                $decoded = json_decode($line, true);
                if (is_array($decoded) && isset($decoded['ts'])) {
                    $entries[] = $decoded;
                }
            }
            if (count($entries) >= $maxEntries * 3) {
                break; // enough raw data, will be sorted and trimmed
            }
        }

        // Sort newest first by timestamp.
        usort($entries, function ($a, $b) {
            return strcmp(
                isset($b['ts']) ? $b['ts'] : '',
                isset($a['ts']) ? $a['ts'] : ''
            );
        });

        return array_slice($entries, 0, $maxEntries);
    }

    /**
     * Get the set of distinct action values found in recent entries.
     *
     * @param array $entries Pre-read entries from readRecent().
     * @return string[]
     */
    public static function getDistinctActions($entries)
    {
        $actions = [];
        foreach ($entries as $e) {
            if (isset($e['action']) && $e['action'] !== '') {
                $actions[$e['action']] = true;
            }
        }
        ksort($actions);
        return array_keys($actions);
    }

    // ── Context resolution ──────────────────────────────────────────

    private static function resolveUser()
    {
        if (!class_exists('LSWebAdmin\\Auth\\CAuthorizer', false)) {
            return '';
        }

        try {
            return CAuthorizer::getUserId();
        } catch (\Exception $e) {
            return '';
        }
    }

    private static function resolveIp()
    {
        return isset($_SERVER['REMOTE_ADDR']) ? (string) $_SERVER['REMOTE_ADDR'] : '';
    }
}
