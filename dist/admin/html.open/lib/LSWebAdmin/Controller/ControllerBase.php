<?php

namespace LSWebAdmin\Controller;

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\Config\Service\ConfigActionContext;
use LSWebAdmin\Config\Service\ConfigActionRequest;
use LSWebAdmin\Config\Service\ConfigActionResult;
use LSWebAdmin\Config\Service\ConfigActionService;
use LSWebAdmin\Config\IO\ConfigPathResolutionResult;
use LSWebAdmin\Config\IO\ConfigPathResolver;
use LSWebAdmin\Config\Service\ConfigBootstrapRequest;
use LSWebAdmin\Config\Service\ConfigBootstrapResult;
use LSWebAdmin\Config\Service\ConfigBootstrapService;
use LSWebAdmin\Config\Load\ConfigLoadRequest;
use LSWebAdmin\Config\Service\ConfigDataSelectionRequest;
use LSWebAdmin\Config\Service\ConfigDataSelectionService;
use LSWebAdmin\Config\Service\ConfigTemplateInstantiationRequest;
use LSWebAdmin\Config\Service\ConfigTemplateInstantiationService;
use LSWebAdmin\Config\Validation\ConfigValidationRequest;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\ConfValidation;
use LSWebAdmin\Product\Current\ConfigData;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\Runtime\SInfo;
use LSWebAdmin\Runtime\ServiceCommandPlanner;
use LSWebAdmin\Runtime\SuspendedVhostMutationService;
use LSWebAdmin\Log\OpsAuditLogger;
use LSWebAdmin\UI\DInfo;
use LSWebAdmin\UI\UIBase;

class ControllerBase
{
	protected static $commandClient;
	protected $_bootstrapResult;
	protected $_disp;
	protected $_service;
    protected $_lastServiceRequestError = '';
    protected $_configDispLoaded = false;

	protected static $instances = [];

	protected function __construct()
	{
		$this->_disp = $this->createDisplay();
	}

    protected function createDisplay()
    {
        return DInfo::singleton();
    }

    protected function getDisplay()
    {
        return $this->_disp;
    }

    protected function hasLoadedConfigDisplay()
    {
        return $this->_configDispLoaded;
    }

    protected function prepareDisplayForConfigRequest()
    {
        $this->resetConfigRuntimeState();
        $this->initializeDisplayForConfigRequest();
    }

    protected function resetConfigRuntimeState()
    {
        $this->replaceBootstrapResult(null);
        $this->resetDisplayForConfigRequest();
    }

    protected function resetDisplayForConfigRequest()
    {
        if (method_exists($this->getDisplay(), 'ResetConfigState')) {
            $this->getDisplay()->ResetConfigState();
        }
    }

    protected function initializeDisplayForConfigRequest()
    {
        $this->getDisplay()->InitConf();
    }

    protected function markConfigDisplayLoaded($data)
    {
        if (is_object($data) && method_exists($data, 'GetRootNode')) {
            $data = $data->GetRootNode();
        }

        $this->getDisplay()->SetPageData($data);
        $this->_configDispLoaded = true;
    }

	public static function singleton()
	{
		$class = get_called_class();
		if (!isset(self::$instances[$class])) {
			self::$instances[$class] = new $class;
		}

		return self::$instances[$class];
	}

	public static function ConfigDispData()
	{
		$cc = static::singleton();

        if ($cc->hasLoadedConfigDisplay()) {
            return $cc->getDisplay();
        }

        $cc->prepareDisplayForConfigRequest();
		$data = $cc->process_config();
        $cc->markConfigDisplayLoaded($data);

		return $cc->getDisplay();
	}

	public static function ServiceData($type)
	{
		$cc = static::singleton();
		$data = $cc->process_service_data($type);

		return $data;
	}

	public static function RuntimePid()
	{
		return static::singleton()->readRuntimePid();
	}

	public static function ServiceRequest($type, $actId = '')
	{
		$cc = static::singleton();
		$result = $cc->process_service_request($type, $actId);

		return $result;
	}

	public static function GetLastServiceRequestError()
	{
		return static::singleton()->readLastServiceRequestError();
	}

	public static function HasChanged()
	{
		return (isset($_SESSION['changed']) ? $_SESSION['changed'] : false);
	}

