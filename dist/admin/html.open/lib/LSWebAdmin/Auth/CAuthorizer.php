<?php

namespace LSWebAdmin\Auth;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Runtime\SInfo;
use LSWebAdmin\UI\UIBase;

class CAuthorizer
{

    private $_id;
    private $_id_field;
    private $_pass;
    private $_pass_field;
    private static $_instance = null;

    /** @var IpThrottle */
    private $_throttle;

    /** @var LoginHistory */
    private $_loginHistory;

    // prevent an object from being constructed
    private function __construct()
    {
        $label = strtoupper(substr(md5(SERVER_ROOT), 0, 16));
        $this->_id_field = 'LSID' . $label;
        $this->_pass_field = 'LSPA' . $label;

        $this->_throttle = new IpThrottle();
        $this->_loginHistory = new LoginHistory();

        session_name('LSUI' . $label); // to prevent conflicts with other app sessions
        $this->setSessionCookieParams();
        session_start();

        if (!array_key_exists('changed', $_SESSION)) {
            $_SESSION['changed'] = false;
        }

        if (!array_key_exists('valid', $_SESSION)) {
            $_SESSION['valid'] = false;
        }

        if (!array_key_exists('token', $_SESSION)) {
            $_SESSION['token'] = self::generateSessionToken();
        }

        // Restore admin-configured login-history retention from session so
        // it takes effect on every page load, not only when admin config is
        // loaded (confMgr / service requests).
        if (isset($_SESSION['loginHistoryRetention'])) {
            $this->_loginHistory->setRetentionDays($_SESSION['loginHistoryRetention']);
        }

        // Restore admin-configured ops audit file retention from session.
        if (isset($_SESSION['opsAuditRetainFiles'])) {
            \LSWebAdmin\Log\OpsAuditLogger::setMaxRotatedFiles($_SESSION['opsAuditRetainFiles']);
        }

        if ($_SESSION['valid']) {

            if (array_key_exists('lastaccess', $_SESSION)) {

                if (isset($_SESSION['timeout']) && $_SESSION['timeout'] > 0 && time() - $_SESSION['lastaccess'] > $_SESSION['timeout']) {
                    $this->clear();
                    if (strpos($_SERVER['SCRIPT_NAME'], '/view/') !== false) {
                        echo json_encode(['login_timeout' => 1]);
                    } else {
                        header("location:/login.php?timedout=1");
                    }
                    exit();
                }

                $this->loadSessionCredentials();
            }
            if (!defined('NO_UPDATE_ACCESS'))
                $this->updateAccessTime();
        }
    }

    public static function singleton()
    {

        if (!isset(self::$_instance)) {
            $c = __CLASS__;
            self::$_instance = new $c;
        }

        return self::$_instance;
    }

    public static function Authorize()
    {
        $auth = CAuthorizer::singleton();
        if (!$auth->IsValid()) {
            $auth->clear();
            if (strpos($_SERVER['SCRIPT_NAME'], '/view/') !== false) {
                echo json_encode(['login_timeout' => 1]);
            } else {
                header("location:/login.php");
            }
            exit();
        }

        if (!$auth->hasTrustedReferrer()) {
            $auth->denyUntrustedReferrer();
        }
    }

