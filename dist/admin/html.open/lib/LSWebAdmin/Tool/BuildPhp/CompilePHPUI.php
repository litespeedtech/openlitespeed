<?php

namespace LSWebAdmin\Tool\BuildPhp;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\UI\UIBase;

class CompilePHPUI
{
    private $steps;
    private $check;

    public function __construct()
    {
        $this->init();
    }

    public function PrintPage()
    {
        echo $this->step_indicator();

        switch ($this->check->GetNextStep()) {
            case 0:
                return $this->print_intro();
            case 1:
                return $this->print_step_1();
            case 2:
                return $this->print_step_2();
            case 3:
                return $this->print_step_3();
            case 4:
                return $this->print_step_4();
            default:
                echo UIBase::error_divmesg('Invalid entrance');
        }
    }

    public function print_intro()
    {
        $buf = $this->form_start();

        $docUrl = 'https://docs.litespeedtech.com/lsws/extapp/php/getting_started/#installation';

        $buf .= '<div class="lst-build-intro">'
            . '<div class="lst-build-intro__hero">'
            . '<span class="lst-build-intro__badge">' . $this->escapeHtml(DMsg::ALbl('buildphp_introbadge')) . '</span>'
            . '<h3 class="lst-build-intro__title">' . $this->escapeHtml(DMsg::ALbl('buildphp_introtitle')) . '</h3>'
            . '<p class="lst-build-intro__lead">' . $this->escapeHtml(DMsg::ALbl('buildphp_introlead')) . '</p>'
            . '</div>'
            . '<div class="lst-build-intro__grid">'
            . '<article class="lst-build-intro-card lst-build-intro-card--recommended">'
            . '<div class="lst-build-intro-card__icon"><i class="lst-icon" data-lucide="package-check"></i></div>'
            . '<div class="lst-build-intro-card__body">'
            . '<h4>' . $this->escapeHtml(DMsg::ALbl('buildphp_introrecommendedtitle')) . '</h4>'
            . '<p>' . $this->escapeHtml(DMsg::ALbl('buildphp_introrecommendedbody')) . '</p>'
            . '<ul>'
            . '<li>' . $this->escapeHtml(DMsg::ALbl('buildphp_introrecommendeditem1')) . '</li>'
            . '<li>' . $this->escapeHtml(DMsg::ALbl('buildphp_introrecommendeditem2')) . '</li>'
            . '<li>' . $this->escapeHtml(DMsg::ALbl('buildphp_introrecommendeditem3')) . '</li>'
            . '</ul>'
            . '<div class="lst-build-intro-card__actions">'
            . '<a class="lst-btn lst-btn--primary" href="' . $this->escapeAttr($docUrl) . '" target="_blank" rel="noopener noreferrer">' . $this->escapeHtml(DMsg::ALbl('buildphp_introinstallguide')) . '</a>'
            . '</div>'
            . '</div>'
            . '</article>'
            . '<article class="lst-build-intro-card lst-build-intro-card--manual">'
            . '<div class="lst-build-intro-card__icon"><i class="lst-icon" data-lucide="hammer"></i></div>'
            . '<div class="lst-build-intro-card__body">'
            . '<h4>' . $this->escapeHtml(DMsg::ALbl('buildphp_intromanualtitle')) . '</h4>'
            . '<p>' . $this->escapeHtml(DMsg::ALbl('buildphp_intromanualbody')) . '</p>'
            . '<ul>'
            . '<li>' . $this->escapeHtml(DMsg::ALbl('buildphp_intromanualitem1')) . '</li>'
            . '<li>' . $this->escapeHtml(DMsg::ALbl('buildphp_intromanualitem2')) . '</li>'
            . '<li>' . $this->escapeHtml(DMsg::ALbl('buildphp_intromanualitem3')) . '</li>'
            . '</ul>'
            . '</div>'
            . '</article>'
            . '</div>'
            . '<div class="lst-build-intro-note">'
            . '<h4>' . $this->escapeHtml(DMsg::ALbl('buildphp_intronotetitle')) . '</h4>'
            . '<p>' . $this->escapeHtml(DMsg::ALbl('buildphp_intronotebody')) . '</p>'
            . '</div>'
            . '<div class="lst-build-intro__footer">'
            . '<button type="submit" name="next" value="1" class="lst-btn lst-btn--info lst-build-intro__continue">' . DMsg::ALbl('buildphp_continuecompile') . '</button>'
            . '</div>'
            . '</div>';

        $buf .= $this->form_end();

        echo $buf;
    }

