<?php

namespace LSWebAdmin\I18n;

use LSWebAdmin\UI\UIBase;

class DMsg
{
    const DEFAULT_LANG = 'en-US';
    const _COOKIE_LANG_ = 'litespeed_admin_lang';

    private static $_supported = [];
    private static $_supportedInitialized = false;
    private static $_curlang = '';
    private static $_curtips = '';

    private static function initSupported()
    {
        if (self::$_supportedInitialized) {
            return;
        }

        $supported = [];
        $codes = [];

        foreach (self::getLangSearchDirs() as $langDir) {
            foreach (glob($langDir . '*_msg.php') ?: [] as $msgFile) {
                if (preg_match('/^([A-Za-z]{2}(?:-[A-Za-z]{2})?)_msg\.php$/', basename($msgFile), $matches)) {
                    $codes[self::normalizeLocaleCode($matches[1])] = true;
                }
            }
        }

        if (empty($codes)) {
            $codes[self::DEFAULT_LANG] = true;
        }

        foreach (self::sortLanguageCodes(array_keys($codes)) as $fileCode) {
            $supported[$fileCode] = [self::getLanguageLabel($fileCode), $fileCode];
        }

        if (!isset($supported[self::DEFAULT_LANG])) {
            $supported = [self::DEFAULT_LANG => [self::getLanguageLabel(self::DEFAULT_LANG), self::DEFAULT_LANG]] + $supported;
        }

        self::$_supported = $supported;
        self::$_supportedInitialized = true;
    }

    private static function getLangDir()
    {
        return dirname(dirname(dirname(__DIR__))) . '/res/lang/';
    }

    private static function getPackSharedLangDir()
    {
        return dirname(dirname(dirname(__DIR__))) . '/pack/shared/res/lang/';
    }

    private static function getPackVariantLangDir()
    {
        $product = defined('PRODUCT') ? PRODUCT : 'ows';
        return dirname(dirname(dirname(__DIR__))) . '/pack/variants/' . $product . '/res/lang/';
    }

    private static function getLangSearchDirs()
    {
        $dirs = [
            self::getLangDir(),
            self::getPackSharedLangDir(),
            self::getPackVariantLangDir(),
        ];

        $unique = [];
        foreach ($dirs as $dir) {
            if (!in_array($dir, $unique, true) && is_dir($dir)) {
                $unique[] = $dir;
            }
        }

        return $unique;
    }

    private static function normalizeLocaleCode($lang)
    {
        if (!is_string($lang) || $lang === '') {
            return '';
        }

        $lang = str_replace('_', '-', trim($lang));
        $parts = explode('-', $lang, 2);
        $primary = strtolower($parts[0]);
        if (!isset($parts[1]) || $parts[1] === '') {
            return $primary;
        }

        return $primary . '-' . strtoupper($parts[1]);
    }

    private static function sortLanguageCodes($codes)
    {
        $priority = [self::DEFAULT_LANG, 'es-ES', 'de-DE', 'fr-FR', 'zh-CN', 'ja-JP'];
        $ordered = [];

        foreach ($priority as $code) {
            if (in_array($code, $codes, true)) {
                $ordered[] = $code;
            }
        }

        $remaining = array_values(array_diff($codes, $ordered));
        sort($remaining, SORT_STRING);

        return array_merge($ordered, $remaining);
    }

    private static function getLanguageLabel($fileCode)
    {
        switch ($fileCode) {
            case 'zh-CN':
                return '中文';
            case 'ja-JP':
                return '日本語';
            case 'es-ES':
                return 'Español';
            case 'de-DE':
                return 'Deutsch';
            case 'fr-FR':
                return 'Français';
            case 'en-US':
                return 'English';
            default:
                return $fileCode;
        }
    }

    private static function findLangFile($fileCode, $suffix)
    {
        foreach (self::getLangSearchDirs() as $langDir) {
            $file = $langDir . $fileCode . $suffix;
            if (file_exists($file)) {
                return $file;
            }
        }

        return null;
    }

    private static function normalizeLang($lang)
    {
        self::initSupported();

        if (!is_string($lang) || trim($lang) === '') {
            return null;
        }

        $lang = trim($lang);
        $legacyAliases = [
            'english' => 'en-US',
            'chinese' => 'zh-CN',
            'japanes' => 'ja-JP',
        ];

        $lookup = strtolower($lang);
        if (isset($legacyAliases[$lookup])) {
            $lang = $legacyAliases[$lookup];
        }

        $lang = self::normalizeLocaleCode($lang);

        foreach (array_keys(self::$_supported) as $supported) {
            if (strcasecmp($supported, $lang) === 0) {
                return $supported;
            }
        }

        return null;
    }

