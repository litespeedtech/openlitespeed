<?php

namespace LSWebAdmin\Auth;

/**
 * Records and retrieves login history for the WebAdmin console.
 *
 * Stores the last N successful and failed login attempts in a single
 * JSON file under the admin tmp directory. The file is append-on-login,
 * trim-on-read to keep it bounded.
 *
 * File: $SERVER_ROOT/admin/tmp/login_history.json
 */
class LoginHistory
{
    const RETENTION_DAYS = 90;
    const MAX_SUCCESSFUL = 500;
    const MAX_FAILED = 2000;
    const FILENAME = 'login_history.json';

    /** @var string Full path to the history file. */
    private $filePath;

    /** @var int Effective retention in days. Defaults to RETENTION_DAYS. */
    private $retentionDays;

    /**
     * @param string|null $dir Override the storage directory (useful for testing).
     *                         Defaults to FileThrottleStore::getDefaultBaseDir().
     */
    public function __construct($dir = null)
    {
        $base = ($dir !== null) ? rtrim($dir, '/') : FileThrottleStore::getDefaultBaseDir();
        $this->filePath = $base . '/' . self::FILENAME;
        $this->retentionDays = self::RETENTION_DAYS;
    }

    /**
     * Override the retention period.
     *
     * Called from CAuthorizer after the admin config is loaded so that the
     * configurable value takes effect for subsequent reads and writes.
     *
     * @param int $days Retention in days (1–365).
     */
    public function setRetentionDays($days)
    {
        $days = (int) $days;
        if ($days >= 1 && $days <= 365) {
            $this->retentionDays = $days;
        }
    }

    /**
     * Get the effective retention period in days.
     *
     * @return int
     */
    public function getRetentionDays()
    {
        return $this->retentionDays;
    }

    /**
     * Record a successful login.
     *
     * @param string $ip   Client IP address.
     * @param string $user Username that authenticated.
     * @return bool True on success.
     */
    public function recordSuccess($ip, $user)
    {
        $data = $this->loadRaw();

        $entry = [
            'ip'   => $ip,
            'user' => $user,
            'time' => time(),
        ];

        $data['last_successful'][] = $entry;

        // Trim to max entries (keep most recent).
        if (count($data['last_successful']) > self::MAX_SUCCESSFUL) {
            $data['last_successful'] = array_slice(
                $data['last_successful'],
                -self::MAX_SUCCESSFUL
            );
        }

        return $this->saveRaw($data);
    }

    /**
     * Record a failed login attempt.
     *
     * @param string $ip     Client IP address.
     * @param string $user   Attempted username.
     * @param string $reason Failure reason: 'invalid_credentials', 'throttled', 'abuse'.
     * @return bool True on success.
     */
    public function recordFailure($ip, $user, $reason = 'invalid_credentials')
    {
        $data = $this->loadRaw();

        $entry = [
            'ip'     => $ip,
            'user'   => $user,
            'time'   => time(),
            'reason' => $reason,
        ];

        $data['last_failed'][] = $entry;

        // Trim to max entries (keep most recent).
        if (count($data['last_failed']) > self::MAX_FAILED) {
            $data['last_failed'] = array_slice(
                $data['last_failed'],
                -self::MAX_FAILED
            );
        }

        return $this->saveRaw($data);
    }

    /**
     * Get recent successful logins.
     *
     * @param int $limit Maximum number of entries to return (default 5).
     * @return array Array of login entries, most recent first.
     */
    public function getRecentSuccessful($limit = 5)
    {
        $data = $this->loadRaw();
        $entries = $data['last_successful'];

        // Return most recent first.
        $entries = array_reverse($entries);

        if ($limit === null) {
            return $entries;
        }

        return array_slice($entries, 0, $limit);
    }

    /**
     * Get recent failed login attempts.
     *
     * @param int $limit Maximum number of entries to return (default 10).
     * @return array Array of failure entries, most recent first.
     */
    public function getRecentFailed($limit = 10)
    {
        $data = $this->loadRaw();
        $entries = $data['last_failed'];

        // Return most recent first.
        $entries = array_reverse($entries);

        if ($limit === null) {
            return $entries;
        }

        return array_slice($entries, 0, $limit);
    }

    public function getStoragePath()
    {
        return $this->filePath;
    }

