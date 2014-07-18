<?php

class DTblDefBase
{
	protected $_tblDef = array();
	protected $_options = array();
	protected $_attrs;

	public function GetTblDef($tblId)
	{
		if (!isset( $this->_tblDef[$tblId] ))
		{
			$funcname = 'add_' . $tblId;
			if (!method_exists($this, $funcname)) {
				die("invalid tid $tblId\n");
			}
			$this->$funcname($tblId);
		}
		return $this->_tblDef[$tblId];
	}

	protected function DupTblDef($tblId, $newId, $newTitle=NULL)
	{
		$tbl = $this->GetTblDef($tblId);
		$newtbl = $tbl->Dup($newId, $newTitle);
		return $newtbl;
	}


	protected function loadCommonOptions()
	{
		$this->_options['text_size'] = 'size="95"';
		$this->_options['tp_vname'] = array( '/\$VH_NAME/', 'requiring variable $VH_NAME');


		$this->_options['symbolLink'] = array( '1'=>'Yes', '2'=>'If Owner Match', '0'=>'No');

		$this->_options['logHeaders'] = array('1'=>'Referrer', '2'=>'UserAgent', '4'=>'Host', '0'=>'None');

		$this->_options['railsEnv'] = array(
				''=>'', '0'=>'Development',	'1'=>'Production', '2'=>'Staging' );

		$this->_options['extType'] = array(
				'fcgi'=>'Fast CGI App', 'fcgiauth'=>'Fast CGI Authorizer',
				'lsapi'=>'LSAPI App',
				'servlet'=>'Servlet Engine', 'proxy'=>'Web Server',
				'logger'=>'Piped Logger',
				'loadbalancer'=>'Load Balancer');

		$this->_options['extAutoStart'] = array(
				'1'=>'Yes', '0'=>'No', '2'=>'Through CGI Daemon');

		$this->_options['extTbl'] = array(
				0=>'type', 1=>'A_EXT_FCGI',
			    'fcgi'=>'A_EXT_FCGI', 'fcgiauth'=>'A_EXT_FCGIAUTH',
				'lsapi'=>'A_EXT_LSAPI',
				'servlet'=>'A_EXT_SERVLET', 'proxy'=>'A_EXT_PROXY',
				'logger'=>'A_EXT_LOGGER',
				'loadbalancer'=>'A_EXT_LOADBALANCER');

		$this->_options['tp_extTbl'] = array(
				0=>'type', 1=>'T_EXT_FCGI',
				'fcgi'=>'T_EXT_FCGI', 'fcgiauth'=>'T_EXT_FCGIAUTH',
				'lsapi'=>'T_EXT_LSAPI',
				'servlet'=>'T_EXT_SERVLET', 'proxy'=>'T_EXT_PROXY',
				'logger'=>'T_EXT_LOGGER',
				'loadbalancer'=>'T_EXT_LOADBALANCER');

		$this->_options['ext_respBuffer'] = array('0'=>'No', '1'=>'Yes',
												  '2'=>'No for Non-Parsed-Header (NPH)');

		$this->_options['on_off'] = array('off'=>'Off', 'on'=>'On');

		$this->_options['logLevel'] = array('ERROR'=>'ERROR', 'WARN'=>'WARNING',
									'NOTICE'=>'NOTICE', 'INFO'=>'INFO', 'DEBUG'=>'DEBUG');

		$ipv6str = isset($_SERVER['LSWS_IPV6_ADDRS']) ? $_SERVER['LSWS_IPV6_ADDRS'] : '';
		$ipv6 = array();
		if ( $ipv6str != '' ) {
			$ipv6['[ANY]'] = '[ANY] IPv6';
			$ips = explode( ',', $ipv6str );
			foreach( $ips as $ip ) {
				if ($pos = strpos( $ip, ':' )) {
					$aip = substr( $ip, $pos+1 );
					$ipv6[$aip] = $aip;
				}
			}
		}
		$ipo = array();
		$ipo['ANY'] = 'ANY';
		$ipstr = isset($_SERVER['LSWS_IPV4_ADDRS']) ? $_SERVER['LSWS_IPV4_ADDRS'] : '';
		if ($ipstr != '') {
			$ips = explode(',', $ipstr);
			foreach( $ips as $ip ) {
				if ($pos = strpos($ip, ':')) {
					$aip = substr($ip, $pos+1);
					$ipo[$aip] = $aip;
					if ( $aip != '127.0.0.1' )
						$ipv6["[::FFFF:$aip]"] = "[::FFFF:$aip]";
				}
			}
		}
		if ( $ipv6str != '' )
			$this->_options['ip'] = $ipo + $ipv6;
		else
			$this->_options['ip'] = $ipo;

	}

