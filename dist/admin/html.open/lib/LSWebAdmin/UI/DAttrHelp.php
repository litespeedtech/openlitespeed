<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\I18n\DMsg;

class DAttrHelp
{

    private $name;
    private $desc;
    private $tips;
    private $syntax;
    private $example;

    public function __construct($name, $desc, $tips = null, $syntax = null, $example = null)
    {
        $this->name = $name;
        $this->desc = self::formatInlineTokens($desc);
        if ($tips) {
            $this->tips = self::formatInlineTokens(str_replace('<br>', '<br/>', $tips));
        }
        $this->syntax = self::formatInlineTokens($syntax);
        $this->example = self::formatInlineTokens($example);
    }

    private static function formatInlineTokens($content)
    {
        if (!is_string($content) || $content === '') {
            return $content;
        }

        $patterns = [
            '/\{tag\}(.*?)\{\/(?:tag)?\}/s' => '<span class="lst-inline-token lst-inline-token--tag">$1</span>',
            '/\{val\}(.*?)\{\/(?:val)?\}/s' => '<span class="lst-inline-token lst-inline-token--value">$1</span>',
            '/\{cmd\}(.*?)\{\/(?:cmd)?\}/s' => '<span class="lst-inline-token lst-inline-token--command">$1</span>',
        ];

        return preg_replace(array_keys($patterns), array_values($patterns), $content);
    }

    private function shouldRenderBlockedReason($blocked_reason)
    {
        if (!is_string($blocked_reason) || $blocked_reason === '') {
            return false;
        }

        $tipContent = trim(strip_tags(implode(' ', array_filter([
            $this->desc,
            $this->tips,
            $this->syntax,
            $this->example,
        ]))));

        if ($tipContent === '') {
            return true;
        }

        $reason = strtolower(trim(strip_tags($blocked_reason)));
        $content = strtolower($tipContent);

        if ($reason !== '' && strpos($content, $reason) !== false) {
            return false;
        }

        if (strpos($reason, 'web host elite') !== false
            && strpos($content, 'only available for web host elite') !== false) {
            return false;
        }

        if (strpos($reason, 'enterprise edition') !== false
            && strpos($content, 'enterprise edition') !== false) {
            return false;
        }

        if ((strpos($reason, 'multiple-worker') !== false || strpos($reason, 'multiple-cpu') !== false)
            && (strpos($content, 'multiple-worker') !== false || strpos($content, 'multiple-cpu') !== false)) {
            return false;
        }

        return true;
    }

    public function Render($blocked_version = 0, $blocked_reason = '')
    {
		$buf = '<button type="button" class="lst-help-trigger" rel="popover" data-placement="right"
				data-original-title="<span class=\'lst-popover-title-badge\'>i</span> <strong>' . $this->name
                . '</strong>" data-html="true" data-content=\'<div>';

        if ($this->shouldRenderBlockedReason($blocked_reason)) {
            $buf .= ' <i>' . $blocked_reason . '</i>';
        } else {
            switch ($blocked_version) {
                case 0:
                    break;
                case 1:
                    $buf .= ' <i>' . DMsg::UIStr('note_entfeature') . '</i>';
                    break;
                case 2:
                case 3:
                    $buf .= ' <i>' . DMsg::UIStr('note_multicpufeature') . '</i>';
                    break;
            }
        }
        $buf .= $this->desc . '<br><br>';
        if ($this->syntax) {
            $buf .= '<div class="lst-popover-mono"><strong>' . DMsg::UIStr('note_syntax') . ':</strong> '
                . $this->syntax . '</div><br>';
        }
        if ($this->example) {
            $buf .= '<div class="lst-popover-mono"><strong>' . DMsg::UIStr('note_example') . ':</strong> '
                . $this->example . '</div><br>';
        }
        if ($this->tips) {
            $buf .= '<strong>' . DMsg::UIStr('note_tips') . ':</strong><ul type=circle>';
            $tips = explode('<br/>', $this->tips);
            foreach ($tips as $ti) {
                $ti = trim($ti);
                if ($ti != '') {
                    $buf .= '<li>' . $ti . '</li>';
                }
            }
            $buf .= '</ul>';
        }

        $buf .= '</div>\'><i class="lst-icon lst-help-icon" data-lucide="circle-help"></i></button>';
        return $buf;
    }

}
