<?php

namespace LSWebAdmin\Product\Base;

abstract class RequestProbeBase
{
    const FLD_IP = 0;
    const FLD_KA_REQ_SERVED = 1;
    const FLD_MODE = 2;
    const FLD_REQ_TIME = 3;
    const FLD_IN_BYTES_CURRENT = 4;
    const FLD_IN_BYTES_TOTAL = 5;
    const FLD_OUT_BYTES_CURRENT = 6;
    const FLD_OUT_BYTES_TOTAL = 7;
    const FLD_VHOST = 8;
    const FLD_HANDLER = 9;
    const FLD_EXTAPP_PROCESS_TIME = 10;
    const FLD_EXTAPP_SOCKET = 11;
    const FLD_EXTAPP_PID = 12;
    const FLD_EXTAPP_CONN_REQ_SERVED = 13;
    const FLD_URL = 14;

    protected $rawData = '';

    protected function __construct()
    {
        $this->rawData = $this->loadRawData();
        if (!is_string($this->rawData)) {
            $this->rawData = '';
        }
    }

    abstract protected function loadRawData();

    public static function GetSnapshot($filters = null)
    {
        $probe = new static();
        return $probe->buildSnapshotReport($filters);
    }

    public static function NormalizeFilters($filters = null)
    {
        if (!is_array($filters)) {
            $filters = $_REQUEST;
        }

        $normalized = [
            'show_top' => 10,
            'show_sort' => 'req_time',
            'vhname' => '',
            'ip' => '',
            'url' => '',
            'req_time' => '',
            'proc_time' => '',
            'in_kb' => '',
            'out_kb' => '',
            'extapp_more' => false,
        ];

        if (isset($filters['show_top']) && is_scalar($filters['show_top'])) {
            $showTop = (int) $filters['show_top'];
            if ($showTop >= 0) {
                $normalized['show_top'] = $showTop;
            }
        }

        if (isset($filters['show_sort']) && is_scalar($filters['show_sort'])) {
            $showSort = trim((string) $filters['show_sort']);
            if (in_array($showSort, ['vhname', 'client', 'req_time', 'extproc_time', 'in', 'out'])) {
                $normalized['show_sort'] = $showSort;
            }
        }

        foreach (['vhname', 'ip', 'url', 'req_time', 'proc_time', 'in_kb', 'out_kb'] as $key) {
            if (isset($filters[$key]) && is_scalar($filters[$key])) {
                $normalized[$key] = trim((string) $filters[$key]);
            }
        }

        if (isset($filters['extapp_more'])) {
            $extAppMore = $filters['extapp_more'];
            $normalized['extapp_more'] = ($extAppMore === '1' || $extAppMore === 1 || $extAppMore === true || $extAppMore === 'on');
        }

        return $normalized;
    }

    protected function buildSnapshotReport($filters = null)
    {
        $filters = self::NormalizeFilters($filters);
        $rows = $this->parseRows($this->rawData);
        $totalCount = count($rows);
        $rows = $this->filterRows($rows, $filters);
        $matchedCount = count($rows);
        $rows = $this->sortRows($rows, $filters['show_sort']);

        if ($filters['show_top'] > 0 && $matchedCount > $filters['show_top']) {
            $rows = array_slice($rows, 0, $filters['show_top']);
        }

        $generatedAt = time();

        return [
            'generated_at' => date('D M j H:i:s T', $generatedAt),
            'generated_at_epoch' => $generatedAt,
            'total_count' => $totalCount,
            'matched_count' => $matchedCount,
            'filtered_count' => count($rows),
            'filters' => $filters,
            'show_extapp_detail' => $filters['extapp_more'],
            'rows' => $rows,
        ];
    }