    /**
     * Get the last successful login (for "Last login" notification).
     *
     * Returns the second-most-recent successful login (the current one
     * is the first, so the "last" one is the one before it).
     *
     * @return array|null Login entry or null if no prior login exists.
     */
    public function getLastLogin()
    {
        $data = $this->loadRaw();
        $entries = $data['last_successful'];
        $count = count($entries);

        // The most recent entry is the current login; we want the one before it.
        if ($count >= 2) {
            return $entries[$count - 2];
        }

        return null;
    }

    /**
     * Get a summary of failed attempts since the last successful login.
     *
     * Useful for showing "3 failed attempts since your last login" on the
     * dashboard after authentication.
     *
     * @return array Associative array with:
     *   - 'count' int  Number of failed attempts
     *   - 'entries' array  The failed attempt entries
     */
    public function getFailedSinceLastLogin()
    {
        $data = $this->loadRaw();
        $successes = $data['last_successful'];
        $failures = $data['last_failed'];

        // Find the timestamp of the second-most-recent successful login.
        $count = count($successes);
        $lastLoginTime = 0;
        if ($count >= 2) {
            $lastLoginTime = (int) $successes[$count - 2]['time'];
        }

        // Filter failures that occurred after the last login.
        $recent = [];
        foreach ($failures as $entry) {
            if ((int) $entry['time'] > $lastLoginTime) {
                $recent[] = $entry;
            }
        }

        return [
            'count'   => count($recent),
            'entries' => $recent,
        ];
    }

    /**
     * Load raw history data from the JSON file.
     *
     * @return array Normalized history structure.
     */
    private function loadRaw()
    {
        $default = [
            'last_successful' => [],
            'last_failed'     => [],
        ];

        if (!is_readable($this->filePath)) {
            return $default;
        }

        $raw = @file_get_contents($this->filePath);
        if ($raw === false || $raw === '') {
            return $default;
        }

        $data = json_decode($raw, true);
        if (!is_array($data)) {
            return $default;
        }

        if (!isset($data['last_successful']) || !is_array($data['last_successful'])) {
            $data['last_successful'] = [];
        }
        if (!isset($data['last_failed']) || !is_array($data['last_failed'])) {
            $data['last_failed'] = [];
        }

        $data['last_successful'] = $this->pruneEntries($data['last_successful'], self::MAX_SUCCESSFUL);
        $data['last_failed'] = $this->pruneEntries($data['last_failed'], self::MAX_FAILED);

        return $data;
    }

    private function pruneEntries(array $entries, $maxEntries)
    {
        $retentionCutoff = time() - ($this->retentionDays * 86400);
        $filtered = [];

        foreach ($entries as $entry) {
            if (!is_array($entry) || !isset($entry['time'])) {
                continue;
            }

            if ((int) $entry['time'] < $retentionCutoff) {
                continue;
            }

            $filtered[] = $entry;
        }

        if (count($filtered) > $maxEntries) {
            $filtered = array_slice($filtered, -$maxEntries);
        }

        return $filtered;
    }

    /**
     * Save raw history data to the JSON file.
     *
     * Uses atomic write (write to .tmp then rename).
     *
     * @param array $data History structure.
     * @return bool True on success.
     */
    private function saveRaw(array $data)
    {
        $dir = dirname($this->filePath);
        if (!is_dir($dir)) {
            if (!@mkdir($dir, 0700, true)) {
                error_log('[WebAdmin Console] Failed to create login history directory: ' . $dir);
                return false;
            }
        }

        $json = json_encode($data, JSON_PRETTY_PRINT);
        if ($json === false) {
            error_log('[WebAdmin Console] Failed to encode login history JSON.');
            return false;
        }

        $tmp = $this->filePath . '.tmp';
        if (@file_put_contents($tmp, $json, LOCK_EX) === false) {
            error_log('[WebAdmin Console] Failed to write login history temp file: ' . $tmp . $this->formatLastPhpError());
            return false;
        }

        @chmod($tmp, 0600);

        if (!@rename($tmp, $this->filePath)) {
            error_log('[WebAdmin Console] Failed to finalize login history file: ' . $this->filePath . $this->formatLastPhpError());
            @unlink($tmp);
            return false;
        }

        return true;
    }

    private function formatLastPhpError()
    {
        $error = error_get_last();
        if (!is_array($error) || !isset($error['message']) || !is_string($error['message']) || $error['message'] === '') {
            return '';
        }

        return ' (' . $error['message'] . ')';
    }
}