    public function print_step_1()
    {
        $buf = $this->form_start();

        if (isset($this->check->pass_val['err'])) {
            $buf .= UIBase::error_divmesg($this->check->pass_val['err']);
        }

        $phpversion = BuildConfig::GetVersion(BuildConfig::PHP_VERSION);
        $select = $this->input_select('phpversel', $phpversion);
        $note = DMsg::ALbl('buildphp_updatever') . ' ' . BuildConfig::GetVersionFilePath();

        $buf .= $this->form_group(DMsg::ALbl('buildphp_phpver'), true, $select, 'phpversel', '', $note);
        $buf .= $this->form_end();

        echo $buf;
    }

    public function print_step_2()
    {
        $options = null;
        $saved_options = null;
        $default_options = null;
        $cur_step = $this->check->GetCurrentStep();
        $pass_val = $this->check->pass_val;

        if ($cur_step == 1) {
            $php_version = $pass_val['php_version'];
            $options = new BuildOptions($php_version);
            $options->setDefaultOptions();
            $default_options = $options;
        } elseif ($cur_step == 2) {
            $options = $pass_val['input_options'];
            $php_version = $options->GetValue('PHPVersion');
            $default_options = new BuildOptions($php_version);
            $default_options->setDefaultOptions();
        } elseif ($cur_step == 3) {
            $php_version = $pass_val['php_version'];
            $options = new BuildOptions($php_version);
            $default_options = new BuildOptions($php_version);
            $default_options->setDefaultOptions();
        }
        if ($options == null) {
            return "NULL options\n";
        }

        $supported = $this->check->GetModuleSupport($php_version);
        $saved_options = $options->getSavedOptions();
        if ($saved_options != null && $cur_step == 3) {
            $options = $saved_options;
        }

        $buf = $this->form_start();
        if (isset($pass_val['err'])) {
            $buf .= UIBase::error_divmesg(DMsg::ALbl('note_inputerr'));
        }

        $input = '<button type="button" class="lst-btn lst-btn--secondary lst-btn--sm"'
            . ($saved_options ? ' ' . $saved_options->gen_loadconf_onclick('IMPORT') : ' disabled="disabled"')
            . '>' . DMsg::ALbl('buildphp_useprevconf') . '</button> &nbsp;&nbsp;<button type="button" class="lst-btn lst-btn--secondary lst-btn--sm" '
            . $default_options->gen_loadconf_onclick('DEFAULT')
            . '>' . DMsg::ALbl('buildphp_restoredefault') . '</button>';
        $buf .= $this->form_group(DMsg::ALbl('buildphp_loadconf'), false, $input);

        $input = $this->input_text('path_env', $options->GetValue('ExtraPathEnv'));
        $err = isset($pass_val['err']['path_env']) ? $pass_val['err']['path_env'] : '';
        $tip = DMsg::GetAttrTip('extrapathenv')->Render();
        $buf .= $this->form_group(DMsg::ALbl('buildphp_extrapathenv'), false, $input, 'path_env', $tip, '', $err);

        $input = $this->input_text('installPath', $options->GetValue('InstallPath'));
        $err = isset($pass_val['err']['installPath']) ? $pass_val['err']['installPath'] : '';
        $tip = DMsg::GetAttrTip('installpathprefix')->Render();
        $buf .= $this->form_group(DMsg::ALbl('buildphp_installpathprefix'), true, $input, 'installPath', $tip, '', $err);

        $input = $this->input_text('compilerFlags', $options->GetValue('CompilerFlags'));
        $err = isset($pass_val['err']['compilerFlags']) ? $pass_val['err']['compilerFlags'] : '';
        $tip = DMsg::GetAttrTip('compilerflags')->Render();
        $buf .= $this->form_group(DMsg::ALbl('buildphp_compilerflags'), false, $input, 'compilerFlags', $tip, '', $err);

        $input = $this->input_textarea('configureParams', $options->GetValue('ConfigParam'), 6, 'soft');
        $err = isset($pass_val['err']['configureParams']) ? $pass_val['err']['configureParams'] : '';
        $tip = DMsg::GetAttrTip('configureparams')->Render();
        $buf .= $this->form_group(DMsg::ALbl('buildphp_confparam'), true, $input, 'configureParams', $tip, '', $err);

        $input = '';
        if ($supported['mailheader']) {
            $input = $this->input_checkbox(
                'addonMailHeader',
                $options->GetValue('AddOnMailHeader'),
                '<a class="lst-soft-link" href="http://choon.net/php-mail-header.php" target="_blank" rel="noopener noreferrer">' . DMsg::ALbl('buildphp_mailheader1') . '</a> (' . DMsg::ALbl('buildphp_mailheader2') . ')'
            );
        }
        if ($supported['suhosin']) {
            $input .= $this->input_checkbox(
                'addonSuhosin',
                $options->GetValue('AddOnSuhosin'),
                '<a class="lst-soft-link" href="http://suhosin.org" target="_blank" rel="noopener noreferrer">Suhosin</a> ' . DMsg::ALbl('buildphp_suhosin')
            );
        }
        if ($supported['memcache']) {
            $input .= $this->input_checkbox(
                'addonMemCache',
                $options->GetValue('AddOnMemCache'),
                '<a class="lst-soft-link" href="http://pecl.php.net/package/memcache" target="_blank" rel="noopener noreferrer">memcache</a> (memcached extension) V' . BuildConfig::GetVersion(BuildConfig::MEMCACHE_VERSION)
            );
        }
        if ($supported['memcache7']) {
            $input .= $this->input_checkbox(
                'addonMemCache7',
                $options->GetValue('AddOnMemCache7'),
                '<a class="lst-soft-link" href="http://pecl.php.net/package/memcache" target="_blank" rel="noopener noreferrer">memcache</a> (memcached extension) V' . BuildConfig::GetVersion(BuildConfig::MEMCACHE7_VERSION)
            );
        }
        if ($supported['memcache8']) {
            $input .= $this->input_checkbox(
                'addonMemCache8',
                $options->GetValue('AddOnMemCache8'),
                '<a class="lst-soft-link" href="http://pecl.php.net/package/memcache" target="_blank" rel="noopener noreferrer">memcache</a> (memcached extension) V' . BuildConfig::GetVersion(BuildConfig::MEMCACHE8_VERSION)
            );
        }
        if ($supported['memcachd']) {
            $input .= $this->input_checkbox(
                'addonMemCachd',
                $options->GetValue('AddOnMemCachd'),
                '<a class="lst-soft-link" href="http://pecl.php.net/package/memcached" target="_blank" rel="noopener noreferrer">memcached</a> (PHP extension for interfacing with memcached via libmemcached library) V' . BuildConfig::GetVersion(BuildConfig::MEMCACHED_VERSION)
            );
        }
        if ($supported['memcachd7']) {
            $input .= $this->input_checkbox(
                'addonMemCachd7',
                $options->GetValue('AddOnMemCachd7'),
                '<a class="lst-soft-link" href="http://pecl.php.net/package/memcached" target="_blank" rel="noopener noreferrer">memcached</a> (PHP extension for interfacing with memcached via libmemcached library) V' . BuildConfig::GetVersion(BuildConfig::MEMCACHED7_VERSION)
            );
        }
        if ($input !== '') {
            $note = DMsg::ALbl('buildphp_updatever') . ' <code>' . $this->escapeHtml(BuildConfig::GetVersionFilePath()) . '</code>';
            $buf .= $this->form_group(DMsg::ALbl('buildphp_addonmodules'), false, $input, '', '', $note, '', 'lst-build-option-list');
        }

        $buf .= $this->form_end();
        echo $buf;
    }

