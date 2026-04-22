<?php

namespace LSWebAdmin\Ajax;

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\RealTimeStats;
use LSWebAdmin\UI\UIBase;

class RealtimeStatsAjax
{
    public static function plotStat()
    {
        $stat = RealTimeStats::GetPlotStats();
        $serverData = $stat->GetServerData();
        $vhostData = $stat->GetVHData();

        $response = '{"serv":' . json_encode($serverData);

        foreach ($vhostData as $vn => $vh) {
            unset($vh['ea']);
            $response .= ', "' . $vn . '":' . json_encode($vh);
        }

        $response .= '}';

        echo $response;
    }

    public static function vhStat()
    {
        $stat = RealTimeStats::GetVHStats();
        $vhostData = $stat->GetVHData();
        $monitorLabel = UIBase::EscapeAttr(DMsg::UIStr('service_addtomonitor'));

        $vbody = '';
        $ebody = '';

        foreach ($vhostData as $vn => $vh) {
            $safeVn = UIBase::Escape($vn);
            $totalCacheHits = (float) $vh[RealTimeStats::FLD_VH_TOTAL_PUB_CACHE_HITS]
                + (float) $vh[RealTimeStats::FLD_VH_TOTAL_PRIV_CACHE_HITS]
                + (float) $vh[RealTimeStats::FLD_VH_TOTAL_STATIC_HITS];
			$vbody .= '<tr><td class="lst-snapshot-actioncol"><button type="button" class="lst-snapshot-monitor-btn" data-lstmonitor="vh" data-lst-call="addMonitorTab" data-lst-call-arg="' . UIBase::EscapeAttr($vn) . '" rel="tooltip" data-placement="right" data-original-title="' . $monitorLabel . '" aria-label="' . $monitorLabel . '">'
                . '<i class="lst-icon" data-lucide="monitor-dot"></i></button></td>'
                . '<td class="lst-vhname lst-snapshot-namecell">' . $safeVn . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_REQ_PROCESSING]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_REQ_PER_SEC]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_TOT_REQS]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_EAP_COUNT]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_EAP_INUSE]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_EAP_IDLE]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_EAP_WAITQUE]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_EAP_REQ_PER_SEC]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_PUB_CACHE_HITS_PER_SEC]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($vh[RealTimeStats::FLD_VH_STATIC_HITS_PER_SEC]) . '</td>'
                . '<td class="lst-snapshot-num">' . UIBase::Escape($totalCacheHits) . '</td></tr>';

            if (isset($vh['ea']) && count($vh['ea']) > 0) {
                foreach ($vh['ea'] as $appname => $ea) {
                    $ebody .= '<tr><td class="lst-exp-scope lst-snapshot-subtle">' . $safeVn . '</td>'
                        . '<td class="lst-exp-type lst-snapshot-subtle">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_TYPE]) . '</td>'
                        . '<td class="lst-exp-name lst-snapshot-namecell">' . UIBase::Escape($appname) . '</td>'
                        . '<td class="lst-snapshot-num">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_CMAXCONN]) . '</td>'
                        . '<td class="lst-snapshot-num">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_EMAXCONN]) . '</td>'
                        . '<td class="lst-snapshot-num">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_POOL_SIZE]) . '</td>'
                        . '<td class="lst-snapshot-num">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_INUSE_CONN]) . '</td>'
                        . '<td class="lst-snapshot-num">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_IDLE_CONN]) . '</td>'
                        . '<td class="lst-snapshot-num">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_WAITQUE_DEPTH]) . '</td>'
                        . '<td class="lst-snapshot-num">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_REQ_PER_SEC]) . '</td>'
                        . '<td class="lst-snapshot-num">' . UIBase::Escape($ea[RealTimeStats::FLD_EA_TOT_REQS]) . '</td></tr>';
                }
            }
        }

        echo json_encode(['vbody' => $vbody, 'ebody' => $ebody]);
    }
}