	public static function getServerLoad()
	{
		$avgload = \sys_getloadavg();
		if ($avgload === false) {
			return 'N/A';
		}
		return implode(', ', array_map(function($load) { return round($load, 3); }, $avgload));
	}

	protected function setChanged($changed)
	{
		$_SESSION['changed'] = $changed;
	}

    protected function getBootstrapResult()
    {
        if (!($this->_bootstrapResult instanceof ConfigBootstrapResult)) {
            $this->_bootstrapResult = new ConfigBootstrapResult();
        }

        return $this->_bootstrapResult;
    }

    protected function replaceBootstrapResult($result)
    {
        $this->_bootstrapResult = ($result instanceof ConfigBootstrapResult)
            ? $result
            : new ConfigBootstrapResult();
    }

    protected function getServerConfigData()
    {
        return $this->getBootstrapResult()->GetServerData();
    }

    protected function setServerConfigData($data)
    {
        $this->getBootstrapResult()->SetServerData($data);
        $this->getDisplay()->SetServData($data);
    }

    protected function getAdminConfigData()
    {
        return $this->getBootstrapResult()->GetAdminData();
    }

    protected function setAdminConfigData($data)
    {
        $this->getBootstrapResult()->SetAdminData($data);
    }

    protected function getCurrentConfigData()
    {
        return $this->getBootstrapResult()->GetCurrentData();
    }

    protected function setCurrentConfigData($data)
    {
        $this->getBootstrapResult()->SetCurrentData($data);
    }

    protected function getSpecialConfigData()
    {
        return $this->getBootstrapResult()->GetSpecialData();
    }

    protected function setSpecialConfigData($data)
    {
        $this->getBootstrapResult()->SetSpecialData($data);
    }

	protected function resolveConfFilePathResult($type, $name = '', $servData = null)
	{
		$sourceServData = ($servData != null) ? $servData : $this->getServerConfigData();
		return ConfigPathResolver::resolve($type, $name, $sourceServData, $this->supportsTemplateConfigPath());
	}

	protected function supportsTemplateConfigPath()
	{
		return true;
	}

	protected function getPidFile()
	{
		trigger_error('Product service must define its PID file', E_USER_ERROR);
	}

	protected function getStatusFile()
	{
		trigger_error('Product service must define its status file', E_USER_ERROR);
	}

	protected function createSInfo()
	{
		return new SInfo($this->getStatusFile());
	}

	protected function readRuntimePid()
	{
		$pidFile = $this->getPidFile();
		if ($pidFile === '' || !is_file($pidFile)) {
			return '';
		}

		$pid = @file_get_contents($pidFile);
		return ($pid === false) ? '' : trim((string) $pid);
	}

	protected function getDebugLogState()
	{
		return SInfo::GetDebugLogState($this->getStatusFile());
	}

	protected function getLicenseInfo()
	{
		return false;
	}

	public static function DebugLogState()
	{
		return static::singleton()->getDebugLogState();
	}

	public static function LicenseInfo()
	{
		return static::singleton()->getLicenseInfo();
	}

	public static function WaitForStatusChange()
	{
		$service = static::singleton()->createSInfo();
		return $service->WaitForChange();
	}

	protected function getConfData()
	{
        $request = ConfigDataSelectionRequest::fromRouteState($this->getRouteState());
		$confdata = ConfigDataSelectionService::resolve(
			$request,
			$this->getServerConfigData(),
			$this->getAdminConfigData(),
			$this->getCurrentConfigData(),
			$this->getSpecialConfigData(),
			$this->allowDetachedCurrentData()
		);

		$this->getDisplay()->SetConfData($confdata);
		return $confdata;
	}

	protected function getConfigDataClass()
	{
		return ConfigData::class;
	}

	protected function allowDetachedCurrentData()
	{
		return true;
	}

