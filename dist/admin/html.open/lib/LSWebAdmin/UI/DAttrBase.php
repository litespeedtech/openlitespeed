<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\ConfValidation;
use LSWebAdmin\Product\Current\DAttr;

/*
 * type: parse  _minVal = pattern, _maxVal = pattern tips
 *
 */
define('ATTR_VAL_NOT_SET', DMsg::UIStr('o_notset'));
define('ATTR_VAL_BOOL_YES', DMsg::UIStr('o_yes'));
define('ATTR_VAL_BOOL_NO', DMsg::UIStr('o_no'));
define('ATTR_NOTE_NUM_RANGE', DMsg::UIStr('note_numvalidrange'));
define('ATTR_NOTE_NUMBER', DMsg::UIStr('note_number'));

class DAttrBase
{

	protected $_key;
	protected $_keyalias;
	public $_helpKey;
	public $_type;
	public $_minVal;
	public $_maxVal;
	public $_label;
	public $_href;
	public $_hrefLink;
	public $_multiInd;
	public $_note;
	public $_icon;
	protected $_inputType;
	protected $_inputAttr;
	protected $_glue;
	protected $_bitFlag = 0;

	const BM_NOTNULL = 1;
	const BM_NOEDIT = 2;
	const BM_HIDE = 4;
	const BM_NOFILE = 8;
	const BM_RAWDATA = 16;

	public function __construct($key, $type, $label, $inputType = null, $allowNull = true, $min = null, $max = null, $inputAttr = null, $multiInd = 0, $helpKey = null)
	{
		$this->_key = $key;
		$this->_type = $type;
		$this->_label = $label;
		$this->_minVal = $min;
		$this->_maxVal = $max;
		$this->_inputType = $inputType;
		$this->_inputAttr = $inputAttr;
		$this->_multiInd = $multiInd;
		$this->_helpKey = ($helpKey == null) ? $key : $helpKey;

		$this->_bitFlag = $allowNull ? 0 : self::BM_NOTNULL;
	}

	public function SetGlue($glue)
	{
		$this->_glue = $glue;
	}

	public function SetFlag($flag)
	{
		$this->_bitFlag |= $flag;
	}

	public function IsFlagOn($flag)
	{
		return (($this->_bitFlag & $flag) == $flag );
	}

	public function GetKey()
	{
		return $this->_key;
	}

	public function GetInputType()
	{
		return $this->_inputType;
	}

	public function dup($key, $label, $helpkey)
	{
		$cname = get_class($this);
		$d = new $cname($this->_key, $this->_type, $this->_label, $this->_inputType, true, $this->_minVal, $this->_maxVal, $this->_inputAttr, $this->_multiInd, $this->_helpKey);

		$d->_glue = $this->_glue;
		$d->_href = $this->_href;
		$d->_hrefLink = $this->_hrefLink;
		$d->_bitFlag = $this->_bitFlag;
		$d->_note = $this->_note;
		$d->_icon = $this->_icon;


		if ($key) {
			$d->_key = $key;
		}
		if ($label) {
			$d->_label = $label;
		}

		if ($helpkey) {
			$d->_helpKey = $helpkey;
		}

		return $d;
	}

	protected function extractCheckBoxOr($inputSource = null)
	{
		if ($inputSource == null) {
			$inputSource = UIBase::GetInputSource();
		}

		$value = 0;
		$novalue = 1;
		foreach ($this->_maxVal as $val => $disp) {
			$name = $this->_key . $val;
			if ($inputSource->HasInput('POST', $name)) {
				$novalue = 0;
				$value = $value | $val;
			}
		}
		return ( $novalue ? '' : (string) $value );
	}

	protected function extractSplitMultiple(&$value)
	{
		if ($this->_glue == ' ') {
			$vals = preg_split("/[,; ]+/", $value, -1, PREG_SPLIT_NO_EMPTY);
		} else {
			$vals = preg_split("/[,;]+/", $value, -1, PREG_SPLIT_NO_EMPTY);
		}

		$vals1 = [];
		foreach ($vals as $val) {
			$val1 = trim($val);
			if (strlen($val1) > 0 && !in_array($val1, $vals1)) {
				$vals1[] = $val1;
			}
		}

		if ($this->_glue == ' ') {
			$value = implode(' ', $vals1);
		} else {
			$value = implode(', ', $vals1);
		}
	}

