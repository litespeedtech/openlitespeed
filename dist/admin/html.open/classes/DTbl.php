<?php

class DTbl
{
	public $_id;
	public $_title;
	public $_dattrs;
	public $_helpKey;

	public $_width = "100%";
	public $_align;
	public $_isMulti;//0:regular, 1:multi-top, 2:multi-detail
	public $_icon;
	public $_addTbl;
	public $_cols;
	public $_hasB = false;

	public $_layerId = NULL;
	public $_dataLoc = NULL;

	public	$_holderIndex = NULL;

	public $_subTbls = NULL;
	public $_defaultExtract = NULL;

	public $_customizedScript;

	public function __construct($id, $title=NULL, $isMulti=0, $addTbl=NULL, $align=0, $icon = NULL)
    {
		$this->_id = $id;
		$this->_title = $title;
		$this->_isMulti = $isMulti;
		$this->_addTbl = $addTbl;
		$this->_align = $align;
		$this->_icon = $icon;
	}
	
	public function dup($newId, $title=NULL)
	{
		$d = new DTbl($newId, $this->_title, $this->_isMulti, $this->_addTbl, $this->_align, $this->_icon);
		$d->_dattrs = $this->_dattrs;
		$d->_helpKey = $this->_helpKey;
		$d->_width = $this->_width;
		$d->_cols = $this->_cols;
		$d->_hasB = $this->_hasB;
		$d->_layerId = $this->_layerId;
		$d->_dataLoc = $this->_dataLoc;
		$d->_holderIndex = $this->_holderIndex;
		$d->_subTbls = $this->_subTbls;
		$d->_defaultExtract = $this->_defaultExtract;
		$d->_customizedScript = $this->_customizedScript;
		
		if ($title != NULL) {
			$d->_title = $title;
		}
		
		return $d;
	}

	public function setRepeated( $holderIndex )
	{
		$this->_holderIndex = $holderIndex;
	}

	public function setAttr(&$attrs, $dataLoc=NULL, $layerId=NULL)
	{
		$this->_dattrs = &$attrs;
		$this->_layerId = $layerId;
		$this->_dataLoc = $dataLoc;
		if ( $this->_isMulti == 1 ) {
			$this->_cols = count($attrs);
			if($this->_icon != NULL) {
				$this->_cols++;
			}
		}
		else {
			$this->_cols = 2;
		}
	}
	
	public function setDataLoc($dataLoc, $layerId=NULL)
	{
		$this->_dataLoc = $dataLoc;
		if ($layerId != NULL)
			$this->_layerId = $layerId;	
	}

	public function GetTags()
	{
		$tags = array();
		$keys = array_keys($this->_dattrs);
		foreach ( $keys as $key )
		{
			$attr = $this->_dattrs[$key];
			$pos = strpos($attr->_key, ':');
			if ( $pos === false )
				$tags[$attr->_key] = $attr->_multiInd;
			else
			{
				$layer = substr($attr->_key, 0, $pos);
				$id = substr($attr->_key, $pos+1);
				$tags[$layer][$id] = $attr->_multiInd;
			}
		}
		return $tags;
	}


