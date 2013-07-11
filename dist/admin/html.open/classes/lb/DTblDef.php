<?php

class DTblDef
{
	protected $_tblDef = array();
	protected $_options;
	protected $_attrs;

	protected function __construct()
	{
		$this->loadCommonOptions();
		$this->loadCommonAttrs();
	}

	public static function GetInstance()
	{
        if ( !isset($GLOBALS['_DTblDef_']) )
			$GLOBALS['_DTblDef_'] = new DTblDef();
		return $GLOBALS['_DTblDef_'];
	}
	
	public function GetTblDef($tblId)
	{
		if (!isset( $this->_tblDef[$tblId] ))
		{
			$funcname = 'add_' . $tblId;
			if (!method_exists($this, $funcname))
				die('invalid tid');			
			$this->$funcname($tblId);
		}
		return $this->_tblDef[$tblId];
	}

	protected function DupTblDef($tblId, $newId, $newTitle=NULL)
	{
		$tbl = $this->GetTblDef($tblId);
		$newtbl = $tbl->dup($newId, $newTitle);
		return $newtbl;
	}
	
	protected function loadCommonOptions()
	{
		$this->_options = array();
		$this->_options['text_size'] = 'size="120"';
		$this->_options['tp_vname'] = array( '/\$VH_NAME/', 'requiring variable $VH_NAME');
		$this->_options['tp_vroot'] = array( '/(\$VH_NAME)|(\$VH_ROOT)/',
											  'requiring variable $VH_NAME or $VH_ROOT');

		$this->_options['logHeaders'] = array('1'=>'Referrer', '2'=>'UserAgent', '4'=>'Host', '0'=>'None');

		$this->_options['workerType'] = array(
				'fcgi'=>'Fast CGI App', 'fcgiauth'=>'Fast CGI Authorizer',
				'lsapi'=>'LSAPI App',
				'servlet'=>'Servlet Engine', 'proxy'=>'Web Server');

		$this->_options['ext_respBuffer'] = array('0'=>'No', '1'=>'Yes',
												  '2'=>'No for Non-Parsed-Header (NPH)');

		$this->_options['chrootmode'] = array( 0=>'Same as Server', 1=>'Virtual Host Root', 2=>'Customized Chroot Path');

		$this->_options['ctxType'] = array(
				'loadbalancer'=> 'Load Balancer',
				'redirect'=>'Redirect');

		$this->_options['ctxTbl'] = array(
				0=>'type', 1=>'VH_CTXB',
				'loadbalancer'=>'VH_CTXB',
				'redirect'=>'VH_CTXR');

		$this->_options['on_off'] = array('off'=>'Off', 'on'=>'On');

		$this->_options['realmType'] = array('file' => 'Password File', 'LDAP' => 'LDAP' );
		$this->_options['realmTbl'] = array( 0=>'type', 1=>'VH_REALM_LDAP', 'file' => 'VH_REALM_FILE', 'LDAP' => 'VH_REALM_LDAP' );
		$this->_options['tp_realmTbl'] = array( 0=>'type', 1=>'VH_REALM_LDAP', 'file' => 'TP_REALM_FILE', 'LDAP' => 'VH_REALM_LDAP' );

		$this->_options['lbType'] = array('layer7' => 'Layer 7');
		
		$this->_options['logLevel'] = array('ERROR'=>'ERROR', 'WARN'=>'WARNING',
									'NOTICE'=>'NOTICE', 'INFO'=>'INFO', 'DEBUG'=>'DEBUG');
		$this->_options['updateInterval'] = array( 86400=>'Daily', 604800=>'Weekly', 2592000=>'Monthly' );

		$ipv6 = array();
		if ( $_SERVER['LSLB_IPV6_ADDRS'] != '' )
		{
			$ipv6['[ANY]'] = '[ANY] IPv6';
			$ips = explode( ',', $_SERVER['LSLB_IPV6_ADDRS'] );
			foreach( $ips as $ip )
				{
					$pos = strpos( $ip, ':' );
					if ( $pos )
					{
						$aip = substr( $ip, $pos+1 );
						$ipv6[$aip] = $aip;
					}
				}
		}
		$ipo = array();
		$ipo['ANY'] = 'ANY';
		$ips = explode(',', $_SERVER['LSLB_IPV4_ADDRS']);
		foreach( $ips as $ip )
		{
			$pos = strpos($ip, ':');
			if ( $pos )
			{
				$aip = substr($ip, $pos+1);
				$ipo[$aip] = $aip;
				if ( $aip != '127.0.0.1' )
					$ipv6["[::FFFF:$aip]"] = "[::FFFF:$aip]";
			}
		}
		if ( $_SERVER['LSLB_IPV6_ADDRS'] != '' )
			$this->_options['ip'] = $ipo + $ipv6;
		else
			$this->_options['ip'] = $ipo;

	}