    /**
     * Refuse a single authenticated request that positively carries a
     * foreign or insecure referrer, without touching the session. Destroying
     * the session here would let any cross-site link click log the admin out.
     *
     * The response must be a page, not a redirect: browsers carry the
     * original (foreign) Referer through a 302, so redirecting to index.php
     * would loop straight back into this block.
     */
    private function denyUntrustedReferrer()
    {
        $referer = UIBase::GrabInput('server', 'HTTP_REFERER');
        \LSWebAdmin\Log\OpsAuditLogger::log(
            'security_block',
            'cross_origin_request',
            'Blocked authenticated request with untrusted referrer: ' . substr($referer, 0, 200),
            'system'
        );

        http_response_code(403);

        if (strpos($_SERVER['SCRIPT_NAME'], '/view/') !== false) {
            header('Content-Type: application/json');
            echo json_encode(['cross_origin_blocked' => 1]);
            exit();
        }

        $title = UIBase::Escape(DMsg::ALbl('l_crossorigin_blocked'));
        $note = UIBase::Escape(DMsg::ALbl('note_crossorigin_blocked'));
        $consoleName = UIBase::Escape(\LSWebAdmin\Product\Current\Product::GetInstance()->getWebAdminConsoleName());

        header('Content-Type: text/html; charset=utf-8');
        echo '<!DOCTYPE html><html lang="en"><head><meta charset="utf-8">'
            . '<meta name="viewport" content="width=device-width, initial-scale=1">'
            . '<title>' . $title . '</title>'
            . '<style>body{font:normal 15px/1.6 Arial,Helvetica,sans-serif;color:#333;background:#f5f6f8;'
            . 'display:flex;align-items:center;justify-content:center;min-height:100vh;margin:0}'
            . 'main{max-width:420px;background:#fff;border-radius:8px;padding:32px 36px;'
            . 'box-shadow:0 2px 12px rgba(0,0,0,.08);text-align:center}'
            . 'h1{font-size:20px;margin:0 0 12px}p{margin:0 0 20px}'
            . 'a{display:inline-block;color:#fff;background:#1568f5;text-decoration:none;'
            . 'border-radius:6px;padding:10px 22px}</style></head>'
            . '<body><main><h1>' . $title . '</h1><p>' . $note . '</p>'
            . '<a href="/index.php">' . $consoleName . '</a></main></body></html>';
        exit();
    }

    private function isHttpsRequest()
    {
        return !empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off';
    }

    private function getRequestHost()
    {
        $host = UIBase::GrabInput('server', 'HTTP_HOST');
        if ($host === '') {
            $host = UIBase::GrabInput('server', 'SERVER_NAME');
        }

        if ($host !== '' && ($pos = strpos($host, ':')) !== false) {
            $host = substr($host, 0, $pos);
        }

        return strtolower($host);
    }

    private function getRequestPort()
    {
        $port = UIBase::GrabInput('server', 'SERVER_PORT', 'int');
        return ($port > 0) ? $port : null;
    }

    private function getCookieDomain()
    {
        return $this->getRequestHost();
    }

    private function getCookiePathWithSameSite($path)
    {
        return $path . '; SameSite=Lax';
    }

    private function setSessionCookieParams()
    {
        if (defined('PHP_VERSION_ID') && PHP_VERSION_ID >= 70300) {
            session_set_cookie_params([
                'lifetime' => 0,
                'path' => '/',
                'domain' => $this->getCookieDomain(),
                'secure' => $this->isHttpsRequest(),
                'httponly' => true,
                'samesite' => 'Lax',
            ]);
            return;
        }

        session_set_cookie_params(
            0,
            $this->getCookiePathWithSameSite('/'),
            $this->getCookieDomain(),
            $this->isHttpsRequest(),
            true
        );
    }

    private function setExpiredCookie($name)
    {
        $outdated = time() - 3600 * 24 * 30;
        if (defined('PHP_VERSION_ID') && PHP_VERSION_ID >= 70300) {
            setcookie($name, '', [
                'expires' => $outdated,
                'path' => '/',
                'domain' => $this->getCookieDomain(),
                'secure' => $this->isHttpsRequest(),
                'httponly' => true,
                'samesite' => 'Lax',
            ]);
            return;
        }

        setcookie(
            $name,
            '',
            $outdated,
            $this->getCookiePathWithSameSite('/'),
            $this->getCookieDomain(),
            $this->isHttpsRequest(),
            true
        );
    }

