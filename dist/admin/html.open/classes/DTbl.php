<?php

class DTbl
{
	public $_id;
	public $_dattrs;
	public $_helpKey;
	public $_cols;
	public $_isMulti;//0:regular, 1:multi-top, 2:multi-detail

	public	$_holderIndex = NULL;
	public $_showParentRef = false;

	public $_subTbls = NULL;
	public $_defaultExtract = NULL;
	public $_linkedTbls = NULL;

	private $_width = '100%';
	private $_align;
	private $_title;
	private $_addTbl;
	private $_icon;
	private $_hasNote;


	private $_sorted_tbl = FALSE;
	private $_sort_ascend;
	private $_sort_key;

	public function __construct($id, $title=NULL, $isMulti=0, $addTbl=NULL, $align=0, $icon=NULL, $hasNote=FALSE)
    {
		$this->_id = $id;
		$this->_title = $title;
		$this->_isMulti = $isMulti;
		$this->_addTbl = $addTbl;
		$this->_align = $align;
		$this->_icon = $icon;
		$this->_hasNote = $hasNote;
	}

	public function Dup($newId, $title=NULL)
	{
		$d = new DTbl($newId, $this->_title, $this->_isMulti, $this->_addTbl, $this->_align, $this->_icon);
		$d->_dattrs = $this->_dattrs;
		$d->_helpKey = $this->_helpKey;
		$d->_width = $this->_width;
		$d->_cols = $this->_cols;
		$d->_hasNote = $this->_hasNote;
		$d->_holderIndex = $this->_holderIndex;
		$d->_subTbls = $this->_subTbls;
		$d->_linkedTbls = $this->_linkedTbls;
		$d->_defaultExtract = $this->_defaultExtract;
		$d->_showParentRef = $this->_showParentRef;

		if ($title != NULL) {
			$d->_title = $title;
		}

		return $d;
	}

	public function SetAttr($attrs, $holderIdx=NULL)
	{
		$this->_dattrs = $attrs;
		$this->_holderIndex = $holderIdx;
		if ( $this->_isMulti == 1 ) {
			$this->_cols = count($attrs);
			if ($this->_hasNote)
				$this->_cols ++;

			if($this->_icon != NULL)
				$this->_cols++;
		}
		else {
			$this->_cols = 3;
		}
	}

	public function GetSubTid($node)
	{
		if ($this->_subTbls == '')
			return NULL;

		$keynode = $node->GetChildren($this->_subTbls[0]);
		if ($keynode == NULL)
			return NULL;
		$newkey = $keynode->Get(CNode::FLD_VAL);
		if ( ($newkey == '0') || !isset($this->_subTbls[$newkey]) ) {
			return $this->_subTbls[1]; // use default
		}
		else
			return $this->_subTbls[$newkey];
	}


	public function PrintHtml($dlayer, $disp)
	{
		if ($this->_holderIndex != NULL && $dlayer != NULL) {
			// populate missing index
			if (is_array($dlayer)) {
				foreach ($dlayer as $key => $nd) {
					if ($nd->GetChildren($this->_holderIndex) == NULL) {
						$nd->AddChild(new CNode($this->_holderIndex, $nd->Get(CNode::FLD_VAL)));
					}
				}
			}
			elseif ($dlayer->GetChildren($this->_holderIndex) == NULL) {
				$dlayer->AddChild(new CNode($this->_holderIndex, $dlayer->Get(CNode::FLD_VAL)));
			}
		}

		if ($disp->IsViewAction())
			$this->print_view($dlayer, $disp);
		else
			$this->print_edit($dlayer, $disp);
	}


