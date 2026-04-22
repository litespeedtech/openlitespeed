<?php

namespace LSWebAdmin\Auth;

/**
 * Generic IP-based rate limiter with sliding-window and exponential backoff.
 *
 * This class owns the throttle *policy* (max failures, backoff schedule,
 * block decisions) while FileThrottleStore owns the I/O. Any caller can
 * use a different "bucket" name to get independent rate limiting:
 *
 *   - 'login'  — brute-force login protection
 *   - 'abuse'  — command-injection / suspicious mutation detection
 *   - 'api'    — future REST API rate limiting
 *
 * The throttle is designed to run without a database, cron jobs, or
 * external services.
 */
class IpThrottle
{
    /** @var FileThrottleStore */
    private $store;

    /** @var int Maximum failures before blocking. */
    private $maxFailures;

    /** @var int Initial block duration in seconds. */
    private $blockWindow;

    /** @var int Maximum backoff cap in seconds. */
    private $maxBackoff;

    /** @var bool Whether throttling is enabled. */
    private $enabled;

    /** @var int Counter: how many requests have been processed this lifecycle. */
    private static $requestCounter = 0;

    /** @var int Run cleanup every N requests (opportunistic). */
    const CLEANUP_INTERVAL = 50;

    /**
     * @param FileThrottleStore|null $store   Storage backend (default: new FileThrottleStore).
     * @param array                  $config  Override defaults:
     *   - 'maxFailures'  int  (default 5)
     *   - 'blockWindow'  int  seconds (default 900 = 15 min)
     *   - 'maxBackoff'   int  seconds (default 14400 = 4 hours)
     *   - 'enabled'      bool (default true)
     */
    public function __construct(FileThrottleStore $store = null, array $config = [])
    {
        $this->store = ($store !== null) ? $store : new FileThrottleStore();

        $this->maxFailures = isset($config['maxFailures']) ? (int) $config['maxFailures'] : 5;
        $this->blockWindow = isset($config['blockWindow']) ? (int) $config['blockWindow'] : 900;
        $this->maxBackoff  = isset($config['maxBackoff'])  ? (int) $config['maxBackoff']  : 14400;
        $this->enabled     = isset($config['enabled'])     ? (bool) $config['enabled']    : true;

        // Sanity: enforce minimums.
        if ($this->maxFailures < 1) {
            $this->maxFailures = 1;
        }
        if ($this->blockWindow < 60) {
            $this->blockWindow = 60;
        }
        if ($this->maxBackoff < $this->blockWindow) {
            $this->maxBackoff = $this->blockWindow;
        }
    }

    /**
     * Check whether an IP is currently blocked for a given bucket.
     *
     * @param string $ip     Client IP address.
     * @param string $bucket Action bucket name.
     * @return bool True if blocked.
     */
    public function isBlocked($ip, $bucket)
    {
        if (!$this->enabled) {
            return false;
        }

        $this->maybeCleanup();

        $state = $this->store->load($ip, $bucket);
        if ($state === null) {
            return false;
        }

        if (isset($state['blocked_until']) && $state['blocked_until'] > time()) {
            return true;
        }

        return false;
    }

    /**
     * Record a failed attempt for an IP + bucket.
     *
     * If the failure count reaches the threshold within the sliding window,
     * a block is applied with exponential backoff.
     *
     * @param string $ip     Client IP address.
     * @param string $bucket Action bucket name.
     * @return void
     */
    public function recordFailure($ip, $bucket)
    {
        if (!$this->enabled) {
            return;
        }

        $now = time();
        $state = $this->store->load($ip, $bucket);

        if ($state === null) {
            $state = [
                'failures'      => 0,
                'first_failure' => $now,
                'last_failure'  => $now,
                'blocked_until' => 0,
                'block_count'   => 0,
                'created'       => $now,
            ];
        }

        $state['failures'] = (isset($state['failures']) ? (int) $state['failures'] : 0) + 1;
        $state['last_failure'] = $now;

        if (!isset($state['first_failure'])) {
            $state['first_failure'] = $now;
        }

        if (!isset($state['block_count'])) {
            $state['block_count'] = 0;
        }

        // Check if we should apply a block.
        $failures = (int) $state['failures'];
        if ($failures >= $this->maxFailures) {
            // All failures should be within the sliding window.
            $windowStart = $now - $this->blockWindow;
            $firstFailure = (int) $state['first_failure'];

            if ($firstFailure >= $windowStart || $failures >= $this->maxFailures) {
                // Apply exponential backoff: blockWindow * 2^blockCount, capped at maxBackoff.
                $blockCount = (int) $state['block_count'];
                $duration = $this->blockWindow * pow(2, $blockCount);
                if ($duration > $this->maxBackoff) {
                    $duration = $this->maxBackoff;
                }

                $state['blocked_until'] = $now + $duration;
                $state['block_count'] = $blockCount + 1;

                error_log(
                    '[SECURITY] Login throttle: IP ' . $ip . ' blocked after '
                    . $failures . ' failures (bucket: ' . $bucket
                    . ', duration: ' . $duration . 's)'
                );
            }
        }

        $this->store->save($ip, $bucket, $state);
    }

