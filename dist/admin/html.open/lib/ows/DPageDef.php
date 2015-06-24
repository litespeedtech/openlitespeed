<?php

class DPageDef
{
	protected $_pageDef = array();
	protected $_fileDef = array();

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

	public static function GetPage($dinfo)
	{
		$pagedef = DPageDef::GetInstance();
		$type = $dinfo->Get(DInfo::FLD_View);
		$pid = $dinfo->Get(DInfo::FLD_PID);
		return $pagedef->_pageDef[$type][$pid];
	}

	public function GetFileMap($type)
	{
		if ( !isset($this->_fileDef[$type]) )
		{
			$funcname = 'add_FileMap_' . $type;
			if (!method_exists($this, $funcname)) {
				die("invalid func name $funcname");
			}
			$this->$funcname();
		}
		return $this->_fileDef[$type];
	}

	private function add_FileMap_serv()
	{
		$map = new DTblMap(array('httpServerConfig', ''), // loadBalancerConfig
				array('S_PROCESS',
						'S_GENERAL',
						new DTblMap(array('logging:log','errorlog$fileName'), 'S_LOG'),
						new DTblMap(array('logging:accessLog','accesslog$fileName'), 'S_ACLOG'),
						'S_INDEX',
						new DTblMap('expires', 'A_EXPIRES'),
                        'S_FILEUPLOAD',
						new DTblMap(array('ipToGeo:geoipDB','*geoipdb$geoipDBFile'), 'S_GEOIP'),
						new DTblMap('tuning',
								array('S_TUNING_OS', 'S_TUNING_CONN', 'S_TUNING_REQ', 'S_TUNING_STATIC', 'S_TUNING_GZIP')),
						new DTblMap(array('security:fileAccessControl','fileAccessControl'), 'S_SEC_FILE'),
						new DTblMap(array('security:perClientConnLimit','perClientConnLimit'), 'S_SEC_CONN'),
						new DTblMap(array('security:CGIRLimit','CGIRLimit'), 'S_SEC_CGI'),
						new DTblMap(array('security:accessDenyDir','accessDenyDir'), 'S_SEC_DENY'),
						new DTblMap(array('security:accessControl','accessControl'), 'A_SEC_AC'),
						new DTblMap(array('extProcessorList:*extProcessor','*extprocessor$name'), 'A_EXT_SEL'),
						new DTblMap(array('scriptHandlerList','scripthandler'),
								new DTblMap(array('*scriptHandler','*addsuffix$suffix'), 'A_SCRIPT')),
						new DTblMap('railsDefaults', 'S_RAILS'),
						new DTblMap(array('moduleList:*module','*module$name'), 'S_MOD'),
						new DTblMap(array('virtualHostList:*virtualHost','*virtualhost$name'), 'V_TOPD'),
						new DTblMap(array('listenerList:*listener','*listener$name'),
								array('L_GENERAL',
										new DTblMap(array('vhostMapList:*vhostMap','*vhmap$vhost'), 'L_VHMAP'),
										'LVT_SSL_CERT', 'LVT_SSL', 'L_SSL_FEATURE', 'LVT_SSL_OCSP', 'LVT_SSL_CLVERIFY',
										new DTblMap(array('moduleList:*module','*module$name'), 'L_MOD'))),
						new DTblMap(array('vhTemplateList:*vhTemplate','*vhTemplate$name'),
								array('T_TOPD', new DTblMap(array('*member','*member$vhName'), 'T_MEMBER'))),
						'SERVICE_SUSPENDVH'	));

		$this->_fileDef['serv'] = $map;
	}

