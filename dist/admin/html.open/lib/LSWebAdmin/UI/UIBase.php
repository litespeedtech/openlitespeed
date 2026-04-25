<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Product\Current\UI as CurrentProductUI;
use LSWebAdmin\Runtime\SInfo;

define('ASSETS_URL', 'res');

class UIBase
{
    protected $uiproperty;
    private static $inputSource;

    private static function getHeroPidLabel()
    {
        if (class_exists(CurrentProductUI::class)) {
            return CurrentProductUI::GetPidLabel();
        }

        return 'SERVER PID';
    }

    private static function stringInput($value)
    {
        if ($value === null) {
            return '';
        }

        if (is_scalar($value)) {
            return trim((string) $value);
        }

        return '';
    }

    public static function Escape($value)
    {
        if ($value === null) {
            return '';
        }

        if (is_bool($value)) {
            $value = $value ? '1' : '0';
        } elseif (!is_scalar($value)) {
            return '';
        }

        return htmlspecialchars((string) $value, ENT_QUOTES | ENT_SUBSTITUTE, 'UTF-8');
    }

    public static function EscapeAttr($value)
    {
        return self::Escape($value);
    }

    public static function EscapeJs($value)
    {
        if ($value === null) {
            $value = '';
        }

        $encoded = json_encode((string) $value, JSON_HEX_TAG | JSON_HEX_APOS | JSON_HEX_QUOT | JSON_HEX_AMP);
        return ($encoded === false) ? '""' : $encoded;
    }

    protected function __construct()
    {
        $this->uiproperty = new UIProperty();
    }

    public static function SetInputSource($inputSource = null)
    {
        self::$inputSource = $inputSource;
    }

    public static function ResetInputSource()
    {
        self::$inputSource = null;
    }

    public static function GetInputSource()
    {
        if (!(self::$inputSource instanceof RequestInput)) {
            self::$inputSource = RequestInput::fromGlobals();
        }

        return self::$inputSource;
    }

    protected function confform_start()
    {
        $formaction = $this->uiproperty->Get(UIProperty::FLD_FORM_ACTION);
        $buf = '
        <!-- ========================== confform STARTS HERE ========================== -->
        <form name="confform" id="confform" method="post" action="' . $formaction . '">
        ';
        return $buf;
    }

    protected function confform_end()
    {
        $buf = '';
        $hiddenvars = $this->uiproperty->Get(UIProperty::FLD_FORM_HIDDENVARS);
        foreach ($hiddenvars as $n => $v) {
            $buf .= '<input type="hidden" name="' . $n . '" value="' . $v . '">';
        }
        $buf .= '</form>
<!-- ========================== confform ENDS HERE ========================== -->
        ';
        return $buf;
    }

    protected function print_conf_page($disp, $page)
    {
        $uiContext = ConfUiContext::fromDisplay($disp);
        $this->uiproperty->Set(UIProperty::FLD_FORM_ACTION, 'index.php?view=confMgr');

        $uiContext->InitUIProps($this->uiproperty);

        $icontitle = $uiContext->GetIconTitle();
        $collapseSingleTopTab = $this->shouldCollapseSingleTopTab($page->GetLabel());
        echo $this->content_header($icontitle[0], $icontitle[1], $collapseSingleTopTab ? '' : $page->GetLabel());
        echo $this->confform_start();
        if (!$collapseSingleTopTab) {
            echo $this->main_tabs();
        }

        $tabPanelClass = 'lst-tab-panel-group';
        if ($collapseSingleTopTab) {
            $tabPanelClass .= ' lst-tab-panel-group--standalone';
        }
        echo '<div class="' . $tabPanelClass . '">';

        $page->PrintHtml($uiContext);

        echo "</div>\n";
        echo $this->confform_end();
    }