	public function getActionLink($disp, $actions, $editTid='', $editRef='')
	{
		$buf = '';
		$allActions = array('a'=>'Add', 'v'=>'View', 'e'=>'Edit',
							'E'=>'Edit', 'd'=>'Delete', 's'=>'Save',
							'b'=>'Back', 'B'=>'Back', 'n'=>'Next',
							'D'=>'Yes', 'c'=>'Cancel', 'C'=>'Cancel',
							'i'=>'Instantiate', 'I'=>'Yes',
							'X' => 'View/Edit');
		$chars = preg_split('//', $actions, -1, PREG_SPLIT_NO_EMPTY);
		foreach ( $chars as $act )
		{
			$ctrlUrl = '<a href="' . $disp->_ctrlUrl . 'm=' . $disp->_mid . '&p=' . $disp->_pid;
			$t = '';
			$r = '';
			$buf .= '&nbsp;&nbsp;&nbsp;';
			$name = $allActions[$act];
			if ( $act == 'b' || $act == 'c' ) {
				$act = 'b';
			}
			elseif ( $act == 'e' ) {
				$t = '&t=' . $this->_id;
			}
			elseif ( $act == 'a' ) {
				$t = '&t=' . $disp->_tid . '`' . $this->_addTbl;
				if ( $disp->_ref != NULL ) {
					$r = '&r=' . $disp->_ref . '`';
				}
			}
			elseif ( strpos('vEdDCBiI', $act) !== false ) {
				if ( $act == 'C' ) {
					$act = 'B';
				}
				
				if ( isset($this->_hasB) && $this->_hasB && $editTid != '' ) {
					$t = '&t=' . $disp->_tid . '`' . $editTid;
					$r = '&r=' . $disp->_ref . '`' . urlencode($editRef);
				}
				else {
					if ( $editTid == '' ) {
						$editTid = $disp->_tid;
					}
					if ( $editRef == '' ) {
						$editRef = $disp->_ref;
					}

					$t = '&t=' . $editTid;
					$r = '&r=' . urlencode($editRef);
				}
			}

			if ( $act == 's' ) {
				$buf .= '<a href="javascript:document.confform.submit()">Save</a>';
			}
			elseif ( $act == 'n' ) {
				$buf .= '<a href="javascript:document.confform.a.value=\'n\';document.confform.submit()">Next</a>';
			}
			elseif( $act == 'X') {
				//vhtop=>vh_... tptop=>tp_.... sltop=>sl_...
				$buf .= '<a href="' . $disp->_ctrlUrl . 'm=' . substr($disp->_type, 0, 2) . '_' . $editRef . "\">{$name}</a>";
			}
			else {
				$buf .= $ctrlUrl . $t . $r . '&a=' . $act . '&tk=' . $disp->_token . '">' . $name . '</a>';
			}
		}
		return $buf;
	}

	public function PrintHtml(&$data, $ref, $disp)
	{
		//$hasB = strpos($_SESSION['t'], '`') || $this->_hasB;
		$viewTags = 'vbsDdBCiI';
		$editTags = 'eEaScn';
		if ( strpos($viewTags, $disp->_act) !== false )
		{
			$this->printViewHtml($data, $ref, $disp);
		}
		else if ( strpos($editTags, $disp->_act) !== false )
		{
			$this->printEditHtml($data, $disp);
		}
	}

	private function printHeader($disp, $actString)
	{
		$colspan = ($this->_cols == 1) ? 1 : $this->_cols + 1;
		$buf = '<tr><td class=xtbl_header colspan="' . $colspan . '">';

		// tooltip
		$table_help = ' ';
		
		$dhelp_item = DATTR_HELP::GetInstance()->GetItem($this->_helpKey);
		if($dhelp_item != NULL) {
			$table_help = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' . $dhelp_item->render($this->_helpKey);
		}
		else if (count($this->_dattrs) == 1) {
			$av = array_values($this->_dattrs);
			$a0 = $av[0];
			$dhelp_item = DATTR_HELP::GetInstance()->GetItem($a0->_helpKey);
			if($dhelp_item != NULL) {
				$is_blocked = $a0->blockedVersion();
				$version = $is_blocked? $a0->_version: 0;
				$table_help = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' . $dhelp_item->render($a0->_helpKey, $version);
			}
			
		}
		
		if ( $actString != NULL )
		{
			$titleActionLink = $this->getActionLink($disp, $actString);

			$buf .= "\n".'<table width="100%" border="0" cellpadding="0" cellspacing="0"><tr>'."\n";
			$buf .= '<td class="xtbl_title">'. $this->_title . $table_help . '</td>';
			$buf .= '<td class="xtbl_edit">'.$titleActionLink.'</td>';
			$buf .= "\n</tr></table>\n";
		}
		else
			$buf .= $this->_title . $table_help;

		$buf .= "</td></tr>\n";

		if ( $this->_isMulti == 1 )
		{
			$buf .= '<tr class="xtbl_title2">';
			$keys = array_keys($this->_dattrs);
			$hasSort = ( $disp->_sort != NULL && $disp->_sort[0] == $this->_id );
			$a = '1'; //ascend
			$url = $disp->_ctrlUrl . 'm=' . $disp->_mid . '&p=' . $disp->_pid;
			if ( $disp->_tid != NULL )
				$url .= '&t=' . $disp->_tid;
			if ( $disp->_ref != NULL )
				$url .= '&r=' . $disp->_ref;

			if($this->_icon != NULL) {
				$buf .= "<td>&nbsp;</td>";
				$buf .= "<td>&nbsp;</td>";
			}


			foreach( $keys as $i )
			{
				$attr = $this->_dattrs[$i];
				$buf .= '<td>' . $attr->_label;
				if ( $attr->_type != 'action' )
				{
					$buf .= ' <a href="' . $url . '&sort=' . $this->_id . '`';
					if ( $hasSort && array_key_exists(2,$disp->_sort) && array_key_exists(1,$disp->_sort) && $disp->_sort[2] == $attr->_key && $disp->_sort[1] == '1' )
						$a = '0';
					else
						$a = '1';

					$buf .= $a . $attr->_key . '">';
					$buf .= ($a=='1')? '<img src="/static/images/icons/up.gif" border=0 width=13 align=absmiddle>' : '<img src="/static/images/icons/down.gif" width=13 align=absmiddle border=0>';
					$buf .= '</a>';
				}
				if ( $attr->_type == 'ctxseq' )
				{
					$attr->_hrefLink = $url . $attr->_href;
				}
				$buf .= '</td>';
			}
			$buf .= "</tr>\n";
		}



		echo $buf;
	}


