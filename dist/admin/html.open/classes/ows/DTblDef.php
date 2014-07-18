<?php

class DTblDef extends DTblDefBase
{
	public static function getInstance()
	{
        if ( !isset($GLOBALS['_DTblDef_']) )
			$GLOBALS['_DTblDef_'] = new DTblDef();
		return $GLOBALS['_DTblDef_'];
	}

	private function __construct()
	{
		$this->loadCommonOptions();
		$this->loadCommonAttrs();
	}

	protected function loadCommonOptions()
	{
		parent::loadCommonOptions();
		$this->_options['scriptHandler'] = array(
				'fcgi'=>'Fast CGI', 'servlet'=>'Servlet Engine',
				'lsapi'=>'LiteSpeed SAPI',
				'proxy'=>'Web Server', 'cgi'=>'CGI',
				'loadbalancer'=>'Load Balancer', 'module'=>'Module Handler' );

		$this->_options['ctxType'] = array(
				'NULL'=>'Static', 'webapp'=>'Java Web App',
				'servlet'=>'Servlet', 'fcgi'=>'Fast CGI',
				'lsapi'=>'LiteSpeed SAPI',
				'proxy'=>'Proxy', 'cgi'=>'CGI',
				'loadbalancer'=> 'Load Balancer',
				'redirect'=>'Redirect',
				'rails'=>'Rack/Rails', 'module'=>'Module Handler');

		$this->_options['ctxTbl'] = array(
				0=>'type', 1=>'VT_CTXG',
				'NULL'=>'VT_CTXG', 'webapp'=>'VT_CTXJ',
				'servlet'=>'VT_CTXS', 'fcgi'=>'VT_CTXF',
				'lsapi'=>'VT_CTXL',
				'proxy'=>'VT_CTXP', 'cgi'=>'VT_CTXC',
				'loadbalancer'=>'VT_CTXB',
				'redirect'=>'VT_CTXR',
				'rails'=>'VT_CTXRL',
				'module'=>'VT_CTXMD');

		$this->_options['realmType'] = array('file' => 'Password File');

	}

	protected function loadCommonAttrs()
	{
		parent::loadCommonAttrs();
		$this->_attrs['mod_params'] = new DAttr('param', 'cust', 'Module Parameters', 'textarea1', true, NULL,
					'use format key=value, one parameter per line','rows="4" cols="60" wrap="off"', 0, 'modParams');
		$this->_attrs['mod_enabled'] = new DAttr('enabled', 'bool', 'Enable Filters', 'radio', true, NULL, NULL, NULL, 0, 'moduleEnabled');
	}

