<?php

class DispInfo
{
	var $_ctrlUrl;

	var $_mid;
	var $_pid;
	var $_tid;
	var $_ref;
	var $_act;
	var $_token;
	var $_tokenInput;
	var $_sort;

	var $_type;
	var $_name;
	var $_tabs;

	var $_titleLabel;
	var $_info;
	var $_err;

	function __construct($url)
	{
		$this->_ctrlUrl = $url;

		$has_mid = FALSE;
		$has_pid = FALSE;

		$this->_mid = $this->_pid = $this->_tid = $this->_ref = NULL;

		$this->_mid = DUtil::getGoodVal(DUtil::grab_input("request",'m'));
		
		if ( $this->_mid == NULL ) { 
			$this->_mid = 'serv';
		}
		else {
			$has_mid = TRUE;
		}
		 
		$pos = strpos($this->_mid, '_');
		if ( $pos > 0 )
		{
			$this->_type = substr($this->_mid, 0, $pos);
			$this->_name = substr($this->_mid, $pos+1);
		}
		else
		{
			$this->_type = $this->_mid ;
			$this->_name = NULL;
		}
		$this->_info['mid'] = $this->_name;

		$this->_tabs = DPageDef::GetInstance()->GetTabDef($this->_type);

		if(!is_array($this->_tabs)) {
			die("Invalid pid");
		}
		$pids = array_keys($this->_tabs);
		
		if ($has_mid) {
			$this->_pid = DUtil::getGoodVal(DUtil::grab_input("request",'p'));
		}
		if ( $this->_pid == NULL)
		{	
			// get default
			$this->_pid = $pids[0];
		}
		else {
			if (!in_array($this->_pid, $pids))	
				die("Invalid pid");
			$has_pid = TRUE;
		}
		
		if ( $has_pid && !isset($_REQUEST['t0'])
			 && isset($_REQUEST['t']) )
		{
			$this->_tid = DUtil::getGoodVal($_REQUEST['t']);

			if ( isset($_GET['t1'])
				 && ( DUtil::getLastId($this->_tid) != DUtil::getGoodVal1($_GET['t1']) ) )
				$this->_tid .= '`' . $_GET['t1'];

			if ( isset($_REQUEST['r']) )
				$this->_ref = DUtil::getGoodVal1($_REQUEST['r']);
			if ( isset($_GET['r1']) )
				$this->_ref .= '`' . DUtil::getGoodVal1($_GET['r1']);
		}

		$this->_act = DUtil::getGoodVal(DUtil::grab_input("request",'a'));
		if ( $this->_act == NULL ) {
			$this->_act = 'v';
		}
		$this->_tokenInput = DUtil::getGoodVal(DUtil::grab_input("request",'tk'));
		$client = CLIENT::singleton();
		$this->_token = $client->token;
		
		if ( isset($_GET['sort']) )
		{
			$sort = DUtil::getGoodVal1($_GET['sort']);
			$pos = strpos($sort, '`');
			$this->_sort[0] = substr($sort, 0, $pos);//tbl
			$this->_sort[1] = substr($sort, $pos+1, 1);//ascend
			$this->_sort[2] = substr($sort, $pos+2);//key
		}
		else
		{
			$this->_sort[0] = 0;
		}

		
	}

	function init($serverName='')
	{
		$label = 'Error';
		switch ( $this->_type )
		{
			case 'serv':	
				$label = 'Server &#187; ' . $serverName;
				break;
			case 'sltop':
			case 'altop':	
				$label = 'Listeners';
				break;
			case 'sl':
			case 'al':		
				$label = 'Listener &#187; ' . $this->_name;
				break;
			case 'vhtop':	
				$label = 'Virtual Hosts';
				break;
			case 'vh':		
				$label = 'Virtual Host &#187; '. $this->_name;
				break;
			case 'tptop':	
				$label = 'Virtual Host Templates';
				break;
			case 'tp':		
				$label = 'Virtual Host Template &#187; '. $this->_name;
				break;
			case 'lbtop':
				$label = 'Clusters';
				break;
			case 'lb':
				$label = 'Cluster &#187; '.$this->_name;
				break;
			case 'admin':	
				$label = 'Admin Server';
				break;
		}
		$this->_titleLabel = $label;

	}


}

