<?php

use LSWebAdmin\Auth\CAuthorizer;
use LSWebAdmin\Auth\LoginHistory;
use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

$loginHistory = CAuthorizer::GetLoginHistory();
$successfulEntries = $loginHistory->getRecentSuccessful(null);
$failedEntries = $loginHistory->getRecentFailed(null);
$retentionDays = $loginHistory->getRetentionDays();
$historyStoragePath = $loginHistory->getStoragePath();

echo UI::content_header('history', DMsg::UIStr('menu_loginhistory'));
?>

<section class="lst-loginhistory-page">

<div class="lst-loginhistory-meta-bar">
    <span class="lst-loginhistory-meta-bar__text" rel="tooltip" data-placement="bottom" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('service_loginhistorystorage') . ': ' . $historyStoragePath); ?>"><?php echo UIBase::Escape(DMsg::UIStr('service_loginhistoryretention', ['%%days%%' => $retentionDays])); ?> <i class="lst-icon" data-lucide="info" aria-hidden="true"></i></span>
</div>

<article>
    <div class="lst-widget lst-loginhistory-widget" id="loginhistory_success_widget">
        <header class="lst-widget-header">
            <span class="lst-widget-icon"><i class="lst-icon" data-lucide="badge-check"></i></span>
            <h2><?php DMsg::EchoUIStr('service_loginhistorysuccessful'); ?></h2>
            <div class="lst-widget-toolbar lst-loginhistory-header-meta" role="status" aria-live="polite">
                <span class="lst-status-pill lst-status-pill--success">
                    <span class="lst-status-count"><?php echo UIBase::Escape((string) count($successfulEntries)); ?></span>
                    <span class="lst-status-text"><?php DMsg::EchoUIStr('note_rows'); ?></span>
                </span>
            </div>
        </header>
        <div class="lst-widget-content">
            <div class="lst-widget-body">
                <div class="lst-loginhistory-toolbar lst-table-toolbar">
                    <div class="lst-loginhistory-summary lst-table-toolbar__summary" id="loginhistory_success_summary" aria-live="polite"></div>
                    <div class="lst-loginhistory-search-slot lst-table-toolbar__search" id="loginhistory_success_search"></div>
                    <div class="lst-loginhistory-rows-slot lst-table-toolbar__rows" id="loginhistory_success_rows"></div>
                </div>
                <div id="loginhistory_success_empty" class="lst-loginhistory-empty"<?php echo !empty($successfulEntries) ? ' hidden' : ''; ?>>
                    <i class="lst-icon" data-lucide="circle-check-big" aria-hidden="true"></i>
                    <span><?php DMsg::EchoUIStr('service_loginhistoryemptysuccess'); ?></span>
                </div>
                <table id="loginhistory_success_table" class="table lst-data-grid-table lst-data-grid-table--interactive lst-loginhistory-table" width="100%"<?php echo empty($successfulEntries) ? ' hidden' : ''; ?>>
                    <thead>
                    <tr>
                        <th><?php DMsg::EchoUIStr('service_time'); ?></th>
                        <th><?php DMsg::EchoUIStr('l_user'); ?></th>
                        <th><?php DMsg::EchoUIStr('l_ip0'); ?></th>
                    </tr>
                    </thead>
                    <tbody>
<?php foreach ($successfulEntries as $entry) { ?>
                    <tr>
                        <td data-order="<?php echo UIBase::EscapeAttr((string) (isset($entry['time']) ? (int) $entry['time'] : 0)); ?>"><?php echo UIBase::Escape(gmdate('Y-m-d H:i:s', isset($entry['time']) ? (int) $entry['time'] : 0) . ' UTC'); ?></td>
                        <td><?php echo UIBase::Escape(isset($entry['user']) ? (string) $entry['user'] : ''); ?></td>
                        <td><code class="lst-loginhistory-ip"><?php echo UIBase::Escape(isset($entry['ip']) ? (string) $entry['ip'] : ''); ?></code></td>
                    </tr>
<?php } ?>
                    </tbody>
                </table>
            </div>
        </div>
    </div>
</article>

