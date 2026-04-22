<?php

namespace LSWebAdmin\Tool\BuildPhp;

use LSWebAdmin\I18n\DMsg;

class BuildTool
{
    public $options = null;
    public $ext_options = [];
    public $dlmethod;
    public $progress_file;
    public $log_file;
    public $extension_used;
    public $build_prepare_script = null;
    public $build_install_script = null;
    public $build_manual_run_script = null;

    public function __construct($input_options)
    {
        if ($input_options == null || !$input_options->IsValidated()) {
            return;
        }
        $this->options = $input_options;
    }

    public function init(&$error, &$optionsaved)
    {
        $optionsaved = $this->options->SaveOptions();
        $buildDir = BuildConfig::Get(BuildConfig::BUILD_DIR);

        $this->progress_file = $buildDir . '/buildphp_' . $this->options->GetBatchId() . '.progress';
        $this->log_file = $buildDir . '/buildphp_' . $this->options->GetBatchId() . '.log';
        $this->build_prepare_script = $buildDir . '/buildphp_' . $this->options->GetBatchId() . '.prep.sh';
        $this->build_install_script = $buildDir . '/buildphp_' . $this->options->GetBatchId() . '.install.sh';
        $this->build_manual_run_script = $buildDir . '/buildphp_manual_run.sh';

        if (file_exists($this->progress_file)) {
            $error = DMsg::Err('buildphp_errinprogress');
            return false;
        }

        if (!$this->detectDownloadMethod()) {
            $error = DMsg::Err('err_faildetectdlmethod');
            return false;
        }

        $this->initDownloadUrl();
        return true;
    }

    public function detectDownloadMethod()
    {
        $os = `uname`;
        $dlmethod = '';

        if (strpos($os, 'FreeBSD') !== false && $this->probeCommandAvailable('fetch')) {
            $dlmethod = 'fetch -o';
        }

        if ($dlmethod == '' && strpos($os, 'SunOS') !== false) {
            if ($this->probeCommandAvailable('curl')) {
                $dlmethod = 'curl -L -o';
            } elseif ($this->probeCommandAvailable('wget')) {
                $dlmethod = 'wget -nv -O';
            }
        }

        if ($dlmethod == '') {
            if ($this->probeCommandAvailable('curl')) {
                $dlmethod = 'curl -L -o';
            } elseif ($this->probeCommandAvailable('wget')) {
                $dlmethod = 'wget -nv -O';
            } else {
                return false;
            }
        }

        $this->dlmethod = $dlmethod;
        return true;
    }

    public function initDownloadUrl()
    {
        $ext = ['__extension_name__' => 'Suhosin'];
        $ver = 'suhosin-' . BuildConfig::GetVersion(BuildConfig::SUHOSIN_VERSION);
        $ext['__extension_dir__'] = $ver;
        $ext['__extension_src__'] = $ver . '.tar.gz';
        $ext['__extension_download_url__'] = 'http://download.suhosin.org/' . $ver . '.tar.gz';
        $ext['__extract_method__'] = 'tar -zxf';
        $ext['__extension_extra_config__'] = '';
        $this->ext_options['Suhosin'] = $ext;

        $ext = ['__extension_name__' => 'MemCache'];
        $ver = 'memcache-' . BuildConfig::GetVersion(BuildConfig::MEMCACHE_VERSION);
        $ext['__extension_dir__'] = $ver;
        $ext['__extension_src__'] = $ver . '.tgz';
        $ext['__extension_download_url__'] = 'http://pecl.php.net/get/' . $ver . '.tgz';
        $ext['__extract_method__'] = 'tar -zxf';
        $ext['__extension_extra_config__'] = '--enable-memcache';
        $this->ext_options['MemCache'] = $ext;

        $ext = ['__extension_name__' => 'MemCache'];
        $ver = 'memcache-' . BuildConfig::GetVersion(BuildConfig::MEMCACHE7_VERSION);
        $ext['__extension_dir__'] = $ver;
        $ext['__extension_src__'] = $ver . '.tgz';
        $ext['__extension_download_url__'] = 'http://pecl.php.net/get/' . $ver . '.tgz';
        $ext['__extract_method__'] = 'tar -zxf';
        $ext['__extension_extra_config__'] = '--enable-memcache';
        $this->ext_options['MemCache7'] = $ext;

        $ext = ['__extension_name__' => 'MemCache'];
        $ver = 'memcache-' . BuildConfig::GetVersion(BuildConfig::MEMCACHE8_VERSION);
        $ext['__extension_dir__'] = $ver;
        $ext['__extension_src__'] = $ver . '.tgz';
        $ext['__extension_download_url__'] = 'http://pecl.php.net/get/' . $ver . '.tgz';
        $ext['__extract_method__'] = 'tar -zxf';
        $ext['__extension_extra_config__'] = '--enable-memcache';
        $this->ext_options['MemCache8'] = $ext;

        $ext = ['__extension_name__' => 'MemCached'];
        $ver = 'memcached-' . BuildConfig::GetVersion(BuildConfig::MEMCACHED_VERSION);
        $ext['__extension_dir__'] = $ver;
        $ext['__extension_src__'] = $ver . '.tgz';
        $ext['__extension_download_url__'] = 'http://pecl.php.net/get/' . $ver . '.tgz';
        $ext['__extract_method__'] = 'tar -zxf';
        $ext['__extension_extra_config__'] = '--enable-memcached';
        $this->ext_options['MemCachd'] = $ext;

        $ext = ['__extension_name__' => 'MemCached'];
        $ver = 'memcached-' . BuildConfig::GetVersion(BuildConfig::MEMCACHED7_VERSION);
        $ext['__extension_dir__'] = $ver;
        $ext['__extension_src__'] = $ver . '.tgz';
        $ext['__extension_download_url__'] = 'http://pecl.php.net/get/' . $ver . '.tgz';
        $ext['__extract_method__'] = 'tar -zxf';
        $ext['__extension_extra_config__'] = '--enable-memcached';
        $this->ext_options['MemCachd7'] = $ext;
    }