    /**
     * Clear throttle record for an IP + bucket (e.g. on successful login).
     *
     * @param string $ip     Client IP address.
     * @param string $bucket Action bucket name.
     * @return void
     */
    public function clearRecord($ip, $bucket)
    {
        $this->store->delete($ip, $bucket);
    }

    /**
     * Get the current throttle status for an IP + bucket.
     *
     * @param string $ip     Client IP address.
     * @param string $bucket Action bucket name.
     * @return array Associative array with keys:
     *   - 'blocked'       bool
     *   - 'failures'      int
     *   - 'blocked_until' int|null  Unix timestamp or null
     *   - 'retry_after'   int      Seconds until unblocked (0 if not blocked)
     */
    public function getStatus($ip, $bucket)
    {
        $state = $this->store->load($ip, $bucket);
        $now = time();

        if ($state === null) {
            return [
                'blocked'       => false,
                'failures'      => 0,
                'blocked_until' => null,
                'retry_after'   => 0,
            ];
        }

        $blockedUntil = isset($state['blocked_until']) ? (int) $state['blocked_until'] : 0;
        $isBlocked = ($blockedUntil > $now);

        return [
            'blocked'       => $isBlocked,
            'failures'      => isset($state['failures']) ? (int) $state['failures'] : 0,
            'blocked_until' => $isBlocked ? $blockedUntil : null,
            'retry_after'   => $isBlocked ? ($blockedUntil - $now) : 0,
        ];
    }

    /**
     * Immediately block an IP for a specific bucket (e.g. abuse detection).
     *
     * This bypasses the normal failure-count threshold and applies a block
     * directly. Useful for the 'abuse' bucket where one offense = immediate block.
     *
     * @param string $ip       Client IP address.
     * @param string $bucket   Action bucket name.
     * @param int    $duration Block duration in seconds (default: 3600 = 1 hour).
     * @return void
     */
    public function blockImmediately($ip, $bucket, $duration = 3600)
    {
        if (!$this->enabled) {
            return;
        }

        $now = time();
        $state = $this->store->load($ip, $bucket);

        if ($state === null) {
            $state = [
                'failures'      => 1,
                'first_failure' => $now,
                'last_failure'  => $now,
                'block_count'   => 0,
                'created'       => $now,
            ];
        }

        $blockCount = isset($state['block_count']) ? (int) $state['block_count'] : 0;

        // Escalate: on repeat offenses, use exponential backoff from the base duration.
        $effectiveDuration = $duration * pow(2, $blockCount);
        if ($effectiveDuration > $this->maxBackoff) {
            $effectiveDuration = $this->maxBackoff;
        }

        $state['blocked_until'] = $now + $effectiveDuration;
        $state['block_count'] = $blockCount + 1;
        $state['last_failure'] = $now;

        error_log(
            '[SECURITY] Abuse detected: IP ' . $ip . ' blocked (bucket: '
            . $bucket . ', duration: ' . $effectiveDuration . 's)'
        );

        $this->store->save($ip, $bucket, $state);
    }

    /**
     * Opportunistic cleanup: run stale-file removal periodically.
     *
     * @return void
     */
    private function maybeCleanup()
    {
        self::$requestCounter++;
        if (self::$requestCounter % self::CLEANUP_INTERVAL === 0) {
            $this->store->cleanup($this->maxBackoff * 2);
        }
    }

    /**
     * Get the underlying store (for testing or admin reporting).
     *
     * @return FileThrottleStore
     */
    public function getStore()
    {
        return $this->store;
    }
}
