<?php

namespace LSWebAdmin\Tool\BuildPhp;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\UI\UIBase;
use LSWebAdmin\Util\PathTool;

class BuildCheck
{
    private $cur_step;
    private $next_step = 0;
    public $pass_val = [];

    public function __construct()
    {
        $this->cur_step = UIBase::GrabInput('ANY', 'curstep');
        $this->validate_step();
    }

    public function GetNextStep()
    {
        return $this->next_step;
    }

    public function GetCurrentStep()
    {
        return $this->cur_step;
    }

    public function GetModuleSupport($php_version)
    {
        $modules = [];
        $v = substr($php_version, 0, 4);

        $modules['suhosin'] = in_array($v, ['5.6.']);
        $modules['mailheader'] = in_array($v, ['5.6.']);
        $modules['memcache'] = in_array($v, ['5.6.']);
        $modules['memcache7'] = in_array($v, ['7.0.', '7.1.', '7.2.', '7.3.', '7.4.']);
        $modules['memcache8'] = in_array($v, ['8.0.', '8.1.', '8.2.', '8.3.']);
        $modules['memcachd'] = in_array($v, ['5.6.']);
        $modules['memcachd7'] = in_array($v, ['7.0.', '7.1.', '7.2.', '7.3.', '7.4.', '8.0.', '8.1.']);

        return $modules;
    }

    private function validate_step()
    {
        switch ($this->cur_step) {
            case '':
                $this->next_step = 0;
                return;

            case '0':
                $this->validate_intro();
                return;

            case '1':
                $this->validate_step1();
                return;

            case '2':
                $this->validate_step2();
                return;

            case '3':
                $this->validate_step3();
                return;
        }
    }

    private function validate_intro()
    {
        $next = UIBase::GrabInput('ANY', 'next');
        $this->next_step = ($next == 1) ? 1 : 0;
        return true;
    }

    private function validate_step1()
    {
        $next = UIBase::GrabInput('ANY', 'next');
        if ($next == 0) {
            $this->next_step = 0;
            return true;
        }

        $selversion = UIBase::GrabInput('post', 'phpversel');
        if ($this->validate_php_version($selversion)) {
            $this->pass_val['php_version'] = $selversion;
        }

        $OS = `uname`;
        if (strpos($OS, 'FreeBSD') !== false) {
            if (!file_exists('/bin/bash') && !file_exists('/usr/bin/bash') && !file_exists('/usr/local/bin/bash')) {
                $this->pass_val['err'] = DMsg::Err('buildphp_errnobash');
            }
        }

        if (isset($this->pass_val['err'])) {
            $this->next_step = 1;
            return false;
        }

        $this->next_step = 2;
        return true;
    }