<article>
    <div class="lst-widget lst-loginhistory-widget" id="loginhistory_failed_widget">
        <header class="lst-widget-header">
            <span class="lst-widget-icon"><i class="lst-icon" data-lucide="shield-alert"></i></span>
            <h2><?php DMsg::EchoUIStr('service_loginhistoryfailed'); ?></h2>
            <div class="lst-widget-toolbar lst-loginhistory-header-meta" role="status" aria-live="polite">
                <span class="lst-status-pill lst-status-pill--danger">
                    <span class="lst-status-count"><?php echo UIBase::Escape((string) count($failedEntries)); ?></span>
                    <span class="lst-status-text"><?php DMsg::EchoUIStr('note_rows'); ?></span>
                </span>
            </div>
        </header>
        <div class="lst-widget-content">
            <div class="lst-widget-body">
                <div class="lst-loginhistory-toolbar lst-table-toolbar">
                    <div class="lst-loginhistory-summary lst-table-toolbar__summary" id="loginhistory_failed_summary" aria-live="polite"></div>
                    <div class="lst-loginhistory-reasonfilter lst-table-toolbar__meta">
                        <label class="lst-loginhistory-reasonfilter__label" for="loginhistory_failed_reason"><?php DMsg::EchoUIStr('service_reason'); ?></label>
                        <select id="loginhistory_failed_reason" class="lst-control-compact">
                            <option value=""><?php DMsg::EchoUIStr('service_allcategories'); ?></option>
                            <option value="invalid_credentials"><?php DMsg::EchoUIStr('service_loginhistoryreason_invalid_credentials'); ?></option>
                            <option value="throttled"><?php DMsg::EchoUIStr('service_loginhistoryreason_throttled'); ?></option>
                            <option value="abuse"><?php DMsg::EchoUIStr('service_loginhistoryreason_abuse'); ?></option>
                        </select>
                    </div>
                    <div class="lst-loginhistory-search-slot lst-table-toolbar__search" id="loginhistory_failed_search"></div>
                    <div class="lst-loginhistory-rows-slot lst-table-toolbar__rows" id="loginhistory_failed_rows"></div>
                </div>
                <div id="loginhistory_failed_empty" class="lst-loginhistory-empty"<?php echo !empty($failedEntries) ? ' hidden' : ''; ?>>
                    <i class="lst-icon" data-lucide="circle-check-big" aria-hidden="true"></i>
                    <span><?php DMsg::EchoUIStr('service_loginhistoryemptyfailed'); ?></span>
                </div>
                <table id="loginhistory_failed_table" class="table lst-data-grid-table lst-data-grid-table--interactive lst-loginhistory-table" width="100%"<?php echo empty($failedEntries) ? ' hidden' : ''; ?>>
                    <thead>
                    <tr>
                        <th><?php DMsg::EchoUIStr('service_time'); ?></th>
                        <th><?php DMsg::EchoUIStr('l_user'); ?></th>
                        <th><?php DMsg::EchoUIStr('l_ip0'); ?></th>
                        <th><?php DMsg::EchoUIStr('service_reason'); ?></th>
                    </tr>
                    </thead>
                    <tbody>
<?php foreach ($failedEntries as $entry) { $reason = isset($entry['reason']) ? (string) $entry['reason'] : ''; ?>
                    <tr data-reason-code="<?php echo UIBase::EscapeAttr($reason); ?>">
                        <td data-order="<?php echo UIBase::EscapeAttr((string) (isset($entry['time']) ? (int) $entry['time'] : 0)); ?>"><?php echo UIBase::Escape(gmdate('Y-m-d H:i:s', isset($entry['time']) ? (int) $entry['time'] : 0) . ' UTC'); ?></td>
                        <td><?php echo UIBase::Escape(isset($entry['user']) ? (string) $entry['user'] : ''); ?></td>
                        <td><code class="lst-loginhistory-ip"><?php echo UIBase::Escape(isset($entry['ip']) ? (string) $entry['ip'] : ''); ?></code></td>
                        <td><?php echo UIBase::Escape(DMsg::UIStr('service_loginhistoryreason_' . $reason)); ?></td>
                    </tr>
<?php } ?>
                    </tbody>
                </table>
