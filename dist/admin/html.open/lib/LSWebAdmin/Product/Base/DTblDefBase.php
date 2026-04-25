<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\DAttr;
use LSWebAdmin\UI\DTbl;
use LSWebAdmin\Product\Current\Service;

class DTblDefBase
{

	protected $_tblDef = [];
	protected $_options = [];
	protected $_attrs;
	protected $_specials = [];

	public function GetTblDef($tblId)
	{
		if (!isset($this->_tblDef[$tblId])) {
			$funcname = 'add_' . $tblId;
			if (!method_exists($this, $funcname)) {
				trigger_error("invalid tid $tblId", E_USER_ERROR);
			}
			$this->$funcname($tblId);
		}
		return $this->_tblDef[$tblId];
	}

	protected function loadSpecials()
	{
		// define special block contains raw data
		$this->addSpecial('rewrite', ['enable', 'logLevel', 'map', 'inherit', 'base'], 'rules');
		$this->addSpecial('virtualHostConfig:rewrite', ['enable', 'logLevel', 'map', 'inherit', 'base'], 'rules'); // for template
		$this->addSpecial('botWhiteList', [], 'list');
	}

	protected function loadModuleSpecials()
	{
		$tags = array_merge(['ls_enabled', 'note', 'internal', 'urlFilter'], $this->getModuleTags());
		$this->addSpecial('module', $tags, 'param');
		$this->addSpecial('urlFilter', ['ls_enabled'], 'param');
	}

	protected function addSpecial($key, $attrList, $catchAllTag)
	{
		$key = strtolower($key);
		$this->_specials[$key] = []; // allow later ones override previous one
		foreach ($attrList as $attr) {
			$this->_specials[$key][] = strtolower($attr);
		}
		$this->_specials[$key]['*'] = $catchAllTag;
	}

	public function MarkSpecialBlock($node)
	{
		$key = strtolower($node->Get(CNode::FLD_KEY));
		if (isset($this->_specials[$key])) {
			$tag = $this->_specials[$key]['*']; // cache all key
			$node->AddRawTag($tag);
			return true;
		}
		return false;
	}

	public function IsSpecialBlockRawContent($node, $testKey)
	{
		$key = strtolower($node->Get(CNode::FLD_KEY));
		if (isset($this->_specials[$key])) {
			if (!in_array(strtolower($testKey), $this->_specials[$key])) {
				return true;
			}
		}
		return false;
	}

	protected function DupTblDef($tblId, $newId, $newTitle = null)
	{
		$tbl = $this->GetTblDef($tblId);
		$newtbl = $tbl->Dup($newId, $newTitle);
		return $newtbl;
	}

	protected static function getDAttrClass()
	{
		return 'LSWebAdmin\\Product\\Current\\DAttr';
	}

	protected static function createDAttr($key, $type, $label, $inputType = null, $allowNull = true, $min = null, $max = null, $inputAttr = null, $multiInd = 0, $helpKey = null)
	{
		$class = static::getDAttrClass();
		return new $class($key, $type, $label, $inputType, $allowNull, $min, $max, $inputAttr, $multiInd, $helpKey);
	}

	protected static function NewIntAttr($key, $label, $allowNull = true, $min = null, $max = null, $helpKey = null)
	{
		return static::createDAttr($key, 'uint', $label, 'text', $allowNull, $min, $max, null, 0, $helpKey);
	}

	protected static function NewBoolAttr($key, $label, $allowNull = true, $helpKey = null)
	{
		return static::createDAttr($key, 'bool', $label, 'radio', $allowNull, null, null, null, 0, $helpKey);
	}

	protected static function NewSelAttr($key, $label, $options, $allowNull = true, $helpKey = null, $inputAttr = null, $multiInd = 0)
	{
		if (is_array($options)) { // fixed options
			return static::createDAttr($key, 'sel', $label, 'select', $allowNull, null, $options, $inputAttr, 0, $helpKey);
        }

		// derived options
		if ($multiInd == 0) {
			return static::createDAttr($key, 'sel1', $label, 'select', $allowNull, $options, null, $inputAttr, 0, $helpKey);
        }

        //sel2 is derived and multi and using text
		return static::createDAttr($key, 'sel2', $label, 'text', $allowNull, $options, null, $inputAttr, 1, $helpKey);
	}

	protected static function NewCheckBoxAttr($key, $label, $options, $allowNull = true, $helpKey = null, $default = null)
	{
		return static::createDAttr($key, 'checkboxOr', $label, 'checkboxgroup', $allowNull, $default, $options, null, 0, $helpKey);
	}

	protected static function NewTextAttr($key, $label, $type, $allowNull = true, $helpKey = null, $multiInd = 0, $inputAttr = null)
	{
		return static::createDAttr($key, $type, $label, 'text', $allowNull, null, null, $inputAttr, $multiInd, $helpKey);
	}

	protected static function NewParseTextAttr($key, $label, $parseformat, $parsehelp, $allowNull = true, $helpKey = null, $multiInd = 0)
	{
		return static::createDAttr($key, 'parse', $label, 'text', $allowNull, $parseformat, $parsehelp, null, $multiInd, $helpKey);
	}

	protected static function NewParseTextAreaAttr($key, $label, $parseformat, $parsehelp, $allowNull = true, $row = 5, $helpKey = null, $viewtextarea = 1, $wrapoff = 0, $multiInd = 0)
	{
		$inputAttr = 'rows="' . $row . '"';
		if ($wrapoff == 1)
			$inputAttr .= ' wrap=off';

		$type = ($viewtextarea == 1) ? 'textarea1' : 'textarea';
		return static::createDAttr($key, 'parse', $label, $type, $allowNull, $parseformat, $parsehelp, $inputAttr, $multiInd, $helpKey);
	}

	protected static function NewTextAreaAttr($key, $label, $type, $allowNull = true, $row = 5, $helpKey = null, $viewtextarea = 1, $wrapoff = 0, $multiInd = 0)
	{
		$inputAttr = 'rows="' . $row . '"';
		if ($wrapoff == 1)
			$inputAttr .= ' wrap="off"';

		$inputtype = ($viewtextarea == 1) ? 'textarea1' : 'textarea';
		return static::createDAttr($key, $type, $label, $inputtype, $allowNull, null, null, $inputAttr, $multiInd, $helpKey);
	}

	protected static function NewPathAttr($key, $label, $type, $reflevel, $rwc = '', $allowNull = true, $helpKey = null, $multiInd = 0)
	{
		return static::createDAttr($key, $type, $label, 'text', $allowNull, $reflevel, $rwc, null, $multiInd, $helpKey);
	}

	protected static function NewCustFlagAttr($key, $label, $flag = 0, $allowNull = true, $type = 'cust', $inputtype = 'text', $helpKey = null, $multiInd = 0)
	{
		$attr = static::createDAttr($key, $type, $label, $inputtype, $allowNull, null, null, null, $multiInd, $helpKey);
		if ($flag != 0)
			$attr->SetFlag($flag);
		return $attr;
	}

	protected static function applyLegacyFde($attr, $fde)
	{
		if ($fde[0] == 'N') {
			$attr->SetFlag(DAttr::BM_NOFILE);
		}

		if ($fde[1] == 'N') {
			$attr->SetFlag(DAttr::BM_HIDE);
		}

		if ($fde[2] == 'N') {
			$attr->SetFlag(DAttr::BM_NOEDIT);
		}

		return $attr;
	}

	protected static function NewPassAttr($key, $label, $allowNull = true, $helpKey = null)
	{
		return static::createDAttr($key, 'cust', $label, 'password', $allowNull, null, null, null, 0, $helpKey);
	}

	protected static function NewViewAttr($key, $label, $helpKey = null) // for view only
	{
		return static::createDAttr($key, 'cust', $label, null, null, null, null, null, 0, $helpKey);
	}

	protected static function NewActionAttr($linkTbl, $act, $allowNull = true)
	{
		return static::createDAttr('action', 'action', DMsg::ALbl('l_action'), null, $allowNull, $linkTbl, $act);
	}

	protected function newAllowOverrideAttr()
	{
		return self::NewCheckBoxAttr('allowOverride', DMsg::ALbl('l_allowoverride'), $this->_options['allowOverride'], true);
	}

	protected function getSharedAppServerEnvOptions()
	{
		return [
			'' => '',
			'0' => 'Development',
			'1' => 'Production',
			'2' => 'Staging'
		];
	}

	protected function getSharedScriptHandlerOptions($extra = [])
	{
		$options = [
			'lsapi' => 'LiteSpeed SAPI',
			'proxy' => 'Web Server (Proxy)',
			'fcgi' => 'Fast CGI',
			'scgi' => 'SCGI',
			'cgi' => 'CGI',
			'loadbalancer' => 'Load Balancer',
			'servlet' => 'Servlet Engine',
			'uwsgi' => 'uWSGI',
		];
		return $options + $extra;
	}

	protected function getSharedContextTypeOptions($extra = [], $staticKey = 'null')
	{
		$options = [
			$staticKey => 'Static',
			'lsapi' => 'LiteSpeed SAPI',
			'proxy' => 'Proxy',
			'fcgi' => 'Fast CGI',
			'scgi' => 'SCGI',
			'cgi' => 'CGI',
			'redirect' => 'Redirect',
			'loadbalancer' => 'Load Balancer',
			'webapp' => 'Java Web App',
			'servlet' => 'Servlet',
			'appserver' => 'App Server',
			'uwsgi' => 'uWSGI',
		];
		return $options + $extra;
	}

	protected function getSharedContextTableOptions($prefix, $extra = [], $staticKey = 'null')
	{
		$options = [
			0 => 'type',
			1 => $prefix . 'G',
			$staticKey => $prefix . 'G',
			'lsapi' => $prefix . 'L',
			'proxy' => $prefix . 'P',
			'fcgi' => $prefix . 'F',
			'scgi' => $prefix . 'SC',
			'cgi' => $prefix . 'C',
			'redirect' => $prefix . 'R',
			'loadbalancer' => $prefix . 'B',
			'webapp' => $prefix . 'J',
			'servlet' => $prefix . 'S',
			'appserver' => $prefix . 'AS',
			'uwsgi' => $prefix . 'UW',
		];
		return $options + $extra;
	}

	protected function getSharedRealmTypeOptions($extra = [])
	{
		$options = ['file' => 'Password File'];
		return $options + $extra;
	}

