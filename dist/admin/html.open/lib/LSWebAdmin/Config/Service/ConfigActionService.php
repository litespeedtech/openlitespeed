<?php

namespace LSWebAdmin\Config\Service;

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Config\CNode;
use LSWebAdmin\Config\IO\ConfigDataEditor;
use LSWebAdmin\Log\OpsAuditLogger;
use LSWebAdmin\UI\DTbl;

class ConfigActionService
{
    public static function execute($request)
    {
        $confdata = $request->GetConfData();
        $displayData = null;
        $hasDisplayData = false;
        $reloadConfig = false;
        $markChanged = false;
        $forceReLogin = false;
        $trimRoute = false;

        switch ($request->GetAct()) {
            case 's':
                $validationResult = self::validatePost($request);
                $displayData = $validationResult->GetExtracted();
                $hasDisplayData = true;

                if ($validationResult->HasErr()) {
                    self::setAct($request, 'S');
                    self::appendTopMessage($request, $displayData->Get(CNode::FLD_ERR));
                    break;
                }

                $isNewEntry = self::isNewEntry($request);
                $changeDiff = self::buildChangeDiff($confdata, $displayData, $request);
                $markChanged = self::shouldMarkChangedAfterSave($request, $displayData);
                $forceReLogin = self::shouldForceReLoginAfterSave($request, $displayData);
                $confdata->SavePost($displayData, $request->GetMutationDisplay());
                $hasDisplayData = false;
                $trimRoute = true;
                if ($isNewEntry) {
                    OpsAuditLogger::configAdd(
                        self::resolveAuditTarget($request),
                        $changeDiff
                    );
                } else {
                    OpsAuditLogger::configSave(
                        self::resolveAuditTarget($request),
                        $changeDiff
                    );
                }
                break;

            case 'a':
                $displayData = new CNode(CNode::K_EXTRACTED, '');
                $hasDisplayData = true;
                break;

            case 'c':
            case 'n':
                $validationResult = self::validatePost($request);
                $displayData = $validationResult->GetExtracted();
                $hasDisplayData = true;
                if ($request->GetAct() == 'n') {
                    $request->SwitchToSubTid($displayData);
                }
                break;

            case 'D':
                $auditTarget = self::resolveAuditTarget($request);
                $auditDetail = self::resolveAuditDetail('delete', $request);
                $confdata->DeleteEntry($request->GetMutationDisplay());
                $markChanged = self::shouldMarkChangedAfterDelete($request);
                $forceReLogin = self::shouldForceReLoginAfterDelete($request);
                $trimRoute = true;
                OpsAuditLogger::configDelete($auditTarget, $auditDetail);
                break;

            case 'I':
                if ($request->InstantiateTemplate()) {
                    $trimRoute = true;
                }
                break;

            case 'd':
            case 'i':
                $displayData = $request->GetCurrentData();
                $hasDisplayData = ($displayData != null);
                self::setConfirmationMessage($request);
                break;

            case 'o':
                break;
        }

        if ($request->ShouldApplyContextOrder()
            && $request->GetCurrentData() != null
            && $request->GetCurrentData()->ChangeContextOrder($request->GetCtxOrder())) {
            $markChanged = true;
            $trimRoute = true;
            OpsAuditLogger::configSave(
                self::resolveAuditTarget($request),
                'reorder contexts'
            );
        } elseif ($request->GetCtxSeq() != 0
            && $request->GetCurrentData() != null
            && $request->GetCurrentData()->ChangeContextSeq($request->GetCtxSeq())) {
            $markChanged = true;
            $trimRoute = true;
            OpsAuditLogger::configSave(
                self::resolveAuditTarget($request),
                'move context seq=' . $request->GetCtxSeq()
            );
        }

        if ($trimRoute) {
            $request->TrimLastId();
            $reloadConfig = true;
        }

        return new ConfigActionResult($displayData, $hasDisplayData, $reloadConfig, $markChanged, $forceReLogin);
    }

    private static function validatePost($request)
    {
        $result = $request->ValidatePost();
        if ($result != null && $result->HasViewNameUpdate()) {
            self::setViewName($request, $result->GetViewName());
        }

        return $result;
    }