    protected function shouldCollapseSingleTopTab($pageLabel)
    {
        $tabs = $this->uiproperty->Get(UIProperty::FLD_TABS);
        if (!is_array($tabs) || count($tabs) !== 1 || $pageLabel !== DMsg::UIStr('tab_top')) {
            return false;
        }

        reset($tabs);
        $tabKey = key($tabs);
        if (!is_string($tabKey) || strlen($tabKey) < 2) {
            return false;
        }

        return (substr($tabKey, 1) === DMsg::UIStr('tab_top'));
    }

    protected function main_tabs()
    {
        $tabs = $this->uiproperty->Get(UIProperty::FLD_TABS);

        $buf = '<div><ul class="lst-tabs" role="tablist">';

        foreach ($tabs as $name => $uri) {
            $buf .= '<li';
            if ($name[0] == '1') {
                $buf .= ' class="active"';
            }

            $buf .= '><a href="' . $uri . '">' . substr($name, 1) . '</a></li>';
        }

        $buf .= "</ul></div>\n";
        return $buf;
    }

    public static function content_header($icon, $title, $subtitle = '', $showMetrics = true, $titleActions = '')
    {
        $serverload = Service::getServerLoad();
        $pid = Service::ServiceData(SInfo::DATA_PID);
        $titleText = self::Escape($title);
        $subtitleText = ($subtitle !== '') ? self::Escape($subtitle) : '';

        $heroClass = $showMetrics ? 'lst-page-hero' : 'lst-page-hero lst-page-hero--solo';

        $buf = '<section class="' . $heroClass . '">'
            . '<div class="lst-page-hero__title">'
            . '<h2 class="lst-page-heading"><i class="lst-icon" data-lucide="'
            . self::EscapeAttr($icon) . '"></i> ' . $titleText;

        if ($titleActions !== '') {
            $titleActions = '<span class="lst-page-heading__actions">' . $titleActions . '</span>';
        }

        if ($subtitleText !== '') {
            $buf .= ' <span>&gt; ' . $subtitleText . '</span>';
        }

        $buf .= $titleActions
            . '</h2>'
            . '</div>';

        if ($showMetrics) {
            $buf .= '<div class="lst-page-hero__metrics">'
                . '<ul id="lst-page-hero-metrics" class="lst-page-hero-metrics">'
                . '<li class="lst-sparks-item lst-sparks-item--pid">'
				. '<a class="lst-btn lst-btn--secondary lst-btn--sm lst-hero-btn lst-hero-btn--restart" rel="tooltip" data-original-title="' . self::EscapeAttr(DMsg::UIStr('menu_restart')) . '" href="#" data-lst-call="lst_restart"><i class="lst-icon" data-lucide="refresh-ccw-dot"></i></a>'
                . '<h5>' . self::Escape(self::getHeroPidLabel()) . ' <span id="lst-pid" class="lst-page-hero-value lst-page-hero-value--secondary">' . self::Escape($pid) . '</span></h5>'
                . '</li>'
                . '<li class="lst-sparks-item lst-sparks-item--load">'
				. '<a class="lst-btn lst-btn--secondary lst-btn--sm lst-hero-btn lst-hero-btn--stats" rel="tooltip" data-original-title="' . self::EscapeAttr(DMsg::UIStr('menu_rtstats')) . '" href="/index.php?view=realtimestats"><i class="lst-icon" data-lucide="activity"></i></a>'
                . '<h5>' . self::Escape(DMsg::UIStr('note_systemload')) . ' <span id="lst-load" class="lst-page-hero-value lst-page-hero-value--secondary">' . self::Escape($serverload) . '</span></h5>'
                . '</li>'
                . '</ul>'
                . '</div>';
        }

        $buf .= '</section>';
        return $buf;
    }

