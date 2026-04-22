<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Log\OpsAuditLogger;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

$entries = OpsAuditLogger::readRecent(500, 2);
$logPath = OpsAuditLogger::getLogPath();
$distinctActions = OpsAuditLogger::getDistinctActions($entries);

$refreshLabel = UIBase::EscapeAttr(DMsg::UIStr('btn_refreshnow'));
$downloadLabel = UIBase::EscapeAttr(DMsg::UIStr('service_opsauditlog_download'));

$titleActions = '<button type="button" id="opsauditlog_refresh_btn" class="lst-btn lst-btn--sm lst-hero-btn lst-opsauditlog-titlebtn lst-opsauditlog-titlebtn--refresh" onclick="location.reload()" rel="tooltip" data-placement="bottom" data-original-title="' . $refreshLabel . '" aria-label="' . $refreshLabel . '">'
    . '<i class="lst-icon" data-lucide="refresh-cw" aria-hidden="true"></i>'
    . '</button>';

if (file_exists($logPath)) {
    $titleActions .= '<a href="#" id="opsauditlog_download" class="lst-btn lst-btn--sm lst-hero-btn lst-opsauditlog-titlebtn lst-opsauditlog-titlebtn--download" rel="tooltip" data-placement="bottom" data-original-title="' . $downloadLabel . '" aria-label="' . $downloadLabel . '">'
        . '<i class="lst-icon" data-lucide="download" aria-hidden="true"></i>'
        . '</a>';
}

echo UI::content_header('history', DMsg::UIStr('menu_opsauditlog'), '', true, $titleActions);
?>

<section class="lst-opsauditlog-page">

<div class="lst-opsauditlog-meta-bar">
    <span class="lst-opsauditlog-meta-bar__text"><?php echo UIBase::Escape(DMsg::UIStr('service_opsauditlog_note_prefix')); ?> <a href="index.php?view=confMgr&amp;m=admin&amp;p=sec" class="lst-opsauditlog-meta-bar__link" rel="tooltip" data-placement="bottom" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('service_opsauditlog_configure')); ?>"><?php echo UIBase::Escape(DMsg::UIStr('service_opsauditlog_note_link')); ?></a> <span class="lst-opsauditlog-meta-bar__info" rel="tooltip" data-placement="bottom" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('service_loginhistorystorage') . ': ' . $logPath); ?>"><i class="lst-icon" data-lucide="info" aria-hidden="true"></i></span></span>
</div>

<article class="lst-opsauditlog-surface" id="opsauditlog_widget">
    <div class="lst-opsauditlog-toolbar lst-table-toolbar">
        <div class="lst-table-toolbar__summary" id="opsauditlog_summary" aria-live="polite"></div>
        <div class="lst-opsauditlog-actionfilter lst-table-toolbar__meta">
            <label class="lst-opsauditlog-actionfilter__label" for="opsauditlog_action"><?php DMsg::EchoUIStr('service_opsauditlog_action'); ?></label>
            <select id="opsauditlog_action" class="lst-control-compact">
                <option value=""><?php DMsg::EchoUIStr('service_allcategories'); ?></option>
<?php foreach ($distinctActions as $act) { ?>
                <option value="<?php echo UIBase::EscapeAttr($act); ?>"><?php echo UIBase::Escape(DMsg::UIStr('service_opsauditlog_action_' . $act)); ?></option>
<?php } ?>
            </select>
        </div>
        <div class="lst-opsauditlog-rangefilter lst-table-toolbar__meta">
            <label class="lst-opsauditlog-rangefilter__label" for="opsauditlog_range"><?php DMsg::EchoUIStr('service_opsauditlog_range'); ?></label>
            <select id="opsauditlog_range" class="lst-control-compact">
                <option value=""><?php DMsg::EchoUIStr('service_opsauditlog_range_all'); ?></option>
                <option value="24h"><?php DMsg::EchoUIStr('service_opsauditlog_range_24h'); ?></option>
                <option value="7d"><?php DMsg::EchoUIStr('service_opsauditlog_range_7d'); ?></option>
                <option value="30d"><?php DMsg::EchoUIStr('service_opsauditlog_range_30d'); ?></option>
            </select>
        </div>
        <div class="lst-table-toolbar__search" id="opsauditlog_search"></div>
        <div class="lst-table-toolbar__rows" id="opsauditlog_rows"></div>
    </div>
    <div id="opsauditlog_empty" class="lst-opsauditlog-empty"<?php echo !empty($entries) ? ' hidden' : ''; ?>>
        <i class="lst-icon" data-lucide="circle-check-big" aria-hidden="true"></i>
        <span><?php DMsg::EchoUIStr('service_opsauditlogempty'); ?></span>
    </div>
    <table id="opsauditlog_table" class="table lst-data-grid-table lst-data-grid-table--interactive lst-opsauditlog-table" width="100%"<?php echo empty($entries) ? ' hidden' : ''; ?>>
        <thead>
        <tr>
            <th><?php DMsg::EchoUIStr('service_time'); ?></th>
            <th><?php DMsg::EchoUIStr('service_opsauditlog_action'); ?></th>
            <th><?php DMsg::EchoUIStr('l_user'); ?></th>
            <th><?php DMsg::EchoUIStr('l_ip0'); ?></th>
            <th><?php DMsg::EchoUIStr('service_opsauditlog_target'); ?></th>
            <th><?php DMsg::EchoUIStr('service_opsauditlog_detail'); ?></th>
        </tr>
        </thead>
        <tbody>