    private static function setConfirmationMessage($request)
    {
        if ($request->GetAct() == 'd') {
            $actions = 'DC';
            $message = DMsg::UIStr('note_confirm_delete');
        } else {
            $actions = 'IC';
            $message = DMsg::UIStr('note_confirm_instantiate');
        }

        $message = '<p>' . $message . '</p>';
        $uiClass = $request->GetUiClass();
        if ($uiClass != null && method_exists($uiClass, 'GetActionButtons')) {
            $message .= $uiClass::GetActionButtons($request->GetActionData($actions), 'text');
        }

        self::appendTopMessage($request, $message);
    }

    private static function setViewName($request, $viewName)
    {
        $request->SetViewName($viewName);
    }

    private static function appendTopMessage($request, $message)
    {
        $request->AddTopMsg($message);
    }

    private static function setAct($request, $act)
    {
        $request->SetAct($act);
    }

    /**
     * Build a human-readable audit target from the config request route.
     *
     * Returns a string like "vh:Example:/rewrite" or "serv:listener:SSL".
     */
    private static function resolveAuditTarget($request)
    {
        $disp = $request->GetMutationDisplay();
        if ($disp === null || !is_object($disp)) {
            return '';
        }

        $parts = [];

        if (method_exists($disp, 'GetRouteState')) {
            $rs = $disp->GetRouteState();
            if ($rs !== null) {
                $confType = method_exists($rs, 'GetConfType') ? trim((string) $rs->GetConfType()) : '';
                $viewName = method_exists($rs, 'GetViewName') ? trim((string) $rs->GetViewName()) : '';
                $tid = method_exists($rs, 'GetTid') ? trim((string) $rs->GetTid()) : '';
                $ref = method_exists($rs, 'GetRef') ? trim((string) $rs->GetRef()) : '';

                if ($confType !== '') {
                    $parts[] = rtrim($confType, '_');
                }
                if ($viewName !== '') {
                    $parts[] = $viewName;
                }
                if ($tid !== '') {
                    $parts[] = str_replace('`', '/', $tid);
                }
                if ($ref !== '') {
                    $parts[] = str_replace('`', '/', $ref);
                }
            }
        }

        return implode(':', $parts);
    }

    /**
     * Build a human-readable audit detail string.
     */
    private static function resolveAuditDetail($verb, $request)
    {
        $disp = $request->GetMutationDisplay();
        if ($disp === null || !is_object($disp)) {
            return $verb;
        }

        $tid = '';
        if (method_exists($disp, 'GetRouteState')) {
            $rs = $disp->GetRouteState();
            if ($rs !== null && method_exists($rs, 'GetLastTid')) {
                $tid = trim((string) $rs->GetLastTid());
            }
        }

        return $tid !== '' ? $verb . ' ' . $tid : $verb;
    }