	protected function loadCommonAttrs()
	{
		$ctxOrder = new DAttr('order', 'cust', 'Order');
		$ctxOrder->_FDE = 'NNN';

		$this->_attrs = array(
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
			'enableExpires' => new DAttr('enableExpires', 'bool', 'Enable Expires', 'radio'),
			'expiresDefault' => new DAttr('expiresDefault', 'parse', 'Expires Default', 'text', true,
										  "/^[AaMm]\d+$/",
										  '[A|M]###, A means client access time, M means file modified time, ### is number of seconds'),
			'expiresByType' => new DAttr(
				'expiresByType', 'parse', 'Expires By Type', 'textarea', true,
				"/^(\*\/\*)|([A-z0-9_\-\.\+]+\/\*)|([A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+)=[AaMm]\d+$/",
				'MIME_type=A|Mseconds, MIME type can be like */*, text/*, text/html',
				'rows="2" cols="50"', 1),

			'extraHeaders' => new DAttr(
				'extraHeaders', 'cust', 'Extra Headers', 'textarea1', true,
				NULL, 'Extra headers will be inserted into HTTP response Header, one header per line, put "NONE" to disable headers inherited from parent content.',
			    'rows="2" cols="60" wrap=off'),


			'realm_type' => new DAttr('type', 'sel', 'DB Type', 'select', false, NULL, $this->_options['realmType'], NULL, 0, 'realmType'),
			'realm_name' => new DAttr('name', 'name', 'Realm Name', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'realmName'),
			'realm_udb_maxCacheSize' => new DAttr('userDB:maxCacheSize', 'uint', 'User DB Max Cache Size',
												  'text', true, 0, '100K', NULL, 0, 'userDBMaxCacheSize'),
			'realm_udb_cacheTimeout' => new DAttr('userDB:cacheTimeout', 'uint',
												  'User DB Cache Timeout (secs)', 'text', true, 0, 3600, NULL, 0, 'userDBCacheTimeout'),
			'realm_gdb_maxCacheSize' => new DAttr('groupDB:maxCacheSize', 'uint',
												  'Group DB Max Cache Size', 'text', true, 0, '100K', NULL, 0, 'groupDBMaxCacheSize'),
			'realm_gdb_cacheTimeout' => new DAttr('groupDB:cacheTimeout', 'uint',
												  'Group DB Cache Timeout (secs)', 'text', true, 0, 3600, NULL, 0, 'groupDBCacheTimeout'),
				

			'lb_mode' => new DAttr('mode', 'sel', 'Mode', 'select', false, 0, array('0'=>'Stateless', '1'=>'Stateful'), NULL, 0, 'lb_mode'),
			'lb_strategy' => new DAttr('strategy', 'sel', 'Strategy', 'select', false, 0, array('0'=>'Least Load', '1'=>'Round Robin', '2'=>'Least Session') ), 
			'lb_type' => new DAttr('type', 'sel', 'Type', 'select', false, NULL, $this->_options['lbType'], NULL, 0, 'lb_type'),
			'lb_insertCookie' => new DAttr('insertCookie', 'bool', 'Insert Tracking Cookie', 'radio', true),
			'lb_cookieName' => new DAttr('cookieName', 'cust', 'Tracking Cookie Name', 'text', true, NULL, NULL, $this->_options['text_size']),
			'lb_sessionExMethod' => new DAttr('sessionExMethod','checkboxOr', 'Session Extraction', 'checkboxgroup',true, NULL, 
				array('1'=>'IP Address', '2'=>'Basic Authentication', '4'=>'Query String', '8'=>'Cookies',
					'16'=>'SSL Session', '32'=>'JVM Route', '64'=>'URL Path Parameter')),
					
			'lb_sessionId' => new DAttr('sessionId', 'cust', 'Session ID String', 'text', true, NULL, NULL, $this->_options['text_size']),
			'lb_sessionTimeout' => new DAttr('sessionTimeout', 'uint', 'Session Timeout', 'text', true, 0),
			'lb_errMessage' => new DAttr('errMessage', 'cust', 'Custom Error Messages', 'textarea', true, NULL, NULL,  'rows="6" cols="50"'),
			
			'worker_type' => new DAttr('type', 'sel', 'Type', 'select', false, NULL, $this->_options['workerType'], NULL, 0, 'worker_type'),
			'workerGroupEnabled' => new DAttr('workerGroupEnabled', 'bool', 'Enabled', 'radio', true ),
			'name'=> new DAttr('name', 'name', 'Name', 'text', false),
			'pcKeepAliveTimeout' => new DAttr('pcKeepAliveTimeout', 'uint', 'Connection Keepalive Timeout', 'text', true, -1, 10000 ),

			'memSoftLimit' => new DAttr('memSoftLimit', 'uint', 'Memory Soft Limit (bytes)', 'text', true, 0),
			'memHardLimit' => new DAttr('memHardLimit', 'uint', 'Memory Hard Limit (bytes)', 'text', true, 0),
			'procSoftLimit' => new DAttr('procSoftLimit', 'uint', 'Process Soft Limit', 'text', true, 0),
			'procHardLimit' => new DAttr('procHardLimit', 'uint', 'Process Hard Limit','text', true, 0),

			'l_vhost' => new DAttr('vhost', 'sel1', 'Virtual Host', 'select', false, array('VHS') ),
			'l_domain' => new DAttr('domain', 'domain', 'Domains', 'text', false, NULL, NULL, $this->_options['text_size'], 1, 'domainName'),
			'tp_templateFile' => new DAttr('templateFile', 'filep', 'Template File', 'text', false, 2, 'rw', $this->_options['text_size']),
			'tp_listeners' => new DAttr('listeners', 'sel2', 'Mapped Listeners', 'text', false, array('LNS'), NULL, $this->_options['text_size'], 1, 'mappedListeners'),
			'tp_vhName' => new DAttr('vhName', 'vhname', 'Virtual Host Name', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'templateVHName'),
			'tp_vhDomain' => new DAttr('vhDomain', 'domain', 'Domain', 'text', true, NULL, NULL, $this->_options['text_size'], 0, 'templateVHDomain'),

			'tp_vrFile' => new DAttr('fileName', 'parse', 'File Name', 'text', true,
									   $this->_options['tp_vroot'][0], $this->_options['tp_vroot'][1],
										 $this->_options['text_size']),

			'tp_name' => new DAttr('name', 'parse', 'Name', 'text', false,
								   $this->_options['tp_vname'][0], $this->_options['tp_vname'][1]),
								   
			'defaultCluster' => new DAttr('defaultCluster', 'sel1', 'Default Cluster', 'select', false, array('CLS')), 

			'vh_maxKeepAliveReq' => new DAttr('maxKeepAliveReq', 'uint', 'Max Keep-Alive Requests', 'text', true, 0, 32767, NULL, 0, 'vhMaxKeepAliveReq'),
			'vh_smartKeepAlive' => new DAttr('smartKeepAlive', 'bool', 'Smart Keep-Alive', 'radio', true, NULL, NULL, NULL, 0, 'vhSmartKeepAlive'),
			'vh_enableGzip' => new DAttr('enableGzip', 'bool', 'Enable GZIP Compression', 'radio'),
			'dynReqPerSec' => new DAttr('dynReqPerSec', 'uint', 'Requests/second', 'text', true, 0),
			'outBandwidth' => new DAttr('outBandwidth', 'uint', 'Outbound Bandwidth (bytes/sec)', 'text', true, 0),
			'inBandwidth' => new DAttr('inBandwidth', 'uint', 'Inbound Bandwidth (bytes/sec)', 'text', true, 0),

			'ctx_order' => $ctxOrder,
			'ctx_type' => new DAttr('type', 'sel', 'Type', 'select', false, NULL, $this->_options['ctxType']),
			'ctx_uri' => new DAttr('uri', 'expuri', 'URI', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'expuri'),
			'ctx_location' => new DAttr('location', 'cust', 'Location', 'text', false, NULL, NULL, $this->_options['text_size']),
			'ctx_allowBrowse' => new DAttr('allowBrowse', 'bool', 'Accessible', 'radio', false),
			'ctx_shandler' => new DAttr('handler', 'sel1', 'Servlet Engine', 'select', false,
										array('ext:servlet')),
			'ctx_realm' => new DAttr('realm', 'sel1', 'Realm', 'select', true, array('realms')),
			'ctx_authName' => new DAttr('authName', 'name', 'Authentication Name', 'text'),
			'ctx_required' => new DAttr('required', 'cust', 'Required', 'text'),
			'ctx_allow' => new DAttr('accessControl:allow', 'subnet', 'Access Allowed', 'textarea', true, NULL, NULL, 'rows="3" cols="60"', 1, 'accessAllowed'),
			'ctx_deny' => new DAttr('accessControl:deny', 'subnet', 'Access Denied', 'textarea', true, NULL, NULL, 'rows="3" cols="60"', 1, 'accessDenied'),
										
			'geoipDBFile' => new DAttr( 'geoipDBFile', 'filep', 'DB File Path', 'text', false, 2, 'r', $this->_options['text_size']),

			'geoipDBCache' => new DAttr( 'geoipDBCache', 'sel',
							'DB Cache Type', 'select', true, 0,
							array( ''=>'', 'Standard'=>'Standard',
								   'MemoryCache' => 'MemoryCache',
								   'CheckCache' => 'CheckCache',
								   'IndexCache' => 'IndexCache' ) ),
			'enableIpGeo' => new DAttr( 'enableIpGeo', 'bool', 'Enable IP GeoLocation', 'radio'),
			'note' => new DAttr('note', 'cust', 'Notes', 'textarea', true, NULL, NULL, 'rows="4" cols="60" wrap=off')

			);

	}

	//	DAttr($key, $type, $label,  $inputType, $allowNull,$min, $max, $inputAttr, $multiInd)

	protected function add_SERV_PROCESS($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Server Process');

		$attr1 = new DAttr('autoRestart', 'bool', 'Auto Restart', 'radio', false);
		$attr2 = new DAttr('enableChroot', 'bool', 'Enable chroot', 'radio', false);

		$attr3 = new DAttr('chrootPath', 'cust', 'chroot Path', 'text', true);
		$attr3->_FDE = 'YYN';

		$runningAs = new DAttr('runningAs', 'cust', 'Running As', 'text', false, NULL, NULL, $this->_options['text_size'] . ' readonly');
		$runningAs->_FDE = 'NYN';

		$user = new DAttr('user', 'cust', NULL, 'text', false);
		$user->_FDE = 'YNN';
		$group = new DAttr('group', 'cust', NULL, 'text', false);
		$group->_FDE = 'YNN';
		$priority = $this->_attrs['priority'];
		$priority->_helpKey = 'serverPriority';
		
		$attrs = array(
			new DAttr('serverName', 'name', 'Server Name', 'text', false),
			$runningAs, $user, $group,
			$priority,
			$attr3, $attr2,
			new DAttr('inMemBufSize', 'uint', 'Memory I/O Buffer', 'text', false, 0, NULL),
			new DAttr('swappingDir', 'cust', 'Swapping Directory', 'text', false, 1, 'rw', $this->_options['text_size']),
			$attr1
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general');
	}

	protected function add_SERV_GENERAL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'General Settings');