    public function print_step_3()
    {
        $options = $this->check->pass_val['build_options'];
        if ($options == null) {
            return;
        }

        $buf = $this->form_start();
        $err = '';
        $optionsaved = '';
        $tool = new BuildTool($options);
        if (!$tool || !$tool->GenerateScript($err, $optionsaved)) {
            $buf .= UIBase::error_divmesg(DMsg::UIStr('buildphp_failgenscript') . " $err");
        } else {
            if ($optionsaved) {
                $buf .= UIBase::info_divmesg(DMsg::UIStr('buildphp_confsaved'));
            } else {
                $buf .= UIBase::error_divmesg(DMsg::UIStr('buildphp_failsaveconf'));
            }

            $_SESSION['progress_file'] = $tool->progress_file;
            $_SESSION['log_file'] = $tool->log_file;

            $cmd = 'bash -c "exec ' . $tool->build_prepare_script . ' 1> ' . $tool->log_file . ' 2>&1" &';
            exec($cmd);

            $buf .= UIBase::warn_divmesg(DMsg::UIStr('buildphp_nobrowserrefresh'));
            $buf .= '<input type="hidden" name="manual_script" value="' . $this->escapeAttr($tool->build_manual_run_script) . '">';
            $buf .= '<input type="hidden" name="extentions" value="' . $this->escapeAttr($tool->extension_used) . '">';
            $buf .= '<section class="lst-build-status-block">'
                    . '<div class="lst-build-status-header"><h5 class="lst-build-status-title">' . DMsg::ALbl('buildphp_mainstatus') . '</h5><span id="statusgraphzone" class="lst-build-status-indicator"><i class="lst-build-status-spinner lst-icon" data-lucide="settings"></i></span></div>'
                    . '<pre class="lst-statuszone" id="statuszone"></pre>'
                    . '</section>'
                    . '<section class="lst-build-status-block lst-build-status-block--log">'
                    . '<div class="lst-build-status-header"><h5 class="lst-build-status-title">' . DMsg::ALbl('buildphp_detaillog') . '</h5></div>'
                    . '<pre class="lst-logzone" id="logzone">' . $this->escapeHtml($cmd) . '</pre>'
                    . '</section>';
        }

        $buf .= $this->form_end();
        echo $buf;
    }