    /**
     * Capture field-level changes before SavePost mutates the tree.
     *
     * Reads old values from the config root for each field present in
     * $extractData, then returns a compact "field: old → new" summary.
     *
     * @param object $confdata     CData holding the config root.
     * @param CNode  $extractData  Validated POST data (new values).
     * @param object $request      The config action request.
     * @return string  Human-readable change summary.
     */
    private static function buildChangeDiff($confdata, $extractData, $request)
    {
        if (!($extractData instanceof CNode) || !is_object($confdata) || !method_exists($confdata, 'GetRootNode')) {
            return self::resolveAuditDetail('save', $request);
        }

        $root = $confdata->GetRootNode();
        if ($root === null) {
            return self::resolveAuditDetail('save', $request);
        }

        $auditTable = self::resolveAuditTable($request);
        $holderIndex = self::resolveAuditIndexKey($auditTable);
        $sensitiveFields = self::resolveSensitiveAuditFields($auditTable);

        // Navigate to the target node using the same path the mutation will use.
        $disp = $request->GetMutationDisplay();
        $oldNode = null;
        $entryRef = '';
        if ($disp !== null && is_object($disp) && method_exists($disp, 'GetRouteState')) {
            $rs = $disp->GetRouteState();
            if ($rs !== null) {
                $oldNode = self::findOldNode($root, $disp, $request);
                // The last ref segment is the entry's identity/index value.
                $entryRef = method_exists($rs, 'GetLastRef') ? trim((string) $rs->GetLastRef()) : '';
            }
        }

        $changes = [];
        $indexOldValue = '';
        $indexNewValue = '';
        $passwordChanged = false;
        $newKeys = $extractData->GetChildKeys();
        foreach ($newKeys as $key) {
            $newChild = $extractData->GetChildren($key);
            $newVal = ($newChild instanceof CNode) ? $newChild->Get(CNode::FLD_VAL) : null;
            $oldVal = null;

            if ($oldNode !== null && $oldNode instanceof CNode) {
                $oldChild = $oldNode->GetChildren($key);
                if ($oldChild instanceof CNode) {
                    $oldVal = $oldChild->Get(CNode::FLD_VAL);
                } elseif ($holderIndex !== '' && $key === $holderIndex) {
                    $oldVal = $oldNode->Get(CNode::FLD_VAL);
                }
            }

            // Normalize empty strings and nulls for comparison.
            $normOld = ($oldVal === null) ? '' : (string) $oldVal;
            $normNew = ($newVal === null) ? '' : (string) $newVal;

            if ($holderIndex !== '' && $key === $holderIndex) {
                $indexOldValue = $normOld;
                $indexNewValue = $normNew;
                continue;
            }

            if ($normOld !== $normNew) {
                if (isset($sensitiveFields[$key])) {
                    $passwordChanged = true;
                    continue;
                }

                // Skip index/identity fields that just echo the entry ref.
                // These show as "(empty) -> lsphp" when the ref is already
                // "lsphp" in the audit target — not a real change.  Only
                // include them when both old and new are non-empty (a rename).
                if ($normOld === '' && $entryRef !== '' && $normNew === $entryRef) {
                    continue;
                }

                $changes[] = self::formatAuditChange($key, $normOld, $normNew, $holderIndex);
            }
        }

        $indexDetail = self::buildIndexedAuditDetail($holderIndex, $indexOldValue, $indexNewValue, $entryRef);
        if ($indexDetail !== '' && (!empty($changes) || $passwordChanged || $indexOldValue !== $indexNewValue)) {
            array_unshift($changes, $indexDetail);
        }

        if ($passwordChanged) {
            $changes[] = self::isNewEntry($request) ? 'password set' : 'password updated';
        }

        if (empty($changes)) {
            return self::resolveAuditDetail('save (no field changes)', $request);
        }

        return implode('; ', $changes);
    }

    /**
     * Navigate the config tree to the node that will be mutated.
     *
     * Uses the same resolution logic as ConfigDataEditor::resolveTarget
     * and CNode::GetChildNode, but in a read-only fashion.
     */
    private static function findOldNode($root, $disp, $request)
    {
        try {
            $confdata = $request->GetConfData();
            if (!is_object($confdata) || !method_exists($confdata, 'GetType')) {
                return null;
            }
            $target = ConfigDataEditor::resolveTarget(
                $confdata->GetType(),
                $disp,
                \LSWebAdmin\Product\Current\DPageDef::class
            );
            $location = $target->GetLocation();
            $ref = $target->GetRef();
            return $root->GetChildNode($location, $ref);
        } catch (\Exception $e) {
            return null;
        }
    }

    /**
     * Truncate a value for display in audit logs.
     */
    private static function truncateValue($value)
    {
        $max = 120;
        if (strlen($value) <= $max) {
            return $value;
        }
        return substr($value, 0, $max) . '...';
    }

    /**
     * Detect whether the current save is creating a new entry (ref ends with ~).
     */
    private static function isNewEntry($request)
    {
        $disp = $request->GetMutationDisplay();
        if ($disp === null || !is_object($disp) || !method_exists($disp, 'GetRouteState')) {
            return false;
        }

        $rs = $disp->GetRouteState();
        if ($rs === null || !method_exists($rs, 'GetLastRef')) {
            return false;
        }

        return $rs->GetLastRef() === '~';
    }

    private static function shouldMarkChangedAfterSave($request, $extractData)
    {
        $tid = self::resolveMutationTableId($request);
        if (self::isAdminUserTableId($tid)) {
            return false;
        }

        $noRestartFields = self::resolveNoRestartFieldsForTable($tid);
        if (empty($noRestartFields) || !($extractData instanceof CNode)) {
            return true;
        }

        $fieldChanges = self::collectChangedFieldMap(
            $extractData,
            self::resolveExistingMutationNode($request),
            self::resolveAuditIndexKey(self::resolveAuditTable($request))
        );

        if (empty($fieldChanges)) {
            return false;
        }

        foreach ($fieldChanges as $field => $change) {
            if (!isset($noRestartFields[$field])) {
                return true;
            }
        }

        return false;
    }

