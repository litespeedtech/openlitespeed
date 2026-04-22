<?php

namespace LSWebAdmin\Auth;

/**
 * File-based persistence backend for IP throttle state.
 *
 * Each tracked IP+bucket combination is stored as a JSON file under the
 * throttle directory. The store handles atomic file operations and stale
 * file cleanup. It is intentionally isolated from the throttle policy
 * logic so it can be unit-tested independently and swapped for a
 * different backend (shared memory, database) in the future.
 *
 * File naming: {bucket}_{ip_hash}.json where ip_hash = substr(md5(ip), 0, 12).
 */
class FileThrottleStore
{
    const DEFAULT_BASE_SUBDIR = '/admin/tmp';
    const DEFAULT_THROTTLE_SUBDIR = '/.throttle';

    private $dir;

    /**
     * @param string|null $dir Override the storage directory (useful for testing).
     */
    public function __construct($dir = null)
    {
        $this->dir = ($dir !== null) ? rtrim($dir, '/') : self::getDefaultDir();
    }

    /**
     * Resolve the default base directory for admin-owned temp/auth files.
     *
     * @return string
     */
    public static function getDefaultBaseDir()
    {
        $serverRoot = self::resolveServerRoot();
        if ($serverRoot !== '') {
            return $serverRoot . self::DEFAULT_BASE_SUBDIR;
        }

        return self::DEFAULT_BASE_SUBDIR;
    }

    /**
     * Resolve the default throttle directory.
     *
     * @return string
     */
    public static function getDefaultDir()
    {
        $baseDir = self::getDefaultBaseDir();
        return rtrim($baseDir, '/') . self::DEFAULT_THROTTLE_SUBDIR;
    }

    /**
     * Load throttle state for an IP + bucket.
     *
     * Returns null if no state exists or if the stored IP doesn't match
     * (hash collision guard).
     *
     * @param string $ip   Raw IP address.
     * @param string $bucket Action bucket name (e.g. 'login', 'abuse', 'api').
     * @return array|null  Decoded state array or null.
     */
    public function load($ip, $bucket)
    {
        $file = $this->filePath($ip, $bucket);
        if (!is_readable($file)) {
            return null;
        }

        $raw = @file_get_contents($file);
        if ($raw === false || $raw === '') {
            return null;
        }

        $data = json_decode($raw, true);
        if (!is_array($data)) {
            return null;
        }

        // Guard against hash collisions: verify stored IP matches.
        if (!isset($data['ip']) || $data['ip'] !== $ip) {
            return null;
        }

        return $data;
    }

    /**
     * Save throttle state for an IP + bucket.
     *
     * Uses atomic write (write to .tmp then rename) to avoid partial reads.
     *
     * @param string $ip     Raw IP address.
     * @param string $bucket Action bucket name.
     * @param array  $data   State array to persist.
     * @return bool  True on success.
     */
    public function save($ip, $bucket, array $data)
    {
        if (!$this->ensureDir()) {
            return false;
        }

        // Always store the raw IP and bucket for verification and debugging.
        $data['ip'] = $ip;
        $data['bucket'] = $bucket;

        $json = json_encode($data, JSON_PRETTY_PRINT);
        if ($json === false) {
            error_log('[WebAdmin Console] Failed to encode throttle state JSON for bucket ' . $bucket);
            return false;
        }

        $file = $this->filePath($ip, $bucket);
        $tmp = $file . '.tmp';

        if (@file_put_contents($tmp, $json, LOCK_EX) === false) {
            error_log('[WebAdmin Console] Failed to write throttle temp file: ' . $tmp . self::formatLastPhpError());
            return false;
        }

        @chmod($tmp, 0600);

        if (!@rename($tmp, $file)) {
            error_log('[WebAdmin Console] Failed to finalize throttle file: ' . $file . self::formatLastPhpError());
            @unlink($tmp);
            return false;
        }

        return true;
    }

    /**
     * Delete throttle state for an IP + bucket.
     *
     * @param string $ip     Raw IP address.
     * @param string $bucket Action bucket name.
     * @return bool  True if deleted or already absent.
     */
    public function delete($ip, $bucket)
    {
        $file = $this->filePath($ip, $bucket);
        if (file_exists($file)) {
            return @unlink($file);
        }
        return true;
    }

