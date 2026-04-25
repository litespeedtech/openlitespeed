<?php

namespace LSWebAdmin\Product\Ows;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Base\DTblDefBase;
use LSWebAdmin\UI\DTbl;

class DTblDef extends DTblDefBase
{
    private static $instance = null;

	public static function getInstance()
    {
        if (self::$instance == null) {
            self::$instance = new self();
        }
        return self::$instance;
    }

    private function __construct()
	{
		$this->loadCommonOptions();
		$this->loadCommonAttrs();
		$this->loadSpecials();
	}

	protected function loadSpecials()
	{
		// define special block contains raw data
		parent::loadSpecials();

		$this->addSpecial('phpIniOverride', [], 'data');
		$this->addSpecial('rewrite', ['enable', 'autoLoadHtaccess', 'logLevel', 'map', 'inherit', 'base'], 'rules');
		$this->addSpecial('virtualHostConfig:rewrite', ['enable', 'autoLoadHtaccess', 'logLevel', 'map', 'inherit', 'base'], 'rules'); // for template

		$tags = array_merge(['ls_enabled', 'note', 'internal', 'urlFilter'], $this->getModuleTags());
		$this->addSpecial('module', $tags, 'param');

		$this->addSpecial('urlFilter', ['ls_enabled'], 'param');
	}

	protected function loadCommonOptions()
	{
		parent::loadCommonOptions();
		$this->_options['scriptHandler'] = $this->getSharedScriptHandlerOptions([
			'module' => DMsg::ALbl('l_modulehandler')
		]);
		$this->_options['captcha'] = [
			'0' => DMsg::ALbl('o_notset'),
			'1' => DMsg::ALbl('o_checkbox'),
			'2' => DMsg::ALbl('o_invisible'),
			'3' => DMsg::ALbl('o_hcaptcha')
            ];		
		$this->_options['ctxType'] = $this->getSharedContextTypeOptions([
			'module' => DMsg::ALbl('l_modulehandler')
		]);
		$this->_options['ctxTbl'] = $this->getSharedContextTableOptions('VT_CTX', [
			'module' => 'VT_CTXMD'
		]);
		$this->_options['realmType'] = $this->getSharedRealmTypeOptions();
	}

	protected function loadCommonAttrs()
	{
		parent::loadCommonAttrs();
		$param = self::NewTextAreaAttr('param', DMsg::ALbl('l_moduleparams'), 'cust', true, 4, 'modParams', 1, 1);
		$param->SetFlag(DAttr::BM_RAWDATA);
		$this->_attrs['mod_params'] = $param;
		$this->_attrs['mod_enabled'] = self::NewBoolAttr('ls_enabled', DMsg::ALbl('l_enablehooks'), true, 'moduleEnabled');
	}