    private static function shouldMarkChangedAfterDelete($request)
    {
        return !self::isAdminUserTableId(self::resolveMutationTableId($request));
    }

    private static function shouldForceReLoginAfterSave($request, $extractData)
    {
        if (!($extractData instanceof CNode)) {
            return false;
        }

        $tid = self::resolveMutationTableId($request);
        if (!self::isAdminUserTableId($tid) || self::isNewEntry($request)) {
            return false;
        }

        $currentUser = trim((string) CAuthorizer::getUserId());
        if ($currentUser === '') {
            return false;
        }

        $auditTable = self::resolveAuditTable($request);
        $holderIndex = self::resolveAuditIndexKey($auditTable);
        $oldNode = self::resolveExistingMutationNode($request);
        $oldName = self::resolveFieldValue($oldNode, 'name', $holderIndex);
        if (!is_string($oldName) || $oldName === '') {
            $oldName = self::resolveMutationRef($request);
        }

        if ($oldName !== $currentUser) {
            return false;
        }

        $newName = trim((string) $extractData->GetChildVal('name'));
        if ($newName !== '' && $newName !== $oldName) {
            return true;
        }

        $sensitiveFields = self::resolveSensitiveAuditFields($auditTable);
        $fieldChanges = self::collectChangedFieldMap($extractData, $oldNode, $holderIndex);
        foreach ($fieldChanges as $field => $change) {
            if (isset($sensitiveFields[$field])) {
                return true;
            }
        }

        return false;
    }

    private static function shouldForceReLoginAfterDelete($request)
    {
        if (!self::isAdminUserTableId(self::resolveMutationTableId($request))) {
            return false;
        }

        $currentUser = trim((string) CAuthorizer::getUserId());
        if ($currentUser === '') {
            return false;
        }

        return self::resolveMutationRef($request) === $currentUser;
    }

    private static function resolveMutationTableId($request)
    {
        $disp = $request->GetMutationDisplay();
        if ($disp === null || !is_object($disp) || !method_exists($disp, 'GetLastTid')) {
            return '';
        }

        return strtoupper(trim((string) $disp->GetLastTid()));
    }

    private static function isAdminUserTableId($tid)
    {
        return in_array($tid, ['ADM_USR', 'ADM_USR_NEW', 'ADM_USR_TOP'], true);
    }

    private static function resolveNoRestartFieldsForTable($tid)
    {
        if ($tid === 'ADM_PHP') {
            return [
                'sessiontimeout' => true,
            ];
        }

        if ($tid === 'ADM_THROTTLE') {
            return [
                'throttleenabled' => true,
                'throttlemaxfailures' => true,
                'throttleblockwindow' => true,
                'throttlemaxbackoff' => true,
                'loginhistoryretention' => true,
                'opsauditretainfiles' => true,
            ];
        }

        return [];
    }

    private static function resolveExistingMutationNode($request)
    {
        $confdata = $request->GetConfData();
        if (!is_object($confdata) || !method_exists($confdata, 'GetRootNode')) {
            return null;
        }

        $root = $confdata->GetRootNode();
        if ($root === null) {
            return null;
        }

        $disp = $request->GetMutationDisplay();
        if ($disp === null || !is_object($disp) || !method_exists($disp, 'GetRouteState')) {
            return null;
        }

        return self::findOldNode($root, $disp, $request);
    }

    private static function resolveMutationRef($request)
    {
        $disp = $request->GetMutationDisplay();
        if ($disp === null || !is_object($disp) || !method_exists($disp, 'GetRouteState')) {
            return '';
        }

        $rs = $disp->GetRouteState();
        if ($rs === null || !method_exists($rs, 'GetLastRef')) {
            return '';
        }

        return trim((string) $rs->GetLastRef());
    }