    public static function getExtensionNotes($extensions)
    {
        $notes = [];
        if (strpos($extensions, 'Suhosin') !== false) {
            $notes[] = '
;				=================
;				Suhosin
;				=================
				extension=suhosin.so

';
        }
        if (strpos($extensions, 'MemCache') !== false) {
            $notes[] = '
;				=================
;				MemCache
;				=================
				extension=memcache.so

';
        }
        if (strpos($extensions, 'MemCachd') !== false) {
            $notes[] = '
;				=================
;				MemCached
;				=================
				extension=memcached.so

';
        }

        if (count($notes) == 0) {
            return '';
        }

        $note = '<li>' . DMsg::UIStr('buildphp_enableextnote') . '<br />';
        $note .= nl2br(implode("\n", $notes));
        $note .= '</li>';
        return $note;
    }

    public function GenerateScript(&$error, &$optionsaved)
    {
        if ($this->progress_file == null && !$this->init($error, $optionsaved)) {
            return false;
        }

        if (!function_exists('posix_getpwuid') || !function_exists('posix_geteuid') || !function_exists('posix_getgrgid')) {
            $error = 'Missing POSIX extension';
            return false;
        }

        $processUser = posix_getpwuid(posix_geteuid());
        if (!is_array($processUser)) {
            $error = 'Failed to detect process user';
            return false;
        }
        $gidinfo = posix_getgrgid($processUser['gid']);
        if (!is_array($gidinfo)) {
            $error = 'Failed to detect process group';
            return false;
        }

        $params = [];
        $params['__php_version__'] = $this->options->GetValue('PHPVersion');
        $params['__progress_f__'] = $this->progress_file;
        $params['__log_file__'] = $this->log_file;
        $params['__php_usr__'] = $processUser['name'];
        $params['__php_usrgroup__'] = $gidinfo['name'];
        $params['__extra_path_env__'] = $this->options->GetValue('ExtraPathEnv');
        $params['__php_build_dir__'] = BuildConfig::Get(BuildConfig::BUILD_DIR);
        $params['__dl_method__'] = $this->dlmethod;
        $params['__install_dir__'] = $this->options->GetValue('InstallPath');
        $params['__compiler_flags__'] = $this->options->GetValue('CompilerFlags');
        $params['__enable_mailheader__'] = $this->options->GetValue('AddOnMailHeader') ? 1 : 0;
        $params['__lsapi_version__'] = BuildConfig::GetVersion(BuildConfig::LSAPI_VERSION);
        $params['__php_conf_options__'] = $this->options->GetValue('ConfigParam');
        $params['__lsws_home__'] = SERVER_ROOT;
        $params['__install_script__'] = $this->build_install_script;

        $prepare_script = $this->renderTemplate('build_common.template', $params, $error);
        if ($prepare_script === false) {
            return false;
        }

        $install_script = $prepare_script;
        $template_script = $this->renderTemplate('build_prepare.template', $params, $error);
        if ($template_script === false) {
            return false;
        }
        $prepare_script .= $template_script;

        $template_script = $this->renderTemplate('build_install.template', $params, $error);
        if ($template_script === false) {
            return false;
        }
        $install_script .= $template_script;

        $prepare_ext_template = $this->loadTemplate('build_prepare_ext.template', $error);
        if ($prepare_ext_template === false) {
            return false;
        }

        $install_ext_template = $this->loadTemplate('build_install_ext.template', $error);
        if ($install_ext_template === false) {
            return false;
        }

        $extList = [];
        if ($this->options->GetValue('AddOnSuhosin')) {
            $extList[] = 'Suhosin';
        }
        if ($this->options->GetValue('AddOnMemCache')) {
            $extList[] = 'MemCache';
        }
        if ($this->options->GetValue('AddOnMemCache7')) {
            $extList[] = 'MemCache7';
        }
        if ($this->options->GetValue('AddOnMemCache8')) {
            $extList[] = 'MemCache8';
        }
        if ($this->options->GetValue('AddOnMemCachd')) {
            $extList[] = 'MemCachd';
        }
        if ($this->options->GetValue('AddOnMemCachd7')) {
            $extList[] = 'MemCachd7';
        }

        foreach ($extList as $extName) {
            $newparams = array_merge($params, $this->ext_options[$extName]);
            $prepare_script .= str_replace(array_keys($newparams), array_values($newparams), $prepare_ext_template);
            $install_script .= str_replace(array_keys($newparams), array_values($newparams), $install_ext_template);
        }
        $this->extension_used = implode('.', $extList);

        $prepare_script .= 'main_msg "**DONE**"' . "\n";
        $install_script .= 'main_msg "**DONE**"' . "\n";

        if (file_put_contents($this->build_prepare_script, $prepare_script) === false) {
            $error = DMsg::Err('buildphp_errcreatescript') . $this->build_prepare_script;
            return false;
        }
        if (chmod($this->build_prepare_script, 0700) == false) {
            $error = DMsg::Err('buildphp_errchmod') . $this->build_prepare_script;
            return false;
        }

        if (file_put_contents($this->build_install_script, $install_script) === false) {
            $error = DMsg::Err('buildphp_errcreatescript') . $this->build_install_script;
            return false;
        }
        if (chmod($this->build_install_script, 0700) == false) {
            $error = DMsg::Err('buildphp_errchmod') . $this->build_install_script;
            return false;
        }

        $template_script = $this->renderTemplate('build_manual_run.template', $params, $error);
        if ($template_script === false) {
            return false;
        }
        if (file_put_contents($this->build_manual_run_script, $template_script) === false) {
            $error = DMsg::Err('buildphp_errcreatescript') . $this->build_manual_run_script;
            return false;
        }
        if (chmod($this->build_manual_run_script, 0700) == false) {
            $error = DMsg::Err('buildphp_errchmod') . $this->build_manual_run_script;
            return false;
        }

        return true;
    }