    private function hasTrustedReferrer()
    {
        $referer = UIBase::GrabInput('server', 'HTTP_REFERER');
        if ($referer === '') {
            // A missing Referer is acceptable: bookmarks, direct URL entry,
            // and privacy browsers/extensions that strip the header would
            // otherwise be logged out on every navigation. Cross-site
            // forgery is covered by the SameSite=Lax session cookie and the
            // per-request CSRF token; this check only rejects requests that
            // positively carry a foreign or insecure referrer.
            return true;
        }

        $parts = parse_url($referer);
        if ($parts === false || empty($parts['host'])) {
            return false;
        }

        if (!hash_equals($this->getRequestHost(), strtolower($parts['host']))) {
            return false;
        }

        if (isset($parts['scheme'])) {
            if (strtolower($parts['scheme']) !== 'https') {
                return false;
            }
        }

        return true;
    }

    public function IsValid()
    {
		if ($_SESSION['valid'] !== true) {
			return false;
		}

        return ($this->loadSessionCredentials() !== false);
    }

    public static function getUserId()
    {
        $auth = CAuthorizer::singleton();
        $credentials = $auth->loadSessionCredentials();
        return ($credentials === false) ? '' : $credentials[0];
    }

    public static function GetToken()
    {
        if (session_status() !== PHP_SESSION_ACTIVE) {
            self::singleton();
        }

        if (!isset($_SESSION['token']) || !is_string($_SESSION['token']) || $_SESSION['token'] === '') {
            $_SESSION['token'] = self::generateSessionToken();
        }

        return $_SESSION['token'];
    }

    public static function VerifyCsrfToken($origin = 'post')
    {
        $token = self::GetToken();
        $provided = UIBase::GrabGoodInput($origin, 'tk');
        if ($provided === '') {
            $provided = UIBase::GrabGoodInput('server', 'HTTP_X_LSWEBADMIN_TOKEN');
        }

        return is_string($provided)
            && $provided !== ''
            && is_string($token)
            && hash_equals($token, $provided);
    }

    public static function RequireCsrfToken($origin = 'post')
    {
        if (self::VerifyCsrfToken($origin)) {
            return;
        }

        http_response_code(403);
        header('Content-Type: text/plain; charset=UTF-8');
        echo 'Invalid request token.';
        exit;
    }

    public static function SetTimeout($timeout)
    {
        $_SESSION['timeout'] = (int) $timeout;
    }

    public static function HasSetTimeout()
    {
        return (isset($_SESSION['timeout']) && $_SESSION['timeout'] >= 60);
    }

    /**
     * Apply throttle configuration from admin settings.
     *
     * Called from ControllerBase after the admin config is loaded, so post-login
     * throttle checks (e.g. abuse bucket) use admin-configured values. The login
     * page itself uses the built-in defaults since admin config is not yet loaded
     * at that point.
     *
     * @param array $config Keys: 'enabled', 'maxFailures', 'blockWindow', 'maxBackoff'.
     */
    public static function SetThrottleConfig(array $config)
    {
        $auth = self::singleton();
        $auth->_throttle = new IpThrottle(new FileThrottleStore(), $config);
    }

    /**
     * Apply login-history retention from admin settings.
     *
     * Called from ControllerBase after the admin config is loaded so that
     * the configured retention period takes effect for subsequent reads.
     *
     * @param int $days Retention in days (1–365).
     */
    public static function SetLoginHistoryRetention($days)
    {
        $days = (int) $days;
        $auth = self::singleton();
        $auth->_loginHistory->setRetentionDays($days);
        $_SESSION['loginHistoryRetention'] = $days;
    }

    /**
     * Get the LoginHistory instance.
     *
     * Used by the login-history page to read the effective retention value
     * after admin config has been applied.
     *
     * @return LoginHistory
     */
    public static function GetLoginHistory()
    {
        return self::singleton()->_loginHistory;
    }

    public function GetCmdHeader()
    {
        $credentials = $this->loadSessionCredentials();
        if ($credentials === false) {
            return '';
        }

        // Defense in depth: refuse to build an auth header containing CR/LF or
        // ':' control characters that would let a poisoned credential inject
        // extra lines into the admin socket protocol.
        if (preg_match('/[\x00-\x1F\x7F]/', $credentials[0])
                || preg_match('/[\x00-\x1F\x7F]/', $credentials[1])
                || strpos($credentials[0], ':') !== false) {
            error_log('[WebAdmin Console] Refusing to build admin command header: stored credential contains control characters.');
            return '';
        }

        return 'auth:' . $credentials[0] . ':' . $credentials[1] . "\n";
    }