    public static function GetActionButtons($actdata, $type)
    {
        $buf = '<div ';
        if ($type == 'toolbar') {
            $buf .= 'class="lst-widget-controls" role="menu">';

            foreach ($actdata as $actKey => $act) {
                $className = 'lst-widget-control-btn lst-widget-control-btn--labeled';
                if (self::isPrimaryToolbarAction($actKey)) {
                    $className .= ' lst-widget-control-btn--primary';
                }
                $buf .= self::RenderActionControl($actKey, $act, $className, 'toolbar');
            }
        } elseif ($type == 'icon') {
            $buf .= 'class="lst-action-toolbar"><ul class="lst-action-list">';

            foreach ($actdata as $actKey => $act) {
                $buf .= '<li>' . self::RenderActionControl($actKey, $act, 'lst-btn lst-btn--secondary lst-action-btn lst-action-btn--icon') . '</li> ' . "\n";
            }

            $buf .= '</ul>';
        } elseif ($type == 'text') {
            $buf .= 'class="lst-action-toolbar"><ul class="lst-action-list lst-action-list--text">';
            foreach ($actdata as $actKey => $act) {
                $btnVariant = self::isDestructiveAction($actKey) ? 'lst-btn--danger' : 'lst-btn--secondary';
                $buf .= '<li class="lst-action-item">' . self::RenderActionControl($actKey, $act, 'lst-btn ' . $btnVariant . ' lst-action-btn lst-action-btn--text', 'text') . '</li> ' . "\n";
            }

            $buf .= '</ul>';
        }
        $buf .= '</div>';
        return $buf;
    }

    private static function RenderActionControl($actKey, $act, $className, $labelMode = null)
    {
        $iconHtml = '<i class="lst-icon" data-lucide="' . self::EscapeAttr($act['ico']) . '"></i>';
        $contentHtml = $iconHtml;
        $accessibilityAttrs = '';
        $overlayAttrs = '';

        if ($labelMode === 'text') {
            $contentHtml = '<span class="lst-action-btn__label">' . $iconHtml . '</span> <strong>'
                . self::Escape($act['label']) . ' </strong>';
        } elseif ($labelMode === 'toolbar') {
            $contentHtml = '<span class="lst-widget-control-btn__label">' . $iconHtml . '</span>'
                . '<span class="lst-widget-control-btn__text">' . self::Escape($act['label']) . '</span>';
        }

        if ($labelMode === null) {
            $accessibilityAttrs = ' aria-label="' . self::EscapeAttr($act['label']) . '"';
			$overlayAttrs = ' rel="tooltip" data-original-title="' . self::EscapeAttr($act['label']) . '"';
        }

        if (!empty($act['submit'])) {
            $submitName = isset($act['submit_name']) ? $act['submit_name'] : 'a';
            $submitValue = isset($act['submit_value']) ? $act['submit_value'] : $actKey;

            return '<button type="submit" name="' . self::EscapeAttr($submitName) . '" value="' . self::EscapeAttr($submitValue)
                . '" class="' . self::EscapeAttr($className) . '"' . $accessibilityAttrs . $overlayAttrs . '>'
                . $contentHtml . '</button>';
        }

        return '<a href="' . self::EscapeAttr($act['href'])
            . '" class="' . self::EscapeAttr($className) . '"' . $accessibilityAttrs . $overlayAttrs . '>'
            . $contentHtml . '</a>';
    }

    private static function isPrimaryToolbarAction($actKey)
    {
        return in_array($actKey, ['s', 'n'], true);
    }

    private static function isDestructiveAction($actKey)
    {
        return in_array($actKey, ['d', 'D'], true);
    }

    public static function GetTblTips($tips)
    {
        $buf = '<div class="lst-message lst-message--success lst-inline-tips"><ul>';
        foreach ($tips as $tip) {
            if ($tip != '') {
                $buf .= "<li>$tip</li>\n";
            }
        }
        $buf .= "</ul></div>\n";
        return $buf;
    }

    public static function message($title = "", $msg = "", $type = "normal")
    {
        return '<div class="lst-message lst-message--danger">' . $msg . '</div>';
    }

    public static function error_divmesg($msg)
    {
        return '<div class="lst-message lst-message--danger">' . $msg . '</div>';
    }

    public static function info_divmesg($msg)
    {
        return '<div class="lst-message lst-message--info">' . $msg . '</div>';
    }

    public static function warn_divmesg($msg)
    {
        return '<div class="lst-message lst-message--warning">' . $msg . '</div>';
    }

