<?php

namespace LSWebAdmin\Product\WebServer;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Base\DTblDefBase as ProductDTblDefBase;
use LSWebAdmin\Product\Current\DAttr;
use LSWebAdmin\UI\DTbl;

class DTblDefBase extends ProductDTblDefBase
{
	protected function loadCommonOptions()
	{
		parent::loadCommonOptions();

		$this->_options['disable_off_enable'] = ['0' => DMsg::ALbl('o_disabled'), '1' => DMsg::ALbl('o_off'), '2' => DMsg::ALbl('l_enabled')];
		$this->_options['notset_off_enable'] = ['' => DMsg::ALbl('o_notset'), '1' => DMsg::ALbl('o_off'), '2' => DMsg::ALbl('l_enabled')];
		$this->_options['appserverEnv'] = $this->getSharedAppServerEnvOptions();
		$this->_options['scriptHandler'] = $this->getSharedScriptHandlerOptions();

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

		$this->_options['extType'] = [
			'lsapi' => DMsg::ALbl('l_extlsapi'),
			'proxy' => DMsg::ALbl('l_extproxy'),
			'fcgi' => DMsg::ALbl('l_fcgiapp'),
			'fcgiauth' => DMsg::ALbl('l_extfcgiauth'),
			'scgi' => DMsg::ALbl('l_extscgi'),
			'servlet' => DMsg::ALbl('l_extservlet'),
			'logger' => DMsg::ALbl('l_extlogger'),
			'loadbalancer' => DMsg::ALbl('l_extlb'),
			'uwsgi' => DMsg::ALbl('l_extuwsgi'),
		];

		$this->_options['sv_extTbl'] = [
			0 => 'type',
			1 => 'SV_EXT_LSAPI',
			'lsapi' => 'SV_EXT_LSAPI',
			'proxy' => 'SV_EXT_PROXY',
			'fcgi' => 'SV_EXT_FCGI',
			'fcgiauth' => 'SV_EXT_FCGIAUTH',
			'scgi' => 'SV_EXT_SCGI',
			'servlet' => 'SV_EXT_SERVLET',
			'logger' => 'SV_EXT_LOGGER',
			'loadbalancer' => 'SV_EXT_LOADBALANCER',
			'uwsgi' => 'SV_EXT_UWSGI',
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
	}

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

	protected function getScriptHandlerAttrs()
	{
		return [
			$this->_attrs['suffix'],
			$this->_attrs['scriptHandler_type'],
			$this->_attrs['scriptHandler'],
		];
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

	protected function loadCommonAttrs()
	{
		parent::loadCommonAttrs();

		$forbidden_ext_groups = ['root', 'sudo', 'wheel', 'shadow', 'lsadm'];
		$forbidden_ext_users = ['root', 'lsadm'];

		$this->_attrs['scriptHandler_type'] = self::NewSelAttr('type', DMsg::ALbl('l_handlertype'), $this->_options['scriptHandler'], false, 'shType', 'data-lst-change-call="lst_conf" data-lst-change-arg="c"');
		$this->_attrs['scriptHandler'] = self::NewSelAttr('handler', DMsg::ALbl('l_handlername'), 'extprocessor:$$type', false, 'shHandlerName');
		$this->_attrs['ext_type'] = self::NewSelAttr('type', DMsg::ALbl('l_type'), $this->_options['extType'], false, 'extAppType');
		$this->_attrs['ext_name'] = self::NewTextAttr('name', DMsg::ALbl('l_name'), 'name', false, 'extAppName');
		$this->_attrs['ext_address'] = self::NewTextAttr('address', DMsg::ALbl('l_address'), 'addr', false, 'extAppAddress');
		$this->_attrs['ext_maxConns'] = self::NewIntAttr('maxConns', DMsg::ALbl('l_maxconns'), false, 1, 2000);
		$this->_attrs['ext_env'] = self::NewParseTextAreaAttr('env', DMsg::ALbl('l_env'), '/\S+=\S+/', DMsg::ALbl('parse_env'), true, 5, null, 0, 1, 2);
		$this->_attrs['ext_initTimeout'] = self::NewIntAttr('initTimeout', DMsg::ALbl('l_inittimeout'), false, 1);
		$this->_attrs['ext_retryTimeout'] = self::NewIntAttr('retryTimeout', DMsg::ALbl('l_retrytimeout'), false, 0);
		$this->_attrs['ext_respBuffer'] = self::NewSelAttr('respBuffer', DMsg::ALbl('l_respbuffer'), ['0' => DMsg::ALbl('o_no'), '1' => DMsg::ALbl('o_yes'), '2' => DMsg::ALbl('o_nofornph')], false);
		$this->_attrs['ext_persistConn'] = self::NewBoolAttr('persistConn', DMsg::ALbl('l_persistconn'));
		$this->_attrs['ext_path'] = self::NewPathAttr('path', DMsg::ALbl('l_command'), 'file1', 3, 'x', true, 'extAppPath');
		$this->_attrs['ext_backlog'] = self::NewIntAttr('backlog', DMsg::ALbl('l_backlog'), true, 1, 100);
		$this->_attrs['ext_instances'] = self::NewIntAttr('instances', DMsg::ALbl('l_instances'), true, 0, 1000);
		$this->_attrs['ext_runOnStartUp'] = self::NewSelAttr('runOnStartUp', DMsg::ALbl('l_runonstartup'), ['' => '', '1' => DMsg::ALbl('o_yes'), '3' => DMsg::ALbl('o_yesdetachmode'), '2' => DMsg::ALbl('o_yesdaemonmode'), '0' => DMsg::ALbl('o_no')]);
		$this->_attrs['ext_user'] = self::NewParseTextAttr('extUser', DMsg::ALbl('l_suexecuser'), '/^(?!(?:' . implode('|', $forbidden_ext_users) . ")\\b)/", null);
		$this->_attrs['ext_group'] = self::NewParseTextAttr('extGroup', DMsg::ALbl('l_suexecgrp'), '/^(?!(?:' . implode('|', $forbidden_ext_groups) . ")\\b)/", null);
		$this->_attrs['appserverEnv'] = self::NewSelAttr('appserverEnv', DMsg::ALbl('l_runtimemode'), $this->_options['appserverEnv']);
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

	protected function getExtAppTopAttrs($actionTbls)
	{
		return [
			$this->_attrs['ext_type'],
			self::NewViewAttr('name', DMsg::ALbl('l_name')),
			self::NewViewAttr('address', DMsg::ALbl('l_address')),
			self::NewActionAttr($actionTbls, 'vEd')
		];
	}

	protected function add_SV_EXT_SEL($id)
	{
		$attrs = [$this->_attrs['ext_type']];
		$this->_tblDef[$id] = DTbl::NewSel($id, DMsg::ALbl('l_newextapp'), $attrs, $this->_options['sv_extTbl']);
	}

	protected function add_T_EXT_SEL($id)
	{
		$attrs = [$this->_attrs['ext_type']];
		$this->_tblDef[$id] = DTbl::NewSel($id, DMsg::ALbl('l_newextapp'), $attrs, $this->_options['tp_extTbl']);
	}

	protected function add_SV_EXT_TOP($id)
	{
		$align = ['left', 'left', 'left', 'center'];
		$attrs = $this->getExtAppTopAttrs($this->_options['sv_extTbl']);
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_extapps'), $attrs, 'name', 'SV_EXT_SEL', $align, null, 'package', true);
	}

	protected function add_SV_EXT_FCGI($id)
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

	protected function add_SV_EXT_FCGIAUTH($id)
	{
		$this->setDerivedExtAppTbl('SV_EXT_FCGI', $id, DMsg::ALbl('l_extfcgiauth'), 'fcgiauth');
	}

	protected function add_SV_EXT_LSAPI($id)
	{
		$this->setDerivedExtAppTbl('SV_EXT_FCGI', $id, DMsg::ALbl('l_extlsapi'), 'lsapi');
	}

	protected function add_SV_EXT_LOADBALANCER($id)
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

	protected function add_SV_EXT_LOGGER($id)
	{
		$attrs = [$this->_attrs['ext_name'],
			self::NewTextAttr('address', DMsg::ALbl('l_loggeraddress'), 'addr', true, 'loggerAddress'),
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

	protected function add_SV_EXT_SERVLET($id)
	{
		$attrs = $this->getExtNetworkAppAttrs($this->_attrs['ext_address']);
		$defaultExtract = ['type' => 'servlet'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_extservlet'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_SV_EXT_SCGI($id)
	{
		$attrs = $this->getExtNetworkAppAttrs($this->_attrs['ext_address'], false);
		$defaultExtract = ['type' => 'scgi'];
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_extscgi'), $attrs, 'name', null, $defaultExtract);
	}

	protected function add_SV_EXT_UWSGI($id)
	{
		$this->setDerivedExtAppTbl('SV_EXT_SCGI', $id, DMsg::ALbl('l_extuwsgi'), 'uwsgi');
	}

	protected function add_SV_EXT_PROXY($id)
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
		$this->setTemplateExtTblDef('SV_EXT_FCGI', $id);
	}

	protected function add_T_EXT_FCGIAUTH($id)
	{
		$this->setTemplateExtTblDef('SV_EXT_FCGIAUTH', $id);
	}

	protected function add_T_EXT_LSAPI($id)
	{
		$this->setTemplateExtTblDef('SV_EXT_LSAPI', $id);
	}

	protected function add_T_EXT_PROXY($id)
	{
		$this->setTemplateExtTblDef('SV_EXT_PROXY', $id);
	}

	protected function add_T_EXT_SCGI($id)
	{
		$this->setTemplateExtTblDef('SV_EXT_SCGI', $id);
	}

	protected function add_T_EXT_LOADBALANCER($id)
	{
		$this->setTemplateExtTblDef('SV_EXT_LOADBALANCER', $id);
	}

	protected function add_T_EXT_LOGGER($id)
	{
		$this->setTemplateExtTblDef('SV_EXT_LOGGER', $id);
	}

	protected function add_T_EXT_SERVLET($id)
	{
		$this->setTemplateExtTblDef('SV_EXT_SERVLET', $id);
	}

	protected function add_T_EXT_UWSGI($id)
	{
		$this->setTemplateExtTblDef('SV_EXT_UWSGI', $id);
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

	protected function add_SVT_EXPIRES($id)
	{
		$attrs = $this->get_expires_attrs();
		$this->_tblDef[$id] = DTbl::NewRegular($id, DMsg::ALbl('l_expires'), $attrs);
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

	protected function add_SVT_SCRIPT($id)
	{
		$attrs = $this->getScriptHandlerAttrs();
		$this->_tblDef[$id] = DTbl::NewIndexed($id, DMsg::ALbl('l_shdef'), $attrs, 'suffix');
	}

	protected function add_SVT_SCRIPT_TOP($id)
	{
		$align = ['center', 'center', 'center', 'center'];
		$attrs = $this->getScriptHandlerAttrs();
		$attrs[] = self::NewActionAttr('SVT_SCRIPT', 'Ed');
		$this->_tblDef[$id] = DTbl::NewTop($id, DMsg::ALbl('l_shdef'), $attrs, 'suffix', 'SVT_SCRIPT', $align, null, 'code');
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
			self::NewPathAttr('vhRoot', DMsg::ALbl('l_vhroot'), 'path', 2, 'x', false),
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


}