	protected function load_server_config($load_admin=false)
	{
		$dataClass = $this->getConfigDataClass();
		$serverResolution = $this->resolveConfFilePathResult(DInfo::CT_SERV);
		$this->applyPathResolutionResult($serverResolution);
		if ($serverResolution->HasPath()) {
            $serverData = new $dataClass(DInfo::CT_SERV, $serverResolution->GetPath(), null, $this->getRouteState());
            $this->setServerConfigData($serverData);
			if (($conferr = $serverData->GetConfErr()) != null) {
				$this->getDisplay()->AddTopMsg($conferr);
			}
		}

		$has_set_timout = CAuthorizer::HasSetTimeout();
		if (!$has_set_timout || $load_admin) {
			$adminResolution = $this->resolveConfFilePathResult(DInfo::CT_ADMIN);
			$this->applyPathResolutionResult($adminResolution);
			if ($adminResolution->HasPath()) {
                $adminData = new $dataClass(DInfo::CT_ADMIN, $adminResolution->GetPath(), null, $this->getRouteState());
                $this->setAdminConfigData($adminData);
				if (($conferr = $adminData->GetConfErr()) != null)
					$this->getDisplay()->AddTopMsg($conferr);
			}

			$this->initializeAdminTimeout($this->getAdminConfigData(), $has_set_timout);
		}
	}

	protected function loadConfig()
	{
        $routeState = $this->getRouteState();
		$loadAdminRequested = ($routeState->GetConfType() == DInfo::CT_ADMIN);
		$bootstrapRequest = new ConfigBootstrapRequest(
			$this->getConfigDataClass(),
            ConfigLoadRequest::fromRouteState($routeState, $this->getDisplay()->GetVHRoot()),
			(!$this->hasSessionTimeout() || $loadAdminRequested),
			$this->allowDetachedCurrentData(),
			$this->resolveConfFilePathResult(DInfo::CT_SERV),
			$this->resolveConfFilePathResult(DInfo::CT_ADMIN),
			function ($type, $name, $servData) {
				return $this->resolveConfFilePathResult($type, $name, $servData);
			},
            $routeState
		);

		$this->applyBootstrapResult(ConfigBootstrapService::execute($bootstrapRequest));
	}

	protected function hasSessionTimeout()
	{
		return CAuthorizer::HasSetTimeout();
	}

    protected function getRouteState()
    {
        return $this->getDisplay()->GetRouteState();
    }

	protected function initializeAdminTimeout($adminData, $has_set_timout = null)
	{
		if ($adminData == null) {
			return;
		}

		if ($has_set_timout == null) {
			$has_set_timout = $this->hasSessionTimeout();
		}

		if (!$has_set_timout) {
			$timeout = $adminData->GetChildVal('sessionTimeout');
			if ($timeout == null)
				$timeout = 60; // default
			CAuthorizer::SetTimeout($timeout);
		}

		// Apply throttle configuration from admin settings.
		$throttleConfig = [];
		$throttleEnabled = $adminData->GetChildVal('throttleEnabled');
		if ($throttleEnabled !== null) {
			$throttleConfig['enabled'] = ($throttleEnabled != 0);
		}
		$throttleMaxFailures = $adminData->GetChildVal('throttleMaxFailures');
		if ($throttleMaxFailures !== null) {
			$throttleConfig['maxFailures'] = (int) $throttleMaxFailures;
		}
		$throttleBlockWindow = $adminData->GetChildVal('throttleBlockWindow');
		if ($throttleBlockWindow !== null) {
			$throttleConfig['blockWindow'] = (int) $throttleBlockWindow;
		}
		$throttleMaxBackoff = $adminData->GetChildVal('throttleMaxBackoff');
		if ($throttleMaxBackoff !== null) {
			$throttleConfig['maxBackoff'] = (int) $throttleMaxBackoff;
		}
		if (!empty($throttleConfig)) {
			CAuthorizer::SetThrottleConfig($throttleConfig);
		}

		// Apply login-history retention from admin settings.
		$loginHistoryRetention = $adminData->GetChildVal('loginHistoryRetention');
		if ($loginHistoryRetention !== null) {
			CAuthorizer::SetLoginHistoryRetention((int) $loginHistoryRetention);
		}

		// Apply ops audit retention from admin settings.
		$opsAuditRetainFiles = $adminData->GetChildVal('opsAuditRetainFiles');
		if ($opsAuditRetainFiles !== null) {
			$opsAuditRetainFiles = (int) $opsAuditRetainFiles;
			OpsAuditLogger::setMaxRotatedFiles($opsAuditRetainFiles);
			$_SESSION['opsAuditRetainFiles'] = $opsAuditRetainFiles;
		}
	}