	protected function toHtmlContent($node, $refUrl = null)
	{
		if ($node == null || !$node->HasVal()) {
			return '<span class="lst-empty-value">' . ATTR_VAL_NOT_SET . '</span>';
		}

		$o = '';
		$value = $node->Get(CNode::FLD_VAL);
		$err = $node->Get(CNode::FLD_ERR);
		$quoted_value = $value ? htmlspecialchars($value, ENT_QUOTES) : '';
		$viewFieldId = 'lst-view-field-' . preg_replace('/[^A-Za-z0-9_-]+/', '-', $this->_key);

		if ($this->_type == 'sel1' && $value && !array_key_exists($value, $this->_maxVal)) {
			$err = 'Invalid value - ' . $quoted_value;
		} elseif ($err) {
			$type3 = substr($this->_type, 0, 3);
			if ($type3 == 'fil' || $type3 == 'pat') {
				$validator = new ConfValidation();
				$validator->chkAttr_file_val($this, $value, $err);
			}
		}

		if ($err) {
			$node->SetErr($err);
			$o .= '<span class="field_error lst-config-error"><i class="lst-icon" data-lucide="triangle-alert"></i> ' . UIBase::Escape($err) . '</span><br>';
		}

		if ($this->_href) {
			$link = $this->_hrefLink;
			if (strpos($link, '$V') !== false) {
				$link = str_replace('$V', urlencode($value), $link);
			}
			$o .= '<span class="field_url"><a href="' . $link . '">';
		} elseif ($refUrl != null) {
			$o .= '<span class="field_refUrl"><a href="' . $refUrl . '">';
		}

		if ($this->_type === 'bool') {
			if ($value === '1') {
				$o .= ATTR_VAL_BOOL_YES;
			} elseif ($value === '0') {
				$o .= ATTR_VAL_BOOL_NO;
			} else {
				$o .= '<span class="lst-empty-value">' . ATTR_VAL_NOT_SET . '</span>';
			}
		} elseif ($this->_type == 'ctxseq') {
			$o = '<span class="lst-ctx-order-value">' . UIBase::Escape($value) . '</span>';
		} elseif ($this->_key == "note") {
			$o .= '<textarea id="' . UIBase::EscapeAttr($viewFieldId) . '" name="' . UIBase::EscapeAttr($this->_key) . '" readonly class="lst-config-readonly-textarea lst-config-readonly-textarea--auto">' . $quoted_value . '</textarea>';
		} elseif ($this->_type === 'sel' || $this->_type === 'sel1') {
			if ($this->_maxVal && array_key_exists($value, $this->_maxVal)) {
				$o .= $this->_maxVal[$value];
			} else {
				$o .= $quoted_value;
			}
		} elseif ($this->_type === 'checkboxOr') {
			$o .= '<div class="lst-config-checklist-view">';
			if ($this->_minVal !== null && ($value === '' || $value === null)) {
				// has default value, for "Not set", set default val
				$value = $this->_minVal;
			}
			foreach ($this->_maxVal as $val => $name) {
				$o .= '<span class="lst-config-checklist-item">';
				if (($value & $val) || ($value === $val) || ($value === '0' && $val === 0)) {
					$o .= '<i class="lst-icon" data-lucide="check-square">';
				} else {
					$o .= '<i class="lst-icon" data-lucide="square">';
				}
				$o .= '</i> ';
				$o .= $name;
				$o .= '</span>';
			}
			$o .= '</div>';
		} elseif ($this->_inputType === 'textarea1') {
			$o .= '<textarea id="' . UIBase::EscapeAttr($viewFieldId) . '" name="' . UIBase::EscapeAttr($this->_key) . '" readonly class="lst-config-readonly-textarea"' . $this->_inputAttr . '>' . $quoted_value . '</textarea>';
		} elseif ($this->_inputType === 'text') {
			$o .= '<span class="field_text">' . $quoted_value . '</span>';
		} elseif ($value) {
			$o .= htmlspecialchars($value);
		}


		if ($this->_href || $refUrl) {
			$o .= '</a></span>';
		}
		return $o;
	}

