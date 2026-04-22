<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\I18n\DMsg;

abstract class ProductBase
{
    protected static $instances = [];
    protected $version = '';
    protected $new_version = '';
    protected $buildDisplay = null;

    protected function __construct()
    {
        $this->version = $this->detectCurrentVersion();
        $this->new_version = $this->detectAvailableVersion();
    }

    public static function GetInstance()
    {
        $class = get_called_class();
        if (!isset(self::$instances[$class])) {
            self::$instances[$class] = new $class();
        }

        return self::$instances[$class];
    }

    public function getVersion()
    {
        return $this->version;
    }

    abstract public function getProductName();

    public function getBuildDisplay()
    {
        if ($this->buildDisplay === null) {
            $this->buildDisplay = $this->detectBuildDisplay();
        }

        return $this->buildDisplay;
    }

    public function refreshVersion()
    {
        $versionfile = $this->getServerRootPath('VERSION');
        if ($versionfile !== '' && is_file($versionfile)) {
            $this->version = trim(file_get_contents($versionfile));
        }
    }

    public function getNewVersion()
    {
        return $this->new_version;
    }

    protected function detectCurrentVersion()
    {
        $matches = [];
        $str = isset($_SERVER['LSWS_EDITION']) ? $_SERVER['LSWS_EDITION'] : '';
        if (preg_match('/(\d.*)$/i', $str, $matches)) {
            return trim($matches[1]);
        }

        $versionfile = $this->getServerRootPath('VERSION');
        if ($versionfile !== '' && is_file($versionfile)) {
            return trim((string) file_get_contents($versionfile));
        }

        return '';
    }

    protected function detectAvailableVersion()
    {
        $releasefile = $this->getServerRootPath('autoupdate/release');
        if ($releasefile === '') {
            return '';
        }

        $releasefilecb = $releasefile . 'cb';
        $newver = '';
        $rel0 = '';
        $rel1 = '';

        if (is_file($releasefilecb)) {
            $rel = trim(file_get_contents($releasefilecb));
            $rel0 = $this->extractReleaseVersion($rel);

            if ($this->version != $rel0) {
                $newver = $rel . ' (' . DMsg::UIStr('note_curbranch') . ') ';
            }
        }

        if (is_file($releasefile)) {
            $rel = trim(file_get_contents($releasefile));
            $rel1 = $this->extractReleaseVersion($rel);

            if ($this->version != $rel1 && $rel0 != $rel1) {
                $newver .= $rel;
            }
        }

        return $newver;
    }

    protected function extractReleaseVersion($release)
    {
        $pos = strpos($release, ' ');
        if ($pos !== false) {
            return substr($release, 0, $pos);
        }

        return $release;
    }

    public static function extractBuildDisplay($versionOutput)
    {
        if (!is_string($versionOutput) || $versionOutput === '') {
            return '';
        }

        if (preg_match('/\(BUILD built:\s*([^)]+)\)/i', $versionOutput, $matches)) {
            return 'BUILD: ' . trim($matches[1]);
        }

        if (preg_match('/BUILD built:\s*(.+)$/i', trim($versionOutput), $matches)) {
            return 'BUILD: ' . trim($matches[1]);
        }

        return '';
    }

    protected function detectBuildDisplay()
    {
        if (!function_exists('exec')) {
            return '';
        }

        foreach ($this->getBuildProbeBinaries() as $relativePath) {
            $binary = $this->getServerRootPath($relativePath);
            if ($binary === '' || !is_file($binary) || !is_executable($binary)) {
                continue;
            }

            $output = [];
            $status = 1;
            exec(escapeshellarg($binary) . ' -v 2>/dev/null', $output, $status);
            if ($status !== 0 || !isset($output[0])) {
                continue;
            }

            $buildDisplay = self::extractBuildDisplay($output[0]);
            if ($buildDisplay !== '') {
                return $buildDisplay;
            }
        }

        return '';
    }

    protected function getBuildProbeBinaries()
    {
        return [];
    }

    protected function getServerRootPath($relativePath)
    {
        if (!defined('SERVER_ROOT') || SERVER_ROOT === '') {
            return '';
        }

        return SERVER_ROOT . ltrim($relativePath, '/');
    }
}
