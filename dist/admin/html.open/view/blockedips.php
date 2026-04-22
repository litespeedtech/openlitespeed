<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

echo UI::content_header('shield-alert', DMsg::UIStr('service_blockedips'));
?>

<section class="lst-blockedips-page">
<article>
    <div class="lst-widget lst-blockedips-widget" id="blockedips_widget">
        <div class="lst-widget-content">
            <div class="lst-widget-body">
                <div class="lst-blockedips-overview">
                    <section class="lst-blockedips-totalcard">
                        <div class="lst-blockedips-totalcard__header">
                            <div class="lst-blockedips-totalcard__label"><?php DMsg::EchoUIStr('service_blockedipcnt'); ?></div>
                        </div>
                        <div class="lst-blockedips-totalcard__metricrow">
                            <div id="blockedips_total" class="lst-blockedips-totalcard__value">0</div>
                            <div class="lst-blockedips-totalcard__actions" role="menu">
                                <button type="button" id="blockedips_refresh_btn" class="lst-blockedips-refreshbtn" disabled rel="tooltip" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_refresh')); ?>" aria-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_refresh')); ?>">
                                    <i class="lst-icon" data-lucide="refresh-cw" aria-hidden="true"></i>
                                    <span><?php DMsg::EchoUIStr('btn_refresh'); ?></span>
                                </button>
                                <div class="lst-spin-icon lst-blockedips-totalcard__status" id="blockedips_refresh_icon" role="status"></div>
                            </div>
                        </div>
                    </section>
                    <div class="lst-blockedips-summarypane">
                        <div id="blockedips_categories" class="lst-blockedips-categories" aria-live="polite"></div>
                    </div>
                </div>

                <div class="lst-blockedips-toolbar lst-table-toolbar">
                    <div class="lst-blockedips-summary lst-table-toolbar__summary" aria-live="polite"></div>
                    <div class="lst-blockedips-reasonfilter lst-table-toolbar__meta">
                        <label class="lst-blockedips-reasonfilter__label" for="blockedips_reason_filter"><?php DMsg::EchoUIStr('service_reason'); ?></label>
                        <select id="blockedips_reason_filter" class="lst-control-compact"></select>
                    </div>
                    <div class="lst-blockedips-search-slot lst-table-toolbar__search"></div>
                    <div class="lst-blockedips-rows-slot lst-table-toolbar__rows"></div>
                    <div class="lst-blockedips-downloads">
                        <div class="lst-dropdown-group" role="group" aria-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_download')); ?>">
                            <button type="button" id="blockedips_download_toggle" class="lst-btn lst-btn--secondary lst-btn--sm lst-dropdown-toggle" data-lst-dropdown="true" aria-haspopup="true" aria-expanded="false">
                                <i class="lst-icon" data-lucide="download" aria-hidden="true"></i>
                                <span><?php DMsg::EchoUIStr('btn_download'); ?></span>
                                <i class="lst-icon" data-lucide="chevron-down" aria-hidden="true"></i>
                            </button>
                            <ul class="lst-dropdown-menu lst-dropdown-menu--right">
                                <li><a id="blockedips_download_csv" href="#">CSV</a></li>
                                <li><a id="blockedips_download_txt" href="#">TXT</a></li>
                            </ul>
                        </div>
                    </div>
                </div>

                <p id="blockedips_statusnote" class="lst-blockedips-statusnote" hidden></p>

                <div id="blockedips_empty" class="lst-blockedips-empty" hidden>
                    <i class="lst-icon" data-lucide="circle-check-big" aria-hidden="true"></i>
                    <span><?php DMsg::EchoUIStr('service_noblockedips'); ?></span>
                </div>

                <table id="blockedips_table" class="table lst-data-grid-table lst-data-grid-table--interactive lst-blockedips-table" width="100%">
                    <thead>
                    <tr>
                        <th><?php DMsg::EchoUIStr('l_ip'); ?></th>
                        <th><?php DMsg::EchoUIStr('service_reason'); ?></th>
                    </tr>
                    </thead>
                    <tbody></tbody>
                </table>
            </div>
        </div>
    </div>
</article>
</section>