	protected function getNote()
	{
		if ($this->_note) {
			return $this->_note;
		}
		if ($this->_type == 'uint') {
			if ($this->_maxVal) {
				return ATTR_NOTE_NUM_RANGE . ': ' . $this->_minVal . ' - ' . $this->_maxVal;
			}
			if ($this->_minVal !== null) {
				return ATTR_NOTE_NUM_RANGE . ' >= ' . $this->_minVal;
			}
		}
		return null;
	}

	public function extractPost($parent, $inputSource = null)
	{
		if ($inputSource == null) {
			$inputSource = UIBase::GetInputSource();
		}

		if ($this->_type == 'checkboxOr') {
			$value = $this->extractCheckBoxOr($inputSource);
		} else {
			$value = $inputSource->GrabInput("POST", $this->_key);
		}
		if ($value) {
			$value = str_replace("\r\n", "\n", $value);
		}

		$key = $this->_key;
		$node = $parent;
		while (($pos = strpos($key, ':')) > 0) {
			$key0 = substr($key, 0, $pos);
			$key = substr($key, $pos + 1);
			if ($node->HasDirectChildren($key0)) {
				$node = $node->GetChildren($key0);
			} else {
				$child = new CNode($key0, '', CNode::T_KB);
				$node->AddChild($child);
				$node = $child;
			}
		}

		if ($this->_multiInd == 2 && $value != null) {
			$v = preg_split("/\n+/", $value, -1, PREG_SPLIT_NO_EMPTY);
			foreach ($v as $vi) {
				$node->AddChild(new CNode($key, trim($vi)));
			}
		} elseif ($this->_type == 'checkboxOr') {
			$node->AddChild(new CNode($key, $value));
		} else {
			if ($this->_multiInd == 1 && $value != null) {
				$this->extractSplitMultiple($value);
			}
			$node->AddChild(new CNode($key, $value));
		}
		return true;
	}

	public function toHtml($pnode, $refUrl = null)
	{
		$node = ($pnode == null) ? null : $pnode->GetChildren($this->_key);
		$o = '';
		if (is_array($node)) {
			foreach ($node as $nd) {
				$o .= $this->toHtmlContent($nd, $refUrl);
				$o .= '<br>';
			}
		} else {
			$o .= $this->toHtmlContent($node, $refUrl);
		}
		return $o;
	}

	public function toInputGroup($pnode, $is_blocked, $helppop)
	{
		$node = ($pnode == null) ? null : $pnode->GetChildren($this->_key);
		$err = '';
		$value = '';

		if (is_array($node)) {
			$value = [];
			foreach ($node as $d) {
				$value[] = $d->Get(CNode::FLD_VAL);
				$e1 = $d->Get(CNode::FLD_ERR);
				if ($e1) {
					$err .= $e1 . '<br>';
				}
			}
		} else {
			if ($node) {
				$value = $node->Get(CNode::FLD_VAL);
				$err = $node->Get(CNode::FLD_ERR);
			} else {
				$value = null;
			}
		}

		$fieldId = 'lst-field-' . preg_replace('/[^A-Za-z0-9_-]+/', '-', $this->_key);

		$labelFor = $fieldId;
		if ($this->_inputType == 'radio' && $this->_type == 'bool') {
			$labelFor = $fieldId . '-1';
		} elseif ($this->_inputType == 'checkboxgroup') {
			$labelFor = null;
		}

		$buf = '<div class="lst-form-row lst-config-edit-row' . ($err ? ' lst-form-row--error">' : '">');
		if ($this->_label) {

			$buf .= '<label class="lst-form-label lst-config-edit-label"';
			if ($labelFor !== null) {
				$buf .= ' for="' . UIBase::EscapeAttr($labelFor) . '"';
			}
			$buf .= '>' . $this->_label;
			if ($this->IsFlagOn(DAttr::BM_NOTNULL)) {
				$buf .= ' *';
			}

			$buf .= "</label>\n";
			$buf .= '<div class="lst-form-field lst-config-edit-field">';
		} else {
			$buf .= '<div class="lst-form-field lst-form-field--full lst-config-edit-field lst-config-edit-field--full">';
		}


		$buf .= $this->toHtmlInput($helppop, $is_blocked, $err, $value, $fieldId);

		$buf .= "</div></div>\n";
		return $buf;
	}