	protected function loadCommonOptions()
	{
		$this->_options['tp_vname'] = ['/\$VH_NAME/', DMsg::ALbl('parse_tpname')];

		$this->_options['symbolLink'] = ['1' => DMsg::ALbl('o_yes'), '2' => DMsg::ALbl('o_ifownermatch'), '0' => DMsg::ALbl('o_no')];
		$this->_options['allowOverride'] = [
			'1' => DMsg::ALbl('o_limit'),
			'2' => DMsg::ALbl('o_auth'),
			'4' => DMsg::ALbl('o_fileinfo'),
			'8' => DMsg::ALbl('o_indexes'),
			'16' => DMsg::ALbl('o_options'),
			'0' => DMsg::ALbl('o_none')
		];

		$this->_options['disable_off_on'] = ['0' => DMsg::ALbl('o_disabled'), '1' => DMsg::ALbl('o_off'), '2' => DMsg::ALbl('o_on')];
		$this->_options['disable_off_enable'] = ['0' => DMsg::ALbl('o_disabled'), '1' => DMsg::ALbl('o_off'), '2' => DMsg::ALbl('l_enabled')];
		$this->_options['notset_off_on'] = ['' => DMsg::ALbl('o_notset'), '1' => DMsg::ALbl('o_off'), '2' => DMsg::ALbl('o_on')];
		$this->_options['notset_off_enable'] = ['' => DMsg::ALbl('o_notset'), '1' => DMsg::ALbl('o_off'), '2' => DMsg::ALbl('l_enabled')];
		$this->_options['appserverEnv'] = $this->getSharedAppServerEnvOptions();
		$this->_options['scriptHandler'] = $this->getSharedScriptHandlerOptions();

		$this->_options['extType'] = [
			'lsapi' => DMsg::ALbl('l_extlsapi'),
			'proxy' => DMsg::ALbl('l_extproxy'),
			'fcgi' => DMsg::ALbl('l_fcgiapp'),
			'fcgiauth' => DMsg::ALbl('l_extfcgiauth'),
            'scgi' => DMsg::ALbl('l_extscgi'),
			'servlet' => DMsg::ALbl('l_extservlet'),
			'logger' => DMsg::ALbl('l_extlogger'),
			'loadbalancer' => DMsg::ALbl('l_extlb'),
            'uwsgi' => Dmsg::ALbl('l_extuwsgi'),
                ];

		$this->_options['extTbl'] = [
			0 => 'type',
			1 => 'A_EXT_LSAPI',
			'lsapi' => 'A_EXT_LSAPI',
			'proxy' => 'A_EXT_PROXY',
			'fcgi' => 'A_EXT_FCGI',
			'fcgiauth' => 'A_EXT_FCGIAUTH',
            'scgi' => 'A_EXT_SCGI',
			'servlet' => 'A_EXT_SERVLET',
			'logger' => 'A_EXT_LOGGER',
			'loadbalancer' => 'A_EXT_LOADBALANCER',
            'uwsgi' => 'A_EXT_UWSGI',
            ];

		$this->_options['tp_extTbl'] = [
			0 => 'type',
			1 => 'T_EXT_LSAPI',
			'lsapi' => 'T_EXT_LSAPI',
			'proxy' => 'T_EXT_PROXY',
			'fcgi' => 'T_EXT_FCGI',
			'fcgiauth' => 'T_EXT_FCGIAUTH',
            'scgi' => 'T_EXT_SCGI',
			'servlet' => 'T_EXT_SERVLET',
			'logger' => 'T_EXT_LOGGER',
			'loadbalancer' => 'T_EXT_LOADBALANCER',
            'uwsgi' => 'T_EXT_UWSGI',
            ];

		$this->_options['logLevel'] = ['ERROR' => 'ERROR', 'WARN' => 'WARNING',
			'NOTICE' => 'NOTICE', 'INFO' => 'INFO', 'DEBUG' => 'DEBUG'];

		$this->_options['aclogctrl'] = [
			0 => DMsg::ALbl('o_ownlogfile'),
			1 => DMsg::ALbl('o_serverslogfile'),
			2 => DMsg::ALbl('o_disabled')];

		$this->_options['cacheEngine'] = [
			'1' => DMsg::ALbl('o_on'),
			'2' => DMsg::ALbl('o_crawler'),
			'4' => DMsg::ALbl('o_esi'),
			'0' => DMsg::ALbl('o_disabled')
		];

		// for shared parse format
		$this->_options['parseFormat'] = [
			'filePermission4' => '/^0?[0-7]{3,4}$/',
			'filePermission3' => '/^0?[0-7]{3}$/'
		];

		$ipv6str = isset($_SERVER['LSWS_IPV6_ADDRS']) ? $_SERVER['LSWS_IPV6_ADDRS'] : '';
		$ipv6 = [];
		if ($ipv6str != '') {
			$ipv6['[ANY]'] = '[ANY] IPv6';
			$ips = explode(',', $ipv6str);
			foreach ($ips as $ip) {
				if (($pos = strpos($ip, ':')) !== false) {
					$aip = substr($ip, $pos + 1);
					$ipv6[$aip] = $aip;
				}
			}
		}
		$ipo = [];
		$ipo['ANY'] = 'ANY IPv4';
		$ipstr = isset($_SERVER['LSWS_IPV4_ADDRS']) ? $_SERVER['LSWS_IPV4_ADDRS'] : '';
		if ($ipstr != '') {
			$ips = explode(',', $ipstr);
			foreach ($ips as $ip) {
				if (($pos = strpos($ip, ':')) !== false) {
					$aip = substr($ip, $pos + 1);
					$ipo[$aip] = $aip;
					if ($aip != '127.0.0.1') {
						$ipv6["[::FFFF:$aip]"] = "[::FFFF:$aip]";
                    }
				}
			}
		}
		if ($ipv6str != '') {
			$this->_options['ip'] = $ipo + $ipv6;
        } else {
			$this->_options['ip'] = $ipo;
        }
	}

	protected function loadCommonAttrs()
	{
		$ctxOrder = self::NewViewAttr('order', DMsg::ALbl('l_order'));
		$ctxOrder->SetFlag(DAttr::BM_NOFILE | DAttr::BM_HIDE | DAttr::BM_NOEDIT);

		$forbidden_ext_groups = ['root', 'sudo', 'wheel', 'shadow', 'lsadm'];
		$forbidden_ext_users = ['root', 'lsadm'];


		$attrs = [
			'priority' => self::NewIntAttr('priority', DMsg::ALbl('l_priority'), true, -20, 20),
			'indexFiles' => self::NewTextAreaAttr('indexFiles', DMsg::ALbl('l_indexfiles'), 'fname', true, 2, null, 0, 0, 1),
			'autoIndex' => self::NewBoolAttr('autoIndex', DMsg::ALbl('l_autoindex')),
			'adminEmails' => self::NewTextAreaAttr('adminEmails', DMsg::ALbl('l_adminemails'), 'email', true, 3, null, 0, 0, 1),
			'suffix' => self::NewParseTextAttr('suffix', DMsg::ALbl('l_suffix'), '/^[A-z0-9_\-]+$/', DMsg::ALbl('parse_suffix'), false, null, 1),
			'fileName2' => self::NewPathAttr('fileName', DMsg::ALbl('l_filename'), 'file0', 2, 'r', false),
			'fileName3' => self::NewPathAttr('fileName', DMsg::ALbl('l_filename'), 'file0', 3, 'r', true),
			'rollingSize' => self::NewIntAttr('rollingSize', DMsg::ALbl('l_rollingsize'), true, null, null, 'log_rollingSize'),
			'keepDays' => self::NewIntAttr('keepDays', DMsg::ALbl('l_keepdays'), true, 0, null, 'log_keepDays'),
			'logFormat' => self::NewTextAttr('logFormat', DMsg::ALbl('l_logformat'), 'cust', true, 'accessLog_logFormat'),
			'logHeaders' => self::NewCheckBoxAttr('logHeaders', DMsg::ALbl('l_logheaders'), ['1' => 'Referrer', '2' => 'UserAgent', '4' => 'Host', '0' => DMsg::ALbl('o_none')], true, 'accessLog_logHeader'),
			'compressArchive' => self::NewBoolAttr('compressArchive', DMsg::ALbl('l_compressarchive'), true, 'log_compressArchive'),
			'extraHeaders' => self::NewTextAreaAttr('extraHeaders', DMsg::ALbl('l_extraHeaders'), 'cust', true, 5, null, 1, 1),
			'scriptHandler_type' => self::NewSelAttr('type', DMsg::ALbl('l_handlertype'), $this->_options['scriptHandler'], false, 'shType', 'data-lst-change-call="lst_conf" data-lst-change-arg="c"'),
			'scriptHandler' => self::NewSelAttr('handler', DMsg::ALbl('l_handlername'), 'extprocessor:$$type', false, 'shHandlerName'),
			'ext_type' => self::NewSelAttr('type', DMsg::ALbl('l_type'), $this->_options['extType'], false, 'extAppType'),
			'name' => self::NewTextAttr('name', DMsg::ALbl('l_name'), 'name', false),
			'ext_name' => self::NewTextAttr('name', DMsg::ALbl('l_name'), 'name', false, 'extAppName'),
			'ext_address' => self::NewTextAttr('address', DMsg::ALbl('l_address'), 'addr', false, 'extAppAddress'),
			'ext_maxConns' => self::NewIntAttr('maxConns', DMsg::ALbl('l_maxconns'), false, 1, 2000),
			'pcKeepAliveTimeout' => self::NewIntAttr('pcKeepAliveTimeout', DMsg::ALbl('l_pckeepalivetimeout'), true, -1, 10000),
			'ext_env' => self::NewParseTextAreaAttr('env', DMsg::ALbl('l_env'), '/\S+=\S+/', DMsg::ALbl('parse_env'), true, 5, null, 0, 1, 2),
			'ext_initTimeout' => self::NewIntAttr('initTimeout', DMsg::ALbl('l_inittimeout'), false, 1),
			'ext_retryTimeout' => self::NewIntAttr('retryTimeout', DMsg::ALbl('l_retrytimeout'), false, 0),
			'ext_respBuffer' => self::NewSelAttr('respBuffer', DMsg::ALbl('l_respbuffer'), ['0' => DMsg::ALbl('o_no'), '1' => DMsg::ALbl('o_yes'), '2' => DMsg::ALbl('o_nofornph')], false),
			'ext_persistConn' => self::NewBoolAttr('persistConn', DMsg::ALbl('l_persistconn')),
			'ext_autoStart' => self::NewSelAttr('autoStart', DMsg::ALbl('l_autostart'), ['2' => DMsg::ALbl('o_thrucgidaemon'), '0' => DMsg::ALbl('o_no')], false),
			'ext_path' => self::NewPathAttr('path', DMsg::ALbl('l_command'), 'file1', 3, 'x', true, 'extAppPath'),
			'ext_backlog' => self::NewIntAttr('backlog', DMsg::ALbl('l_backlog'), true, 1, 100),
			'ext_instances' => self::NewIntAttr('instances', DMsg::ALbl('l_instances'), true, 0, 1000),
			'ext_runOnStartUp' => self::NewSelAttr('runOnStartUp', DMsg::ALbl('l_runonstartup'), ['' => '', '1' => DMsg::ALbl('o_yes'), '3' => DMsg::ALbl('o_yesdetachmode'), '2' => DMsg::ALbl('o_yesdaemonmode'), '0' => DMsg::ALbl('o_no')]),
			'ext_user' => self::NewParseTextAttr('extUser', DMsg::ALbl('l_suexecuser'), '/^(?!(?:' . implode('|', $forbidden_ext_users) . ")\\b)/", null),
			'ext_group' => self::NewParseTextAttr('extGroup', DMsg::ALbl('l_suexecgrp'), '/^(?!(?:' . implode('|', $forbidden_ext_groups) . ")\\b)/", null),
			'cgiUmask' => self::NewParseTextAttr('umask', DMsg::ALbl('l_umask'), $this->_options['parseFormat']['filePermission3'], DMsg::ALbl('parse_umask')),
			'memSoftLimit' => self::NewIntAttr('memSoftLimit', DMsg::ALbl('l_memsoftlimit'), true, 0),
			'memHardLimit' => self::NewIntAttr('memHardLimit', DMsg::ALbl('l_memhardlimit'), true, 0),
			'procSoftLimit' => self::NewIntAttr('procSoftLimit', DMsg::ALbl('l_procsoftlimit'), true, 0),
			'procHardLimit' => self::NewIntAttr('procHardLimit', DMsg::ALbl('l_prochardlimit'), true, 0),
			'ssl_renegProtection' => self::NewBoolAttr('renegProtection', DMsg::ALbl('l_renegprotection')),
			'sslSessionCache' => self::NewBoolAttr('sslSessionCache', DMsg::ALbl('l_sslSessionCache')),
			'sslSessionTickets' => self::NewBoolAttr('sslSessionTickets', DMsg::ALbl('l_sslSessionTickets')),
			'l_vhost' => self::NewSelAttr('vhost', DMsg::ALbl('l_vhost'), 'virtualhost', false, 'virtualHostName'),
			'l_domain' => self::NewTextAttr('domain', DMsg::ALbl('l_domains'), 'domain', false, 'domainName', 1),
			'tp_templateFile' => self::NewPathAttr('templateFile', DMsg::ALbl('l_templatefile'), 'filetp', 2, 'rwc', false),
			'tp_listeners' => self::NewSelAttr('listeners', DMsg::ALbl('l_mappedlisteners'), 'listener', false, 'mappedListeners', null, 1),
			'tp_vhName' => self::NewTextAttr('vhName', DMsg::ALbl('l_vhname'), 'vhname', false, 'templateVHName'),
			'tp_vhDomain' => self::NewTextAttr('vhDomain', DMsg::ALbl('l_domain'), 'domain', true, 'templateVHDomain'),
			'tp_vhAliases' => self::NewTextAttr('vhAliases', DMsg::ALbl('l_vhaliases'), 'domain', true, 'templateVHAliases', 1),
			'tp_vhRoot' => self::NewParseTextAttr('vhRoot', DMsg::ALbl('l_defaultvhroot'), $this->_options['tp_vname'][0], $this->_options['tp_vname'][1], false, 'templateVHRoot'),
			'tp_vrFile' => self::NewParseTextAttr('fileName', DMsg::ALbl('l_filename'), '/(\$VH_NAME)|(\$VH_ROOT)/', DMsg::ALbl('parse_tpfile'), false, 'tp_vhFileName'),
			'tp_name' => self::NewParseTextAttr('name', DMsg::ALbl('l_name'), $this->_options['tp_vname'][0], $this->_options['tp_vname'][1], false, 'extAppName'),
			'vh_maxKeepAliveReq' => self::NewIntAttr('maxKeepAliveReq', DMsg::ALbl('l_maxkeepalivereq'), true, 0, 32767, 'vhMaxKeepAliveReq'),
			'vh_cgroups' => self::NewSelAttr('cgroups', DMsg::ALbl('l_cgroups'), $this->_options['notset_off_on']),
			'vh_enableGzip' => self::NewBoolAttr('enableGzip', DMsg::ALbl('l_enablecompress'), true, 'vhEnableGzip'),
			'vh_enableBr' => self::NewBoolAttr('enableBr', DMsg::ALbl('l_enablebrotli'), true, 'vhEnableBr'),
			'vh_allowSymbolLink' => self::NewSelAttr('allowSymbolLink', DMsg::ALbl('l_allowsymbollink'), $this->_options['symbolLink']),
			'vh_enableScript' => self::NewBoolAttr('enableScript', DMsg::ALbl('l_enablescript'), false),
			'vh_restrained' => self::NewBoolAttr('restrained', DMsg::ALbl('l_restrained'), false),
			'vh_setUIDMode' => self::NewSelAttr('setUIDMode', DMsg::ALbl('l_setuidmode'), ['' => '', 0 => 'Server UID', 1 => 'CGI File UID', 2 => 'DocRoot UID'], true, 'setUidMode'),
			'vh_suexec_user' => self::NewTextAttr('user', DMsg::ALbl('l_suexecuser1'), 'cust', true, 'suexecUser'),
			'vh_suexec_group' => self::NewTextAttr('group', DMsg::ALbl('l_suexecgrp1'), 'cust', true, 'suexecGroup'),
			'staticReqPerSec' => self::NewIntAttr('staticReqPerSec', DMsg::ALbl('l_staticreqpersec'), true, 0),
			'dynReqPerSec' => self::NewIntAttr('dynReqPerSec', DMsg::ALbl('l_dynreqpersec'), true, 0),
			'outBandwidth' => self::NewIntAttr('outBandwidth', DMsg::ALbl('l_outbandwidth'), true, 0),
			'inBandwidth' => self::NewIntAttr('inBandwidth', DMsg::ALbl('l_inbandwidth'), true, 0),
			'ctx_order' => $ctxOrder,
			'ctx_type' => self::NewSelAttr('type', DMsg::ALbl('l_contexttype'), $this->_options['ctxType'], false, 'ctxType'),
			'ctx_uri' => self::NewTextAttr('uri', DMsg::ALbl('l_uri'), 'expuri', false, 'expuri'),
			'ctx_location' => self::NewTextAttr('location', DMsg::ALbl('l_location'), 'cust'),
			'ctx_shandler' => self::NewSelAttr('handler', DMsg::ALbl('l_servletengine'), 'extprocessor:servlet', false, 'servletEngine'),
			'appserverEnv' => self::NewSelAttr('appserverEnv', DMsg::ALbl('l_runtimemode'), $this->_options['appserverEnv']),
			'geoipDBFile' => self::NewPathAttr('geoipDBFile', DMsg::ALbl('l_geoipdbfile'), 'filep', 2, 'r', false),
			'enableIpGeo' => self::NewBoolAttr('enableIpGeo', DMsg::ALbl('l_enableipgeo')),
			'enable_pagespeed' => self::NewBoolAttr('enabled', DMsg::ALbl('l_enablepagespeed'), true, 'pagespeedEnabled'),
			'pagespeed_setting' => self::NewTextAreaAttr('param', DMsg::ALbl('l_pagespeedsettings'), 'cust', true, 25, 'pagespeedParams', 1, 1),
			'note' => self::NewTextAreaAttr('note', DMsg::ALbl('l_notes'), 'note', true, 4, null, 0),
		];
		self::applyLegacyFde($attrs['enable_pagespeed'], 'YNN');
		self::applyLegacyFde($attrs['pagespeed_setting'], 'YNN');
		$this->_attrs = $attrs;
		$this->applyCommonAttrConfig();
	}