    public function Reauthenticate()
    {
        $credentials = $this->loadSessionCredentials();
        if ($credentials === false) {
            $this->clear();
            if (strpos($_SERVER['SCRIPT_NAME'], '/view/') !== false) {
                echo json_encode(['login_timeout' => 1]);
            } else {
                header("location:/login.php?timedout=1");
            }
            exit();
        }

        $uid = $credentials[0];
        $password = $credentials[1];
        $auth = $this->authUser($uid, $password);

        if (!$auth) {
            $this->clear();
            if (strpos($_SERVER['SCRIPT_NAME'], '/view/') !== false) {
                echo json_encode(['login_timeout' => 1]);
            } else {
                header("location:/login.php?timedout=1");
            }
            exit();
        }

    }

    public function ShowLogin(&$msg, &$msgType)
    {
        $timedout = UIBase::GrabInput('get', 'timedout', 'int');
        $logoff = UIBase::GrabInput('get', 'logoff', 'int');
        $relogin = UIBase::GrabInput('get', 'relogin', 'int');
        $failed = UIBase::GrabInput('get', 'failed', 'int');
        $msg = '';
        $msgType = 'info';

        // Require a valid CSRF token to honor a logoff=1 request so the
        // logout action cannot be triggered cross-origin via a stray GET
        // (e.g. an attacker-controlled <img src="/login.php?logoff=1">).
        if ($logoff == 1 && !self::VerifyCsrfToken('get')) {
            $logoff = 0;
        }

        if ($timedout == 1 || $logoff == 1 || $failed == 1) {
            $this->clear();

            if ($timedout == 1) {
                $msg = DMsg::Err('err_sessiontimeout');
                $msgType = 'warning';
            } elseif ($logoff == 1) {
                $msg = DMsg::Err('err_loggedoff');
                $msgType = 'info';
            } elseif ($failed == 1) {
                $msg = DMsg::Err('err_blockfailed');
                $msgType = 'error';
            }
            return true;
        }

        if ($relogin == 1) {
            $msg = DMsg::Err('err_reloginrequired');
            $msgType = 'info';
            return true;
        }

        if ($this->IsValid()) {
            return false;
        }

        $clientIp = UIBase::GrabInput('server', 'REMOTE_ADDR');

        // Check if IP is blocked before allowing a login attempt.
        if ($clientIp !== '' && $this->_throttle->isBlocked($clientIp, 'login')) {
            $status = $this->_throttle->getStatus($clientIp, 'login');
            $retryMinutes = (int) ceil($status['retry_after'] / 60);
            $msg = DMsg::UIStr('err_throttled', ['%%minutes%%' => $retryMinutes]);
            $msgType = 'error';

            // Record the blocked attempt in login history. Cap and sanitize
            // the attempted username to prevent log bloat / injection from a
            // user-controlled POST field.
            $attemptUser = isset($_POST['userid']) ? UIBase::GrabInput('POST', 'userid') : '';
            if ($attemptUser !== '') {
                $attemptUser = self::sanitizeUserForHistory($attemptUser);
                $this->_loginHistory->recordFailure($clientIp, $attemptUser, 'throttled');
            }
            return true;
        }

        $userid = null;
        $pass = null;

        if (isset($_POST['userid'])) {
            $userid = UIBase::GrabInput('POST', 'userid');
            $pass = UIBase::GrabInput('POST', 'pass');
            $msg = DMsg::Err('err_login');
            $msgType = 'error';
        }

        if ($userid != null && ($this->authenticate($userid, $pass) === true)) {
            return false;
        }

        return true;
    }

    private function updateAccessTime($isAuthenticated = false)
    {
        $_SESSION['lastaccess'] = time();
        if ($isAuthenticated) {
            $_SESSION['valid'] = true;
        }
    }