	private function get_print_header($disp, $actString, $hasSort=FALSE)
	{
		$buf = '<tr><td class=xtbl_header colspan="' . $this->_cols . '">';

		// tooltip
		$table_help = ' ';

		$dhelp_item = DATTR_HELP::GetInstance()->GetItem($this->_helpKey);
		if($dhelp_item != NULL) {
			$table_help = '&nbsp;&nbsp;&nbsp;&nbsp' . $dhelp_item->render($this->_helpKey);
		}
		else if (count($this->_dattrs) == 1 && $this->_cols == 1) {
			$av = array_values($this->_dattrs);
			$a0 = $av[0];
			if ($a0->_label == NULL || $a0->_label == $this->_title) {
				$dhelp_item = DATTR_HELP::GetInstance()->GetItem($a0->_helpKey);
				if($dhelp_item != NULL) {
					$is_blocked = $a0->blockedVersion();
					$version = $is_blocked? $a0->_version: 0;
					$table_help = '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' . $dhelp_item->render($a0->_helpKey, $version);
				}
			}

		}
		$title = $this->_title;
		$ref = $disp->Get(DInfo::FLD_REF);
		if ($this->_showParentRef && $ref != NULL) {
			$pos = strpos($ref, '`');
			if ($pos !== FALSE)
				$title .= ' - ' . substr($ref, 0, $pos);
			else
				$title .= ' - ' . $ref;
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
			$titleActionLink = $disp->GetActionLink($actString, $this->_id, '', $this->_addTbl);
			$buf .= '<span class="xtbl_edit">'.$titleActionLink.'</span>';
		}

		$buf .= "</td></tr>\n";

		if ( $this->_isMulti == 1 )
		{
			$buf .= '<tr class="xtbl_title2">';
			if ($hasSort) {
				$this->_sorted_tbl  = FALSE;
				$sortval = $disp->Get(DInfo::FLD_SORT);
				if ($sortval != NULL) {
					$pos = strpos($sortval, '`');
					if ($this->_id == substr($sortval, 0, $pos)) {
						$this->_sorted_tbl = TRUE;
						$this->_sort_ascend = $sortval[$pos+1];
						$this->_sort_key = substr($sortval, $pos+2);
					}
				}
			}
			$url = $disp->Get(DInfo::FLD_CtrlUrl);
			if ( $disp->Get(DInfo::FLD_TID) != NULL )
				$url .= '&t=' . $disp->Get(DInfo::FLD_TID);
			if ( $disp->Get(DInfo::FLD_REF) != NULL )
				$url .= '&r=' . $disp->Get(DInfo::FLD_REF);

			if ($this->_icon != NULL)
				$buf .= '<td class="icon"></td>';

			if ($this->_hasNote)
				$buf .= '<td class="icon"></td>';

			foreach( $keys as $i )
			{
				$attr = $this->_dattrs[$i];
				if ( $attr->IsFlagOn(DAttr::BM_HIDE) )
					continue;

				$buf .= '<td';
				if ( isset($this->_align[$i]) )
					$buf .= ' align="'.$this->_align[$i] .'"';

				$buf .= '>' . $attr->_label;
				if ( $hasSort && $attr->_type != 'action' )
				{
					$buf .= ' <a href="' . $url . '&sort=' . $this->_id . '`';
					if ($this->_sorted_tbl && $this->_sort_ascend == 1 && $this->_sort_key == $attr->GetKey())
						$a = '0';
					else
						$a = '1'; //ascend

					$buf .= $a . $attr->GetKey() . '"><img src="/static/images/icons/';
					$buf .= ($a=='1')? 'up' : 'down';
					$buf .= '.gif" border="0" width="13" align="absmiddle"></a>';
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

	private function print_view($dlayer, $disp)
	{
		$buf = '<div><table width="'.$this->_width.'" class="xtbl" border="0" cellpadding="5" cellspacing="1">' . "\n";
		$ref = $disp->GetLast(DInfo::FLD_REF);
		$disptid = $disp->Get(DInfo::FLD_TID);
		$hasB = ($disptid != '');

		if ( $this->_isMulti == 1) {
			if ( $this->_addTbl == NULL )
				$actString = 'E'; //e';
			else if ( $this->_addTbl != 'N' )
				$actString = 'a';
			else
				$actString = '';

			if ($hasB)
				$actString .= 'B';

			$hasSort = ($dlayer != NULL && is_array($dlayer));
			$buf .= $this->get_print_header($disp, $actString, $hasSort);

			if ( $dlayer != NULL)
			{
				if (!is_array($dlayer)) {
					$dlayer = array($dlayer->Get(CNode::FLD_VAL) => $dlayer);
				}

				if ($hasSort && $this->_sorted_tbl) {
					$sorted = array();
					$is_num = TRUE;
					foreach ($dlayer as $key => $node) {
						$val = $node->GetChildVal($this->_sort_key);
						if ($is_num && !is_numeric($val))
							$is_num = FALSE;
						$sorted[$key] = $val;
					}
					$flag = $is_num ? SORT_NUMERIC : SORT_STRING;
					if ($this->_sort_ascend == 1)
						asort($sorted, $flag);
					else
						arsort($sorted, $flag);
					$keys = array_keys($sorted);
				}
				else
					$keys = array_keys($dlayer);

				$action_attr = NULL;
				foreach ($this->_dattrs as $attr) {
					if ($attr->_type == 'action') {
						if ( $attr->IsFlagOn(DAttr::BM_NOTNULL) && strpos($attr->_maxVal, 'd') !== FALSE && count($dlayer) == 1	)
							$attr->_maxVal = str_replace('d', '', $attr->_maxVal); // do not allow delete if only one left
						$action_attr = $attr;
						break;
					}
				}
				$index = 0;
				foreach ( $keys as $key ) {
					$nd = $dlayer[$key];
					$buf .= $this->get_print_line_multi($nd, $key, $index, $disp, $action_attr);
					$index ++;
				}
			}
		}
		else
		{
			$actString = 'E';
			if ($hasB)
				$actString .= 'B';
			if ($ref != NULL && is_array($dlayer)) {
				$dlayer = $dlayer[$ref];
			}

			$buf .= $this->get_print_header($disp, $actString);

			foreach ($this->_dattrs as $attr) {
				$buf .= $this->get_print_line($dlayer, $disp, $attr);
			}
		}

		$buf .= '</table></div>';
		echo "$buf \n";

	}

	private function get_print_tips($tips)
	{
		$buf = '<div class="tips"><ul>';
		foreach( $tips as $tip )
		{
			if($tip != '') {
				$buf .= "<li>$tip</li>\n";
			}
		}
		$buf .= '</ul></div>';
		return $buf;
	}

	private function print_edit($dlayer, $disp)
	{
		$buf = '';
		$ref = $disp->GetLast(DInfo::FLD_REF);

		if ($ref != NULL && is_array($dlayer)) {
			$dlayer = $dlayer[$ref];
		}

		$tips = DTblTips::getTips($this->_id);

		if ( $tips != NULL ) {
			$buf .= $this->get_print_tips($tips);
		}

		$buf .= "\n". '<div><table width="'.$this->_width.'" class="xtbl" border="0" cellpadding="5" cellspacing="1">' . "\n";

		$actString = ( (substr($this->_id, -3) == 'SEL') ? 'n':'s' ) . 'B';
		$buf .= $this->get_print_header($disp, $actString);

		foreach ($this->_dattrs as $attr) {
			$buf .= $this->get_print_inputline($dlayer, $disp, $attr);
		}

		$buf .= '</table></div>';
		echo "$buf \n";
	}

	private function get_print_line($node, $disp, $attr)
	{
		$valwid = 0;
		if ( $attr == NULL || $attr->IsFlagOn(DAttr::BM_HIDE)) {
			return '';
		}

		$is_blocked = $attr->blockedVersion();
		$version = $is_blocked? $attr->_version: 0;
		if ( $attr->_type == 'sel1' && $node != NULL && $node->GetChildVal($attr->GetKey()) != NULL ) {
			$attr->SetDerivedSelOptions($disp->GetDerivedSelOptions($this->_id, $attr->_minVal, $node));
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
			//$link = $disp->_ctrlUrl . 'm=' . $disp->_mid . '&p=' . $disp->_pid;
			$link = $disp->Get(DInfo::FLD_CtrlUrl);
			if ( $disp->Get(DInfo::FLD_TID) != NULL )
				$link .= '&t=' . $disp->Get(DInfo::FLD_TID);
			if ( $disp->Get(DInfo::FLD_REF) != NULL )
				$link .= '&r='. $disp->Get(DInfo::FLD_REF);

			$link .= $attr->_href;
			$attr->_hrefLink = str_replace('$R', $disp->Get(DInfo::FLD_REF), $link);
		}

		$buf .= ($attr->toHtml($node));


		$buf .= "</td></tr>\n";
		return $buf;
	}

	private function get_print_inputline($dlayer, $disp, $attr)
	{
		if ( $attr->IsFlagOn(DAttr::BM_NOEDIT))
			return '';

		$valwid = 0;
		$is_blocked = $attr->blockedVersion();
		$version = $is_blocked? $attr->_version: 0;
		if ( $attr->_type == 'sel1' ) {
			$attr->SetDerivedSelOptions($disp->GetDerivedSelOptions($this->_id, $attr->_minVal, $dlayer));
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
			if ($attr->IsFlagOn(DAttr::BM_NOTNULL))
				$buf .= ' *';

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
		if ($is_blocked) {
			$buf .= ' class="xtbl_value_blocked" ';
		}
		if ($valwid > 0) {
			$buf .= " width=\"$valwid\"";
		}
		$buf .= '>';

		if ($is_blocked)
			$buf .= $attr->toHtml($dlayer);
		else
			$buf .= $attr->toHtmlInput($dlayer);

		$buf .= "</td></tr>\n";

		return $buf;
	}

	private function get_print_line_multi($data, $key0, $htmlid, $disp, $action_attr)
	{
		$buf = '<tr class="xtbl_value">';

		$keys = array_keys($this->_dattrs);

		//allow index field clickable, same as first action
		$actionLink = null;
		$indexActionLink = null;

		if ($action_attr != NULL) {

			if ( is_array($action_attr->_minVal) ) {
				$index = $action_attr->_minVal[0];
				$type = $data->GetChildVal($index);
				$ti = isset($action_attr->_minVal[$type]) ? $action_attr->_minVal[$type] : $action_attr->_minVal[1];
			}
			else
				$ti = $action_attr->_minVal;

			$actionLink = $disp->GetActionLink( $action_attr->_maxVal, $ti, $key0);

			$tmp_a = strpos($actionLink, '"') +1;
			$tmp_e = strpos($actionLink, '">');
			$indexActionLink = substr($actionLink, $tmp_a, $tmp_e - $tmp_a);
		}

		foreach( $keys as $key )
		{
			$attr = $this->_dattrs[$key];
			if ( $attr->IsFlagOn(DAttr::BM_HIDE) )
				continue;

			if($key == 0) {
				if ($this->_icon != NULL) {
					if ($attr->GetKey() == 'type' && is_array($attr->_maxVal) && is_array($this->_icon)) {
						$type = $data->GetChildVal('type');
						$icon_name = isset($this->_icon[$type]) ? $this->_icon[$type] : 'application';
					}
					else {
						$icon_name = $this->_icon;
					}
					$buf .= '<td class="icon"><img src="/static/images/icons/'. $icon_name . '.gif"></td>';
				}
				if ($this->_hasNote) {
					$buf .= '<td class="icon">';
					if ( ($note = $data->GetChildVal('note')) != NULL ) {
						$buf .= '<img class="xtip-hover-info' . $htmlid . '" src="/static/images/icons/info.gif">'
								. '<div id="xtip-note-info' . $htmlid . '" class="snp-mouseoffset notedefault">' . $note . '</div>';
					}
					$buf .= '</td>';
				}
			}

			$buf .= '<td';
			if ( isset($this->_align[$key]) )
				$buf .= ' align="'.$this->_align[$key] .'"';
			$buf .= '>';

			if ( $attr->_type == 'action' )	{
				$buf .= $actionLink;
			}
			else {
				if ( $attr->_type == 'sel1' && $data->GetChildVal($attr->GetKey()) != NULL ) {
					$attr->SetDerivedSelOptions($disp->GetDerivedSelOptions($this->_id, $attr->_minVal, $data));
				}
				if ($attr->GetKey() == $this->_holderIndex)
					$buf .= $attr->toHtml($data, $indexActionLink);
				else
					$buf .= $attr->toHtml($data, NULL);
			}
			$buf .= '</td>';
		}
		$buf .= "</tr>\n";

		return $buf;
	}


}