    private function validate_step2()
    {
        $next = UIBase::GrabInput('ANY', 'next');
        if ($next == 0) {
            $this->next_step = 1;
            return true;
        }

        $php_version = UIBase::GrabGoodInput('ANY', 'buildver');
        if (!$this->validate_php_version($php_version)) {
            $this->next_step = 0;
            return false;
        }
        $this->pass_val['php_version'] = $php_version;

        $options = new BuildOptions($php_version);
        $options->SetValue('ExtraPathEnv', UIBase::GrabGoodInput('ANY', 'path_env'));
        $options->SetValue('InstallPath', UIBase::GrabGoodInput('ANY', 'installPath'));

        $compilerFlags = UIBase::GrabGoodInput('ANY', 'compilerFlags');
        $configParams = UIBase::GrabGoodInput('ANY', 'configureParams');

        $options->SetValue('ConfigParam', $configParams);
        $options->SetValue('CompilerFlags', $compilerFlags);
        $options->SetValue('AddOnSuhosin', null != UIBase::GrabGoodInput('ANY', 'addonSuhosin'));
        $options->SetValue('AddOnMailHeader', null != UIBase::GrabGoodInput('ANY', 'addonMailHeader'));
        $options->SetValue('AddOnMemCache', null != UIBase::GrabGoodInput('ANY', 'addonMemCache'));
        $options->SetValue('AddOnMemCache7', null != UIBase::GrabGoodInput('ANY', 'addonMemCache7'));
        $options->SetValue('AddOnMemCache8', null != UIBase::GrabGoodInput('ANY', 'addonMemCache8'));
        $options->SetValue('AddOnMemCachd', null != UIBase::GrabGoodInput('ANY', 'addonMemCachd'));
        $options->SetValue('AddOnMemCachd7', null != UIBase::GrabGoodInput('ANY', 'addonMemCachd7'));

        $v1 = $this->validate_extra_path_env($options->GetValue('ExtraPathEnv'));
        $v2 = $this->validate_install_path($options->GetValue('InstallPath'));
        $v3 = $this->validate_complier_flags($compilerFlags);
        $v4 = $this->validate_config_params($configParams);

        if (!$v1 || !$v2 || !$v3 || !$v4) {
            $options->SetType('INPUT');
            $options->SetValidated(false);
            $this->pass_val['input_options'] = $options;
            $this->next_step = 2;
            return false;
        }

        if (version_compare($php_version, '7.4', '>=')) {
            if (strpos($configParams, '-litespeed') === false) {
                $configParams .= " '--enable-litespeed'";
            } elseif (strpos($configParams, '--with-litespeed') !== false) {
                $configParams = str_replace('--with-litespeed', '--enable-litespeed', $configParams);
            }
        } else {
            if (strpos($configParams, '-litespeed') === false) {
                $configParams .= " '--with-litespeed'";
            } elseif (strpos($configParams, '--enable-litespeed') !== false) {
                $configParams = str_replace('--enable-litespeed', '--with-litespeed', $configParams);
            }
        }

        $configParams = "'--prefix=" . $options->GetValue('InstallPath') . "' " . $configParams;
        $options->SetValue('ConfigParam', escapeshellcmd($configParams));
        $options->SetValue('CompilerFlags', escapeshellcmd($compilerFlags));
        $options->SetType('BUILD');
        $options->SetValidated(true);
        $this->pass_val['build_options'] = $options;
        $this->next_step = 3;
        return true;
    }

    private function validate_step3()
    {
        $php_version = UIBase::GrabGoodInput('ANY', 'buildver');
        if ($php_version == '') {
            echo 'missing php_version';
            return false;
        }

        $this->pass_val['php_version'] = $php_version;

        $next = UIBase::GrabInput('ANY', 'next');
        if ($next == 0) {
            $this->next_step = 2;
            return true;
        }

        if (!isset($_SESSION['progress_file'])) {
            echo 'missing progress file';
            return false;
        }
        $progress_file = $_SESSION['progress_file'];

        if (!isset($_SESSION['log_file'])) {
            echo 'missing log file';
            return false;
        }
        $log_file = $_SESSION['log_file'];
        if (!file_exists($log_file)) {
            echo 'logfile does not exist';
            return false;
        }

        $manual_script = UIBase::GrabGoodInput('ANY', 'manual_script');
        if ($manual_script == '' || !file_exists($manual_script)) {
            echo 'missing manual script';
            return false;
        }

        $this->pass_val['progress_file'] = $progress_file;
        $this->pass_val['log_file'] = $log_file;
        $this->pass_val['manual_script'] = $manual_script;
        $this->pass_val['extentions'] = UIBase::GrabGoodInput('ANY', 'extentions');
        $this->next_step = 4;
        return true;
    }

    private function validate_php_version($version)
    {
        $php_ver = BuildConfig::GetVersion(BuildConfig::PHP_VERSION);
        if (in_array($version, $php_ver)) {
            return true;
        }

        $this->pass_val['err'] = 'Illegal';
        return false;
    }

    private function validate_extra_path_env($extra_path_env)
    {
        if ($extra_path_env === '') {
            return true;
        }

        $envp = preg_split('/:/', $extra_path_env);
        foreach ($envp as $p) {
            if (!is_dir($p)) {
                $this->pass_val['err']['path_env'] = DMsg::Err('err_invalidpath') . $p;
                return false;
            }
        }

        return true;
    }

