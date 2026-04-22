<?php

namespace LSWebAdmin\Ajax;

use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Runtime\SInfo;

class ListenerVhMapAjax
{
    public static function full()
    {
        echo json_encode(self::buildReport());
    }

    private static function buildReport()
    {
        $sinfo = Service::ServiceData(SInfo::DATA_Status_LV);
        $listeners = $sinfo->Get(SInfo::FLD_Listener);
        $vhosts = $sinfo->Get(SInfo::FLD_VHosts);

        if (!is_array($listeners)) {
            $listeners = [];
        }

        if (!is_array($vhosts)) {
            $vhosts = [];
        }

        $listenerAddresses = [];
        foreach ($listeners as $listenerName => $listener) {
            if (isset($listener['addr']) && $listener['addr'] !== '') {
                $listenerAddresses[$listenerName] = (string) $listener['addr'];
            }
        }

        $listenerMap = [];
        $vhostItems = [];
        $mappingCount = 0;
        $activeDomains = [];

        foreach ($vhosts as $vhostName => $vhost) {
            if (!isset($vhost['domains']) || !is_array($vhost['domains']) || count($vhost['domains']) < 1) {
                continue;
            }

            $vhostListenerRows = [];
            $vhostDomains = [];

            foreach ($vhost['domains'] as $domain => $listenerNames) {
                $domainText = trim((string) $domain);
                $parts = preg_split('/\s+/', trim((string) $listenerNames), -1, PREG_SPLIT_NO_EMPTY);
                $domainParts = preg_split('/\s*,\s*/', $domainText, -1, PREG_SPLIT_NO_EMPTY);

                if (!is_array($parts) || count($parts) < 1) {
                    continue;
                }

                if ($domainText !== '' && !in_array($domainText, $vhostDomains, true)) {
                    $vhostDomains[] = $domainText;
                }

                if (is_array($domainParts)) {
                    foreach ($domainParts as $domainPart) {
                        if ($domainPart !== '' && $domainPart !== '*') {
                            $activeDomains[$domainPart] = true;
                        }
                    }
                }

                foreach ($parts as $listenerName) {
                    if (!isset($listenerMap[$listenerName])) {
                        $listenerMap[$listenerName] = [
                            'name' => (string) $listenerName,
                            'address' => isset($listenerAddresses[$listenerName]) ? $listenerAddresses[$listenerName] : '',
                            'rows' => [],
                        ];
                    }

                    if (!isset($vhostListenerRows[$listenerName])) {
                        $vhostListenerRows[$listenerName] = [
                            'name' => (string) $listenerName,
                            'address' => isset($listenerAddresses[$listenerName]) ? $listenerAddresses[$listenerName] : '',
                            'domains' => [],
                        ];
                    }

                    $listenerMap[$listenerName]['rows'][$vhostName] = [
                        'vhost' => (string) $vhostName,
                        'domains' => $domainText,
                    ];

                    if ($domainText !== '' && !in_array($domainText, $vhostListenerRows[$listenerName]['domains'], true)) {
                        $vhostListenerRows[$listenerName]['domains'][] = $domainText;
                    }
                }
            }

            if (count($vhostListenerRows) < 1) {
                continue;
            }

            ksort($vhostListenerRows, SORT_NATURAL | SORT_FLAG_CASE);
            foreach ($vhostListenerRows as $listenerName => $listenerRow) {
                sort($listenerRow['domains'], SORT_NATURAL | SORT_FLAG_CASE);
                $vhostListenerRows[$listenerName]['domains'] = implode(', ', $listenerRow['domains']);
            }

            sort($vhostDomains, SORT_NATURAL | SORT_FLAG_CASE);

            $mappingCount += count($vhostListenerRows);

            $vhostItems[] = [
                'name' => (string) $vhostName,
                'domains' => implode(', ', $vhostDomains),
                'listeners' => array_values($vhostListenerRows),
                'listener_count' => count($vhostListenerRows),
            ];
        }

        usort($vhostItems, function ($left, $right) {
            return strnatcasecmp($left['name'], $right['name']);
        });

        $listenerItems = [];
        foreach ($listenerMap as $listenerName => $listenerData) {
            $rows = array_values($listenerData['rows']);
            usort($rows, function ($left, $right) {
                return strnatcasecmp($left['vhost'], $right['vhost']);
            });

            $listenerItems[] = [
                'name' => (string) $listenerName,
                'address' => (string) $listenerData['address'],
                'rows' => $rows,
                'row_count' => count($rows),
            ];
        }

        usort($listenerItems, function ($left, $right) {
            return strnatcasecmp($left['name'], $right['name']);
        });

        return [
            'summary' => [
                'listener_count' => count($listenerItems),
                'vhost_count' => count($vhostItems),
                'domain_count' => count($activeDomains),
                'mapping_count' => $mappingCount,
            ],
            'listeners' => $listenerItems,
            'vhosts' => $vhostItems,
        ];
    }
}
