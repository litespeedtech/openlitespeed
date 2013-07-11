<?php

class DPageDef
{
	protected $_pageDef = array();
	protected $_fileDef = array();
	protected $_tabDef = array();

	protected function __construct()
	{
		$this->defineAll();
	}

	public static function GetInstance()
	{
        if ( !isset($GLOBALS['_DPageDef_']) )
			$GLOBALS['_DPageDef_'] = new DPageDef();
		return $GLOBALS['_DPageDef_'];
	}

	public function GetFileDef($type)
	{
		if ( !isset($this->_fileDef[$type]) )
		{
			$funcname = 'add_FilePage_' . $type;
			$this->$funcname();
		}
		return $this->_fileDef[$type];
	}

	public function GetPageDef($type, $pid)
	{
		return $this->_pageDef[$type][$pid];
	}

	public function GetTabDef($type)
	{
		return $this->_tabDef[$type];
	}

	protected function add_FilePage_serv()
	{
		$pages = array();
		$pages[] = new DFileSect(
			array(
				'SERV_PROCESS', 'SERV_GENERAL', 
				'SERV_LOG', 'SERV_ACLOG', 'A_GEOIP',
				'SERV_TUNING_CONN', 'SERV_TUNING_REQ', 'SERV_TUNING_GZIP',
				'SERV_SEC_CONN', 'A_SEC_CC', 'A_SEC_CR',
				'A_SECAC', 'VH_TOP_D', 'SERVICE_SUSPENDVH'
				));

		$pages[] = new DFileSect(
			array('LB_GENERAL', 'LB_SESSION', 'LB_ERRMSG', 'LB_WORKERGRP'), 
			'loadBalancerList:loadBalancer', 'clusters', 'name' );
				
		$pages[] = new DFileSect(
			array('L_GENERAL', 'L_VHMAP', 'L_CERT', 'L_SSL'), 
			'listenerList:listener', 'listeners', 'name' );

		$pages[] = new DFileSect(
			array('TP', 'TP_MEMBER'),
			'vhTemplateList:vhTemplate', 'tpTop', 'name');

		$this->_fileDef['serv'] = &$pages;
	}

	protected function add_FilePage_vh()
	{
		$pages = array();
		$pages[] = new DFileSect(
			array(
				'VH_GENERAL', 'VH_ACLOG', 'VH_ERRPG',
				'TP_SEC_CONN', 'VH_SECHL', 'A_SECAC', 'VH_REALM_SEL',
				'A_SEC_CC', 'A_SEC_CR',
				'VH_CTX_SEL',
				'VH_REWRITE_CTRL', 'VH_REWRITE_MAP', 'VH_REWRITE_RULE'
				));	
		$this->_fileDef['vh'] = &$pages;
	}

	protected function add_FilePage_tp()
	{
		$pages = array();
		$pages[] = new DFileSect(
			array('TP_GENERAL1'));

		$pages[] = new DFileSect(
			array(
				'TP_GENERAL2', 'TP_ACLOG', 'VH_ERRPG',
				'TP_SEC_CONN', 'VH_SECHL', 'A_SECAC', 'TP_REALM_SEL', 
				'A_SEC_CC', 'A_SEC_CR',
				'VH_CTX_SEL',
				'VH_REWRITE_CTRL', 'VH_REWRITE_MAP', 'VH_REWRITE_RULE'
				),
			'virtualHostConfig');

		$this->_fileDef['tp'] = &$pages;
	}
	
	protected function add_FilePage_admin()
	{
		$pages = array();
		$pages[] = new DFileSect(
			array('ADMIN_PHP', 'VH_LOG', 'VH_ACLOG', 'A_SECAC'));

		$pages[] = new DFileSect(
			array('ADMIN_L_GENERAL', 'L_CERT', 'L_SSL'),
			'listenerList:listener', 'listeners', 'name' );

		$this->_fileDef['admin'] = &$pages;
	}

