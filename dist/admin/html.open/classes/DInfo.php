<?php

class DInfo
{
	const FLD_ConfType = 1;
	const FLD_ConfErr = 2;
	const FLD_View = 3;
	const FLD_ViewName = 4;
	const FLD_TopMsg = 5;

	const FLD_CtrlUrl = 10;
	const FLD_MID = 11;
	const FLD_PID = 12;
	const FLD_TID = 13;
	const FLD_REF = 14;
	const FLD_ACT = 15;
	const FLD_SORT = 16;
	const FLD_TOKEN = 17;
	const FLD_PgData = 18;
	const FLD_ConfData = 19;

	// conftype
	const CT_SERV = 'serv';
	const CT_VH = 'vh';
	const CT_TP = 'tp';
	const CT_ADMIN = 'admin';
	const CT_EX = 'special';

	private $_confType;
	private $_view; //_type (serv, sltop, sl, vhtop, vh, tptop, tp, lbtop, lb, admin, altop, al)
	private $_viewName = NULL; // _name


	private $_ctrlUrl = 'confMgr.php?';

	private $_mid = 'serv'; // default
	private $_pid = NULL;
	private $_tid = NULL;
	private $_ref = NULL;
	private $_act;
	private $_token;

	private $_confErr;
	private $_pageData;
	private $_confData;
	private $_servData; // for populate vh level derived options

	private $_tabs;
	private $_topTabs;
	private $_sort;

	private $_topMsg;
	private $_isPrintingLinkedTbl;

	var $_err;

	public function ShowDebugInfo()
	{
		return "DINFO: conftype={$this->_confType} view={$this->_view} viewname={$this->_viewName} mid={$this->_mid} pid={$this->_pid} tid={$this->_tid} ref={$this->_ref} act={$this->_act}\n";
	}

	function __construct()
	{
		$has_pid = FALSE;
		$mid = GUIBase::GrabGoodInput("request",'m');

		if ($mid != NULL) {
			$this->_mid = $mid;
			$pid = GUIBase::GrabGoodInput("request",'p');
			if ($pid != NULL) {
				$this->_pid = $pid;
				$has_pid = TRUE;
			}
		}

		if ( ($pos = strpos($this->_mid, '_')) > 0 ) {
			$this->_view = substr($this->_mid, 0, $pos);
			$this->_viewName = substr($this->_mid, $pos+1);
			if ($this->_pid == ''
					|| $this->_view == 'sl' || $this->_view == 'al'
					|| $this->_pid == 'base' || $this->_pid == 'mbr')
				$this->_ref = $this->_viewName; // still in serv conf
		}
		else {
			$this->_view = $this->_mid ;
		}

		$this->_confType = ( $this->_mid[0] == 'a' ) ? self::CT_ADMIN: self::CT_SERV;

		$this->_tabs = DPageDef::GetInstance()->GetTabDef($this->_view);

		if ($has_pid) {
			if (!array_key_exists($this->_pid, $this->_tabs))
				die("Invalid pid - {$this->_pid} \n");
		}
		else {
			$this->_pid = key($this->_tabs); // default
		}

		if ( $has_pid && !isset($_REQUEST['t0']) && isset($_REQUEST['t']) )
		{
			$this->_tid = GUIBase::GrabGoodInput('request', 't');

			$t1 = GUIBase::GrabGoodInputWithReset('request', 't1');
			if ($t1 != NULL	 && ( $this->GetLast(self::FLD_TID) != $t1) )
				$this->_tid .= '`' . $t1;

			if ( ($r = GUIBase::GrabGoodInputWithReset('request', 'r')) != NULL )
				$this->_ref = $r;
			if ( ($r1 = GUIBase::GrabGoodInputWithReset('request', 'r1')) != NULL )
				$this->_ref .= '`' . $r1;
		}

		$this->_act = GUIBase::GrabGoodInput("request",'a');
		if ( $this->_act == NULL ) {
			$this->_act = 'v';
		}

		$tokenInput = GUIBase::GrabGoodInput("request",'tk');
		global $_SESSION;
		$this->_token =  $_SESSION['token'];
		if ($this->_act != 'v' && $this->_token != $tokenInput) {
			die('Illegal entry point!');
		}

		if ($this->_act == 'B') {
			$this->TrimLastId();
			$this->_act = 'v';
		}

		$this->_sort = GUIBase::GrabGoodInput("request",'sort');
	}

