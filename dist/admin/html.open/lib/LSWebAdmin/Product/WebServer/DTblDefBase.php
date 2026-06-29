<?php

namespace LSWebAdmin\Product\WebServer;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Base\DTblDefBase as ProductDTblDefBase;
use LSWebAdmin\UI\DTbl;

class DTblDefBase extends ProductDTblDefBase
{
	protected function loadCommonOptions()
	{
		parent::loadCommonOptions();

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

}