<?php foreach ($entries as $entry) {
    $ts = isset($entry['ts']) ? (string) $entry['ts'] : '';
    $action = isset($entry['action']) ? (string) $entry['action'] : '';
    $user = isset($entry['user']) ? (string) $entry['user'] : '';
    $ip = isset($entry['ip']) ? (string) $entry['ip'] : '';
    $target = isset($entry['target']) ? (string) $entry['target'] : '';
    $detail = isset($entry['detail']) ? (string) $entry['detail'] : '';
    $epoch = ($ts !== '') ? strtotime($ts) : 0;
    $actionLabel = DMsg::UIStr('service_opsauditlog_action_' . $action);
?>
        <tr data-action-code="<?php echo UIBase::EscapeAttr($action); ?>">
            <td data-order="<?php echo UIBase::EscapeAttr((string) $epoch); ?>"><?php echo UIBase::Escape($ts !== '' ? gmdate('Y-m-d H:i:s', $epoch) . ' UTC' : ''); ?></td>
            <td><span class="lst-opsauditlog-action lst-opsauditlog-action--<?php echo UIBase::EscapeAttr($action); ?>"><?php echo UIBase::Escape($actionLabel); ?></span></td>
            <td><?php echo UIBase::Escape($user); ?></td>
            <td><code class="lst-opsauditlog-ip"><?php echo UIBase::Escape($ip); ?></code></td>
            <td><code class="lst-opsauditlog-target"><?php echo UIBase::Escape($target); ?></code></td>
            <td class="lst-opsauditlog-detail"><?php echo UIBase::Escape($detail); ?></td>
        </tr>
<?php } ?>
        </tbody>
    </table>
</article>

</section>

