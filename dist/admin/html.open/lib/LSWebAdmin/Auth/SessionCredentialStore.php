<?php

namespace LSWebAdmin\Auth;

use function LSWebAdmin\Legacy\Crypto\PMA_blowfish_decrypt;
use function LSWebAdmin\Legacy\Crypto\PMA_blowfish_encrypt;

require_once dirname(__DIR__) . '/Legacy/Crypto/Blowfish.php';

class SessionCredentialStore
{
    const SESSION_FIELD = '_auth_credentials';
    const KEY_FILE = 'admin/conf/.auth_session_key';
    const OPENSSL_CIPHER = 'AES-256-CBC';
    const OPENSSL_VERSION = 'A1';
    const BLOWFISH_VERSION = 'B1';

    private static $masterKey = null;

    public static function store($user, $password)
    {
        $payload = json_encode([
            'u' => (string) $user,
            'p' => (string) $password,
        ]);

        if (!is_string($payload) || $payload === '') {
            return false;
        }

        $encrypted = self::encrypt($payload);
        if ($encrypted === false) {
            return false;
        }

        $_SESSION[self::SESSION_FIELD] = $encrypted;
        return true;
    }

    public static function load()
    {
        if (!isset($_SESSION[self::SESSION_FIELD])) {
            return false;
        }

        $payload = self::decrypt($_SESSION[self::SESSION_FIELD]);
        if (!is_string($payload) || $payload === '') {
            return false;
        }

        $data = json_decode($payload, true);
        if (!is_array($data) || !isset($data['u']) || !isset($data['p'])) {
            return false;
        }

        if (!is_string($data['u']) || !is_string($data['p'])) {
            return false;
        }

        return [$data['u'], $data['p']];
    }

    public static function clear()
    {
        unset($_SESSION[self::SESSION_FIELD]);
    }

    private static function encrypt($plaintext)
    {
        $encKey = self::deriveKey('enc');
        $macKey = self::deriveKey('mac');

        if (self::canUseOpenSsl()) {
            $iv = self::randomBytes(16);
            $ciphertext = openssl_encrypt($plaintext, self::OPENSSL_CIPHER, $encKey, OPENSSL_RAW_DATA, $iv);
            if ($ciphertext === false) {
                return false;
            }

            $payload = self::OPENSSL_VERSION . $iv . $ciphertext;
        } else {
            $secret = bin2hex($encKey);
            $ciphertext = PMA_blowfish_encrypt($plaintext, $secret);
            if (!is_string($ciphertext) || $ciphertext === '') {
                return false;
            }

            $payload = self::BLOWFISH_VERSION . $ciphertext;
        }

        $mac = hash_hmac('sha256', $payload, $macKey, true);
        return base64_encode($payload . $mac);
    }

    private static function decrypt($encoded)
    {
        if (!is_string($encoded) || $encoded === '') {
            return false;
        }

        $data = base64_decode($encoded, true);
        if ($data === false || strlen($data) <= 34) {
            return false;
        }

        $payload = substr($data, 0, -32);
        $mac = substr($data, -32);
        $calc = hash_hmac('sha256', $payload, self::deriveKey('mac'), true);

        if (!hash_equals($calc, $mac)) {
            return false;
        }

        $version = substr($payload, 0, 2);
        if ($version === self::OPENSSL_VERSION) {
            if (strlen($payload) < 18 || !self::canUseOpenSsl()) {
                return false;
            }

            $iv = substr($payload, 2, 16);
            $ciphertext = substr($payload, 18);
            $plaintext = openssl_decrypt($ciphertext, self::OPENSSL_CIPHER, self::deriveKey('enc'), OPENSSL_RAW_DATA, $iv);
            return ($plaintext === false) ? false : $plaintext;
        }

        if ($version === self::BLOWFISH_VERSION) {
            $secret = bin2hex(self::deriveKey('enc'));
            return PMA_blowfish_decrypt(substr($payload, 2), $secret);
        }

        return false;
    }

    private static function canUseOpenSsl()
    {
        return function_exists('openssl_encrypt') && function_exists('openssl_decrypt');
    }

    private static function deriveKey($purpose)
    {
        return hash_hmac('sha256', $purpose, self::getMasterKey(), true);
    }

    private static function getMasterKey()
    {
        if (self::$masterKey !== null) {
            return self::$masterKey;
        }

        $key = self::loadPersistedKey();
        if ($key === false) {
            $key = self::createPersistedKey();
        }
        if ($key === false) {
            $key = hash('sha256', self::getFallbackKeyMaterial(), true);
        }

        self::$masterKey = $key;
        return self::$masterKey;
    }

    private static function loadPersistedKey()
    {
        $file = self::getKeyFilePath();
        if (!is_readable($file)) {
            return false;
        }

        $data = trim(@file_get_contents($file));
        if ($data === '') {
            return false;
        }

        $key = base64_decode($data, true);
        if ($key === false || strlen($key) < 32) {
            return false;
        }

        return substr($key, 0, 32);
    }

    private static function createPersistedKey()
    {
        $dir = dirname(self::getKeyFilePath());
        if (!is_dir($dir) || !is_writable($dir)) {
            return false;
        }

        $key = self::randomBytes(32);
        $saved = @file_put_contents(self::getKeyFilePath(), base64_encode($key) . "\n", LOCK_EX);
        if ($saved === false) {
            return false;
        }

        @chmod(self::getKeyFilePath(), 0600);
        return $key;
    }

    private static function getKeyFilePath()
    {
        return rtrim(SERVER_ROOT, '/') . '/' . self::KEY_FILE;
    }

    private static function getFallbackKeyMaterial()
    {
        $htpasswd = rtrim(SERVER_ROOT, '/') . '/admin/conf/htpasswd';
        $fingerprint = '';
        if (is_readable($htpasswd)) {
            $fingerprint = hash_file('sha256', $htpasswd);
        }

        return SERVER_ROOT . '|' . session_name() . '|' . php_uname() . '|' . $htpasswd . '|' . $fingerprint;
    }

    private static function randomBytes($length)
    {
        if (function_exists('random_bytes')) {
            return random_bytes($length);
        }

        if (function_exists('openssl_random_pseudo_bytes')) {
            $bytes = openssl_random_pseudo_bytes($length);
            if ($bytes !== false && strlen($bytes) >= $length) {
                return substr($bytes, 0, $length);
            }
        }

        $bytes = '';
        while (strlen($bytes) < $length) {
            $bytes .= hash('sha256', uniqid(mt_rand(), true) . microtime(true), true);
        }

        return substr($bytes, 0, $length);
    }
}