    private static function parseAcceptedLanguages($header)
    {
        if (!is_string($header) || trim($header) === '') {
            return [];
        }

        $weighted = [];
        foreach (explode(',', $header) as $index => $entry) {
            $parts = explode(';', trim($entry));
            $lang = self::normalizeLocaleCode($parts[0]);
            if ($lang === '') {
                continue;
            }

            $quality = 1.0;
            if (isset($parts[1]) && preg_match('/q=([0-9.]+)/i', $parts[1], $matches)) {
                $quality = (float) $matches[1];
            }

            $weighted[] = [
                'lang' => $lang,
                'quality' => $quality,
                'index' => $index,
            ];
        }

        usort($weighted, function ($left, $right) {
            if ($left['quality'] === $right['quality']) {
                return $left['index'] - $right['index'];
            }
            return ($left['quality'] > $right['quality']) ? -1 : 1;
        });

        return array_column($weighted, 'lang');
    }

    private static function detectBrowserLang()
    {
        self::initSupported();

        $requested = self::parseAcceptedLanguages(UIBase::GrabInput('server', 'HTTP_ACCEPT_LANGUAGE'));
        $supported = array_keys(self::$_supported);

        foreach ($requested as $lang) {
            foreach ($supported as $candidate) {
                if (strcasecmp($candidate, $lang) === 0) {
                    return $candidate;
                }
            }

            $primary = strtolower(strtok($lang, '-'));
            foreach ($supported as $candidate) {
                if (strtolower(strtok($candidate, '-')) === $primary) {
                    return $candidate;
                }
            }
        }

        return isset(self::$_supported[self::DEFAULT_LANG]) ? self::DEFAULT_LANG : array_key_first(self::$_supported);
    }

    private static function getCookieDomain()
    {
        $domain = UIBase::GrabInput('server', 'HTTP_HOST');
        if ($domain === '') {
            $domain = UIBase::GrabInput('server', 'SERVER_NAME');
        }

        if ($domain !== '' && ($pos = strpos($domain, ':')) !== false) {
            $domain = substr($domain, 0, $pos);
        }

        return $domain;
    }

    private static function init()
    {
        self::initSupported();
        $lang = isset(self::$_supported[self::DEFAULT_LANG]) ? self::DEFAULT_LANG : array_key_first(self::$_supported);

        if (isset($_SESSION[self::_COOKIE_LANG_]) && ($sessionLang = self::normalizeLang($_SESSION[self::_COOKIE_LANG_])) !== null) {
            $lang = $sessionLang;
        } else {
            $cookieLang = self::normalizeLang(UIBase::GrabGoodInput('cookie', self::_COOKIE_LANG_));
            if ($cookieLang !== null) {
                $lang = $cookieLang;
            } else {
                $browserLang = self::detectBrowserLang();
                if ($browserLang !== null) {
                    $lang = $browserLang;
                }
            }
            self::SetLang($lang);
        }

        self::$_curlang = $lang;

        $msgfile = self::findLangFile(self::DEFAULT_LANG, '_msg.php');
        if ($msgfile !== null) {
            // maybe called from command line for converter tool
            include $msgfile;

            if ($lang != self::DEFAULT_LANG) {
                $localizedMsgFile = self::findLangFile($lang, '_msg.php');
                if ($localizedMsgFile !== null) {
                    include $localizedMsgFile;
                }
            }
        }
    }

    private static function init_tips()
    {
        if (self::$_curlang == '') {
            self::init();
        }

        if (self::$_curlang != self::DEFAULT_LANG) {
            $defaultTipsFile = self::findLangFile(self::DEFAULT_LANG, '_tips.php');
            if ($defaultTipsFile !== null) {
                include $defaultTipsFile;
            }
        }

        $tipsFile = self::findLangFile(self::$_curlang, '_tips.php');
        if ($tipsFile === null) {
            $tipsFile = self::findLangFile(self::DEFAULT_LANG, '_tips.php');
        }

        if ($tipsFile !== null) {
            self::$_curtips = basename($tipsFile);
            include $tipsFile;
        }
    }

    public static function GetSupportedLang(&$cur_lang)
    {
        self::initSupported();

        if (self::$_curlang == '') {
            self::init();
        }

        $cur_lang = self::$_curlang;
        return self::$_supported;
    }

