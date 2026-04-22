<?php

namespace LSWebAdmin\Tool\BuildPhp;

class BuildOptions
{
    private $base_ver;
    private $type;
    private $batch_id;
    private $validated = false;
    private $vals = [
        'OptionVersion' => 5,
        'PHPVersion' => '',
        'ExtraPathEnv' => '',
        'InstallPath' => '',
        'CompilerFlags' => '',
        'ConfigParam' => '',
        'AddOnSuhosin' => false,
        'AddOnMailHeader' => false,
        'AddOnMemCache' => false,
        'AddOnMemCache7' => false,
        'AddOnMemCache8' => false,
        'AddOnMemCachd' => false,
        'AddOnMemCachd7' => false,
    ];

    public function __construct($version = '')
    {
        if ($version !== '' && !$this->setVersion($version)) {
            return;
        }

        $this->type = 'NONE';
        $this->batch_id = '' . time() . '.' . rand(1, 9);
    }

    public function SetValue($name, $val)
    {
        $this->vals[$name] = $val;
    }

    public function GetValue($name)
    {
        return $this->vals[$name];
    }

    public function GetBatchId()
    {
        return $this->batch_id;
    }

    public function SetType($optionsType)
    {
        $this->type = $optionsType;
    }

    public function GetType()
    {
        return $this->type;
    }

    public function IsValidated()
    {
        return $this->validated;
    }

    public function SetValidated($isValid)
    {
        $this->validated = $isValid;
    }

    public function setVersion($version)
    {
        $supportedVersions = BuildConfig::GetVersion(BuildConfig::PHP_VERSION);
        if (!in_array($version, $supportedVersions, true)) {
            return false;
        }

        $dotPos = strpos($version, '.');
        if ($dotPos === false || $dotPos === 0) {
            return false;
        }

        $base = substr($version, 0, $dotPos);
        $this->base_ver = $base;
        $this->vals['PHPVersion'] = $version;
        return true;
    }

    public function setDefaultOptions()
    {
        $params = BuildConfig::Get(BuildConfig::DEFAULT_PARAMS);

        $this->vals['ExtraPathEnv'] = '';
        $this->vals['InstallPath'] = BuildConfig::Get(BuildConfig::DEFAULT_INSTALL_DIR) . $this->base_ver;
        $this->vals['CompilerFlags'] = '';
        $this->vals['ConfigParam'] = $params[$this->base_ver];
        $this->vals['AddOnSuhosin'] = false;
        $this->vals['AddOnMailHeader'] = false;
        $this->vals['AddOnMemCache'] = false;
        $this->vals['AddOnMemCache7'] = false;
        $this->vals['AddOnMemCache8'] = false;
        $this->vals['AddOnMemCachd'] = false;
        $this->vals['AddOnMemCachd7'] = false;

        $this->type = 'DEFAULT';
        $this->validated = true;
    }

    public function getSavedOptions()
    {
        $filename = BuildConfig::Get(BuildConfig::LAST_CONF) . $this->base_ver . '.options2';
        if (!file_exists($filename)) {
            return null;
        }

        $str = file_get_contents($filename);
        if ($str === false || $str === '') {
            return null;
        }

        $vals = unserialize($str);
        if (!is_array($vals) || !isset($vals['PHPVersion'])) {
            return null;
        }

        $saved_options = new BuildOptions($vals['PHPVersion']);
        $saved_options->type = 'IMPORT';
        $saved_options->vals = $vals;
        return $saved_options;
    }

    public function SaveOptions()
    {
        if (!$this->validated) {
            return false;
        }

        $saved_val = $this->vals;
        $saved_val['ConfigParam'] = trim(
            preg_replace("/ ?'--(prefix=)[^ ]*' */", ' ', $saved_val['ConfigParam'])
        );

        $serialized_str = serialize($saved_val);
        $filename = BuildConfig::Get(BuildConfig::LAST_CONF) . $this->base_ver . '.options2';
        return file_put_contents($filename, $serialized_str);
    }

    public function gen_loadconf_onclick($method)
    {
        if ($this->GetType() != $method) {
            return 'disabled';
        }

        $params = $this->escapeJsValue($this->vals['ConfigParam']);
        $flags = $this->escapeJsValue($this->vals['CompilerFlags']);
        $extraPathEnv = $this->escapeJsValue($this->vals['ExtraPathEnv']);
        $installPath = $this->escapeJsValue($this->vals['InstallPath']);
        $addon_suhosin = $this->vals['AddOnSuhosin'] ? 'true' : 'false';
        $addon_mailHeader = $this->vals['AddOnMailHeader'] ? 'true' : 'false';
        $addon_memcache = $this->vals['AddOnMemCache'] ? 'true' : 'false';
        $addon_memcache7 = $this->vals['AddOnMemCache7'] ? 'true' : 'false';
        $addon_memcache8 = $this->vals['AddOnMemCache8'] ? 'true' : 'false';
        $addon_memcachd = $this->vals['AddOnMemCachd'] ? 'true' : 'false';
        $addon_memcachd7 = $this->vals['AddOnMemCachd7'] ? 'true' : 'false';

        $loc = 'document.buildform';
        return "onClick=\"$loc.path_env.value='{$extraPathEnv}';
		$loc.installPath.value = '{$installPath}';
		$loc.compilerFlags.value = '{$flags}';
		$loc.configureParams.value = '{$params}';
        if ($loc.addonMailHeader != null)
            $loc.addonMailHeader.checked = {$addon_mailHeader};
		if ($loc.addonMemCache != null)
			$loc.addonMemCache.checked = {$addon_memcache};
		if ($loc.addonMemCache7 != null)
			$loc.addonMemCache7.checked = {$addon_memcache7};
		if ($loc.addonMemCache8 != null)
			$loc.addonMemCache8.checked = {$addon_memcache8};
		if ($loc.addonMemCachd != null)
			$loc.addonMemCachd.checked = {$addon_memcachd};
		if ($loc.addonMemCachd7 != null)
			$loc.addonMemCachd7.checked = {$addon_memcachd7};
		if ($loc.addonSuhosin != null)
			$loc.addonSuhosin.checked = {$addon_suhosin};
		\"";
    }

    private function escapeJsValue($value)
    {
        return str_replace(
            ["\\", "'"],
            ["\\\\", "\\'"],
            (string) $value
        );
    }
}