	private function printViewHtml(&$data, $ref, $disp)
	{
		//echo  "\n". '<table align="center" width=100% class="gt" border="0" cellpadding="5" cellspacing="1">' . "\n";


		echo "\n". '<table width="'.$this->_width.'" class=xtbl border="0" cellpadding="5" cellspacing="1">' . "\n";


		if ( $this->_isMulti == 1)
		{
			if ( $this->_addTbl == NULL )
				$actString = 'e';
			else if ( $this->_addTbl != 'N' )
				$actString = 'a';
			else
				$actString = '';

			if ( $this->_hasB )
				$actString .= 'B';

			$this->printHeader($disp, $actString);

			if ( $data != NULL && count($data) > 0 )
			{
				$keys = array_keys($data);
				if ( $disp->_sort[0] == $this->_id )
				{
					if ( array_key_exists(2,$disp->_sort) &&  $this->_holderIndex == $disp->_sort[2] )
					{
						if ( $disp->_sort[1] == 1 )
							sort($keys);
						else
							rsort($keys);
					}
					else
					{

						//default context view sort
						if(!array_key_exists(1,$disp->_sort) &&  $disp->_pid == "context" ) {
							$disp->_sort[2] = "order";
							$disp->_sort[1] = 0; //asc
						}

						$s = array();

						if(array_key_exists(2,$disp->_sort)){
							$by = $disp->_sort[2];
						}
						else {
							$by = NULL;
						}

						foreach( $keys as $key )
						{
							$d = &$data[$key];

							if(array_key_exists($by, $d)) {
								$a = $d[$by];
								$a = $a->GetVal();
							}
							else {
								$a = NULL;
							}

							$s[$key] = $a;
						}



						if ( array_key_exists(1,$disp->_sort) && $disp->_sort[1] == 1 )
						{
							if ( $by == 'order' )
								asort($s, SORT_NUMERIC);
							else
								asort($s, SORT_STRING);
						}
						else
						{
							if ( $by == 'order' )
								arsort($s, SORT_NUMERIC);
							else
								arsort($s, SORT_STRING);
						}
						$keys = array_keys($s);
					}
				}

				$action_attr = NULL;
				foreach ($this->_dattrs as $attr) {
					if ($attr->_type == 'action') {
						$action_attr = $attr;
						break;
					}
				}
				foreach ( $keys as $key ) {
					$this->printHtmlLine1($data[$key], $key, $disp, $action_attr);
				}

			}
		}
		else
		{
			if ( $ref == NULL )
			{
				$actString = 'e';
				if ( $this->_hasB )
					$actString .= 'B';
			}
			else
			{
				$actString = 'Ed';
				$actString .= ($this->_hasB ? 'B' : 'b');
			}
			$this->printHeader($disp, $actString);

			if ( isset($this->_customizedScript)
				 && $this->_customizedScript[0] != NULL )
			{
				$customizedData = $data;
				$customizedInfo = $disp->_info;
				include($this->_customizedScript[0]);
			}
			else
			{

				$keys = array_keys($this->_dattrs);
				foreach( $keys as $i )
				{
					$this->printHtmlLine($data, $disp, $i);
				}

			}
		}

		echo "\n</table>\n";

	}

	private function printTips($tips)
	{
		$buf = '<table border=0 cellspacing=0 cellpadding=0 id=tips><tr><td><ul type=circle>';
		foreach( $tips as $tip )
		{
			if(strlen($tip)) {
				$buf .= '<li>' . $tip .  "\n";
			}
		}
		$buf .= '</ul></td></tr></table>';
		echo $buf;
	}