    protected function defineAll()
	{
		$type = 'serv';

		$page = new DPage($type, 'general', 'General',
						  'Server General Settings',
						  array('SERV_PROCESS',	'SERV_GENERAL', 'A_GEOIP_TOP'));
		$page->_helpLink = 'LB_ServGeneral_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'log', 'Log',
						  'Server Log Settings',
						  array('SERV_LOG', 'SERV_ACLOG'));
		$page->_helpLink = 'LB_ServGeneral_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'tuning', 'Tuning',
						  'Server Tuning Settings',
						  array('SERV_TUNING_CONN', 'SERV_TUNING_REQ', 'SERV_TUNING_GZIP'));
		$page->_helpLink = 'LB_ServTuning_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'security', 'Security',
						  'Server Security Settings',
						  array('SERV_SEC_CONN', 'A_SECAC'));
		$page->_helpLink = 'LB_ServSecurity_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'requestfilter', 'Request Filter',
						  'Server Request Filter Settings',
						  array('A_SEC_CC', 'A_SEC_CR_TOP'));
		$page->_helpLink = 'RequestFilter_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type = 'lbtop';
		$page = new DPage($type, 'cluster', 'Clusters',
						  'Clusters',
						  array('LB_TOP'));
		$page->_helpLink = 'LB_LoadBalancer_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type = 'lb';
		$page = new DPage($type, 'lbgeneral', 'General',
						  'Load Balancer General Settings',
						  array('LB_GENERAL', 'LB_SESSION', 'LB_ERRMSG'),
						  'lb', 'mid');
		$page->_helpLink = 'LB_LoadBalancer_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'lbworker', 'Worker Group',
						  'Load Balancer Worker Group Settings',
						  array('LB_WORKERGRP_TOP'),
						  'lb', 'mid');
		$page->_helpLink = 'LB_LoadBalancer_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type = 'tptop';
		$page = new DPage($type, 'tpTop', 'Virtual Host Templates',
						  'Virtual Host Templates',
						  array('TP_TOP'));
		$page->_helpLink = 'Templates_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type = 'vhtop';
		$page = new DPage($type, 'vhTop', 'Virtual Hosts',
						  'Virtual Hosts Settings',
						  array('VH_TOP'));
		$page->_helpLink = 'LB_VirtualHosts_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type = 'sltop';
		$page = new DPage($type, 'slTop', 'Server Listeners',
						  'Listeners Settings',
						  array('L_TOP' ));
		$page->_helpLink = 'Listeners_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type = 'sl';
		$page = new DPage($type, 'lgeneral', 'General',
						  'Listener General Settings',
						  array('L_GENERAL', 'L_VHMAP_TOP'),
						  'listeners', 'mid');
		$page->_helpLink = 'Listeners_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'lsecure', 'SSL',
						  'Listener SSL Settings',
						  array('L_CERT', 'L_SSL'),
						  'listeners', 'mid');
		$page->_helpLink = 'Listeners_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type1 = 'tp';
		$type = 'vh';
	
		$page = new DPage($type, 'base', 'Basic',
						  'Virtual Host Base',
						  array('VH_BASE'));
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type1, 'member', 'Template',
						  'Template Settings',
						  array('TP', 'TP_MEMBER_TOP'));
		$page->_helpLink = 'Templates_Help.html';
		$this->_pageDef[$type1][$page->_id] = $page;

		$page = new DPage($type1, 'general', 'General',
						  'Template General Settings',
						  array('TP_GENERAL','TP_ACLOG',
						        'VH_ERRPG_TOP'));
		$page->_helpLink = 'VHGeneral_Help.html';
		$this->_pageDef[$type1][$page->_id] = $page;

		$page = new DPage($type, 'security', 'Security',
						  'Virtual Host Security Settings',
				  array('TP_SEC_CONN','VH_SECHL','A_SECAC', 'VH_REALM_TOP'));
		$page->_helpLink = 'VHSecurity_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type1, 'security', 'Security',
						  'Template Security Settings',
						  array('TP_SEC_CONN','VH_SECHL','A_SECAC', 'VH_REALM_TOP'));
		$page->_helpLink = 'VHSecurity_Help.html';
		$this->_pageDef[$type1][$page->_id] = $page;

		$page = new DPage($type, 'requestfilter', 'Request Filter',
						  'Template Request Filter Settings',
						  array('A_SEC_CC', 'A_SEC_CR_TOP'));
		$page->_helpLink = 'RequestFilter_Help.html';
		$this->_pageDef[$type1][$page->_id] = $page;
		
		$page = new DPage($type, 'general', 'General',
						  'Virtual Host General Settings',
						  array('VH_GENERAL', 'VH_ACLOG',
 								'VH_ERRPG_TOP' ));
		$page->_helpLink = 'LB_VHGeneral_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'requestfilter', 'Request Filter',
						  'VHost Request Filter Settings',
						  array('A_SEC_CC', 'A_SEC_CR_TOP'));
		$page->_helpLink = 'RequestFilter_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;
		
		$page = new DPage($type, 'rewrite', 'Rewrite',
						  'Virtual Host Rewrite Settings',
						  array('VH_REWRITE_CTRL', 'VH_REWRITE_MAP_TOP',
								'VH_REWRITE_RULE'));
		$page->_helpLink = 'LB_Rewrite_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;
		$this->_pageDef[$type1][$page->_id] = $page;

		$page = new DPage($type, 'context', 'Context',
						  'Virtual Host Context Settings',
						  array('VH_CTX_TOP'));
		$page->_helpLink = 'LB_Context_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;
		$this->_pageDef[$type1][$page->_id] = $page;

		$type = 'admin';
		$page = new DPage($type, 'general', 'General',
						  'Web Admin Settings',
						  array('ADMIN_PHP', 'VH_LOG', 'VH_ACLOG'));
		$page->_helpLink = 'AdminGeneral_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'security', 'Security',
						  'Web Admin Security Settings',
						  array('A_SECAC', 'ADMIN_USR_TOP'));
		$page->_helpLink = 'AdminGeneral_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type = 'altop';
		$page = new DPage($type, 'alTop', 'Admin Listeners',
						  'Listeners Settings',
						  array('ADMIN_L_TOP' ));
		$page->_helpLink = 'AdminListener_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$type = 'al';
		$page = new DPage($type, 'lgeneral', 'General',
						  'Admin Listener General Settings',
						  array('ADMIN_L_GENERAL'),
						  'listeners', 'mid');
		$page->_helpLink = 'AdminListener_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$page = new DPage($type, 'lsecure', 'SSL',
						  'Listener SSL Settings',
						  array('L_CERT', 'L_SSL'),
						  'listeners', 'mid');
		$page->_helpLink = 'AdminListener_Help.html';
		$this->_pageDef[$type][$page->_id] = $page;

		$types = array_keys($this->_pageDef);
		foreach ( $types as $type )
		{
			$pids = array_keys( $this->_pageDef[$type] );
			foreach ( $pids as $pid )
			{
				$this->_tabDef[$type][$pid] = $this->_pageDef[$type][$pid]->_name;
			}
		}

	}


}
