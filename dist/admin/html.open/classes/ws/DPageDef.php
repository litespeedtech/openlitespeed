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
        if ( !isset($GLOBALS['_DPageDef_']) ) {
			$GLOBALS['_DPageDef_'] = new DPageDef();
        }
		return $GLOBALS['_DPageDef_'];
	}

	public function GetFileDef($type)
	{
		if ( !isset($this->_fileDef[$type]) )
		{
			$funcname = 'add_FilePage_' . $type;
			if (!method_exists($this, $funcname)) {
				die("invalid func name $funcname");
			}
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
				'SERV_PROCESS', 'APACHE_CONF_FILE', 'SERV_GENERAL',
				'SERV_LOG', 'SERV_ACLOG', 'SERV_INDEX',
				'A_HTACCESS', 'SERV_EXPIRES',
				'A_GEOIP', 'SERV_APACHECONF',
				'SERV_TUNING_OS', 'SERV_TUNING_CONN', 'SERV_TUNING_REQ', 'SERV_TUNING_STATIC', 'SERV_TUNING_GZIP',
				'SERV_SEC_FILE', 'SERV_SEC_CONN', 'SERV_SEC_CGI',
				'SERV_SEC_CC', 'A_SEC_CR',	'SERV_SEC_DENY',
				'A_SECAC', 'A_EXT_SEL',	'A_SCRIPT',
				'SERV_CACHE_STORAGE', 'A_CACHE_POLICY', 'SERV_CACHE_NCURL', 'SERV_CACHE_NCDOMAIN', 
				'S_RAILS', 'S_RAILS_HOSTING',
				'VH_TOP_D', 'SERVICE_SUSPENDVH'
				));

		$pages[] = new DFileSect(
			array('L_GENERAL', 'L_VHMAP', 'L_SSL_CERT', 'L_SSL', 'L_SSL_FEATURE', 'L_SSL_OCSP', 'L_SSL_CLIENT_VERIFY'),
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
				'VH_GENERAL', 'VH_LOG',	'VH_ACLOG',	'VH_INDXF',	'VH_ERRPG',
				'A_SCRIPT',	'A_HTACCESS', 'VH_EXPIRES', 'VH_APACHECONF',
				'VH_SECHL', 'VH_SEC_CC', 'A_SEC_CR', 'A_SECAC', 'VH_REALM_SEL',
				'A_CACHE_POLICY',
				'A_EXT_SEL', 'VH_CTX_SEL',
				'VH_REWRITE_CTRL', 'VH_REWRITE_MAP', 'VH_REWRITE_RULE',
				'VH_SSL_CERT', 'VH_SSL_SSL', 'VH_SSL_FEATURE', 'VH_SSL_OCSP', 'VH_SSL_CLIENT_VERIFY',
				'VH_FRONTPG', 'VH_AWSTATS'
				));
		$this->_fileDef['vh'] = &$pages;
	}

	protected function add_FilePage_tp()
	{
		$pages = array();
		$pages[] = new DFileSect(
			array('TP_GENERAL1',	'TP_SEC_FILE', 'TP_SEC_CONN', 'TP_SEC_CGI'));

		$pages[] = new DFileSect(
			array(
				'TP_GENERAL2', 'TP_LOG', 'TP_ACLOG', 'VH_INDXF', 'VH_ERRPG',
				'A_SCRIPT',	'A_HTACCESS', 'VH_EXPIRES', 'VH_APACHECONF',
				'VH_SECHL', 'VH_SEC_CC', 'A_SEC_CR', 'A_SECAC', 'TP_REALM_SEL',
				'A_CACHE_POLICY',
				'TP_EXT_SEL', 'VH_CTX_SEL',
				'VH_REWRITE_CTRL', 'VH_REWRITE_MAP', 'VH_REWRITE_RULE',
				'VH_SSL_CERT', 'VH_SSL_SSL', 'VH_SSL_FEATURE', 'VH_SSL_OCSP', 'VH_SSL_CLIENT_VERIFY',
				'VH_FRONTPG', 'TP_AWSTATS'),
			'virtualHostConfig');

		$this->_fileDef['tp'] = &$pages;
	}

	protected function add_FilePage_admin()
	{
		$pages = array();
		$pages[] = new DFileSect(
			array('ADMIN_PHP', 'VH_LOG', 'VH_ACLOG', 'A_SECAC'));

		$pages[] = new DFileSect(
			array('ADMIN_L_GENERAL', 'L_SSL_CERT', 'L_SSL', 'L_SSL_FEATURE', 'L_SSL_CLIENT_VERIFY'),
			'listenerList:listener', 'listeners', 'name' );

		$this->_fileDef['admin'] = &$pages;
	}

	protected function defineAll()
	{
		$page = new DPage('serv', 'general', 'General',
						  'Server General Settings',
						  array('SERV_PROCESS', 'SERV_GENERAL', 'SERV_INDEX', 'A_HTACCESS', 
						  	'SERV_EXPIRES', 'A_GEOIP_TOP', 'APACHE_CONF_FILE', 'SERV_APACHECONF'));
		$page->_helpLink = 'ServGeneral_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;

		$page = new DPage('serv', 'log', 'Log',
						  'Server Log Settings',
						  array('SERV_LOG', 'SERV_ACLOG'));
		$page->_helpLink = 'ServGeneral_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;


		$page = new DPage('serv', 'tuning', 'Tuning',
						  'Server Tuning Settings',
						  array('SERV_TUNING_OS', 'SERV_TUNING_CONN', 'SERV_TUNING_REQ', 'SERV_TUNING_STATIC', 'SERV_TUNING_GZIP'));
		$page->_helpLink = 'ServTuning_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;

		$page = new DPage('serv', 'security', 'Security',
						  'Server Security Settings',
						  array('SERV_SEC_FILE', 'SERV_SEC_CONN', 'SERV_SEC_CGI', 'SERV_SEC_DENY', 'A_SECAC'));
		$page->_helpLink = 'ServSecurity_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;

		$page = new DPage('serv', 'requestfilter', 'Request Filter',
						  'Server Request Filter Settings',
						  array('SERV_SEC_CC', 'A_SEC_CR_TOP'));
		$page->_helpLink = 'RequestFilter_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;

		$page = new DPage('serv', 'cache', 'Cache',
						  'Server Cache Settings',
						  array('SERV_CACHE_STORAGE', 'A_CACHE_POLICY', 'SERV_CACHE_NCURL', 'SERV_CACHE_NCDOMAIN'));
		$page->_helpLink = 'Cache_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;

		$page = new DPage('serv', 'ext', 'External App',
						  'Server Level External Applications',
						  array('A_EXT_TOP'));
		$page->_helpLink = 'ExtApp_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;

		$page = new DPage('serv', 'scriptHandler', 'Script Handler',
						  'Server Level Script Handler',
						  array('A_SCRIPT_TOP'));
		$page->_helpLink = 'ScriptHandler_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;

		$page = new DPage('serv', 'railsDefaults', 'Rack/Rails',
						  'Rack/Rails Settings', 
						  array('S_RAILS', 'S_RAILS_HOSTING'));
		$page->_helpLink = 'Rails_Help.html';
		$this->_pageDef['serv'][$page->_id] = $page;

		$page = new DPage('tptop', 'tpTop', 'Virtual Host Templates',
						  'Virtual Host Templates',
						  array('TP_TOP'));
		$page->_helpLink = 'Templates_Help.html';
		$this->_pageDef['tptop'][$page->_id] = $page;

		$page = new DPage('vhtop', 'vhTop', 'Virtual Hosts',
						  'Virtual Hosts Settings',
						  array('VH_TOP'));
		$page->_helpLink = 'VirtualHosts_Help.html';
		$this->_pageDef['vhtop'][$page->_id] = $page;

		$page = new DPage('sltop', 'slTop', 'Listeners',
						  'Listeners Settings',
						  array('L_TOP' ));
		$page->_helpLink = 'Listeners_Help.html';
		$this->_pageDef['sltop'][$page->_id] = $page;

		$page = new DPage('sl', 'lgeneral', 'General',
						  'Listener General Settings',
						  array('L_GENERAL', 'L_VHMAP_TOP'));
		$page->_helpLink = 'Listeners_Help.html';
		$this->_pageDef['sl'][$page->_id] = $page;

		$page = new DPage('sl', 'lsecure', 'SSL',
						  'Listener SSL Settings',
						  array('L_SSL_CERT', 'L_SSL', 'L_SSL_FEATURE', 'L_SSL_OCSP', 'L_SSL_CLIENT_VERIFY'));
		$page->_helpLink = 'Listeners_Help.html';
		$this->_pageDef['sl'][$page->_id] = $page;

		$page = new DPage('tp', 'member', 'Template',
						  'Template Settings',
						  array('TP', 'TP_MEMBER_TOP'));
		$page->_helpLink = 'Templates_Help.html';
		$this->_pageDef['tp'][$page->_id] = $page;

		$page = new DPage('tp', 'general', 'General',
						  'Template General Settings',
						  array('TP_GENERAL', 'TP_LOG', 'TP_ACLOG', 'VH_INDXF',
								'VH_ERRPG_TOP', 'A_SCRIPT_TOP', 'A_HTACCESS', 'VH_EXPIRES', 'VH_APACHECONF'));
		$page->_helpLink = 'VHGeneral_Help.html';
		$this->_pageDef['tp'][$page->_id] = $page;

		$page = new DPage('tp', 'security', 'Security',
						  'Tempage Security Settings',
						  array('TP_SEC_FILE', 'TP_SEC_CONN', 'TP_SEC_CGI',
								'VH_SECHL',	'VH_SEC_CC', 'A_SEC_CR_TOP', 'A_SECAC', 'TP_REALM_TOP'));
		$page->_helpLink = 'VHSecurity_Help.html';
		$this->_pageDef['tp'][$page->_id] = $page;

		$page = new DPage('tp', 'ext', 'External App',
						  'Template External Applications',
						  array('TP_EXT_TOP'));
		$page->_helpLink = 'ExtApp_Help.html';
		$this->_pageDef['tp'][$page->_id] = $page;

		$page = new DPage('vh', 'base', 'Basic',
						  'Virtual Host Base',
						  array('VH_BASE', 'VH_BASE_CONNECTION','VH_BASE_SECURITY', 'VH_BASE_THROTTLE'));
						  
		$page->_helpLink = 'VHGeneral_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;
		
		$page = new DPage('vh', 'general', 'General',
						  'Virtual Host General Settings',
						  array('VH_GENERAL', 'VH_INDXF', 'VH_ERRPG_TOP', 'A_HTACCESS', 'VH_EXPIRES', 'VH_APACHECONF'));
		$page->_helpLink = 'VHGeneral_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;

		$page = new DPage('vh', 'log', 'Log',
						  'Virtual Host Log Settings',
						  array('VH_LOG', 'VH_ACLOG'));
		$page->_helpLink = 'VHGeneral_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;

		$page = new DPage('vh', 'security', 'Security',
						  'Virtual Host Security Settings',
						  array( 'VH_SECHL', 'A_SECAC', 'VH_REALM_TOP')); //post beta 'VH_SEC_ACCESS', 'VH_SEC_THROTTLE', 'VH_SEC_EXTAPP',
		$page->_helpLink = 'VHSecurity_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;

		$page = new DPage('vh', 'requestfilter', 'Request Filter',
						  'Virtual Host Request Filter Settings',
						  array('VH_SEC_CC', 'A_SEC_CR_TOP'));
		$page->_helpLink = 'VHRequestFilter_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;

		$page = new DPage('vh', 'cache', 'Cache',
						  'Virtual Host Cache Settings',
						  array('A_CACHE_POLICY'));
		$page->_helpLink = 'Cache_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;

		$page = new DPage('vh', 'ext', 'External App',
						  'Virtual Host Level External Applications',
						  array('A_EXT_TOP'));
		$page->_helpLink = 'ExtApp_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;

		$page = new DPage('vh', 'scriptHandler', 'Script Handler',
						  'Virtual Host Script Handler',
						  array('A_SCRIPT_TOP'));
		$page->_helpLink = 'VHGeneral_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;

		$page = new DPage('vh', 'rewrite', 'Rewrite',
						  'Virtual Host Rewrite Settings',
						  array('VH_REWRITE_CTRL', 'VH_REWRITE_MAP_TOP', 'VH_REWRITE_RULE'));
		$page->_helpLink = 'Rewrite_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;
		$this->_pageDef['tp'][$page->_id] = $page;

		$page = new DPage('vh', 'context', 'Context',
						  'Virtual Host Context Settings',
						  array('VH_CTX_TOP'));
		$page->_helpLink = 'Context_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;
		$this->_pageDef['tp'][$page->_id] = $page;

		$page = new DPage('vh', 'vhssl', 'SSL',
						  'Virtual Host SSL Settings',
						  array('VH_SSL_CERT', 'VH_SSL_SSL', 'VH_SSL_FEATURE', 'VH_SSL_OCSP', 'VH_SSL_CLIENT_VERIFY'));
		$page->_helpLink = 'Listeners_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;
		$this->_pageDef['tp'][$page->_id] = $page;

		$page = new DPage('vh', 'addons', 'Add-ons',
						  'Virtual Host Add-ons Settings',
						  array('VH_FRONTPG', 'VH_AWSTATS'));
		$page->_helpLink = 'VHAddOns_Help.html';
		$this->_pageDef['vh'][$page->_id] = $page;

		$page = new DPage('tp', 'addons', 'Add-ons',
						  'Virtual Host Add-ons Settings',
						  array('VH_FRONTPG', 'TP_AWSTATS'));
		$page->_helpLink = 'VHAddOns_Help.html';
		$this->_pageDef['tp'][$page->_id] = $page;

		$page = new DPage('admin', 'general', 'General',
						  'Web Admin Settings',
						  array('ADMIN_PHP', 'VH_LOG', 'VH_ACLOG'));
		$page->_helpLink = 'AdminGeneral_Help.html';
		$this->_pageDef['admin'][$page->_id] = $page;

		$page = new DPage('admin', 'security', 'Security',
						  'Web Admin Security Settings',
						  array('A_SECAC', 'ADMIN_USR_TOP'));
		$page->_helpLink = 'AdminGeneral_Help.html';
		$this->_pageDef['admin'][$page->_id] = $page;

		$page = new DPage('altop', 'alTop', 'Admin Listeners',
						  'Listeners Settings',
						  array('ADMIN_L_TOP' ));
		$page->_helpLink = 'AdminListener_Help.html';
		$this->_pageDef['altop'][$page->_id] = $page;

		$page = new DPage('al', 'lgeneral', 'General',
						  'Admin Listener General Settings',
						  array('ADMIN_L_GENERAL'));
		$page->_helpLink = 'AdminListener_Help.html';
		$this->_pageDef['al'][$page->_id] = $page;

		$page = new DPage('al', 'lsecure', 'SSL',
						  'Listener SSL Settings',
						  array('L_SSL_CERT', 'L_SSL', 'L_SSL_FEATURE', 'L_SSL_CLIENT_VERIFY'));
		$page->_helpLink = 'AdminListener_Help.html';
		$this->_pageDef['al'][$page->_id] = $page;		

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