    /**
     * Remove stale throttle files older than $maxAge seconds.
     *
     * This runs opportunistically during normal operations (not via cron)
     * to maintain the zero-dependency model.
     *
     * @param int $maxAge Maximum file age in seconds (default: 8 hours = maxBackoff * 2).
     * @return int Number of files cleaned up.
     */
    public function cleanup($maxAge = 28800)
    {
        $cleaned = 0;

        if (!is_dir($this->dir)) {
            return $cleaned;
        }

        $now = time();
        $handle = @opendir($this->dir);
        if ($handle === false) {
            return $cleaned;
        }

        while (($entry = readdir($handle)) !== false) {
            if ($entry === '.' || $entry === '..') {
                continue;
            }

            // Only process .json throttle files.
            if (substr($entry, -5) !== '.json') {
                continue;
            }

            $path = $this->dir . '/' . $entry;
            if (!is_file($path)) {
                continue;
            }

            $mtime = @filemtime($path);
            if ($mtime !== false && ($now - $mtime) > $maxAge) {
                if (@unlink($path)) {
                    $cleaned++;
                }
            }
        }

        closedir($handle);
        return $cleaned;
    }

    /**
     * List all currently tracked throttle records.
     *
     * Useful for admin reporting / dashboard display.
     *
     * @param string|null $bucket Filter by bucket name, or null for all.
     * @return array Array of state arrays.
     */
    public function listAll($bucket = null)
    {
        $results = [];

        if (!is_dir($this->dir)) {
            return $results;
        }

        $handle = @opendir($this->dir);
        if ($handle === false) {
            return $results;
        }

        while (($entry = readdir($handle)) !== false) {
            if ($entry === '.' || $entry === '..') {
                continue;
            }
            if (substr($entry, -5) !== '.json') {
                continue;
            }

            $path = $this->dir . '/' . $entry;
            $raw = @file_get_contents($path);
            if ($raw === false) {
                continue;
            }

            $data = json_decode($raw, true);
            if (!is_array($data)) {
                continue;
            }

            if ($bucket !== null && (!isset($data['bucket']) || $data['bucket'] !== $bucket)) {
                continue;
            }

            $results[] = $data;
        }

        closedir($handle);
        return $results;
    }

    /**
     * Get the storage directory path.
     *
     * @return string
     */
    public function getDir()
    {
        return $this->dir;
    }

    /**
     * Build the file path for an IP + bucket combination.
     *
     * @param string $ip     Raw IP address.
     * @param string $bucket Action bucket name.
     * @return string Full file path.
     */
    private function filePath($ip, $bucket)
    {
        $hash = substr(md5($this->normalizeIp($ip)), 0, 12);
        $safeBucket = preg_replace('/[^a-zA-Z0-9_]/', '', $bucket);
        return $this->dir . '/' . $safeBucket . '_' . $hash . '.json';
    }

    /**
     * Normalize an IP address for consistent hashing.
     *
     * IPv6 addresses are lowercased. If inet_pton/inet_ntop are available,
     * the address is round-tripped through them to canonicalize compressed
     * and expanded forms.
     *
     * @param string $ip Raw IP address.
     * @return string Normalized IP.
     */
    private function normalizeIp($ip)
    {
        $ip = strtolower(trim($ip));

        // Normalize IPv6 via inet_pton/inet_ntop if available.
        if (strpos($ip, ':') !== false && function_exists('inet_pton') && function_exists('inet_ntop')) {
            $packed = @inet_pton($ip);
            if ($packed !== false) {
                $canonical = @inet_ntop($packed);
                if ($canonical !== false) {
                    return $canonical;
                }
            }
        }

        return $ip;
    }

    /**
     * Ensure the throttle directory exists.
     *
     * @return bool True if the directory exists or was created.
     */
    private function ensureDir()
    {
        if (is_dir($this->dir)) {
            return true;
        }

        if (@mkdir($this->dir, 0700, true)) {
            return true;
        }

        error_log('[WebAdmin Console] Failed to create throttle directory: ' . $this->dir . self::formatLastPhpError());
        return false;
    }

    /**
     * Resolve SERVER_ROOT without requiring the full view bootstrap.
     *
     * @return string
     */
    private static function resolveServerRoot()
    {
        if (defined('SERVER_ROOT')) {
            $serverRoot = SERVER_ROOT;
            if (is_string($serverRoot) && $serverRoot !== '') {
                return rtrim($serverRoot, '/');
            }
        }

        if (isset($_SERVER['LS_SERVER_ROOT']) && is_string($_SERVER['LS_SERVER_ROOT']) && $_SERVER['LS_SERVER_ROOT'] !== '') {
            return rtrim($_SERVER['LS_SERVER_ROOT'], '/');
        }

        $envRoot = getenv('LS_SERVER_ROOT');
        if (is_string($envRoot) && $envRoot !== '') {
            return rtrim($envRoot, '/');
        }

        if (is_dir('/usr/local/lsws/admin')) {
            return '/usr/local/lsws';
        }

        $repoRoot = realpath(__DIR__ . '/../../../');
        if ($repoRoot !== false) {
            return rtrim($repoRoot, '/');
        }

        return '';
    }

    private static function formatLastPhpError()
    {
        $error = error_get_last();
        if (!is_array($error) || !isset($error['message']) || !is_string($error['message']) || $error['message'] === '') {
            return '';
        }

        return ' (' . $error['message'] . ')';
    }
}
