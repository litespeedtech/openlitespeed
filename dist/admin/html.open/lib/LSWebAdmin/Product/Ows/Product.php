<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Base\ProductBase;

class Product extends ProductBase
{
    public function getProductName()
    {
        return 'OpenLiteSpeed';
    }

    protected function getBuildProbeBinaries()
    {
        return [
            'bin/openlitespeed',
            'bin/lshttpd',
        ];
    }

    protected function detectAvailableVersion()
    {
        $newVersion = '';
        $currentBranchVersion = '';

        $currentBranchRelease = $this->readReleaseFile('autoupdate/releasecb');
        if ($currentBranchRelease !== '') {
            $currentBranchVersion = $this->extractReleaseVersion($currentBranchRelease);

            if ($this->version != $currentBranchVersion) {
                $newVersion = $currentBranchRelease . ' (' . DMsg::UIStr('note_curbranch') . ') ';
            }
        }

        $latestRelease = $this->readReleaseFile('autoupdate/release');
        if ($latestRelease !== '') {
            $latestVersion = $this->extractReleaseVersion($latestRelease);

            if ($this->version != $latestVersion && $currentBranchVersion != $latestVersion) {
                $newVersion .= $latestRelease;
            }
        }

        return $newVersion;
    }
}
