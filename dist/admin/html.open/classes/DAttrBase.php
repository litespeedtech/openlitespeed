<?php

class DAttrBase
{
	public $_htmlName;
	public $_key;
	public $_helpKey;
	public $_type;
	public $_minVal;
	public $_maxVal;
	public $_inputType;
	public $_inputAttr;
	public $_label;
	public $_allowNull;
	public $_glue;
	public $_href;
	public $_hrefLink;
	public $_multiInd;
	public $_FDE = 'YYY'; //File:Display:Editable
	public $_note;
	public $_icon;
	public $_linkedkeys;

	public function __construct($key, $type, $label, $inputType=NULL, $allowNull=true,
								$min=NULL, $max=NULL, $inputAttr=NULL, $multiInd=0, $helpKey=NULL)
	{
		$this->_htmlName = $key;
		$this->_key = $key;
		$this->_type = $type;
		$this->_label = $label;
		$this->_minVal = $min;
		$this->_maxVal = $max;
		$this->_inputType = $inputType;
		$this->_inputAttr = $inputAttr;
		$this->_allowNull = $allowNull;
		$this->_multiInd = $multiInd;
		$this->_helpKey = ($helpKey == NULL)? $key:$helpKey;
	}

	public function dup($key, $label, $helpkey)
	{
		$cname = get_class($this);
		$d = new $cname($this->_key, $this->_type, $this->_label, $this->_inputType, $this->_allowNull,
			$this->_minVal, $this->_maxVal, $this->_inputAttr, $this->_multiInd, $this->_helpKey);

		$d->_htmlName = $this->_htmlName;
		$d->_glue = $this->_glue;
		$d->_href = $this->_href;
		$d->_hrefLink = $this->_hrefLink;
		$d->_FDE = $this->_FDE;
		$d->_note = $this->_note;
		$d->_icon = $this->_icon;
		$d->_linkedkeys = $this->_linkedkeys;


		if ( $key != NULL )
		{
			$d->_htmlName = $key;
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
		return ( $novalue ? '' : $value );
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

	protected function toHtmlContent($cval, $refUrl=NULL)
	{
		$o = '';
		if ( $cval == NULL || !$cval->HasVal()) {
			$o .= '<span class="field_novalue">Not Set</span>';
			return $o;
		}

		$value = $cval->GetVal();
		$err = $cval->GetErr();

		if ( $this->_type == 'sel1' && $value != NULL && !array_key_exists($value, $this->_maxVal) ) {
		    $err = 'Invalid value - ' . htmlspecialchars($value,ENT_QUOTES);
		}
		else if ( $err != NULL ) {
			$type3 = substr($this->_type, 0, 3);
			if ( $type3 == 'fil' || $type3 == 'pat' ) {
				$validator = new ConfValidation();
				$validator->chkAttr_file_val($this, $value, $err);
				error_log('revalidate path ' . $value);
			}
		}

		if ( $err ) {
			$cval->SetErr($err);
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
		elseif ( $this->_type == 'action' ) {
			$o .= $value;
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
		return null;
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


	protected function genOptions($options, $selValue)
	{
		$o = '';
		if ( $options )
		{
			foreach ( $options as $key => $value )
			{
				$o .= '<option value="' . $key .'"';
				if ( $key == $selValue ) {
					if (!($selValue === '' && $key === 0)
						&& !($selValue === NULL && $key === 0)
						&& !($selValue === '0' && $key === '')
						&& !($selValue === 0 && $key === ''))
						$o .= ' selected';
				}
				$o .= ">$value</option>\n";
			}
		}
		return $o;
	}

	public function extractPost()
	{
		$cval = NULL;
		$postVal = DUtil::grab_input("post",$this->_htmlName);
		if (get_magic_quotes_gpc()) {
			$postVal = stripslashes($postVal);
		}
		if ( $this->_multiInd == 2 )
		{
			$cval = array();
			$v = preg_split("/\n+/", $postVal, -1, PREG_SPLIT_NO_EMPTY);
			foreach( $v as $vi )
			{
				$cval[] = new CVal(trim($vi));
			}
		}
		else if ( $this->_type === 'checkboxOr' )
		{
			$value = $this->extractCheckBoxOr();
			$cval = new CVal($value);
		}
		else
		{
			$value = trim($postVal);
			if ( $this->_multiInd == 1 ) {
				$this->extractSplitMultiple( $value );
			}
			$cval = new CVal($value);
		}
		return $cval;
	}

	public function toHtml(&$data, $refUrl=NULL, $linkedData=NULL)
	{
		$o = '';
		if ( $this->_type == 'action' )
		{
			$a = '';
			$o .= $this->toHtmlContent(new CVal($data));
		}
		else if ( is_array( $data ) )
		{
			for ( $i = 0 ; $i < count($data) ; ++$i )
			{
				$o .= $this->toHtmlContent($data[$i], $refUrl);
				$o .= '<br>';
			}
		}
		else
		{
			$o .= $this->toHtmlContent($data, $refUrl);
		}
		return $o;
	}

	public function toHtmlInput(&$data, $seq=NULL, $isDisable=false)
	{
		$err = '';
		if ( is_array($data) )
		{
			$value = '';
			foreach( $data as $d )
			{
				$value[] = $d->GetVal();
				$e1 = $this->check_split($d->GetErr(), 70);
				if ( $e1 != NULL )
					$err .= $e1 .'<br>';
			}
		}
		else
		{
			if($data != NULL) {
				$value = $data->GetVal();
				$err = $this->check_split($data->GetErr(), 70);
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
		$name = $this->_htmlName;
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
		if ( $this->_inputType === 'text' )
		{
			$input .= '<input class="' . $style . '" type="text" name="'.$this->_htmlName.'" '. $inputAttr.' value="' .htmlspecialchars($value,ENT_QUOTES). '">';
			return $input;
		}
		if ( $this->_inputType === 'password' )
		{
			$input .= '<input class="' . $style . '" type="password" name="'.$this->_htmlName.'" '.$inputAttr.' value="' .$value. '">';
			return $input;
		}
		if ( $this->_inputType === 'textarea' || $this->_inputType === 'textarea1' )
		{
		    $input .= '<textarea name="'.$name.'" '.$inputAttr.'>'. htmlspecialchars($value,ENT_QUOTES). '</textarea>';
			return $input;

		}
		if ( $this->_inputType === 'radio' && $this->_type === 'bool')
		{
			$input .= '<input type="radio" id="'.$name.'1" name="'.$name.'" '.$inputAttr.' value="1" ';
			if ( $value == '1' )
				$input .= 'checked';
			$input .= '><label for="'.$name.'1"> Yes </label>&nbsp;&nbsp;&nbsp;&nbsp;';
			$input .= '<input type="radio" id="'.$name.'0" name="'.$name.'" '.$inputAttr.' value="0" ';
			if ( $value == '0' )
				$input .= 'checked';
			$input .= '><label for="'.$name.'0"> No </label>';
			if ( $this->_allowNull )
			{
				$input .= '&nbsp;&nbsp;&nbsp;&nbsp;<input type="radio" id="'.$name.'" name="'.$name.'" '.$inputAttr.' value="" ';
				if ( $value != '0' && $value != '1' )
					$input .= 'checked';
				$input .= '><label for="'.$name.'"> Not Set </label>';
			}
			return $input;
		}

		if ( $this->_inputType === 'checkbox' )
		{
			$id = $name . $value['val'];
			$input .= '<input type="checkbox" id="'.$id.'" name="'.$name.'" '.$inputAttr.' value="'.$value['val'].'"';
			if ( $value['chk'] )
				$input .= ' checked';
			$input .= '><label for="'.$id.'"> ' . $value['val'] . ' </label>';
			return $input;
		}

		if ( $this->_inputType === 'checkboxgroup' )
		{
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
				$id = $name.$val;
				$input .= '<input type="checkbox" id="'.$id.'" name="'.$id.'" value="'.$val.'"';
				if ( ($value & $val) || ($value === $val) || ($value === '0' && $val === 0) )
					$input .= ' checked';
				$input .= ($val == '0') ? $js0 : $js1;
				$input .= '><label for="'.$id.'"> ' . $disp . ' </label>&nbsp;&nbsp;';
			}
			return $input;
		}

		if ( $this->_inputType === 'select' )
		{
			$input .= '<select name="'.$name.'" '.$inputAttr.'>';
			$input .= ($this->genOptions($this->_maxVal, $value));
			$input .= '</select>';
			return $input;
		}


	}

	public function populate_sel1_options($info, &$data)
	{
		$options = array();
		if ( $this->_allowNull ) {
			$options[''] = '';
		}

		foreach( $this->_minVal as $loc )
		{
			$d = $info;
			$locs = explode(':', $loc);
			foreach ( $locs as $l )
			{
				if ( substr($l, 0, 2) == '$$' )
				{ //$$type!fcgi
					$t = strpos($l, '!');
					$tag = substr($l, 2, $t-2);
					$tag0 = substr($l, $t+1); // default
					if (isset($data[$tag]) && $data[$tag]->HasVal()) {
						$l = $data[$tag]->GetVal();
					}
					else {
						$l = $tag0;
					}
				}

				$d = $d[$l];
			}
			if ( isset($d) ) {
				$options = $options + $d; // this is array add
			}
		}
		$this->_maxVal = $options;
	}



}