    public function print_step_4()
    {
        $manual_script = $this->check->pass_val['manual_script'];
        if ($manual_script == null) {
            return;
        }

        $buf = $this->form_start();
        $ver = $this->check->pass_val['php_version'];
        $binname = 'lsphp-' . $ver;

        $repl = ['%%server_root%%' => SERVER_ROOT, '%%binname%%' => $binname, '%%phpver%%' => $ver[0]];
        $notes = '<ul><li>' . DMsg::UIStr('buildphp_binarylocnote', $repl) . '</li>';

        if ($this->check->pass_val['extentions'] != '') {
            $notes .= "\n" . BuildTool::getExtensionNotes($this->check->pass_val['extentions']);
        }
        $notes .= '</ul>';

        $buf .= UIBase::info_divmesg($notes);

        $echo_cmd = 'echo "For security reason, please log in and manually run the pre-generated script to continue."';
        exec($echo_cmd . ' > ' . $this->check->pass_val['log_file']);
        exec($echo_cmd . ' > ' . $this->check->pass_val['progress_file']);

        $repl = ['%%manual_script%%' => $manual_script];
        $buf .= UIBase::warn_divmesg(DMsg::UIStr('buildphp_manualrunnotice', $repl));
        $buf .= '<section class="lst-build-status-block">'
                . '<div class="lst-build-status-header"><h5 class="lst-build-status-title">' . DMsg::ALbl('buildphp_mainstatus') . '</h5><span id="statusgraphzone" class="lst-build-status-indicator"><i class="lst-build-status-spinner lst-icon" data-lucide="settings"></i></span></div>'
                . '<pre class="lst-statuszone" id="statuszone"></pre>'
                . '</section>'
                . '<section class="lst-build-status-block lst-build-status-block--log">'
                . '<div class="lst-build-status-header"><h5 class="lst-build-status-title">' . DMsg::ALbl('buildphp_detaillog') . '</h5></div>'
                . '<pre class="lst-logzone" id="logzone"></pre>'
                . '</section>';

        $buf .= $this->form_end();
        echo $buf;
    }

