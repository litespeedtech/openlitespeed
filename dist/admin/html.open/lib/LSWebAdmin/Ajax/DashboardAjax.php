<?php

namespace LSWebAdmin\Ajax;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Log\LogFilter;
use LSWebAdmin\Product\Current\Product;
use LSWebAdmin\Product\Current\RealTimeStats;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Runtime\SInfo;
use LSWebAdmin\UI\UIBase;

class DashboardAjax
{
    public static function pidLoad()
    {
        $pid = Service::RuntimePid();
        $loadAverage = self::getLoadAverage();

        $data = [
            'pid' => $pid,
            'running' => ($pid !== ''),
            'serverload' => count($loadAverage) ? implode(', ', $loadAverage) : 'N/A',
        ];

        echo json_encode($data);
    }

    public static function dashStat()
    {
        echo json_encode(RealTimeStats::GetDashPlot());
    }

    public static function dashSummary()
    {
        $stats = RealTimeStats::GetDashPlot();
        $product = Product::GetInstance();
        $status = self::buildStatusPayload(false);
        $pid = trim((string) Service::ServiceData(SInfo::DATA_PID));
        $loadAverage = self::getLoadAverage();
        $reqPerSec = isset($stats[RealTimeStats::FLD_S_REQ_PER_SEC]) ? (float) $stats[RealTimeStats::FLD_S_REQ_PER_SEC] : 0;
        $reqProcessing = isset($stats[RealTimeStats::FLD_S_REQ_PROCESSING]) ? (int) $stats[RealTimeStats::FLD_S_REQ_PROCESSING] : 0;
        $pubCachePerSec = isset($stats[RealTimeStats::FLD_S_PUB_CACHE_HITS_PER_SEC]) ? (float) $stats[RealTimeStats::FLD_S_PUB_CACHE_HITS_PER_SEC] : 0;
        $privCachePerSec = isset($stats[RealTimeStats::FLD_S_PRIV_CACHE_HITS_PER_SEC]) ? (float) $stats[RealTimeStats::FLD_S_PRIV_CACHE_HITS_PER_SEC] : 0;
        $staticHitsPerSec = isset($stats[RealTimeStats::FLD_S_STATIC_HITS_PER_SEC]) ? (float) $stats[RealTimeStats::FLD_S_STATIC_HITS_PER_SEC] : 0;
        $plainConn = isset($stats[RealTimeStats::FLD_PLAINCONN]) ? (int) $stats[RealTimeStats::FLD_PLAINCONN] : 0;
        $maxConn = isset($stats[RealTimeStats::FLD_MAXCONN]) ? (int) $stats[RealTimeStats::FLD_MAXCONN] : 0;
        $sslConn = isset($stats[RealTimeStats::FLD_SSLCONN]) ? (int) $stats[RealTimeStats::FLD_SSLCONN] : 0;
        $maxSslConn = isset($stats[RealTimeStats::FLD_MAXSSL_CONN]) ? (int) $stats[RealTimeStats::FLD_MAXSSL_CONN] : 0;
        $bpsIn = (isset($stats[RealTimeStats::FLD_BPS_IN]) ? (float) $stats[RealTimeStats::FLD_BPS_IN] : 0)
            + (isset($stats[RealTimeStats::FLD_SSL_BPS_IN]) ? (float) $stats[RealTimeStats::FLD_SSL_BPS_IN] : 0);
        $bpsOut = (isset($stats[RealTimeStats::FLD_BPS_OUT]) ? (float) $stats[RealTimeStats::FLD_BPS_OUT] : 0)
            + (isset($stats[RealTimeStats::FLD_SSL_BPS_OUT]) ? (float) $stats[RealTimeStats::FLD_SSL_BPS_OUT] : 0);
        $blockedIpCount = isset($stats[RealTimeStats::FLD_BLOCKEDIP_COUNT]) ? (int) $stats[RealTimeStats::FLD_BLOCKEDIP_COUNT] : 0;
        $versionDisplay = trim($product->getProductName() . ' ' . $product->getVersion());
        if ($versionDisplay === '') {
            $versionDisplay = $product->getProductName();
        }

        $res = [
            'pid' => $pid,
            'running' => ($pid !== ''),
            'uptime' => isset($stats[RealTimeStats::FLD_UPTIME]) ? trim((string) $stats[RealTimeStats::FLD_UPTIME]) : '',
            'load_average' => $loadAverage,
            'load_average_display' => count($loadAverage) ? implode(' / ', $loadAverage) : 'N/A',
            'product_name' => $product->getProductName(),
            'version' => $product->getVersion(),
            'version_display' => $versionDisplay,
            'req_per_sec' => $reqPerSec,
            'req_per_sec_display' => self::formatRate($reqPerSec),
            'req_processing' => $reqProcessing,
            'avail_conn' => isset($stats[RealTimeStats::FLD_AVAILCONN]) ? (int) $stats[RealTimeStats::FLD_AVAILCONN] : 0,
            'pub_cache_per_sec' => $pubCachePerSec,
            'priv_cache_per_sec' => $privCachePerSec,
            'static_hits_per_sec' => $staticHitsPerSec,
            'total_cache_per_sec' => $pubCachePerSec + $privCachePerSec,
            'plain_conn' => $plainConn,
            'max_conn' => $maxConn,
            'ssl_conn' => $sslConn,
            'max_ssl_conn' => $maxSslConn,
            'bps_in' => $bpsIn,
            'bps_out' => $bpsOut,
            'throughput_in_display' => self::formatThroughput($bpsIn),
            'throughput_out_display' => self::formatThroughput($bpsOut),
            'blocked_ip_count' => $blockedIpCount,
            'l_running' => $status['l_running'],
            'l_broken' => $status['l_broken'],
            'v_running' => $status['v_running'],
            'v_disabled' => $status['v_disabled'],
            'v_err' => $status['v_err'],
        ];

        echo json_encode($res);
    }

