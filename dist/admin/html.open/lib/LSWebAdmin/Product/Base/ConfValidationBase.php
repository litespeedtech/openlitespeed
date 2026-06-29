<?php

namespace LSWebAdmin\Product\Base;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\Config\Validation\CValidation;
use LSWebAdmin\Config\Validation\ConfigDeleteValidationResult;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\UI\DTbl;

class ConfValidationBase extends CValidation
{
    public function ValidateDelete($request)
    {
        return ConfigDeleteValidationResult::allow();
    }

    protected function validateManagedConfigFilePolicy($attr, $path, &$err)
    {
        $policy = $this->getManagedConfigFilePolicy($attr);
        if (!is_array($policy)) {
            return 1;
        }

        $directory = isset($policy['directory']) ? $policy['directory'] : '';
        $suffix = isset($policy['suffix']) ? $policy['suffix'] : '';
        $locationMessage = isset($policy['location_message']) ? $policy['location_message'] : '';
        $suffixMessage = isset($policy['suffix_message']) ? $policy['suffix_message'] : '';

        if ($directory !== '' && !$this->isPathInsideDirectory($path, SERVER_ROOT . $directory)) {
            $err = $locationMessage;
            return -1;
        }

        if ($suffix !== '' && !$this->endsWith($path, $suffix)) {
            $err = $suffixMessage;
            return -1;
        }

        return 1;
    }

    protected function getManagedConfigFilePolicy($attr)
    {
        return null;
    }

    protected function validateExternalAppDelete($request)
    {
        $target = $request->GetTarget();
        if (!$this->isExternalAppDeleteTid($target->GetTid())) {
            return ConfigDeleteValidationResult::allow();
        }

        $root = $request->GetRootNode();
        $app = $this->getTargetNode($root, $target);
        if (!($app instanceof CNode)) {
            return ConfigDeleteValidationResult::allow();
        }

        $appName = $this->nodeName($target->GetRef(), $app);
        $appType = $app->GetChildVal('type');
        if ($appType === null || $appType === '') {
            $appType = $this->getExternalAppTypeFromTid($target->GetTid());
        }

        if ($appName === '' || $appType === '') {
            return ConfigDeleteValidationResult::allow();
        }

        $owner = $this->getExternalAppOwnerNode($root, $target->GetLocation());
        if (!($owner instanceof CNode)) {
            return ConfigDeleteValidationResult::allow();
        }

        $references = [];
        $ownerLabel = $this->getCurrentConfigOwnerLabel($request, $target->GetLocation());
        $this->collectScriptHandlerExternalAppReferences($owner, $appType, $appName, $ownerLabel, $references);
        $this->collectContextExternalAppReferences($owner->GetChildren('context'), $appType, $appName, $ownerLabel, $references);

        if (empty($references)) {
            if ($this->isServerLevelExternalAppDelete($request, $target)) {
                return ConfigDeleteValidationResult::allow([
                    $this->deleteReferenceMessage('note_delete_ref_extapp_vhost_unscanned'),
                ]);
            }

            return ConfigDeleteValidationResult::allow();
        }

        array_unshift($references, $this->deleteReferenceMessage('note_delete_ref_extapp', [
            '%%name%%' => $appName,
        ]));
        return ConfigDeleteValidationResult::block($references);
    }

    protected function collectScriptHandlerExternalAppReferences($owner, $appType, $appName, $ownerLabel, &$references)
    {
        $scriptHandlers = $owner->GetChildren('scripthandler:addsuffix');
        if ($scriptHandlers == null) {
            $scriptHandlers = $owner->GetChildren('scripthandler');
        }

        foreach ($this->asNodeList($scriptHandlers) as $suffix => $scriptHandler) {
            if ($scriptHandler->GetChildVal('handler') == $appName
                && $this->scriptHandlerTypeMatchesExternalApp($scriptHandler->GetChildVal('type'), $appType)) {
                $references[] = $this->deleteReferenceMessage('note_delete_ref_script_handler_field', [
                    '%%owner%%' => $ownerLabel,
                    '%%name%%' => $this->nodeName($suffix, $scriptHandler),
                    '%%field%%' => 'handler',
                ]);
            }
        }
    }

    protected function collectContextExternalAppReferences($contexts, $appType, $appName, $ownerLabel, &$references)
    {
        foreach ($this->asNodeList($contexts) as $uri => $context) {
            if ($context->GetChildVal('handler') == $appName
                && $this->contextTypeMatchesExternalApp($context->GetChildVal('type'), $appType)) {
                $references[] = $this->deleteReferenceMessage('note_delete_ref_context_field', [
                    '%%owner%%' => $ownerLabel,
                    '%%name%%' => $this->nodeName($uri, $context),
                    '%%field%%' => 'handler',
                ]);
            }
        }
    }

    protected function deleteReferenceMessage($tag, $replacements = [])
    {
        return DMsg::UIStr($tag, $replacements);
    }

    protected function deleteReferenceOwner($label, $name = '')
    {
        return ($name !== '') ? $label . ' ' . $name : $label;
    }

    protected function asNodeList($nodes)
    {
        if ($nodes == null) {
            return [];
        }

        if ($nodes instanceof CNode) {
            return [$nodes->Get(CNode::FLD_VAL) => $nodes];
        }

        return is_array($nodes) ? $nodes : [];
    }

    protected function nodeName($key, $node)
    {
        $value = ($node instanceof CNode) ? $node->Get(CNode::FLD_VAL) : null;
        return ($value !== null && $value !== '') ? $value : $key;
    }