    protected function parseRows($rawData)
    {
        $rows = [];
        $lines = explode("\n", $rawData);

        foreach ($lines as $line) {
            $line = trim($line);
            if ($line === '') {
                continue;
            }

            $cols = explode("\t", $line);
            for ($i = 0; $i <= self::FLD_URL; $i++) {
                if (!isset($cols[$i])) {
                    $cols[$i] = '';
                }
            }

            $rows[] = [
                'client_ip' => (string) $cols[self::FLD_IP],
                'keep_alive_requests' => (int) $cols[self::FLD_KA_REQ_SERVED],
                'mode' => (string) $cols[self::FLD_MODE],
                'request_time' => (float) $cols[self::FLD_REQ_TIME],
                'request_time_raw' => (string) $cols[self::FLD_REQ_TIME],
                'in_bytes_current' => (int) $cols[self::FLD_IN_BYTES_CURRENT],
                'in_bytes_total' => (int) $cols[self::FLD_IN_BYTES_TOTAL],
                'out_bytes_current' => (int) $cols[self::FLD_OUT_BYTES_CURRENT],
                'out_bytes_total' => (int) $cols[self::FLD_OUT_BYTES_TOTAL],
                'vhost' => (string) $cols[self::FLD_VHOST],
                'handler' => (string) $cols[self::FLD_HANDLER],
                'extapp_process_time' => (float) $cols[self::FLD_EXTAPP_PROCESS_TIME],
                'extapp_process_time_raw' => (string) $cols[self::FLD_EXTAPP_PROCESS_TIME],
                'extapp_socket' => (string) $cols[self::FLD_EXTAPP_SOCKET],
                'extapp_pid' => (string) $cols[self::FLD_EXTAPP_PID],
                'extapp_conn_req_served' => (string) $cols[self::FLD_EXTAPP_CONN_REQ_SERVED],
                'request_url' => trim((string) $cols[self::FLD_URL], '"'),
            ];
        }

        return $rows;
    }

    protected function filterRows(array $rows, array $filters)
    {
        $filtered = [];
        $reqTimeFilter = ($filters['req_time'] !== '') ? (float) $filters['req_time'] : null;
        $procTimeFilter = ($filters['proc_time'] !== '') ? (float) $filters['proc_time'] : null;
        $inKbFilter = ($filters['in_kb'] !== '') ? (1024 * (float) $filters['in_kb']) : null;
        $outKbFilter = ($filters['out_kb'] !== '') ? (1024 * (float) $filters['out_kb']) : null;

        foreach ($rows as $row) {
            if ($reqTimeFilter !== null && $row['request_time'] <= $reqTimeFilter) {
                continue;
            }
            if ($procTimeFilter !== null && $row['extapp_process_time'] <= $procTimeFilter) {
                continue;
            }
            if ($inKbFilter !== null && $row['in_bytes_total'] <= $inKbFilter) {
                continue;
            }
            if ($outKbFilter !== null && $row['out_bytes_current'] <= $outKbFilter) {
                continue;
            }
            if (!self::matchesPattern($filters['ip'], $row['client_ip'], false)) {
                continue;
            }
            if (!self::matchesPattern($filters['vhname'], $row['vhost'], true)) {
                continue;
            }
            if (!self::matchesPattern($filters['url'], $row['request_url'], true)) {
                continue;
            }

            $filtered[] = $row;
        }

        return $filtered;
    }

    protected static function matchesPattern($pattern, $subject, $caseInsensitive)
    {
        if ($pattern === '') {
            return true;
        }

        $regex = '~' . str_replace('~', '\~', (string) $pattern) . '~' . ($caseInsensitive ? 'i' : '');
        return (@preg_match($regex, (string) $subject) === 1);
    }

    protected function sortRows(array $rows, $showSort)
    {
        usort($rows, function ($left, $right) use ($showSort) {
            switch ($showSort) {
                case 'vhname':
                    return strcasecmp($left['vhost'], $right['vhost']);

                case 'client':
                    return strcmp($left['client_ip'], $right['client_ip']);

                case 'extproc_time':
                    return $this->compareNumericDesc($left['extapp_process_time'], $right['extapp_process_time']);

                case 'in':
                    return $this->compareNumericDesc($left['in_bytes_total'], $right['in_bytes_total']);

                case 'out':
                    return $this->compareNumericDesc($left['out_bytes_current'], $right['out_bytes_current']);

                case 'req_time':
                default:
                    return $this->compareNumericDesc($left['request_time'], $right['request_time']);
            }
        });

        return $rows;
    }

    protected function compareNumericDesc($left, $right)
    {
        if ($left == $right) {
            return 0;
        }

        return ($left < $right) ? 1 : -1;
    }
}