<?php if (!empty($failedEntries)) { ?>
                <div class="lst-loginhistory-failed-note">
                    <a href="index.php?view=confMgr&amp;m=admin&amp;p=sec"><?php echo UIBase::Escape(DMsg::UIStr('service_loginhistoryfailednote')); ?></a>
                </div>
<?php } ?>
            </div>
        </div>
    </div>
</article>

</section>

<script type="text/javascript">
    pageSetUp();

    var loginHistoryPageText = {
        showing: <?php echo json_encode(DMsg::UIStr('note_showing')); ?>,
        rows: <?php echo json_encode(DMsg::UIStr('note_rows')); ?>,
        filter: <?php echo json_encode(DMsg::UIStr('note_filter')); ?>
    };

    function formatLoginHistorySummary(api) {
        var info = api ? api.page.info() : null,
            visible = 0,
            total = 0;

        if (!info) {
            return loginHistoryPageText.showing + " 0 / 0";
        }

        visible = Math.max(info.end - info.start, 0);
        total = Math.max(parseInt(info.recordsDisplay, 10) || 0, 0);

        return loginHistoryPageText.showing + " " + visible.toLocaleString() + " / " + total.toLocaleString();
    }

    function syncLoginHistoryControls(tableSelector, summarySelector, searchSelector, rowsSelector) {
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

        $(summarySelector).text(formatLoginHistorySummary(api));
    }

    function bindLoginHistoryChrome(tableSelector, summarySelector, searchSelector, rowsSelector) {
        lstTuneDataTableChrome(tableSelector, {
            placeholder: loginHistoryPageText.filter,
            lengthLabel: loginHistoryPageText.rows,
            bindDraw: true,
            onUpdate: function () {
                syncLoginHistoryControls(tableSelector, summarySelector, searchSelector, rowsSelector);
            }
        });

        syncLoginHistoryControls(tableSelector, summarySelector, searchSelector, rowsSelector);
    }

    function installFailedReasonFilter() {
        if (window.lstLoginHistoryReasonFilterInstalled || !$.fn.dataTable || !$.fn.dataTable.ext || !$.fn.dataTable.ext.search) {
            return;
        }

        $.fn.dataTable.ext.search.push(function (settings, data, dataIndex) {
            var selectedReason = $("#loginhistory_failed_reason").val() || "",
                table = document.getElementById("loginhistory_failed_table"),
                row;

            if (!table || settings.nTable !== table || selectedReason === "") {
                return true;
            }

            row = settings.aoData && settings.aoData[dataIndex] ? settings.aoData[dataIndex].nTr : null;
            return ($(row).attr("data-reason-code") || "") === selectedReason;
        });

        window.lstLoginHistoryReasonFilterInstalled = true;
    }

    var pagefunction = function () {
        lstLoadDataTableAssets(function () {
            if ($("#loginhistory_success_table tbody tr").length) {
                $("#loginhistory_success_table").DataTable({
                    autoWidth: false,
                    info: false,
                    order: [[0, "desc"]],
                    pageLength: 10
                });
                bindLoginHistoryChrome("#loginhistory_success_table", "#loginhistory_success_summary", "#loginhistory_success_search", "#loginhistory_success_rows");
            } else {
                $("#loginhistory_success_summary").text(loginHistoryPageText.showing + " 0 / 0");
            }

            installFailedReasonFilter();

            if ($("#loginhistory_failed_table tbody tr").length) {
                $("#loginhistory_failed_table").DataTable({
                    autoWidth: false,
                    info: false,
                    order: [[0, "desc"]],
                    pageLength: 10
                });
                bindLoginHistoryChrome("#loginhistory_failed_table", "#loginhistory_failed_summary", "#loginhistory_failed_search", "#loginhistory_failed_rows");
                $("#loginhistory_failed_reason").on("change", function () {
                    $("#loginhistory_failed_table").DataTable().draw();
                });
            } else {
                $("#loginhistory_failed_summary").text(loginHistoryPageText.showing + " 0 / 0");
            }
        });
    };

    pagefunction();
</script>