    private static function collectChangedFieldMap($extractData, $oldNode, $holderIndex)
    {
        $changes = [];
        if (!($extractData instanceof CNode)) {
            return $changes;
        }

        foreach ($extractData->GetChildKeys() as $key) {
            $newChild = $extractData->GetChildren($key);
            $newVal = ($newChild instanceof CNode) ? $newChild->Get(CNode::FLD_VAL) : null;
            $oldVal = self::resolveFieldValue($oldNode, $key, $holderIndex);

            $normOld = ($oldVal === null) ? '' : (string) $oldVal;
            $normNew = ($newVal === null) ? '' : (string) $newVal;
            if ($normOld !== $normNew) {
                $changes[strtolower((string) $key)] = [
                    'old' => $normOld,
                    'new' => $normNew,
                ];
            }
        }

        return $changes;
    }

    private static function resolveFieldValue($node, $key, $holderIndex)
    {
        if (!($node instanceof CNode)) {
            return null;
        }

        $child = $node->GetChildren($key);
        if ($child instanceof CNode) {
            return $child->Get(CNode::FLD_VAL);
        }

        if ($holderIndex !== '' && strtolower((string) $key) === $holderIndex) {
            return $node->Get(CNode::FLD_VAL);
        }

        return null;
    }

    private static function resolveAuditTable($request)
    {
        $disp = $request->GetMutationDisplay();
        if ($disp === null || !is_object($disp) || !method_exists($disp, 'GetLastTid')) {
            return null;
        }

        $tid = trim((string) $disp->GetLastTid());
        if ($tid === '') {
            return null;
        }

        $tableDefClass = method_exists($request, 'GetTableDefClass') ? $request->GetTableDefClass() : null;
        if (!is_string($tableDefClass) || $tableDefClass === '' || !method_exists($tableDefClass, 'GetInstance')) {
            return null;
        }

        try {
            $tableDef = call_user_func([$tableDefClass, 'GetInstance']);
            if (!is_object($tableDef) || !method_exists($tableDef, 'GetTblDef')) {
                return null;
            }

            return $tableDef->GetTblDef($tid);
        } catch (\Exception $e) {
            return null;
        }
    }

    private static function resolveAuditIndexKey($auditTable)
    {
        if ($auditTable === null || !is_object($auditTable) || !method_exists($auditTable, 'Get')) {
            return '';
        }

        $indexKey = $auditTable->Get(DTbl::FLD_INDEX);
        return is_string($indexKey) ? strtolower(trim($indexKey)) : '';
    }

    private static function resolveSensitiveAuditFields($auditTable)
    {
        $fields = [
            'passwd' => true,
        ];

        if ($auditTable === null || !is_object($auditTable) || !method_exists($auditTable, 'Get')) {
            return $fields;
        }

        $attrs = $auditTable->Get(DTbl::FLD_DATTRS);
        if (!is_array($attrs)) {
            return $fields;
        }

        foreach ($attrs as $attr) {
            if (!is_object($attr) || !method_exists($attr, 'GetKey')) {
                continue;
            }

            $key = trim((string) $attr->GetKey());
            if ($key === '') {
                continue;
            }
            $key = strtolower($key);

            if (method_exists($attr, 'GetInputType') && $attr->GetInputType() === 'password') {
                $fields[$key] = true;
            }
        }

        return $fields;
    }

    private static function formatAuditChange($key, $oldValue, $newValue, $holderIndex)
    {
        $display = (string) $key . ': ';
        if ($oldValue === '') {
            return $display . '(empty) -> ' . self::truncateValue($newValue);
        }

        if ($newValue === '') {
            return $display . self::truncateValue($oldValue) . ' -> (empty)';
        }

        return $display . self::truncateValue($oldValue) . ' -> ' . self::truncateValue($newValue);
    }

    private static function buildIndexedAuditDetail($holderIndex, $oldValue, $newValue, $entryRef)
    {
        if (!is_string($holderIndex) || $holderIndex === '') {
            return '';
        }

        if ($oldValue !== '' && $newValue !== '' && $oldValue !== $newValue) {
            return $holderIndex . ': '
                . self::truncateValue($oldValue)
                . ' -> '
                . self::truncateValue($newValue);
        }

        $identity = '';
        if ($newValue !== '') {
            $identity = $newValue;
        } elseif ($oldValue !== '') {
            $identity = $oldValue;
        } elseif ($entryRef !== '' && $entryRef !== '~') {
            $identity = $entryRef;
        }

        if ($identity === '') {
            return '';
        }

        return $holderIndex . ': ' . self::truncateValue($identity);
    }
}