	protected function loadCommonAttrs()
	{
		$ctxOrder = new DAttr('order', 'cust', 'Order');
		$ctxOrder->SetFlag(DAttr::BM_NOFILE | DAttr::BM_HIDE | DAttr::BM_NOEDIT);

		$attrs = array(
			'priority' => new DAttr('priority', 'uint', 'Priority', 'text', true, -20, 20),
			'indexFiles' => new DAttr('indexFiles', 'fname', 'Index Files', 'textarea', true, NULL, NULL, 'rows="2" cols="60"', 1),
			'autoIndex' => new DAttr('autoIndex', 'bool', 'Auto Index', 'radio'),
			'adminEmails' => new DAttr('adminEmails', 'email', 'Administrator Email', 'textarea', true, NULL, NULL, 'rows="3" cols="60"', 1),
			'suffix' => new DAttr('suffix', 'parse', 'Suffixes', 'text', false,
								  "/^[A-z0-9_\-]+$/", 'comma-separated list, allowed character[A-z0-9_\-]', $this->_options['text_size'], 1),
			'fileName2' => new DAttr('fileName', 'file0', 'File Name', 'text', false, 2, 'r', $this->_options['text_size']),
			'fileName3' => new DAttr('fileName', 'file0', 'File Name', 'text', true, 3, 'r', $this->_options['text_size']),

			'rollingSize' => new DAttr('rollingSize', 'uint', 'Rolling Size (bytes)', 'text', true, NULL, NULL, NULL, NULL, 'log_rollingSize'),
			'keepDays' => new DAttr('keepDays', 'uint', 'Keep Days', 'text', true, 0, NULL, NULL, 0, 'accessLog_keepDays'),
			'logFormat' => new DAttr('logFormat', 'cust', 'Log Format', 'text', true, NULL, NULL, $this->_options['text_size'], 0, 'accessLog_logFormat'),
			'logHeaders' => new DAttr('logHeaders',	'checkboxOr', 'Log Headers', 'checkboxgroup', true, NULL, $this->_options['logHeaders'], NULL, 0, 'accessLog_logHeader'),
			'compressArchive' => new DAttr('compressArchive', 'bool', 'Compress Archive', 'radio', true, NULL, NULL, NULL, 0, 'accessLog_compressArchive'),

			'extraHeaders' => new DAttr(
				'extraHeaders', 'cust', 'Extra Headers', 'textarea1', true,
				NULL, 'Extra headers will be inserted into HTTP response Header, one header per line, put "NONE" to disable headers inherited from parent content.',
			    'rows="2" cols="60" wrap=off'),

			'scriptHandler_type' => new DAttr('type', 'sel', 'Handler Type', 'select', false, 0,
											  $this->_options['scriptHandler'],
											  'onChange="document.confform.a.value=\'c\';document.confform.submit()"', 0, 'shType'),

			'scriptHandler' => new DAttr('handler', 'sel1', 'Handler Name', 'select', false,
										'extprocessor:$$type', NULL, NULL, 0, 'shHandlerName'),

			'ext_type' => new DAttr('type', 'sel', 'Type', 'select', false, NULL, $this->_options['extType']),
			'name'=> new DAttr('name', 'name', 'Name', 'text', false),
			'ext_name'=> new DAttr('name', 'name', 'Name', 'text', false, NULL, NULL, NULL, NULL, 'extAppName'),
			'ext_address' => new DAttr('address', 'addr', 'Address', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'extAppAddress'),
			'ext_maxConns' => new DAttr('maxConns', 'uint', 'Max Connections', 'text', false, 1, 2000),
			'pcKeepAliveTimeout' => new DAttr('pcKeepAliveTimeout', 'uint', 'Connection Keepalive Timeout', 'text', true, -1, 10000 ),

			'ext_env' => new DAttr('env', 'parse', 'Environment', 'textarea', true,
								   "/\S+=\S+/", 'PROPERTY=VALUE', 'rows="5" cols="50" wrap=off', 2),
			'ext_initTimeout' => new DAttr('initTimeout', 'uint', 'Initial Request Timeout (secs)', 'text', false, 1),
			'ext_retryTimeout' => new DAttr('retryTimeout', 'uint', 'Retry Timeout (secs)', 'text', false, 0),
			'ext_respBuffer' => new DAttr('respBuffer', 'sel', 'Response Buffering', 'select', false, 0, $this->_options['ext_respBuffer']),
			'ext_persistConn' => new DAttr('persistConn', 'bool', 'Persistent Connection', 'radio', true ),
			'ext_autoStart' => new DAttr('autoStart', 'sel', 'Auto Start', 'select', false, NULL, $this->_options['extAutoStart']),
			'ext_path' => new DAttr('path', 'file1', 'Command', 'text', true, 3, 'x', $this->_options['text_size'], 0, 'extAppPath'),
			'ext_backlog' => new DAttr('backlog', 'uint', 'Back Log', 'text', true, 1, 100),
			'ext_instances' => new DAttr('instances', 'uint', 'Instances', 'text', true, 0, 1000),
			'ext_runOnStartUp' => new DAttr('runOnStartUp', 'sel', 'Run On Start Up', 'select', true, NULL,
							array(
							    ''=>'', '1'=>'Yes', '0'=>'No', '2'=>'suEXEC Daemon' )),
			'ext_user' => new DAttr('extUser', 'cust', "suEXEC User", 'text', true ),
			'ext_group' => new DAttr('extGroup', 'cust', "suEXEC Group", 'text', true ),

			'cgiUmask' => new DAttr('umask', 'parse', 'umask', 'text', true, "/^[0-7][0-7][0-7]$/", '[000-777].'),
			'memSoftLimit' => new DAttr('memSoftLimit', 'uint', 'Memory Soft Limit (bytes)', 'text', true, 0),
			'memHardLimit' => new DAttr('memHardLimit', 'uint', 'Memory Hard Limit (bytes)', 'text', true, 0),
			'procSoftLimit' => new DAttr('procSoftLimit', 'uint', 'Process Soft Limit', 'text', true, 0),
			'procHardLimit' => new DAttr('procHardLimit', 'uint', 'Process Hard Limit','text', true, 0),

		    'ssl_renegProtection' => new DAttr('renegProtection', 'bool', 'SSL Renegotiation Protection', 'radio'),
			'l_vhost' => new DAttr('vhost', 'sel1', 'Virtual Host', 'select', false, 'virtualhost', NULL, NULL, 0, 'virtualHostName' ),
			'l_domain' => new DAttr('domain', 'domain', 'Domains', 'text', false, NULL, NULL, $this->_options['text_size'], 1, 'domainName'),
			'tp_templateFile' => new DAttr('templateFile', 'filetp', 'Template File', 'text', false, 2, 'rwc', $this->_options['text_size']),
			'tp_listeners' => new DAttr('listeners', 'sel2', 'Mapped Listeners', 'text', false, 'listener', NULL, $this->_options['text_size'], 1, 'mappedListeners'),
			'tp_vhName' => new DAttr('vhName', 'vhname', 'Virtual Host Name', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'templateVHName'),
			'tp_vhDomain' => new DAttr('vhDomain', 'domain', 'Domain', 'text', true, NULL, NULL, $this->_options['text_size'], 0, 'templateVHDomain'),
			'tp_vhRoot' => new DAttr('vhRoot', 'parse', 'Default Virtual Host Root', 'text', false,
									 $this->_options['tp_vname'][0], $this->_options['tp_vname'][1],
									 $this->_options['text_size'], 0, 'templateVHRoot'),

			'tp_vrFile' => new DAttr('fileName', 'parse', 'File Name', 'text', true,
					'/(\$VH_NAME)|(\$VH_ROOT)/', 'requiring variable $VH_NAME or $VH_ROOT',	$this->_options['text_size'], 0, 'templateFileRef'),

			'tp_name' => new DAttr('name', 'parse', 'Name', 'text', false,
								   $this->_options['tp_vname'][0], $this->_options['tp_vname'][1], NULL, 0, 'tpextAppName'),
			'vh_maxKeepAliveReq' => new DAttr('maxKeepAliveReq', 'uint', 'Max Keep-Alive Requests', 'text', true, 0, 32767, NULL, 0, 'vhMaxKeepAliveReq'),
			'vh_smartKeepAlive' => new DAttr('smartKeepAlive', 'bool', 'Smart Keep-Alive', 'radio', true, NULL, NULL, NULL, 0, 'vhSmartKeepAlive'),
			'vh_enableGzip' => new DAttr('enableGzip', 'bool', 'Enable GZIP Compression', 'radio'),
			'vh_spdyAdHeader' => new DAttr('spdyAdHeader', 'parse', 'SPDY Advertisement', 'text', true, "/^\d+:npn-spdy\/[23]$/", 'required format: ssl_port:npn-spdy/version like 443:npn-spdy/3', $this->_options['text_size']),
			'vh_allowSymbolLink' => new DAttr('allowSymbolLink', 'sel', 'Follow Symbolic Link', 'select', true, 0, $this->_options['symbolLink']),
			'vh_enableScript' => new DAttr('enableScript', 'bool', 'Enable Scripts/ExtApps', 'radio', false),
			'vh_restrained' => new DAttr('restrained', 'bool', 'Restrained', 'radio', false),
			'vh_setUIDMode' => new DAttr('setUIDMode', 'sel', 'ExtApp Set UID Mode', 'select', true, 2, array( 0=>'Server UID', 1=>'CGI File UID', 2=>'DocRoot UID'), NULL, 0, 'setUidMode'),
			'staticReqPerSec' => new DAttr('staticReqPerSec', 'uint', 'Static Requests/second', 'text', true, 0),
			'dynReqPerSec' => new DAttr('dynReqPerSec', 'uint', 'Dynamic Requests/second', 'text', true, 0),
			'outBandwidth' => new DAttr('outBandwidth', 'uint', 'Outbound Bandwidth (bytes/sec)', 'text', true, 0),
			'inBandwidth' => new DAttr('inBandwidth', 'uint', 'Inbound Bandwidth (bytes/sec)', 'text', true, 0),

			'ctx_order' => $ctxOrder,
			'ctx_type' => new DAttr('type', 'sel', 'Type', 'select', false, NULL, $this->_options['ctxType']),
			'ctx_uri' => new DAttr('uri', 'expuri', 'URI', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'expuri'),
			'ctx_location' => new DAttr('location', 'cust', 'Location', 'text', false, NULL, NULL, $this->_options['text_size']),
			'ctx_shandler' => new DAttr('handler', 'sel1', 'Servlet Engine', 'select', false,
										'extprocessor:servlet', NULL, NULL, 0, 'servletEngine'),
			'railsEnv' => new DAttr('railsEnv', 'sel', 'Run-time Mode', 'select', true, 0, $this->_options['railsEnv']),

			'geoipDBFile' => new DAttr( 'geoipDBFile', 'filep', 'DB File Path', 'text', false, 2, 'r', $this->_options['text_size']),

			'geoipDBCache' => new DAttr( 'geoipDBCache', 'sel',
							'DB Cache Type', 'select', true, 0,
							array( ''=>'', 'Standard'=>'Standard',
								   'MemoryCache' => 'MemoryCache',
								   'CheckCache' => 'CheckCache',
								   'IndexCache' => 'IndexCache' ) ),
			'enableIpGeo' => new DAttr( 'enableIpGeo', 'bool', 'Enable IP GeoLocation', 'radio'),
			'note' => new DAttr('note', 'cust', 'Notes', 'textarea', true, NULL, NULL, 'rows="4" cols="60" wrap=off'),

		);
		$this->_attrs = $attrs;
	}