    public static function genOptions($options, $selValue, $keyIsValue = false)
    {
        $o = '';
        if ($options) {
            foreach ($options as $key => $value) {
                if ($keyIsValue) {
                    $key = $value;
                }
                if ($key === 'forcesel') {
                    $o .= '<option disabled ';
                    if ($selValue === null || $selValue === '') {
                        $o .= 'selected';
                    }
                } else {
                    $o .= '<option value="' . self::EscapeAttr($key) . '"';

                    if (($key == $selValue)
                            && !($selValue === '' && $key === 0)
                            && !($selValue === null && $key === 0)
                            && !($selValue === '0' && $key === '')
                            && !($selValue === 0 && $key === '')) {
                        $o .= ' selected="selected"';
                    }
                }
                $o .= '>' . self::Escape($value) . "</option>\n";
            }
        }
        return $o;
    }

    public static function Get_LangDropdown()
    {
        if (!\LSWebAdmin\Product\Current\UI::SupportsLanguageSelector()) {
            return '';
        }

        $langlist = DMsg::GetSupportedLang($curlang);

        $buf = '<a href="#" class="lst-dropdown-toggle lst-lang-toggle" data-lst-dropdown="true" aria-haspopup="true" aria-expanded="false" aria-controls="lst-lang"><span>'
                . self::Escape($langlist[$curlang][0]) . '</span> <i class="lst-icon" data-lucide="chevron-down"></i> </a>
                <ul id="lst-lang" class="lst-dropdown-menu lst-lang-menu" role="menu" aria-hidden="true">';

        foreach ($langlist as $lang => $linfo) {
            $buf .= '<li role="presentation"';
            if ($lang == $curlang) {
                $buf .= ' class="active"';
            }
            $buf .= '><a href="#" data-lang="' . self::EscapeAttr($lang) . '" role="menuitemradio" aria-checked="'
                    . (($lang == $curlang) ? 'true' : 'false') . '">' . self::Escape($linfo[0]) . '</a></li>';
        }
        $buf .= "</ul>\n";
        return $buf;
    }

    public static function Get_LangSidebarDropdown()
    {
        if (!\LSWebAdmin\Product\Current\UI::SupportsLanguageSelector()) {
            return '';
        }

        $langlist = DMsg::GetSupportedLang($curlang);

        $buf = '<div class="lst-sidebar-utility lst-sidebar-utility--lang lst-show-mobile">'
                . '<div class="lst-sidebar-utility__label">' . self::Escape(DMsg::UIStr('note_language')) . '</div>'
                . '<a href="#" class="lst-dropdown-toggle lst-lang-toggle lst-sidebar-utility__trigger" data-lst-dropdown="true" aria-haspopup="true" aria-expanded="false" aria-controls="lst-lang-sidebar"><span>'
                . self::Escape($langlist[$curlang][0]) . '</span> <i class="lst-icon" data-lucide="chevron-down"></i> </a>
                <ul id="lst-lang-sidebar" class="lst-dropdown-menu lst-lang-menu lst-sidebar-utility__menu" role="menu" aria-hidden="true">';

        foreach ($langlist as $lang => $linfo) {
            $buf .= '<li role="presentation"';
            if ($lang == $curlang) {
                $buf .= ' class="active"';
            }
            $buf .= '><a href="#" data-lang="' . self::EscapeAttr($lang) . '" role="menuitemradio" aria-checked="'
                    . (($lang == $curlang) ? 'true' : 'false') . '">' . self::Escape($linfo[0]) . '</a></li>';
        }
        $buf .= "</ul></div>\n";
        return $buf;
    }

    public static function GrabInput($origin, $name, $type = '')
    {
        return self::GetInputSource()->GrabInput($origin, $name, $type);
    }

    public static function GrabGoodInput($origin, $name, $type = '')
    {
        return self::GetInputSource()->GrabGoodInput($origin, $name, $type);
    }

    public static function GrabGoodInputWithReset($origin, $name, $type = '')
    {
        return self::GetInputSource()->GrabGoodInputWithReset($origin, $name, $type);
    }

}
