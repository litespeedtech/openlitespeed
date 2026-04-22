<?php

namespace LSWebAdmin\Config\Validation;

use LSWebAdmin\Config\CNode;
use LSWebAdmin\UI\DTbl;

class ListenerPostTableValidationRule implements PostTableValidationRuleInterface
{
    public function Supports($request, $table)
    {
        $tid = $table->Get(DTbl::FLD_ID);
        if (in_array($tid, ['L_GENERAL', 'L_GENERAL_NEW', 'LT_GENERAL', 'LT_GENERAL_NEW', 'ADM_L_GENERAL'], true)) {
            return true;
        }

        return (($request->GetView() == 'sl' || $request->GetView() == 'al') && $tid == 'LVT_SSL_CERT');
    }

    public function Validate($request, $extracted)
    {
        $tid = $request->GetTid();
        if (in_array($tid, ['L_GENERAL', 'L_GENERAL_NEW', 'LT_GENERAL', 'LT_GENERAL_NEW', 'ADM_L_GENERAL'], true)) {
            return $this->validateGeneralListener($request, $extracted);
        }

        return $this->validateSslCert($request, $extracted);
    }

    private function validateGeneralListener($request, $extracted)
    {
        $isValid = 1;
        $ip = $extracted->GetChildVal('ip');
        $port = $extracted->GetChildVal('port');
        $isV6Ip = ($ip == '[ANY]') || (strpos($ip, ':') !== false);

        $confdata = $request->GetConfData();
        $lastref = $request->GetCurrentRef();
        $nodes = $confdata->GetRootNode()->GetChildren('listener');

        foreach ($nodes as $ref => $node) {
            if ($ref == $lastref) {
                continue;
            }

            $nodeport = $node->GetChildVal('port');
            if ($port != $nodeport) {
                continue;
            }

            $nodeip = $node->GetChildVal('ip');
            $isV6Node = ($nodeip == '[ANY]') || (strpos($nodeip, ':') !== false);
            if (($ip == $nodeip)
                || ($ip == '[ANY]' && $isV6Node)
                || ($isV6Ip && $nodeip == '[ANY]')
                || ($ip == 'ANY' && !$isV6Node)
                || (!$isV6Ip && $nodeip == 'ANY')) {
                $extracted->SetChildErr('port', 'This port is already in use.');
                $isValid = -1;
                break;
            }
        }

        $ip0 = ($ip == 'ANY') ? '*' : $ip;
        $extracted->AddChild(new CNode('address', "$ip0:$port"));

        return $isValid;
    }

    private function validateSslCert($request, $extracted)
    {
        if (!$this->isCurrentListenerSecure($request)) {
            return 1;
        }

        $isValid = 1;
        $err = 'Value must be set for secured listener. ';
        if ($extracted->GetChildVal('keyFile') == null) {
            $this->setChildErr($extracted, 'keyFile', $err);
            $isValid = -1;
        }
        if ($extracted->GetChildVal('certFile') == null) {
            $this->setChildErr($extracted, 'certFile', $err);
            $isValid = -1;
        }

        return $isValid;
    }

    private function isCurrentListenerSecure($request)
    {
        $confdata = $request->GetConfData();
        $listener = $confdata->GetChildNodeById('listener', $request->GetViewName());
        if ($listener == null) {
            return false;
        }

        return ($listener->GetChildVal('secure') == 1);
    }

    private function setChildErr($extracted, $key, $err)
    {
        if ($extracted->GetChildren($key) == null) {
            $extracted->AddChild(new CNode($key, ''));
        }

        $extracted->SetChildErr($key, $err);
    }
}