	//	DAttr($key, $type, $label,  $inputType, $allowNull,$min, $max, $inputAttr, $multiInd)
	protected function get_expires_attrs()
	{
		return array(
				new DAttr('enableExpires', 'bool', 'Enable Expires', 'radio'),
				new DAttr('expiresDefault', 'parse', 'Expires Default', 'text', true, "/^[AaMm]\d+$/",
						'[A|M]###, A means client access time, M means file modified time, ### is number of seconds'),
				new DAttr(	'expiresByType', 'parse', 'Expires By Type', 'textarea', true,
						"/^(\*\/\*)|([A-z0-9_\-\.\+]+\/\*)|([A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+)=[AaMm]\d+$/",
						'MIME_type=A|Mseconds, MIME type can be like */*, text/*, text/html',
						'rows="2" cols="50"', 1)
		);

	}

	protected function add_S_INDEX($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Index Files');
		$attrs = array(
				$this->_attrs['indexFiles'],
				$this->_attrs['autoIndex'],
				new DAttr('autoIndexURI', 'uri', 'Auto Index URI', 'text', true, NULL, NULL, $this->_options['text_size'])
		);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_S_LOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Server Log');

		$dbg_options = array('10'=>'HIGH', '5'=>'MEDIUM', '2'=>'LOW', '0'=>'NONE');
		$attrs = array(
				$this->_attrs['fileName2']->dup(NULL, NULL, 'log_fileName'),
				new DAttr('logLevel', 'sel', 'Log Level', 'select', false, 0, $this->_options['logLevel'], NULL, 0, 'log_logLevel'),
				new DAttr('debugLevel', 'sel', 'Debug Level', 'select', false, 0, $dbg_options, NULL, 0, 'log_debugLevel'),
				$this->_attrs['rollingSize'],
				new DAttr('enableStderrLog', 'bool', 'Enable stderr Log', 'radio', true, NULL, NULL, NULL, 0, 'log_enableStderrLog')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'fileName');
	}