    private function isExternalAppDeleteTid($tid)
    {
        if (!is_string($tid)) {
            return false;
        }

        return (strpos($tid, 'SV_EXT_') === 0 || strpos($tid, 'T_EXT_') === 0)
            && $tid !== 'SV_EXT_SEL'
            && $tid !== 'SV_EXT_TOP'
            && $tid !== 'T_EXT_SEL'
            && $tid !== 'T_EXT_TOP';
    }

    private function getTargetNode($root, $target)
    {
        $location = $target->GetLocation();
        $ref = $target->GetRef();
        return $root->GetChildrenByLoc($location, $ref);
    }

    private function getExternalAppTypeFromTid($tid)
    {
        static $types = [
            'FCGI' => 'fcgi',
            'FCGIAUTH' => 'fcgiauth',
            'LSAPI' => 'lsapi',
            'LOADBALANCER' => 'loadbalancer',
            'LOGGER' => 'logger',
            'PROXY' => 'proxy',
            'SCGI' => 'scgi',
            'SERVLET' => 'servlet',
            'UWSGI' => 'uwsgi',
        ];

        foreach ($types as $marker => $type) {
            if (strpos($tid, '_' . $marker) !== false) {
                return $type;
            }
        }

        return '';
    }

    private function getExternalAppOwnerNode($root, $targetLocation)
    {
        $ownerLocation = $this->getLocationBeforeLayer($targetLocation, 'extprocessor');
        if ($ownerLocation === '') {
            return $root;
        }

        return $root->GetChildren($ownerLocation);
    }

    private function getLocationBeforeLayer($location, $layerName)
    {
        $owner = [];
        foreach (explode(':', (string) $location) as $layer) {
            $normalized = ltrim($layer, '*');
            if (($varpos = strpos($normalized, '$')) > 0) {
                $normalized = substr($normalized, 0, $varpos);
            }

            if (strcasecmp($normalized, $layerName) === 0) {
                break;
            }

            if ($layer !== '') {
                $owner[] = $layer;
            }
        }

        return implode(':', $owner);
    }

    private function getCurrentConfigOwnerLabel($request, $targetLocation)
    {
        $display = null;
        $actionRequest = $request->GetActionRequest();
        if ($actionRequest != null && method_exists($actionRequest, 'GetMutationDisplay')) {
            $display = $actionRequest->GetMutationDisplay();
        }

        $confType = (is_object($display) && method_exists($display, 'GetConfType')) ? $display->GetConfType() : '';
        $ref = (is_object($display) && method_exists($display, 'GetRef')) ? $display->GetRef() : '';

        if ($confType == 'tp_') {
            return $this->deleteReferenceOwner(DMsg::UIStr('tab_tp'), $ref);
        }

        if ($confType == 'vh_' || strpos((string) $targetLocation, 'virtualHostConfig') !== false) {
            return $this->deleteReferenceOwner(DMsg::UIStr('menu_vh_'), $ref);
        }

        return DMsg::UIStr('menu_serv');
    }

    private function isServerLevelExternalAppDelete($request, $target)
    {
        $actionRequest = $request->GetActionRequest();
        $display = null;
        if ($actionRequest != null && method_exists($actionRequest, 'GetMutationDisplay')) {
            $display = $actionRequest->GetMutationDisplay();
        }

        $confType = (is_object($display) && method_exists($display, 'GetConfType')) ? $display->GetConfType() : '';
        if ($confType !== '' && $confType != 'serv') {
            return false;
        }

        return $this->getLocationBeforeLayer($target->GetLocation(), 'extprocessor') === '';
    }

    private function scriptHandlerTypeMatchesExternalApp($handlerType, $appType)
    {
        return $handlerType !== null && $handlerType !== '' && $handlerType == $appType;
    }

    private function contextTypeMatchesExternalApp($contextType, $appType)
    {
        if ($contextType == $appType) {
            return true;
        }

        return ($appType == 'servlet' && $contextType == 'webapp');
    }

    protected function isValidAttr($attr, $cval)
    {
        $isValid = parent::isValidAttr($attr, $cval);

        if ($isValid == 1 && $attr->_type == 'modulename') {
            $res = $this->chkAttrModuleName($cval);
            if ($res != 1) {
                return $res;
            }
        }

        return $isValid;
    }

    protected function chkAttrModuleName($cval)
    {
        $name = $cval->Get(CNode::FLD_VAL);
        if (preg_match("/[<>&%\s]/", $name)) {
            $cval->SetErr('invalid characters in name');
            return -1;
        }

        return 1;
    }

    protected function validatePostTbl($tbl, $extracted)
    {
        if ($tbl->Get(DTbl::FLD_ID) == 'S_MOD') {
            return $this->chkPostTblServModule($extracted);
        }

        return parent::validatePostTbl($tbl, $extracted);
    }

    protected function chkPostTblServModule($extracted)
    {
        $isValid = 1;

        $name = $extracted->GetChildVal('name');
        if ($extracted->GetChildVal('internal') == 0) {
            if ($name != 'cache') {
                $module = SERVER_ROOT . "modules/{$name}.so";
                if (!file_exists($module)) {
                    $extracted->SetChildErr('name', "cannot find external module: $module");
                    $isValid = -1;
                }
            } else {
                $extracted->SetChildErr('internal', 'This is a built-in internal module');
                $isValid = -1;
            }
        }

        return $isValid;
    }

    protected function endsWith($value, $suffix)
    {
        $suffixLength = strlen($suffix);
        if ($suffixLength === 0) {
            return true;
        }

        if (!is_string($value) || strlen($value) < $suffixLength) {
            return false;
        }

        return substr_compare($value, $suffix, -$suffixLength, $suffixLength) === 0;
    }
}