<script type="text/javascript">
    pageSetUp();

    var blockedIpPageText = {
        showing: <?php echo json_encode(DMsg::UIStr('note_showing')); ?>,
        rows: <?php echo json_encode(DMsg::UIStr('note_rows')); ?>,
        filter: <?php echo json_encode(DMsg::UIStr('note_filter')); ?>,
        allCategories: <?php echo json_encode(DMsg::UIStr('service_allcategories')); ?>,
        noBlockedIps: <?php echo json_encode(DMsg::UIStr('service_noblockedips')); ?>,
        loadFailed: <?php echo json_encode(DMsg::UIStr('err_failreadfile')); ?>
    };

    var blockedIpTable = null,
        blockedIpTableState = {
            page: 0,
            length: 25,
            search: "",
            reason: "",
            totalCount: 0
        };

    function blockedIpsEndpoint(id) {
        return "view/ajax_data.php?id=" + id;
    }

    function blockedIpsEscape(value) {
        return $("<div>").text(value == null ? "" : value).html();
    }

    function formatBlockedIpCount(value) {
        var count = parseInt(value, 10);

        if (isNaN(count) || count < 0) {
            count = 0;
        }

        return count.toLocaleString();
    }

    function blockedIpRefreshSpinner() {
        return '<i class="lst-icon lst-spin" data-lucide="loader-2"></i>';
    }

    function setBlockedIpStatus(message, isError) {
        var $status = $("#blockedips_statusnote");

        if (!message) {
            $status.prop("hidden", true).removeClass("is-error").text("");
            return;
        }

        $status.text(message).prop("hidden", false).toggleClass("is-error", !!isError);
    }

    function renderBlockedIpOverview(payload) {
        var categories = payload.categories || [],
            html = "",
            i;

        $("#blockedips_total").text(formatBlockedIpCount(payload.total_count));

        for (i = 0; i < categories.length; i++) {
            html += '<div class="lst-blockedips-category">'
                    + '<div class="lst-blockedips-category__head">'
                    + '<span class="lst-blockedips-code">' + blockedIpsEscape(categories[i].code) + '</span>'
                    + '<strong class="lst-blockedips-category__count">' + blockedIpsEscape(formatBlockedIpCount(categories[i].count)) + '</strong>'
                    + '</div>'
                    + '<div class="lst-blockedips-category__reason">' + blockedIpsEscape(categories[i].reason) + '</div>'
                    + '</div>';
        }

        $("#blockedips_categories").html(html);
    }

    function populateBlockedIpReasonFilter(categories, selectedReason) {
        var options = ['<option value="">' + blockedIpsEscape(blockedIpPageText.allCategories) + '</option>'],
            i,
            value;

        for (i = 0; i < categories.length; i++) {
            value = categories[i].code || "";
            options.push('<option value="' + blockedIpsEscape(value) + '">' + blockedIpsEscape(categories[i].reason) + '</option>');
        }

        $("#blockedips_reason_filter").html(options.join("")).val(selectedReason || "");
    }

    function buildBlockedIpRows(items) {
        var rows = "",
            i,
            code,
            reason,
            orderValue;

        for (i = 0; i < items.length; i++) {
            code = items[i].code || "";
            reason = items[i].reason || "";
            orderValue = (code ? code + " " : "") + reason;
            rows += '<tr data-reason-code="' + blockedIpsEscape(code) + '">'
                    + '<td><code class="lst-blockedips-ip">' + blockedIpsEscape(items[i].ip) + '</code></td>'
                    + '<td class="lst-blockedips-reasoncol" data-order="' + blockedIpsEscape(orderValue) + '">'
                    + (code ? '<span class="lst-blockedips-code lst-blockedips-code--inline">' + blockedIpsEscape(code) + '</span>' : '')
                    + '<span class="lst-blockedips-reasontext">' + blockedIpsEscape(reason) + '</span>'
                    + '</td>'
                    + '</tr>';
        }

        return rows;
    }

    function captureBlockedIpTableState() {
        var state = {
            page: 0,
            length: blockedIpTableState.length || 25,
            search: blockedIpTableState.search || "",
            reason: $("#blockedips_reason_filter").val() || ""
        };

        if ($.fn.dataTable && $.fn.dataTable.isDataTable("#blockedips_table")) {
            var api = $("#blockedips_table").DataTable();
            state.page = api.page();
            state.length = api.page.len();
            state.search = api.search();
        }

        return state;
    }

    function formatBlockedIpSummary(api) {
        var info = api ? api.page.info() : null,
            visible = 0,
            total = 0;

        if (!info) {
            return blockedIpPageText.showing + " 0 / 0";
        }

        visible = Math.max(info.end - info.start, 0);
        total = Math.max(parseInt(info.recordsDisplay, 10) || 0, 0);

        return blockedIpPageText.showing + " " + formatBlockedIpCount(visible) + " / " + formatBlockedIpCount(total);
    }

    function syncBlockedIpControls() {
        var $table = $("#blockedips_table"),
            wrapper = $table.closest(".dt-container"),
            searchSlot = $(".lst-blockedips-search-slot"),
            rowsSlot = $(".lst-blockedips-rows-slot"),
            summarySlot = $(".lst-blockedips-summary"),
            filter = wrapper.find(".dt-search").first(),
            length = wrapper.find(".dt-length").first();

        if (filter.length && filter.parent()[0] !== searchSlot[0]) {
            searchSlot.append(filter);
        }

        if (length.length && length.parent()[0] !== rowsSlot[0]) {
            rowsSlot.append(length);
        }

        if (blockedIpTable) {
            summarySlot.text(formatBlockedIpSummary(blockedIpTable));
        }
        else {
            summarySlot.text(blockedIpPageText.showing + " 0 / 0");
        }

        updateBlockedIpDownloadLinks();
    }

    function ensureBlockedIpReasonFilter() {
        if (window.lstBlockedIpReasonFilterInstalled || !$.fn.dataTable || !$.fn.dataTable.ext || !$.fn.dataTable.ext.search) {
            return;
        }

        $.fn.dataTable.ext.search.push(function (settings, data, dataIndex) {
            var selectedReason = $("#blockedips_reason_filter").val() || "",
                table = document.getElementById("blockedips_table"),
                row;

            if (!table || settings.nTable !== table || selectedReason === "") {
                return true;
            }

            row = settings.aoData && settings.aoData[dataIndex] ? settings.aoData[dataIndex].nTr : null;
            return ($(row).attr("data-reason-code") || "") === selectedReason;
        });

        window.lstBlockedIpReasonFilterInstalled = true;
    }

    function updateBlockedIpDownloadLinks() {
        var search = blockedIpTable ? blockedIpTable.search() : "",
            reason = $("#blockedips_reason_filter").val() || "",
            hasItems = parseInt(blockedIpTableState.totalCount, 10) > 0,
            $downloadToggle = $("#blockedips_download_toggle"),
            $downloadLinks = $("#blockedips_download_csv, #blockedips_download_txt"),
            query = [];

        if (search !== "") {
            query.push("search=" + encodeURIComponent(search));
        }

        if (reason !== "") {
            query.push("reason=" + encodeURIComponent(reason));
        }

        if (!hasItems) {
            $downloadToggle.closest(".lst-dropdown-group").removeClass("open");
            $downloadToggle
                .addClass("disabled")
                .attr("aria-disabled", "true")
                .attr("aria-expanded", "false");
            $downloadLinks
                .attr("href", "#")
                .addClass("disabled")
                .attr("aria-disabled", "true")
                .attr("tabindex", "-1");
            return;
        }

        $downloadToggle
            .removeClass("disabled")
            .removeAttr("aria-disabled");
        $downloadLinks
            .removeClass("disabled")
            .removeAttr("aria-disabled")
            .removeAttr("tabindex");
        $("#blockedips_download_csv").attr("href", "view/ajax_data.php?id=downloadblockedips&format=csv" + (query.length ? "&" + query.join("&") : ""));
        $("#blockedips_download_txt").attr("href", "view/ajax_data.php?id=downloadblockedips&format=txt" + (query.length ? "&" + query.join("&") : ""));
    }

    function setBlockedIpEmptyState(totalCount) {
        var hasItems = parseInt(totalCount, 10) > 0;

        $("#blockedips_empty").prop("hidden", hasItems);
        $("#blockedips_table").toggle(hasItems);
    }

    function getBlockedIpTableOptions(state) {
        return {
            autoWidth: false,
            info: false,
            order: [[1, "asc"], [0, "asc"]],
            pageLength: state.length || 25
        };
    }

    function resetBlockedIpTableChrome() {
        $(".lst-blockedips-search-slot .dt-search").remove();
        $(".lst-blockedips-rows-slot .dt-length").remove();
    }

    function updateBlockedIpTableRows(items) {
        var api = lstGetDataTableApi("#blockedips_table"),
            markup = buildBlockedIpRows(items || []),
            rows = $("<tbody>").html(markup).children("tr"),
            $tbody = $("#blockedips_table tbody");

        if (!api) {
            $tbody.empty().html(markup);
            return;
        }

        api.clear();
        if (rows.length) {
            api.rows.add(rows.toArray());
        }
        api.draw(false);
    }

    function ensureBlockedIpTable(state) {
        if (!$.fn.dataTable.isDataTable("#blockedips_table")) {
            blockedIpTable = $("#blockedips_table").DataTable(getBlockedIpTableOptions(state));
            lstTuneDataTableChrome("#blockedips_table", {
                placeholder: blockedIpPageText.filter,
                lengthLabel: blockedIpPageText.rows,
                bindDraw: true,
                onUpdate: syncBlockedIpControls
            });
        }
        else {
            blockedIpTable = $("#blockedips_table").DataTable();
        }

        if (blockedIpTable && blockedIpTable.page.len() !== (state.length || 25)) {
            blockedIpTable.page.len(state.length || 25);
        }

        syncBlockedIpControls();
    }

    function renderBlockedIpTable(payload, state) {
        blockedIpTableState = state;
        blockedIpTableState.totalCount = parseInt(payload.total_count, 10) || 0;

        populateBlockedIpReasonFilter(payload.categories || [], state.reason);
        setBlockedIpEmptyState(payload.total_count);

        if (parseInt(payload.total_count, 10) < 1) {
            resetBlockedIpTableChrome();
            lstDestroyDataTable("#blockedips_table");
            blockedIpTable = null;
            syncBlockedIpControls();
            return;
        }

        ensureBlockedIpReasonFilter();

        updateBlockedIpTableRows(payload.items || []);
        ensureBlockedIpTable(state);

        if (state.search) {
            blockedIpTable.search(state.search);
        }
        else {
            blockedIpTable.search("");
        }

        blockedIpTable.draw();

        if (blockedIpTable.page.info().pages > 0) {
            blockedIpTable.page(Math.min(state.page, blockedIpTable.page.info().pages - 1)).draw("page");
        }

        $("#blockedips_reason_filter").val(state.reason || "");
        if (state.reason) {
            blockedIpTable.draw();
        }
    }

    function refreshBlockedIps() {
        var state = captureBlockedIpTableState(),
            $refreshBtn = $("#blockedips_refresh_btn"),
            $refreshIcon = $("#blockedips_refresh_icon");

        $.ajax({
            url: blockedIpsEndpoint("blockedipsfull"),
            type: "GET",
            dataType: "json",
            beforeSend: function () {
                $refreshBtn.prop("disabled", true);
                $refreshIcon.html(blockedIpRefreshSpinner());
                setBlockedIpStatus("");
            }
        })
        .done(function (payload) {
            renderBlockedIpOverview(payload);
            renderBlockedIpTable(payload, state);
            lst_refreshFooterTime();
            $refreshIcon.empty();
        })
        .fail(function () {
            setBlockedIpStatus(blockedIpPageText.loadFailed, true);
            $refreshIcon.empty();
        })
        .always(function () {
            $refreshBtn.prop("disabled", false);
        });
    }

    var pagefunction = function () {
        $("#blockedips_refresh_btn").on("click", function () {
            refreshBlockedIps();
        });

        $("#blockedips_download_toggle").on("click", function (event) {
            if ($(this).attr("aria-disabled") === "true") {
                event.preventDefault();
                event.stopPropagation();
            }
        });

        $("#blockedips_download_csv, #blockedips_download_txt").on("click", function (event) {
            if ($(this).hasClass("disabled") || $(this).attr("aria-disabled") === "true") {
                event.preventDefault();
                return;
            }
        });

        $("#blockedips_reason_filter").on("change", function () {
            blockedIpTableState.reason = $(this).val() || "";
            if (blockedIpTable) {
                blockedIpTable.draw();
            } else {
                updateBlockedIpDownloadLinks();
            }
        });

        lstLoadDataTableAssets(function () {
            $("#blockedips_refresh_btn").prop("disabled", false);
            refreshBlockedIps();
        });
    };

    pagefunction();
</script>