	protected function add_S_ACLOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Access Log');
		$attrs = array(
				$this->_attrs['fileName2']->dup(NULL, NULL, 'accessLog_fileName'),
				new DAttr('pipedLogger', 'sel1', 'Piped Logger', 'select', true,
						'extprocessor:logger', NULL, NULL, 0, 'accessLog_pipedLogger'),
				$this->_attrs['logFormat'],
				$this->_attrs['logHeaders'],
				$this->_attrs['rollingSize'],
				$this->_attrs['keepDays'],
				$this->_attrs['compressArchive']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'fileName');
	}

	protected function add_A_HTACCESS($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'HT Access');

		$attrs = array(
				$this->_attrs['allowOverride'],
				new DAttr('accessFileName', 'accf', 'Access File Name', 'text')
		);

		$this->_tblDef[$id]->SetAttr($attrs);
		$this->_tblDef[$id]->_helpKey = 'TABLEhtaccess';
	}

	protected function add_A_EXPIRES($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Expires Settings');

		$attrs = $this->get_expires_attrs();

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_S_GEOIP_TOP($id)
	{
		$align = array('center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'IP to GeoLocation DB', 1, 'S_GEOIP', $align, "database", TRUE);

		$attrs = array(
				$this->_attrs['geoipDBFile'],
				$this->_attrs['geoipDBCache'],
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'S_GEOIP', 'Ed'));
		$this->_tblDef[$id]->SetAttr($attrs, 'geoipDBFile');
		$this->_tblDef[$id]->_helpKey = 'TABLEgeolocationDB';
	}

	protected function add_S_GEOIP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'IP to GeoLocation DB configuration', 2);

		$attrs = array(
				$this->_attrs['geoipDBFile'],
				$this->_attrs['geoipDBCache'],
				$this->_attrs['note'],
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'geoipDBFile');
		$this->_tblDef[$id]->_helpKey = 'TABLEgeolocationDB';
	}

	protected function add_AE_APACHECONF($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Apache Style Configurations');

		$attrs = array($this->_attrs['apacheconf']);

		$this->_tblDef[$id]->SetAttr($attrs );
	}

	protected function add_S_TUNING_CONN($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Connection');

		$attrs = array(
				new DAttr('maxConnections', 'uint', 'Max Connections', 'text', false, 1),
				new DAttr('maxSSLConnections', 'uint', 'Max SSL Connections', 'text', false, 0),
				new DAttr('connTimeout', 'uint', 'Connection Timeout (secs)', 'text', false, 10, 1000000),
				new DAttr('maxKeepAliveReq', 'uint', 'Max Keep-Alive Requests', 'text', false, 0, 32767),
				new DAttr('smartKeepAlive', 'bool', 'Smart Keep-Alive', 'radio', false),
				new DAttr('keepAliveTimeout', 'uint', 'Keep-Alive Timeout (secs)', 'text', false, 0, 60),
				new DAttr('sndBufSize', 'uint', 'Send Buffer Size (bytes)', 'text', false, 0, 65535),
				new DAttr('rcvBufSize', 'uint', 'Receive Buffer Size (bytes)', 'text', false, 0, 65535),
		);

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_S_TUNING_REQ($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Request/Response');

		$attrs = array(
				new DAttr('maxReqURLLen', 'uint', 'Max Request URL Length (bytes)', 'text', false, 200, 8192),
				new DAttr('maxReqHeaderSize', 'uint', 'Max Request Header Size (bytes)', 'text', false, 1024, 16380),
				new DAttr('maxReqBodySize', 'uint', 'Max Request Body Size (bytes)', 'text', false, '1M', NULL ),
				new DAttr('maxDynRespHeaderSize', 'uint', 'Max Dynamic Response Header Size (bytes)', 'text', false, 200, 8192),
				new DAttr('maxDynRespSize', 'uint', 'Max Dynamic Response Body Size (bytes)', 'text', false, '1M', '2047M')
		);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_S_TUNING_GZIP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'GZIP Compression');

		$parseFormat = "/^(\!)?(\*\/\*)|([A-z0-9_\-\.\+]+\/\*)|([A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+)$/";
		$parseHelp = '(!)MIME types, like */*, text/*, text/html, text/*, !text/css';

		$attrs = array(
				new DAttr('enableGzipCompress', 'bool', 'Enable Compression', 'radio', false),
				new DAttr('enableDynGzipCompress', 'bool', 'Enable Dynamic Compression', 'radio', false),
				new DAttr('gzipCompressLevel', 'uint', 'Compression Level (Dynamic Content)', 'text', true, 1, 9),
				new DAttr('compressibleTypes', 'parse', 'Compressible Types', 'textarea', true, $parseFormat, $parseHelp, 'rows="5" cols="50"', 1),
				new DAttr('gzipAutoUpdateStatic', 'bool', 'Auto Update Static File', 'radio'),
				new DAttr('gzipCacheDir', 'cust', 'Static GZIP Cache Directory', 'text', true, 1, 'rw', $this->_options['text_size']),

				new DAttr('gzipStaticCompressLevel', 'uint', 'Compression Level (Static File)', 'text', true, 1, 9),
				new DAttr('gzipMaxFileSize', 'uint', 'Max Static File Size (bytes)', 'text', true, '1K'),
				new DAttr('gzipMinFileSize', 'uint', 'Min Static File Size (bytes)', 'text', true, 200)
		);
		$this->_tblDef[$id]->SetAttr($attrs);
	}


	protected function add_S_SEC_FILE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'File Access');

		$parseFormat = "/^[0-7]{3,4}/";
		$parseHelp1 = '3 digits or more octet number, Default required permission mask is 004 - readable by everyone';
		$parseHelp2 = '3 digits or more octet number, Default restricted permission mask is 041111 - executable+sticky+directory';

		$attrs = array(
				new DAttr('followSymbolLink', 'sel', 'Follow Symbolic Link', 'select', false, 0, $this->_options['symbolLink']),
				new DAttr('checkSymbolLink', 'bool', 'Check Symbolic Link', 'radio', false),
				new DAttr('forceStrictOwnership', 'bool', 'Force Strict Ownership', 'radio', false),
				new DAttr('requiredPermissionMask', 'parse', 'Required Permission Mask', 'text', true, $parseFormat, $parseHelp1),
				new DAttr('restrictedPermissionMask', 'parse', 'Restricted Permission Mask', 'text', true, $parseFormat, $parseHelp2),
				new DAttr('restrictedScriptPermissionMask', 'parse', 'Script Restricted Permission Mask', 'text', true, $parseFormat, $parseHelp2),
				new DAttr('restrictedDirPermissionMask', 'parse', 'Script Restricted Directory Permission Mask', 'text', true, $parseFormat, $parseHelp2)

		);

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_S_SEC_CONN($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Per Client Throttling');

		$attrs = array(
				$this->_attrs['staticReqPerSec'],
				$this->_attrs['dynReqPerSec'],
				$this->_attrs['outBandwidth'],
				$this->_attrs['inBandwidth'],
				new DAttr('softLimit', 'uint', 'Connection Soft Limit',  'text', true, 0),
				new DAttr('hardLimit', 'uint', 'Connection Hard Limit', 'text', true, 0),
				new DAttr('blockBadReq', 'bool', 'Block Bad Request', 'radio', true),
				new DAttr('gracePeriod', 'uint', 'Grace Period (sec)', 'text', true, 1, 3600),
				new DAttr('banPeriod', 'uint', 'Banned Period (sec)', 'text', true, 0)

		);
		$this->_tblDef[$id]->SetAttr($attrs);
		$this->_tblDef[$id]->_helpKey = 'TABLEperClientConnLimit';
	}

	protected function add_S_SEC_CGI($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'CGI Settings');

		$attrs = array(
				new DAttr('cgidSock', 'addr', 'CGI Daemon Socket', 'text', true, NULL, NULL, $this->_options['text_size']),

				new DAttr('maxCGIInstances', 'uint', 'Max CGI Instances', 'text', true, 1, 2000),
				new DAttr('minUID', 'uint', 'Minimum UID', 'text', true, 10),
				new DAttr('minGID', 'uint', 'Minimum GID', 'text', true, 5),
				new DAttr('forceGID', 'uint', 'Force GID', 'text', true, 0),
				$this->_attrs['cgiUmask'],
				$this->_attrs['priority']->dup(NULL, 'CGI Priority', 'CGIPriority'),
				new DAttr('CPUSoftLimit', 'uint', 'CPU Soft Limit (sec)', 'text', true, 0),
				new DAttr('CPUHardLimit', 'uint', 'CPU Hard Limit (sec)', 'text', true, 0),
				$this->_attrs['memSoftLimit'],
				$this->_attrs['memHardLimit'],
				$this->_attrs['procSoftLimit'],
				$this->_attrs['procHardLimit']
		);
		$this->_tblDef[$id]->SetAttr($attrs);
		$this->_tblDef[$id]->_helpKey = 'TABLEcgiResource';
	}

	protected function add_S_SEC_DENY($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Access Denied Directories');

		$attr = new DAttr('dir', 'dir', NULL, 'textarea', true, 2, ',', 'rows="15" cols="78" wrap=off', 2, 'accessDenyDir');
		$attrs = array( $attr );
		$this->_tblDef[$id]->SetAttr($attrs);
		$this->_tblDef[$id]->_cols = 1;
		$this->_tblDef[$id]->_helpKey = 'accessDenyDir';
	}

	protected function add_A_SEC_AC($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Access Control');

		$attrs = array(
				new DAttr('allow', 'subnet', 'Allowed List', 'textarea', true, NULL, NULL, 'rows="5" cols="60"',1, 'accessControl_allow'),
				new DAttr('deny', 'subnet', 'Denied List', 'textarea', true, NULL, NULL, 'rows="5" cols="60"', 1, 'accessControl_deny'));
		$this->_tblDef[$id]->SetAttr($attrs);
		$this->_tblDef[$id]->_cols = 1;
		$this->_tblDef[$id]->_helpKey = 'TABLEaccessControl';
	}

	protected function add_A_EXT_SEL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'New External App');
		$this->_tblDef[$id]->_subTbls = $this->_options['extTbl'];

		$attrs =  array($this->_attrs['ext_type']);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_T_EXT_SEL($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_SEL', $id);
		$this->_tblDef[$id]->_subTbls = $this->_options['tp_extTbl'];
	}

	protected function add_A_EXT_TOP($id)
	{
		$align = array('left', 'left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'External Applications', 1, 'A_EXT_SEL', $align, "application", TRUE);

		$attrs = array(
				$this->_attrs['ext_type'],
				new DAttr('name', 'cust', 'Name'),
				new DAttr('address', 'cust', 'Address'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, $this->_options['extTbl'], 'vEd')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_A_EXT_FCGI($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'FastCGI App Definition', 2);
		$attrs = array(
				$this->_attrs['ext_name'],
				$this->_attrs['ext_address'],
				$this->_attrs['note'],
				$this->_attrs['ext_maxConns'],
				$this->_attrs['ext_env'],
				$this->_attrs['ext_initTimeout'],
				$this->_attrs['ext_retryTimeout'],
				$this->_attrs['ext_persistConn'],
				$this->_attrs['pcKeepAliveTimeout'],
				$this->_attrs['ext_respBuffer'],
				$this->_attrs['ext_autoStart'],
				$this->_attrs['ext_path'],
				$this->_attrs['ext_backlog'],
				$this->_attrs['ext_instances'],
				$this->_attrs['ext_user'],
				$this->_attrs['ext_group'],
				$this->_attrs['cgiUmask'],
				$this->_attrs['ext_runOnStartUp'],
				new DAttr('extMaxIdleTime', 'uint', 'Max Idle Time', 'text', true, -1 ),
				$this->_attrs['priority']->dup(NULL, NULL, 'extAppPriority'),
				$this->_attrs['memSoftLimit'],
				$this->_attrs['memHardLimit'],
				$this->_attrs['procSoftLimit'],
				$this->_attrs['procHardLimit']);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'fcgi');
	}

	protected function add_A_EXT_FCGIAUTH($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_FCGI', $id, 'FastCGI Authorizer Definition');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'fcgiauth');
	}

	protected function add_A_EXT_LSAPI($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_FCGI', $id, 'LiteSpeed SAPI App Definition');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'lsapi');
	}

	protected function add_A_EXT_LOADBALANCER($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Load Balancer Definition', 2);

		$parseFormat = "/^(fcgi|fcigauth|lsapi|servlet|proxy)::.+$/";
		$parseHelp = 'ExtAppType::ExtAppName, e.g. fcgi::myphp, servlet::tomcat';

		$attrs = array( $this->_attrs['ext_name'],
				new DAttr('workers', 'parse', 'Workers', 'textarea', true, $parseFormat, $parseHelp, 'rows="3" cols="50"', 1),
				$this->_attrs['note'],
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'loadbalancer');
	}

	protected function add_A_EXT_LOGGER($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Piped Logger Definition', 2);
		$attrs = array( $this->_attrs['ext_name'],
				new DAttr('address', 'addr', 'Address for remote logger (Optional)', 'text', true, NULL, NULL, $this->_options['text_size']),
				//$this->_attrs['ext_address'],
				$this->_attrs['note'],
				$this->_attrs['ext_maxConns'],
				$this->_attrs['ext_env'],
				$this->_attrs['ext_path'],
				$this->_attrs['ext_instances'],
				$this->_attrs['ext_user'],
				$this->_attrs['ext_group'],
				$this->_attrs['cgiUmask'],
				$this->_attrs['priority']->dup(NULL, NULL, 'extAppPriority'));
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'logger');
	}

	protected function add_A_EXT_SERVLET($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Servlet Engine Definition', 2);
		$attrs = array( $this->_attrs['ext_name'],
				$this->_attrs['ext_address'],
				$this->_attrs['note'],
				$this->_attrs['ext_maxConns'],
				$this->_attrs['pcKeepAliveTimeout'],
				$this->_attrs['ext_env'],
				$this->_attrs['ext_initTimeout'],
				$this->_attrs['ext_retryTimeout'],
				$this->_attrs['ext_respBuffer']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'servlet');
	}

	protected function add_A_EXT_PROXY($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Web Server Definition', 2);
		$attrs = array( $this->_attrs['ext_name'],
				new DAttr('address', 'wsaddr', 'Address', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'expWSAddress'),
				$this->_attrs['note'],
				$this->_attrs['ext_maxConns'],
				$this->_attrs['pcKeepAliveTimeout'],
				$this->_attrs['ext_env'],
				$this->_attrs['ext_initTimeout'],
				$this->_attrs['ext_retryTimeout'],
				$this->_attrs['ext_respBuffer']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'proxy');
	}

	protected function add_T_EXT_TOP($id)
	{
		$align = array('center', 'center', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'External Applications', 1, 'T_EXT_SEL', $align);

		$attrs = array(
				$this->_attrs['ext_type'],
				new DAttr('name', 'cust', 'Name'),
				new DAttr('address', 'cust', 'Address'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, $this->_options['tp_extTbl'], 'vEd')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_T_EXT_FCGI($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_FCGI', $id);
		$this->_tblDef[$id]->_dattrs[0] = $this->_attrs['tp_name'];
	}

	protected function add_T_EXT_FCGIAUTH($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_FCGIAUTH', $id);
		$this->_tblDef[$id]->_dattrs[0] = $this->_attrs['tp_name'];
	}

	protected function add_T_EXT_LSAPI($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_LSAPI', $id);
		$this->_tblDef[$id]->_dattrs[0] = $this->_attrs['tp_name'];
	}

	protected function add_T_EXT_LOADBALANCER($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_LOADBALANCER', $id);
		$this->_tblDef[$id]->_dattrs[0] = $this->_attrs['tp_name'];
	}

	protected function add_T_EXT_LOGGER($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_LOGGER', $id);
		$this->_tblDef[$id]->_dattrs[0] = $this->_attrs['tp_name'];
	}

	protected function add_T_EXT_SERVLET($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_SERVLET', $id);
		$this->_tblDef[$id]->_dattrs[0] = $this->_attrs['tp_name'];
	}

	protected function add_T_EXT_PROXY($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('A_EXT_PROXY', $id);
		$this->_tblDef[$id]->_dattrs[0] = $this->_attrs['tp_name'];
	}

	protected function add_A_SCRIPT($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Script Handler Definition', 2);

		$attrs = array(
				$this->_attrs['suffix'],
				$this->_attrs['scriptHandler_type'],
				$this->_attrs['scriptHandler'],
				$this->_attrs['note'],
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'suffix');
	}

	protected function add_A_SCRIPT_FILE($id)
	{
		// for file use only
		$this->_tblDef[$id] = new DTbl($id, 'Script Handler Definition', 2);

		$attrs = array(new DAttr('add', 'cust', NULL));
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_A_SCRIPT_TOP($id)
	{
		$align = array('center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Script Handler Definition', 1, 'A_SCRIPT', $align, "script");

		$attrs = array(
				$this->_attrs['suffix'],
				$this->_attrs['scriptHandler_type'],
				$this->_attrs['scriptHandler'],
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'A_SCRIPT', 'Ed'));
		$this->_tblDef[$id]->SetAttr($attrs, 'suffix');
	}

	protected function add_S_RAILS($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Rack/Rails Settings');
		$attrs = array(
				new DAttr('rubyBin', 'file', 'Ruby Path', 'text', true, 1, 'x', $this->_options['text_size']),
				$this->_attrs['railsEnv'],
				$this->_attrs['ext_maxConns'],
				$this->_attrs['ext_env'],
				$this->_attrs['ext_initTimeout'],
				$this->_attrs['ext_retryTimeout'],
				$this->_attrs['pcKeepAliveTimeout'],
				$this->_attrs['ext_respBuffer'],
				$this->_attrs['ext_backlog'],
				$this->_attrs['ext_runOnStartUp'],
				new DAttr('extMaxIdleTime', 'uint', 'Max Idle Time', 'text', true, -1 ),
				$this->_attrs['priority']->dup(NULL, NULL, 'extAppPriority'),
				$this->_attrs['memSoftLimit'],
				$this->_attrs['memHardLimit'],
				$this->_attrs['procSoftLimit'],
				$this->_attrs['procHardLimit']);
		$this->_tblDef[$id]->SetAttr($attrs);
		$this->_tblDef[$id]->_helpKey = 'TABLErailsDefault';
	}

	protected function add_V_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host List', 1, 'V_TOPD', $align, "web");

		$attrs = array(
				new DAttr('name', 'cust', 'Name'),
				new DAttr('vhRoot', 'cust', 'Virtual Host Root'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'V_TOPD', 'Xd'));
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_V_BASE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Base');

		$attrs = array(
				new DAttr('name', 'vhname', 'Virtual Host Name', 'text', false, NULL, NULL, NULL, 0, 'vhName'),
				new DAttr('vhRoot', 'cust', 'Virtual Host Root', 'text', false, NULL, NULL, $this->_options['text_size']),// do not check path for vhroot, it may be different owner
				new DAttr('configFile', 'filevh', 'Config File', 'text', false, 3, 'rwc', $this->_options['text_size']),
				$this->_attrs['note']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_V_BASE_CONN($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Connection');

		$attrs = array(
				$this->_attrs['vh_maxKeepAliveReq'],
				$this->_attrs['vh_smartKeepAlive']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_V_BASE_THROTTLE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Per Client Throttling');

		$attrs = array(
				$this->_attrs['staticReqPerSec'],
				$this->_attrs['dynReqPerSec'],
				$this->_attrs['outBandwidth'],
				$this->_attrs['inBandwidth']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_L_TOP($id)
	{
		$align = array('center', 'center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Listener List', 1, 'L_GENERAL', $align, 'link', TRUE);

		$attrs = array(
				new DAttr('name', 'cust', 'Listener Name'),
				new DAttr('ip', 'cust', 'IP Address'),
				new DAttr('port', 'cust', 'Port'),
				new DAttr('secure', 'bool', 'Secure'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'L_GENERAL', 'Xd')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_ADM_L_TOP($id)
	{
		$align = array('center', 'center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Listener List', 1, 'ADM_L_GENERAL', $align, 'link', TRUE);

		$attrs = array(
				new DAttr('name', 'cust', 'Listener Name'),
				new DAttr('ip', 'cust', 'IP Address'),
				new DAttr('port', 'cust', 'Port'),
				new DAttr('secure', 'bool', 'Secure'),
				new DAttr('action', 'action', 'Action', NULL, FALSE, 'ADM_L_GENERAL', 'Xd')//cannot delete all
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_ADM_L_GENERAL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Admin Listener Address Settings');

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

		$attrs = array(
				$name,
				$addr, $ip,	$port,
				new DAttr('secure', 'bool', 'Secure', 'radio', false, NULL, NULL, NULL, 0, 'listenerSecure'),
				$this->_attrs['note'],
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_L_VHMAP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Mappings', 2);

		$attrs = array(
				$this->_attrs['l_vhost'],
				$this->_attrs['l_domain']);
		$this->_tblDef[$id]->SetAttr($attrs, 'vhost');
		$this->_tblDef[$id]->_helpKey = 'TABLEvirtualHostMapping';
	}

	protected function add_L_VHMAP_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Mappings', 1, 'L_VHMAP', $align, "web_link", FALSE);

		$attrs = array(
				$this->_attrs['l_vhost'],
				$this->_attrs['l_domain'],
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'L_VHMAP', 'Ed'));

		$this->_tblDef[$id]->SetAttr($attrs, 'vhost');
		$this->_tblDef[$id]->_helpKey = 'TABLEvirtualHostMapping';
	}

	protected function add_LVT_SSL_CERT($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'SSL Private Key & Certificate');

		$attrs = array(
				new DAttr('keyFile', 'cust', 'Private Key File', 'text', true, NULL, '', $this->_options['text_size']),
				new DAttr('certFile', 'cust', 'Certificate File', 'text', true, NULL, '', $this->_options['text_size']),
				new DAttr('certChain', 'bool', 'Chained Certificate', 'radio'),
				new DAttr('CACertPath', 'cust', 'CA Certificate Path', 'text', true, NULL, '', $this->_options['text_size']),
				new DAttr('CACertFile', 'cust', 'CA Certificate File', 'text', true, NULL, '', $this->_options['text_size']),
		);
		$this->_tblDef[$id]->SetAttr($attrs);
		$this->_tblDef[$id]->_helpKey = 'TABLEsslCert';
	}

	protected function add_LVT_SSL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'SSL Protocol');
		$sslversion = array('1'=>'SSL v3.0', '2'=>'TLS v1.0', '4'=>'TLS v1.1', '8'=>'TLS v1.2');

		$attrs = array(
				new DAttr('sslProtocol', 'checkboxOr', 'Protocol Version', 'checkboxgroup', TRUE, NULL, $sslversion),
				new DAttr('ciphers', 'cust', 'Ciphers', 'text', true, NULL, '', $this->_options['text_size']),
				new DAttr('enableECDHE', 'bool', 'Enable ECDH Key Exchange', 'radio', true),
				new DAttr('enableDHE', 'bool', 'Enable DH Key Exchange', 'radio', true),
				new DAttr('DHParam', 'cust', 'DH Parameter', 'text', true, NULL, '', $this->_options['text_size']),
		);

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_L_SSL_FEATURE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Security & Features');

		$attrs = array(
			$this->_attrs['ssl_renegProtection'],
			new DAttr('enableSpdy',	'checkboxOr', 'Enable SPDY', 'checkboxgroup', true, NULL, array('1'=>'SPDY/2', '2'=>'SPDY/3', '0'=>'None'))
		    );
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_VT_SSL_FEATURE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Security');

		$attrs = array($this->_attrs['ssl_renegProtection']);

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_LVT_SSL_OCSP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'OCSP Stapling');

		$attrs = array(
				new DAttr('enableStapling', 'bool', 'Enable OCSP Stapling', 'radio', true),
				new DAttr('ocspRespMaxAge', 'uint', 'OCSP Response Max Age (secs)', 'text', true, -1, NULL),
				new DAttr('ocspResponder', 'httpurl', 'OCSP Responder', 'text', true, NULL, NULL, $this->_options['text_size']),
				new DAttr('ocspCACerts', 'cust', 'OCSP CA Certificates', 'text', true, NULL, '', $this->_options['text_size']),
		);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_T_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Template List', 1, 'T_TOPD', $align, "form");

		$attrs = array(
				new DAttr('name', 'cust', 'Name'),
				new DAttr('listeners', 'cust', 'Mapped Listeners'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'T_TOPD', 'Xd'));
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_T_TOPD($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Template', 2);

		$attrs = array(
				new DAttr('name', 'vhname', 'Template Name', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'templateName'),
				$this->_attrs['tp_templateFile'],
				$this->_attrs['tp_listeners'],
				$this->_attrs['note']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_T_MEMBER_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Member Virtual Hosts', 1, 'T_MEMBER', $align, "web", FALSE);

		$attrs = array(
				$this->_attrs['tp_vhName'],
				$this->_attrs['tp_vhDomain'],
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'T_MEMBER', 'vEdi'));

		$this->_tblDef[$id]->SetAttr($attrs, 'vhName');
	}

	protected function add_T_MEMBER($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Member Virtual Host', 2);
		$vhroot = new DAttr('vhRoot', 'cust', 'Member Virtual Host Root', 'text', true, NULL, NULL, $this->_options['text_size'], 0, 'memberVHRoot');
		$vhroot->_note = 'Optional: You can set up a different VH_ROOT other than the default one';
		$attrs = array(
				$this->_attrs['tp_vhName'],
				$this->_attrs['tp_vhDomain'],
				new DAttr('vhAliases', 'domain', 'Aliases', 'text', true, NULL, NULL, $this->_options['text_size'], 1, 'templateVHAliases'),
				$vhroot
		);

		$this->_tblDef[$id]->SetAttr($attrs, 'vhName');
	}


	protected function add_V_GENERAL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'General');

		$attrs = array(
				new DAttr('docRoot', 'cust', 'Document Root', 'text', false, NULL, '', $this->_options['text_size']),//no validation, maybe suexec owner
				$this->_attrs['adminEmails'],
				$this->_attrs['vh_enableGzip'],
				$this->_attrs['enableIpGeo'],
				$this->_attrs['vh_spdyAdHeader']);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_V_LOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Log');

		$attrs = array(
				new DAttr('useServer', 'bool', 'Use Server\'s Log', 'radio', false, NULL, NULL, NULL, 0, 'logUseServer'),
				$this->_attrs['fileName3']->dup(NULL, NULL, 'log_fileName'),
				new DAttr('logLevel', 'sel', 'Log Level', 'select', true, 0, $this->_options['logLevel'], NULL, 0, 'vhlog_logLevel'),
				$this->_attrs['rollingSize']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'fileName');
	}

	protected function add_V_ACLOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Access Log');
		$use_options = array(0=>'Own Log File', 1=>'Server\'s Log File', 2=>'Disabled');

		$attrs = array(
				new DAttr('useServer', 'sel', 'Log Control', 'select', false, 0, $use_options, NULL, 0, 'aclogUseServer'),
				$this->_attrs['fileName3']->dup(NULL, NULL, 'accessLog_fileName'),
				new DAttr('pipedLogger', 'sel1', 'Piped Logger', 'select', true,
						'extprocessor:logger', NULL, NULL, 0, 'accessLog_pipedLogger'),
				$this->_attrs['logFormat'],
				$this->_attrs['logHeaders'],
				$this->_attrs['rollingSize'],
				$this->_attrs['keepDays'],
				new DAttr('bytesLog', 'file0', 'Bytes log', 'text', true, 3, 'r', $this->_options['text_size'], 0, 'accessLog_bytesLog'),
				$this->_attrs['compressArchive']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'fileName');
	}

	protected function add_VT_INDXF($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Index Files');
		$options = array(0=>'No', 1=>'Yes', 2=>'Addition');

		$attrs = array(
				new DAttr('useServer', 'sel', 'Use Server Index Files', 'select', false, 0, $options, NULL, 0, 'indexUseServer'),
				$this->_attrs['indexFiles'],
				$this->_attrs['autoIndex'],
				new DAttr('autoIndexURI', 'uri', 'Auto Index URI', 'text', true, NULL, NULL, $this->_options['text_size'])
		);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_VT_ERRPG_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Customized Error Pages', 1, 'VT_ERRPG', $align, "file", TRUE);
		$errCodeOptions = &CustStatusCode::Get();
		$attrs = array(
				new DAttr('errCode', 'sel', 'Error Code', NULL, false, 0, $errCodeOptions),
				new DAttr('url', 'cust', 'URL'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'VT_ERRPG', 'Ed'));
		$this->_tblDef[$id]->SetAttr($attrs, 'errCode');
		$this->_tblDef[$id]->_helpKey = 'TABLEerrPage';
	}

	protected function add_VT_ERRPG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Customized Error Pages', 2);
		$errCodeOptions = &CustStatusCode::Get();
		$attrs = array(
				new DAttr('errCode', 'sel', 'Error Code', 'select', false, 0, $errCodeOptions),
				new DAttr('url', 'custom', 'URL', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'errURL'),
				$this->_attrs['note'],
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'errCode');
		$this->_tblDef[$id]->_helpKey = 'TABLEerrPage';
	}

	protected function get_realm_attrs()
	{
		return array(
				'realm_type' => new DAttr('type', 'sel', 'DB Type', 'select', false, NULL, $this->_options['realmType'], NULL, 0, 'realmType'),
				'realm_name' => new DAttr('name', 'name', 'Realm Name', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'realmName'),
				'realm_udb_maxCacheSize' => new DAttr('userDB:maxCacheSize', 'uint', 'User DB Max Cache Size',	'text', true, 0, '100K', NULL, 0, 'userDBMaxCacheSize'),
				'realm_udb_cacheTimeout' => new DAttr('userDB:cacheTimeout', 'uint', 'User DB Cache Timeout (secs)', 'text', true, 0, 3600, NULL, 0, 'userDBCacheTimeout'),
				'realm_gdb_maxCacheSize' => new DAttr('groupDB:maxCacheSize', 'uint', 'Group DB Max Cache Size', 'text', true, 0, '100K', NULL, 0, 'groupDBMaxCacheSize'),
				'realm_gdb_cacheTimeout' => new DAttr('groupDB:cacheTimeout', 'uint','Group DB Cache Timeout (secs)', 'text', true, 0, 3600, NULL, 0, 'groupDBCacheTimeout'));
	}

	protected function add_V_REALM_FILE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Password File Realm Definition', 2);
		$udbLoc = new DAttr('userDB:location', 'file', 'User DB Location', 'text', false, 3, 'rc', $this->_options['text_size'], 0, 'userDBLocation');
		$udbLoc->_href = '&t1=V_UDB_TOP&r1=$R';
		$gdbLoc = new DAttr('groupDB:location', 'file', 'Group DB Location', 'text', true, 3, 'rc', $this->_options['text_size'], 0, 'GroupDBLocation');
		$gdbLoc->_href = '&t1=V_GDB_TOP&r1=$R';

		$realm_attr = $this->get_realm_attrs();
		$attrs = array(
				$realm_attr['realm_name'],
				$this->_attrs['note'],
				$udbLoc,
				$realm_attr['realm_udb_maxCacheSize'],
				$realm_attr['realm_udb_cacheTimeout'],
				$gdbLoc,
				$realm_attr['realm_gdb_maxCacheSize'],
				$realm_attr['realm_gdb_cacheTimeout']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'file');
	}

	protected function add_T_REALM_FILE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Password File Realm Definition', 2);

		$realm_attr = $this->get_realm_attrs();
		$attrs = array(
				$realm_attr['realm_name'],
				$this->_attrs['note'],
				new DAttr('userDB:location', 'cust', 'User DB Location', 'text', false, 3, 'rc', $this->_options['text_size'], 0, 'userDBLocation'),
				$realm_attr['realm_udb_maxCacheSize'],
				$realm_attr['realm_udb_cacheTimeout'],
				new DAttr('groupDB:location', 'cust', 'Group DB Location', 'text', true, 3, 'rc', $this->_options['text_size'], 0, 'GroupDBLocation'),
				$realm_attr['realm_gdb_maxCacheSize'],
				$realm_attr['realm_gdb_cacheTimeout']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'file');
	}

	protected function add_V_UDB_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'User DB Entries', 1, 'V_UDB', $align, 'administrator', FALSE);

		$attrs = array(
				new DAttr('name', 'cust', 'User Name'),
				new DAttr('group', 'cust', 'Groups'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'V_UDB', 'Ed')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_showParentRef = true;
	}

	protected function add_V_UDB($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'User DB Entry', 2);

		$attrs = array(
				new DAttr('name', 'name', 'User Name', 'text', false, NULL, NULL, $this->_options['text_size']),
				new DAttr('group', 'name', 'Groups', 'text', true, NULL, NULL, $this->_options['text_size'], 1),
				new DAttr('pass', 'cust', 'New Password', 'password', true, NULL, NULL, $this->_options['text_size']),
				new DAttr('pass1', 'cust', 'Retype Password', 'password', true, NULL, NULL, $this->_options['text_size'])
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_showParentRef = true;
	}

	protected function add_V_GDB_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Group DB Entries', 1, 'V_GDB', $align, NULL, FALSE);

		$attrs = array(
				new DAttr('name', 'cust', 'Group Name'),
				new DAttr('users', 'cust', 'Users'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'V_GDB', 'Ed')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name'); // 'GDB!$0'
		$this->_tblDef[$id]->_showParentRef = true;
	}

	protected function add_V_GDB($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Group DB Entry', 2);
		$users = new DAttr('users', 'name', 'Users', 'textarea', true, NULL, NULL, 'rows="15" cols="50"', 1);
		$users->SetGlue(' ');

		$attrs = array(
				new DAttr('name', 'name', 'Group Name', 'text', false, NULL, NULL, $this->_options['text_size']),
				$users,
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
		$this->_tblDef[$id]->_showParentRef = true;
	}

	protected function add_VT_REWRITE_CTRL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Rewrite Control');

		$attrs = array(
				new DAttr('enable', 'bool', 'Enable Rewrite', 'radio', true, NULL, NULL, NULL, 0, 'enableRewrite'),
				new DAttr('logLevel', 'uint', 'Log Level', 'text', true, 0, 9, NULL, 0, 'rewriteLogLevel'));

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_VT_REWRITE_MAP_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Rewrite Map', 1, 'VT_REWRITE_MAP', $align, "redirect", TRUE);

		$attrs = array(
				new DAttr('name', 'cust', 'Name'),
				new DAttr('location', 'cust', 'Location'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'VT_REWRITE_MAP', 'Ed')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_VT_REWRITE_MAP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Rewrite Map');

		$parseFormat = "/^((txt|rnd):\/*)|(int:(toupper|tolower|escape|unescape))$/";
		$parseHelp = 'txt:/location or rnd:/location or int:(toupper|tolower|escape|unescape)';

		$attrs = array(
				new DAttr('name', 'name',  'Name', 'text', false, NULL, NULL, NULL, 0, 'rewriteMapName'),
				new DAttr('location', 'parse', 'Location', 'text', true, $parseFormat, $parseHelp, $this->_options['text_size'], 0, 'rewriteMapLocation'),
				$this->_attrs['note'],
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_VT_REWRITE_RULE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Rewrite Rules');

		$attrs = array(
				new DAttr('rules', 'cust', NULL, 'textarea1', true, NULL, NULL, 'rows="15" cols="60" wrap=off'));
		$this->_tblDef[$id]->SetAttr($attrs);
		$this->_tblDef[$id]->_cols = 1;
		$this->_tblDef[$id]->_helpKey = 'rewriteRules';
	}

	protected function add_VT_CTX_SEL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'New Context');
		$this->_tblDef[$id]->_subTbls = $this->_options['ctxTbl'];

		$attrs = array($this->_attrs['ctx_type']);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function get_ctx_attrs($type)
	{
		if ($type == 'auth') {
			return array(
					new DAttr('realm', 'sel1', 'Realm', 'select', true, 'realm'),
					new DAttr('authName', 'name', 'Authentication Name', 'text'),
					new DAttr('required', 'cust', 'Require (Authorized Users/Groups)', 'text'),
					new DAttr('accessControl:allow', 'subnet', 'Access Allowed', 'textarea', true, NULL, NULL, 'rows="3" cols="60"', 1, 'accessAllowed'),
					new DAttr('accessControl:deny', 'subnet', 'Access Denied', 'textarea', true, NULL, NULL, 'rows="3" cols="60"', 1, 'accessDenied'),
					new DAttr('authorizer', 'sel1', 'Authorizer', 'select', true, 'extprocessor:fcgiauth', NULL, NULL, 0, 'extAuthorizer')
			);
		}
		if ($type == 'rewrite') {
			return array(
					new DAttr('rewrite:enable', 'bool', 'Enable Rewrite', 'radio', true, NULL, NULL, NULL, 0, 'enableRewrite'),
					new DAttr('rewrite:inherit', 'bool', 'Rewrite Inherit', 'radio', true, NULL, NULL, NULL, 0, 'rewriteInherit'),
					new DAttr('rewrite:base', 'uri', 'Rewrite Base', 'text', true, NULL, NULL, $this->_options['text_size'], 0, 'rewriteBase'),
					new DAttr('rewrite:rules', 'cust', 'Rewrite Rules', 'textarea1', true, NULL, NULL, 'rows="6" cols="60" wrap=off', 0, 'rewriteRules')
			);
		}
		if ($type == 'charset') {
			return array(
					new DAttr('addDefaultCharset', 'sel', 'Add Default Charset', 'select', true, 0, $this->_options['on_off']),
					new DAttr('defaultCharsetCustomized', 'cust', 'Customized Default Charset', 'text'),
					$this->_attrs['enableIpGeo']
			);
		}
		if ($type == 'uri') {
			return array(
					$this->_attrs['ctx_uri'],
					$this->_attrs['ctx_order']);
		}

	}

	protected function add_VT_WBSOCK_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Web Socket Proxy Setup', 1, 'VT_WBSOCK', $align, 'web_link', TRUE);

		$attrs = array(
				new DAttr('uri', 'cust', 'URI'),
				new DAttr('address', 'cust', 'Address'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'VT_WBSOCK', 'Ed'));
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
	}

	protected function add_VT_WBSOCK($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Web Socket Definition', 2);

		$attrs = array(
				$this->_attrs['ctx_uri']->dup(NULL,NULL,'wsuri'),
				$this->_attrs['ext_address']->dup(NULL, NULL, 'wsaddr'),
				$this->_attrs['note'],
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'uri');
	}

	protected function add_T_GENERAL1($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Base');

		$attrs = array(
				$this->_attrs['tp_vhRoot'],
				$this->_attrs['tp_vrFile']->dup('configFile', 'Config File', NULL),
				$this->_attrs['vh_maxKeepAliveReq'],
				$this->_attrs['vh_smartKeepAlive']
		);
		// to do: need check path contain VH variable.
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_T_GENERAL2($id)
	{
		// for file use only
		$this->_tblDef[$id] = new DTbl($id, 'Base2');
		$attrs = array(
				$this->_attrs['tp_vrFile']->dup('docRoot', 'Document Root', NULL),
				$this->_attrs['adminEmails'],
				$this->_attrs['vh_enableGzip'],
				$this->_attrs['enableIpGeo'],
				$this->_attrs['vh_spdyAdHeader']
		);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_T_SEC_FILE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'File Access Control');

		$attrs = array(
				$this->_attrs['vh_allowSymbolLink'],
				$this->_attrs['vh_enableScript'],
				$this->_attrs['vh_restrained']
		);

		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_T_SEC_CONN($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Per Client Throttling');

		$attrs = array(
				$this->_attrs['staticReqPerSec'],
				$this->_attrs['dynReqPerSec'],
				$this->_attrs['outBandwidth'],
				$this->_attrs['inBandwidth']
		);
		$this->_tblDef[$id]->SetAttr($attrs);
	}

	protected function add_T_LOG($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('V_LOG', $id);
		$this->_tblDef[$id]->_dattrs[1] =  $this->_attrs['tp_vrFile'];
	}

	protected function add_T_ACLOG($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('V_ACLOG', $id);
		$this->_tblDef[$id]->_dattrs[1] =  $this->_attrs['tp_vrFile'];
	}

	protected function add_ADM_PHP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'General');

		$attrs = array(
				new DAttr('enableCoreDump', 'bool', 'Enable Core Dump', 'radio', false ),
				new DAttr('sessionTimeout', 'uint', 'Session Timeout (secs)', 'text', true, 60, NULL, NULL, 0, 'consoleSessionTimeout')
		);
		$this->_tblDef[$id]->SetAttr($attrs);
	}


	protected function add_ADM_USR_TOP($id)
	{
		$align = array('left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Web Admin Users', 1, 'ADM_USR_NEW', $align, "administrator", FALSE);

		$attrs = array(
				new DAttr('name', 'cust', 'Admin User Name'),
				new DAttr('action', 'action', 'Action', NULL, FALSE, 'ADM_USR', 'Ed') //not allow null - cannot delete all
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_ADM_USR($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Web Admin User', 2);

		$attrs = array(
				new DAttr('name', 'name', 'Web Admin User Name', 'text', false, NULL, NULL, $this->_options['text_size']),
				new DAttr('oldpass', 'cust', 'Old Password', 'password', false, NULL, NULL, $this->_options['text_size']),
				new DAttr('pass', 'cust', 'New Password', 'password', false, NULL, NULL, $this->_options['text_size']),
				new DAttr('pass1', 'cust', 'Retype Password', 'password', false, NULL, NULL, $this->_options['text_size'])
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_ADM_USR_NEW($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'New Web Admin User', 2);

		$attrs = array(
				new DAttr('name', 'name', 'Web Admin User Name', 'text', false, NULL, NULL, $this->_options['text_size']),
				new DAttr('pass', 'cust', 'New Password', 'password', false, NULL, NULL, $this->_options['text_size']),
				new DAttr('pass1', 'cust', 'Retype Password', 'password', false, NULL, NULL, $this->_options['text_size'])
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'name');
	}

	protected function add_ADM_ACLOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Access Log');
		$use_options = array(0=>'Own Log File', 1=>'Server\'s Log File', 2=>'Disabled');

		$attrs = array(
				new DAttr('useServer', 'sel', 'Log Control', 'select', false, 0, $use_options, NULL, 0, 'aclogUseServer'),
				$this->_attrs['fileName3']->dup(NULL, NULL, 'accessLog_fileName'),
				$this->_attrs['logFormat'],
				$this->_attrs['logHeaders'],
				$this->_attrs['rollingSize'],
				$this->_attrs['keepDays'],
				new DAttr('bytesLog', 'file0', 'Bytes log', 'text', true, 3, 'r', $this->_options['text_size'], 0, 'accessLog_bytesLog'),
				$this->_attrs['compressArchive']
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'fileName');
	}

	protected function add_S_MIME_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'MIME Type Definition', 1, 'S_MIME', $align, "file", FALSE);

		$attrs = array(
				new DAttr('suffix', 'cust', 'Suffix'),
				new DAttr('type', 'cust', 'MIME Type'),
				new DAttr('action', 'action', 'Action', NULL, TRUE, 'S_MIME', 'Ed')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'suffix');
	}

	protected function add_S_MIME($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'MIME Type Entry', 2);
		$attrs = array(
				$this->_attrs['suffix']->dup('suffix', 'Suffixes', mimesuffix),
				new DAttr('type', 'parse', 'Mime Type', 'text', false,
						"/^[A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+(\s*;?.*)$/", 'like text/html', $this->_options['text_size'], 0, 'mimetype')
		);
		$this->_tblDef[$id]->SetAttr($attrs, 'suffix');
	}

	protected function add_SERVICE_SUSPENDVH($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Suspend Vhosts');

		$attr0 = new DAttr('suspendedVhosts', 'vhname', NULL);
		$attr0->multiInd = 1;
		$attr0->SetFlag(DAttr::BM_HIDE | DAttr::BM_NOEDIT);
		$attrs = array( $attr0);
		$this->_tblDef[$id]->SetAttr($attrs);
	}
}

