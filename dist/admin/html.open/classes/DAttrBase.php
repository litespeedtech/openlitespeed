<?php

/*
 * type: parse  _minVal = pattern, _maxVal = pattern tips
 *
 */

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

	public function __construct($key, $type, $label, $inputType=NULL, $allowNull=true,
								$min=NULL, $max=NULL, $inputAttr=NULL, $multiInd=0, $helpKey=NULL)
	{
		$this->_key = $key;
		$this->_type = $type;
		$this->_label = $label;
		$this->_minVal = $min;
		$this->_maxVal = $max;
		$this->_inputType = $inputType;
		$this->_inputAttr = $inputAttr;
		$this->_multiInd = $multiInd;
		$this->_helpKey = ($helpKey == NULL)? $key:$helpKey;

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

	public function dup($key, $label, $helpkey)
	{
		$cname = get_class($this);
		$d = new $cname($this->_key, $this->_type, $this->_label, $this->_inputType, TRUE,
			$this->_minVal, $this->_maxVal, $this->_inputAttr, $this->_multiInd, $this->_helpKey);

		$d->_glue = $this->_glue;
		$d->_href = $this->_href;
		$d->_hrefLink = $this->_hrefLink;
		$d->_bitFlag = $this->_bitFlag;
		$d->_note = $this->_note;
		$d->_icon = $this->_icon;


		if ( $key != NULL )
		{
			$d->_key = $key;
		}
		if ($label != NULL)
			$d->_label = $label;

		if ($helpkey != NULL)
			$d->_helpKey = $helpkey;

		return $d;
	}

	protected function extractCheckBoxOr()
	{
		$value = 0;
		$novalue = 1;
		foreach( $this->_maxVal as $val => $disp )
		{
			$name = $this->_key . $val;
			if ( isset( $_POST[$name] ) )
			{
				$novalue = 0;
				$value = $value | $val;
			}
		}
		return ( $novalue ? '' : (string)$value );
	}

	protected function extractSplitMultiple(&$value)
	{
		if ( $this->_glue == ' ' )
			$vals = preg_split("/[,; ]+/", $value, -1, PREG_SPLIT_NO_EMPTY);
		else
			$vals = preg_split("/[,;]+/", $value, -1, PREG_SPLIT_NO_EMPTY);

		$vals1 = array();
		foreach( $vals as $val )
		{
			$val1 = trim($val);
			if ( strlen($val1) > 0 && !in_array($val1,$vals1)) {
				$vals1[] = $val1;
			}
		}

		if ( $this->_glue == ' ')
			$value = implode(' ', $vals1);
		else
			$value = implode(', ', $vals1);
	}

	protected function toHtmlContent($node, $refUrl=NULL)
	{
		if ( $node == NULL || !$node->HasVal())
			return '<span class="field_novalue">Not Set</span>';

		$o = '';
		$value = $node->Get(CNode::FLD_VAL);
		$err = $node->Get(CNode::FLD_ERR);

		if ( $this->_type == 'sel1' && $value != NULL && !array_key_exists($value, $this->_maxVal) ) {
		    $err = 'Invalid value - ' . htmlspecialchars($value,ENT_QUOTES);
		}
		else if ( $err != NULL ) {
			$type3 = substr($this->_type, 0, 3);
			if ( $type3 == 'fil' || $type3 == 'pat' ) {
				$validator = new ConfValidation();
				$validator->chkAttr_file_val($this, $value, $err);
			}
		}

		if ( $err ) {
			$node->SetErr($err);
			$o .= '<span class="field_error">*' . $this->check_split($err, 70) . '</span><br>';
		}

		if ( $this->_href ) {
			$link = $this->_hrefLink;
			if ( strpos($link, '$V') ) {
				$link = str_replace('$V', urlencode($value), $link);
			}
			$o .= '<span class="field_url"><a href="' . $link . '">';
		} elseif ( $refUrl != NULL ) {
			$o .= '<span class="field_refUrl"><a href="' . $refUrl . '">';
		}


		if ( $this->_type === 'bool' ) {
			if ( $value === '1' ) {
				$o .= 'Yes';
			}
			elseif ( $value === '0' ) {
				$o .= 'No';
			}
			else {
				$o .= '<span class="field_novalue">Not Set</span>';
			}
		}
		else if($this->_key == "note") {
			$o .= "<textarea readonly rows=4 cols=60 style='width:100%'>";
			$o .= htmlspecialchars($value,ENT_QUOTES);
			$o .= "</textarea>";
		}
		elseif ( $this->_type === 'sel' || $this->_type === 'sel1' ) {
			if ( $this->_maxVal != NULL && array_key_exists($value, $this->_maxVal) ) {
				$o .= $this->_maxVal[$value];
			}
			else {
			    $o .= htmlspecialchars($value,ENT_QUOTES);
			}
		}
		elseif ( $this->_type === 'checkboxOr' ) {
			if ($this->_minVal !== NULL && ($value === '' || $value === NULL) ) {
				// has default value, for "Not set", set default val
				$value = $this->_minVal;
			}
			foreach( $this->_maxVal as $val=>$name ) {
				if ( ($value & $val) || ($value === $val) || ($value === '0' && $val === 0) ) {
					$gif = 'checked.gif';
				}
				else {
					$gif = 'unchecked.gif';
				}
				$o .= '<img src="/static/images/graphics/'.$gif.'" width="12" height="12"> ';
				$o .= $name . '&nbsp;&nbsp;&nbsp;';
			}
		}
		elseif ( $this->_inputType === 'textarea1' ) {
		    $o .= '<textarea readonly '. $this->_inputAttr .'>' . htmlspecialchars($value,ENT_QUOTES) . '</textarea>';
		}
		elseif ( $this->_inputType === 'text' ) {
		    $o .= htmlspecialchars($this->check_split($value, 60), ENT_QUOTES);
		}
		elseif ( $this->_type == 'ctxseq' ) {
			$o = $value . '&nbsp;&nbsp;<a href=' . $this->_hrefLink . $value . '>&nbsp;+&nbsp;</a>' ;
			$o .= '/<a href=' . $this->_hrefLink . '-' . $value . '>&nbsp;-&nbsp;' ;
		}
		else {
			$o .= htmlspecialchars($value);
		}


		if ( $this->_href || $refUrl != NULL) {
			$o .= '</a></span>';
		}
		return $o;
	}

	protected function getNote()
	{
		if ( $this->_note != NULL )
			return $this->_note;
		if ( $this->_type == 'uint' )
		{
			if ( $this->_maxVal )
				return 'number valid range: '. $this->_minVal . ' - ' . $this->_maxVal;
			elseif ( $this->_minVal !== NULL )
				return 'number >= '. $this->_minVal ;
		}
		return NULL;
	}

	protected function check_split($value, $width)
	{
		if ( $value == NULL )
			return NULL;

		$changed = false;
		$val = explode(' ', $value);
		for( $i = 0 ; $i < count($val) ; ++$i )
		{
			if ( strlen($val[$i]) > $width )
			{
				$val[$i] = chunk_split($val[$i], $width, ' ');
				$changed = true;
			}
		}
		if ( $changed )
			return implode(' ', $val);
		else
			return $value;
	}

	public function extractPost($parent)
	{
		if ( $this->_type == 'checkboxOr' )
			$value = $this->extractCheckBoxOr();
		else {
			$value = GUIBase::GrabInput("post",$this->_key);
			if (get_magic_quotes_gpc())
				$value = stripslashes($value);
		}

		$key = $this->_key;
		$node = $parent;
		while (($pos = strpos($key, ':')) > 0 ) {
			$key0 = substr($key, 0, $pos);
			$key = substr($key, $pos+1);
			if ($node->HasDirectChildren($key0))
				$node = $node->GetChildren($key0);
			else {
				$child = new CNode($key0, '', CNode::T_KB);
				$node->AddChild($child);
				$node = $child;
			}
		}

		if ( $this->_multiInd == 2 && $value != NULL) {
			$v = preg_split("/\n+/", $value, -1, PREG_SPLIT_NO_EMPTY);
			foreach( $v as $vi )
				$node->AddChild(new CNode($key, trim($vi)));
		}
		elseif ( $this->_type == 'checkboxOr' ) {
			$node->AddChild(new CNode($key, $value));
		}
		else
		{
			if ( $this->_multiInd == 1 && $value != NULL) {
				$this->extractSplitMultiple( $value );
			}
			$node->AddChild(new CNode($key, $value));
		}
		return TRUE;
	}

	public function toHtml($pnode, $refUrl=NULL)
	{
		$node = ($pnode == NULL) ? NULL : $pnode->GetChildren($this->_key);
		$o = '';
		if ( is_array( $node ) ) {
			foreach ($node as $nd) {
				$o .= $this->toHtmlContent($nd, $refUrl);
				$o .= '<br>';
			}
		}
		else {
			$o .= $this->toHtmlContent($node, $refUrl);
		}
		return $o;
	}

	public function toHtmlInput($pnode, $seq=NULL, $isDisable=false)
	{
		$node = ($pnode == NULL) ? NULL : $pnode->GetChildren($this->_key);
		$err = '';
		$spacer = '&nbsp;&nbsp;&nbsp;&nbsp;';
		$checked = ' checked';

		if (is_array($node)) {
			$value = '';
			foreach( $node as $d ) {
				$value[] = $d->Get(CNode::FLD_VAL);
				$e1 = $this->check_split($d->Get(CNode::FLD_ERR), 70);
				if ( $e1 != NULL )
					$err .= $e1 .'<br>';
			}
		}
		else {
			if ($node != NULL) {
				$value = $node->Get(CNode::FLD_VAL);
				$err = $this->check_split($node->Get(CNode::FLD_ERR), 70);
			}
			else {
				$value = NULL;
			}
		}

		if ( is_array( $value ) && $this->_inputType != 'checkbox' )
		{
			if ( $this->_multiInd == 1 )
				$glue = ', ';
			else
				$glue = "\n";
			$value = implode( $glue, $value );
		}
		$name = $this->_key;
		if ( $seq != NULL )
			$name .= $seq;

		$inputAttr = $this->_inputAttr;
		if ( $isDisable )
			$inputAttr .= ' disabled';

		$input = '';
		$note = $this->getNote();
		if ( $note )
		    $input .= '<span class="field_note">'. htmlspecialchars($note,ENT_QUOTES) .'</span><br>';
		if ( $err != '' )
		{
			$input .= '<span class="field_error">*';
			$type3 = substr($this->_type, 0, 3);
			if ( $type3 == 'fil' || $type3 == 'pat' )
			{
				$input .= $err . '</span><br>';
			}
			else
				$input .= htmlspecialchars($err,ENT_QUOTES) . '</span><br>';
		}

		$style = 'xtbl_value';
		if ( $this->_inputType == 'text' )	{
			$input .= '<input class="' . $style . '" type="text" name="'. $name .'" '. $inputAttr.' value="' .htmlspecialchars($value,ENT_QUOTES). '">';
		}
		elseif ( $this->_inputType == 'password' )	{
			$input .= '<input class="' . $style . '" type="password" name="' . $name . '" ' . $inputAttr .' value="' .$value. '">';
		}
		elseif ( $this->_inputType == 'textarea' || $this->_inputType == 'textarea1' ) {
		    $input .= '<textarea name="'.$name.'" '.$inputAttr.'>'. htmlspecialchars($value,ENT_QUOTES). '</textarea>';
		}
		elseif ( $this->_inputType == 'radio' && $this->_type == 'bool') {
			$input .= "<input type=\"radio\" id=\"{$name}1\" name=\"{$name}\" $inputAttr value=\"1\"";
			if ( $value == '1' )
				$input .= $checked;
			$input .= "><label for=\"{$name}1\"> Yes </label>$spacer
				<input type=\"radio\" id=\"{$name}0\" name=\"{$name}\" $inputAttr value=\"0\"";
			if ( $value == '0' )
				$input .= $checked;
			$input .= "><label for=\"{$name}0\"> No </label>";
			if ( !$this->IsFlagOn(self::BM_NOTNULL) ) {
				$input .= "$spacer <input type=\"radio\" id=\"{$name}\" name=\"{$name}\" $inputAttr value=\"\"";
				if ( $value != '0' && $value != '1' )
					$input .= $checked;
				$input .= "><label for=\"{$name}\"> Not Set </label>";
			}
		}
		elseif ( $this->_inputType == 'checkbox' )	{
			$id = $name . $value['val'];
			$input .= '<input type="checkbox" id="'.$id.'" name="'.$name.'" '.$inputAttr.' value="'.$value['val'].'"';
			if ( $value['chk'] )
				$input .= $checked;
			$input .= '><label for="'.$id.'"> ' . $value['val'] . ' </label>';
		}
		elseif ( $this->_inputType == 'checkboxgroup' ) {
			if ($this->_minVal !== NULL && ($value === '' || $value === NULL) ) {
				// has default value, for "Not set", set default val
				$value = $this->_minVal;
			}
			$js0 = $js1 = '';
			if (array_key_exists('0', $this->_maxVal)) {
				$chval = array_keys($this->_maxVal);
				foreach ($chval as $chv) {
					if ($chv == '0')
						$js1 = "document.confform.$name$chv.checked=false;";
					else
						$js0 .= "document.confform.$name$chv.checked=false;";
				}
				$js1 = " onclick=\"$js1\"";
				$js0 = " onclick=\"$js0\"";
			}
			foreach( $this->_maxVal as $val=>$disp )
			{
				$id = $name . $val;
				$input .= "<input type=\"checkbox\" id=\"{$id}\" name=\"{$id}\" value=\"{$val}\"";
				if ( ($value & $val) || ($value === $val) || ($value === '0' && $val === 0) )
					$input .= $checked;
				$input .= ($val == '0') ? $js0 : $js1;
				$input .= "><label for=\"{$id}\"> $disp </label> $spacer";
			}
		}
		elseif ( $this->_inputType == 'select' ) {
			$input .= '<select name="'.$name.'" '.$inputAttr.'>';
			$input .= GUIBase::genOptions($this->_maxVal, $value);
			$input .= '</select>';
		}
		return $input;

	}

	public function SetDerivedSelOptions($derived)
	{
		$options = array();
		if ( !$this->IsFlagOn(self::BM_NOTNULL) )
			$options[''] = '';
		if ($derived != NULL)
			$options = array_merge($options, $derived);
		$this->_maxVal = $options;
	}


}
