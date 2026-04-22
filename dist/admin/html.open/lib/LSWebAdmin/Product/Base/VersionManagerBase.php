<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\Controller\AdminCommandClient;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\Service;

abstract class VersionManagerBase
{
    const ACTION_UPGRADE = 'upgrade';
    const ACTION_SWITCH = 'switchTo';
    const ACTION_REMOVE = 'remove';

    abstract protected function getProduct();

    abstract protected function downloadReleaseArchive($version);

    protected function buildUpgradeCommand($version)
    {
        return 'upgrade:' . $version;
    }

    public function getViewData()
    {
        $product = $this->getProduct();

        return [
            'product_name' => $product->getProductName(),
            'edition' => method_exists($product, 'getEdition') ? $product->getEdition() : '',
            'version' => $product->getVersion(),
            'current_build' => method_exists($product, 'getCurrentBuild') ? $product->getCurrentBuild() : '',
            'release_log_url' => method_exists($product, 'getReleaseLogUrl') ? $product->getReleaseLogUrl() : '',
            'available_releases' => method_exists($product, 'getAvailableReleases') ? $product->getAvailableReleases() : [],
            'installed_releases' => method_exists($product, 'getInstalledReleases') ? $product->getInstalledReleases() : [],
            'license' => Service::LicenseInfo(),
            'has_validate_license' => $this->supportsValidateLicense(),
        ];
    }

    public function perform($action, $version = '')
    {
        switch ((string) $action) {
            case self::ACTION_UPGRADE:
                return $this->upgrade($version);

            case self::ACTION_SWITCH:
                return $this->switchTo($version);

            case self::ACTION_REMOVE:
                return $this->remove($version);

            case 'validatelicense':
                if ($this->supportsValidateLicense()) {
                    return $this->validateLicense();
                }
                return null;
        }

        return null;
    }

    protected function supportsValidateLicense()
    {
        return false;
    }

    protected function upgrade($version)
    {
        if (!$this->isValidVersion($version)) {
            return $this->errorMessage(DMsg::UIStr('service_versionmanager_invalidversion'));
        }

        if (!$this->downloadReleaseArchive($version)) {
            return $this->errorMessage(DMsg::UIStr('service_versionmanager_downloadfailed') . ' ' . $version);
        }

        return $this->runCommand(
            $this->buildUpgradeCommand($version),
            DMsg::UIStr('service_versionmanager_upgrade_started') . ' ' . $version
        );
    }

    protected function switchTo($version)
    {
        if (!$this->isValidVersion($version)) {
            return $this->errorMessage(DMsg::UIStr('service_versionmanager_invalidversion'));
        }

        return $this->runCommand(
            'mgrver:' . $version,
            DMsg::UIStr('service_versionmanager_switch_started') . ' ' . $version
        );
    }

    protected function remove($version)
    {
        if (!$this->isValidVersion($version)) {
            return $this->errorMessage(DMsg::UIStr('service_versionmanager_invalidversion'));
        }

        return $this->runCommand(
            'mgrver:-d ' . $version,
            DMsg::UIStr('service_versionmanager_remove_started') . ' ' . $version
        );
    }

    protected function validateLicense()
    {
        return $this->runCommand(
            'ValidateLicense',
            DMsg::UIStr('service_versionmanager_validate_started')
        );
    }

    protected function runCommand($command, $successMessage)
    {
        if (!$this->sendCommand($command)) {
            return $this->errorMessage(DMsg::UIStr('service_versionmanager_commandfailed'));
        }

        $this->waitForStatusChange();
        $this->getProduct()->refreshVersion();

        return [
            'type' => 'success',
            'text' => $successMessage,
        ];
    }

    protected function sendCommand($command)
    {
        CAuthorizer::singleton()->Reauthenticate();
        return $this->getAdminCommandClient()->sendCommand($command . "\n");
    }

    protected function waitForStatusChange()
    {
        Service::WaitForStatusChange();
    }

    protected function getAdminCommandClient()
    {
        return new AdminCommandClient();
    }

    protected function getServerRoot()
    {
        if (defined('SERVER_ROOT') && SERVER_ROOT !== '') {
            return SERVER_ROOT;
        }

        if (isset($_SERVER['LS_SERVER_ROOT']) && is_string($_SERVER['LS_SERVER_ROOT']) && $_SERVER['LS_SERVER_ROOT'] !== '') {
            return rtrim($_SERVER['LS_SERVER_ROOT'], '/') . '/';
        }

        return '';
    }

    protected function isValidVersion($version)
    {
        return (preg_match('/^\d+\.\d+(\.\d+)?(RC\d+)?$/', (string) $version) === 1);
    }

    protected function errorMessage($text)
    {
        return [
            'type' => 'danger',
            'text' => $text,
        ];
    }
}