		$attr2 = new DAttr('disableInitLogRotation', 'bool', 'Disable Initial Log Rotation', 'radio', true);

		$attr3 = new DAttr('showVersionNumber', 'bool', 'Expose Server Version', 'radio', false);

		$attr4 = new DAttr('useIpInProxyHeader', 'sel', 'Use Client IP in Header', 'select', true, NULL, array('0'=>'No', '1'=>'Yes', '2'=>'Trusted IP Only') );
		
		$attr5 = new DAttr('autoUpdateInterval', 'sel', 'Check For Update', 'select', false,
							NULL, $this->_options['updateInterval'] );
		$attr6 = new DAttr('autoUpdateDownloadPkg', 'bool', 'Download Updates', 'radio', false);

		$attr7 = new DAttr('adminRoot', 'cust', NULL, NULL, false);
		$attr7->_FDE = 'YNN';

		$attrs = array( $attr2, $attr3,	$this->_attrs['enableIpGeo'],
				$attr4, $attr5, $attr6,
						$this->_attrs['adminEmails'], $attr7 );
		$this->_tblDef[$id]->setAttr($attrs, 'general');
	}

	protected function add_SERV_LOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Server Log');

		$dbg_options = array('10'=>'HIGH', '5'=>'MEDIUM', '2'=>'LOW', '0'=>'NONE');
		$log_filename = $this->_attrs['fileName2'];
		$log_filename->_helpKey = 'log_fileName';
		$log_level = new DAttr('logLevel', 'sel', 'Log Level', 'select', false, 0, $this->_options['logLevel'], NULL, 0, 'log_logLevel');
		$log_debugLevel = new DAttr('debugLevel', 'sel', 'Debug Level', 'select', false, 0, $dbg_options, NULL, 0, 'log_debugLevel');
		$attrs = array(
			$log_filename,
			$log_level,
			$log_debugLevel, 
			$this->_attrs['rollingSize'],
			new DAttr('enableStderrLog', 'bool', 'Enable stderr Log', 'radio', true, NULL, NULL, NULL, 0, 'log_enableStderrLog')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general:log', 'logging:log');
	}
	
	protected function add_SERV_ACLOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Access Log');
		$aclog_fileName = $this->_attrs['fileName2'];
		$aclog_fileName->_helpKey = 'accessLog_fileName';
		$attrs = array(
			$aclog_fileName,
			new DAttr('pipedLogger', 'cust', 'Piped Logger', 'text', true,
					NULL, NULL, $this->_options['text_size'], 0, 'accessLog_pipedLogger'),
			$this->_attrs['logFormat'],
			$this->_attrs['logHeaders'],
			$this->_attrs['rollingSize'],
			$this->_attrs['keepDays'],
			$this->_attrs['compressArchive']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general:aclog', 'logging:accessLog');
	}
	

	protected function add_SERV_TUNING_CONN($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Connection');

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
			new DAttr('maxConnections', 'uint', 'Max Connections', 'text', false, 1),
			new DAttr('maxSSLConnections', 'uint', 'Max SSL Connections', 'text', false, 0),
			new DAttr('connTimeout', 'uint', 'Connection Timeout (secs)', 'text', false, 10, 1000000),
			new DAttr('maxKeepAliveReq', 'uint', 'Max Keep-Alive Requests', 'text', false, 0, 32767),
			new DAttr('smartKeepAlive', 'bool', 'Smart Keep-Alive', 'radio', false),
			new DAttr('keepAliveTimeout', 'uint', 'Keep-Alive Timeout (secs)', 'text', false, 0, 60),
			new DAttr('sndBufSize', 'uint', 'Send Buffer Size (bytes)', 'text', false, 0, 65535),
			new DAttr('rcvBufSize', 'uint', 'Receive Buffer Size (bytes)', 'text', false, 0, 65535),
			new DAttr('eventDispatcher', 'sel', 'I/O Event Dispatcher', 'select', true, 0, $edoptions),
			new DAttr('SSLCryptoDevice', 'sel', 'SSL Hardware Accelerator', 'select', true, 0, $ssloptions)
			);
		$this->_tblDef[$id]->setAttr($attrs, 'tuning', 'tuning');
	}

	protected function add_SERV_TUNING_REQ($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Request/Response');

		$attrs = array(
			new DAttr('maxReqURLLen', 'uint', 'Max Request URL Length (bytes)', 'text', false, 200, 8192),
			new DAttr('maxReqHeaderSize', 'uint', 'Max Request Header Size (bytes)', 'text', false, 512, 16380),
			new DAttr('maxReqBodySize', 'uint', 'Max Request Body Size (bytes)', 'text', false, '1M', NULL),
			new DAttr('maxDynRespHeaderSize', 'uint', 'Max Dynamic Response Header Size (bytes)', 'text', false, 200, 8192),
			new DAttr('maxDynRespSize', 'uint', 'Max Dynamic Response Body Size (bytes)', 'text', false, '1M', '2047M')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'tuning', 'tuning');
	}

	protected function add_SERV_TUNING_GZIP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'GZIP Compression');

		$parseFormat = "/^(\!)?(\*\/\*)|([A-z0-9_\-\.\+]+\/\*)|([A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+)$/";
		$parseHelp = '(!)MIME types, like */*, text/*, text/html, text/*, !text/css';

		$attrs = array(
			new DAttr('enableGzipCompress', 'bool', 'Enable Compression', 'radio', false),
			new DAttr('gzipCompressLevel', 'uint', 'Compression Level (Dynamic Content)', 'text', true, 1, 9),
			new DAttr('compressibleTypes', 'parse', 'Compressible Types', 'textarea', true, $parseFormat, $parseHelp, 'rows="5" cols="50"', 1)
			);
		$this->_tblDef[$id]->setAttr($attrs, 'tuning', 'tuning');
	}

	protected function add_SERV_SEC_CONN($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Per Client Throttling');

		$attrs = array(
			$this->_attrs['dynReqPerSec'],
			$this->_attrs['outBandwidth'],
			$this->_attrs['inBandwidth'],
			new DAttr('softLimit', 'uint', 'Connection Soft Limit',  'text', true, 0),
			new DAttr('hardLimit', 'uint', 'Connection Hard Limit', 'text', true, 0),
			new DAttr('gracePeriod', 'uint', 'Grace Period (sec)', 'text', true, 1, 3600),
			new DAttr('banPeriod', 'uint', 'Banned Period (sec)', 'text', true, 0)

			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:conn', 'security:perClientConnLimit');
		$this->_tblDef[$id]->_helpKey = 'TABLEperClientConnLimit';
	}

	protected function add_LB_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Clusters List', 1, 'LB_TOP_D', $align, "network");
		$attrs = array(
		new DAttr('name', 'cust', 'Name', NULL, false, NULL, NULL, NULL, 0, 'lb_name'),
		$this->_attrs['lb_type'],
		$this->_attrs['lb_mode'],
		$this->_attrs['lb_strategy'],
		new DAttr('action', 'action', 'Action', NULL, false, 'LB_TOP_D', 'Xd'));
		$this->_tblDef[$id]->setAttr($attrs, 'clusters');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_LB_TOP_D($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Define a New Cluster', 2);

		$attrs = array( 
			new DAttr('name', 'name', 'Cluster Name', 'text', false, NULL, NULL, NULL, 0, 'lb_name'),
			$this->_attrs['lb_type'],
			$this->_attrs['lb_mode'],
			$this->_attrs['lb_strategy'],
			$this->_attrs['lb_sessionExMethod'],
			$this->_attrs['lb_sessionId'],
			$this->_attrs['lb_sessionTimeout'],
			$this->_attrs['lb_insertCookie'],
			$this->_attrs['lb_cookieName'],
			$this->_attrs['lb_errMessage'],
			$this->_attrs['note']);
		$this->_tblDef[$id]->setAttr($attrs, 'clusters', 'loadBalancerList:loadBalancer');
		$this->_tblDef[$id]->setRepeated('name');
	}
	
	protected function add_LB_GENERAL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Load Balancer Definition', 2);
		$name = $this->_attrs['name'];
		$name->_helpKey = 'lb_name';

		$attrs = array( $name,
			$this->_attrs['lb_type'],
			$this->_attrs['lb_mode'],
			$this->_attrs['lb_strategy'],
			$this->_attrs['note']);
		$this->_tblDef[$id]->setAttr($attrs);
	}

	protected function add_LB_SESSION($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Session Management', 2);

		$attrs = array($this->_attrs['lb_sessionExMethod'],
			$this->_attrs['lb_sessionId'],
			$this->_attrs['lb_sessionTimeout'],
			$this->_attrs['lb_insertCookie'],
			$this->_attrs['lb_cookieName']);
		$this->_tblDef[$id]->setAttr($attrs);
	}

	protected function add_LB_ERRMSG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Custom Error Messages', 2);

		$attrs = array( 
				new DAttr('errMessage', 'cust', null, 'textarea', true, NULL, NULL,  'rows="15" cols="70"')
		);
		$this->_tblDef[$id]->setAttr($attrs);
		$this->_tblDef[$id]->_helpKey = 'TABLEClusterErrMsg';
	}

	protected function add_LB_WORKERGRP_TOP($id)
	{
		$align = array('left', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Worker Groups', 1, 'LB_WORKERGRP', $align, 'application');

		$attrs = array(
		new DAttr('name', 'cust', 'Name'),
		$this->_attrs['worker_type'],
		$this->_attrs['workerGroupEnabled'],
		new DAttr('action', 'action', 'Action', NULL, false, 'LB_WORKERGRP', 'vEd')
		);
		$this->_tblDef[$id]->setAttr($attrs, 'workers', 'workerGroupList:workerGroup');
		$this->_tblDef[$id]->setRepeated( 'name' );
	}

	protected function add_LB_WORKERGRP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Worker Group Definition', 2);
		$respBufferOptions = array('0'=>'No', '1'=>'Yes',
			'2'=>'No for Non-Parsed-Header (NPH)');

		$parseFormat = "/^(\(.+\))?([^)(]+->)?[^->:)(]+(:[0-9]+)?$/";
		$parseHelp = "[(NodeTag)][SourceIP->]DestinationAddress[:DestinationPort]";

		$attr_nodeAddr = new DAttr('nodeAddresses', 'parse', 'Node Address List', 'textarea', true, $parseFormat, $parseHelp, 'rows="3" cols="50"', 1);
		$attr_nodeAddr->_note = 'comma separated list of [(NodeTag)][SourceIP->]DestinationAddress[:DestinationPort], e.g. 192.168.0.11, (node1)192.168.0.11, (node1)192.168.0.10->192.168.0.11:80';
		
		$name = $this->_attrs['name'];
		$name->_helpKey = 'workerName';
		$attrs = array(
		$name,
		$this->_attrs['worker_type'],
		$this->_attrs['workerGroupEnabled'],
		new DAttr('sourceIP', 'sel', 'Source IP', 'select', true, 0, $this->_options['ip']),
		new DAttr('defaultTargetPort', 'uint', 'Default Target Port', 'text', true, 1, 65535),
		new DAttr('maxConns', 'uint', 'Max Connections', 'text', false, 0, 2000),
		new DAttr('env', 'parse', 'Environment', 'textarea', true,"/\S+=\S+/", 'PROPERTY=VALUE', 'rows="5" cols="50" wrap=off', 2),
		new DAttr('initTimeout', 'uint', 'Initial Request Timeout (secs)', 'text', false, 1),
		new DAttr('retryTimeout', 'uint', 'Retry Timeout (secs)', 'text', false, 0),
		new DAttr('persistConn', 'bool', 'Persistent Connection', 'radio', true ),
		new DAttr('pcKeepAliveTimeout', 'uint', 'Connection Keepalive Timeout', 'text', true, -1, 10000 ),
		new DAttr('respBuffer', 'sel', 'Response Buffering', 'select', false, 0, $respBufferOptions),
		new DAttr('pingUrl', 'url', 'Ping URL', 'text', true, NULL, NULL, $this->_options['text_size']),
		new DAttr('pingInterval', 'uint', 'Ping Interval (secs)', 'text', true, 1),
		$attr_nodeAddr,
		);
		$this->_tblDef[$id]->setAttr($attrs, 'workers', 'workerGroupList:workerGroup');
		$this->_tblDef[$id]->setRepeated( 'name' );
	}
	
	protected function add_A_SECAC($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Access Control');

		$attrs = array(
			new DAttr('allow', 'subnet', 'Allowed List', 'textarea', true, NULL, NULL, 'rows="5" cols="60"',1, 'accessControl_allow'),
			new DAttr('deny', 'subnet', 'Denied List', 'textarea', true, NULL, NULL, 'rows="5" cols="60"', 1, 'accessControl_deny'));
		$this->_tblDef[$id]->setAttr($attrs, 'sec:ac', 'security:accessControl' );
		$this->_tblDef[$id]->_cols = 1;
		$this->_tblDef[$id]->_helpKey = 'TABLEaccessControl';
	}
	
	
	protected function add_A_SEC_CC($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Request Filter');

		$attrs = array(
			new DAttr('enableCensorship', 'bool', 'Enable Request Filtering', 'radio'),
			new DAttr('logLevel', 'uint', 'Log Level', 'text', true, 0, 9, NULL, 0, 'censorLogLevel'),
			new DAttr('defaultAction', 'cust', 'Default Action', 'text', true, NULL, NULL, $this->_options['text_size']),
			new DAttr('scanPOST', 'bool', 'Scan Request Body', 'radio'));
		$this->_tblDef[$id]->setAttr($attrs, 'sec:cc', 'security:censorshipControl' );
	}

	protected function add_A_SEC_CR_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Request Filtering Rule Set', 1, 'A_SEC_CR', $align, "filter");

		$attrs = array(
			new DAttr('name', 'cust', 'Name', NULL, true, NULL, NULL, NULL, 0, 'censorRuleSetName'),
			new DAttr('ruleSetAction', 'cust', 'Action'),
			new DAttr('enabled', 'bool', 'Enabled', 'radio', false, NULL, NULL, NULL, 0, 'censorRuleSetEnabled'),
			new DAttr('action', 'action', 'Action', NULL, false, 'A_SEC_CR', 'vEd')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:cr', 'security:censorshipRuleSet');
		$this->_tblDef[$id]->setRepeated('name');
	}
	
	protected function add_A_SEC_CR($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Request Filtering Rule Set');

		$attrs = array(
			new DAttr('name', 'cust', 'Name', 'text', false, null, null, $this->_options['text_size'], 0, 'censorRuleSetName'),
			new DAttr('ruleSetAction', 'cust', 'Action', 'text', true, null, null, $this->_options['text_size']),
			new DAttr('enabled', 'bool', 'Enabled', 'radio', false, NULL, NULL, NULL, 0, 'censorRuleSetEnabled'),
			new DAttr('ruleSet', 'cust', 'Rules Definition', 'textarea1', true, NULL, NULL, 'rows="15" cols="60" wrap=off', 0, 'censorRuleSet'),
			$this->_attrs['note'],
			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:cr', 'security:censorshipRuleSet');
		$this->_tblDef[$id]->setRepeated('name');
		
	}

	protected function add_A_GEOIP_TOP($id)
	{
		$align = array('center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'IP to GeoLocation DB', 1, 'A_GEOIP', $align, "database");

		$attrs = array(
			$this->_attrs['geoipDBFile'],
			$this->_attrs['geoipDBCache'],
			new DAttr('action', 'action', 'Action', NULL, false, 'A_GEOIP', 'Ed'));
		$this->_tblDef[$id]->setAttr($attrs, 'geoipDB');
		$this->_tblDef[$id]->setRepeated('geoipDBFile');
		$this->_tblDef[$id]->_helpKey = 'TABLEgeolocationDB';
	}

	protected function add_A_GEOIP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'IP to GeoLocation DB configuration', 2);

		$attrs = array(
			$this->_attrs['geoipDBFile'],
			$this->_attrs['geoipDBCache'],
			$this->_attrs['note'],
			);
		$this->_tblDef[$id]->setAttr($attrs, 'geoipDB', 'ipToGeo:geoipDB');
		$this->_tblDef[$id]->setRepeated('geoipDBFile');
		$this->_tblDef[$id]->_helpKey = 'TABLEgeolocationDB';
	}


	protected function add_VH_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host List', 1, 'VH_TOP_D', $align, "web");

		$attrs = array(
			new DAttr('name', 'cust', 'Name'),
			new DAttr('defaultCluster', 'cust', 'Default Cluster'),
			new DAttr('action', 'action', 'Action', NULL, false, 'VH_TOP_D', 'Xd'));
		$this->_tblDef[$id]->setAttr($attrs, 'vhTop');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_VH_TOP_D($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Hosts', 2);

		$attrs = array(
			new DAttr('name', 'vhname', 'Virtual Host Name', 'text', false),
			$this->_attrs['defaultCluster'],
			new DAttr('configFile', 'file', 'Configure File', 'text', false, 3, 'rwc', $this->_options['text_size']),
			$this->_attrs['note']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'vhTop', 'virtualHostList:virtualHost');
		$this->_tblDef[$id]->setRepeated('name');
	}
	
	protected function add_VH_BASE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Base');

		$attrs = array(
			new DAttr('name', 'vhname', 'Virtual Host Name', 'text', false),
			$this->_attrs['defaultCluster'],
			new DAttr('configFile', 'file', 'Configure File', 'text', false, 3, 'rwc', $this->_options['text_size']),
			$this->_attrs['note']
			);
		$this->_tblDef[$id]->setAttr($attrs);

	}
	
	protected function add_VH_GENERAL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'General');

		$attrs = array(
			$this->_attrs['vh_maxKeepAliveReq'],
			$this->_attrs['vh_smartKeepAlive'],
			$this->_attrs['vh_enableGzip'],
			$this->_attrs['enableIpGeo'],
			$this->_attrs['adminEmails'],
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general');
	}

	protected function add_VH_ACLOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Access Log');
		$use_options = array(0=>'Own Log File', 1=>'Server\'s Log File', 2=>'Disabled');
		$log_filename = $this->_attrs['fileName3'];
		$log_filename->_helpKey = 'accessLog_fileName';

		$attrs = array(
			new DAttr('useServer', 'sel', 'Log Control', 'select', false, 0, $use_options, NULL, 0, 'aclogUseServer'),
			$log_filename,
			new DAttr('pipedLogger', 'cust', 'Piped Logger', 'text', true, 
						NULL, NULL, $this->_options['text_size'], 0, 'accessLog_pipedLogger'),
			//new DAttr('pipedLogger', 'sel1', 'Piped Logger', 'select', true,
				//						  array( 'ext:logger'), NULL, NULL, 0, 'accessLog_pipedLogger'),
			$this->_attrs['logFormat'],
			$this->_attrs['logHeaders'],
			$this->_attrs['rollingSize'],
			$this->_attrs['keepDays'],
			new DAttr('bytesLog', 'file0', 'Bytes log', 'text', true, 3, 'r', $this->_options['text_size'], 0, 'accessLog_bytesLog'),
			$this->_attrs['compressArchive']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general:aclog', 'logging:accessLog');
	}
	
	
	protected function add_VH_ERRPG_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Customized Error Pages', 1, 'VH_ERRPG', $align, "file");
		$errCodeOptions = &CustStatusCode::Get();
		$attrs = array(
			new DAttr('errCode', 'sel', 'Error Code', NULL, false, 0, $errCodeOptions),
			new DAttr('action', 'action', 'Action', NULL, false, 'VH_ERRPG', 'Ed'));
		$this->_tblDef[$id]->setAttr($attrs, 'general:errorPage');
		$this->_tblDef[$id]->setRepeated('errCode');
		$this->_tblDef[$id]->_helpKey = 'TABLEerrPage';
	}

	protected function add_VH_ERRPG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Customized Error Pages', 2);
		$errCodeOptions = &CustStatusCode::Get();
		$attrs = array(
			new DAttr('errCode', 'sel', 'Error Code', 'select', false, 0, $errCodeOptions),
  		    $this->_attrs['lb_errMessage'],
			$this->_attrs['note'],
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general:errorPage', 'customErrorPages:errorPage');
		$this->_tblDef[$id]->setRepeated('errCode');
		$this->_tblDef[$id]->_helpKey = 'TABLEerrPage';
	}
	

	protected function add_VH_SECHL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Hotlink Protection');

		$attrs = array(
			new DAttr('enableHotlinkCtrl', 'bool', 'Enable Hotlink Protection', 'radio', false),
			new DAttr('suffixes', 'suffix', 'Suffix', 'text', false, NULL, NULL, $this->_options['text_size'], 1),
			new DAttr('redirectUri', 'url', 'Redirect URL', 'text', true, NULL, NULL, $this->_options['text_size'] ),
			new DAttr('allowDirectAccess', 'bool', 'Allow Direct Access', 'radio', false),
			new DAttr('onlySelf', 'bool', 'Only Self Reference', 'radio', false),
			new DAttr('allowedHosts', 'domain', 'Allowed Domains', 'text', true, NULL, NULL, $this->_options['text_size'], 1),
			new DAttr('matchedHosts', 'cust', 'REGEX Matched Domains', 'text', true, NULL, NULL, $this->_options['text_size'])
			);
		$this->_tblDef[$id]->setAttr($attrs,  'sec:hotlink', 'security:hotlinkCtrl');
		$this->_tblDef[$id]->_helpKey = 'TABLEvhHotlink';
	}

	protected function add_VH_REALM_TOP($id)
	{
		$align = array('center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Realms List', 1, 'VH_REALM_SEL', $align, "shield");

		$attrs = array(
			new DAttr('name', 'cust', 'Realm Name'),
			$this->_attrs['realm_type'],
			new DAttr('action', 'action', 'Action', NULL, false, $this->_options['realmTbl'], 'vEd')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:realm');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_helpKey = 'TABLErealms';
	}

	protected function add_VH_REALM_SEL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'New Realm');
		$this->_tblDef[$id]->_subTbls = $this->_options['realmTbl'];

		$attrs[] = $this->_attrs['realm_type'];
 		$this->_tblDef[$id]->setAttr($attrs, 'sec:realm', 'security:realmList:realm');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_helpKey = 'TABLErealms';
	}

	protected function add_VH_REALM_FILE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Password File Realm Definition', 2);
		$udbLoc = new DAttr('userDB:location', 'file', 'User DB Location', 'text', false, 3, 'rc', $this->_options['text_size'], 0, 'userDBLocation');
		$udbLoc->_href = '&t1=VH_UDB_TOP&r1=$R';
		$gdbLoc = new DAttr('groupDB:location', 'file', 'Group DB Location', 'text', true, 3, 'rc', $this->_options['text_size'], 0, 'GroupDBLocation');
		$gdbLoc->_href = '&t1=VH_GDB_TOP&r1=$R';

		$attrs = array(
			$this->_attrs['realm_name'],
			$udbLoc,
			$this->_attrs['note'],
			$this->_attrs['realm_udb_maxCacheSize'],
			$this->_attrs['realm_udb_cacheTimeout'],
			$gdbLoc,
			$this->_attrs['realm_gdb_maxCacheSize'],
			$this->_attrs['realm_gdb_cacheTimeout']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:realm');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'file');
	}

	protected function add_VH_REALM_LDAP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'LDAP Realm Definition', 2);

		$parseFormat = "/^(ldap|ldaps):\/\//";
		$parseHelp = 'start with ldap:// or ldaps://';

		$attrs = array(
			$this->_attrs['realm_name'],
			new DAttr('userDB:location', 'parse', 'User DB Location', 'text', false, $parseFormat, $parseHelp, $this->_options['text_size'], 0, 'userDBLocation'),
			new DAttr('userDB:attrPasswd', 'name', 'Password Attribute', 'text', true, NULL, NULL, NULL, 0, 'userDB_attrPasswd'),
			new DAttr('userDB:attrMemberOf', 'name', 'Member-of Attribute', 'text', true, NULL, NULL, NULL, 0, 'userDB_attrMemberOf'),
			$this->_attrs['realm_udb_maxCacheSize'],
			$this->_attrs['realm_udb_cacheTimeout'],
			new DAttr('groupDB:location', 'parse', 'Group DB Location', 'text', true, $parseFormat, $parseHelp, $this->_options['text_size'], 0, 'GroupDBLocation'),
			new DAttr('groupDB:attrGroupMember', 'name', 'Group Member Attribute', 'text', true, NULL, NULL, NULL, 0, 'groupDB_attrGroupMember'),
			$this->_attrs['realm_gdb_maxCacheSize'],
			$this->_attrs['realm_gdb_cacheTimeout'],
			new DAttr('LDAPBindDN', 'cust', 'LDAP Bind DN', 'text', true, NULL, NULL, $this->_options['text_size']),
			new DAttr('LDAPBindPasswd', 'cust', 'LDAP Bind Password', 'text'),
			$this->_attrs['note'],
			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:realm');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'LDAP');
	}
	
	protected function add_VH_UDB_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'User DB Entries', 1, 'VH_UDB', $align);

		$attrs = array(
			new DAttr('name', 'cust', 'User Name'),
			new DAttr('group', 'cust', 'Groups'),
			new DAttr('action', 'action', 'Action', NULL, false, 'VH_UDB', 'Ed')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'UDB');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_hasB = true;
	}

	protected function add_VH_UDB($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'User DB Entry', 2);

		$attrs = array(
			new DAttr('name', 'name', 'User Name', 'text', false, NULL, NULL, $this->_options['text_size']),
			new DAttr('group', 'name', 'Groups', 'text', true, NULL, NULL, $this->_options['text_size'], 1),
			new DAttr('pass', 'cust', 'New Password', 'password', true, NULL, NULL, $this->_options['text_size']),
			new DAttr('pass1', 'cust', 'Retype Password', 'password', true, NULL, NULL, $this->_options['text_size'])
			);
		$this->_tblDef[$id]->setAttr($attrs, 'UDB!$0');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_defaultExtract = array('passwd'=>'$$');
		$this->_tblDef[$id]->_hasB = true;
	}

	protected function add_VH_GDB_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Group DB Entries', 1, 'VH_GDB', $align);

		$attrs = array(
			new DAttr('name', 'cust', 'Group Name'),
			new DAttr('users', 'cust', 'Users'),
			new DAttr('action', 'action', 'Action', NULL, false, 'VH_GDB', 'Ed')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'GDB');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_hasB = true;
	}

	protected function add_VH_GDB($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Group DB Entry', 2);
		$users = new DAttr('users', 'name', 'Users', 'textarea', true, NULL, NULL, 'rows="15" cols="50"', 1);
		$users->_glue = ' ';

		$attrs = array(
			new DAttr('name', 'name', 'Group Name', 'text', false, NULL, NULL, $this->_options['text_size']),
			$users,
			);
		$this->_tblDef[$id]->setAttr($attrs, 'GDB!$0');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_hasB = true;
	}

	protected function add_VH_REWRITE_CTRL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Rewrite Control');

		$attrs = array(
			new DAttr('enable', 'bool', 'Enable Rewrite', 'radio', true, NULL, NULL, NULL, 0, 'enableRewrite'),
			new DAttr('logLevel', 'uint', 'Log Level', 'text', true, 0, 9, NULL, 0, 'rewriteLogLevel'));

		$this->_tblDef[$id]->setAttr($attrs, 'rewrite', 'rewrite');

	}
	
	protected function add_VH_REWRITE_RULE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Rewrite Rules');

		$attrs = array(
			new DAttr('rules', 'cust', NULL, 'textarea1', true, NULL, NULL, 'rows="15" cols="60" wrap=off'));

		$this->_tblDef[$id]->setAttr($attrs, 'rewrite', 'rewrite');
		$this->_tblDef[$id]->_helpKey = 'rewriteRules';
	}

	protected function add_VH_REWRITE_MAP_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Rewrite Map', 1, 'VH_REWRITE_MAP', $align, "redirect");

		$attrs = array(
			new DAttr('name', 'cust', 'Name'),
			new DAttr('location', 'cust', 'Location'),
			new DAttr('action', 'action', 'Action', NULL, false, 'VH_REWRITE_MAP', 'Ed')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'rewrite:map', 'rewrite:map');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_VH_REWRITE_MAP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Rewrite Map');

		$parseFormat = "/^((txt|rnd):\/*)|(int:(toupper|tolower|escape|unescape))$/";
		$parseHelp = 'txt:/location or rnd:/location or int:(toupper|tolower|escape|unescape)';

		$attrs = array(
			new DAttr('name', 'name',  'Name', 'text', false, NULL, NULL, NULL, 0, 'rewriteMapName'),
			new DAttr('location', 'parse', 'Location', 'text', true, $parseFormat, $parseHelp, $this->_options['text_size'], 0, 'rewriteMapLocation'),
			$this->_attrs['note'],
			);
		$this->_tblDef[$id]->setAttr($attrs, 'rewrite:map', 'rewrite:map');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_VH_CTX_TOP($id)
	{
		$align = array('center', 'left', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Context List', 1, 'VH_CTX_SEL', $align, array("NULL" => "file", "proxy" => "network", "redirect" => "redirect"));
		$o = new DAttr('order', 'ctxseq', 'Order');
		$o->_href = '&ctxseq=';
		$o->_FDE = 'NYN';

		$attrs = array(
			$this->_attrs['ctx_type'],
			new DAttr('uri', 'cust', 'URI'),
			$o,
			new DAttr('action', 'action', 'Action', NULL, false, $this->_options['ctxTbl'], 'vEd')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'context');
		$this->_tblDef[$id]->setRepeated('uri');
	}

	protected function add_VH_CTX_SEL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'New Context');
		$this->_tblDef[$id]->_subTbls = $this->_options['ctxTbl'];

		$attrs[] = $this->_attrs['ctx_type'];
		$this->_tblDef[$id]->setAttr($attrs, 'context', 'contextList:context');
		$this->_tblDef[$id]->setRepeated('uri');
	}

	protected function add_VH_CTXB($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Load Balancer Context Definition', 2);
		$attrs = array(
			$this->_attrs['ctx_uri'],
			$this->_attrs['ctx_order'],
			new DAttr('handler', 'sel1', 'Cluster', 'select', false, array('CLS'), NULL, NULL, 0, 'extCluster'), 
			$this->_attrs['note'],
			$this->_attrs['ctx_allow'],
			$this->_attrs['ctx_deny'],
			$this->_attrs['ctx_realm'],
			$this->_attrs['ctx_authName'],
			$this->_attrs['ctx_required'],
			new DAttr('addDefaultCharset', 'sel', 'Add Default Charset', 'select', true, 0, $this->_options['on_off']),
			new DAttr('defaultCharsetCustomized', 'cust', 'Customized Default Charset', 'text'),
			new DAttr('rewrite:enable', 'bool', 'Enable Rewrite', 'radio', true, NULL, NULL, NULL, 0, 'enableRewrite'),
			new DAttr('rewrite:inherit', 'bool', 'Rewrite Inherit', 'radio', true, NULL, NULL, NULL, 0, 'rewriteInherit'),
			new DAttr('rewrite:base', 'uri', 'Rewrite Base', 'text', true, NULL, NULL, $this->_options['text_size'], 0, 'rewriteBase'),
			new DAttr('rewrite:rules','cust', 'Rewrite Rules', 'textarea1', true, NULL,NULL, 'rows="6" cols="60" wrap=off', 0, 'rewriteRules'),
			$this->_attrs['enableIpGeo']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'context');
		$this->_tblDef[$id]->setRepeated('uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'loadbalancer');
		$this->_tblDef[$id]->_helpKey = 'TABLEloadBalancerContext';
	}

	protected function add_VH_CTXR($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Redirect Context Definition', 2);

		$options = &CustStatusCode::GetRedirect();

		$attrs = array(
			$this->_attrs['ctx_uri'],
			$this->_attrs['ctx_order'],
			new DAttr('externalRedirect', 'bool',  'External Redirect', 'radio', false, NULL, NULL, NULL, 0, 'externalredirect'),
			new DAttr('statusCode', 'sel', 'Status Code', 'select', true, 0, $options, NULL, 0, 'statuscode'),
			new DAttr('location', 'url', 'Destination URI', 'text', true, NULL, NULL, $this->_options['text_size'], 0, 'destinationuri'),
			$this->_attrs['note'],
			$this->_attrs['ctx_allow'],
			$this->_attrs['ctx_deny']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'context');
		$this->_tblDef[$id]->setRepeated('uri');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'redirect');
		$this->_tblDef[$id]->_helpKey = 'TABLEredirectContext';
	}

	protected function add_TP_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Template List', 1, 'TP1', $align, "form");

		$attrs = array(
			new DAttr('name', 'cust', 'Name'),
			new DAttr('listeners', 'cust', 'Mapped Listeners'),
			new DAttr('action', 'action', 'Action', NULL, false, 'TP1', 'Xd'));
		$this->_tblDef[$id]->setAttr($attrs, 'tpTop');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_TP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Template Location', 2);

		$attrs = array(
			new DAttr('name', 'name', 'Template Name', 'text', false, NULL, NULL, $this->_options['text_size'], 0, 'templateName'),
			$this->_attrs['tp_templateFile'],
			$this->_attrs['tp_listeners'],
			$this->_attrs['note']
			);
		$this->_tblDef[$id]->setAttr($attrs);
	}

	protected function add_TP1($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Template', 2);

		$attrs = array(
			new DAttr('name', 'name', 'Template Name', 'text', false, NULL, NULL, $this->_options['text_size']),
			$this->_attrs['tp_templateFile'],
			$this->_attrs['tp_listeners'],
			$this->_attrs['note']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'tpTop');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_TP_MEMBER_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Member Virtual Hosts', 1, 'TP_MEMBER', $align, "web");

		$attrs = array(
			$this->_attrs['tp_vhName'],
			$this->_attrs['tp_vhDomain'],
			new DAttr('action', 'action', 'Action', NULL, false, 'TP_MEMBER', 'vEdi'));

		$this->_tblDef[$id]->setAttr($attrs, 'member', 'member');
		$this->_tblDef[$id]->setRepeated('vhName');
	}

	protected function add_TP_MEMBER($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Member Virtual Host', 2);

		$attrs = array(
			$this->_attrs['tp_vhName'],
			$this->_attrs['tp_vhDomain'],
			$this->_attrs['defaultCluster'],
			new DAttr('vhAliases', 'domain', 'Aliases', 'text', true, NULL, NULL, $this->_options['text_size'], 1)
			);

		$this->_tblDef[$id]->setAttr($attrs, 'member', 'member');
		$this->_tblDef[$id]->setRepeated('vhName');
	}

	protected function add_TP_GENERAL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Template Settings');
		$configFile = $this->_attrs['tp_vrFile'];
		$configFile->rename('configFile', 'Configure File');

		$attrs = array(
			$configFile,
			$this->_attrs['vh_maxKeepAliveReq'],
			$this->_attrs['vh_smartKeepAlive'],
			$this->_attrs['vh_enableGzip'],
			$this->_attrs['enableIpGeo'],
			$this->_attrs['adminEmails']
			);
