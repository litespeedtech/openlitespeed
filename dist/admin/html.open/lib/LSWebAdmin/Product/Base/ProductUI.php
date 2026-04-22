<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\UI\UIBase;

class ProductUI extends UIBase
{
    public static function SupportsRequestSnapshot()
    {
        return false;
    }

    public static function SupportsVersionManager()
    {
        return false;
    }

    public static function SupportsLanguageSelector()
    {
        return true;
    }

    public static function UsesToolPageStyle($view)
    {
        return in_array($view, array('compilePHP', 'logviewer', 'loginhistory', 'opsauditlog', 'realtimestats', 'blockedips', 'listenervhmap'), true);
    }

    public static function GetPidLabel()
    {
        return 'SERVER PID';
    }

    public static function GetHelpMenu()
    {
        return self::BuildHelpMenu(array());
    }

    protected static function BuildHelpMenu($subItems)
    {
        return array(
            'title' => DMsg::UIStr('menu_help'),
            'icon' => 'circle-help',
            'sub' => $subItems,
        );
    }

    public static function GetRequestSnapshotDetailLink()
    {
        return '';
    }

    public static function GetRequestProcessingLabel($label)
    {
        return $label;
    }

    public static function GetVersionNotice($version, $newVersion)
    {
        return '<span class="lst-version-value lst-version-value--compact">'
            . UIBase::Escape(DMsg::UIStr('note_version') . ' ' . $version) . '</span>';
    }

    public static function GetNewVersionUrl()
    {
        return '';
    }

    public static function IsNewVersionUrlExternal()
    {
        return false;
    }

    public static function PrintConfPage($disp, $page)
    {
        $ui = new static();
        $ui->print_conf_page($disp, $page);
    }
}