    public function clear()
    {
        $this->_id = null;
        $this->_pass = null;
        SessionCredentialStore::clear();

        if (session_status() === PHP_SESSION_ACTIVE) {
            session_unset();
            session_destroy();
        }

        $this->clearLegacyAuthCookies();

        $this->setExpiredCookie(session_name());
    }

    public function InvalidateForReLogin()
    {
        $this->_id = null;
        $this->_pass = null;
        SessionCredentialStore::clear();

        if (session_status() === PHP_SESSION_ACTIVE) {
            $_SESSION['valid'] = false;
            unset($_SESSION['lastaccess']);
            $_SESSION['token'] = self::generateSessionToken();
            @session_regenerate_id(true);
        }

        $this->clearLegacyAuthCookies();
    }

    private function authUser($authUser, $authPass)
    {
        $auth = false;
        $authUser1 = escapeshellcmd($authUser);

        // Reject control characters in either field — they could inject
        // additional protocol lines into the admin socket auth header.
        if (preg_match('/[\x00-\x1F\x7F]/', (string) $authUser)
                || preg_match('/[\x00-\x1F\x7F]/', (string) $authPass)) {
            return false;
        }

        if (($authUser === $authUser1)
                && !preg_match('/[:\/]/', $authUser)
                && strlen($authUser) && strlen($authPass)) {
            $filename = SERVER_ROOT . 'admin/conf/htpasswd';
            $fd = @fopen($filename, 'r');
            if (!$fd) {
                return false;
            }

            while (($line = fgets($fd)) !== false) {
                $line = trim($line);
                if ($line === '') {
                    continue;
                }

                $parts = explode(':', $line, 2);
                if (count($parts) < 2) {
                    continue;
                }

                $user = $parts[0];
                $hash = $parts[1];
                if ($user == $authUser) {
                    if (password_verify($authPass, $hash)) {
                        $auth = true;
                    }
                    break;
                }
            }

            fclose($fd);
        }
        return $auth;
    }

    private function authenticate($authUser, $authPass)
    {
        $clientIp = UIBase::GrabInput('server', 'REMOTE_ADDR');
        $auth = $this->authUser($authUser, $authPass);

        if ($auth) {
            session_regenerate_id(true);
            // Rotate the CSRF token alongside the session ID so a token a
            // pre-auth visitor may have seen cannot be replayed after login.
            $_SESSION['token'] = self::generateSessionToken();

            if (!SessionCredentialStore::store($authUser, $authPass)) {
                error_log('[WebAdmin Console] Failed to store encrypted session credentials.' . PHP_EOL);
                $this->clear();
                return false;
            }

            $this->_id = $authUser;
            $this->_pass = $authPass;
            $this->updateAccessTime(true);
            $this->clearLegacyAuthCookies();

            // Clear throttle record on successful login.
            if ($clientIp !== '') {
                $this->_throttle->clearRecord($clientIp, 'login');
            }

            // Record successful login in history.
            $this->_loginHistory->recordSuccess($clientIp, $authUser);
        } else {
            // Record failure in throttle and login history.
            if ($clientIp !== '') {
                $this->_throttle->recordFailure($clientIp, 'login');
            }
            $sanitizedUser = self::sanitizeUserForHistory($authUser);
            $this->_loginHistory->recordFailure($clientIp, $sanitizedUser, 'invalid_credentials');

            $this->emailFailedLogin($sanitizedUser);
        }

        return $auth;
    }