// to do: need check path contain VH variable.
		$this->_tblDef[$id]->setAttr($attrs, 'general');
	}

	protected function add_TP_GENERAL1($id)
	{
		// for file use only
		$this->_tblDef[$id] = new DTbl($id);

		$configFile = $this->_attrs['tp_vrFile'];
		$configFile->rename('configFile', 'Configure File');

		$attrs = array(
			$configFile,
			$this->_attrs['defaultCluster']
			);
// to do: need check path contain VH variable.
		$this->_tblDef[$id]->setAttr($attrs, 'general');
	}

	protected function add_TP_GENERAL2($id)
	{
		// for file use only
		$this->_tblDef[$id] = new DTbl($id);
		$attrs = array(
		$this->_attrs['vh_maxKeepAliveReq'],
		$this->_attrs['vh_smartKeepAlive'],
		$this->_attrs['vh_enableGzip'],
			$this->_attrs['enableIpGeo'],
			$this->_attrs['adminEmails']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general');
	}

	protected function add_TP_ACLOG($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('VH_ACLOG', $id);
		$this->_tblDef[$id]->_dattrs[1] =  $this->_attrs['tp_vrFile'];
	}

	protected function add_TP_SEC_CONN($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Per Client Throttling Control');

		$attrs = array(
			$this->_attrs['dynReqPerSec'],
			$this->_attrs['outBandwidth'],
			$this->_attrs['inBandwidth']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:conn', 'security');
	}

	protected function add_TP_REALM_TOP($id)
	{
		$align = array('center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Realms List', 1, 'TP_REALM_SEL', $align);

		$attrs = array(
			new DAttr('name', 'cust', 'Realm Name'),
			$this->_attrs['realm_type'],
			new DAttr('action', 'action', 'Action', NULL, false, $this->_options['tp_realmTbl'], 'vEd')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:realm');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_TP_REALM_SEL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'New Realm');
		$this->_tblDef[$id]->_subTbls = $this->_options['tp_realmTbl'];

		$attrs[] = $this->_attrs['realm_type'];
 		$this->_tblDef[$id]->setAttr($attrs, 'sec:realm', 'security:realmList:realm');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_TP_REALM_FILE($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Password File Realm Definition', 2);

		$attrs = array(
			$this->_attrs['realm_name'],
			new DAttr('userDB:location', 'cust', 'User DB Location', 'text', false, 3, 'rc', $this->_options['text_size']),
			$this->_attrs['realm_udb_maxCacheSize'],
			$this->_attrs['realm_udb_cacheTimeout'],
			new DAttr('groupDB:location', 'cust', 'Group DB Location', 'text', true, 3, 'rc', $this->_options['text_size']),
			$this->_attrs['realm_gdb_maxCacheSize'],
			$this->_attrs['realm_gdb_cacheTimeout']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'sec:realm');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_defaultExtract = array('type'=>'file');
	}


	protected function add_ADMIN_PHP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'General');

		$attrs = array(
			new DAttr('enableCoreDump', 'bool', 'Enable Core Dump', 'radio', false ),
			new DAttr('sessionTimeout', 'uint', 'Session Timeout (secs)', 'text', true, 60)
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general');
	}

	protected function add_ADMIN_USR_TOP($id)
	{
		$align = array('left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Web Admin Users', 1, 'ADMIN_USR_NEW', $align, "administrator");

		$attrs = array(
			new DAttr('name', 'cust', 'Admin User Name'),
			new DAttr('action', 'action', 'Action', NULL, false, 'ADMIN_USR', 'Ed')
			);
		$this->_tblDef[$id]->setAttr($attrs, 'ADM');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_ADMIN_USR($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Web Admin User', 2);

		$attrs = array(
			new DAttr('name', 'name', 'Web Admin User Name', 'text', false, NULL, NULL, $this->_options['text_size']),
			new DAttr('oldpass', 'cust', 'Old Password', 'password', true, NULL, NULL, $this->_options['text_size']),
			new DAttr('pass', 'cust', 'New Password', 'password', true, NULL, NULL, $this->_options['text_size']),
			new DAttr('pass1', 'cust', 'Retype Password', 'password', true, NULL, NULL, $this->_options['text_size'])
			);
		$this->_tblDef[$id]->setAttr($attrs, 'ADM');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_defaultExtract = array('passwd'=>'$$');
	}
	
	protected function add_ADMIN_USR_NEW($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'New Web Admin User', 2);

		$attrs = array(
			new DAttr('name', 'name', 'Web Admin User Name', 'text', false, NULL, NULL, $this->_options['text_size']),
			new DAttr('pass', 'cust', 'New Password', 'password', true, NULL, NULL, $this->_options['text_size']),
			new DAttr('pass1', 'cust', 'Retype Password', 'password', true, NULL, NULL, $this->_options['text_size'])
			);
		$this->_tblDef[$id]->setAttr($attrs, 'ADM');
		$this->_tblDef[$id]->setRepeated('name');
		$this->_tblDef[$id]->_defaultExtract = array('passwd'=>'$$');
	}
	
	protected function add_L_TOP($id)
	{
		$align = array('center', 'center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Listeners', 1, 'L_GENERAL1', $align, "link");

		$attrs = array(
			new DAttr('name', 'cust', 'Listener Name'),
			new DAttr('ip', 'cust', 'IP Address'),
			new DAttr('port', 'cust', 'Port'),
			new DAttr('secure', 'bool', 'Secure'),
			new DAttr('action', 'action', 'Action', NULL, false, 'L_GENERAL1', 'Xd'),

			);
		$this->_tblDef[$id]->setAttr($attrs, 'listeners');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_L_GENERAL1($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('L_GENERAL', $id);
		$this->_tblDef[$id]->setDataLoc('listeners');
		$this->_tblDef[$id]->setRepeated('name');
	}

	protected function add_L_GENERAL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Address Settings');

		$name =	new DAttr('name', 'name', 'Listener Name', 'text', false);
		$name->_helpKey = 'listenerName';
		$addr = new DAttr('address', 'cust', 'Address', NULL, false);
		$addr->_FDE = 'YNN';
		$ip = new DAttr('ip', 'sel', 'IP Address', 'select', false, 0, $this->_options['ip']);
		$ip->_FDE = 'NYY';
		$ip->_helpKey = 'listenerIP';
		$port = new DAttr('port', 'uint', 'Port', 'text', false, 0, 65535);
		$port->_FDE = 'NYY';
		$port->_helpKey = 'listenerPort';

		$attrs = array(
			$name,
			$addr, $ip,	$port, 
			new DAttr('secure', 'bool', 'Secure', 'radio', false, NULL, NULL, NULL, 0, 'listenerSecure'),
			$this->_attrs['note'],
			);
		$this->_tblDef[$id]->setAttr($attrs);
		$this->_tblDef[$id]->_defaultExtract = array('address'=>'$$');
	}
	
	protected function add_L_VHMAP_TOP($id)
	{
		$align = array('left', 'left', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Mappings', 1, 'L_VHMAP', $align, "web_link");

		$attrs = array(
			$this->_attrs['l_vhost'],
			$this->_attrs['l_domain'],
			new DAttr('action', 'action', 'Action', NULL, false, 'L_VHMAP', 'Ed'));

		$this->_tblDef[$id]->setAttr($attrs, 'vhMap', 'vhostMapList:vhostMap');
		$this->_tblDef[$id]->setRepeated('vhost');
		$this->_tblDef[$id]->_helpKey = 'TABLEvirtualHostMapping';
	}

	protected function add_L_VHMAP($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Mappings', 2);

		$attrs = array(
			$this->_attrs['l_vhost'],
			$this->_attrs['l_domain']);
		$this->_tblDef[$id]->setAttr($attrs, 'vhMap', 'vhostMapList:vhostMap');
		$this->_tblDef[$id]->setRepeated('vhost');
		$this->_tblDef[$id]->_helpKey = 'TABLEvirtualHostMapping';
	}

	protected function add_L_CERT($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'SSL Private Key & Certificate');

		$attrs = array(
			new DAttr('keyFile', 'file', 'Private Key File', 'text', false, 2, '', $this->_options['text_size']),
			new DAttr('certFile', 'file', 'Certificate File', 'text', false, 2, '', $this->_options['text_size']),
			new DAttr('certChain', 'bool', 'Chained Certificate', 'radio'),
			new DAttr('CACertPath', 'path', 'CA Certificate Path', 'text', true, 2, '', $this->_options['text_size']),
			new DAttr('CACertFile', 'file', 'CA Certificate File', 'text', true, 2, '', $this->_options['text_size'])

			);
		$this->_tblDef[$id]->setAttr($attrs);
	}

	protected function add_L_SSL($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'SSL Protocol');
		
		$attrs = array(
			new DAttr('sslProtocol', 'checkboxOr', 'SSL Protocol Version', 'checkboxgroup', FALSE, NULL, $this->_options['sslversion']),
			new DAttr_SSLCipher('ciphers', 'sslcipher', 'Encryption Level', 'cust', FALSE),
		);

		$this->_tblDef[$id]->setAttr($attrs);
	}
	
	protected function add_ADMIN_L_TOP($id)
	{
		$align = array('center', 'center', 'center', 'center', 'center');
		$this->_tblDef[$id] = new DTbl($id, 'Listeners', 1, 'ADMIN_L_GENERAL1', $align, "link");

		$attrs = array(
			new DAttr('name', 'cust', 'Listener Name'),
			new DAttr('ip', 'cust', 'IP Address'),
			new DAttr('port', 'cust', 'Port'),
			new DAttr('secure', 'bool', 'Secure'),
			new DAttr('action', 'action', 'Action', NULL, false, 'ADMIN_L_GENERAL1', 'Xd'),

			);
		$this->_tblDef[$id]->setAttr($attrs, 'listeners');
		$this->_tblDef[$id]->setRepeated('name');
	}
		
	protected function add_ADMIN_L_GENERAL($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('L_GENERAL', $id, 'Admin Listener Address Settings');
	}

	protected function add_ADMIN_L_GENERAL1($id)
	{
		$this->_tblDef[$id] = $this->DupTblDef('ADMIN_L_GENERAL', $id);
		$this->_tblDef[$id]->setDataLoc('listeners');
		$this->_tblDef[$id]->setRepeated('name');
	}
	
	protected function add_VH_LOG($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Virtual Host Log');
		$log_filename = $this->_attrs['fileName3'];
		$log_filename->_helpKey = 'log_fileName';

		$attrs = array(
			new DAttr('useServer', 'bool', 'Use Server\'s Log', 'radio', false, NULL, NULL, NULL, 0, 'logUseServer'),
			$log_filename,
			new DAttr('logLevel', 'sel', 'Log Level', 'select', true, 0, $this->_options['logLevel'], NULL, 0, 'vhlog_logLevel'),
			$this->_attrs['rollingSize']
			);
		$this->_tblDef[$id]->setAttr($attrs, 'general:log', 'logging:log');
	}
		
	protected function add_SERVICE_SUSPENDVH($id)
	{
		$this->_tblDef[$id] = new DTbl($id, 'Suspend Vhosts');
		
		$attr0 = new DAttr('suspendedVhosts', 'vhname', NULL);
		$attr0->multiInd = 1;
		$attr0->_FDE = 'YNN';
		$attrs = array( $attr0);
		$this->_tblDef[$id]->setAttr($attrs, 'service');
	}	
	
}