    public static function dashStatus()
    {
        echo json_encode(self::buildStatusPayload(true));
    }

    public static function dashLog()
    {
        $logfilter = Service::ServiceData(SInfo::DATA_DASH_LOG);
        $debug = Service::ServiceData(SInfo::DATA_DEBUGLOG_STATE);

        $res = [
            'debuglog' => $debug,
            'logfound' => $logfilter->Get(LogFilter::FLD_TOTALFOUND),
            'logsearched' => $logfilter->Get(LogFilter::FLD_TOTALSEARCHED),
            'loglevel' => $logfilter->Get(LogFilter::FLD_LEVEL),
            'logfoundmesg' => $logfilter->Get(LogFilter::FLD_OUTMESG),
            'log_body' => $logfilter->GetLogOutput(),
        ];

        echo json_encode($res);
    }

    private static function getLoadAverage()
    {
        $avgload = sys_getloadavg();
        if ($avgload === false || !is_array($avgload)) {
            return [];
        }

        return array_map(
            function ($load) {
                return round((float) $load, 3);
            },
            array_slice($avgload, 0, 3)
        );
    }

    private static function formatRate($value, $precision = 1)
    {
        $value = (float) $value;
        if ((int) $value == $value) {
            return number_format($value, 0);
        }

        return number_format($value, $precision);
    }

    private static function formatThroughput($kilobytesPerSecond)
    {
        $value = max(0.0, (float) $kilobytesPerSecond);
        $unit = 'KB';

        if ($value >= 1024) {
            $value /= 1024;
            $unit = 'MB';
        }

        if ($value >= 1024) {
            $value /= 1024;
            $unit = 'GB';
        }

        $precision = ($value >= 10 || (int) $value == $value) ? 0 : 1;

        return number_format($value, $precision) . ' ' . $unit;
    }

