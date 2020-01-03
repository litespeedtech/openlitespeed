<?php

class Product
{

    const PROD_NAME = 'OpenLiteSpeed';

    private $version;
    private $new_version;

    private function __construct()
    {
        $matches = [];
        $str = $_SERVER['LSWS_EDITION'];
        if (preg_match('/(\d.*)$/i', $str, $matches)) {
            $this->version = trim($matches[1]);
        }
        $releasefile = SERVER_ROOT . 'autoupdate/release';
        $releasefilecb = $releasefile . 'cb';
        $newver = '';
        $rel0 = $rel1 = '';

        if (file_exists($releasefilecb)) {
            $rel = trim(file_get_contents($releasefilecb));
            if (($pos = strpos($rel, ' ')) !== false) {
                $rel0 = substr($rel, 0, $pos);
            } else {
                $rel0 = $rel;
            }
            
            if ($this->version != $rel0) {
                $newver = $rel . ' (' . DMsg::UIStr('note_curbranch') . ') '; // will carry over extra string if it's in release
            }
        }
        
        if (file_exists($releasefile)) {
            $rel = trim(file_get_contents($releasefile));
            if (($pos = strpos($rel, ' ')) !== false) {
                $rel1 = substr($rel, 0, $pos);
            } else {
                $rel1 = $rel;
            }
            
            if ($this->version != $rel1 && $rel0 != $rel1) {
                $newver .= $rel; // will carry over extra string if it's in release
            }
        }

        $this->new_version = $newver;
    }

    public static function GetInstance()
    {
        if (!isset($GLOBALS['_PRODUCT_']))
            $GLOBALS['_PRODUCT_'] = new Product();
        return $GLOBALS['_PRODUCT_'];
    }

    public function getVersion()
    {
        return $this->version;
    }

    public function refreshVersion()
    {
        $versionfile = SERVER_ROOT . 'VERSION';
        $this->version = trim(file_get_contents($versionfile));
    }

    public function getNewVersion()
    {
        return $this->new_version;
    }

}
