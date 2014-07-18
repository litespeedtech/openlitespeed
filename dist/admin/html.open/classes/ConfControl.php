<?php

class ConfControl
{
	private $_confType; //'serv','admin'
	private $_serv;
	private $_curOne; //vh or tp
	private $_special;

	private $_disp;

	private static $_instance = NULL;

	private function __construct()
	{
		$this->_disp = new DInfo();
		$this->loadConfig();
	}

	public static function singleton()
	{
		if (!isset(self::$_instance)) {
			$c = __CLASS__;
			self::$_instance = new $c;
		}

		return self::$_instance;
	}

	public static function GetDInfo()
	{
		$cc = ConfControl::singleton();
		$data = $cc->process();
		// temp
		if (is_a($data, 'CData'))
			$data = $data->GetRootNode();

		$cc->_disp->Set(DInfo::FLD_PgData, $data);

		return $cc->_disp;
	}

	public static function ExportConf()
	{
		$cc = ConfControl::singleton();
		$root = $cc->_serv->GetRootNode();

		$listeners = array();

		if (($lns = $root->GetChildren('listener')) != NULL) {
			if (!is_array($lns))
				$lns = array($lns);
			foreach ($lns as $listener)
				$listeners[$listener->Get(CNode::FLD_VAL)] = $listener->GetChildVal('address');
		}

		$vhnames = $cc->_serv->GetChildrenValues('virtualhost');

		if (($tps = $root->GetChildren('vhTemplate')) != NULL) {
			if (!is_array($tps))
				$tps = array($tps);
			foreach ($tps as $tp) {
				if (($member = $tp->GetChildren('member')) != NULL) {
					if (is_array($member))
						$vhnames = array_merge($vhnames, $member);
					else
						$vhnames[] = $member->Get(CNode::FLD_VAL);
				}
			}
		}

		$serverlog = PathTool::GetAbsFile($root->GetChildVal('errorlog'), 'SR');

		$data = array('listener' => $listeners, 'vhost' => $vhnames,
				'serv' => $cc->_serv->GetId(), 'servLog' => $serverlog);

		return $data;
	}


	public function GetConfFilePath($type, $name='')
	{
		$path = NULL;
		$servconf = SERVER_ROOT . "conf/httpd_config.conf" ; //fixed location
		if ( $type == DInfo::CT_SERV) {
			return $servconf;
		}
		elseif ( $type ==  DInfo::CT_ADMIN) {
			$adminRoot = PathTool::GetAbsFile('$SERVER_ROOT/admin/','SR'); //fixed loc

			if ($name == '') {
				$path = $adminRoot . 'conf/admin_config.conf' ;
			} elseif ($name == 'key') {
				$path = $adminRoot . 'conf/jcryption_keypair' ;
			}
		}
		elseif ( $type ==  DInfo::CT_VH ) {
			$vh = $this->_serv->GetChildNodeById('virtualhost', $name);
			if ($vh != NULL) {
				$vhrootpath = $vh->GetChildVal('vhRoot');
				$path = PathTool::GetAbsFile($vh->GetChildVal('configFile'), 'VR', $name, $vhrootpath);
			}
			else {
				die ("cannot find config file for vh $name\n");
				// should set as conf err
			}
		}
		elseif ($type == DInfo::CT_TP) {
			$tp = $this->_serv->GetChildNodeById('vhTemplate', $name);
			if ($tp != NULL)
				$path = PathTool::GetAbsFile($tp->GetChildVal('templateFile'), 'SR');
			else {
				die ("cannot find config file for tp $name\n");
				// should set as conf err
			}
		}

		return $path;
	}

	private function getConfData()
	{
		$view = $this->_disp->Get(DInfo::FLD_View);
		$pid = $this->_disp->Get(DInfo::FLD_PID);
		$tid = $this->_disp->Get(DInfo::FLD_TID);

		if ( ($view == DInfo::CT_SERV && strpos($tid, 'S_MIME') !== FALSE)
			|| ($view == DInfo::CT_ADMIN && $pid == 'usr')
			|| ($view == DInfo::CT_VH && (strpos($tid, 'V_UDB') !== FALSE || strpos($tid, 'V_GDB') !== FALSE))) {
			return $this->_special;
		}
		elseif (($view == DInfo::CT_VH && $pid != 'base') || ($view == DInfo::CT_TP && $pid != 'mbr'))
			return $this->_curOne;
		else
			return $this->_serv;
	}

