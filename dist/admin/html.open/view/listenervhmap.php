<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

$pageTitle = 'Active Listener VHost Mappings';
$initialTab = (isset($_GET['tab']) && $_GET['tab'] === 'vhosts') ? 'vhosts' : 'listeners';
$initialListener = isset($_GET['listener']) && is_scalar($_GET['listener']) ? trim((string) $_GET['listener']) : '';
$initialVhost = isset($_GET['vhost']) && is_scalar($_GET['vhost']) ? trim((string) $_GET['vhost']) : '';

echo UI::content_header('plug-zap', $pageTitle);
?>

<section class="lst-vhmap-page">
<article>
    <div class="lst-widget lst-vhmap-widget" id="listenervhmap_widget">
        <div class="lst-widget-content">
            <div class="lst-widget-body">
                <div class="lst-vhmap-overview" aria-live="polite">
                    <section class="lst-vhmap-statcard">
                        <div class="lst-vhmap-statcard__label"><?php DMsg::EchoUIStr('menu_sl'); ?></div>
                        <div id="listenervhmap_listener_total" class="lst-vhmap-statcard__value">0</div>
                        <div class="lst-vhmap-statcard__meta"><?php DMsg::EchoUIStr('service_running'); ?></div>
                    </section>
                    <section class="lst-vhmap-statcard">
                        <div class="lst-vhmap-statcard__label"><?php DMsg::EchoUIStr('menu_vh'); ?></div>
                        <div id="listenervhmap_vhost_total" class="lst-vhmap-statcard__value">0</div>
                        <div class="lst-vhmap-statcard__meta"><?php DMsg::EchoUIStr('service_running'); ?></div>
                    </section>
                    <section class="lst-vhmap-statcard">
                        <div class="lst-vhmap-statcard__label"><?php DMsg::EchoUIStr('l_domains'); ?></div>
                        <div id="listenervhmap_mapping_total" class="lst-vhmap-statcard__value">0</div>
                        <div class="lst-vhmap-statcard__meta"><?php DMsg::EchoUIStr('service_total'); ?></div>
                    </section>
                </div>

                <p id="listenervhmap_statusnote" class="lst-vhmap-statusnote" hidden></p>

                <div class="lst-vhmap-tabsbar">
                    <ul class="lst-tabs lst-tabs--compact lst-tabs--config-hover lst-tab-trailing-nav" id="listenervhmap_tabs">
                        <li class="active"><a href="#listenervhmap_listener_panel" data-lst-vhmap-tab="listeners"><i class="lst-icon" data-lucide="plug-zap"></i> <?php DMsg::EchoUIStr('menu_sl'); ?></a></li>
                        <li><a href="#listenervhmap_vhost_panel" data-lst-vhmap-tab="vhosts"><i class="lst-icon" data-lucide="server"></i> <?php DMsg::EchoUIStr('menu_vh'); ?></a></li>
                    </ul>
                    <button type="button" id="listenervhmap_refresh_btn" class="lst-btn lst-btn--primary lst-btn--sm" disabled>
                        <i class="lst-icon" data-lucide="refresh-cw" aria-hidden="true"></i>
                        <span><?php DMsg::EchoUIStr('btn_refresh'); ?></span>
                    </button>
                </div>

                <div class="lst-vhmap-panelgroup">
                    <section id="listenervhmap_listener_panel" class="lst-vhmap-panel is-active">
                        <div class="lst-vhmap-toolbar lst-table-toolbar">
                            <div class="lst-vhmap-toolbar__meta">
                                <label class="lst-vhmap-field" for="listenervhmap_listener_select">
                                    <span class="lst-vhmap-field__label"><?php DMsg::EchoUIStr('l_listenername'); ?></span>
                                    <select id="listenervhmap_listener_select" class="lst-control-compact"></select>
                                </label>
                            </div>
                            <div id="listenervhmap_listener_summary" class="lst-vhmap-toolbar__summary" aria-live="polite"></div>
                            <div id="listenervhmap_listener_search" class="lst-vhmap-toolbar__search"></div>
                            <div id="listenervhmap_listener_rows" class="lst-vhmap-toolbar__rows"></div>
                        </div>

                        <div class="lst-vhmap-tablewrap">
                            <table id="listenervhmap_listener_table" class="table lst-data-grid-table lst-data-grid-table--interactive lst-vhmap-table" width="100%">
                                <thead>
                                <tr>
                                    <th>#</th>
                                    <th><?php DMsg::EchoUIStr('menu_vh'); ?></th>
                                    <th><?php DMsg::EchoUIStr('l_domains'); ?></th>
                                </tr>
                                </thead>
                                <tbody></tbody>
                            </table>
                        </div>
                    </section>

                    <section id="listenervhmap_vhost_panel" class="lst-vhmap-panel" hidden>
                        <div class="lst-vhmap-toolbar lst-table-toolbar">
                            <div class="lst-vhmap-toolbar__meta">
                                <label class="lst-vhmap-field" for="listenervhmap_vhost_input">
                                    <span class="lst-vhmap-field__label">Virtual Host Name or Domain Name</span>
                                    <input id="listenervhmap_vhost_input" class="lst-choice-control lst-vhmap-vhost-input" type="text" autocomplete="off" placeholder="<?php echo UIBase::EscapeAttr(DMsg::UIStr('service_regexhint')); ?>">
                                </label>
                            </div>
                            <div id="listenervhmap_vhost_summary" class="lst-vhmap-toolbar__summary" aria-live="polite"></div>
                        </div>

                        <div class="lst-vhmap-tablewrap">
                            <table class="table lst-data-grid-table lst-vhmap-table" width="100%">
                                <thead>
                                <tr>
                                    <th><?php DMsg::EchoUIStr('menu_vh'); ?></th>
                                    <th><?php DMsg::EchoUIStr('l_listenername'); ?></th>
                                    <th><?php DMsg::EchoUIStr('l_domains'); ?></th>
                                </tr>
                                </thead>
                                <tbody id="listenervhmap_vhost_body"></tbody>
                            </table>
                        </div>
                    </section>
                </div>
            </div>
        </div>
    </div>