	protected function add_S_PROCESS($id) //keep
	{
		$this->_tblDef[$id] = new DTbl($id, 'Server Process');

		$runningAs = new DAttr('runningAs', 'cust', 'Running As', 'text', false);
		$runningAs->SetFlag(DAttr::BM_NOFILE | DAttr::BM_NOEDIT);

		$user = new DAttr('user', 'cust', NULL, 'text', false);
		$user->SetFlag(DAttr::BM_HIDE | DAttr::BM_NOEDIT);

		$group = new DAttr('group', 'cust', NULL, 'text', false);
		$group->SetFlag(DAttr::BM_HIDE | DAttr::BM_NOEDIT);

		$attrs = array(
			new DAttr('serverName', 'name', 'Server Name', 'text', false),
			new DAttr('httpdWorkers', 'uint', 'Number of Workers', 'text', true, 1, 16),
			$runningAs, $user, $group,
			$this->_attrs['priority']->dup(NULL, NULL, 'serverPriority'),
			new DAttr('inMemBufSize', 'uint', 'Memory I/O Buffer', 'text', false, 0, NULL),
			new DAttr('swappingDir', 'cust', 'Swapping Directory', 'text', false, 1, 'rw', $this->_options['text_size']),
			new DAttr( 'autoFix503', 'bool', 'Auto Fix 503 Error', 'radio', true ),
			new DAttr('gracefulRestartTimeout', 'uint', 'Graceful Restart Timeout (secs)', 'text', true, -1, 2592000)

			);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_S_GENERAL($id) // keep
	{
		$this->_tblDef[$id] = new DTbl($id, 'General Settings');

		$attr_mime = new DAttr('mime', 'file', 'MIME Settings', 'text', false, 2, 'rw', $this->_options['text_size']);
		$attr_mime->_href = '&t=S_MIME_TOP';

		$attr_ar = new DAttr('adminRoot', 'cust', NULL, NULL, false);
		$attr_ar->SetFlag(DAttr::BM_HIDE | DAttr::BM_NOEDIT);

		$attrs = array(
			$attr_mime,
			new DAttr('disableInitLogRotation', 'bool', 'Disable Initial Log Rotation', 'radio', true),
			new DAttr('showVersionNumber', 'sel', 'Server Signature', 'select', false, NULL, array('0'=>'Hide Version', '1'=>'Show Version', '2'=>'Hide Full Header')),
			$this->_attrs['enableIpGeo'],
			new DAttr('useIpInProxyHeader', 'sel', 'Use Client IP in Header', 'select', true, NULL, array('0'=>'No', '1'=>'Yes', '2'=>'Trusted IP Only') ),
			$this->_attrs['adminEmails'],
			$attr_ar );
		$this->_tblDef[$id]->SetAttr($attrs);

	}

	protected function add_S_TUNING_OS($id) //keep
	{
		$this->_tblDef[$id] = new DTbl($id, 'OS Optimization');

		$ssloptions = array( 'null' => '(built-in) OpenSSL internal engine',
				     'auto' => '(auto) Automatically use available devices',
				     'aesni' => '(aesni) Intel AES-NI engine',
				     'dynamic'=>'(dynamic) Dynamic engine loading support',
				     'cswift' => '(cswift) CryptoSwift hardware engine support',
				     'chil' => '(chil) nCipher hardware engine support',
				     'atalla' => '(atalla) Atalla hardware engine support',
				     'nuron' => '(nuron) Nuron hardware engine support',
				     'ubsec' => '(ubsec) UBSEC hardware engine support',
				     'aep' => '(aep) Aep hardware engine support',
				     'sureware' => '(sureware) SureWare hardware engine support',
				     '4758cca' => '(4758cca) IBM 4758 CCA hardware engine support' );

		$edoptions = array( 'best'    => 'best (All platforms)',
				    'poll'    => 'poll (All platforms)',
				    'epoll'   => 'epoll (Linux 2.6 kernel)',
				    'kqueue'  => 'kqueue (FreeBSD/Mac OS X)',
				    'devpoll' => 'devpoll (Solaris)');

		$attrs = array(
			new DAttr('eventDispatcher', 'sel', 'I/O Event Dispatcher', 'select', true, 0, $edoptions),
			new DAttr('SSLCryptoDevice', 'sel', 'SSL Hardware Accelerator', 'select', true, 0, $ssloptions),
			);

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_S_TUNING_STATIC($id)  // keep
	{
		$this->_tblDef[$id] = new DTbl($id, 'Static File Delivery Optimization');

		$etag_options = array( '4'=>'iNode', '8'=>'Modified Time', '16'=>'Size', '0'=>'None');
		$attrs = array(
			new DAttr('maxCachedFileSize', 'uint', 'Max Cached Small File Size (bytes)', 'text', false, 0, 1048576),
			new DAttr('totalInMemCacheSize', 'uint', 'Total Small File Cache Size (bytes)', 'text', false, 0),
			new DAttr('maxMMapFileSize', 'uint', 'Max MMAP File Size (bytes)', 'text', false, 0),
			new DAttr('totalMMapCacheSize', 'uint', 'Total MMAP Cache Size (bytes)', 'text', false, 0),
			new DAttr('useSendfile', 'bool', 'Use sendfile()', 'radio'),
			new DAttr('fileETag', 'checkboxOr', 'File ETag', 'checkboxgroup', true, 28, $etag_options),
			);

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function get_module_hookpoints_attrs($type)
	{
		//type: S: server level, T: Top level
		$tags = array('L4_BEGINSESSION','L4_ENDSESSION','L4_RECVING','L4_SENDING',
				'HTTP_BEGIN','RECV_REQ_HEADER','URI_MAP','HTTP_AUTH',
				'RECV_REQ_BODY','RCVD_REQ_BODY','RECV_RESP_HEADER','RECV_RESP_BODY','RCVD_RESP_BODY',
				'HANDLER_RESTART','SEND_RESP_HEADER','SEND_RESP_BODY','HTTP_END',
				'MAIN_INITED','MAIN_PREFORK','MAIN_POSTFORK','WORKER_POSTFORK','WORKER_ATEXIT','MAIN_ATEXIT');
		$list = array();
		$i = 0;
		foreach($tags as $tag) {
			$i ++;
			if ($type == 'S')
				$list[] = new DAttr($tag, 'uint', "Hook::$tag Priority", 'text', true, -6000, 6000);
			elseif ($type == 'ST') {
				$list[] = new DAttr($tag, 'uint', "<span title=\"Hook::$tag priority\">H$i</span>", 'text', true);
			}
		}
		return $list;
	}

	protected function add_S_MOD_TOP($id)
	{
		$align = array('center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Server Modules Definition', 1, 'S_MOD', $align, 'module', TRUE);

		$attrs = array_merge(
				array(new DAttr('name', 'cust', 'Module'),
					new DAttr('enabled', 'bool', '<span title="Enable Filters">EnabFilt</span>', 'radio', true)),
				$this->get_module_hookpoints_attrs('ST'),
				array(new DAttr('action', 'action', 'Action', NULL, TRUE, 'S_MOD', 'vEd'))
			);

		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_S_MOD($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Server Module Definition', 2);

		$attrs = array_merge(
				array(new DAttr('name', 'modulename', 'Module', 'text', false, NULL, NULL, NULL, 0, 'modulename'),
						$this->_attrs['note'],
						new DAttr('internal', 'bool', 'Is Internal', 'radio', true, NULL, NULL, NULL, 0, 'internalmodule'),
						$this->_attrs['mod_params'],
						$this->_attrs['mod_enabled']),
				$this->get_module_hookpoints_attrs('S'));

		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_helpKey = 'TABLEservModules';
	}

	protected function add_VT_MOD_TOP($id)
	{
		$align = array('center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Module Configuration', 1, 'VT_MOD', $align, 'module', TRUE);

		$attrs = array(new DAttr('name', 'cust', 'Module'),
					$this->_attrs['mod_params'],
					$this->_attrs['mod_enabled'],
					new DAttr('action', 'action', 'Action', NULL, TRUE, 'VT_MOD', 'vEd'));

		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_helpKey = 'TABLEvhModules';
	}

	protected function add_VT_MOD($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Module Configuration', 2);

		$attrs = array(new DAttr('name', 'sel1', 'Module', 'select', false, 'module', NULL, NULL, 0, 'moduleNameSel'),
						$this->_attrs['note'],
						$this->_attrs['mod_params'],
						$this->_attrs['mod_enabled']->dup(NULL, NULL, 'moduleEnabled_vh'));

		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_linkedTbls = array('VT_MOD_FILTERTOP');
		$this->_tblDef[$id]->_helpKey = 'TABLEvhModules';
	}

	protected function add_VT_MOD_FILTERTOP($id)
	{
		$align = array('center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'URL Filters', 1, 'VT_MOD_FILTER', $align, 'filter', FALSE);

		$attrs = array( new DAttr('uri', 'cust', 'URI'),
						$this->_attrs['mod_params'],
						$this->_attrs['mod_enabled'],
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'VT_MOD_FILTER', 'vEd'));

		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_showParentRef = true;
		$this->_tblDef[$id]->_helpKey = 'TABLEvhModuleUrlFilters';
	}

	protected function add_VT_MOD_FILTER($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'URL Filter', 2);

		$attrs = array($this->_attrs['ctx_uri'],
					$this->_attrs['mod_params'],
					$this->_attrs['mod_enabled']->dup(NULL, NULL, 'moduleEnabled_vh'));

		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_showParentRef = true;
		$this->_tblDef[$id]->_helpKey = 'TABLEvhModuleUrlFilters';
	}

	protected function add_L_MOD_TOP($id)
	{
		$align = array('center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Module Configuration', 1, 'L_MOD', $align, 'module', TRUE);

		$attrs = array( new DAttr('name', 'cust', 'Module'),
						$this->_attrs['mod_params'],
						$this->_attrs['mod_enabled'],
						new DAttr('action', 'action', 'Action', NULL, TRUE, 'L_MOD', 'vEd')	);

		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_helpKey = 'TABLElistenerModules';
	}

	protected function add_L_MOD($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Module Configuration', 2);

		$attrs = array(new DAttr('name', 'sel1', 'Module', 'select', false, 'module', NULL, NULL, 0, 'moduleNameSel'),
						$this->_attrs['note'],
						$this->_attrs['mod_params'],
						$this->_attrs['mod_enabled']->dup(NULL, NULL, 'moduleEnabled_lst'));

		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_helpKey = 'TABLElistenerModules';
	}

	protected function add_V_TOPD($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Hosts', 2);

		$attrs = array(
			new DAttr('name', 'vhname', 'Virtual Host Name', 'text', false, NULL, NULL, NULL, 0, 'vhName'),
			new DAttr('vhRoot', 'cust', 'Virtual Host Root', 'text', false, NULL, NULL, $this->_options['text_size']),//no validation, maybe suexec owner
			new DAttr('configFile', 'filevh', 'Config File', 'text', false, 3, 'rwc', $this->_options['text_size']),
			$this->_attrs['note'],
			$this->_attrs['vh_allowSymbolLink'],
			$this->_attrs['vh_enableScript'],
			$this->_attrs['vh_restrained'],
			$this->_attrs['vh_maxKeepAliveReq'],
			$this->_attrs['vh_smartKeepAlive'],
			$this->_attrs['vh_setUIDMode'],
			$this->_attrs['staticReqPerSec'],
			$this->_attrs['dynReqPerSec'],
			$this->_attrs['outBandwidth'],
			$this->_attrs['inBandwidth'],
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_V_BASE_SEC($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Security');

		$attrs = array(
			$this->_attrs['vh_allowSymbolLink'],
			$this->_attrs['vh_enableScript'],
			$this->_attrs['vh_restrained'],
			$this->_attrs['vh_setUIDMode']
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');

	}

	protected function add_V_REALM_TOP($id)
	{
		$align = array('center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Realms List', 1, 'V_REALM_FILE', $align, 'shield', FALSE);
		$realm_attr = $this->get_realm_attrs();

		$attrs = array(
			$realm_attr['realm_name'],
			$realm_attr['realm_type'],
			new DAttr('action', 'action', 'Action', NULL, TRUE, 'V_REALM_FILE', 'vEd')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_helpKey = 'TABLErealms';
	}

	protected function add_T_REALM_TOP($id)
	{
		$align = array('center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Realms List', 1, 'T_REALM_FILE', $align, 'shield', FALSE);
		$realm_attr = $this->get_realm_attrs();

		$attrs = array(
			$realm_attr['realm_name'],
			$realm_attr['realm_type'],
			new DAttr('action', 'action', 'Action', NULL, TRUE, 'T_REALM_FILE', 'vEd')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_helpKey = 'TABLErealms';
	}

	protected function add_VT_CTX_TOP($id)
	{
		$align = array('center', 'left', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Context List', 1, 'VT_CTX_SEL', $align, array('NULL' => 'file',
				 'proxy' => 'network', 'redirect' => 'redirect', 'module'=>'module'), TRUE);
		$o = new DAttr('order', 'ctxseq', 'Order');
		$o->_href = '&ctxseq=';
		$o->SetFlag(DAttr::BM_NOFILE | DAttr::BM_NOEDIT);

		$attrs = array(
			$this->_attrs['ctx_type'],
			new DAttr('uri', 'cust', 'URI'),
			$o,
			new DAttr('action', 'action', 'Action', NULL, TRUE, $this->_options['ctxTbl'], 'vEd')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
	}

	protected function add_VT_CTXG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Static Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
			array(
				$this->_attrs['ctx_location'],
					new DAttr('allowBrowse', 'bool', 'Accessible', 'radio', false),
				$this->_attrs['note']),
				$this->get_expires_attrs(),
				array(
				$this->_attrs['extraHeaders'],

				new DAttr('addMIMEType', 'parse', 'MIME Type', 'textarea', true,
						  "/[A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+(\s+[A-z0-9_\-\+]+)+/",
						  'MIME_type suffix suffix, next MIME_type... MIME type is like text/html, followed by one or more suffix types separated by space. Use comma to separate different MIME types.',
						  'rows="2" cols="60"', 1),
				new DAttr('forceType', 'parse', 'Force MIME Type', 'text', true,
						  "/^([A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+)|(NONE)$/i",
						  'MEME_type like text/html, or put "NONE" to disable inherited Force Type.'),
				new DAttr('defaultType', 'parse', 'Default MIME Type', 'text', true,
						  "/^[A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+$/", 'MEME_type like text/html'),
				$this->_attrs['indexFiles'],
				$this->_attrs['autoIndex']),
			$this->get_ctx_attrs('auth'),
			$this->get_ctx_attrs('rewrite'),
			$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'NULL');
		$this->_tblDef[$id]->_helpKey = 'TABLEgeneralContext';
	}

	protected function add_VT_CTXJ($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Java WebApp Context Definition', 2);
		$attrs = array_merge(
				array(
			new DAttr('uri', 'uri', 'URI', 'text', false, NULL, NULL, $this->_options['text_size']),
			$this->_attrs['ctx_order'],
			$this->_attrs['ctx_location']->dup(NULL, NULL, 'javaWebApp_location'),
			$this->_attrs['ctx_shandler'],
			$this->_attrs['note']),
				$this->get_expires_attrs(),
				array(
			$this->_attrs['extraHeaders'],
			$this->_attrs['indexFiles'],
			$this->_attrs['autoIndex']),
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'webapp');
		$this->_tblDef[$id]->_helpKey = 'TABLEjavaWebAppContext';
	}

	protected function add_VT_CTXRL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Rack/Rails Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				array(
			$this->_attrs['ctx_location'],
			$this->_attrs['note'],
			$this->_attrs['railsEnv'],
			new DAttr('maxConns', 'uint', 'Max Connections', 'text', true, 1, 2000),
			$this->_attrs['ext_env']),
				$this->get_expires_attrs(),
				array(
			$this->_attrs['extraHeaders'],
			$this->_attrs['indexFiles'],
			$this->_attrs['autoIndex']),
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('rewrite'),
				$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'rails');
		$this->_tblDef[$id]->_helpKey = 'TABLErailsContext';
	}

	protected function add_VT_CTXS($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Servlet Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				array(
			$this->_attrs['ctx_shandler'],
			$this->_attrs['note'],
			$this->_attrs['extraHeaders']),
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'servlet');
		$this->_tblDef[$id]->_helpKey = 'TABLEservletContext';
	}

	protected function add_VT_CTXF($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'FCGI Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				array(
						new DAttr('handler', 'sel1', 'Fast CGI App', 'select', false,
								'extprocessor:fcgi', NULL, NULL, 0, 'fcgiapp'),
						$this->_attrs['note'],
						$this->_attrs['extraHeaders'],
						$this->get_ctx_attrs('auth'),
						$this->get_ctx_attrs('charset')
				));
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'fcgi');
		$this->_tblDef[$id]->_helpKey = 'TABLEfcgiContext';
	}