    private function probeCommandAvailable($command)
    {
        $status = 1;
        $output = [];
        $probePath = $this->getProbePath();
        $cmd = 'PATH=' . escapeshellarg($probePath) . ' command -v ' . escapeshellarg($command) . ' >/dev/null 2>&1';
        exec($cmd, $output, $status);
        return $status === 0;
    }

    private function getProbePath()
    {
        $paths = [];
        $extraPathEnv = trim((string) $this->options->GetValue('ExtraPathEnv'), ':');
        if ($extraPathEnv !== '') {
            $paths[] = $extraPathEnv;
        }
        $paths[] = '/bin';
        $paths[] = '/usr/bin';
        $paths[] = '/usr/local/bin';
        return implode(':', $paths);
    }

    private function renderTemplate($templateFile, array $params, &$error)
    {
        $template = $this->loadTemplate($templateFile, $error);
        if ($template === false) {
            return false;
        }
        return str_replace(array_keys($params), array_values($params), $template);
    }

    private function loadTemplate($templateFile, &$error)
    {
        $templatePath = BuildConfig::GetTemplateDir() . '/' . $templateFile;
        $template = file_get_contents($templatePath);
        if ($template === false) {
            $error = DMsg::Err('err_failreadfile') . $templatePath;
            return false;
        }
        return $template;
    }
}