    private function validate_install_path($path)
    {
        $path = PathTool::clean($path);
        if ($path == '') {
            $this->pass_val['err']['installPath'] = DMsg::Err('err_valcannotempty');
            return false;
        }
        if ($path[0] != '/') {
            $this->pass_val['err']['installPath'] = DMsg::Err('err_requireabspath');
            return false;
        }
        if (preg_match('/([;&"|#$?`])/', $path)) {
            $this->pass_val['err']['installPath'] = DMsg::Err('err_illegalcharfound');
            return false;
        }

        if (!is_dir($path)) {
            if (is_file($path)) {
                $this->pass_val['err']['installPath'] = DMsg::Err('err_invalidpath');
                return false;
            }
            $testpath = dirname($path);
            if (!is_dir($testpath)) {
                $this->pass_val['err']['installPath'] = DMsg::Err('err_parentdirnotexist');
                return false;
            }
        } else {
            $testpath = $path;
        }

        if ($testpath == '.' || $testpath == '/' || PathTool::isDenied($testpath)) {
            $this->pass_val['err']['installPath'] = 'Illegal location';
            return false;
        }

        return true;
    }

    private function validate_complier_flags(&$cflags)
    {
        if ($cflags === '') {
            return true;
        }

        if (preg_match('/([;&"|#$?`])/', $cflags)) {
            if (strpos($cflags, '"') !== false) {
                $this->pass_val['err']['compilerFlags'] = DMsg::Err('buildphp_errquotes');
            } else {
                $this->pass_val['err']['compilerFlags'] = DMsg::Err('err_illegalcharfound');
            }
            return false;
        }

        $flag = [];
        $a = str_replace("\n", ' ', $cflags);
        $a = trim($a) . ' ';
        $FLAGS = 'CFLAGS|CPPFLAGS|CXXFLAGS|LDFLAGS';
        while (strlen($a) > 0) {
            $m = null;
            if (preg_match("/^($FLAGS)=[^'^\"^ ]+\s+/", $a, $matches)) {
                $m = $matches[0];
            } elseif (preg_match("/^($FLAGS)='[^'^\"]+'\s+/", $a, $matches)) {
                $m = $matches[0];
            }
            if ($m != null) {
                $a = substr($a, strlen($m));
                $flag[] = rtrim($m);
            } else {
                $pe = $a;
                $ipos = strpos($pe, ' ');
                if ($ipos !== false) {
                    $pe = substr($a, 0, $ipos);
                }
                $this->pass_val['err']['compilerFlags'] = DMsg::Err('err_invalidvalat') . $pe;
                return false;
            }
        }
        if (!empty($flag)) {
            $cflags = implode(' ', $flag);
        } else {
            $cflags = '';
        }
        return true;
    }

    private function validate_config_params(&$config_params)
    {
        if (preg_match('/([;&"|#$?`])/', $config_params)) {
            if (strpos($config_params, '"') !== false) {
                $this->pass_val['err']['configureParams'] = DMsg::Err('buildphp_errquotes');
            } else {
                $this->pass_val['err']['configureParams'] = DMsg::Err('err_illegalcharfound');
            }
            return false;
        }

        $params = [];
        $a = str_replace("\n", ' ', $config_params);
        $a = trim($a) . ' ';
        while (strlen($a) > 0) {
            $m = null;
            if (preg_match("/^'--[a-zA-Z_\\-0-9]+=[^=^'^;]+'\s+/", $a, $matches)) {
                $m = $matches[0];
            } elseif (preg_match("/^'--[a-zA-Z_\\-0-9]+'\s+/", $a, $matches)) {
                $m = $matches[0];
            } elseif (preg_match("/^--[a-zA-Z_\\-0-9]+=[^=^'^;^ ]+\s+/", $a, $matches)) {
                $m = $matches[0];
            } elseif (preg_match("/^--[a-zA-Z_\\-0-9]+\s+/", $a, $matches)) {
                $m = $matches[0];
            }
            if ($m != null) {
                $a = substr($a, strlen($m));
                if (!preg_match("/(--prefix=)|(--with-apxs)|(--enable-fastcgi)/", $m)) {
                    $m = trim(rtrim($m), "'");
                    $params[] = "'$m'";
                }
            } else {
                $pe = $a;
                $ipos = strpos($pe, ' ');
                if ($ipos !== false) {
                    $pe = substr($a, 0, $ipos);
                }
                $this->pass_val['err']['configureParams'] = DMsg::Err('err_invalidvalat') . $pe;
                return false;
            }
        }

        if (empty($params)) {
            $this->pass_val['err']['configureParams'] = DMsg::Err('err_valcannotempty');
            return false;
        }

        $config_params = implode(' ', $params);
        return true;
    }
}