	private function add_FileMap_vh_()
	{
		$map = new DTblMap(array('virtualHostConfig', ''),
				array('V_GENERAL',
						new DTblMap(array('logging:log','errorlog$fileName'), 'V_LOG'),
						new DTblMap(array('logging:accessLog','accesslog$fileName'), 'V_ACLOG'),
						new DTblMap('index', 'VT_INDXF'),
						new DTblMap(array('customErrorPages:errorPage', '*errorpage$errCode'), 'VT_ERRPG'),
						new DTblMap(array('scriptHandlerList','scripthandler'),
								new DTblMap(array('*scriptHandler','*addsuffix$suffix'), 'A_SCRIPT')),
						new DTblMap('expires', 'A_EXPIRES'),
                    'VT_FILEUPLOAD',
						new DTblMap(array('security:accessControl','accessControl'), 'A_SEC_AC'),
						new DTblMap(array('security:realmList:*realm','*realm$name'), 'V_REALM_FILE'),
						new DTblMap(array('extProcessorList:*extProcessor','*extprocessor$name'), 'A_EXT_SEL'),
						new DTblMap(array('contextList:*context', '*context$uri'), 'VT_CTX_SEL'),
						new DTblMap('rewrite',
								array('VT_REWRITE_CTRL',
										new DTblMap(array('*map', '*map$name'), 'VT_REWRITE_MAP'),
						'VT_REWRITE_RULE')),
						new DTblMap('vhssl',
								array('LVT_SSL_CERT', 'LVT_SSL', 'VT_SSL_FEATURE', 'LVT_SSL_OCSP', 'LVT_SSL_CLVERIFY')),
						new DTblMap(array('websocketList:*websocket', '*websocket$uri'), 'VT_WBSOCK'),
						new DTblMap(array('moduleList:*module','*module$name'),
								array('VT_MOD',
										new DTblMap(array('urlFilterList:*urlFilter', '*urlFilter$uri'), 'VT_MOD_FILTER')))
						));

		$this->_fileDef['vh_'] = $map;
	}

	private function add_FileMap_tp_()
	{
		$map = new DTblMap(array('virtualHostTemplate', ''),
				array('T_GENERAL1', 'T_SEC_FILE', 'T_SEC_CONN', 'T_SEC_CGI',
						new DTblMap('virtualHostConfig',
								array('T_GENERAL2',
										new DTblMap(array('logging:log','errorlog$fileName'), 'T_LOG'),
										new DTblMap(array('logging:accessLog','accesslog$fileName'), 'T_ACLOG'),
										new DTblMap('index', 'VT_INDXF'),
										new DTblMap(array('customErrorPages:errorPage', '*errorpage$errCode'), 'VT_ERRPG'),
										new DTblMap(array('scriptHandlerList','scripthandler'),
												new DTblMap(array('*scriptHandler','*addsuffix$suffix'), 'A_SCRIPT')),
										new DTblMap('expires', 'A_EXPIRES'),
										new DTblMap(array('security:accessControl','accessControl'), 'A_SEC_AC'),
										new DTblMap(array('security:realmList:*realm','*realm$name'), 'T_REALM_FILE'),
										new DTblMap(array('extProcessorList:*extProcessor','*extprocessor$name'), 'T_EXT_SEL'),
										new DTblMap(array('contextList:*context', '*context$uri'), 'VT_CTX_SEL'),
										new DTblMap('rewrite',
												array('VT_REWRITE_CTRL',
														new DTblMap(array('*map', '*map$name'), 'VT_REWRITE_MAP'),
										'VT_REWRITE_RULE')),
										new DTblMap('vhssl',
												array('LVT_SSL_CERT', 'LVT_SSL', 'VT_SSL_FEATURE', 'LVT_SSL_OCSP', 'LVT_SSL_CLVERIFY')),
										new DTblMap(array('websocketList:*websocket', '*websocket$uri'), 'VT_WBSOCK'),
										new DTblMap(array('moduleList:*module','*module$name'),
												array('VT_MOD',
														new DTblMap(array('urlFilterList:*urlFilter', '*urlFilter$uri'), 'VT_MOD_FILTER')))
								))));

		$this->_fileDef['tp_'] = $map;
	}

	private function add_FileMap_admin()
	{
		$map = new DTblMap(array('adminConfig', ''),
				array('ADM_PHP',
						new DTblMap(array('logging:log','errorlog$fileName'), 'V_LOG'),
						new DTblMap(array('logging:accessLog','accesslog$fileName'), 'ADM_ACLOG'),
						new DTblMap(array('security:accessControl','accessControl'), 'A_SEC_AC'),
						new DTblMap(array('listenerList:*listener','*listener$name'),
								array('ADM_L_GENERAL', 'LVT_SSL_CERT', 'LVT_SSL', 'L_SSL_FEATURE', 'LVT_SSL_CLVERIFY'))
				));

		$this->_fileDef['admin'] = $map;
	}

	public function GetTabDef($view)
	{
		if(!isset($this->_pageDef[$view]))
			die("Invalid tabs $view");

		$tabs = array();
		foreach ($this->_pageDef[$view] as $p) {
			$tabs[$p->GetID()] = $p->GetLabel();
		}
		return $tabs;
	}

