<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Base\ProductUI;

class UI extends ProductUI
{
    public static function GetPidLabel()
    {
        return 'OLS PID';
    }

    public static function GetNewVersionUrl()
    {
        return 'https://openlitespeed.org/release-log/?utm_source=Open&utm_medium=WebAdmin';
    }

    public static function IsNewVersionUrlExternal()
    {
        return true;
    }

    public static function GetHelpMenu()
    {
        return self::BuildHelpMenu(array(
            'docs' => array(
                'title' => DMsg::UIStr('menu_usermanual'),
                'url_target' => '_blank',
                'url' => DMsg::DocsUrl(),
            ),
            'guides' => array(
                'title' => DMsg::UIStr('menu_docs'),
                'url' => 'https://docs.openlitespeed.org/',
                'url_target' => '_blank',
            ),
            'devgroup' => array(
                'title' => DMsg::UIStr('menu_productannouncements'),
                'url' => 'https://groups.google.com/forum/#!forum/openlitespeed-development',
                'url_target' => '_blank',
            ),
            'releaselog' => array(
                'title' => DMsg::UIStr('menu_releaselog'),
                'url' => 'https://openlitespeed.org/release-log/?utm_source=Open&utm_medium=WebAdmin',
                'url_target' => '_blank',
            ),
            'forum' => array(
                'title' => DMsg::UIStr('menu_forum'),
                'url' => 'https://forum.openlitespeed.org/?utm_source=Open&utm_medium=WebAdmin',
                'url_target' => '_blank',
            ),
            'slack' => array(
                'title' => DMsg::UIStr('menu_slack'),
                'url' => 'https://www.litespeedtech.com/slack',
                'url_target' => '_blank',
            ),
            'cloudimage' => array(
                'title' => DMsg::UIStr('menu_cloudimage'),
                'url' => 'https://docs.litespeedtech.com/cloud/images/?utm_source=Open&utm_medium=WebAdmin',
                'url_target' => '_blank',
            ),
        ));
    }
}