	public function Get($field)
	{
		switch ($field) {
			case self::FLD_CtrlUrl:
				return "{$this->_ctrlUrl}m={$this->_mid}&p={$this->_pid}";
			case self::FLD_View: return $this->_view;
			case self::FLD_ViewName: return $this->_viewName;
			case self::FLD_TopMsg: return $this->_topMsg;
			case self::FLD_ConfType: return $this->_confType;
			case self::FLD_ConfErr: return $this->_confErr;
			case self::FLD_MID: return $this->_mid;
			case self::FLD_PID: return $this->_pid;
			case self::FLD_TID: return $this->_tid;
			case self::FLD_REF: return $this->_ref;
			case self::FLD_ACT: return $this->_act;
			case self::FLD_PgData: return $this->_pageData;
			case self::FLD_ConfData: return $this->_confData;
			case self::FLD_TOKEN: return $this->_token;
			case self::FLD_SORT: return $this->_sort;
			default: error_log("invalid DInfo field : $field\n");
		}
	}

	public function Set($field, $value)
	{
		switch ($field) {
			case self::FLD_ConfErr:
				$this->_confErr = $value;
				break;
			case self::FLD_ACT:
				$this->_act = $value;
				break;
			case self::FLD_PgData:
				$this->_pageData = $value;
				break;
			case self::FLD_ConfData:
				$this->_confData = $value;
				break;
			case self::FLD_REF:
				$this->_ref = $value;
				break;
			case self::FLD_ViewName:
				$this->_viewName = $value;
				if ($value == NULL) // by delete
					$value = 'top';
				else
					$value = '_' . $value;

				if ( ($pos = strpos($this->_mid, '_')) > 0 ) {
					$this->_mid = substr($this->_mid, 0, $pos) . $value;
				}
				break;
			case self::FLD_TopMsg:
				$this->_topMsg[] = $value;
				break;
			default: die("not supported - $field");
		}
	}

	public function SetPrintingLinked($printinglinked)
	{
		$this->_isPrintingLinkedTbl = $printinglinked;
	}

	public function InitServ($servdata)
	{
		if ( $this->_confType == self::CT_SERV ) {
			$l = count($servdata->GetChildrenValues('listener'));
			$v = count($servdata->GetChildrenValues('virtualhost'));
			$t = count($servdata->GetChildrenValues('vhTemplate'));

			$this->_topTabs = array('Server' => 'serv', "Listeners ($l)" => 'sltop',
					"Virtual Hosts ($v)" => 'vhtop', "Virtual Host Templates ($t)" => 'tptop');
			$this->_servData = $servdata;
		}
		elseif ( $this->_confType == self::CT_ADMIN ) {
			$l = count($servdata->GetChildrenValues('listener'));
			$this->_topTabs = array('Admin' => 'admin', "Listeners ($l)" => 'altop');
		}

		if ($this->_view == 'serv')
			$this->_viewName = $servdata->GetId();

		if ($this->_act == 'd' || $this->_act == 'i') {
			$this->_topMsg[] = $this->getActionConfirm();
		}

	}

	public function GetTabs()
	{
		$buf = '<form name="confform" method="post" action="confMgr.php">
				<div class="xtab" style="margin-left:-8px;"><ul>';
		$uri = $this->_ctrlUrl . 'm=';

		foreach ( $this->_topTabs as $tabName => $mid ) {
			$buf .= '<li';
			if ($mid == $this->_mid)
				$buf .= ' class="on"';

			$buf .= "><a href='{$uri}{$mid}'>{$tabName}</a></li>";
		}

		$buf .= "</ul></div>\n";

		$navchar = '&#187';
		$labels = array('serv' => "Server $navchar", 'sl' => "Listener $navchar",
				'vh' => "Virtual Host $navchar", 'tp' => "Virtual Host Template $navchar",
				'lb' => "Cluster $navchar", 'admin' => 'WebAdmin Console', 'al' => "WebAdmin Listener $navchar");
		if (array_key_exists($this->_view, $labels)) {
			$buf .= "<div><h2>{$labels[$this->_view]} {$this->_viewName}</h2><div>\n";
		}

		$buf .= '<div class="xtab"><ul>';
		$uri .= $this->_mid . '&p=';

		foreach ( $this->_tabs as $pid => $tabName )
		{
			$buf .= '<li';
			if ($pid == $this->_pid)
				$buf .= ' class="on"';

			$buf .= "><a href='{$uri}{$pid}'>{$tabName}</a></li>";
		}

		$buf .= "</ul></div>\n";
		return $buf;
	}