	protected function defineAll()
	{
		$id = 'g';
		$page = new DPage($id, DMsg::UIStr('tab_g'), new DTblMap('',
				array('S_PROCESS','S_GENERAL','S_INDEX',
						new DTblMap('expires', 'A_EXPIRES'),
                        'S_FILEUPLOAD',
						new DTblMap('*geoipdb$geoipDBFile', 'S_GEOIP_TOP', 'S_GEOIP')),
				new DTblMap('*index', array('S_MIME_TOP', 'S_MIME'))));
		$this->_pageDef['serv'][$id] = $page;

		$id = 'log';
		$page = new DPage($id, DMsg::UIStr('tab_log'), new DTblMap('',
				array(new DTblMap('errorlog$fileName', 'S_LOG'),
						new DTblMap('accesslog$fileName', 'S_ACLOG'))));
		$this->_pageDef['serv'][$id] = $page;

		$id = 'tuning';
		$page = new DPage($id, DMsg::UIStr('tab_tuning'), new DTblMap('tuning',
				array('S_TUNING_OS', 'S_TUNING_CONN', 'S_TUNING_REQ', 'S_TUNING_STATIC', 'S_TUNING_GZIP')));
		$this->_pageDef['serv'][$id] = $page;

		$id = 'sec';
		$page = new DPage($id, DMsg::UIStr('tab_sec'), new DTblMap('',
				array(new DTblMap('fileAccessControl', 'S_SEC_FILE'),
						new DTblMap('perClientConnLimit', 'S_SEC_CONN'),
						new DTblMap('CGIRLimit', 'S_SEC_CGI'),
						new DTblMap('accessDenyDir', 'S_SEC_DENY'),
						new DTblMap('accessControl', 'A_SEC_AC'))));
		$this->_pageDef['serv'][$id] = $page;

		$id = 'ext';
		$page = new DPage($id, DMsg::UIStr('tab_ext'),
				new DTblMap('*extprocessor$name', 'A_EXT_TOP',
						array('A_EXT_SEL', 'A_EXT_FCGI', 'A_EXT_FCGIAUTH', 'A_EXT_LSAPI', 'A_EXT_SERVLET', 'A_EXT_PROXY', 'A_EXT_LOGGER', 'A_EXT_LOADBALANCER')));
		$this->_pageDef['serv'][$id] = $page;

		$id = 'sh';
		$page = new DPage($id, DMsg::UIStr('tab_sh'), new DTblMap('scripthandler:*addsuffix$suffix', 'A_SCRIPT_TOP', 'A_SCRIPT'));
		$this->_pageDef['serv'][$id] = $page;

		$id = 'rails';
		$page = new DPage($id, DMsg::UIStr('tab_rails'), new DTblMap('railsDefaults', 'S_RAILS'));
		$this->_pageDef['serv'][$id] = $page;

		$id = 'mod';
		$page = new DPage($id, DMsg::UIStr('tab_mod'), new DTblMap('*module$name', 'S_MOD_TOP', 'S_MOD'));
		$this->_pageDef['serv'][$id] = $page;

		$id = 'top';
		$page = new DPage($id, DMsg::UIStr('tab_top'), new DTblMap('*listener$name', 'L_TOP', 'L_GENERAL'));
		$this->_pageDef['sl'][$id] = $page;

		$id = 'lg';
		$page = new DPage($id, DMsg::UIStr('tab_g'), new DTblMap('*listener$name',
				array('L_GENERAL', new DTblMap('*vhmap$vhost', 'L_VHMAP_TOP', 'L_VHMAP'))));
		$this->_pageDef['sl_'][$id] = $page;

		$id = 'lsec';
		$page = new DPage($id, DMsg::UIStr('tab_ssl'), new DTblMap('*listener$name',
				array('LVT_SSL_CERT', 'LVT_SSL', 'L_SSL_FEATURE', 'LVT_SSL_OCSP', 'LVT_SSL_CLVERIFY')));
		$this->_pageDef['sl_'][$id] = $page;

		$id = 'lmod';
		$page = new DPage($id, DMsg::UIStr('tab_mod'), new DTblMap('*listener$name', new DTblMap('*module$name', 'L_MOD_TOP', 'L_MOD')));
		$this->_pageDef['sl_'][$id] = $page;

		$id = 'top';
		$page = new DPage($id, DMsg::UIStr('tab_top'), new DTblMap('*virtualhost$name', 'V_TOP', 'V_TOPD'));
		$this->_pageDef['vh'][$id] = $page;

		//$id = 'top';
		$page = new DPage($id, DMsg::UIStr('tab_top'), new DTblMap('*vhTemplate$name', 'T_TOP', 'T_TOPD'));
		$this->_pageDef['tp'][$id] = $page;

		$id = 'mbr';
		$page = new DPage($id, DMsg::UIStr('tab_tp'), new DTblMap('*vhTemplate$name',
				array('T_TOPD', new DTblMap('*member$vhName', 'T_MEMBER_TOP', 'T_MEMBER'))));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'base';
		$page = new DPage($id, DMsg::UIStr('tab_base'), new DTblMap('*virtualhost$name',
				array('V_BASE', 'V_BASE_CONN','V_BASE_SEC', 'V_BASE_THROTTLE')));
		$this->_pageDef['vh_'][$id] = $page;

		$id = 'g';
		$page = new DPage($id, DMsg::UIStr('tab_g'), new DTblMap('',
				array('V_GENERAL',
						new DTblMap('index', 'VT_INDXF'),
						new DTblMap('*errorpage$errCode', 'VT_ERRPG_TOP', 'VT_ERRPG'),
						new DTblMap('expires', 'A_EXPIRES'),
                    'VT_FILEUPLOAD')));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_g'), new DTblMap('',
				array('T_GENERAL1', new DTblMap('virtualHostConfig',
						array('T_GENERAL2',
								new DTblMap('index', 'VT_INDXF'),
								new DTblMap('*errorpage$errCode',  'VT_ERRPG_TOP', 'VT_ERRPG'),
								new DTblMap('scripthandler', new DTblMap(array('*scriptHandler','*addsuffix$suffix'), 'A_SCRIPT')),
								new DTblMap('expires', 'A_EXPIRES'),
                            'VT_FILEUPLOAD')))));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'log';
		$page = new DPage($id, DMsg::UIStr('tab_log'), new DTblMap('',
				array(new DTblMap('errorlog$fileName', 'V_LOG'),
						new DTblMap('accesslog$fileName', 'V_ACLOG'))));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_log'), new DTblMap('virtualHostConfig',
				array(new DTblMap('errorlog$fileName', 'T_LOG'),
						new DTblMap('accesslog$fileName', 'T_ACLOG'))));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'sec';
		$page = new DPage($id, DMsg::UIStr('tab_sec'), new DTblMap('',
				array(new DTblMap('accessControl', 'A_SEC_AC'),
						new DTblMap('*realm$name', 'V_REALM_TOP', 'V_REALM_FILE')),
				new DTblMap('*index', array('V_UDB_TOP', 'V_UDB', 'V_GDB_TOP','V_GDB'))));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_sec'), new DTblMap('',
				array('T_SEC_FILE', 'T_SEC_CONN', 'T_SEC_CGI',
						new DTblMap('virtualHostConfig',
								array(new DTblMap('accessControl', 'A_SEC_AC'),
										new DTblMap('*realm$name', 'T_REALM_TOP', 'T_REALM_FILE'))))));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'ext';
		$page = new DPage($id, DMsg::UIStr('tab_ext'),
				new DTblMap('*extprocessor$name', 'A_EXT_TOP',
						array('A_EXT_SEL', 'A_EXT_FCGI', 'A_EXT_FCGIAUTH', 'A_EXT_LSAPI', 'A_EXT_SERVLET', 'A_EXT_PROXY', 'A_EXT_LOGGER', 'A_EXT_LOADBALANCER')));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_ext'),
				new DTblMap('virtualHostConfig:*extprocessor$name', 'T_EXT_TOP',
						array('T_EXT_SEL', 'T_EXT_FCGI', 'T_EXT_FCGIAUTH', 'T_EXT_LSAPI', 'T_EXT_SERVLET', 'T_EXT_PROXY', 'T_EXT_LOGGER', 'T_EXT_LOADBALANCER')));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'sh';
		$page = new DPage($id, DMsg::UIStr('tab_sh'), new DTblMap('scripthandler:*addsuffix$suffix', 'A_SCRIPT_TOP', 'A_SCRIPT'));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_sh'), new DTblMap('virtualHostConfig:scripthandler:*addsuffix$suffix', 'A_SCRIPT_TOP', 'A_SCRIPT'));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'rw';
		$page = new DPage($id, DMsg::UIStr('tab_rewrite'), new DTblMap('rewrite',
				array('VT_REWRITE_CTRL',
						new DTblMap('*map$name', 'VT_REWRITE_MAP_TOP', 'VT_REWRITE_MAP'),
						'VT_REWRITE_RULE')));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_rewrite'), new DTblMap('virtualHostConfig:rewrite',
				array('VT_REWRITE_CTRL',
						new DTblMap('*map$name', 'VT_REWRITE_MAP_TOP', 'VT_REWRITE_MAP'),
						'VT_REWRITE_RULE')));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'ctx';
		$page = new DPage($id, DMsg::UIStr('tab_ctx'), new DTblMap('*context$uri', 'VT_CTX_TOP',
				array('VT_CTX_SEL', 'VT_CTXG', 'VT_CTXJ', 'VT_CTXS', 'VT_CTXF', 'VT_CTXL',
						'VT_CTXP', 'VT_CTXC', 'VT_CTXB', 'VT_CTXR', 'VT_CTXRL', 'VT_CTXMD')));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_ctx'), new DTblMap('virtualHostConfig:*context$uri', 'VT_CTX_TOP',
				array('VT_CTX_SEL', 'VT_CTXG', 'VT_CTXJ', 'VT_CTXS', 'VT_CTXF', 'VT_CTXL',
						'VT_CTXP', 'VT_CTXC', 'VT_CTXB', 'VT_CTXR', 'VT_CTXRL')));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'vhssl';
		$page = new DPage($id, DMsg::UIStr('tab_ssl'), new DTblMap('vhssl',
				array('LVT_SSL_CERT', 'LVT_SSL', 'VT_SSL_FEATURE', 'LVT_SSL_OCSP', 'LVT_SSL_CLVERIFY')));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_ssl'), new DTblMap('virtualHostConfig:vhssl',
				array('LVT_SSL_CERT', 'LVT_SSL', 'VT_SSL_FEATURE', 'LVT_SSL_OCSP', 'LVT_SSL_CLVERIFY')));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'wsp';
		$page = new DPage($id, DMsg::UIStr('tab_wsp'), new DTblMap('*websocket$uri', 'VT_WBSOCK_TOP', 'VT_WBSOCK'));
		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_wsp'), new DTblMap('virtualHostConfig:*websocket$uri', 'VT_WBSOCK_TOP', 'VT_WBSOCK'));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'mod';
		$page = new DPage($id, DMsg::UIStr('tab_mod'), new DTblMap('*module$name',
				array('VT_MOD_TOP',
						new DTblMap('*urlFilter$uri', NULL, array('VT_MOD_FILTERTOP', 'VT_MOD_FILTER'))), 'VT_MOD'));

		$this->_pageDef['vh_'][$id] = $page;

		$page = new DPage($id, DMsg::UIStr('tab_mod'), new DTblMap('virtualHostConfig:*module$name',
				array('VT_MOD_TOP',
						new DTblMap('*urlFilter$uri', NULL, array('VT_MOD_FILTERTOP', 'VT_MOD_FILTER'))), 'VT_MOD'));
		$this->_pageDef['tp_'][$id] = $page;

		$id = 'g';
		$page = new DPage($id, DMsg::UIStr('tab_g'), new DTblMap('',
				array('ADM_PHP',
						new DTblMap('errorlog$fileName', 'V_LOG'),
						new DTblMap('accesslog$fileName', 'ADM_ACLOG'),
						new DTblMap('accessControl', 'A_SEC_AC'))));
		$this->_pageDef['admin'][$id] = $page;

		$id = 'usr';
		$page = new DPage($id, DMsg::UIStr('tab_user'), new DTblMap('*index', 'ADM_USR_TOP', array('ADM_USR', 'ADM_USR_NEW')));
		$this->_pageDef['admin'][$id] = $page;

		$id = 'top';
		$page = new DPage($id, DMsg::UIStr('tab_top'), new DTblMap('*listener$name', 'ADM_L_TOP', 'ADM_L_GENERAL'));
		$this->_pageDef['al'][$id] = $page;

		$id = 'lg';
		$page = new DPage($id, DMsg::UIStr('tab_g'), new DTblMap('*listener$name', 'ADM_L_GENERAL'));
		$this->_pageDef['al_'][$id] = $page;

		$id = 'lsec';
		$page = new DPage($id, DMsg::UIStr('tab_ssl'), new DTblMap('*listener$name',
				array('LVT_SSL_CERT', 'LVT_SSL', 'L_SSL_FEATURE', 'LVT_SSL_CLVERIFY')));
		$this->_pageDef['al_'][$id] = $page;

	}


}