	private function loadConfig()
	{
		$conftype = $this->_disp->Get(DInfo::FLD_ConfType);
		$confpath = $this->GetConfFilePath($conftype);
		$this->_serv = new CData($conftype, $confpath);
		if (($conferr = $this->_serv->GetConfErr()) != NULL)
			$this->_disp->Set(DInfo::FLD_TopMsg, $conferr);

		$this->_disp->InitServ($this->_serv);

		$view = $this->_disp->Get(DInfo::FLD_View);
		$pid = $this->_disp->Get(DInfo::FLD_PID);
		$tid = $this->_disp->Get(DInfo::FLD_TID);

		if (($view == DInfo::CT_VH && $pid != 'base') || ($view == DInfo::CT_TP && $pid != 'mbr')) {
			$confpath = $this->GetConfFilePath($view, $this->_disp->Get(DInfo::FLD_ViewName));
			$this->_curOne = new CData($view, $confpath);
			if (($conferr = $this->_curOne->GetConfErr()) != NULL)
				$this->_disp->Set(DInfo::FLD_TopMsg, $conferr);
		}

		// special
		if ($view == DInfo::CT_SERV && strpos($tid, 'S_MIME') !== FALSE) {
			$mime = $this->_serv->GetChildrenValues('mime');
			$file = PathTool::GetAbsFile($mime[0], 'SR');
			$this->_special = new CData(DInfo::CT_EX, $file, 'MIME');
			if (($conferr = $this->_special->GetConfErr()) != NULL)
				$this->_disp->Set(DInfo::FLD_TopMsg, $conferr);

		}
		elseif ($view == DInfo::CT_ADMIN && $pid == 'usr') {
			$file = SERVER_ROOT . 'admin/conf/htpasswd';
			$this->_special = new CData(DInfo::CT_EX, $file, 'ADMUSR');
			if (($conferr = $this->_special->GetConfErr()) != NULL)
				$this->_disp->Set(DInfo::FLD_TopMsg, $conferr);
		}
		elseif ($view == DInfo::CT_VH &&
				(($isudb = strpos($tid, 'V_UDB')) !== FALSE || strpos($tid, 'V_GDB') !== FALSE )) {
			$realm = $this->_curOne->GetChildNodeById('realm', $this->_disp->GetFirst(DInfo::FLD_REF));
			if ($realm != NULL) {
				$isudb = ($isudb !== FALSE);
				$file = $realm->GetChildVal($isudb ? 'userDB:location' : 'groupDB:location');
				$vhroot = $this->_disp->GetVHRoot();
				$file = PathTool::GetAbsFile($file, 'VR', $this->_disp->Get(DInfo::FLD_ViewName), $vhroot);
				$this->_special = new CData(DInfo::CT_EX, $file, $isudb ? 'V_UDB' : 'V_GDB');
				if (($conferr = $this->_special->GetConfErr()) != NULL)
					$this->_disp->Set(DInfo::FLD_TopMsg, $conferr);
			}
		}

		$adminconf = NULL;
		if (!CAuthorizer::HasSetTimeout() && $conftype == DInfo::CT_SERV) {
			$adminpath = $this->GetConfFilePath(DInfo::CT_ADMIN);
			$adminconf = new CData(DInfo::CT_ADMIN, $adminpath);
			if (($conferr = $adminconf->GetConfErr()) != NULL)
				$this->_disp->Set(DInfo::FLD_TopMsg, $conferr);
		}
		elseif ( $conftype == DInfo::CT_ADMIN) {
			$adminconf = $this->_serv;
		}
		if ($adminconf != NULL) {
			$timeout = $adminconf->GetRootNode()->GetChildVal('sessionTimeout');
			if ($timeout == NULL)
				$timeout = 60; // default
			CAuthorizer::SetTimeout($timeout);
		}
	}

	private function process()
	{
		//init here
		$confdata = $this->getConfData($this->_disp);
		$this->_disp->Set(DInfo::FLD_ConfData, $confdata);

		$needTrim = 0;
		$tblDef = DTblDef::GetInstance();
		$disp_act = $this->_disp->Get(DInfo::FLD_ACT);

		if ( $disp_act == 's' )
		{
			$validator = new ConfValidation();
			$extracted = $validator->ExtractPost($this->_disp);
			if ($extracted->HasErr()) {
				$this->_disp->Set(DInfo::FLD_ACT, 'S');
				$this->_disp->Set(DInfo::FLD_TopMsg, $extracted->Get(CNode::FLD_ERR));
				return $extracted;
			}
			else {
				$confdata->SavePost( $extracted, $this->_disp);
				$needTrim = 2;
			}
		}
		elseif ($disp_act == 'a') {
			$added = new CNode(CNode::K_EXTRACTED, '');
			return $added;
		}
		else if ( $disp_act == 'c' || $disp_act == 'n')
		{ // what is 'c': create
			$validator = new ConfValidation();
			$extracted = $validator->ExtractPost($this->_disp );
			if ($disp_act == 'n')
				$this->_disp->SwitchToSubTid($extracted);
			return $extracted;
		}
		else if ($disp_act == 'D' )
		{
			$confdata->DeleteEntry($this->_disp);
			$needTrim = 1;
		}
		else if ( $disp_act == 'I' )
		{
			if ( $this->instantiateTemplate() ) {
				$needTrim = 1;
			}
		}

		$ctxseq = GUIBase::GrabGoodInputWithReset('get', 'ctxseq', 'int');
		if ($ctxseq != 0) {
			if ($this->_curOne->ChangeContextSeq($ctxseq))
				$needTrim = 1;
		}

		if ( $needTrim ) {
			$this->_disp->TrimLastId();
			// need reload
			$this->loadConfig();
			$confdata = $this->getConfData($this->_disp);
			$this->_disp->Set(DInfo::FLD_ConfData, $confdata);
		}

		return $confdata;
	}