	protected function loadModuleCommonAttrs()
	{
		$param = self::NewTextAreaAttr('param', DMsg::ALbl('l_moduleparams'), 'cust', true, 4, 'modParams', 1, 1);
		$param->SetFlag(DAttr::BM_RAWDATA);
		$this->_attrs['mod_params'] = $param;
		$this->_attrs['mod_enabled'] = self::NewBoolAttr('ls_enabled', DMsg::ALbl('l_enablehooks'), true, 'moduleEnabled');
	}

	protected function applyCommonAttrConfig()
	{
	}

	//	DAttr($key, $type, $label,  $inputType, $allowNull,$min, $max, $inputAttr, $multiInd)
	protected function get_expires_attrs()
	{
		return [
			self::NewBoolAttr('enableExpires', DMsg::ALbl('l_enableexpires')),
			self::NewParseTextAttr('expiresDefault', DMsg::ALbl('l_expiresdefault'), '/^[AaMm]\d+$/', DMsg::ALbl('parse_expiresdefault')),
			self::NewParseTextAreaAttr('expiresByType', DMsg::ALbl('l_expiresByType'), '/^(\*\/\*)|([A-z0-9_\-\.\+]+\/\*)|([A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+)=[AaMm]\d+$/', DMsg::ALbl('parse_expiresByType'), true, 2, null, 0, 0, 1)
		];
	}

	protected static function newHiddenPermissionMaskAttr($key, $label, $parseFormat, $parseHelp)
	{
		$attr = self::NewParseTextAttr($key, $label, $parseFormat, $parseHelp);
		$attr->SetFlag(DAttr::BM_HIDE | DAttr::BM_NOEDIT);
		return $attr;
	}

	protected function getThrottleAttrs()
	{
		return [
			$this->_attrs['staticReqPerSec'],
			$this->_attrs['dynReqPerSec'],
			$this->_attrs['outBandwidth'],
			$this->_attrs['inBandwidth'],
		];
	}

	protected function getRecaptchaCommonAttrs($parseFormat, $parseHelp)
	{
		return [
			self::NewBoolAttr('enabled', DMsg::ALbl('l_recapenabled'), true, 'enableRecaptcha'),
			self::NewParseTextAttr('siteKey', DMsg::ALbl('l_sitekey'), $parseFormat, $parseHelp, true, 'recaptchaSiteKey'),
			self::NewParseTextAttr('secretKey', DMsg::ALbl('l_secretKey'), $parseFormat, $parseHelp, true, 'recaptchaSecretKey'),
			self::NewSelAttr('type', DMsg::ALbl('l_recaptype'), $this->_options['captcha'], true, 'recaptchaType'),
			self::NewIntAttr('maxTries', DMsg::ALbl('l_maxTries'), true, 0, 65535, 'recaptchaMaxTries'),
		];
	}

	protected function getCachePolicyAttrs($serverLevel)
	{
		$attrs = [
			self::NewBoolAttr('cachePolicy:checkPublicCache', DMsg::ALbl('l_checkpubliccache'), true, 'checkPublicCache'),
			self::NewBoolAttr('cachePolicy:checkPrivateCache', DMsg::ALbl('l_checkprivatecache'), true, 'checkPrivateCache'),
			self::NewBoolAttr('cachePolicy:respectCacheable', DMsg::ALbl('l_respectcachecontrol'), true, 'respectCacheable'),
			self::NewIntAttr('cachePolicy:maxCacheObjSize', DMsg::ALbl('l_maxobjectsize'), true, '1K', '100M'),
			self::NewIntAttr('cachePolicy:maxStaleAge', DMsg::ALbl('l_cachestaleage'), true, 0),
			self::NewTextAreaAttr('cachePolicy:cacheKeyMod', DMsg::ALbl('l_cachekeymod'), 'cust', true, 3, null, 0),
		];

		if (!$serverLevel) {
			$attrs[] = self::NewBoolAttr('cachePolicy:enableCache', DMsg::ALbl('l_publiclycacheall'));
			$attrs[] = self::NewBoolAttr('cachePolicy:enablePrivateCache', DMsg::ALbl('l_privatelycacheall'));
		}

		$attrs[] = self::NewIntAttr('cachePolicy:expireInSeconds', DMsg::ALbl('l_cacheexpiretime'), true, 0);
		$attrs[] = self::NewIntAttr('cachePolicy:privateExpireInSeconds', DMsg::ALbl('l_privatecacheexpiretime'), true, 0);
		$attrs[] = self::NewBoolAttr('cachePolicy:microCache5xx', DMsg::ALbl('l_microcache5xx'));
		$attrs[] = self::NewBoolAttr('cachePolicy:enablePostCache', DMsg::ALbl('l_enablepostcache'));

		foreach ($attrs as $attr) {
			$attr->_feature = 1;
		}

		return $attrs;
	}

	protected static function getSslProtocolAttrs($includeProtocol = true)
	{
		$attrs = [];

		if ($includeProtocol) {
			$attrs[] = self::NewCheckBoxAttr('sslProtocol', DMsg::ALbl('l_protocolver'), ['1' => 'SSL v3.0', '2' => 'TLS v1.0', '4' => 'TLS v1.1', '8' => 'TLS v1.2', '16' => 'TLS v1.3']);
		}

		$attrs[] = self::NewTextAttr('ciphers', DMsg::ALbl('l_ciphers'), 'cust');
		$attrs[] = self::NewBoolAttr('enableECDHE', DMsg::ALbl('l_enableecdhe'));
		$attrs[] = self::NewBoolAttr('enableDHE', DMsg::ALbl('l_enabledhe'));
		$attrs[] = self::NewTextAttr('DHParam', DMsg::ALbl('l_dhparam'), 'cust');

		return $attrs;
	}

	protected function getSslFeatureAttrs($enableQuicLabel, $enableQuicHelpKey)
	{
		return [
			$this->_attrs['ssl_renegProtection'],
			$this->_attrs['sslSessionCache'],
			$this->_attrs['sslSessionTickets'],
			self::NewCheckBoxAttr('enableSpdy', DMsg::ALbl('l_alpn'), ['4' => 'HTTP/2', '8' => 'HTTP/3', '0' => DMsg::ALbl('o_none')]),
			self::NewBoolAttr('enableQuic', $enableQuicLabel, true, $enableQuicHelpKey),
		];
	}

	protected function getAppServerDefaultAttrs($binLabel, $binHelpKey)
	{
		return [
			self::NewPathAttr('binPath', $binLabel, 'file', 1, 'x', true, $binHelpKey),
			$this->_attrs['appserverEnv'],
			$this->_attrs['ext_maxConns'],
			$this->_attrs['ext_env'],
			$this->_attrs['ext_initTimeout'],
			$this->_attrs['ext_retryTimeout'],
			$this->_attrs['pcKeepAliveTimeout'],
			$this->_attrs['ext_respBuffer'],
			$this->_attrs['ext_backlog'],
			$this->_attrs['ext_runOnStartUp'],
			self::NewIntAttr('extMaxIdleTime', DMsg::ALbl('l_maxidletime'), true, -1),
			$this->_attrs['priority']->dup(null, null, 'extAppPriority'),
			$this->_attrs['memSoftLimit'],
			$this->_attrs['memHardLimit'],
			$this->_attrs['procSoftLimit'],
			$this->_attrs['procHardLimit']
		];
	}

	protected function getExtNetworkAppAttrs($addressAttr, $includeEnv = true)
	{
		$attrs = [
			$this->_attrs['ext_name'],
			$addressAttr,
			$this->_attrs['note'],
			$this->_attrs['ext_maxConns'],
		];

		if ($includeEnv) {
			$attrs[] = $this->_attrs['ext_env'];
		}

		$attrs[] = $this->_attrs['ext_initTimeout'];
		$attrs[] = $this->_attrs['ext_retryTimeout'];
		$attrs[] = $this->_attrs['pcKeepAliveTimeout'];
		$attrs[] = $this->_attrs['ext_respBuffer'];

		return $attrs;
	}