    public static function SetLang($lang)
    {
        self::initSupported();

        $normalized = self::normalizeLang($lang);
        if (PHP_SAPI !== 'cli' && $normalized !== null) {
            $_SESSION[self::_COOKIE_LANG_] = $normalized;
            self::$_curlang = '';
            self::$_curtips = '';
            $domain = self::getCookieDomain();
            $secure = !empty($_SERVER['HTTPS']) && $_SERVER['HTTPS'] !== 'off';
            $httponly = true;

            setcookie(self::_COOKIE_LANG_, $normalized, strtotime('+10 days'), '/',
                $domain, $secure, $httponly);
        }
    }

    public static function GetAttrTip($label)
    {
        if ($label == '') {
            return null;
        }

        global $_tipsdb;

        if (self::$_curtips == '') {
            self::init_tips();
        }

        if (isset($_tipsdb[$label])) {
            return $_tipsdb[$label];
        }

        return null;
    }

    public static function GetEditTips($labels)
    {
        global $_tipsdb;

        if (self::$_curtips == '') {
            self::init_tips();
        }

        $tips = [];
        foreach ($labels as $label) {
            $label = 'EDTP:' . $label;
            if (isset($_tipsdb[$label])) {
                $tips = array_merge($tips, $_tipsdb[$label]);
            }
        }
        if (empty($tips)) {
            return null;
        }

        return $tips;
    }

    public static function UIStr($tag, $repl = '')
    {
        if ($tag == '') {
            return null;
        }

        global $_gmsg;
        if (self::$_curlang == '') {
            self::init();
        }

        if (isset($_gmsg[$tag])) {
            if ($repl == '') {
                return $_gmsg[$tag];
            }
            if (is_array($repl)) {
                $search = array_keys($repl);
                $replace = array_values($repl);
                return str_replace($search, $replace, $_gmsg[$tag]);
            }

            return $_gmsg[$tag];
        }

        return 'Unknown';
    }

    public static function EchoUIStr($tag, $repl = '')
    {
        echo self::UIStr($tag, $repl);
    }

    public static function DocsUrl()
    {
        self::initSupported();

        if (self::$_curlang == '') {
            self::init();
        }

        $url = '/docs/';
        if (self::$_curlang != self::DEFAULT_LANG) {
            $url .= self::$_curlang . '/';
        }
        return $url;
    }

    public static function ALbl($tag)
    {
        if ($tag == '') {
            return null;
        }

        global $_gmsg;
        if (self::$_curlang == '') {
            self::init();
        }

        if (isset($_gmsg[$tag])) {
            return $_gmsg[$tag];
        }

        return 'Unknown';
    }

    public static function Err($tag)
    {
        if ($tag == '') {
            return null;
        }

        global $_gmsg;
        if (self::$_curlang == '') {
            self::init();
        }

        if (isset($_gmsg[$tag])) {
            return $_gmsg[$tag] . ' ';
        }

        return 'Unknown';
    }

    private static function echo_sort_keys($lang_array, $priority)
    {
        $keys = array_keys($lang_array);
        $key2 = [];
        foreach ($keys as $key) {
            $pos = strpos($key, '_') + 1;
            $key2[substr($key, 0, $pos)][] = substr($key, $pos);
        }

        foreach ($priority as $pri) {
            if (isset($key2[$pri])) {
                sort($key2[$pri]);
                foreach ($key2[$pri] as $subid) {
                    $id = $pri . $subid;
                    echo '$_gmsg[\'' . $id . '\'] = \'' . addslashes($lang_array[$id]) . "'; \n";
                }
                echo "\n\n";
                unset($key2[$pri]);
            }
        }

        if (count($key2) > 0) {
            echo "// *** Not in priority \n";
            print_r($key2);
        }
    }

    public static function Util_SortMsg($lang, $option)
    {
        self::initSupported();

        $lang = self::normalizeLang($lang);
        if ($lang === null) {
            echo "language $lang not supported! \n" .
            "Currently supported:" . print_r(array_keys(self::$_supported), true);
            return;
        }

        global $_gmsg;

        $msgFile = self::findLangFile($lang, '_msg.php');
        if ($msgFile === null) {
            echo "Cannot find language file for $lang\n";
            return;
        }
        include $msgFile;

        echo "<?php\n\n";

        if ($option == 'all') {
            echo '$_gmsg = ' . var_export($_gmsg, true) . ";\n";
            return;
        }

        //priority
        self::echo_sort_keys($_gmsg, $option);
    }

}
