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
        session_set_cookie_params(0, '/', $this->getCookieDomain(), $this->isHttpsRequest(), true);
        session_start();

        if (!array_key_exists('changed', $_SESSION)) {
            $_SESSION['changed'] = false;
        }

        if (!array_key_exists('valid', $_SESSION)) {
            $_SESSION['valid'] = false;
        }

        if (!array_key_exists('token', $_SESSION)) {
            $_SESSION['token'] = microtime();
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

    private function hasTrustedReferrer()
    {
        $referer = UIBase::GrabInput('server', 'HTTP_REFERER');
        if ($referer === '') {
            return false;
        }

        $parts = parse_url($referer);
        if ($parts === false || empty($parts['host'])) {
            return false;
        }

        if (!hash_equals($this->getRequestHost(), strtolower($parts['host']))) {
            return false;
        }

        if (isset($parts['scheme'])) {
            $expectedScheme = $this->isHttpsRequest() ? 'https' : 'http';
            if (strtolower($parts['scheme']) !== $expectedScheme) {
                return false;
            }
        }

        $requestPort = $this->getRequestPort();
        if ($requestPort !== null && isset($parts['port']) && (int) $parts['port'] !== $requestPort) {
            return false;
        }

        return true;
    }

    public function IsValid()
    {
		if ($_SESSION['valid'] !== true) {
			return false;
		}

        return ($this->loadSessionCredentials() !== false) && $this->hasTrustedReferrer();
    }

    public static function getUserId()
    {
        $auth = CAuthorizer::singleton();
        $credentials = $auth->loadSessionCredentials();
        return ($credentials === false) ? '' : $credentials[0];
    }

    public static function GetToken()
    {
        return $_SESSION['token'];
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

            // Record the blocked attempt in login history.
            $attemptUser = isset($_POST['userid']) ? UIBase::GrabInput('POST', 'userid') : '';
            if ($attemptUser !== '') {
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

        $outdated = time() - 3600 * 24 * 30;
        $domain = $this->getCookieDomain();
        setcookie(session_name(), '', $outdated, '/', $domain);
    }

    public function InvalidateForReLogin()
    {
        $this->_id = null;
        $this->_pass = null;
        SessionCredentialStore::clear();

        if (session_status() === PHP_SESSION_ACTIVE) {
            $_SESSION['valid'] = false;
            unset($_SESSION['lastaccess']);
            $_SESSION['token'] = microtime();
            @session_regenerate_id(true);
        }

        $this->clearLegacyAuthCookies();
    }

    private function authUser($authUser, $authPass)
    {
        $auth = false;
        $authUser1 = escapeshellcmd($authUser);

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
            $this->_loginHistory->recordFailure($clientIp, escapeshellcmd($authUser), 'invalid_credentials');

            $this->emailFailedLogin(escapeshellcmd($authUser));
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

        error_log("[WebAdmin Console] Failed Login Attempt - username:$authUser ip:$ip url:$url" . PHP_EOL);

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
        $outdated = time() - 3600 * 24 * 30;
        $domain = $this->getCookieDomain();
        setcookie($this->_id_field, '', $outdated, '/', $domain);
        setcookie($this->_pass_field, '', $outdated, '/', $domain);
    }

}