	private function printEditHtml(&$data, $disp)
	{

		$tips = DTblTips::getTips($this->_id);

		if ( $tips != NULL ) {
			$this->printTips($tips);
		}

		echo "\n". '<table width="'.$this->_width.'" class=xtbl border="0" cellpadding="5" cellspacing="1">' . "\n";


		$actString = ( (substr($this->_id, -3) == 'SEL')?'n':'s' ) .
			( $this->_hasB ? 'B':'b');
		$this->printHeader($disp, $actString);

		if ( isset($this->_customizedScript)
			 && $this->_customizedScript[1] != NULL )
		{
			$customizedData = $data;
			$customizedInfo = $disp->_info;
			include($this->_customizedScript[1]);
		}
		else
		{
			$keys = array_keys($this->_dattrs);
			foreach( $keys as $key )
			{
				$this->printHtmlInputLine($data, $disp->_info, $key);
			}
		}

		echo "\n</table>\n";

	}

	private function printHtmlLine(&$data, $disp, $index)
	{
		$valwid = 0;
		$attr = $this->_dattrs[$index];
		if ( $attr == NULL || $attr->_FDE[1] == 'N') {
			return;
		}

		$is_blocked = $attr->blockedVersion();
		$version = $is_blocked? $attr->_version: 0;
		if ( $attr->_type == 'sel1' ) {
			$this->populate_sel1_options($disp->_info, $data, $attr);
		}

		$buf = '<tr class="xtbl_value">';
		if ( $attr->_label ) {
	
			if ($is_blocked) {
				$buf .= '<td class="xtbl_label_blocked">';
			}
			else {
				$buf .= '<td class="xtbl_label">';
			}
			$buf .= $attr->_label;

			if ($this->_cols == 1) {
				$buf .= '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
			}
			else {
				$buf .= '</td><td class=icon>';
			}

			//add tooltip
			$dhelp_item = DATTR_HELP::GetInstance()->GetItem($attr->_helpKey);
			if($dhelp_item != NULL) {
				$buf .= $dhelp_item->render($attr->_helpKey, $version);
			}
			else {
				$buf .= ' ';
			}
			
			$buf .= '</td>';
		}

		if ($this->_cols == 1) {
			$buf .= '</tr><tr class="xtbl_value"><td ';			
		}
		else {
			$buf .= '<td ';
		}
		if ($attr->blockedVersion()) {
			$buf .= 'class="xtbl_value_blocked" ';
		}
		if ($valwid > 0) {
			$buf .= "width=$valwid";
		}
		$buf .= '>';
		
	
		if ( $attr->_href )
		{
			$link = $disp->_ctrlUrl . 'm=' . $disp->_mid . '&p=' . $disp->_pid;
			if ( $disp->_tid != NULL )
				$link .= '&t=' . $disp->_tid;
			if ( $disp->_ref != NULL )
				$link .= '&r='. $disp->_ref;

			$link .= $attr->_href;
			$attr->_hrefLink = str_replace('$R', $disp->_ref, $link);
		}
		
		$buf .= ($attr->toHtml($data[$attr->_key]));
		
		
		$buf .= "</td></tr>\n";
		echo $buf;
	}


	private function printHtmlInputLine(&$data, &$info, $key)
	{
		$attr = $this->_dattrs[$key];
		if ( $attr == NULL || $attr->_FDE[2] == 'N')
			return;

		$valwid = 0;
		$is_blocked = $attr->blockedVersion();
		$version = $is_blocked? $attr->_version: 0;
		if ( $attr->_type == 'sel1' ) {
			$this->populate_sel1_options($info, $data, $attr);
		}

		$buf = '<tr class="xtbl_value">';
		if ( $attr->_label ) {
	
			if ($is_blocked) {
				$buf .= '<td class="xtbl_label_blocked">';
			}
			else {
				$buf .= '<td class="xtbl_label">';
			}
			$buf .= $attr->_label;

			if ($this->_cols == 1) {
				$buf .= '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
			}
			else {
				$buf .= '</td><td class=icon>';
			}

			//add tooltip
			$dhelp_item = DATTR_HELP::GetInstance()->GetItem($attr->_helpKey);
			if($dhelp_item != NULL) {
				$buf .= $dhelp_item->render($attr->_helpKey, $version);
			}
			else {
				$buf .= ' ';
			}
			
			$buf .= '</td>';
		}

		if ($this->_cols == 1) {
			$buf .= '</tr><tr class="xtbl_value"><td ';			
		}
		else {
			$buf .= '<td ';
		}
		if ($attr->blockedVersion()) {
			$buf .= 'class="xtbl_value_blocked" ';
		}
		if ($valwid > 0) {
			$buf .= "width=$valwid";
		}
		$buf .= '>';
		
		if ($is_blocked) {
			$buf .= ($attr->toHtml($data[$attr->_key]));
		}
		else {
			$buf .= ($attr->toHtmlInput($data[$attr->_key]));
		}
		
		
		$buf .= "</td></tr>\n";
		echo $buf;
		
	}