	protected function toHtmlInput($helppop, $isDisabled, $err, $value, $fieldId)
	{
		$spacer = '&nbsp;&nbsp;&nbsp;&nbsp;';
		$checked = ' checked="checked"';

		$inputGroupClass = 'lst-input-group lst-config-input-group';
		$inputGroupAttr = '';
		if ($this->_inputType == 'password') {
			$inputGroupClass .= ' lst-password-field';
			$inputGroupAttr = ' data-lst-password-field="1"';
		}

		$input = '<div class="' . $inputGroupClass . '"' . $inputGroupAttr . '>';
		$input .= '<span class="lst-input-addon lst-config-help-addon">' . $helppop . '</span>' . "\n"; // need this even empty, for alignment

		if (is_array($value) && $this->_inputType != 'checkbox') {
			$glue = ($this->_multiInd == 1) ? ', ' : "\n";
			$value = implode($glue, $value);
		}
		$name = $this->_key;

		$inputAttr = $this->_inputAttr;
		if ($isDisabled) {
			$inputAttr .= ' disabled="disabled"';
		}
		$quoted_value = $value ? htmlspecialchars($value, ENT_QUOTES) : $value;

		$style = 'lst-choice-control';
		if ($this->_inputType == 'text') {
			$input .= '<input id="' . UIBase::EscapeAttr($fieldId) . '" class="' . $style . '" type="text" name="' . $name . '" ' . $inputAttr . ' value="' . $quoted_value . '">';
		} elseif ($this->_inputType == 'password') {
			$showPasswordLabel = UIBase::EscapeAttr(DMsg::ALbl('btn_showpassword'));
			$hidePasswordLabel = UIBase::EscapeAttr(DMsg::ALbl('btn_hidepassword'));
			$passwordToggleAttr = $isDisabled ? ' disabled="disabled"' : '';
			$input .= '<input id="' . UIBase::EscapeAttr($fieldId) . '" class="' . $style . '" data-lst-password-input="1" type="password" name="' . $name . '" ' . $inputAttr . ' value="' . $quoted_value . '">';
			$input .= '<button class="lst-input-action lst-password-toggle lst-config-password-toggle" type="button" rel="tooltip" data-placement="left" data-original-title="' . $showPasswordLabel . '" data-lst-password-toggle="1" data-lst-show-label="' . $showPasswordLabel . '" data-lst-hide-label="' . $hidePasswordLabel . '" aria-label="' . $showPasswordLabel . '" aria-pressed="false"' . $passwordToggleAttr . '>';
			$input .= '<i class="lst-icon" data-lucide="eye" data-lst-password-show-icon="1" aria-hidden="true"></i>';
			$input .= '<i class="lst-icon" data-lucide="eye-off" data-lst-password-hide-icon="1" aria-hidden="true" hidden></i>';
			$input .= '</button>';
		} elseif ($this->_inputType == 'textarea' || $this->_inputType == 'textarea1') {
			$input .= '<textarea id="' . UIBase::EscapeAttr($fieldId) . '" name="' . $name . '" class="' . $style . '" ' . $inputAttr . '>' . $quoted_value . '</textarea>';
		} elseif ($this->_inputType == 'radio' && $this->_type == 'bool') {

			$input .= '<div class="lst-choice-control lst-config-choice-control"><div class="lst-radio-group"><label class="lst-choice-item">';
			$input .= '<input id="' . UIBase::EscapeAttr($fieldId . '-1') . '" type="radio" name="' . $name . '" ' . $inputAttr . ' value="1"';
			if ($value == '1') {
				$input .= $checked;
			}
			$input .= '><span>' . ATTR_VAL_BOOL_YES . '</span></label><label class="lst-choice-item">'
					. '<input id="' . UIBase::EscapeAttr($fieldId . '-0') . '" type="radio" name="' . $name . '" ' . $inputAttr . ' value="0"';
			if ($value == '0') {
				$input .= $checked;
			}
			$input .= '><span>' . ATTR_VAL_BOOL_NO . '</span></label>';
			if (!$this->IsFlagOn(self::BM_NOTNULL)) {
				$input .= '<label class="lst-choice-item">';
				$input .= '<input id="' . UIBase::EscapeAttr($fieldId . '-unset') . '" type="radio" name="' . $name . '" ' . $inputAttr . ' value=""';
				if ($value != '0' && $value != '1') {
					$input .= $checked;
				}
				$input .= '><span>' . ATTR_VAL_NOT_SET . '</span></label>';
			}
			$input .= '</div></div>';
		} elseif ($this->_inputType == 'checkboxgroup') {
			$input .= '<div class="lst-choice-control lst-config-choice-control">';
			if ($this->_minVal !== null && ($value === '' || $value === null)) {
				// has default value, for "Not set", set default val
				$value = $this->_minVal;
			}
			$exclusiveGroupName = array_key_exists('0', $this->_maxVal) ? $name : null;
			foreach ($this->_maxVal as $val => $disp) {
				$id = $name . $val;
				$input .= "<label class=\"lst-choice-item\" for=\"{$id}\"><input type=\"checkbox\" id=\"{$id}\" name=\"{$id}\" value=\"{$val}\"";
				if (($value === $val) || ($value === '0' && $val === 0) || (intval($value) & intval($val)) > 0) {
					$input .= $checked;
				}
				if ($exclusiveGroupName !== null) {
					$input .= ' data-lst-exclusive-group="' . UIBase::EscapeAttr($exclusiveGroupName) . '"';
					$input .= ' data-lst-exclusive-role="' . (($val == '0') ? 'default' : 'member') . '"';
				}
				$input .= "><span>{$disp}</span></label>";
			}
			$input .= '</div>';
		} elseif ($this->_inputType == 'select') {
			$options = $this->_maxVal;
			if (is_array($options) && $this->IsFlagOn(self::BM_NOTNULL) && !array_key_exists('forcesel', $options)) {
				$options = ['forcesel' => '-- ' . DMsg::UIStr('note_select_option') . ' --'] + $options;
			}
			$input .= '<select id="' . UIBase::EscapeAttr($fieldId) . '" class="lst-choice-control" name="' . $name . '" ' . $inputAttr . '>';
			$input .= UIBase::genOptions($options, $value);
			$input .= '</select>';
		}

		$input .= "</div>\n";
		if ($err) {
			$input .= '<span class="lst-form-error lst-config-error"><i class="lst-icon" data-lucide="triangle-alert"></i> ';
			$type3 = substr($this->_type, 0, 3);
			$input .= ( $type3 == 'fil' || $type3 == 'pat' ) ? $err : htmlspecialchars($err, ENT_QUOTES);
			$input .= '</span>';
		}

		$note = $this->getNote();
		if ($note) {
			$input .= '<p class="lst-form-note lst-config-note">' . htmlspecialchars($note, ENT_QUOTES) . '</p>';
		}
		return $input;
	}

	public function SetDerivedSelOptions($derived)
	{
		$options = [];
		if ($this->IsFlagOn(self::BM_NOTNULL)) {
			$options['forcesel'] = '-- ' . DMsg::UIStr('note_select_option') . ' --';
		} else {
			$options[''] = '';
		}
		if ($derived) { // cannot use array_merge, we need to keep the same index in case of numeric key
            foreach ($derived as $k => $v) {
                $options[$k] = $v; 
            }
		}
		$this->_maxVal = $options;
	}

}
