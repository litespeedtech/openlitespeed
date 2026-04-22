<?php

namespace LSWebAdmin\UI;

use LSWebAdmin\I18n\DMsg;

class RealtimeStatsRenderer
{
    public static function renderPlotTab($tabId, $bottomDef, $plotDef, $tabStatus)
    {
        $buf = '<div id="' . $tabId . '"';
        if ($tabStatus != -1) {
            $buf .= ' class="lst-tab-view lst-tab-view--chart';
            if ($tabStatus == 1) {
                $buf .= ' is-active';
            }
            $buf .= '" aria-hidden="' . (($tabStatus == 1) ? 'false' : 'true') . '"';
        }
        $buf .= '><div class="lst-widget-body-toolbar lst-plot-toolbar lst-plot-toggles" id="rev-toggles"><div class="lst-plot-toggle-group">';

        foreach ($plotDef as $definition) {
            $buf .= '<label class="lst-plot-toggle"><input type="checkbox" ';
            if ($definition[2]) {
                $buf .= 'checked="checked" ';
            }
            $buf .= 'data-lst-stat-idx="' . $definition[0] . '"><i></i><span>' . $definition[1] . "</span></label>\n";
        }

        $buf .= '</div></div>
                <div class="lst-widget-body lst-plot-body">
                    <div class="lst-live-stage">
                        <div class="chart-large lst-live-chart lst-live-chart--accent"></div>
                    </div>
                ';

        $buf .= '<div class="show-stat-microcharts lst-stat-grid" style="--lst-stat-grid-columns: ' . count($bottomDef) . ';">';

        foreach ($bottomDef as $buttonGroup) {
            $buf .= '<div class="lst-stat-grid__group">';
            foreach ($buttonGroup as $definition) {
                $tip = isset($definition[4]) ? $definition[4] : '';
                $buf .= self::renderStatBottomRow($definition[0], $definition[1], $definition[2], $definition[3], $tip);
            }
            $buf .= "</div>\n";
        }

        $buf .= "</div>\n";

        $buf .= '</div>
        </div>';
        return $buf;
    }

    private static function renderStatBottomRow($seq, $label, $textColor, $maxColor, $tip)
    {
        if ($tip && ($helpItem = DMsg::GetAttrTip($tip)) != null) {
            $help = $helpItem->Render();
        } else {
            $help = '';
        }

        $buf = '<div class="lst-stat-line">' . $label . ' ' . $help . ' <span class="lst-stat-val';
        if ($textColor != '') {
            $buf .= ' lst-tone-' . $textColor;
        }
        $buf .= '" data-lst-stat-idx="' . $seq . '"></span>';
        if ($maxColor != '') {
            $buf .= '<div class="smaller-stat lst-hide-tablet-range lst-stat-meta"><span class="lst-stat-max lst-stat-max--'
                    . $maxColor . '" hidden data-lst-stat-idx="' . $seq . '"></span></div>';
        }
        $buf .= "</div>\n";
        return $buf;
    }
}