	protected function applyBootstrapResult($result)
	{
		if (!($result instanceof ConfigBootstrapResult)) {
			return;
		}

		foreach ($result->GetMessages() as $message) {
			$this->getDisplay()->AddTopMsg($message);
		}

        $this->replaceBootstrapResult($result);

		if ($this->getServerConfigData() != null) {
			$this->getDisplay()->SetServData($this->getServerConfigData());
		}

		foreach ($result->GetLoadedData() as $data) {
			if (($conferr = $data->GetConfErr()) != null) {
				$this->getDisplay()->AddTopMsg($conferr);
			}
		}

		$this->initializeAdminTimeout($this->getAdminConfigData());
	}

	protected function applyPathResolutionResult($result)
	{
		if ($result instanceof ConfigPathResolutionResult && $result->HasError()) {
			$this->getDisplay()->AddTopMsg($result->GetError());
		}
	}

	protected function process_config()
	{
		$this->normalizeConfigRequest();
		$this->loadConfig();

		$confdata = $this->getConfData();
		$actionRequest = $this->buildConfigActionRequest($confdata);

		if ($actionRequest == null) {
			return $confdata;
		}

        $result = ConfigActionService::execute($actionRequest);

		return $this->resolveConfigActionResult(
            $result,
			$confdata
		);
	}

	protected function normalizeConfigRequest()
	{
	}

	protected function buildConfigActionRequest($confdata)
	{
		return new ConfigActionRequest(
			ConfigActionContext::fromDisplay($this->getDisplay()),
			$confdata,
			$this->getCurrentConfigData(),
			function () {
				return $this->validatePostRequest();
			},
			function () {
				return $this->instantiateTemplate();
			},
			UI::class,
			UIBase::GrabGoodInputWithReset('ANY', 'ctxseq', 'int'),
			UIBase::GrabGoodInputWithReset('ANY', 'ctxorder')
		);
	}

	protected function resolveConfigActionResult($result, $confdata)
	{
		if (!($result instanceof ConfigActionResult)) {
			return $confdata;
		}

		if ($result->ShouldMarkChanged()) {
			$this->setChanged(true);
		}

        if ($result->ShouldForceReLogin()) {
            $this->redirectToLoginForReauthentication();
            return $confdata;
        }

		if ($result->ShouldReloadConfig()) {
			$this->loadConfig();
			$confdata = $this->getConfData();

            if ($this->shouldRedirectAfterConfigPost()) {
                $this->redirectToCurrentConfigView();
            }
		}

		return $result->ResolveDisplayData($confdata);
	}

    protected function redirectToLoginForReauthentication()
    {
        CAuthorizer::singleton()->InvalidateForReLogin();

        if (PHP_SAPI === 'cli' || headers_sent()) {
            return;
        }

        header('Location: /login.php?relogin=1', true, 303);
        exit;
    }

    protected function shouldRedirectAfterConfigPost()
    {
        if (PHP_SAPI === 'cli' || headers_sent()) {
            return false;
        }

        return (strtoupper((string) UIBase::GrabInput('SERVER', 'REQUEST_METHOD')) === 'POST');
    }

    protected function redirectToCurrentConfigView()
    {
        header('Location: ' . $this->buildCurrentConfigViewUrl(), true, 303);
        exit;
    }

    protected function buildCurrentConfigViewUrl()
    {
        $routeState = $this->getRouteState();
        $params = [
            'view' => 'confMgr',
            'm' => $routeState->GetMid(),
            'p' => $routeState->GetPid(),
        ];

        $tid = $routeState->GetTid();
        if ($tid != null && $tid !== '') {
            $params['t'] = $tid;
        }

        $ref = $routeState->GetRef();
        if ($ref != null && $ref !== '') {
            $params['r'] = $ref;
        }

        return 'index.php?' . http_build_query($params);
    }

	protected function validatePostRequest()
	{
		$validator = new ConfValidation();
        return $validator->ValidateRequest(ConfigValidationRequest::fromRouteState(
            $this->getRouteState(),
            $this->getDisplay()->GetConfData(),
            $this->getDisplay()->GetVHRoot(),
            function ($tid, $loc, $node) {
                return $this->getDisplay()->GetDerivedSelOptions($tid, $loc, $node);
            },
            UIBase::GetInputSource()
        ));
	}


	protected function instantiateTemplate()
	{
        $routeState = $this->getRouteState();
		$tpname = $routeState->GetViewName();
		$vhname = $routeState->GetLastRef();
		$result = ConfigTemplateInstantiationService::execute(new ConfigTemplateInstantiationRequest(
			$this->getServerConfigData(),
			$tpname,
			$vhname,
			function ($templateName) {
				return $this->resolveConfFilePathResult(DInfo::CT_TP, $templateName);
			},
			$this->getConfigDataClass(),
            $routeState
		));

		foreach ($result->GetMessages() as $message) {
			$this->getDisplay()->AddTopMsg($message);
		}

		return $result->IsSuccessful();
	}