	protected function add_VT_CTXL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'LiteSpeed SAPI Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				array(
			new DAttr('handler', 'sel1', 'LSAPI App', 'select', false,
					  'extprocessor:lsapi', NULL, NULL, 0, 'lsapiapp'),
			$this->_attrs['note'],
			$this->_attrs['extraHeaders']),
			$this->get_ctx_attrs('auth'),
			$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'lsapi');
		$this->_tblDef[$id]->_helpKey = 'TABLElsapiContext';
	}

	protected function add_VT_CTXMD($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Module Handler Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
			array(
				new DAttr('handler', 'sel1', 'Module Handler', 'select', false,
						  'module', NULL, NULL, 0, 'moduleNameSel'),
				$this->_attrs['note'],
				$this->_attrs['mod_params'],
				$this->_attrs['extraHeaders']),
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'module');
		$this->_tblDef[$id]->_helpKey = 'TABLElmodContext';
	}

	protected function add_VT_CTXB($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Load Balancer Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				array(
			new DAttr('handler', 'sel1', 'Load Balancer', 'select', false,
					  'extprocessor:loadbalancer', NULL, NULL, 0, 'lbapp'),
			$this->_attrs['note']),
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'loadbalancer');
	}

	protected function add_VT_CTXP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Proxy Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				array(
			new DAttr('handler', 'sel1', 'Web Server', 'select', false,
					 'extprocessor:proxy', NULL, NULL, 0, 'proxyWebServer'),
			$this->_attrs['note'],
			$this->_attrs['extraHeaders']),
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'proxy');
		$this->_tblDef[$id]->_helpKey = 'TABLEproxyContext';
	}

	protected function add_VT_CTXC($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'CGI Context Definition', 2);
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				array(
			$this->_attrs['ctx_location']->dup(NULL, 'Path', 'cgi_path'),
			$this->_attrs['note'],
			$this->_attrs['extraHeaders'],
			new DAttr('allowSetUID', 'bool', 'Allow Set UID', 'radio')),
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('rewrite'),
				$this->get_ctx_attrs('charset')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'cgi');
		$this->_tblDef[$id]->_helpKey = 'TABLEcgiContext';
	}

	protected function add_VT_CTXR($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Redirect Context Definition', 2);

		$options = &CustStatusCode::GetRedirect();

		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				array(
			new DAttr('externalRedirect', 'bool',  'External Redirect', 'radio', false, NULL, NULL, NULL, 0, 'externalredirect'),
			new DAttr('statusCode', 'sel', 'Status Code', 'select', true, 0, $options, NULL, 0, 'statuscode'),
			new DAttr('location', 'url', 'Destination URI', 'text', true, NULL, NULL, $this->_options['text_size'], 0, 'destinationuri'),
			$this->_attrs['note']),
				$this->get_ctx_attrs('auth')
			);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'redirect');
		$this->_tblDef[$id]->_helpKey = 'TABLEredirectContext';
	}

	protected function add_T_SEC_CGI($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'ExtApp Resource Control');

		$attrs = array(
			$this->_attrs['vh_setUIDMode'],
			);
		$this->_tblDef[$id]->SetAttr($attrs);
	}


	protected function add_L_GENERAL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Address Settings');

		$name =	new DAttr('name', 'name', 'Listener Name', 'text', false);
		$name->_helpKey = 'listenerName';
		$addr = new DAttr('address', 'cust', 'Address', NULL, false);
		$addr->SetFlag(DAttr::BM_HIDE | DAttr::BM_NOEDIT);
		$ip = new DAttr('ip', 'sel', 'IP Address', 'select', false, 0, $this->_options['ip']);
		$ip->SetFlag(DAttr::BM_NOFILE);
		$ip->_helpKey = 'listenerIP';
		$port = new DAttr('port', 'uint', 'Port', 'text', false, 0, 65535);
		$port->SetFlag(DAttr::BM_NOFILE);
		$port->_helpKey = 'listenerPort';

		$processes = isset($_SERVER['LSWS_CHILDREN']) ? $_SERVER['LSWS_CHILDREN'] : 1;
		for( $i = 1; $i <= $processes; ++$i )
			$bindoptions[1<<($i-1)] = "Process $i";

		$binding = new DAttr('binding', 'checkboxOr', 'Binding', 'checkboxgroup', true, NULL, $bindoptions);
		$binding->_helpKey = 'listenerBinding';

		$attrs = array(
				$name,
				$addr, $ip,	$port, $binding,
				new DAttr('secure', 'bool', 'Secure', 'radio', false, NULL, NULL, NULL, 0, 'listenerSecure'),
				$this->_attrs['note'],
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_LVT_SSL_CLVERIFY($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Client Verification');

		$attrs = array(
			new DAttr('clientVerify', 'sel', 'Client Verification', 'select', true, NULL,
						array('0'=>'none','1'=>'optional','2'=>'require','3'=>'optional_no_ca' )),
		    new DAttr('verifyDepth', 'uint', 'Verify Depth', 'text', true, 0, 100),
		    new DAttr('crlPath', 'cust', 'Client Revocation Path', 'text', true, 2, '', $this->_options['text_size']),
		    new DAttr('crlFile', 'cust', 'Client Revocation File', 'text', true, 2, '', $this->_options['text_size']),
		    );
		$this->_tblDef[$id]->SetAttr($attrs);
	}

}