</article>
</section>

<script type="text/javascript">
    pageSetUp();

    var listenerVhMapText = {
        showing: <?php echo json_encode(DMsg::UIStr('note_showing')); ?>,
        rows: <?php echo json_encode(DMsg::UIStr('note_rows')); ?>,
        filter: <?php echo json_encode(DMsg::UIStr('note_filter')); ?>,
        loadFailed: <?php echo json_encode(DMsg::UIStr('err_failreadfile')); ?>,
        notAvailable: <?php echo json_encode(DMsg::UIStr('service_notavailable')); ?>,
        regexHint: <?php echo json_encode(DMsg::UIStr('service_regexhint')); ?>,
        regexNoMatch: "No active mapped listeners matched the current vhost/domain expression.",
        regexInvalid: "Invalid regular expression.",
        vhostsLabel: "vhosts",
        listenerRowsLabel: "listener rows"
    };

    var listenerVhMapState = {
        activeTab: <?php echo json_encode($initialTab); ?>,
        initialListener: <?php echo json_encode($initialListener); ?>,
        initialVhost: <?php echo json_encode($initialVhost); ?>,
        data: null,
        listenerTable: null,
        listenerRowsTotal: 0
    };

    function listenerVhMapEndpoint() {
        return "view/ajax_data.php?id=listenervhmapfull";
    }

    function listenerVhMapTrim(value) {
        return String(value == null ? "" : value).trim();
    }

    function listenerVhMapEscape(value) {
        return $("<div>").text(value == null ? "" : value).html();
    }

    function listenerVhMapFormatCount(value) {
        var count = parseInt(value, 10);

        if (isNaN(count) || count < 0) {
            count = 0;
        }

        return count.toLocaleString();
    }

    function setListenerVhMapStatus(message, isError) {
        var $status = $("#listenervhmap_statusnote");

        if (!message) {
            $status.prop("hidden", true).removeClass("is-error").text("");
            return;
        }

        $status.text(message).prop("hidden", false).toggleClass("is-error", !!isError);
    }

    function setListenerVhMapBusy(isBusy) {
        var $button = $("#listenervhmap_refresh_btn");

        $button.prop("disabled", !!isBusy);
        $button.toggleClass("is-loading", !!isBusy);
        $button.find(".lst-icon").toggleClass("lst-spin", !!isBusy);
    }

    function updateOverview(summary) {
        $("#listenervhmap_listener_total").text(listenerVhMapFormatCount(summary.listener_count));
        $("#listenervhmap_vhost_total").text(listenerVhMapFormatCount(summary.vhost_count));
        $("#listenervhmap_mapping_total").text(listenerVhMapFormatCount(summary.domain_count));
    }

    function findListenerByName(name) {
        var listeners = listenerVhMapState.data ? listenerVhMapState.data.listeners : [],
            i;

        for (i = 0; i < listeners.length; i++) {
            if (listeners[i].name === name) {
                return listeners[i];
            }
        }

        return null;
    }

    function getSelectedListenerName() {
        var listeners = listenerVhMapState.data ? listenerVhMapState.data.listeners : [],
            current = listenerVhMapTrim($("#listenervhmap_listener_select").val()),
            i;

        if (!listeners.length) {
            return "";
        }

        if (listenerVhMapState.initialListener !== "") {
            for (i = 0; i < listeners.length; i++) {
                if (listeners[i].name === listenerVhMapState.initialListener) {
                    listenerVhMapState.initialListener = "";
                    return listeners[i].name;
                }
            }
        }

        for (i = 0; i < listeners.length; i++) {
            if (listeners[i].name === current) {
                return current;
            }
        }

        return listeners[0].name;
    }

    function populateListenerSelect() {
        var listeners = listenerVhMapState.data ? listenerVhMapState.data.listeners : [],
            selected = getSelectedListenerName(),
            html = "",
            label,
            i;

        for (i = 0; i < listeners.length; i++) {
            label = listeners[i].name + (listeners[i].address ? " - " + listeners[i].address : "");
            html += '<option value="' + listenerVhMapEscape(listeners[i].name) + '">' + listenerVhMapEscape(label) + '</option>';
        }

        $("#listenervhmap_listener_select").html(html).val(selected);
    }

    function getListenerTableOptions() {
        return {
            autoWidth: false,
            info: false,
            ordering: false,
            pageLength: 10,
            language: lstDataTableLanguage({
                prev: <?php echo json_encode(DMsg::UIStr('btn_prev')); ?>,
                next: <?php echo json_encode(DMsg::UIStr('btn_next')); ?>
            })
        };
    }

    function formatListenerSummary(api, totalRows) {
        var info = api ? api.page.info() : null,
            visible = 0;

        if (!info || totalRows < 1) {
            return listenerVhMapText.showing + " 0 / 0";
        }

        visible = Math.max(info.end - info.start, 0);
        return listenerVhMapText.showing + " " + listenerVhMapFormatCount(visible) + " / " + listenerVhMapFormatCount(totalRows);
    }

    function syncListenerTableControls() {
        var api = lstGetDataTableApi("#listenervhmap_listener_table"),
            totalRows = listenerVhMapState.listenerRowsTotal || 0,
            showControls = totalRows > 10,
            wrapper = $("#listenervhmap_listener_table").closest(".dt-container"),
            searchSlot = $("#listenervhmap_listener_search"),
            rowsSlot = $("#listenervhmap_listener_rows"),
            summarySlot = $("#listenervhmap_listener_summary"),
            filter = wrapper.find(".dt-search").first(),
            length = wrapper.find(".dt-length").first(),
            paging = wrapper.find(".dt-paging").first();

        if (filter.length && filter.parent()[0] !== searchSlot[0]) {
            searchSlot.append(filter);
        }

        if (length.length && length.parent()[0] !== rowsSlot[0]) {
            rowsSlot.append(length);
        }

        searchSlot.toggle(showControls);
        rowsSlot.toggle(showControls);
        paging.toggle(showControls);
        summarySlot.text(formatListenerSummary(api, totalRows));
    }

    function ensureListenerTable() {
        if (!$.fn.dataTable) {
            return null;
        }

        if (!$.fn.dataTable.isDataTable("#listenervhmap_listener_table")) {
            listenerVhMapState.listenerTable = $("#listenervhmap_listener_table").DataTable(getListenerTableOptions());
            lstTuneDataTableChrome("#listenervhmap_listener_table", {
                placeholder: listenerVhMapText.filter,
                lengthLabel: listenerVhMapText.rows,
                bindDraw: true,
                onUpdate: syncListenerTableControls
            });
        } else {
            listenerVhMapState.listenerTable = $("#listenervhmap_listener_table").DataTable();
        }

        return listenerVhMapState.listenerTable;
    }

    function renderListenerTab(resetTableState) {
        var selectedName,
            listener,
            rows,
            api,
            rowNodes,
            tbody = $("#listenervhmap_listener_table tbody"),
            html = "",
            i;

        populateListenerSelect();
        selectedName = getSelectedListenerName();
        listener = findListenerByName(selectedName);

        if (!listener) {
            listenerVhMapState.listenerRowsTotal = 0;
            tbody.html('<tr><td colspan="3" class="lst-cell-empty">&mdash;</td></tr>');
            $("#listenervhmap_listener_summary").text(listenerVhMapText.showing + " 0 / 0");
            return;
        }

        rows = listener.rows || [];
        listenerVhMapState.listenerRowsTotal = rows.length;

        for (i = 0; i < rows.length; i++) {
            html += '<tr><td class="lst-vhmap-indexcol">' + (i + 1) + '</td>'
                + '<td class="lst-vhmap-namecell">' + listenerVhMapEscape(rows[i].vhost) + '</td>'
                + '<td class="lst-vhmap-domainscell">' + (rows[i].domains ? listenerVhMapEscape(rows[i].domains) : '&mdash;') + '</td></tr>';
        }

        api = ensureListenerTable();
        if (!api) {
            tbody.html(html || '<tr><td colspan="3" class="lst-cell-empty">&mdash;</td></tr>');
            $("#listenervhmap_listener_summary").text(listenerVhMapText.showing + " " + listenerVhMapFormatCount(rows.length) + " / " + listenerVhMapFormatCount(rows.length));
            return;
        }

        rowNodes = $("<tbody>").html(html).children("tr");
        api.clear();
        if (rowNodes.length) {
            api.rows.add(rowNodes.toArray());
        }

        if (resetTableState) {
            api.search("");
            api.page.len(10);
        }

        api.draw(false);
        if (resetTableState && api.page.info().pages > 0) {
            api.page(0).draw("page");
        }

        syncListenerTableControls();
    }

    function escapeRegexText(pattern) {
        return pattern.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
    }

    function buildVhostRegexResult(patternText) {
        var vhosts = listenerVhMapState.data ? listenerVhMapState.data.vhosts : [],
            query = listenerVhMapTrim(patternText),
            regex,
            rows = [],
            matchedVhosts = 0,
            i,
            j;

        if (query === "") {
            return {
                rows: [],
                matchedVhosts: 0,
                invalid: false,
                empty: true
            };
        }

        try {
            regex = new RegExp(query, "i");
        } catch (error) {
            try {
                regex = new RegExp(escapeRegexText(query), "i");
            } catch (ignore) {
                return {
                    rows: [],
                    matchedVhosts: 0,
                    invalid: true,
                    empty: false
                };
            }
        }

        for (i = 0; i < vhosts.length; i++) {
            if (!regex.test(vhosts[i].name) && !regex.test(vhosts[i].domains || "")) {
                continue;
            }

            matchedVhosts++;
            for (j = 0; j < vhosts[i].listeners.length; j++) {
                rows.push({
                    vhost: vhosts[i].name,
                    listener: vhosts[i].listeners[j].name,
                    listenerAddress: vhosts[i].listeners[j].address || "",
                    domains: vhosts[i].listeners[j].domains
                });
            }
        }

        return {
            rows: rows,
            matchedVhosts: matchedVhosts,
            invalid: false,
            empty: false
        };
    }

    function renderVhostTab() {
        var result = buildVhostRegexResult($("#listenervhmap_vhost_input").val()),
            html = "",
            summaryText,
            i;

        if (result.invalid) {
            summaryText = listenerVhMapText.regexInvalid;
        } else if (result.empty) {
            summaryText = "";
        } else if (!result.rows.length) {
            summaryText = listenerVhMapText.regexNoMatch;
        } else {
            summaryText = listenerVhMapFormatCount(result.matchedVhosts) + " " + listenerVhMapText.vhostsLabel + " / "
                + listenerVhMapFormatCount(result.rows.length) + " " + listenerVhMapText.listenerRowsLabel;
        }

        for (i = 0; i < result.rows.length; i++) {
            html += '<tr><td class="lst-vhmap-namecell">' + listenerVhMapEscape(result.rows[i].vhost) + '</td>'
                + '<td class="lst-vhmap-namecell">' + listenerVhMapEscape(result.rows[i].listener + (result.rows[i].listenerAddress ? ' - ' + result.rows[i].listenerAddress : '')) + '</td>'
                + '<td class="lst-vhmap-domainscell">' + (result.rows[i].domains ? listenerVhMapEscape(result.rows[i].domains) : '&mdash;') + '</td></tr>';
        }

        if (!html) {
            html = '<tr><td colspan="3" class="lst-cell-empty">&mdash;</td></tr>';
        }

        $("#listenervhmap_vhost_summary").text(summaryText);
        $("#listenervhmap_vhost_body").html(html);
    }

    function renderPage() {
        if (!listenerVhMapState.data) {
            return;
        }

        updateOverview(listenerVhMapState.data.summary || {
            listener_count: 0,
            vhost_count: 0,
            domain_count: 0,
            mapping_count: 0
        });

        renderListenerTab(true);

        if (listenerVhMapState.initialVhost !== "") {
            $("#listenervhmap_vhost_input").val(listenerVhMapState.initialVhost);
            listenerVhMapState.initialVhost = "";
        }

        renderVhostTab();
    }

    function setActiveTab(tabName) {
        listenerVhMapState.activeTab = (tabName === "vhosts") ? "vhosts" : "listeners";
        $("#listenervhmap_tabs li").removeClass("active");
        $("#listenervhmap_tabs a[data-lst-vhmap-tab='" + listenerVhMapState.activeTab + "']").closest("li").addClass("active");
        $(".lst-vhmap-panel").removeClass("is-active").prop("hidden", true);
        $("#listenervhmap_" + (listenerVhMapState.activeTab === "listeners" ? "listener" : "vhost") + "_panel").addClass("is-active").prop("hidden", false);
    }

    function refreshListenerVhMap() {
        $.ajax({
            url: listenerVhMapEndpoint(),
            type: "GET",
            dataType: "json",
            beforeSend: function () {
                setListenerVhMapBusy(true);
                setListenerVhMapStatus("");
            }
        })
        .done(function (payload) {
            listenerVhMapState.data = payload;
            renderPage();
            setActiveTab(listenerVhMapState.activeTab);
            lst_refreshFooterTime();
        })
        .fail(function () {
            setListenerVhMapStatus(listenerVhMapText.loadFailed, true);
        })
        .always(function () {
            setListenerVhMapBusy(false);
            if (typeof window.lstRenderLucide === "function") {
                window.lstRenderLucide(document.getElementById("listenervhmap_widget"));
            }
        });
    }

    var pagefunction = function () {
        $("#listenervhmap_tabs").on("click", "a[data-lst-vhmap-tab]", function (event) {
            event.preventDefault();
            setActiveTab($(this).attr("data-lst-vhmap-tab"));
        });

        $("#listenervhmap_refresh_btn").on("click", function () {
            refreshListenerVhMap();
        });

        $("#listenervhmap_listener_select").on("change", function () {
            renderListenerTab(true);
        });

        $("#listenervhmap_vhost_input").on("input", function () {
            renderVhostTab();
        });

        setActiveTab(listenerVhMapState.activeTab);

        lstLoadDataTableAssets(function () {
            $("#listenervhmap_refresh_btn").prop("disabled", false);
            refreshListenerVhMap();
        });
    };

    pagefunction();
</script>