	protected function add_S_PROCESS($id) //keep
	{
		$attrs = [
			self::NewTextAttr('serverName', DMsg::ALbl('l_servername'), 'name'),
			self::NewIntAttr('httpdWorkers', DMsg::ALbl('l_numworkers'), true, 1, 16),
			self::NewCustFlagAttr('runningAs', DMsg::ALbl('l_runningas'), (DAttr::BM_NOFILE | DAttr::BM_NOEDIT)),
			self::NewCustFlagAttr('user', null, (DAttr::BM_HIDE | DAttr::BM_NOEDIT), false),
			self::NewCustFlagAttr('group', null, (DAttr::BM_HIDE | DAttr::BM_NOEDIT), false),
			$this->_attrs['priority']->dup(null, null, 'serverPriority'),
			DTblDefBase::NewIntAttr('cpuAffinity', DMsg::ALbl('l_cpuaffinity'), true, 0, 64),
			DTblDefBase::NewSelAttr('enableLVE', DMsg::ALbl('l_enablelve'),
					[0 => DMsg::ALbl('o_disabled'), 1 => "LVE", 2 => "CageFS", 3 => DMsg::ALbl('o_cagefswithoutsuexec')]),
			self::NewIntAttr('inMemBufSize', DMsg::ALbl('l_inmembufsize'), false, 0),
			self::NewTextAttr('swappingDir', DMsg::ALbl('l_swappingdir'), 'cust', false),
			self::NewBoolAttr('autoFix503', DMsg::ALbl('l_autofix503')),
			self::NewBoolAttr('enableh2c', DMsg::ALbl('l_enableh2c')),
			self::NewIntAttr('gracefulRestartTimeout', DMsg::ALbl('l_gracefulrestarttimeout'), true, -1, 2592000),
			self::NewTextAttr('statDir', DMsg::ALbl('l_statDir'), 'cust'),
			self::NewBoolAttr('jsonReports', DMsg::ALbl('l_jsonreports')),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_serverprocess'), $attrs);
	}

	protected function add_S_GENERAL($id) // keep
	{
		$attr_mime = self::NewPathAttr('mime', DMsg::ALbl('l_mimesettings'), 'file', 2, 'rw', false);
		$attr_mime->_href = '&t=S_MIME_TOP';

		$attrs = [
			$attr_mime,
			self::NewBoolAttr('disableInitLogRotation', DMsg::ALbl('l_disableinitlogrotation')),
			self::NewSelAttr('showVersionNumber', DMsg::ALbl('l_serversig'),
					[
						'0' => DMsg::ALbl('o_hidever'),
						'1' => DMsg::ALbl('o_showver'),
						'2' => DMsg::ALbl('o_hidefullheader'),
					], false),
			$this->_attrs['enableIpGeo'],
			self::NewSelAttr('useIpInProxyHeader', DMsg::ALbl('l_useipinproxyheader'),
					[
						'0' => DMsg::ALbl('o_no'),
						'1' => DMsg::ALbl('o_yes'),
						'2' => DMsg::ALbl('o_trustediponly'),
						'3' => DMsg::ALbl('o_keepheaderfortrusted'),
						'4' => DMsg::ALbl('o_use_last_ip_for elb'),
			]),
			$this->_attrs['adminEmails'],
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_generalsettings'), $attrs);
	}

	protected function add_S_AUTOLOADHTA($id)
	{
		$attrs = [
			self::NewBoolAttr('autoLoadHtaccess', DMsg::ALbl('l_autoLoadRewriteHtaccess')),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_rewritecontrol'), $attrs);
	}

	protected function add_VT_REWRITE_CTRL($id)
	{
		$attrs = [
			self::NewBoolAttr('enable', DMsg::ALbl('l_enablerewrite'), true, 'enableRewrite'),
			self::NewBoolAttr('autoLoadHtaccess', DMsg::ALbl('l_autoLoadRewriteHtaccess')),
			self::NewIntAttr('logLevel', DMsg::ALbl('l_loglevel'), true, 0, 9, 'rewriteLogLevel')
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_rewritecontrol'), $attrs);
	}

	protected function add_VT_REWRITE_MAP_TOP($id)
	{
		$align = ['left', 'left', 'center'];
		$name = self::NewViewAttr('name', DMsg::ALbl('l_name'));
		$location = self::NewViewAttr('location', DMsg::ALbl('l_location'));
		$action = self::NewActionAttr('VT_REWRITE_MAP', 'Ed');
		$attrs = [
			$name,
			$location,
			$action
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_rewritemap'), $attrs, 'name', 'VT_REWRITE_MAP', $align, null, 'arrow-right-left', true);
	}

	protected function add_VT_REWRITE_MAP($id)
	{
		$parseFormat = '/^((txt|rnd):\/*)|(int:(toupper|tolower|escape|unescape))$/';
		$name = self::NewTextAttr('name', DMsg::ALbl('l_name'), 'name', false, 'rewriteMapName');
		$location = self::NewParseTextAttr('location', DMsg::ALbl('l_location'), $parseFormat, DMsg::ALbl('parse_rewritemaplocation'), true, 'rewriteMapLocation');
		$note = $this->_attrs['note']->dup(null, null, null);

		$attrs = [
			$name,
			$location,
			$note,
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_rewritemap'), $attrs, 'name');
	}

	protected function add_VT_REWRITE_RULE($id)
	{
		$rules = self::NewTextAreaAttr('rules', null, 'cust', true, 5, 'rewriteRules', 1, 1); // no label, will use table header
		$attrs = [
			$rules
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_rewriterules'), $attrs, 'rewriteRules', 1);
	}

	protected function add_S_FILEUPLOAD($id)
	{
		$attrs = [
			self::NewPathAttr('uploadTmpDir', DMsg::ALbl('l_uploadtmpdir'), 'path', 2),
			self::NewParseTextAttr('uploadTmpFilePermission', DMsg::ALbl('l_uploadtmpfilepermission'), $this->_options['parseFormat']['filePermission3'], DMsg::ALbl('parse_uploadtmpfilepermission')),
			self::NewBoolAttr('uploadPassByPath', DMsg::ALbl('l_uploadpassbypath'))
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_uploadfile'), $attrs, 'fileUpload');
	}

	protected function add_VT_FILEUPLOAD($id)
	{
		$attrs = [
			self::NewPathAttr('uploadTmpDir', DMsg::ALbl('l_uploadtmpdir'), 'path', 3),
			self::NewParseTextAttr('uploadTmpFilePermission', DMsg::ALbl('l_uploadtmpfilepermission'), $this->_options['parseFormat']['filePermission3'], DMsg::ALbl('parse_uploadtmpfilepermission')),
			self::NewBoolAttr('uploadPassByPath', DMsg::ALbl('l_uploadpassbypath'))
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_uploadfile'), $attrs, 'fileUpload');
	}

	protected function add_VT_PHPINIOVERRIDE($id)
	{
		$override = self::NewTextAreaAttr('data', null, 'cust', true, 6, 'phpIniOverride', 1, 1);
		$override->SetFlag(DAttr::BM_RAWDATA);
		$attrs = [
			$override,
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_phpinioverride'), $attrs, 'phpIniOverride', 1);
	}

	protected function add_S_TUNING_OS($id) //keep
	{
		$attrs = [
			self::NewTextAttr('shmDefaultDir', DMsg::ALbl('l_shmDefaultDir'), 'cust'),
			self::NewTextAreaAttr('proxyProtocol', DMsg::ALbl('l_proxyprotocol'), 'subnet', true, 5, null, 1, 0, 1),
		];

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_tuningos'), $attrs);
	}

	protected function add_S_TUNING_STATIC($id)
	{
		$attrs = [
			self::NewIntAttr('maxCachedFileSize', DMsg::ALbl('l_maxcachedfilesize'), false, 0, 1048576),
			self::NewIntAttr('totalInMemCacheSize', DMsg::ALbl('l_totalinmemcachesize'), false, 0),
			self::NewIntAttr('maxMMapFileSize', DMsg::ALbl('l_maxmmapfilesize'), false, 0),
			self::NewIntAttr('totalMMapCacheSize', DMsg::ALbl('l_totalmmapcachesize'), false, 0),
			self::NewBoolAttr('useSendfile', DMsg::ALbl('l_usesendfile')),
			self::NewSelAttr('useAIO', DMsg::ALbl('l_useaio'),
					[
						'0' => DMsg::ALbl('o_no'),
						'1' => 'POSIX AIO',
						'2' => 'Linux AIO',
						'3' => 'io_uring',
					], false),
			self::NewSelAttr('AIOBlockSize', DMsg::ALbl('l_aioblocksize'),
					[
						'1' => '128K', '2' => '256K', '3' => '512K',
						'4' => '1M', '5' => '2M', '6' => '4M', '7' => '8M',
					], false),
			self::NewCheckBoxAttr('fileETag', DMsg::ALbl('l_fileetag'),
					['4' => 'iNode', '8' => DMsg::ALbl('o_modifiedtime'), '16' => DMsg::ALbl('o_size'), '0' => DMsg::ALbl('o_none')], true, null, 28),
		];

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_tuningstatic'), $attrs);
	}

	protected function add_S_TUNING_SSL($id)
	{
		$attrs = [
			self::NewTextAttr('sslDefaultCiphers', DMsg::ALbl('l_sslDefaultCiphers'), 'cust'),
			self::NewBoolAttr('sslStrongDhKey', DMsg::ALbl('l_sslStrongDhKey')),
			self::NewBoolAttr('sslEnableMultiCerts', DMsg::ALbl('l_sslEnableMultiCerts')),
			$this->_attrs['sslSessionCache'],
			self::NewIntAttr('sslSessionCacheSize', DMsg::ALbl('l_sslSessionCacheSize'), true, 512),
			self::NewIntAttr('sslSessionCacheTimeout', DMsg::ALbl('l_sslSessionCacheTimeout'), true, 10, 1000000),
			$this->_attrs['sslSessionTickets'],
			self::NewIntAttr('sslSessionTicketLifetime', DMsg::ALbl('l_sslSessionTicketLifetime'), true, 10, 1000000),
			self::NewTextAttr('sslSessionTicketKeyFile', DMsg::ALbl('l_sslSessionTicketKeyFile'), 'cust'),
			self::NewParseTextAttr('sslOcspProxy', DMsg::ALbl('l_ocspproxy'), '/^((http|https):\/\/)?([A-z0-9._\-]+|\[[[:xdigit:]:]+\])(:\d+)$/', DMsg::ALbl('parse_ocspproxy')),
			self::NewBoolAttr('sslStrictSni', DMsg::ALbl('l_sslStrictSni')),
            self::NewSelAttr('acme', DMsg::ALbl('l_acme'), $this->_options['disable_off_on']),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_tuningsslsettings'), $attrs, 'sslGlobal');
	}

	protected function add_S_MOD_TOP($id)
	{
		$align = ['center', 'center', 'center', 'center'];

		$attrs = [self::NewViewAttr('name', DMsg::ALbl('l_module')),
			self::NewBoolAttr('internal', DMsg::ALbl('l_internal'), true, 'internalmodule'),
			$this->_attrs['mod_params'],
			$this->_attrs['mod_enabled'],
			self::NewActionAttr('S_MOD', 'vEd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_servermodulesdef'), $attrs, 'name', 'S_MOD', $align, null, 'puzzle', TRUE);
	}

	protected function getModuleTags()
	{
		$tags = ['L4_BEGINSESSION', 'L4_ENDSESSION', 'L4_RECVING', 'L4_SENDING',
			'HTTP_BEGIN', 'RECV_REQ_HEADER', 'URI_MAP', 'HTTP_AUTH',
			'RECV_REQ_BODY', 'RCVD_REQ_BODY', 'RECV_RESP_HEADER', 'RECV_RESP_BODY', 'RCVD_RESP_BODY',
			'HANDLER_RESTART', 'SEND_RESP_HEADER', 'SEND_RESP_BODY', 'HTTP_END',
			'MAIN_INITED', 'MAIN_PREFORK', 'MAIN_POSTFORK', 'WORKER_POSTFORK', 'WORKER_ATEXIT', 'MAIN_ATEXIT'];
		return $tags;
	}

	protected function add_S_MOD($id)
	{
		$attrs = [self::NewTextAttr('name', DMsg::ALbl('l_module'), 'modulename', false, 'modulename'),
			$this->_attrs['note'],
			self::NewBoolAttr('internal', DMsg::ALbl('l_internal'), true, 'internalmodule'),
			$this->_attrs['mod_params'],
			$this->_attrs['mod_enabled']
		];

		$tags = $this->getModuleTags();

		$hook = DMsg::ALbl('l_hook');
		$priority = DMsg::ALbl('l_priority');
		foreach ($tags as $tag) {
			$attrs[] = self::NewIntAttr($tag, "{$hook}::{$tag} {$priority}", true, -6000, 6000);
		}
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_servermoduledef'), $attrs, 'name', 'servModules');
	}

	protected function add_VT_MOD_TOP($id)
	{
		$align = ['center', 'center', 'center', 'center'];

		$attrs = [self::NewViewAttr('name', DMsg::ALbl('l_module')),
			$this->_attrs['mod_params'],
			$this->_attrs['mod_enabled']->dup(null, null, 'moduleEnabled_vh'),
			self::NewActionAttr('VT_MOD', 'vEd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_vhmodules'), $attrs, 'name', 'VT_MOD', $align, 'vhModules', 'puzzle', TRUE);
	}

	protected function add_VT_MOD($id)
	{
		$attrs = [self::NewSelAttr('name', DMsg::ALbl('l_module'), 'module', false, 'moduleNameSel'),
			$this->_attrs['note'],
			$this->_attrs['mod_params'],
			$this->_attrs['mod_enabled']->dup(null, null, 'moduleEnabled_vh')
		];

		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_vhmodule'), $attrs, 'name', 'vhModules');
		$this->_tblDef[$id]->Set(DTbl::FLD_LINKEDTBL, ['VT_MOD_FILTERTOP']);
	}

	protected function add_VT_MOD_FILTERTOP($id)
	{
		$align = ['center', 'center', 'center', 'center'];

		$attrs = [self::NewViewAttr('uri', DMsg::ALbl('l_uri')),
			$this->_attrs['mod_params'],
			self::NewActionAttr('VT_MOD_FILTER', 'vEd')
		];

		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_urlfilter'), $attrs, 'uri', 'VT_MOD_FILTER', $align, 'vhModuleUrlFilters', 'filter', FALSE);
		$this->_tblDef[$id]->Set(Dtbl::FLD_SHOWPARENTREF, true);
	}

	protected function add_VT_MOD_FILTER($id)
	{
		$attrs = [$this->_attrs['ctx_uri'],
			$this->_attrs['mod_params'],
		];

		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_urlfilter'), $attrs, 'uri', 'vhModuleUrlFilters');
		$this->_tblDef[$id]->Set(Dtbl::FLD_SHOWPARENTREF, true);
	}

	protected function add_L_MOD_TOP($id)
	{
		$align = ['center', 'center', 'center', 'center'];

		$attrs = [self::NewViewAttr('name', DMsg::ALbl('l_module')),
			$this->_attrs['mod_params'],
			$this->_attrs['mod_enabled']->dup(null, null, 'moduleEnabled_lst'),
			self::NewActionAttr('L_MOD', 'vEd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_listenermodules'), $attrs, 'name', 'L_MOD', $align, 'listenerModules', 'puzzle', TRUE);
	}

	protected function add_L_MOD($id)
	{
		$attrs = [self::NewSelAttr('name', DMsg::ALbl('l_module'), 'module', false, 'moduleNameSel'),
			$this->_attrs['note'],
			$this->_attrs['mod_params'],
			$this->_attrs['mod_enabled']->dup(null, null, 'moduleEnabled_lst')
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_listenermodule'), $attrs, 'name', 'listenerModules');
	}

	protected function add_V_TOPD($id)
	{
		$attrs = [
			self::NewTextAttr('name', DMsg::ALbl('l_vhname'), 'vhname', false, 'vhName'),
			self::NewTextAttr('vhRoot', DMsg::ALbl('l_vhroot'), 'cust', false), //no validation, maybe suexec owner
			self::NewPathAttr('configFile', DMsg::ALbl('l_configfile'), 'filevh', 3, 'rwc', false),
			$this->_attrs['note'],
			$this->_attrs['vh_allowSymbolLink'],
			$this->_attrs['vh_enableScript'],
			$this->_attrs['vh_restrained'],
			$this->_attrs['vh_maxKeepAliveReq'],
			$this->_attrs['vh_setUIDMode'],
			$this->_attrs['vh_suexec_user'],
			$this->_attrs['vh_suexec_group'],
			$this->_attrs['staticReqPerSec'],
			$this->_attrs['dynReqPerSec'],
			$this->_attrs['outBandwidth'],
			$this->_attrs['inBandwidth'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_vhost'), $attrs, 'name');
	}

	protected function add_V_BASE_SEC($id)
	{
		$attrs = [
			$this->_attrs['vh_allowSymbolLink'],
			$this->_attrs['vh_enableScript'],
			$this->_attrs['vh_restrained'],
			$this->_attrs['vh_setUIDMode'],
			$this->_attrs['vh_suexec_user'],
			$this->_attrs['vh_suexec_group'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::UIStr('tab_sec'), $attrs, 'name');
	}

	protected function add_V_GENERAL($id)
	{
		$attrs = [
			self::NewTextAttr('docRoot', DMsg::ALbl('l_docroot'), 'cust', false), //no validation, maybe suexec owner
			$this->_attrs['tp_vhDomain'], // this setting is a new way, will merge with listener map settings for backward compatible
			$this->_attrs['tp_vhAliases'],
			$this->_attrs['adminEmails']->dup(null, null, 'vhadminEmails'),
			$this->_attrs['vh_enableGzip'],
			$this->_attrs['vh_enableBr'],
			$this->_attrs['enableIpGeo'],
			$this->_attrs['vh_cgroups'],
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_coresettings'), $attrs, 'vhGeneral');
	}

	protected function add_T_GENERAL2($id)
	{
		$attrs = [
			$this->_attrs['tp_vrFile']->dup('docRoot', DMsg::ALbl('l_docroot'), 'templateVHDocRoot'),
			$this->_attrs['adminEmails']->dup(null, null, 'vhadminEmails'),
			$this->_attrs['vh_enableGzip'],
			$this->_attrs['vh_enableBr'],
			$this->_attrs['enableIpGeo'],
			$this->_attrs['vh_cgroups'],
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_vhostconfigdefaults'), $attrs, 'templateVHConfigDefaults');
	}

	protected function add_V_REALM_TOP($id)
	{
		$align = ['center', 'center', 'center'];
		$realm_attr = $this->get_realm_attrs();

		$attrs = [
			$realm_attr['realm_name'],
			self::NewViewAttr('userDB:location', DMsg::ALbl('l_userdblocation'), 'userDBLocation'),
			self::NewActionAttr('V_REALM_FILE', 'vEd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_realmlist'), $attrs, 'name', 'V_REALM_FILE', $align, 'realms', 'shield', TRUE);
	}

	protected function add_T_REALM_TOP($id)
	{
		$align = ['center', 'center', 'center'];
		$realm_attr = $this->get_realm_attrs();

		$attrs = [
			$realm_attr['realm_name'],
			self::NewViewAttr('userDB:location', DMsg::ALbl('l_userdblocation'), 'userDBLocation'),
			self::NewActionAttr('T_REALM_FILE', 'vEd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_realmlist'), $attrs, 'name', 'T_REALM_FILE', $align, 'realms', 'shield', TRUE);
	}

	protected function add_VT_CTX_TOP($id)
	{
		$align = ['left', 'left', 'center', 'center'];

		$attrs = [
			$this->_attrs['ctx_type'],
			self::NewViewAttr('uri', DMsg::ALbl('l_uri')),
			self::NewBoolAttr('allowBrowse', DMsg::ALbl('l_allowbrowse'), false),
			self::NewCustFlagAttr('order', DMsg::ALbl('l_order'), (DAttr::BM_NOFILE | DAttr::BM_NOEDIT), true, 'ctxseq'),
			self::NewActionAttr($this->_options['ctxTbl'], 'vEd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_contextlist'), $attrs, 'uri', 'VT_CTX_SEL', $align, null,
						$this->getContextTypeIcons(), TRUE);
	}

	protected function add_VT_CTXG($id)
	{
		$override = self::NewTextAreaAttr('phpIniOverride:data', DMsg::ALbl('l_phpinioverride'), 'cust', true, 6, 'phpIniOverride', 1, 1);
		$override->SetFlag(DAttr::BM_RAWDATA);

		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					$this->_attrs['ctx_location'],
					self::NewBoolAttr('allowBrowse', DMsg::ALbl('l_allowbrowse'), false),
					$this->_attrs['note']
				],
				$this->get_expires_attrs(),
				[
					$this->_attrs['extraHeaders'],
					self::NewParseTextAreaAttr('addMIMEType', DMsg::ALbl('l_mimetype'),
							'/[A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+(\s+[A-z0-9_\-\+]+)+/', DMsg::ALbl('parse_mimetype'),
							true, 2, null, 0, 0, 1),
					self::NewParseTextAttr('forceType', DMsg::ALbl('l_forcemimetype'),
							'/^([A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+)|(NONE)$/i', DMsg::ALbl('parse_forcemimetype')),
					self::NewParseTextAttr('defaultType', DMsg::ALbl('l_defaultmimetype'),
							'/^[A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+$/', DMsg::ALbl('parse_defaultmimetype')),
					$this->_attrs['indexFiles'],
					$this->_attrs['autoIndex']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('rewrite'),
				$this->get_ctx_attrs('charset'),
				[
					$override,
				]
		);
		$defaultExtract = ['type' => 'null'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxg'), $attrs, 'uri', 'generalContext', $defaultExtract);
	}

	protected function add_VT_CTXJ($id)
	{
		$attrs = array_merge(
				[
					self::NewTextAttr('uri', DMsg::ALbl('l_uri'), 'uri', false),
					$this->_attrs['ctx_order'],
					$this->_attrs['ctx_location']->dup(null, null, 'javaWebApp_location'),
					$this->_attrs['ctx_shandler'],
					$this->_attrs['note']
				],
				$this->get_expires_attrs(),
				[
					$this->_attrs['extraHeaders'],
					$this->_attrs['indexFiles'],
					$this->_attrs['autoIndex']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'webapp'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxj'), $attrs, 'uri', 'javaWebAppContext', $defaultExtract);
	}

	protected function add_VT_CTXAS($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewTextAttr('location', DMsg::ALbl('l_location'), 'cust', false, 'as_location'),
					self::NewPathAttr('binPath', DMsg::ALbl('l_binpath'), 'file', 1, 'x'),
					self::NewSelAttr('appType', DMsg::ALbl('l_apptype'),
							['' => '', 'rails' => 'Rails', 'wsgi' => 'WSGI', 'node' => 'Node']),
					self::NewTextAttr('startupFile', DMsg::ALbl('l_startupfile'), 'cust', true, 'as_startupfile'),
					$this->_attrs['note'],
					$this->_attrs['appserverEnv'],
					self::NewIntAttr('maxConns', DMsg::ALbl('l_maxconns'), true, 1, 2000),
					$this->_attrs['ext_env']
				],
				$this->get_expires_attrs(),
				[
					$this->_attrs['extraHeaders'],
					$this->_attrs['indexFiles'],
					$this->_attrs['autoIndex']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('rewrite'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'appserver'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxas'), $attrs, 'uri', 'appserverContext', $defaultExtract);
	}

	protected function add_VT_CTXS($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					$this->_attrs['ctx_shandler'],
					$this->_attrs['note'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'servlet'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxs'), $attrs, 'uri', 'servletContext', $defaultExtract);
	}

	protected function add_VT_CTXF($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewSelAttr('handler', DMsg::ALbl('l_fcgiapp'), 'extprocessor:fcgi', false, 'fcgiapp'),
					$this->_attrs['note'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'fcgi'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxf'), $attrs, 'uri', 'fcgiContext', $defaultExtract);
	}

	protected function add_VT_CTXL($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewSelAttr('handler', DMsg::ALbl('l_lsapiapp'), 'extprocessor:lsapi', false, 'lsapiapp'),
					$this->_attrs['note'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'lsapi'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxl'), $attrs, 'uri', 'lsapiContext', $defaultExtract);
	}

	protected function add_VT_CTXSC($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewSelAttr('handler', DMsg::ALbl('l_extscgi'), 'extprocessor:scgi', false, 'scgiapp'),
					$this->_attrs['note'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'scgi'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxsc'), $attrs, 'uri', 'scgiContext', $defaultExtract);
	}

	protected function add_VT_CTXUW($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewSelAttr('handler', DMsg::ALbl('l_extuwsgi'), 'extprocessor:uwsgi', false, 'uwsgiapp'),
					$this->_attrs['note'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'uwsgi'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxuw'), $attrs, 'uri', 'uwsgiContext', $defaultExtract);
	}

	protected function add_VT_CTXMD($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewSelAttr('handler', DMsg::ALbl('l_modulehandler'), 'module', false, 'moduleNameSel'),
					$this->_attrs['note'],
					$this->_attrs['mod_params'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'module'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxmd'), $attrs, 'uri', 'lmodContext', $defaultExtract);
	}

	protected function add_VT_CTXB($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewSelAttr('handler', DMsg::ALbl('l_loadbalancer'), 'extprocessor:loadbalancer', false, 'lbapp'),
					$this->_attrs['note'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'loadbalancer'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxb'), $attrs, 'uri', 'lbContext', $defaultExtract);
	}

	protected function add_VT_CTXP($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewSelAttr('handler', DMsg::ALbl('l_webserver'), 'extprocessor:proxy', false, 'proxyWebServer'),
					$this->_attrs['note'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'proxy'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxp'), $attrs, 'uri', 'proxyContext', $defaultExtract);
	}

	protected function add_VT_CTXC($id)
	{
		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					$this->_attrs['ctx_location']->dup(null, DMsg::ALbl('l_path'), 'cgi_path'),
					$this->_attrs['note'],
					$this->_attrs['extraHeaders'],
					self::NewBoolAttr('allowSetUID', DMsg::ALbl('l_allowsetuid'))
				],
				$this->get_ctx_attrs('auth'),
				$this->get_ctx_attrs('rewrite'),
				$this->get_ctx_attrs('charset')
		);
		$defaultExtract = ['type' => 'cgi'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxc'), $attrs, 'uri', 'cgiContext', $defaultExtract);
	}

	protected function add_VT_CTXR($id)
	{
		$options = $this->get_cust_status_code();

		$attrs = array_merge(
				$this->get_ctx_attrs('uri'),
				[
					self::NewBoolAttr('externalRedirect', DMsg::ALbl('l_externalredirect'), false, 'externalredirect'),
					self::NewSelAttr('statusCode', DMsg::ALbl('l_statuscode'), $options, true, 'statuscode'),
					self::NewTextAttr('location', DMsg::ALbl('l_desturi'), 'url', true, 'destinationuri'),
					$this->_attrs['note'],
					$this->_attrs['extraHeaders']
				],
				$this->get_ctx_attrs('auth')
		);
		$defaultExtract = ['type' => 'redirect'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_ctxr'), $attrs, 'uri', 'redirectContext', $defaultExtract);
	}

	protected function add_VT_SSL_ACME($id)
	{
		$attrs = [
            self::NewSelAttr('enabled', DMsg::ALbl('l_enabled'), $this->_options['notset_off_enable'], true, 'acme_vhenable'),
			self::NewTextAttr('api', DMsg::ALbl('l_acme_api'), 'vhname', true, 'acme_dns_type'),
            self::NewParseTextAreaAttr('env', DMsg::ALbl('l_env'), '/\S+=\S+/', DMsg::ALbl('parse_env'), true, 5, 'acme_env', 0, 1, 2),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_acme'), $attrs);
	}

}