    private function init()
    {
        $this->steps = [
            0 => DMsg::ALbl('buildphp_step0'),
            1 => DMsg::ALbl('buildphp_step1'),
            2 => DMsg::ALbl('buildphp_step2'),
            3 => DMsg::ALbl('buildphp_step3'),
            4 => DMsg::ALbl('buildphp_step4'),
        ];

        $this->check = new BuildCheck();
    }

    private function step_indicator()
    {
        $cur_step = $this->check->GetNextStep();
        $buf = '<div class="lst-build-stepper lst-build-wizard"><ul class="lst-build-stepper__list">';
        foreach ($this->steps as $i => $title) {
            $class = '';
            $label = $i;

            if ($i == $cur_step) {
                $class = 'active';
            } elseif ($cur_step > 0 && $i < $cur_step) {
                $class = 'complete';
                $label = '<i class="lst-icon" data-lucide="check"></i>';
            }

            $buf .= '<li class="lst-build-stepper__item';
            if ($class) {
                $buf .= ' ' . $class;
            }
            $buf .= '"><span class="lst-build-stepper__step">' . $label . '</span>
					<span class="lst-build-stepper__title">' . $title . '</span></li>';
        }
        $buf .= '</ul></div>';
        return $buf;
    }

    private function action_btn($label, $value, $id = '', $disabled = '')
    {
        $buf = '<button type="submit" name="next" value="' . $this->escapeAttr($value) . '" ';
        if ($id != '') {
            $buf .= 'id="' . $id . '" ';
        }

        $btnClass = ($value === '0') ? 'lst-btn--secondary' : 'lst-btn--info';

        $buf .= 'class="lst-btn ' . $btnClass;
        if ($disabled) {
            $buf .= ' disabled';
        }
        $buf .= '"';
        if ($disabled) {
            $buf .= ' disabled="disabled"';
        }
        $buf .= '>' . $label . "</button>\n";
        return $buf;
    }

    private function form_actions($cur_step)
    {
        $hasNext = ($cur_step < 4 && $cur_step != 0);
        $hasPrev = ($cur_step == 1 || $cur_step == 2 || $cur_step == 3);
        $nextDisabled = ($cur_step == 3);
        $prevDisabled = false;

        if (!$hasNext && !$hasPrev) {
            return '';
        }

        $buf = '<div class="lst-build-form-footer" role="menu"><div class="lst-build-form-shell"><div class="lst-build-form-footer__actions">';

        if ($hasPrev) {
            $buf .= '<div class="lst-build-form-footer__group lst-build-form-footer__group--secondary">'
                . $this->action_btn(DMsg::UIStr('btn_prev'), '0', 'prevbtn', $prevDisabled)
                . '</div>';
        }

        if ($hasNext) {
            $nextLabel = ($cur_step == 0) ? DMsg::ALbl('buildphp_continuecompile') : DMsg::UIStr('btn_next');
            $buf .= '<div class="lst-build-form-footer__group lst-build-form-footer__group--primary">'
                . $this->action_btn($nextLabel, '1', 'nextbtn', $nextDisabled)
                . '</div>';
        }

        $buf .= '</div></div></div>';
        return $buf;
    }

