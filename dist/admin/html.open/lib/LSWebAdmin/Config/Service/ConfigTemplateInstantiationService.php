<?php

namespace LSWebAdmin\Config\Service;

use LSWebAdmin\Product\Base\DTblMap;
use LSWebAdmin\UI\DInfo;
use LSWebAdmin\Util\PathTool;
use LSWebAdmin\Config\CNode;
use LSWebAdmin\Log\OpsAuditLogger;

class ConfigTemplateInstantiationService
{
    public static function execute($request)
    {
        if (!($request instanceof ConfigTemplateInstantiationRequest)) {
            return new ConfigTemplateInstantiationResult(false);
        }

        $serverData = $request->GetServerData();
        $templateName = $request->GetTemplateName();
        $virtualHostName = $request->GetVirtualHostName();
        if ($serverData == null || $templateName == null || $templateName === '' || $virtualHostName == null || $virtualHostName === '') {
            return new ConfigTemplateInstantiationResult(false);
        }

        $serverTemplateNode = $serverData->GetChildNodeById('vhTemplate', $templateName);
        if ($serverTemplateNode == null) {
            return new ConfigTemplateInstantiationResult(false);
        }

        $serverMemberNode = $serverTemplateNode->GetChildNodeById('member', $virtualHostName);
        if ($serverMemberNode == null) {
            return new ConfigTemplateInstantiationResult(false);
        }

        $templatePathResult = $request->ResolveTemplatePath();
        if ($templatePathResult->HasError()) {
            return new ConfigTemplateInstantiationResult(false, [$templatePathResult->GetError()]);
        }
        if (!$templatePathResult->HasPath()) {
            return new ConfigTemplateInstantiationResult(false);
        }

        $dataClass = $request->GetConfigDataClass();
        $routeState = $request->GetRouteState();
        $templateData = new $dataClass(DInfo::CT_TP, $templatePathResult->GetPath(), null, $routeState);
        if (($confErr = $templateData->GetConfErr()) != null) {
            return new ConfigTemplateInstantiationResult(false, [$confErr]);
        }

        $templateRoot = $templateData->GetRootNode();
        $configFile = $templateRoot->GetChildVal('configFile');
        if ($configFile == null) {
            return new ConfigTemplateInstantiationResult(false);
        }

        $virtualHostRootPath = '';
        if (strncasecmp('$VH_ROOT', $configFile, 8) == 0) {
            $virtualHostRootPath = $serverMemberNode->GetChildVal('vhRoot');
            if ($virtualHostRootPath == null) {
                $virtualHostRootPath = $templateRoot->GetChildVal('vhRoot');
                if ($virtualHostRootPath == null) {
                    return new ConfigTemplateInstantiationResult(false);
                }
            }
        }

        $configFile = PathTool::GetAbsFile($configFile, 'VR', $virtualHostName, $virtualHostRootPath);
        $virtualHostData = new $dataClass(DInfo::CT_VH, $configFile, "`$virtualHostName", $routeState);
        if (($confErr = $virtualHostData->GetConfErr()) != null) {
            return new ConfigTemplateInstantiationResult(false, [$confErr]);
        }

        $domains = $serverMemberNode->GetChildVal('vhDomain');
        if ($domains == null) {
            $domains = $virtualHostName;
        }
        $domain = $domains;
        $alias = '';
        if (($domainAlias = $serverMemberNode->GetChildVal('vhAliases')) != null) {
            $domains .= ", $domainAlias";
            $alias = $domainAlias;
        }

        $virtualHostRoot = $templateRoot->GetChildren('virtualHostConfig');
        if ($virtualHostRoot == false) {
            return new ConfigTemplateInstantiationResult(false);
        }
        $virtualHostRoot->AddChild(new CNode('vhDomain', $domain));
        $virtualHostRoot->AddChild(new CNode('vhAliases', $alias));

        $virtualHostData->SetRootNode($virtualHostRoot);
        $virtualHostData->SaveFile();

        $baseMap = new DTblMap(['', '*virtualhost$name'], 'V_TOPD');
        $templateRoot->AddChild(new CNode('name', $virtualHostName));
        $templateRoot->AddChild(new CNode('note', "Instantiated from template $templateName"));
        $baseMap->Convert(0, $templateRoot, 1, $serverData->GetRootNode());
        $serverMemberNode->RemoveFromParent();

        $listeners = $serverTemplateNode->GetChildVal('listeners');
        $listenerNames = preg_split("/, /", $listeners, -1, PREG_SPLIT_NO_EMPTY);
        foreach ($listenerNames as $listenerName) {
            $listener = $serverData->GetChildNodeById('listener', $listenerName);
            if ($listener != null) {
                $virtualHostMap = new CNode('vhmap', $virtualHostName);
                $virtualHostMap->AddChild(new CNode('domain', $domains));
                $listener->AddChild($virtualHostMap);
            } else {
                error_log("cannot find listener $listenerName \n");
            }
        }

        $serverData->SaveFile();

        OpsAuditLogger::configInstantiate(
            'vh:' . $virtualHostName,
            'instantiated from template ' . $templateName
        );

        return new ConfigTemplateInstantiationResult(true);
    }
}