<script type="text/javascript">
    pageSetUp();

    var opsAuditLogPageText = {
        showing: <?php echo json_encode(DMsg::UIStr('note_showing')); ?>,
        rows: <?php echo json_encode(DMsg::UIStr('note_rows')); ?>,
        filter: <?php echo json_encode(DMsg::UIStr('note_filter')); ?>
    };

    function formatOpsAuditLogSummary(api) {
        var info = api ? api.page.info() : null,
            visible = 0,
            total = 0;

        if (!info) {
            return opsAuditLogPageText.showing + " 0 / 0";
        }

        visible = Math.max(info.end - info.start, 0);
        total = Math.max(parseInt(info.recordsDisplay, 10) || 0, 0);

        return opsAuditLogPageText.showing + " " + visible.toLocaleString() + " / " + total.toLocaleString();
    }

    function syncOpsAuditLogControls(tableSelector, summarySelector, searchSelector, rowsSelector) {
        var api = lstGetDataTableApi(tableSelector),
            wrapper = $(tableSelector).closest(".dt-container"),
            search = wrapper.find(".dt-search").first(),
            length = wrapper.find(".dt-length").first();

        if (search.length && search.parent()[0] !== $(searchSelector)[0]) {
            $(searchSelector).append(search);
        }

        if (length.length && length.parent()[0] !== $(rowsSelector)[0]) {
            $(rowsSelector).append(length);
        }

        $(summarySelector).text(formatOpsAuditLogSummary(api));
    }

    function bindOpsAuditLogChrome(tableSelector, summarySelector, searchSelector, rowsSelector) {
        lstTuneDataTableChrome(tableSelector, {
            placeholder: opsAuditLogPageText.filter,
            lengthLabel: opsAuditLogPageText.rows,
            bindDraw: true,
            onUpdate: function () {
                syncOpsAuditLogControls(tableSelector, summarySelector, searchSelector, rowsSelector);
            }
        });

        syncOpsAuditLogControls(tableSelector, summarySelector, searchSelector, rowsSelector);
    }

    function installOpsAuditLogActionFilter() {
        if (window.lstOpsAuditLogActionFilterInstalled || !$.fn.dataTable || !$.fn.dataTable.ext || !$.fn.dataTable.ext.search) {
            return;
        }

        $.fn.dataTable.ext.search.push(function (settings, data, dataIndex) {
            var selectedAction = $("#opsauditlog_action").val() || "",
                table = document.getElementById("opsauditlog_table"),
                row;

            if (!table || settings.nTable !== table || selectedAction === "") {
                return true;
            }

            row = settings.aoData && settings.aoData[dataIndex] ? settings.aoData[dataIndex].nTr : null;
            return ($(row).attr("data-action-code") || "") === selectedAction;
        });

        window.lstOpsAuditLogActionFilterInstalled = true;
    }

    function installOpsAuditLogRangeFilter() {
        if (window.lstOpsAuditLogRangeFilterInstalled || !$.fn.dataTable || !$.fn.dataTable.ext || !$.fn.dataTable.ext.search) {
            return;
        }

        $.fn.dataTable.ext.search.push(function (settings, data, dataIndex) {
            var selectedRange = $("#opsauditlog_range").val() || "",
                table = document.getElementById("opsauditlog_table"),
                cutoff, epoch, row, td;

            if (!table || settings.nTable !== table || selectedRange === "") {
                return true;
            }

            cutoff = 0;
            var now = Math.floor(Date.now() / 1000);
            if (selectedRange === "24h") {
                cutoff = now - 86400;
            } else if (selectedRange === "7d") {
                cutoff = now - 604800;
            } else if (selectedRange === "30d") {
                cutoff = now - 2592000;
            }

            if (cutoff === 0) {
                return true;
            }

            row = settings.aoData && settings.aoData[dataIndex] ? settings.aoData[dataIndex].nTr : null;
            if (!row) {
                return true;
            }
            td = $(row).find("td").eq(0);
            epoch = parseInt(td.attr("data-order") || "0", 10);
            return epoch >= cutoff;
        });

        window.lstOpsAuditLogRangeFilterInstalled = true;
    }

    var pagefunction = function () {
        lstLoadDataTableAssets(function () {
            installOpsAuditLogActionFilter();
            installOpsAuditLogRangeFilter();

            if ($("#opsauditlog_table tbody tr").length) {
                $("#opsauditlog_table").DataTable({
                    autoWidth: false,
                    info: false,
                    lengthChange: true,
                    order: [[0, "desc"]],
                    pageLength: 10
                });
                bindOpsAuditLogChrome("#opsauditlog_table", "#opsauditlog_summary", "#opsauditlog_search", "#opsauditlog_rows");
                $("#opsauditlog_action, #opsauditlog_range").on("change", function () {
                    $("#opsauditlog_table").DataTable().draw();
                });
            } else {
                $("#opsauditlog_summary").text(opsAuditLogPageText.showing + " 0 / 0");
            }
        });

        // Download handler — serves the raw log file as a text download.
        $("#opsauditlog_download").on("click", function (e) {
            e.preventDefault();
            var table = $("#opsauditlog_table").DataTable(),
                rows = table ? table.rows({search: "applied"}).data().toArray() : [],
                lines = [],
                i, cells;

            // Export what the user sees (filtered) as JSON-per-line.
            var trNodes = table ? table.rows({search: "applied"}).nodes() : [];
            for (i = 0; i < trNodes.length; i++) {
                var tr = trNodes[i],
                    entry = {
                        ts: $(tr).find("td").eq(0).text().replace(" UTC", "").replace(" ", "T") + "Z",
                        action: $(tr).attr("data-action-code") || "",
                        user: $(tr).find("td").eq(2).text(),
                        ip: $(tr).find("td").eq(3).text(),
                        target: $(tr).find("td").eq(4).text(),
                        detail: $(tr).find("td").eq(5).text()
                    };
                lines.push(JSON.stringify(entry));
            }

            if (lines.length === 0) {
                return;
            }

            var blob = new Blob([lines.join("\n") + "\n"], {type: "text/plain"});
            var url = URL.createObjectURL(blob);
            var a = document.createElement("a");
            a.href = url;
            var now = new Date(),
                pad = function (n) { return n < 10 ? "0" + n : "" + n; },
                stamp = now.getFullYear()
                    + pad(now.getMonth() + 1)
                    + pad(now.getDate()) + "_"
                    + pad(now.getHours())
                    + pad(now.getMinutes())
                    + pad(now.getSeconds());
            a.download = "ops_audit_" + stamp + ".log";
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);
        });
    };

    pagefunction();
</script>