	private function printHtmlLine1(&$data, $key0, $disp, $action_attr)
	{
		static $htmlid;

		if(isset($htmlid)) {
			$htmlid++;
		}
		else {
			$htmlid = 1;
		}

		$buf = '<tr class="xtbl_value">';

		$keys = array_keys($this->_dattrs);
		
		//allow index field clickable, same as first action
		$actionLink = null;
		$indexActionLink = null;
		
		if ($action_attr != NULL) {
			
			if ( is_array($action_attr->_minVal) )
			{
				$index = $action_attr->_minVal[0];
				$ti = $action_attr->_minVal[$data[$index]->GetVal()];
				if ( $ti == NULL )
					$ti = $action_attr->_minVal[1];
			}
			else
				$ti = $action_attr->_minVal;

			$actionLink = $this->getActionLink($disp, $action_attr->_maxVal, $ti, $key0);
			
			$tmp_a = strpos($actionLink, '"') +1;
			$tmp_e = strpos($actionLink, '">');
			$indexActionLink = substr($actionLink, $tmp_a, $tmp_e - $tmp_a);
		}
		
		foreach( $keys as $key )
		{
			$attr = $this->_dattrs[$key];

			if($key == 0 && $this->_icon != NULL) {
				if($attr->_key == "type" &&  is_array($attr->_maxVal) && is_array($this->_icon)) {

					if(array_key_exists($data['type']->GetVal(), $this->_icon)) {
						$icon_name = $this->_icon[$data['type']->GetVal()];
					}
					else {
						$icon_name = "application";
					}

					$buf .= "<td class=icon><img src='/static/images/icons/". $icon_name . ".gif'></td>";
				}
				else {
					$buf .= "<td class=icon><img src='/static/images/icons/{$this->_icon}.gif'></td>";
				}

				if(isset($data['note']) && $data['note']->HasVal()) {
					$buf .= "<td class=icon width=10 align=center><img class='xtip-hover-info{$htmlid}' src='/static/images/icons/info.gif'>";
					$buf .= '<div id="xtip-note-info' . $htmlid . '" class="snp-mouseoffset notedefault">' . $data['note']->GetVal() 
						. '</div></td>';
				}
				else {
					$buf .= "<td>&nbsp;</td>";
				}
			}

			$buf .= '<td';
			if ( isset($this->_align[$key]) )
				$buf .= ' align="'.$this->_align[$key] .'"';
			$buf .= '>';

			if ( $attr->_type == 'action' )	{
				$buf .= ($attr->toHtml($actionLink));
			}
			else {
				if ( $attr->_type == 'sel1' ) {
					$this->populate_sel1_options($disp->_info, $data, $attr);
				}
				if ($attr->_key == $this->_holderIndex) {
					$buf .= ($attr->toHtml($data[$attr->_key], $indexActionLink));
				}
				else {
					$buf .= ($attr->toHtml($data[$attr->_key]));
				}
			}
			$buf .= '</td>';
		}
		$buf .= '</tr>'."\n";

		echo $buf;
	}

	public function populate_sel1_options($info, &$data, $attr)
	{
		$options = array();
		if ( $attr->_allowNull ) {
			$options[''] = '';
		}
		foreach( $attr->_minVal as $loc ) 
		{
			$d = $info;
			$locs = explode(':', $loc);
			foreach ( $locs as $l )
			{
				if ( substr($l, 0, 2) == '$$' )
				{ //$$type!fcgi
					$t = strpos($l, '!');
					$tag = substr($l, 2, $t-2);
					$tag0 = substr($l, $t+1);
					if (isset($data[$tag]) && $data[$tag]->HasVal()) {
						$l = $data[$tag]->GetVal();
					}
					else {
						$l = $tag0;
					}
				}

				$d = &$d[$l]; //here everything breaks
			}
			if ( isset($d) ) {
				$options = $options + $d;
			}
		}
		$attr->_maxVal = $options;
	}


}