	protected function process_service_request($type, $actId)
	{
        $this->resetLastServiceRequestError();

		// has pending command process
		if (file_exists(SInfo::GetAdminFile())) {
            $this->rememberLastServiceRequestError(DMsg::UIStr('service_requestinprogress'));
			return false;
		}

		$plan = ServiceCommandPlanner::plan($type, $actId);
		if ($plan == null) {
			error_log("illegal type in process_service_request $type ");
            $this->rememberLastServiceRequestError(DMsg::UIStr('service_requestfailed'));
			return false;
		}

		if ($plan->RequiresServerConfigMutation()) {
			if ($this->getServerConfigData() == null) {
				$this->load_server_config();
			}
			SuspendedVhostMutationService::apply($this->getServerConfigData(), $plan->GetSuspendedVhostAction(), $actId);
		}

		if ($plan->ShouldResetChanged()) {
			$this->setChanged(false);
		}

		$this->_service = $this->createSInfo();
		if (!$this->issueCmd($plan->GetCommand())) {
            $message = trim((string) self::getCommandClient()->getLastError());
            $this->rememberLastServiceRequestError(($message !== '') ? $message : DMsg::UIStr('service_requestfailed'));
			return false;
		}
		$this->_service->WaitForChange();

		// Audit the service action after issuing the command.
		switch ($type) {
			case SInfo::SREQ_RESTART_SERVER:
				OpsAuditLogger::restart();
				break;
			case SInfo::SREQ_TOGGLE_DEBUG:
				OpsAuditLogger::toggleDebug($this->getDebugLogState());
				break;
			case SInfo::SREQ_VH_RELOAD:
				OpsAuditLogger::vhostReload($actId);
				break;
			case SInfo::SREQ_VH_ENABLE:
				OpsAuditLogger::vhostEnable($actId);
				break;
			case SInfo::SREQ_VH_DISABLE:
				OpsAuditLogger::vhostDisable($actId);
				break;
		}

		return true;
	}

    protected function readLastServiceRequestError()
    {
        return $this->_lastServiceRequestError;
    }

    protected function rememberLastServiceRequestError($message)
    {
        $this->_lastServiceRequestError = (string) $message;
    }

    protected function resetLastServiceRequestError()
    {
        $this->_lastServiceRequestError = '';
    }


	protected function process_service_data($type)
	{
		// process static type
		switch ($type) {
			case SInfo::DATA_PID:
				return $this->readRuntimePid();

			case SInfo::DATA_DEBUGLOG_STATE:
				return $this->getDebugLogState();
		}

		// require config data
		$this->_service = $this->createSInfo();
		$this->load_server_config();
		$this->_service->Init($this->getServerConfigData());

		switch($type) {
			case SInfo::DATA_ADMIN_EMAIL:
				return $this->getServerConfigData()->GetChildVal('adminEmails');

			case SInfo::DATA_DASH_LOG:
			case SInfo::DATA_VIEW_LOG:
				return $this->_service->LoadLog($type);

			case SInfo::DATA_Status_LV:
				$this->_service->LoadStatus();
				return $this->_service;

			default: trigger_error("ControllerBase: illegal type in process_service_data: $type", E_USER_ERROR);
		}

		return false;
	}

	public static function getCommandSocket($cmd)
	{
		return self::getCommandClient()->openAuthenticatedSocket($cmd);
	}

	protected function issueCmd($cmd)
	{
		CAuthorizer::singleton()->Reauthenticate();

		$commandline = '';
		if (is_array($cmd)) {
			foreach( $cmd as $line ) {
				$commandline .= $line . "\n";
			}
		}
		else {
			$commandline = $cmd . "\n";
		}

		return self::getCommandClient()->sendCommand($commandline);
	}

	public function retrieveCommandData($cmd)
	{
		return self::getCommandClient()->fetchCommandData($cmd);
	}

	protected static function getCommandClient()
	{
		if (self::$commandClient == null) {
			self::$commandClient = new AdminCommandClient();
		}

		return self::$commandClient;
	}

}