    private function form_start()
    {
        $cur_step = $this->check->GetNextStep();
        $buf = '<form name="buildform" id="buildform" class="lst-build-form" method="post" action="index.php?view=compilePHP">
  <div class="lst-widget lst-build-widget">
  <header role="heading" class="lst-widget-header">';

        $buf .= '<span class="lst-widget-icon"><i class="lst-icon" data-lucide="arrow-right-circle"></i></span>
   <h2><strong> ' . $cur_step . '</strong> - ' . $this->steps[$cur_step];

        if ($cur_step > 1) {
            $buf .= ' for PHP ' . $this->check->pass_val['php_version'];
        }

        $buf .= '</h2></header>
		<div role="content" class="lst-widget-content"><div class="lst-widget-body lst-form lst-build-form-body">
			<div class="lst-build-form-shell">';
        return $buf;
    }

    private function form_end()
    {
        $cur_step = $this->check->GetNextStep();
        if (isset($this->check->pass_val['php_version'])) {
            $version = $this->check->pass_val['php_version'];
        } else {
            $version = '';
        }

        return '</div>' . $this->form_actions($cur_step) . '</div></div></div>
    <input type="hidden" name="curstep" value="' . $this->escapeAttr($cur_step) . '">
    <input type="hidden" name="buildver" value="' . $this->escapeAttr($version) . '">
    </form>';
    }

    private function form_group($label, $required, $input, $fieldId = '', $tip = '', $note = '', $err = '', $inputWrapClass = 'lst-input-group lst-build-input-group')
    {
        $buf = '<div class="lst-form-row lst-build-form-group';
        if ($err) {
            $buf .= ' lst-form-row--error';
        }
        $buf .= '"><label class="lst-form-label lst-build-form-label"';
        if ($fieldId !== '') {
            $buf .= ' for="' . $this->escapeAttr($fieldId) . '"';
        }
        $buf .= '>' . $label;

        if ($required) {
            $buf .= ' *';
        }

        $buf .= '</label><div class="lst-form-field lst-build-form-field"><div class="' . $this->escapeAttr($inputWrapClass) . '">';
        if ($tip) {
            $buf .= '<span class="lst-input-addon lst-build-help-addon">' . $tip . '</span>';
        }

        $buf .= $input . '</div>';
        if ($err) {
            $buf .= '<span class="lst-form-error lst-build-form-error"><i class="lst-icon" data-lucide="triangle-alert"></i> ' . $this->escapeHtml($err) . '</span>';
        }
        if ($note) {
            $buf .= '<p class="lst-form-note lst-build-form-note">' . $note . '</p>';
        }

        $buf .= '</div></div>';
        return $buf;
    }

    private function input_select($name, $options, $val = '')
    {
        return '<select class="lst-choice-control" name="' . $name . '" id="' . $name . '">'
            . UIBase::genOptions($options, $val, true)
            . '</select>';
    }

    private function input_text($name, $val)
    {
        return '<input class="lst-choice-control" type="text" name="' . $name . '" id="' . $name . '" value="' . $this->escapeAttr($val) . '">';
    }

    private function input_textarea($name, $value, $rows, $wrap = 'off')
    {
        return '<textarea class="lst-choice-control" name="' . $name . '" id="' . $name . '" rows="' . $rows . '" wrap="' . $wrap . '">' . $this->escapeHtml($value) . "</textarea>\n";
    }

    private function input_checkbox($name, $value, $label)
    {
        $buf = '<div class="lst-build-option"><label><input type="checkbox" name="' . $name . '" id="' . $name . '"';
        if ($value) {
            $buf .= ' checked';
        }
        $buf .= '><span class="lst-build-option__text">' . $label . "</span></label></div>\n";
        return $buf;
    }

    private function escapeAttr($value)
    {
        return htmlspecialchars((string) $value, ENT_QUOTES);
    }

    private function escapeHtml($value)
    {
        return htmlspecialchars((string) $value, ENT_QUOTES);
    }
}