	public function PrintEndForm()
	{
		$buf = '
<input type="hidden" name="a" value="s">
<input type="hidden" name="m" value="' . $this->_mid
		. '"><input type="hidden" name="p" value="' . $this->_pid
		. '"><input type="hidden" name="t" value="' . $this->_tid
		. '"><input type="hidden" name="r" value="' . $this->_ref
		. '"><input type="hidden" name="tk" value="' . $this->_token
		. '"><input type="hidden" name="file_create" value=""></div></form>';
		return $buf;
	}

	public function IsViewAction()
	{
		//$viewTags = 'vsDdBCiI';
		$editTags = 'EaScn';
		return ( strpos($editTags, $this->_act) === FALSE );
	}

	public function GetActionLink($actions, $editTid='', $editRef='', $addTid='')
	{
		$buf = '';
		$allActions = array('a'=>'Add', 'v'=>'View', 'E'=>'Edit', 's'=>'Save', 'B'=>'Back', 'n'=>'Next',
				'd'=>'Delete', 'D'=>'Yes', 'C'=>'Cancel', 'i'=>'Instantiate', 'I'=>'Yes', 'X' => 'View/Edit');
		$chars = preg_split('//', $actions, -1, PREG_SPLIT_NO_EMPTY);

		$ctrlUrl = '<a href="' . $this->Get(DInfo::FLD_CtrlUrl);

		$cur_tid = $this->GetLast(self::FLD_TID);
		foreach ( $chars as $act )
		{
			$buf .= ' &nbsp;';
			$name = $allActions[$act];
			if ( $act == 'C' ) {
				$act = 'B';
			}

			if ($act == 'B' && $this->_isPrintingLinkedTbl)
				continue; // do not print Back for linked view

			if ( $act == 's' ) {
				$buf .= '<a href="javascript:document.confform.submit()">Save</a>';
			}
			elseif ( $act == 'n' ) {
				$buf .= '<a href="javascript:document.confform.a.value=\'n\';document.confform.submit()">Next</a>';
			}
			elseif( $act == 'X') {
				//vhtop=>vh_... tptop=>tp_.... sltop=>sl_...
				$buf .= '<a href="' . $this->_ctrlUrl . 'm=' . substr($this->_view, 0, 2) . '_' . $editRef . "\">{$name}</a>";
			}
			else {

				if ($act == 'a') {
					$edittid = $addTid;
					$editref = '~';
				}
				else {
					$edittid = $editTid;
					$editref = $editRef;
				}

				if ($edittid == '' || $edittid == $cur_tid)
					$t = $this->_tid;
				elseif ($cur_tid != NULL && $cur_tid != $edittid)
					$t = $this->_tid . '`' . $edittid;
				else
					$t = $edittid;

				if ($editref == '') {
					$r = $this->_ref;
				}
				elseif ($this->_ref != NULL && $this->_ref != $editref)
					$r = $this->_ref . '`' . $editref;
				else
					$r = $editref;

				$t = '&t=' . $t;
				$r = '&r=' . urlencode($r);

				$buf .= $ctrlUrl . $t . $r . '&a=' . $act . '&tk=' . $this->_token . '">' . $name . '</a>';
			}
		}
		return $buf;
	}

	public function TrimLastId()
	{
		if (($pos = strrpos($this->_tid, '`')) !== FALSE)
			$this->_tid = substr($this->_tid, 0, $pos);
		else
			$this->_tid = NULL;

		if (($pos = strrpos($this->_ref, '`')) !== FALSE)
			$this->_ref = substr($this->_ref, 0, $pos);
		elseif ($this->_view == 'sl' || $this->_view == 'al' || $this->_pid == 'base' || $this->_pid == 'mbr')
			$this->_ref = $this->_viewName; // still in serv conf
		else
			$this->_ref = NULL;
	}

	public function GetLast($field)
	{
		$id = NULL;
		if ($field == self::FLD_TID)
			$id = $this->_tid;
		elseif ($field == self::FLD_REF)
			$id = $this->_ref;

		if ( $id != NULL && ($pos = strrpos($id, '`')) !== FALSE )
			if (strlen($id) > $pos + 1)
				$id = substr($id, $pos+1);
			else
				$id = '';

		return $id;
	}

	public function GetFirst($field)
	{
		$id = NULL;
		if ($field == self::FLD_TID)
			$id = $this->_tid;
		elseif ($field == self::FLD_REF)
			$id = $this->_ref;

		if ( $id != NULL && ($pos = strpos($id, '`')) !== FALSE ) {
			$id = substr($id, 0, $pos);
		}

		return $id;
	}