	protected function setDerivedExtAppTbl($sourceTblId, $id, $title, $type)
	{
		$this->_tblDef[$id] = $this->DupTblDef($sourceTblId, $id, $title);
		$this->_tblDef[$id]->Set(DTbl::FLD_DEFAULTEXTRACT, ['type' => $type]);
	}

	protected function setTemplateExtTblDef($sourceTblId, $id)
	{
		$this->_tblDef[$id] = $this->DupTblDef($sourceTblId, $id);
		$this->_tblDef[$id]->ResetAttrEntry(0, $this->_attrs['tp_name']);
	}

	protected function setTemplateFileNameTblDef($sourceTblId, $id)
	{
		$this->_tblDef[$id] = $this->DupTblDef($sourceTblId, $id);
		$this->_tblDef[$id]->ResetAttrEntry(1, $this->_attrs['tp_vrFile']);
	}

	protected function setExtAppSelTbl($id, $subTbls)
	{
		$attrs = [$this->_attrs['ext_type']];
		$this->_tblDef[$id] = DTbl::NewSel($id, DMsg::ALbl('l_newextapp'), $attrs, $subTbls);
	}

	protected function getExtAppTopAttrs($actionTbls)
	{
		return [
			$this->_attrs['ext_type'],
			self::NewViewAttr('name', DMsg::ALbl('l_name')),
			self::NewViewAttr('address', DMsg::ALbl('l_address')),
			self::NewActionAttr($actionTbls, 'vEd')
		];
	}

	protected function newAccessLogControlAttr()
	{
		return self::NewSelAttr('useServer', DMsg::ALbl('l_logcontrol'), $this->_options['aclogctrl'], false, 'aclogUseServer');
	}

	protected function getAccessLogTopAttrs($fileAttr, $actionTbl)
	{
		return [
			$this->newAccessLogControlAttr(),
			$fileAttr,
			$this->_attrs['logFormat'],
			$this->_attrs['rollingSize'],
			self::NewActionAttr($actionTbl, 'Ed'),
		];
	}

	protected function getAccessLogAttrs($fileAttr)
	{
		return [
			$this->newAccessLogControlAttr(),
			$fileAttr,
			self::NewSelAttr('pipedLogger', DMsg::ALbl('l_pipedlogger'), 'extprocessor:logger', true, 'accessLog_pipedLogger'),
			$this->_attrs['logFormat'],
			$this->_attrs['logHeaders'],
			$this->_attrs['rollingSize'],
			$this->_attrs['keepDays'],
			$this->_attrs['compressArchive'],
			self::NewPathAttr('bytesLog', DMsg::ALbl('l_byteslog'), 'file0', 3, 'r', true, 'accessLog_bytesLog'),
		];
	}

	protected function getContextTypeIcons()
	{
		return [
			'null' => 'file-text',
			'lsapi' => 'rocket',
			'proxy' => 'arrow-right-left',
			'fcgi' => 'gauge',
			'scgi' => 'gauge',
			'cgi' => 'square-terminal',
			'redirect' => 'arrow-up-right',
			'loadbalancer' => 'scale',
			'webapp' => 'app-window',
			'servlet' => 'package',
			'appserver' => 'server-cog',
			'uwsgi' => 'circuit-board',
			'module' => 'puzzle'
		];
	}

	protected function getScriptHandlerAttrs()
	{
		return [
			$this->_attrs['suffix'],
			$this->_attrs['scriptHandler_type'],
			$this->_attrs['scriptHandler'],
		];
	}