	private function instantiateTemplate()
	{
		$tpname = $this->_disp->Get(DInfo::FLD_ViewName);

		$vhname = $this->_disp->GetLast(DInfo::FLD_REF);
		$s_tpnode = $this->_serv->GetChildNodeById('vhTemplate', $tpname);
		if ($s_tpnode == NULL)
			return FALSE;
		$s_vhnode = $s_tpnode->GetChildNodeById('member', $vhname);
		if ($s_vhnode == NULL)
			return FALSE;

		$confpath = $this->GetConfFilePath(DInfo::CT_TP, $tpname);
		$tp = new CData(DInfo::CT_TP, $confpath);
		if (($conferr = $tp->GetConfErr()) != NULL) {
			$this->_disp->Set(DInfo::FLD_TopMsg, $conferr);
			return FALSE;
		}
		$tproot = $tp->GetRootNode();

		$configfile = $tproot->GetChildVal('configFile');
		if ($configfile == NULL)
			return FALSE;
		$vhRoot_path = '';
		if ( strncasecmp('$VH_ROOT', $configfile, 8) == 0 ) {
			$vhRoot_path = $s_vhnode->GetChildVal('vhRoot'); // customized
			if ($vhRoot_path == NULL) {
				//default
				$vhRoot_path = $tproot->GetChildVal('vhRoot');
				if ($vhRoot_path == NULL)
					return FALSE;
			}
		}
		$configfile = PathTool::GetAbsFile($configfile, 'VR', $vhname, $vhRoot_path);
		$vh = new CData(DInfo::CT_VH, $configfile, "`$vhname");
		if (($conferr = $vh->GetConfErr()) != NULL) {
			$this->_disp->Set(DInfo::FLD_TopMsg, $conferr);
			return FALSE;
		}

		$vhroot = $tproot->GetChildren('virtualHostConfig');
		if ($vhroot == FALSE)
			return FALSE;
		$vh->SetRootNode($vhroot);
		$vh->SaveFile();

		// save serv file
		$basemap = new DTblMap(array('','*virtualhost$name'), 'V_TOPD');
		$tproot->AddChild(new CNode('name', $vhname));
		$tproot->AddChild(new CNode('note', "Instantiated from template $tpname"));
		$basemap->Convert(0, $tproot, 1, $this->_serv->GetRootNode());
		$s_vhnode->RemoveFromParent();

		$domains = $s_vhnode->GetChildVal('vhDomain');
		if ($domains == NULL)
			$domains = $vhname; // default
		if ( ($domainalias = $s_vhnode->GetChildVal('vhAliases')) != NULL )
			$domains .= ", $domainalias";

		$listeners = $s_tpnode->GetChildVal('listeners');
		$lns = preg_split("/, /", $listeners, -1, PREG_SPLIT_NO_EMPTY);

		foreach( $lns as $ln) {
			$listener = $this->_serv->GetChildNodeById('listener', $ln);
			if ($listener != NULL) {
				$vhmap = new CNode('vhmap', $vhname);
				$vhmap->AddChild(new CNode('domain', $domains));
				$listener->AddChild($vhmap);
			}
			else {
				error_log("cannot find listener $ln \n");
			}
		}
		$this->_serv->SaveFile();
		return TRUE;
	}

	public function EnableDisableVh($act, $actId)
	{
		$haschanged = FALSE;
		$cur_disabled = array();
		$key = 'suspendedVhosts';
		$curnode = $this->_serv->GetRootNode()->GetChildren($key);
		if ($curnode != NULL && $curnode->Get(CNode::FLD_VAL) != NULL)
			$cur_disabled = preg_split("/[,;]+/", $curnode->Get(CNode::FLD_VAL), -1, PREG_SPLIT_NO_EMPTY);

		$found = in_array($actId, $cur_disabled);
		if ($act == 'disable') {
			if (!$found) {
				$cur_disabled[] = $actId;
				$haschanged = TRUE;
			}
		}
		elseif ($act == 'enable') {
			if ($found) {
				$key = array_search($actId, $cur_disabled);
				unset($cur_disabled[$key]);
				$haschanged = TRUE;
			}
		}
		if ($haschanged) {
			$vals = implode(',', $cur_disabled);
			if ($curnode == NULL)
				$this->_serv->GetRootNode()->AddChild(new CNode($key, $vals));
			else
				$curnode->SetVal($vals);
			$this->_serv->SaveFile();
		}
	}

	public static function GetAdminEmails()
	{
		$cc = ConfControl::singleton();
		$emails = $cc->_serv->GetChildVal('adminEmails');
		return $emails;
	}

}

