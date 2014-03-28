<?php

class DTbl
{
	public $_id;
	public $_title;
	public $_dattrs;
	public $_helpKey;

	public $_width = '100%';
	public $_align;
	public $_isMulti;//0:regular, 1:multi-top, 2:multi-detail
	public $_icon;
	public $_addTbl;
	public $_cols;
	public $_hasNote = false;
	public $_hasB = false;

	public $_layerId = NULL;
	public $_dataLoc = NULL;

	public	$_holderIndex = NULL;

	public $_subTbls = NULL;
	public $_defaultExtract = NULL;

	public $_linkedTbls = NULL;
	public $_showParentRef = false;

	public function __construct($id, $title=NULL, $isMulti=0, $addTbl=NULL, $align=0, $icon=NULL, $hasNote=NULL)
    {
		$this->_id = $id;
		$this->_title = $title;
		$this->_isMulti = $isMulti;
		$this->_addTbl = $addTbl;
		$this->_align = $align;
		$this->_icon = $icon;
		if ($hasNote === NULL && $this->_isMulti)
			$this->_hasNote = TRUE;
	}

	public function dup($newId, $title=NULL)
	{
		$d = new DTbl($newId, $this->_title, $this->_isMulti, $this->_addTbl, $this->_align, $this->_icon);
		$d->_dattrs = $this->_dattrs;
		$d->_helpKey = $this->_helpKey;
		$d->_width = $this->_width;
		$d->_cols = $this->_cols;
		$d->_hasNote = $this->_hasNote;
		$d->_hasB = $this->_hasB;
		$d->_layerId = $this->_layerId;
		$d->_dataLoc = $this->_dataLoc;
		$d->_holderIndex = $this->_holderIndex;
		$d->_subTbls = $this->_subTbls;
		$d->_defaultExtract = $this->_defaultExtract;
		$d->_linkedTbls = $this->_linkedTbls;
		$d->_showParentRef = $this->_showParentRef;

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
			if ($this->_hasNote)
				$this->_cols++;
			if($this->_icon != NULL)
				$this->_cols++;
		}
		else {
			$this->_cols = 3;
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

	public function PrintHtml(&$data, $ref, $disp, $isEdit)
	{
		if ($isEdit)
			$this->printEditHtml($data, $disp);
		else
			$this->printViewHtml($data, $ref, $disp);
	}

	private function getPrintHeader($disp, $actString)
	{
		$buf = '<tr><td class=xtbl_header colspan="' . $this->_cols . '">';

		// tooltip
		$table_help = ' ';

		$dhelp_item = DATTR_HELP::GetInstance()->GetItem($this->_helpKey);
		if($dhelp_item != NULL) {
			$table_help = '&nbsp;&nbsp;&nbsp;&nbsp' . $dhelp_item->render($this->_helpKey);
		}
		else if (count($this->_dattrs) == 1) {
			$av = array_values($this->_dattrs);
			$a0 = $av[0];
			if ($a0->_inputType == 'textarea1') {
				$dhelp_item = DATTR_HELP::GetInstance()->GetItem($a0->_helpKey);
				if($dhelp_item != NULL) {
					$is_blocked = $a0->blockedVersion();
					$version = $is_blocked? $a0->_version: 0;
					$table_help = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' . $dhelp_item->render($a0->_helpKey, $version);
				}
			}

		}
		$title = $this->_title;
		if ($this->_showParentRef && $disp->_ref != NULL) {
			$pos = strpos($disp->_ref, '`');
			if ($pos !== FALSE)
				$title .= ' - ' . substr($disp->_ref, 0, $pos);
			else
				$title .= ' - ' . $disp->_ref;
		}
		$buf .= '<span class="xtbl_title">'. $title . $table_help . '</span>';

		$all_blocked = TRUE;
		$keys = array_keys($this->_dattrs);
		foreach( $keys as $i )
		{
			if ( !$this->_dattrs[$i]->blockedVersion() ) {
				$all_blocked = FALSE;
				break;
			}
		}
		if ($all_blocked)
			$actString = NULL;

		if ( $actString != NULL ) {
			$titleActionLink = $this->getActionLink($disp, $actString);
			$buf .= '<span class="xtbl_edit">'.$titleActionLink.'</span>';
		}

		$buf .= "</td></tr>\n";

		if ( $this->_isMulti == 1 )
		{
			$buf .= '<tr class="xtbl_title2">';

			$hasSort = ( $disp->_sort != NULL && $disp->_sort[0] == $this->_id );
			$a = '1'; //ascend
			$url = $disp->_ctrlUrl . 'm=' . $disp->_mid . '&p=' . $disp->_pid;
			if ( $disp->_tid != NULL )
				$url .= '&t=' . $disp->_tid;
			if ( $disp->_ref != NULL )
				$url .= '&r=' . $disp->_ref;

			if ($this->_icon != NULL)
				$buf .= '<td class="icon"></td>';

			if ($this->_hasNote)
				$buf .= '<td class="icon"></td>';

			foreach( $keys as $i )
			{
				$attr = $this->_dattrs[$i];
				if ( $attr->_FDE[1] == 'N' )
					continue;

				$buf .= '<td';
				if ( isset($this->_align[$i]) )
					$buf .= ' align="'.$this->_align[$i] .'"';

				$buf .= '>' . $attr->_label;
				if ( $attr->_type != 'action' )
				{
					$buf .= ' <a href="' . $url . '&sort=' . $this->_id . '`';
					if ( $hasSort && array_key_exists(2,$disp->_sort) && array_key_exists(1,$disp->_sort) && $disp->_sort[2] == $attr->_key && $disp->_sort[1] == '1' )
						$a = '0';
					else
						$a = '1';

					$buf .= $a . $attr->_key . '">';
					$buf .= ($a=='1')? '<img src="/static/images/icons/up.gif" border="0" width="13" align="absmiddle">' : '<img src="/static/images/icons/down.gif" width="13" align="absmiddle" border="0">';
					$buf .= '</a>';
				}
				if ( $attr->_type == 'ctxseq' ) {
					$attr->_hrefLink = $url . $attr->_href;
				}
				$buf .= '</td>';
			}
			$buf .= "</tr>\n";
		}

		return $buf;
	}

	private function printViewHtml(&$data, $ref, $disp)
	{
		$buf = '<div><table width="'.$this->_width.'" class="xtbl" border="0" cellpadding="5" cellspacing="1">' . "\n";

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

			$buf .= $this->getPrintHeader($disp, $actString);

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

						if(array_key_exists(2,$disp->_sort))
							$by = $disp->_sort[2];
						else
							$by = NULL;

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
					$buf .= $this->getPrintHtmlLine1($data[$key], $key, $disp, $action_attr);
				}


			}
		}
		else
		{
			if ( $ref == NULL ) {
				$actString = 'e';
				if ( $this->_hasB )
					$actString .= 'B';
			}
			else {
				$actString = 'Ed';
				$actString .= ($this->_hasB ? 'B' : 'b');
			}
			$buf .= $this->getPrintHeader($disp, $actString);

			$keys = array_keys($this->_dattrs);
			foreach( $keys as $i )	{
				$buf .= $this->getPrintHtmlLine($data, $disp, $i);
			}
		}

		$buf .= '</table></div>';
		echo "$buf \n";

	}

	private function getPrintTips($tips)
	{
		$buf = '<div class="tips"><ul>';
		foreach( $tips as $tip )
		{
			if(strlen($tip)) {
				$buf .= "<li>$tip</li>\n";
			}
		}
		$buf .= '</ul></div>';
		return $buf;
	}

	private function printEditHtml(&$data, $disp)
	{
		$buf = '';

		$tips = DTblTips::getTips($this->_id);

		if ( $tips != NULL ) {
			$buf .= $this->getPrintTips($tips);
		}

		$buf .= "\n". '<div><table width="'.$this->_width.'" class="xtbl" border="0" cellpadding="5" cellspacing="1">' . "\n";

		$actString = ( (substr($this->_id, -3) == 'SEL') ? 'n':'s' ) . ( $this->_hasB ? 'B':'b');
		$buf .= $this->getPrintHeader($disp, $actString);

		$keys = array_keys($this->_dattrs);
		foreach ( $keys as $key ) {
			$buf .= $this->getPrintHtmlInputLine($data, $disp->_info, $key);
		}

		$buf .= '</table></div>';
		echo "$buf \n";
	}

	private function getPrintHtmlLine(&$data, $disp, $index)
	{
		$valwid = 0;
		$attr = $this->_dattrs[$index];
		if ( $attr == NULL || $attr->_FDE[1] == 'N') {
			return '';
		}

		$is_blocked = $attr->blockedVersion();
		$version = $is_blocked? $attr->_version: 0;
		if ( $attr->_type == 'sel1' ) {
			$attr->populate_sel1_options($disp->_info, $data);
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
				$buf .= '</td><td class="icon">';
			}

			//add tooltip
			$dhelp_item = DATTR_HELP::GetInstance()->GetItem($attr->_helpKey);
			if($dhelp_item != NULL) {
				$buf .= $dhelp_item->render($attr->_helpKey, $version);
			}

			$buf .= '</td>';
		}

		if ($this->_cols == 1) {
			$buf .= '</tr><tr class="xtbl_value"><td';
		}
		else {
			$buf .= '<td';
		}
		if ($attr->blockedVersion()) {
			$buf .= ' class="xtbl_value_blocked"';
		}
		if ($valwid > 0) {
			$buf .= " width=\"$valwid\"";
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
		return $buf;
	}


	private function getPrintHtmlInputLine(&$data, &$info, $key)
	{
		$attr = $this->_dattrs[$key];
		if ( $attr == NULL || $attr->_FDE[2] == 'N')
			return '';

		$valwid = 0;
		$is_blocked = $attr->blockedVersion();
		$version = $is_blocked? $attr->_version: 0;
		if ( $attr->_type == 'sel1' ) {
			$attr->populate_sel1_options($info, $data);
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
				$buf .= '</td><td class="icon">';
			}

			//add tooltip
			$dhelp_item = DATTR_HELP::GetInstance()->GetItem($attr->_helpKey);
			if($dhelp_item != NULL) {
				$buf .= $dhelp_item->render($attr->_helpKey, $version);
			}

			$buf .= '</td>';
		}

		if ($this->_cols == 1) {
			$buf .= '</tr><tr class="xtbl_value"><td';
		}
		else {
			$buf .= '<td';
		}
		if ($attr->blockedVersion()) {
			$buf .= ' class="xtbl_value_blocked" ';
		}
		if ($valwid > 0) {
			$buf .= " width=\"$valwid\"";
		}
		$buf .= '>';

		if ($is_blocked)
			$buf .= ($attr->toHtml($data[$attr->_key]));
		else
			$buf .= ($attr->toHtmlInput($data[$attr->_key]));

		$buf .= "</td></tr>\n";

		return $buf;
	}

	private function getPrintHtmlLine1(&$data, $key0, $disp, $action_attr)
	{
		static $htmlid;

		if (isset($htmlid))
			$htmlid++;
		else
			$htmlid = 1;

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
			if ( $attr->_FDE[1] == 'N' )
				continue;

			$linkedData = NULL;
			if ($attr->_linkedkeys != NULL) {
				$linkedData = $data[$attr->_linkedkeys];
			}

			if($key == 0) {
				if ($this->_icon != NULL) {
					if($attr->_key == "type" &&  is_array($attr->_maxVal) && is_array($this->_icon)) {
						$icon_name = array_key_exists($data['type']->GetVal(), $this->_icon) ?
							$this->_icon[$data['type']->GetVal()] : 'application';
					}
					else {
						$icon_name = $this->_icon;
					}
					$buf .= '<td class="icon"><img src="/static/images/icons/'. $icon_name . '.gif"></td>';
				}
				if ($this->_hasNote) {
					$buf .= '<td class="icon">';
					if(isset($data['note']) && $data['note']->HasVal()) {
						$buf .= '<img class="xtip-hover-info' . $htmlid . '" src="/static/images/icons/info.gif">'
								. '<div id="xtip-note-info' . $htmlid . '" class="snp-mouseoffset notedefault">'
								. $data['note']->GetVal() . '</div>';
					}
					$buf .= '</td>';
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
					$attr->populate_sel1_options($disp->_info, $data);
				}
				if ($attr->_key == $this->_holderIndex) {
					$buf .= ($attr->toHtml($data[$attr->_key], $indexActionLink, $linkedData));
				}
				else {
					$buf .= ($attr->toHtml($data[$attr->_key], NULL, $linkedData));
				}
			}
			$buf .= '</td>';
		}
		$buf .= "</tr>\n";

		return $buf;
	}


}