	protected function add_S_INDEX($id)
	{
		$attrs = [
			$this->_attrs['indexFiles'],
			$this->_attrs['autoIndex'],
			self::NewTextAttr('autoIndexURI', DMsg::ALbl('l_autoindexuri'), 'uri')
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_indexfiles'), $attrs);
	}

	protected function add_S_LOG($id)
	{
		$attrs = [
			$this->_attrs['fileName2']->dup(null, null, 'log_fileName'),
			self::NewSelAttr('logLevel', DMsg::ALbl('l_loglevel'), $this->_options['logLevel'], false, 'log_logLevel'),
			self::NewSelAttr('debugLevel', DMsg::ALbl('l_debuglevel'), ['10' => DMsg::ALbl('o_high'), '5' => DMsg::ALbl('o_medium'), '2' => DMsg::ALbl('o_low'), '0' => DMsg::ALbl('o_none')], false, 'log_debugLevel'),
			$this->_attrs['rollingSize'],
			$this->_attrs['keepDays'],
			$this->_attrs['compressArchive'],
			self::NewBoolAttr('enableStderrLog', DMsg::ALbl('l_enablestderrlog'), true, 'log_enableStderrLog'),
			//self::NewBoolAttr('enableAioLog', DMsg::ALbl('l_enableaiolog'), true, 'log_enableAioLog'),
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_serverlog'), $attrs, 'fileName');
	}

	protected function add_S_ACLOG_TOP($id)
	{
		$attrs = [
			$this->_attrs['fileName2']->dup(null, null, 'accessLog_fileName'),
			$this->_attrs['logFormat'],
			$this->_attrs['rollingSize'],
			self::NewActionAttr('S_ACLOG', 'Ed'),
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_accesslog'), $attrs, 'fileName', 'S_ACLOG');
	}

	protected function add_S_ACLOG($id)
	{
		$attrs = [
			$this->_attrs['fileName2']->dup(null, null, 'accessLog_fileName'),
			self::NewSelAttr('pipedLogger', DMsg::ALbl('l_pipedlogger'), 'extprocessor:logger', true, 'accessLog_pipedLogger'),
			$this->_attrs['logFormat'],
			$this->_attrs['logHeaders'],
			$this->_attrs['rollingSize'],
			$this->_attrs['keepDays'],
			$this->_attrs['compressArchive']
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_accesslog'), $attrs, 'fileName');
	}

	protected function add_A_EXPIRES($id)
	{
		$attrs = $this->get_expires_attrs();
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_expires'), $attrs);
	}

	protected function add_S_GEOIP_TOP($id)
	{
		$align = ['center', 'center', 'center'];

		$attrs = [
			$this->_attrs['geoipDBFile'],
			self::NewViewAttr('geoipDBName', DMsg::ALbl('l_dbname')),
			self::NewActionAttr('S_GEOIP', 'Ed')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_geoipdb'), $attrs, 'geoipDBFile', 'S_GEOIP', $align, 'geolocationDB', 'database', true);
	}

	protected function add_S_GEOIP($id)
	{
		$attrs = [
			$this->_attrs['geoipDBFile'],
			self::NewTextAttr('geoipDBName', DMsg::ALbl('l_dbname'), 'dbname', false),
			self::NewParseTextAreaAttr('maxMindDBEnv', DMsg::ALbl('l_envvariable'), '/^\S+[ \t]+\S+$/', DMsg::ALbl('parse_geodbenv'), true, 5, null, 0, 1, 2),
			$this->_attrs['note'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_geoipdb'), $attrs, 'geoipDBFile', 'geolocationDB');
	}

	protected function add_S_IP2LOCATION($id)
	{
		$attrs = [
			self::NewPathAttr('ip2locDBFile', DMsg::ALbl('l_ip2locDBFile'), 'filep', 2, 'r'),
			self::NewSelAttr('ip2locDBCache', DMsg::ALbl('l_ip2locDBCache'),
					[
						'' => '',
						'FileIo' => 'File System',
						'MemoryCache' => 'Memory',
						'SharedMemoryCache' => 'Shared Memory'
			]),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_ip2locDB'), $attrs);
	}

	protected function add_S_TUNING_CONN($id)
	{
		$attrs = [
			self::NewIntAttr('maxConnections', DMsg::ALbl('l_maxconns'), false, 1),
			self::NewIntAttr('maxSSLConnections', DMsg::ALbl('l_maxsslconns'), false, 0),
			self::NewIntAttr('connTimeout', DMsg::ALbl('l_conntimeout'), false, 10, 1000000),
			self::NewIntAttr('maxKeepAliveReq', DMsg::ALbl('l_maxkeepalivereq'), false, 0, 32767),
			self::NewIntAttr('keepAliveTimeout', DMsg::ALbl('l_keepalivetimeout'), false, 0, 60),
			self::NewIntAttr('sndBufSize', DMsg::ALbl('l_sndbufsize'), true, 0, '512K'),
			self::NewIntAttr('rcvBufSize', DMsg::ALbl('l_rcvbufsize'), true, 0, '512K'),
		];

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_connection'), $attrs);
	}

	protected function add_S_TUNING_REQ($id)
	{
		$attrs = [
			self::NewIntAttr('maxReqURLLen', DMsg::ALbl('l_maxrequrllen'), false, 200, 65530),
			self::NewIntAttr('maxReqHeaderSize', DMsg::ALbl('l_maxreqheadersize'), false, 1024, 65530),
			self::NewIntAttr('maxReqBodySize', DMsg::ALbl('l_maxreqbodysize'), false, '1M', null),
			self::NewIntAttr('maxDynRespHeaderSize', DMsg::ALbl('l_maxdynrespheadersize'), false, 200, '64K'),
			self::NewIntAttr('maxDynRespSize', DMsg::ALbl('l_maxdynrespsize'), false, '1M', null)
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_reqresp'), $attrs);
	}

	protected function add_S_TUNING_GZIP($id)
	{
		$parseFormat = '/^(\!)?(\*\/\*)|([A-z0-9_\-\.\+]+\/\*)|([A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+)|default$/';

		$attrs = [
			// general
			self::NewBoolAttr('enableGzipCompress', DMsg::ALbl('l_enablecompress'), false),
			self::NewParseTextAreaAttr('compressibleTypes', DMsg::ALbl('l_compressibletypes'), $parseFormat, DMsg::ALbl('parse_compressibletypes'), true, 5, null, 0, 0, 1),
			// dyn
			self::NewBoolAttr('enableDynGzipCompress', DMsg::ALbl('l_enabledyngzipcompress'), false),
			self::NewIntAttr('gzipCompressLevel', DMsg::ALbl('l_gzipcompresslevel'), true, 1, 9),
			// self::NewIntAttr('enableBrCompress', DMsg::ALbl('l_brcompresslevel'), true, 0, 6), // OLS does not support dynamic brotli
			// static
			self::NewBoolAttr('gzipAutoUpdateStatic', DMsg::ALbl('l_gzipautoupdatestatic')),
			self::NewIntAttr('gzipStaticCompressLevel', DMsg::ALbl('l_gzipstaticcompresslevel'), true, 1, 9),
			self::NewIntAttr('brStaticCompressLevel', DMsg::ALbl('l_brstaticcompresslevel'), true, 0, 11), // 0 will disable
			self::NewTextAttr('gzipCacheDir', DMsg::ALbl('l_gzipcachedir'), 'cust'),
			self::NewIntAttr('gzipMaxFileSize', DMsg::ALbl('l_gzipmaxfilesize'), true, '1K'),
			self::NewIntAttr('gzipMinFileSize', DMsg::ALbl('l_gzipminfilesize'), true, 200)
		];

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_gzipbr'), $attrs);
	}

	protected function add_S_TUNING_QUIC($id)
	{
		$congest_options = ['' => 'Default', '1' => 'Cubic', '2' => 'BBR'];
		$attrs = [
			self::NewBoolAttr('quicEnable', DMsg::ALbl('l_enablequic')),
			self::NewTextAttr('quicShmDir', DMsg::ALbl('l_quicshmdir'), 'cust'),
			self::NewTextAttr('quicVersions', DMsg::ALbl('l_quicversions'), 'cust'),
			self::NewSelAttr('quicCongestionCtrl', DMsg::ALbl('l_congestionctrl'), $congest_options),
			self::NewIntAttr('quicCfcw', DMsg::ALbl('l_quiccfcw'), true, '64K', '512M'),
			self::NewIntAttr('quicMaxCfcw', DMsg::ALbl('l_quicmaxcfcw'), true, '64K', '512M'),
			self::NewIntAttr('quicSfcw', DMsg::ALbl('l_quicsfcw'), true, '64K', '128M'),
			self::NewIntAttr('quicMaxSfcw', DMsg::ALbl('l_quicmaxsfcw'), true, '64K', '128M'),
			self::NewIntAttr('quicMaxStreams', DMsg::ALbl('l_quicmaxstreams'), true, 10, 1000),
			self::NewIntAttr('quicHandshakeTimeout', DMsg::ALbl('l_quichandshaketimeout'), true, 1, 15),
			self::NewIntAttr('quicIdleTimeout', DMsg::ALbl('l_quicidletimeout'), true, 10, 30),
			self::NewBoolAttr('quicEnableDPLPMTUD', DMsg::ALbl('l_quicenabledplpmtud')),
			self::NewIntAttr('quicBasePLPMTU', DMsg::ALbl('l_quicbaseplpmtu'), true, 0, 65527),
			self::NewIntAttr('quicMaxPLPMTU', DMsg::ALbl('l_quicmaxplpmtu'), true, 0, 65527),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_quic'), $attrs);
	}

	protected function add_S_SEC_FILE($id)
	{
		$parseFormat = $this->_options['parseFormat']['filePermission4'];
		$parseHelp = DMsg::ALbl('parse_secpermissionmask');

		$attrs = [
			self::NewSelAttr('followSymbolLink', DMsg::ALbl('l_followsymbollink'), $this->_options['symbolLink'], false),
			self::NewBoolAttr('checkSymbolLink', DMsg::ALbl('l_checksymbollink'), false),
			self::NewBoolAttr('forceStrictOwnership', DMsg::ALbl('l_forcestrictownership'), false),
			self::newHiddenPermissionMaskAttr('requiredPermissionMask', DMsg::ALbl('l_requiredpermissionmask'), $parseFormat, $parseHelp),
			self::newHiddenPermissionMaskAttr('restrictedPermissionMask', DMsg::ALbl('l_restrictedpermissionmask'), $parseFormat, $parseHelp),
			self::newHiddenPermissionMaskAttr('restrictedScriptPermissionMask', DMsg::ALbl('l_restrictedscriptpermissionmask'), $parseFormat, $parseHelp),
			self::newHiddenPermissionMaskAttr('restrictedDirPermissionMask', DMsg::ALbl('l_restricteddirpermissionmask'), $parseFormat, $parseHelp),
		];

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_fileaccess'), $attrs);
	}

	protected function add_S_SEC_CONN($id)
	{
		$attrs = array_merge($this->getThrottleAttrs(), [
			self::NewIntAttr('softLimit', DMsg::ALbl('l_softlimit'), true, 0),
			self::NewIntAttr('hardLimit', DMsg::ALbl('l_hardlimit'), true, 0),
			self::NewBoolAttr('blockBadReq', DMsg::ALbl('l_blockbadreq')),
			self::NewIntAttr('gracePeriod', DMsg::ALbl('l_graceperiod'), true, 1, 3600),
			self::NewIntAttr('banPeriod', DMsg::ALbl('l_banperiod'), true, 0)
		]);

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_perclientthrottle'), $attrs, 'perClientConnLimit');
	}

	protected function add_S_SEC_CGI($id)
	{
		$attrs = [
			self::NewTextAttr('cgidSock', DMsg::ALbl('l_cgidsock'), 'addr'),
			self::NewIntAttr('maxCGIInstances', DMsg::ALbl('l_maxCGIInstances'), true, 1, 2000),
			self::NewIntAttr('minUID', DMsg::ALbl('l_minuid'), true, 10),
			self::NewIntAttr('minGID', DMsg::ALbl('l_mingid'), true, 5),
			self::NewIntAttr('forceGID', DMsg::ALbl('l_forcegid'), true, 0),
			$this->_attrs['cgiUmask'],
			$this->_attrs['priority']->dup(null, DMsg::ALbl('l_cgipriority'), 'CGIPriority'),
			self::NewIntAttr('CPUSoftLimit', DMsg::ALbl('l_cpusoftlimit'), true, 0),
			self::NewIntAttr('CPUHardLimit', DMsg::ALbl('l_cpuhardlimit'), true, 0),
			$this->_attrs['memSoftLimit'],
			$this->_attrs['memHardLimit'],
			$this->_attrs['procSoftLimit'],
			$this->_attrs['procHardLimit'],
			self::NewSelAttr('cgroups', DMsg::ALbl('l_cgroups'), $this->_options['disable_off_on']),
		];

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_cgisettings'), $attrs, 'cgiResource');
	}

	protected function add_S_SEC_RECAP($id)
	{
		$parseFormat = '/^[A-z0-9\-_]{20,100}$/';
		$parseHelp = DMsg::ALbl('parse_recaptchakey');
		$botlist = self::NewTextAreaAttr('botWhiteList:list', DMsg::ALbl('l_botWhiteList'), 'cust', true, 5, 'recaptchaBotWhiteList', 0, 1);
		$botlist->SetFlag(DAttr::BM_RAWDATA);

		$attrs = array_merge($this->getRecaptchaCommonAttrs($parseFormat, $parseHelp), [
			self::NewIntAttr('allowedRobotHits', DMsg::ALbl('l_allowedRobotHits'), true, 0, 65535, 'recaptchaAllowedRobotHits'),
			$botlist,
			self::NewIntAttr('regConnLimit', DMsg::ALbl('l_regConnLimit'), true, 0, null, 'recaptchaRegConnLimit'),
			self::NewIntAttr('sslConnLimit', DMsg::ALbl('l_sslConnLimit'), true, 0, null, 'recaptchaSslConnLimit'),
		]);
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_lsrecaptcha'), $attrs, 'lsrecaptcha');
	}

	protected function add_VT_SEC_RECAP($id)
	{
		$parseFormat = '/^[A-z0-9\-_]{20,100}$/';
		$parseHelp = DMsg::ALbl('parse_recaptchakey');

		$attrs = array_merge($this->getRecaptchaCommonAttrs($parseFormat, $parseHelp), [
			self::NewIntAttr('regConnLimit', DMsg::ALbl('l_concurrentReqLimit'), true, 0, null, 'recaptchaVhReqLimit'),
		]);
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_lsrecaptcha'), $attrs, 'lsrecaptcha');
	}

	protected function add_S_SEC_BUBBLEWRAP($id)
	{
		$attrs = [
			self::NewSelAttr('bubbleWrap', DMsg::ALbl('l_bubblewrap'), $this->_options['disable_off_enable']),
			self::NewTextAreaAttr('bubbleWrapCmd', DMsg::ALbl('l_bubblewrapcmd'), 'cust', true, 3, null, 0),
			self::NewSelAttr('namespace', DMsg::ALbl('l_namespace'), $this->_options['disable_off_enable']),
			self::NewTextAttr('namespaceConf', DMsg::ALbl('l_namespaceConf'), 'cust'),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_containers'), $attrs);
	}

	protected function add_VT_SEC_BUBBLEWRAP($id)
	{
		$attrs = [
			self::NewSelAttr('bubbleWrap', DMsg::ALbl('l_bubblewrap'), $this->_options['notset_off_enable']),
			self::NewSelAttr('namespace', DMsg::ALbl('l_namespace'), $this->_options['notset_off_enable']),
			self::NewTextAttr('namespaceConfVhAdd', DMsg::ALbl('l_namespaceConfVhAdd'), 'cust'),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_containers'), $attrs);
	}

	protected function add_S_SEC_DENY($id)
	{
		$attrs = [
			self::NewTextAreaAttr('dir', null, 'cust', true, 15, 'accessDenyDir', 0, 1, 2)
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_accessdenydir'), $attrs, 'accessDenyDir', 1);
	}

	protected function add_A_SEC_AC($id)
	{
		$attrs = [
			self::NewTextAreaAttr('allow', DMsg::ALbl('l_accessallow'), 'subnet', true, 5, 'accessControl_allow', 0, 0, 1),
			self::NewTextAreaAttr('deny', DMsg::ALbl('l_accessdeny'), 'subnet', true, 5, 'accessControl_deny', 0, 0, 1)
		];

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_accesscontrol'), $attrs, 'accessControl', 1);
	}

	protected function add_A_HTACCESS($id)
	{
		$attrs = [
			$this->newAllowOverrideAttr(),
			self::NewParseTextAttr('accessFileName', DMsg::ALbl('l_accessfilename'), '/^[A-Za-z0-9._-]+$/', DMsg::ALbl('parse_accessfilename'))
		];

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_htaccess'), $attrs, 'htaccess');
	}

	protected function add_A_EXT_SEL($id)
	{
		$this->setExtAppSelTbl($id, $this->_options['extTbl']);
	}

	protected function add_T_EXT_SEL($id)
	{
		$this->setExtAppSelTbl($id, $this->_options['tp_extTbl']);
	}

	protected function add_A_EXT_TOP($id)
	{
		$align = ['left', 'left', 'left', 'center'];
		$attrs = $this->getExtAppTopAttrs($this->_options['extTbl']);
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_extapps'), $attrs, 'name', 'A_EXT_SEL', $align, null, 'package', true);
	}

	protected function add_A_EXT_FCGI($id)
	{
		$attrs = [
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
			self::NewIntAttr('extMaxIdleTime', DMsg::ALbl('l_maxidletime'), true, -1),
			$this->_attrs['priority']->dup(null, null, 'extAppPriority'),
			$this->_attrs['memSoftLimit'],
			$this->_attrs['memHardLimit'],
			$this->_attrs['procSoftLimit'],
			$this->_attrs['procHardLimit']
		];
		$defaultExtract = ['type' => 'fcgi'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_fcgiapp'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_A_EXT_FCGIAUTH($id)
	{
		$this->setDerivedExtAppTbl('A_EXT_FCGI', $id, DMsg::ALbl('l_extfcgiauth'), 'fcgiauth');
	}

	protected function add_A_EXT_LSAPI($id)
	{
		$this->setDerivedExtAppTbl('A_EXT_FCGI', $id, DMsg::ALbl('l_extlsapi'), 'lsapi');
	}

	protected function add_A_EXT_LOADBALANCER($id)
	{
		$parseFormat = '/^(lsapi|proxy|fcgi|fcgiauth|scgi||servlet|uwsgi)::.+$/';
		$parseHelp = 'ExtAppType::ExtAppName, allowed types are lsapi, proxy, fcgi, fcgiauth, scgi, servlet and uwsgi. e.g. fcgi::myphp, servlet::tomcat';

		$attrs = [$this->_attrs['ext_name'],
			self::NewParseTextAreaAttr('workers', DMsg::ALbl('l_workers'), $parseFormat, $parseHelp, true, 3, 'extWorkers', 0, 0, 1),
			$this->_attrs['note'],
		];
		$defaultExtract = ['type' => 'loadbalancer'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_extlb'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_A_EXT_LOGGER($id)
	{
		$attrs = [$this->_attrs['ext_name'],
			self::NewTextAttr('address', DMsg::ALbl('l_loggeraddress'), 'addr', true, 'loggerAddress'), //optional
			$this->_attrs['note'],
			$this->_attrs['ext_maxConns'],
			$this->_attrs['ext_env'],
			$this->_attrs['ext_path'],
			$this->_attrs['ext_instances'],
			$this->_attrs['ext_user'],
			$this->_attrs['ext_group'],
			$this->_attrs['cgiUmask'],
			$this->_attrs['priority']->dup(null, null, 'extAppPriority')
		];
		$defaultExtract = ['type' => 'logger'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_extlogger'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_A_EXT_SERVLET($id)
	{
		$attrs = $this->getExtNetworkAppAttrs($this->_attrs['ext_address']);
		$defaultExtract = ['type' => 'servlet'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_extservlet'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_A_EXT_SCGI($id)
	{
		$attrs = $this->getExtNetworkAppAttrs($this->_attrs['ext_address'], false);
		$defaultExtract = ['type' => 'scgi'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_extscgi'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_A_EXT_UWSGI($id)
	{
		$this->setDerivedExtAppTbl('A_EXT_SCGI', $id, DMsg::ALbl('l_extuwsgi'), 'uwsgi');
	}

	protected function add_A_EXT_PROXY($id)
	{
		$attrs = $this->getExtNetworkAppAttrs(
			self::NewTextAttr('address', DMsg::ALbl('l_address'), 'wsaddr', false, 'expWSAddress')
		);
		$defaultExtract = ['type' => 'proxy'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_extproxy'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_T_EXT_TOP($id)
	{
		$align = ['center', 'center', 'left', 'center'];
		$attrs = $this->getExtAppTopAttrs($this->_options['tp_extTbl']);
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_extapps'), $attrs, 'name', 'T_EXT_SEL', $align, null, 'package', true);
	}

	protected function add_T_EXT_FCGI($id)
	{
		$this->setTemplateExtTblDef('A_EXT_FCGI', $id);
	}

	protected function add_T_EXT_FCGIAUTH($id)
	{
		$this->setTemplateExtTblDef('A_EXT_FCGIAUTH', $id);
	}

	protected function add_T_EXT_LSAPI($id)
	{
		$this->setTemplateExtTblDef('A_EXT_LSAPI', $id);
	}

	protected function add_T_EXT_PROXY($id)
	{
		$this->setTemplateExtTblDef('A_EXT_PROXY', $id);
	}

	protected function add_T_EXT_SCGI($id)
	{
		$this->setTemplateExtTblDef('A_EXT_SCGI', $id);
	}

	protected function add_T_EXT_LOADBALANCER($id)
	{
		$this->setTemplateExtTblDef('A_EXT_LOADBALANCER', $id);
	}

	protected function add_T_EXT_LOGGER($id)
	{
		$this->setTemplateExtTblDef('A_EXT_LOGGER', $id);
	}

	protected function add_T_EXT_SERVLET($id)
	{
		$this->setTemplateExtTblDef('A_EXT_SERVLET', $id);
	}

	protected function add_T_EXT_UWSGI($id)
	{
		$this->setTemplateExtTblDef('A_EXT_UWSGI', $id);
	}

	protected function add_A_SCRIPT($id)
	{
		$attrs = $this->getScriptHandlerAttrs();
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_shdef'), $attrs, 'suffix');
	}

	protected function add_A_SCRIPT_TOP($id)
	{
		$align = ['center', 'center', 'center', 'center'];
		$attrs = $this->getScriptHandlerAttrs();
		$attrs[] = self::NewActionAttr('A_SCRIPT', 'Ed');
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_shdef'), $attrs, 'suffix', 'A_SCRIPT', $align, null, 'code');
	}

	protected function add_S_RAILS($id)
	{
		$attrs = $this->getAppServerDefaultAttrs(DMsg::ALbl('l_rubybin'), 'rubyBin');
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_railssettings'), $attrs, 'railsDefaults');
	}

	protected function add_S_WSGI($id)
	{
		$attrs = $this->getAppServerDefaultAttrs(DMsg::ALbl('l_wsgibin'), 'wsgiBin');
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_wsgisettings'), $attrs, 'wsgiDefaults');
	}

	protected function add_S_NODEJS($id)
	{
		$attrs = $this->getAppServerDefaultAttrs(DMsg::ALbl('l_nodebin'), 'nodeBin');
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_nodesettings'), $attrs, 'nodeDefaults');
	}

	protected function add_S_CACHE_STORAGE($id)
	{
		$cacheFeatures = self::NewCheckBoxAttr('cacheEngine', DMsg::ALbl('l_cachefeatures'), $this->_options['cacheEngine'], true, null, '');
		$cacheFeatures->_feature = 1;

		$cacheStorePath = self::NewTextAttr('storage:cacheStorePath', DMsg::ALbl('l_storagepath'), 'cust', true, 'cacheStorePath');
		$cacheStorePath->_feature = 1;

		$cacheMgrPath = self::NewTextAttr('storage:cacheMgrStorePath', DMsg::ALbl('l_cachemanagerpath'), 'cust', true, 'cacheMgrStorePath');
		$cacheMgrPath->_feature = 1;

		$pubStoreExpire = self::NewIntAttr('storage:pubStoreExpireMinutes', DMsg::ALbl('l_publicstorageexpireminutes'), true, 60, null, 'pubStoreExpireMinutes');
		$pubStoreExpire->_feature = 1;

		$purgeNoHitTimeout = self::NewIntAttr('storage:purgeNoHitTimeout', DMsg::ALbl('l_nohitexpireminutes'), true, 0, null, 'purgeNoHitTimeout');
		$purgeNoHitTimeout->_feature = 1;

		$attrs = [$cacheFeatures, $cacheStorePath, $cacheMgrPath, $pubStoreExpire, $purgeNoHitTimeout];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_cachestoragesettings'), $attrs, 'cache');
	}

	protected function add_S_CACHE_POLICY($id)
	{
		$attrs = $this->getCachePolicyAttrs(true);
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_cachepolicy'), $attrs, 'cache');
	}

	protected function add_A_CACHE_NCURL($id)
	{
		$noCacheUrl = self::NewTextAreaAttr('noCacheUrl', null, 'cust', true, 10, 'noCacheUrl', 1, 1);
		$noCacheUrl->_feature = 1;

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_donotcacheurl'), [$noCacheUrl], 'cache', 1);
	}

	protected function add_S_CACHE_NCDOMAIN($id)
	{
		$noCacheDomain = self::NewTextAreaAttr('noCacheDomain', null, 'cust', true, 10, 'noCacheDomain', 1, 1);
		$noCacheDomain->_feature = 1;

		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_donotcachedomains'), [$noCacheDomain], 'cache', 1);
	}

	protected function add_S_PAGESPEED($id)
	{
		$activate = self::applyLegacyFde(
			self::NewBoolAttr('activate', DMsg::ALbl('l_activatepagespeed'), true, 'pagespeedActivate'),
			'YNN'
		);
		$attrs = [
			$activate,
			$this->_attrs['enable_pagespeed'],
			$this->_attrs['pagespeed_setting']
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_pagespeedsettings'), $attrs, 'modpagespeed');
	}

	protected function add_V_PAGESPEED($id)
	{
		$attrs = [
			$this->_attrs['enable_pagespeed'],
			$this->_attrs['pagespeed_setting']
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_pagespeedsettings'), $attrs, 'modpagespeed');
	}

	protected function getModuleTags()
	{
		return ['L4_BEGINSESSION', 'L4_ENDSESSION', 'L4_RECVING', 'L4_SENDING',
			'HTTP_BEGIN', 'RECV_REQ_HEADER', 'URI_MAP', 'HTTP_AUTH',
			'RECV_REQ_BODY', 'RCVD_REQ_BODY', 'RECV_RESP_HEADER', 'RECV_RESP_BODY', 'RCVD_RESP_BODY',
			'HANDLER_RESTART', 'SEND_RESP_HEADER', 'SEND_RESP_BODY', 'HTTP_END',
			'MAIN_INITED', 'MAIN_PREFORK', 'MAIN_POSTFORK', 'WORKER_POSTFORK', 'WORKER_ATEXIT', 'MAIN_ATEXIT'];
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
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_servermodulesdef'), $attrs, 'name', 'S_MOD', $align, null, 'puzzle', true);
	}

	protected function add_S_MOD($id)
	{
		$attrs = [self::NewTextAttr('name', DMsg::ALbl('l_module'), 'modulename', false, 'modulename'),
			$this->_attrs['note'],
			self::NewBoolAttr('internal', DMsg::ALbl('l_internal'), true, 'internalmodule'),
			$this->_attrs['mod_params'],
			$this->_attrs['mod_enabled']
		];

		$hook = DMsg::ALbl('l_hook');
		$priority = DMsg::ALbl('l_priority');
		foreach ($this->getModuleTags() as $tag) {
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
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_vhmodules'), $attrs, 'name', 'VT_MOD', $align, 'vhModules', 'puzzle', true);
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

		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_urlfilter'), $attrs, 'uri', 'VT_MOD_FILTER', $align, 'vhModuleUrlFilters', 'filter', false);
		$this->_tblDef[$id]->Set(DTbl::FLD_SHOWPARENTREF, true);
	}

	protected function add_VT_MOD_FILTER($id)
	{
		$attrs = [$this->_attrs['ctx_uri'],
			$this->_attrs['mod_params'],
		];

		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_urlfilter'), $attrs, 'uri', 'vhModuleUrlFilters');
		$this->_tblDef[$id]->Set(DTbl::FLD_SHOWPARENTREF, true);
	}

	protected function add_L_MOD_TOP($id)
	{
		$align = ['center', 'center', 'center', 'center'];

		$attrs = [self::NewViewAttr('name', DMsg::ALbl('l_module')),
			$this->_attrs['mod_params'],
			$this->_attrs['mod_enabled']->dup(null, null, 'moduleEnabled_lst'),
			self::NewActionAttr('L_MOD', 'vEd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_listenermodules'), $attrs, 'name', 'L_MOD', $align, 'listenerModules', 'puzzle', true);
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

	protected function add_V_TOP($id)
	{
		$align = ['left', 'left', 'center'];

		$attrs = [
			self::NewViewAttr('name', DMsg::ALbl('l_name')),
			self::NewViewAttr('vhRoot', DMsg::ALbl('l_vhroot')),
			self::NewActionAttr('V_TOPD', 'Xd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_vhostlist'), $attrs, 'name', 'V_TOPD', $align, null, 'server', true)->enableTableControls(20);
	}

	protected function add_V_BASE($id)
	{
		$attrs = [
			self::NewTextAttr('name', DMsg::ALbl('l_vhname'), 'vhname', false, 'vhName'),
			self::NewTextAttr('vhRoot', DMsg::ALbl('l_vhroot'), 'cust', false), // do not check path for vhroot, it may be different owner
			self::NewPathAttr('configFile', DMsg::ALbl('l_configfile'), 'filevh', 3, 'rwc', false),
			$this->_attrs['note']
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_vhostregistration'), $attrs, 'name', 'vhBase');
	}

	protected function add_V_BASE_CONN($id)
	{
		$attrs = [
			$this->_attrs['vh_maxKeepAliveReq'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_connection'), $attrs, 'name');
	}

	protected function add_V_BASE_THROTTLE($id)
	{
		$attrs = $this->getThrottleAttrs();
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_perclientthrottle'), $attrs, 'name');
	}

	protected function add_L_TOP($id)
	{
		$align = ['center', 'center', 'center', 'center', 'center'];

		$attrs = [
			self::NewViewAttr('name', DMsg::ALbl('l_listenername')),
			self::NewViewAttr('ip', DMsg::ALbl('l_ip')),
			self::NewViewAttr('port', DMsg::ALbl('l_port')),
			self::NewBoolAttr('secure', DMsg::ALbl('l_secure')),
			self::NewActionAttr('L_GENERAL', 'Xd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_listenerlist'), $attrs, 'name', 'L_GENERAL', $align, null, 'plug', true)
			->enableTableControls(10);
	}

	protected function add_L_GENERAL($id)
	{
		$ip = self::NewSelAttr('ip', DMsg::ALbl('l_ip'), $this->_options['ip'], false, 'listenerIP');
		$ip->SetFlag(DAttr::BM_NOFILE);
		$port = self::NewIntAttr('port', DMsg::ALbl('l_port'), false, 0, 65535, 'listenerPort');
		$port->SetFlag(DAttr::BM_NOFILE);

		$bindOptions = [];
		$processes = isset($_SERVER['LSWS_CHILDREN']) ? $_SERVER['LSWS_CHILDREN'] : 1;
		for ($i = 1; $i <= $processes; ++$i) {
			$bindOptions[1 << ($i - 1)] = 'Process ' . $i;
		}

		$attrs = [
			self::NewTextAttr('name', DMsg::ALbl('l_listenername'), 'name', false, 'listenerName'),
			self::NewCustFlagAttr('address', DMsg::ALbl('l_address'), (DAttr::BM_HIDE | DAttr::BM_NOEDIT), false),
			$ip,
			$port,
			self::NewCheckBoxAttr('binding', DMsg::ALbl('l_binding'), $bindOptions, true, 'listenerBinding'),
			self::NewBoolAttr('reusePort', DMsg::ALbl('l_reuseport')),
			self::NewBoolAttr('secure', DMsg::ALbl('l_secure'), false, 'listenerSecure'),
			$this->_attrs['note'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_addresssettings'), $attrs, 'name');
	}

	protected function add_ADM_L_TOP($id)
	{
		$align = ['center', 'center', 'center', 'center', 'center'];

		$attrs = [
			self::NewViewAttr('name', DMsg::ALbl('l_listenername')),
			self::NewViewAttr('ip', DMsg::ALbl('l_ip')),
			self::NewViewAttr('port', DMsg::ALbl('l_port')),
			self::NewBoolAttr('secure', DMsg::ALbl('l_secure')),
			self::NewActionAttr('ADM_L_GENERAL', 'Xd', false)//cannot delete all
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_listenerlist'), $attrs, 'name', 'ADM_L_GENERAL', $align, null, 'plug', true);
	}

	protected function add_ADM_L_GENERAL($id)
	{
		$name = self::NewTextAttr('name', DMsg::ALbl('l_listenername'), 'name', false, 'listenerName');
		$addr = self::NewCustFlagAttr('address', DMsg::ALbl('l_address'), (DAttr::BM_HIDE | DAttr::BM_NOEDIT), false);
		$ip = self::NewSelAttr('ip', DMsg::ALbl('l_ip'), $this->_options['ip'], false, 'listenerIP');
		$ip->SetFlag(DAttr::BM_NOFILE);
		$port = self::NewIntAttr('port', DMsg::ALbl('l_port'), false, 0, 65535, 'listenerPort');
		$port->SetFlag(DAttr::BM_NOFILE);

		$attrs = [
			$name,
			$addr, $ip, $port,
			self::NewBoolAttr('secure', DMsg::ALbl('l_secure'), false, 'listenerSecure'),
			$this->_attrs['note'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_adminlistenersettings'), $attrs, 'name');
	}

	protected function add_L_VHMAP($id)
	{
		$attrs = [
			$this->_attrs['l_vhost'],
			$this->_attrs['l_domain']
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_vhmappings'), $attrs, 'vhost', 'virtualHostMapping');
	}

	protected function add_L_VHMAP_TOP($id)
	{
		$align = ['left', 'left', 'center'];

		$attrs = [
			$this->_attrs['l_vhost'],
			$this->_attrs['l_domain'],
			self::NewActionAttr('L_VHMAP', 'Ed')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_vhmappings'), $attrs, 'vhost', 'L_VHMAP', $align, 'virtualHostMapping', 'plug-zap', false)
			->enableTableControls(20);
	}

	protected function add_LVT_SSL_CERT($id)
	{
		$attrs = [
			self::NewTextAttr('keyFile', DMsg::ALbl('l_keyfile'), 'cust'),
			self::NewTextAttr('certFile', DMsg::ALbl('l_certfile'), 'cust'),
			self::NewBoolAttr('certChain', DMsg::ALbl('l_certchain')),
			self::NewTextAttr('CACertPath', DMsg::ALbl('l_cacertpath'), 'cust'),
			self::NewTextAttr('CACertFile', DMsg::ALbl('l_cacertfile'), 'cust'),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_ssl'), $attrs, 'sslCert');
	}

	protected function add_L_SSL($id)
	{
		$attrs = self::getSslProtocolAttrs();
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_sslprotocol'), $attrs);
	}

	protected function add_VT_SSL($id)
	{
		$attrs = self::getSslProtocolAttrs(false);
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_sslprotocol'), $attrs);
	}

	protected function add_L_SSL_FEATURE($id)
	{
		$attrs = $this->getSslFeatureAttrs(DMsg::ALbl('l_openudpport'), 'allowQuic');
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_securityandfeatures'), $attrs);
	}

	protected function add_VT_SSL_FEATURE($id)
	{
		$attrs = $this->getSslFeatureAttrs(DMsg::ALbl('l_enablequic'), 'vhEnableQuic');
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::UIStr('tab_sec'), $attrs);
	}

	protected function add_LVT_SSL_OCSP($id)
	{
		$attrs = [
			self::NewBoolAttr('enableStapling', DMsg::ALbl('l_enablestapling')),
			self::NewIntAttr('ocspRespMaxAge', DMsg::ALbl('l_ocsprespmaxage'), true, -1),
			self::NewTextAttr('ocspResponder', DMsg::ALbl('l_ocspresponder'), 'httpurl'),
			self::NewTextAttr('ocspCACerts', DMsg::ALbl('l_ocspcacerts'), 'cust'),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_ocspstapling'), $attrs, 'sslOCSP');
	}

	protected function add_LVT_SSL_CLVERIFY($id)
	{
		$attrs = [
			self::NewSelAttr('clientVerify', DMsg::ALbl('l_clientverify'), [
				'0' => 'none',
				'1' => 'optional',
				'2' => 'require',
				'3' => 'optional_no_ca'
			]),
			self::NewIntAttr('verifyDepth', DMsg::ALbl('l_verifydepth'), true, 0, 100),
			self::NewTextAttr('crlPath', DMsg::ALbl('l_crlpath'), 'cust'),
			self::NewTextAttr('crlFile', DMsg::ALbl('l_crlfile'), 'cust'),
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_clientverify'), $attrs);
	}

	protected function add_T_TOP($id)
	{
		$align = ['left', 'left', 'center'];

		$attrs = [
			self::NewViewAttr('name', DMsg::ALbl('l_name')),
			self::NewViewAttr('listeners', DMsg::ALbl('l_mappedlisteners')),
			self::NewActionAttr('T_TOPD', 'Xd')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_tplist'), $attrs, 'name', 'T_TOPD', $align, null, 'form', true);
	}

	protected function add_T_TOPD($id)
	{
		$attrs = [
			self::NewTextAttr('name', DMsg::ALbl('l_tpname'), 'vhname', false, 'templateName'),
			$this->_attrs['tp_templateFile'],
			$this->_attrs['tp_listeners'],
			$this->_attrs['note']
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_vhtemplate'), $attrs, 'name');
	}

	protected function add_T_MEMBER_TOP($id)
	{
		$align = ['left', 'left', 'center'];

		$attrs = [
			$this->_attrs['tp_vhName'],
			$this->_attrs['tp_vhDomain'],
			self::NewActionAttr('T_MEMBER', 'vEdi')
		];

		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_membervhosts'), $attrs, 'vhName', 'T_MEMBER', $align, null, 'server', false);
	}

	protected function add_T_MEMBER($id)
	{
		$vhroot = self::NewTextAttr('vhRoot', DMsg::ALbl('l_membervhroot'), 'cust', true, 'memberVHRoot');
		$vhroot->_note = DMsg::ALbl('l_membervhroot_note');

		$attrs = [
			$this->_attrs['tp_vhName'],
			$this->_attrs['tp_vhDomain'],
			$this->_attrs['tp_vhAliases'],
			$vhroot
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_membervhosts'), $attrs, 'vhName');
	}

	protected function add_V_LOG($id)
	{
		$attrs = [
			self::NewBoolAttr('useServer', DMsg::ALbl('l_useServer'), false, 'logUseServer'),
			$this->_attrs['fileName3']->dup(null, null, 'vhlog_fileName'),
			self::NewSelAttr('logLevel', DMsg::ALbl('l_loglevel'), $this->_options['logLevel'], true, 'vhlog_logLevel'),
			$this->_attrs['rollingSize'],
			$this->_attrs['keepDays'],
			$this->_attrs['compressArchive'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_vhlog'), $attrs, 'fileName');
	}

	protected function add_V_ACLOG_TOP($id)
	{
		$attrs = $this->getAccessLogTopAttrs(
			$this->_attrs['fileName3']->dup(null, null, 'accessLog_fileName'),
			'V_ACLOG'
		);
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_accesslog'), $attrs, 'fileName', 'V_ACLOG');
	}

	protected function add_V_ACLOG($id)
	{
		$attrs = $this->getAccessLogAttrs($this->_attrs['fileName3']->dup(null, null, 'vhaccessLog_fileName'));
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_accesslog'), $attrs, 'fileName');
	}

	protected function add_VT_INDXF($id)
	{
		$attrs = [
			self::NewSelAttr('useServer', DMsg::ALbl('l_useserverindexfiles'), [0 => DMsg::ALbl('o_no'), 1 => DMsg::ALbl('o_yes'), 2 => 'Addition'], false, 'indexUseServer'),
			$this->_attrs['indexFiles'],
			$this->_attrs['autoIndex'],
			self::NewTextAttr('autoIndexURI', DMsg::ALbl('l_autoindexuri'), 'uri')
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_indexfiles'), $attrs);
	}

	protected function get_cust_status_code()
	{
		$status = [
			300 => 'Multiple Choices',
			301 => 'Moved Permanently',
			302 => 'Found',
			303 => 'See Other',
			305 => 'Use Proxy',
			307 => 'Temporary Redirect',
			400 => 'Bad Request',
			401 => 'Unauthorized',
			402 => 'Payment Required',
			403 => 'Forbidden',
			404 => 'Not Found',
			405 => 'Method Not Allowed',
			406 => 'Not Acceptable',
			407 => 'Proxy Authentication Required',
			408 => 'Request Time-out',
			409 => 'Conflict',
			410 => 'Gone',
			411 => 'Length Required',
			412 => 'Precondition Failed',
			413 => 'Request Entity Too Large',
			414 => 'Request-URI Too Large',
			415 => 'Unsupported Media Type',
			416 => 'Requested range not satisfiable',
			417 => 'Expectation Failed',
			500 => 'Internal Server Error',
			501 => 'Not Implemented',
			502 => 'Bad Gateway',
			503 => 'Service Unavailable',
			504 => 'Gateway Time-out',
			505 => 'HTTP Version not supported'
		];
		$options = [];
		foreach ($status as $key => $value) {
			$options[$key] = "$key  $value";
		}

		return $options;
	}

	protected function add_VT_ERRPG_TOP($id)
	{
		$align = ['left', 'left', 'center'];
		$errCodeOptions = $this->get_cust_status_code();
		$attrs = [
			self::NewSelAttr('errCode', DMsg::ALbl('l_errcode'), $errCodeOptions, false),
			self::NewViewAttr('url', DMsg::ALbl('l_url')),
			self::NewActionAttr('VT_ERRPG', 'Ed')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_custerrpages'), $attrs, 'errCode', 'VT_ERRPG', $align, 'errPage', 'file-text', true);
	}

	protected function add_VT_ERRPG($id)
	{
		$attrs = [
			self::NewSelAttr('errCode', DMsg::ALbl('l_errcode'), $this->get_cust_status_code(), false),
			self::NewTextAttr('url', DMsg::ALbl('l_url'), 'cust', false, 'errURL'),
			$this->_attrs['note'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_custerrpages'), $attrs, 'errCode', 'errPage');
	}

	protected function get_realm_attrs()
	{
		return [
			'realm_type' => self::NewSelAttr('type', DMsg::ALbl('l_realmtype'), $this->_options['realmType'], false, 'realmType'),
			'realm_name' => self::NewTextAttr('name', DMsg::ALbl('l_realmname'), 'vhname', false, 'realmName'),
			'realm_udb_maxCacheSize' => self::NewIntAttr('userDB:maxCacheSize', DMsg::ALbl('l_userdbmaxcachesize'), true, 0, '100K', 'userDBMaxCacheSize'),
			'realm_udb_cacheTimeout' => self::NewIntAttr('userDB:cacheTimeout', DMsg::ALbl('l_userdbcachetimeout'), true, 0, 3600, 'userDBCacheTimeout'),
			'realm_gdb_maxCacheSize' => self::NewIntAttr('groupDB:maxCacheSize', DMsg::ALbl('l_groupdbmaxcachesize'), true, 0, '100K', 'groupDBMaxCacheSize'),
			'realm_gdb_cacheTimeout' => self::NewIntAttr('groupDB:cacheTimeout', DMsg::ALbl('l_groupdbcachetimeout'), true, 0, 3600, 'groupDBCacheTimeout')
		];
	}

	protected function add_V_REALM_FILE($id)
	{
		$udbLoc = self::NewPathAttr('userDB:location', DMsg::ALbl('l_userdblocation'), 'file', 3, 'rwc', false, 'userDBLocation');
		$udbLoc->_href = '&t1=V_UDB_TOP&r1=$R';
		$gdbLoc = self::NewPathAttr('groupDB:location', DMsg::ALbl('l_groupdblocation'), 'file', 3, 'rwc', true, 'GroupDBLocation');
		$gdbLoc->_href = '&t1=V_GDB_TOP&r1=$R';

		$realm_attr = $this->get_realm_attrs();
		$attrs = [
			$realm_attr['realm_name'],
			$this->_attrs['note'],
			$udbLoc,
			$realm_attr['realm_udb_maxCacheSize'],
			$realm_attr['realm_udb_cacheTimeout'],
			$gdbLoc,
			$realm_attr['realm_gdb_maxCacheSize'],
			$realm_attr['realm_gdb_cacheTimeout']
		];
		$defaultExtract = ['type' => 'file'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_passfilerealmdef'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_T_REALM_FILE($id)
	{
		$realm_attr = $this->get_realm_attrs();
		$attrs = [
			$realm_attr['realm_name'],
			$this->_attrs['note'],
			self::NewTextAttr('userDB:location', DMsg::ALbl('l_userdblocation'), 'cust', false, 'userDBLocation'),
			$realm_attr['realm_udb_maxCacheSize'],
			$realm_attr['realm_udb_cacheTimeout'],
			self::NewTextAttr('groupDB:location', DMsg::ALbl('l_groupdblocation'), 'cust', true, 'GroupDBLocation'),
			$realm_attr['realm_gdb_maxCacheSize'],
			$realm_attr['realm_gdb_cacheTimeout']
		];
		$defaultExtract = ['type' => 'file'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_passfilerealmdef'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_V_UDB_TOP($id)
	{
		$align = ['left', 'left', 'center'];

		$attrs = [
			self::NewViewAttr('name', DMsg::ALbl('l_username')),
			self::NewViewAttr('group', DMsg::ALbl('l_groups')),
			self::NewActionAttr('V_UDB', 'Ed')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_userdbentries'), $attrs, 'name', 'V_UDB', $align, null, 'user', false);
		$this->_tblDef[$id]->Set(DTbl::FLD_SHOWPARENTREF, true);
	}

	protected function add_V_UDB($id)
	{
		$attrs = [
			self::NewTextAttr('name', DMsg::ALbl('l_username'), 'name', false, 'UDBusername'),
			self::NewTextAttr('group', DMsg::ALbl('l_groups'), 'name', true, 'UDBgroup', 1),
			self::NewPassAttr('pass', DMsg::ALbl('l_newpass'), false, 'UDBpass'),
			self::NewPassAttr('pass1', DMsg::ALbl('l_retypepass'), false, 'UDBpass1')
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_userdbentry'), $attrs, 'name');
		$this->_tblDef[$id]->Set(DTbl::FLD_SHOWPARENTREF, true);
	}

	protected function add_V_GDB_TOP($id)
	{
		$align = ['left', 'left', 'center'];

		$attrs = [
			self::NewViewAttr('name', DMsg::ALbl('l_groupname')),
			self::NewViewAttr('users', DMsg::ALbl('l_users')),
			self::NewActionAttr('V_GDB', 'Ed')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_groupdbentries'), $attrs, 'name', 'V_GDB', $align);
		$this->_tblDef[$id]->Set(DTbl::FLD_SHOWPARENTREF, true);
	}

	protected function add_V_GDB($id)
	{
		$users = self::NewTextAreaAttr('users', DMsg::ALbl('l_users'), 'name', true, 15, 'gdb_users', 0, 0, 1);
		$users->SetGlue(' ');

		$attrs = [
			self::NewTextAttr('name', DMsg::ALbl('l_groupname'), 'vhname', false, 'gdb_groupname'),
			$users,
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_groupdbentry'), $attrs, 'name');
		$this->_tblDef[$id]->Set(DTbl::FLD_SHOWPARENTREF, true);
	}

	protected function add_VT_CTX_SEL($id)
	{
		$attrs = [$this->_attrs['ctx_type']];
		$this->_tblDef[$id] = DTbl::NewSel($id, DMsg::ALbl('l_newcontext'), $attrs, $this->_options['ctxTbl']);
	}

	protected function get_ctx_attrs($type)
	{
		if ($type == 'auth') {
			return [
				self::NewSelAttr('realm', DMsg::ALbl('l_realm'), 'realm'),
				self::NewTextAttr('authName', DMsg::ALbl('l_authname'), 'name'),
				self::NewTextAttr('required', DMsg::ALbl('l_requiredauthuser'), 'cust'),
				self::NewTextAreaAttr('accessControl:allow', DMsg::ALbl('l_accessallowed'), 'subnet', true, 3, 'accessAllowed', 0, 0, 1),
				self::NewTextAreaAttr('accessControl:deny', DMsg::ALbl('l_accessdenied'), 'subnet', true, 3, 'accessDenied', 0, 0, 1),
				self::NewSelAttr('authorizer', DMsg::ALbl('l_authorizer'), 'extprocessor:fcgiauth', true, 'extAuthorizer')
			];
		}
		if ($type == 'rewrite') {
			$rules = self::NewTextAreaAttr('rewrite:rules', DMsg::ALbl('l_rewriterules'), 'cust', true, 6, 'rewriteRules', 1, 1);
			$rules->SetFlag(DAttr::BM_RAWDATA);
			return [
				self::NewBoolAttr('rewrite:enable', DMsg::ALbl('l_enablerewrite'), true, 'enableRewrite'),
				self::NewBoolAttr('rewrite:inherit', DMsg::ALbl('l_rewriteinherit'), true, 'rewriteInherit'),
				self::NewTextAttr('rewrite:base', DMsg::ALbl('l_rewritebase'), 'uri', true, 'rewriteBase'),
				$rules,
			];
		}
		if ($type == 'charset') {
			return [
				self::NewTextAttr('addDefaultCharset', DMsg::ALbl('l_adddefaultcharset'), 'charset'),
				$this->_attrs['enableIpGeo']
			];
		}
		if ($type == 'uri') {
			return [
				$this->_attrs['ctx_uri'],
				$this->_attrs['ctx_order']
			];
		}
	}

	protected function add_VT_WBSOCK_TOP($id)
	{
		$align = ['left', 'left', 'center'];

		$attrs = [
			self::NewViewAttr('uri', DMsg::ALbl('l_uri')),
			self::NewViewAttr('address', DMsg::ALbl('l_address')),
			self::NewActionAttr('VT_WBSOCK', 'Ed')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_websocketsetup'), $attrs, 'uri', 'VT_WBSOCK', $align, null, 'cable', true);
	}

	protected function add_VT_WBSOCK($id)
	{
		$attrs = [
			$this->_attrs['ctx_uri']->dup(null, null, 'wsuri'),
			$this->_attrs['ext_address']->dup(null, null, 'wsaddr'),
			$this->_attrs['note'],
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_websocketdef'), $attrs, 'uri');
	}

	protected function add_T_GENERAL1($id)
	{
		$attrs = [
			$this->_attrs['tp_vhRoot'],
			self::NewParseTextAttr('configFile', DMsg::ALbl('l_instantiatedvhconfigfile'), '/\$VH_NAME.*\.conf$/', DMsg::ALbl('parse_tpvhconffile'), false, 'templateVHConfigFile'),
			$this->_attrs['vh_maxKeepAliveReq'],
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_templatemembersettings'), $attrs, 'templateMemberSettings');
	}

	protected function add_T_SEC_FILE($id)
	{
		$attrs = [
			$this->_attrs['vh_allowSymbolLink'],
			$this->_attrs['vh_enableScript'],
			$this->_attrs['vh_restrained']
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_fileaccesscontrol'), $attrs);
	}

	protected function add_T_SEC_CONN($id)
	{
		$attrs = $this->getThrottleAttrs();
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_perclientthrottle'), $attrs);
	}

	protected function add_T_SEC_CGI($id)
	{
		$attrs = [
			$this->_attrs['vh_setUIDMode'],
			$this->_attrs['vh_suexec_user'],
			$this->_attrs['vh_suexec_group'],
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_extappsec'), $attrs);
	}

	protected function add_T_LOG($id)
	{
		$this->setTemplateFileNameTblDef('V_LOG', $id);
	}

	protected function add_T_ACLOG_TOP($id)
	{
		$attrs = $this->getAccessLogTopAttrs($this->_attrs['tp_vrFile'], 'T_ACLOG');
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_accesslog'), $attrs, 'fileName', 'T_ACLOG');
	}

	protected function add_T_ACLOG($id)
	{
		$this->setTemplateFileNameTblDef('V_ACLOG', $id);
	}

	protected function add_ADM_PHP($id)
	{
		$attrs = [
			self::NewBoolAttr('enableCoreDump', DMsg::ALbl('l_enablecoredump'), false),
			self::NewIntAttr('sessionTimeout', DMsg::ALbl('l_sessiontimeout'), true, 60, null, 'consoleSessionTimeout')
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::UIStr('tab_g'), $attrs);
	}

	protected function add_ADM_THROTTLE($id)
	{
		$attrs = [
			self::NewBoolAttr('throttleEnabled', DMsg::ALbl('l_throttleenabled'), false),
			self::NewIntAttr('throttleMaxFailures', DMsg::ALbl('l_throttlemaxfailures'), true, 1, 10),
			self::NewIntAttr('throttleBlockWindow', DMsg::ALbl('l_throttleblockwindow'), true, 60, 86400),
			self::NewIntAttr('throttleMaxBackoff', DMsg::ALbl('l_throttlemaxbackoff'), true, 60, 86400),
			self::NewIntAttr('loginHistoryRetention', DMsg::ALbl('l_loginhistoryretention'), true, 1, 365),
			self::NewIntAttr('opsAuditRetainFiles', DMsg::ALbl('l_opsauditretainfiles'), true, 1, 100)
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_loginthrottle'), $attrs);
	}

	protected function add_ADM_USR_TOP($id)
	{
		$align = ['left', 'center'];
		$attrs = [
			self::NewViewAttr('name', DMsg::ALbl('l_username')),
			self::NewActionAttr('ADM_USR', 'Ed', false) //not allow null - cannot delete all
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_adminusers'), $attrs, 'name', 'ADM_USR_NEW', $align, null, 'user');
	}

	protected function add_ADM_USR($id)
	{
		$attrs = [
			self::NewTextAttr('name', DMsg::ALbl('l_username'), 'admname', false, 'adminUserName'),
			self::NewPassAttr('oldpass', DMsg::ALbl('l_oldpass'), false, 'adminOldPass'),
			self::NewPassAttr('pass', DMsg::ALbl('l_newpass'), false, 'adminNewPass'),
			self::NewPassAttr('pass1', DMsg::ALbl('l_retypepass'), false, 'adminRetypePass')
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_adminuser'), $attrs, 'name');
	}

	protected function add_ADM_USR_NEW($id)
	{
		$attrs = [
			self::NewTextAttr('name', DMsg::ALbl('l_username'), 'admname', false, 'adminUserName'),
			self::NewPassAttr('pass', DMsg::ALbl('l_newpass'), false, 'adminNewPass'),
			self::NewPassAttr('pass1', DMsg::ALbl('l_retypepass'), false, 'adminRetypePass')
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_newadminuser'), $attrs, 'name');
	}

	protected function add_ADM_ACLOG($id)
	{
		$attrs = $this->getAccessLogAttrs($this->_attrs['fileName3']->dup(null, null, 'accessLog_fileName'));
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_accesslog'), $attrs, 'fileName');
	}

	protected function add_S_MIME_TOP($id)
	{
		$align = ['left', 'left', 'center'];

		$attrs = [
			self::NewViewAttr('suffix', DMsg::ALbl('l_suffix'), 'mimesuffix'),
			self::NewViewAttr('type', DMsg::ALbl('l_mimetype')),
			self::NewActionAttr('S_MIME', 'Ed')
		];
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_mimetypedef'), $attrs, 'suffix', 'S_MIME', $align, null, 'file-text')
			->enableClientFilter();
	}

	protected function add_S_MIME($id)
	{
		$attrs = [
			$this->_attrs['suffix']->dup('suffix', DMsg::ALbl('l_suffix'), 'mimesuffix'),
			self::NewParseTextAttr('type', DMsg::ALbl('l_mimetype'), '/^[A-z0-9_\-\.\+]+\/[A-z0-9_\-\.\+]+(\s*;?.*)$/', DMsg::ALbl('parse_mimetype'), false, 'mimetype')
		];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_mimetypeentry'), $attrs, 'suffix');
	}

	protected function add_SERVICE_SUSPENDVH($id)
	{
		$attrs = [
			self::NewCustFlagAttr('suspendedVhosts', null, (DAttr::BM_HIDE | DAttr::BM_NOEDIT), true, 'vhname', null, null, 1)
		];
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_suspendvh'), $attrs);
	}

}