    private function emailFailedLogin($authUser)
    {
        $ip = UIBase::GrabInput('server', 'REMOTE_ADDR');
        $url = UIBase::GrabInput('server', 'SCRIPT_URI');
        if ($url === '') {
            $url = UIBase::GrabInput('server', 'REQUEST_URI');
        }

        // Strip CR/LF before log writes to prevent log forgery via
        // attacker-controlled URI / IP / username fields.
        $logUser = preg_replace('/[\r\n]+/', ' ', (string) $authUser);
        $logIp = preg_replace('/[\r\n]+/', ' ', (string) $ip);
        $logUrl = preg_replace('/[\r\n]+/', ' ', (string) $url);

        error_log("[WebAdmin Console] Failed Login Attempt - username:$logUser ip:$logIp url:$logUrl" . PHP_EOL);

        $emails = Service::ServiceData(SInfo::DATA_ADMIN_EMAIL);
        if ($emails != null) {

            $repl = [
                '%%host_ip%%'  => UIBase::GrabInput('server', 'SERVER_ADDR'),
                '%%date%%'     => date('d-M-Y H:i:s \U\T\C'),
                '%%authUser%%' => $authUser,
                '%%ip%%'       => $ip,
                '%%url%%'      => $url];

            $subject = DMsg::UIStr('mail_failedlogin');
            $contents = DMsg::UIStr('mail_failedlogin_c', $repl);
            mail($emails, $subject, $contents);
        }
    }

    private function loadSessionCredentials()
    {
        if ($this->_id !== null && $this->_pass !== null) {
            return [$this->_id, $this->_pass];
        }

        $credentials = SessionCredentialStore::load();
        if (!is_array($credentials) || count($credentials) != 2) {
            return false;
        }

        $this->_id = $credentials[0];
        $this->_pass = $credentials[1];
        return $credentials;
    }

    private function clearLegacyAuthCookies()
    {
        $this->setExpiredCookie($this->_id_field);
        $this->setExpiredCookie($this->_pass_field);
    }

    /**
     * Normalize a user-supplied username for safe inclusion in audit logs,
     * email notifications, and login_history.json.
     *
     * Caps length and strips control characters so an attacker cannot bloat
     * the history file or inject log lines via the POST 'userid' field.
     */
    private static function sanitizeUserForHistory($user)
    {
        $user = (string) $user;
        // Strip control characters (including CR/LF) that could forge log lines.
        $user = preg_replace('/[\x00-\x1F\x7F]/', '', $user);
        if ($user === null) {
            return '';
        }
        if (function_exists('mb_substr')) {
            $user = mb_substr($user, 0, 64, 'UTF-8');
        } elseif (strlen($user) > 64) {
            $user = substr($user, 0, 64);
        }
        return $user;
    }

    private static function generateSessionToken()
    {
        // 1) PHP 7.0+: random_bytes() throws on CSPRNG failure.
        if (function_exists('random_bytes')) {
            try {
                return bin2hex(random_bytes(32));
            } catch (\Exception $e) {
                // fall through to the next source
            }
        }

        // 2) openssl_random_pseudo_bytes: on Linux this draws from
        //    /dev/urandom which IS a CSPRNG. The $crypto_strong out
        //    parameter is a hint that not all libssl builds set
        //    reliably (some PHP 5.6 + older libssl combinations
        //    leave it false even though the bytes are genuinely
        //    strong). Trust the call as long as it returns enough
        //    bytes; the actual entropy comes from the kernel.
        if (function_exists('openssl_random_pseudo_bytes')) {
            $bytes = openssl_random_pseudo_bytes(32);
            if (is_string($bytes) && strlen($bytes) >= 32) {
                return bin2hex(substr($bytes, 0, 32));
            }
        }

        // 3) Last fallback: read /dev/urandom directly. PHP 5.6
        //    has no native CSPRNG outside the two above; this is
        //    still a real CSPRNG on Linux/BSD, just without the
        //    convenience wrappers.
        if (is_readable('/dev/urandom')) {
            $fp = @fopen('/dev/urandom', 'rb');
            if ($fp !== false) {
                $bytes = @fread($fp, 32);
                @fclose($fp);
                if (is_string($bytes) && strlen($bytes) >= 32) {
                    return bin2hex(substr($bytes, 0, 32));
                }
            }
        }

        // No CSPRNG available at all — refuse to issue a guessable
        // token rather than weakening CSRF defense to a hashed
        // mt_rand value. trigger_error stops the request.
        error_log('[WebAdmin Console] No CSPRNG available (random_bytes / openssl_random_pseudo_bytes / /dev/urandom all failed); cannot issue session token.');
        trigger_error('lswebadmin: no CSPRNG available for session token generation', E_USER_ERROR);
        return '';
    }

}