    private static function buildStatusPayload($includeTables = true)
    {
        $sinfo = Service::ServiceData(SInfo::DATA_Status_LV);
        $listeners = $sinfo->Get(SInfo::FLD_Listener);
        $noteListenerRunning = DMsg::ALbl('service_running');
        $noteListenerNotRunning = DMsg::ALbl('service_notrunning');
        $viewConfigLabel = DMsg::UIStr('btn_viewconfig');
        $maxVhDisplay = 5;

        $listenerBody = '';
        $running = 0;
        $broken = 0;
        foreach ($listeners as $lname => $listener) {
            $safeName = UIBase::Escape($lname);
            $listenerConfigUrl = 'index.php?view=confMgr&m=' . rawurlencode('sl_' . $lname);
            $listenerNameCell = '<span class="lst-status-namecell"><span class="lst-status-name">' . $safeName . '</span>'
                . '<a class="lst-status-config-link" href="' . UIBase::EscapeAttr($listenerConfigUrl)
                . '" rel="tooltip" data-placement="top" data-original-title="' . UIBase::EscapeAttr($viewConfigLabel)
                . '" aria-label="' . UIBase::EscapeAttr($viewConfigLabel) . '"><i class="lst-icon" data-lucide="arrow-up-right"></i></a></span>';
            $listenerRow = '<tr><td class="lst-listener-name">' . $listenerNameCell . '</td><td>';
            if (isset($listener['addr'])) {
                $running++;
                $addr = UIBase::Escape($listener['addr']);
                $state = '<span class="lst-vh-state lst-vh-state--success">'
                    . UIBase::Escape($noteListenerRunning)
                    . '</span>';
            } else {
                $broken++;
                $addr = UIBase::Escape($listener['daddr']);
                $state = '<span class="lst-vh-state lst-vh-state--danger">'
                    . UIBase::Escape($noteListenerNotRunning)
                    . '</span>';
            }
            $listenerRow .= $addr . '</td>';

            // Mapped VHosts column
            $mappedVhNames = isset($listener['map']) ? array_keys($listener['map']) : [];
            $vhCount = count($mappedVhNames);
            $listenerRow .= '<td class="lst-listener-vhmap">';
            if ($vhCount === 0) {
                $listenerRow .= '<span class="lst-cell-empty">&mdash;</span>';
            } else {
                $displayNames = array_slice($mappedVhNames, 0, $maxVhDisplay);
                $listenerRow .= '<span class="lst-listener-vhmap-names">'
                    . UIBase::Escape(implode(', ', $displayNames))
                    . '</span>';
                if ($vhCount > $maxVhDisplay) {
                    $listenerMapUrl = 'index.php?view=listenervhmap&tab=listeners&listener=' . rawurlencode($lname);
                    $listenerRow .= ' <a class="lst-listener-vhmap-more" href="'
                        . UIBase::EscapeAttr($listenerMapUrl)
                        . '">... (' . $vhCount . ' total)</a>';
                }
            }
            $listenerRow .= '</td>';

            // VH Count column
            $listenerRow .= '<td class="lst-listener-vhcount">' . $vhCount . '</td>';

            $listenerRow .= '<td class="lst-state-actioncol"><div class="lst-vh-statecell">'
                . $state
                . '</div></td></tr>' . "\n";

            if ($includeTables) {
                $listenerBody .= $listenerRow;
            }
        }

        $vhosts = $sinfo->Get(SInfo::FLD_VHosts);
        $runningVhosts = 0;
        $disabledVhosts = 0;
        $errorVhosts = 0;
        $vhostBody = '';

        $noteError = DMsg::ALbl('service_error');
        $noteActive = DMsg::ALbl('service_active');
        $noteSuspendVh = DMsg::ALbl('btn_suspend');
        $noteEnableVh = DMsg::ALbl('btn_resume');
        $noteDisabled = DMsg::ALbl('service_disabled');

        foreach ($vhosts as $vn => $vh) {
            $safeVn = UIBase::Escape($vn);
            $vhostConfigUrl = 'index.php?view=confMgr&m=' . rawurlencode('vh_' . $vn);
            $vhostNameCell = '<span class="lst-status-namecell"><span class="lst-status-name">' . $safeVn . '</span>'
                . '<a class="lst-status-config-link" href="' . UIBase::EscapeAttr($vhostConfigUrl)
                . '" rel="tooltip" data-placement="top" data-original-title="' . UIBase::EscapeAttr($viewConfigLabel)
                . '" aria-label="' . UIBase::EscapeAttr($viewConfigLabel) . '"><i class="lst-icon" data-lucide="arrow-up-right"></i></a></span>';
            $vhostRow = '<tr data-vn="' . UIBase::EscapeAttr($vn) . '"><td class="lst-vh-name">' . $vhostNameCell . '</td><td class="lst-vh-template">';

            if (isset($vh['templ']) && $vh['templ'] !== '') {
                $vhostRow .= UIBase::Escape($vh['templ']);
            } else {
                $vhostRow .= '<span class="lst-cell-empty">&mdash;</span>';
            }

            $vhostRow .= '</td><td class="lst-vh-domains-cell">';
            if (isset($vh['domains']) && count($vh['domains']) > 0) {
                $vhostRow .= '<span class="lst-vh-domains">' . UIBase::Escape(implode(', ', array_keys($vh['domains']))) . '</span>';
            } else {
                $vhostRow .= '<span class="lst-cell-empty">&mdash;</span>';
            }

            $vhostRow .= '</td><td class="lst-state-actioncol"><div class="lst-vh-statecell">';

            if ($vh['running'] == -1) {
                $errorVhosts++;
                $vhostRow .= '<span class="lst-vh-state lst-vh-state--danger">'
                    . UIBase::Escape($noteError)
                    . '</span>';
                $actions = '';
            } elseif ($vh['running'] == 1) {
                $runningVhosts++;
                $vhostRow .= '<span class="lst-vh-state lst-vh-state--success">'
                    . UIBase::Escape($noteActive)
                    . '</span>';
                $actions = '<a class="lst-inline-action lst-inline-action--pause" data-action="lstvhcontrol" data-lstact="disable">'
                    . UIBase::Escape($noteSuspendVh) . '</a>';
            } else {
                $disabledVhosts++;
                $vhostRow .= '<span class="lst-vh-state lst-vh-state--warning">'
                    . UIBase::Escape($noteDisabled)
                    . '</span>';
                $actions = '<a class="lst-inline-action lst-inline-action--play" data-action="lstvhcontrol" data-lstact="enable">'
                    . UIBase::Escape($noteEnableVh) . '</a>';
            }

            if ($actions !== '') {
                $vhostRow .= $actions;
            }

            $vhostRow .= '</div></td></tr>';

            if ($includeTables) {
                $vhostBody .= $vhostRow;
            }
        }

        $res = [
            'l_running' => $running,
            'l_broken' => $broken,
            'v_running' => $runningVhosts,
            'v_disabled' => $disabledVhosts,
            'v_err' => $errorVhosts,
        ];

        if ($includeTables) {
            $res['l_body'] = $listenerBody;
            $res['v_body'] = $vhostBody;
        }

        return $res;
    }
}