	public function GetParentRef()
	{
		if (($pos = strrpos($this->_ref, '`')) !== FALSE)
			return substr($this->_ref, 0, $pos);
		else
			return '';
	}

	public function SwitchToSubTid($extracted)
	{
		if (($pos = strrpos($this->_tid, '`')) !== FALSE) {
			$tid0 = substr($this->_tid, 0, $pos+1);
			$tid = substr($this->_tid, $pos+1);
		}
		else {
			$tid0 = '';
			$tid = $this->_tid;
		}
		$tbl = DTblDef::getInstance()->GetTblDef($tid);

		$newkey = $extracted->GetChildVal($tbl->_subTbls[0]);
		$subtid = '';
		if ($newkey != NULL) {
			if ($newkey == '0' || !isset($tbl->_subTbls[$newkey]))
				$subtid = $tbl->_subTbls[1];
			else
				$subtid = $tbl->_subTbls[$newkey];
		}

		$this->_tid = $tid0 . $subtid;
	}

	public function GetDerivedSelOptions($tid, $loc, $node)
	{
		$o = array();

		if (substr($loc, 0, 13) == 'extprocessor:') {
			$type = substr($loc, 13);
			if ($type == '$$type') {
				if ($node != NULL)
					$type = $node->GetChildVal('type');
				else
					$type = 'fcgi';
			}
			if ($type == 'cgi') {
				$o['cgi'] = 'CGI Daemon';
				return $o;
			}
			if ($type == 'module') {
				$modules = $this->_servData->GetChildrenValues('module');
				if ($modules != NULL) {
					foreach ($modules as $mn)
						$o[$mn] = $mn;
				}
				return $o;
			}

			$exps = array();
			if (($servexps = $this->_servData->GetRootNode()->GetChildren('extprocessor')) != NULL) {
				if (is_array($servexps)) {
					foreach($servexps as $exname => $ex) {
						if ($ex->GetChildVal('type') == $type)
							$exps[] = $exname;
					}
				}
				elseif ($servexps->GetChildVal('type') == $type)
					$exps[] = $servexps->Get(CNode::FLD_VAL);
			}

			if ($this->_view == DInfo::CT_SERV) {
				foreach($exps as $exname) {
					$o[$exname] = $exname;
				}
				return $o;
			}
			foreach ($exps as $exname) {
				$o[$exname] = "[Server Level]: $exname";
			}

			$loc = ($this->_view == DInfo::CT_TP) ? 'virtualHostConfig:extprocessor' : 'extprocessor';
			if (($vhexps = $this->_confData->GetRootNode()->GetChildren($loc)) != NULL) {
				if (is_array($vhexps)) {
					foreach($vhexps as $exname => $ex) {
						if ($ex->GetChildVal('type') == $type)
							$o[$exname] = "[VHost Level]: $exname";
					}
				}
				else if ($vhexps->GetChildVal('type') == $type) {
					$exname = $vhexps->Get(CNode::FLD_VAL);
					$o[$exname] = "[VHost Level]: $exname";
				}
			}
			return $o;
		}

		if (in_array($loc, array('virtualhost','listener','module'))) {
			$names = $this->_servData->GetChildrenValues($loc);
		}
		elseif ($loc == 'realm') {
			if ($this->_view == DInfo::CT_TP)
				$loc = "virtualHostConfig:$loc";
			$names = $this->_confData->GetChildrenValues($loc);
		}

		sort($names);
		foreach ($names as $name)
			$o["$name"] = $name;

		return $o;
	}

	private function getActionConfirm()
	{
		if ( $this->_act == 'd' ) {
			//should validate whether there is reference to this entry
			$mesg = 'Are you sure you want to delete this entry?<br><span class="edit-link">'
					. $this->GetActionLink('DC')
					. '</span><br>This will be permanently removed from the configuration file.';
		}
		elseif ( $this->_act == 'i' )
		{
			$mesg = 'Are you sure you want to instantiate this virtual host?<br><span class="edit-link">'
					. $this->GetActionLink('IC')
					. '</span><br>This will create a dedicated configuration file for this virtual host.';
		}

		return $mesg;
	}

	public function GetVHRoot()
	{
		// type = 'SR', 'VR'
		if ($this->_view == self::CT_VH) {
			$vn = $this->_viewName;
			if (($vh = $this->_servData->GetChildNodeById('virtualhost', $vn)) != NULL) {
				$vhroot = PathTool::GetAbsFile($vh->GetChildVal('vhRoot'),'SR', $vn);
				return $vhroot;
			}
		}
		return NULL;
	}
}

