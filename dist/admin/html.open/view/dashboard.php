<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\Product;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

$dashboardProduct = Product::GetInstance();
$dashboardBuildDisplay = $dashboardProduct->getBuildDisplay();
$dashboardNewVersion = trim((string) $dashboardProduct->getNewVersion());
$dashboardNewVersionUrl = ($dashboardNewVersion !== '') ? UI::GetNewVersionUrl() : '';
$dashboardNewVersionExternal = ($dashboardNewVersion !== '') ? UI::IsNewVersionUrlExternal() : false;
$dashboardRateLabel = static function ($label) {
    return UIBase::Escape($label);
};

echo UI::content_header('layout-dashboard', DMsg::UIStr('menu_dashboard'), '', false);
?>

<section class="lst-dashboard-page">
        <article>
            <div class="lst-dashboard-statusbar" id="dashboard_statusbar">
                <div class="lst-dashboard-statusbar__group lst-dashboard-statusbar__group--version">
                    <span id="dash_version_value" class="lst-dashboard-statusbar__value">&mdash;</span>
<?php if ($dashboardNewVersion !== '' && $dashboardNewVersionUrl !== '') { ?>
                    <a class="lst-dashboard-statusbar__link" href="<?php echo UIBase::EscapeAttr($dashboardNewVersionUrl); ?>"<?php echo $dashboardNewVersionExternal ? ' rel="noopener noreferrer" target="_blank"' : ''; ?>><?php echo UIBase::Escape(DMsg::UIStr('note_newver') . ': ' . $dashboardNewVersion); ?></a>
<?php } ?>
                    <span class="lst-dashboard-statusbar__meta"><?php echo ($dashboardBuildDisplay !== '') ? UIBase::Escape($dashboardBuildDisplay) : ''; ?></span>
                </div>
                <div class="lst-dashboard-statusbar__group lst-dashboard-statusbar__group--pid">
                    <span id="dash_pid_dot" class="lst-dashboard-statusdot is-off" aria-hidden="true"></span>
                    <span class="lst-dashboard-statusbar__label"><?php DMsg::EchoUIStr('service_pid'); ?></span>
                    <span id="dash_pid_value" class="lst-dashboard-statusbar__value">&mdash;</span>
                    <span class="lst-dashboard-statusbar__sep" aria-hidden="true"></span>
                    <span id="dash_pid_state" class="lst-dashboard-statusbar__meta"><?php DMsg::EchoUIStr('note_unavailable'); ?></span>
                    <button type="button" class="lst-dashboard-statusbar__btn" data-lst-call="lst_restart" rel="tooltip" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('menu_restart')); ?>" aria-label="<?php echo UIBase::Escape(DMsg::UIStr('menu_restart')); ?>">
                        <i class="lst-icon" data-lucide="refresh-ccw-dot" aria-hidden="true"></i>
                    </button>
                </div>
                <div class="lst-dashboard-statusbar__group">
                    <span class="lst-dashboard-statusbar__label"><?php DMsg::EchoUIStr('note_systemload'); ?></span>
                    <span id="dash_load_value" class="lst-dashboard-statusbar__value">&mdash;</span>
                </div>
            </div>
        </article>

        <article>
            <div class="lst-widget lst-dashboard-summary-widget" id="dashboard_summary_widget">
                <header class="lst-widget-header">
                    <span class="lst-widget-icon"><i class="lst-icon" data-lucide="activity"></i></span>
                    <h2><?php DMsg::EchoUIStr('menu_rtstats'); ?></h2>
                    <div class="lst-widget-toolbar" role="menu">
                        <button type="button" id="dash_summary_refresh_btn" class="lst-card-link lst-dashboard-refreshbtn" rel="tooltip" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_refresh')); ?>" aria-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_refresh')); ?>">
                            <i class="lst-icon" data-lucide="refresh-cw" aria-hidden="true"></i>
                            <span><?php DMsg::EchoUIStr('btn_refresh'); ?></span>
                        </button>
                        <a class="lst-card-link" href="index.php?view=realtimestats">
                            <span><?php DMsg::EchoUIStr('btn_view'); ?></span>
                            <i class="lst-icon" data-lucide="chevron-right" aria-hidden="true"></i>
                        </a>
                    </div>
                </header>

                <div class="lst-widget-content">
                    <div class="lst-widget-body lst-dashboard-summary-body">
                        <div class="lst-dashboard-band lst-dashboard-band--metrics">
                            <div class="lst-dashboard-card-grid lst-dashboard-card-grid--metrics">
                                <section class="lst-dashboard-statcard lst-dashboard-statcard--metric" id="dash_card_requests">
                                    <div class="lst-dashboard-statlabel"><?php echo $dashboardRateLabel(DMsg::UIStr('service_reqpersec')); ?></div>
                                    <div id="dash_req_value" class="lst-dashboard-statvalue">&mdash;</div>
                                    <div class="lst-dashboard-statmeta">
                                        <?php DMsg::EchoUIStr('service_reqprocessing'); ?>:
                                        <span id="dash_req_processing">0</span>
                                    </div>
                                </section>
                                <section class="lst-dashboard-statcard lst-dashboard-statcard--metric" id="dash_card_connections">
                                    <div class="lst-dashboard-statlabel"><?php DMsg::EchoUIStr('service_connections'); ?></div>
                                    <div id="dash_conn_value" class="lst-dashboard-statvalue">&mdash;</div>
                                    <div class="lst-dashboard-statmeta">
                                        <?php DMsg::EchoUIStr('service_maxconn'); ?>:
                                        <span id="dash_conn_max">0</span>
                                    </div>
                                </section>
                                <section class="lst-dashboard-statcard lst-dashboard-statcard--metric" id="dash_card_ssl">
                                    <div class="lst-dashboard-statlabel"><?php DMsg::EchoUIStr('service_sslconnections'); ?></div>
                                    <div id="dash_ssl_value" class="lst-dashboard-statvalue">&mdash;</div>
                                    <div class="lst-dashboard-statmeta">
                                        <?php DMsg::EchoUIStr('service_maxsslconn'); ?>:
                                        <span id="dash_ssl_max">0</span>
                                    </div>
                                </section>
                                <section class="lst-dashboard-statcard lst-dashboard-statcard--metric" id="dash_card_throughput">
                                    <div class="lst-dashboard-statlabel"><?php DMsg::EchoUIStr('service_throughputps'); ?></div>
                                    <div id="dash_throughput_value" class="lst-dashboard-statvalue lst-dashboard-statvalue--compact">&mdash;</div>
                                    <div class="lst-dashboard-statmeta"><?php DMsg::EchoUIStr('service_throughput_meta'); ?></div>
                                </section>
                                <section class="lst-dashboard-statcard lst-dashboard-statcard--metric" id="dash_card_cache">
                                    <div class="lst-dashboard-statlabel"><?php echo $dashboardRateLabel(DMsg::UIStr('service_cachehitspersec')); ?></div>
                                    <div id="dash_cache_value" class="lst-dashboard-statvalue">&mdash;</div>
                                    <div class="lst-dashboard-statmeta lst-dashboard-statmeta--triple">
                                        <span><?php DMsg::EchoUIStr('service_public'); ?>: <span id="dash_cache_public">0</span></span>
                                        <span><?php DMsg::EchoUIStr('service_private'); ?>: <span id="dash_cache_private">0</span></span>
                                    </div>
                                </section>
                                <section class="lst-dashboard-statcard lst-dashboard-statcard--metric" id="dash_card_static">
                                    <div class="lst-dashboard-statlabel"><?php echo $dashboardRateLabel(DMsg::UIStr('service_statichitspersec')); ?></div>
                                    <div id="dash_static_value" class="lst-dashboard-statvalue">&mdash;</div>
                                    <div class="lst-dashboard-statmeta">&nbsp;</div>
                                </section>
                            </div>
                        </div>

                        <div class="lst-dashboard-band lst-dashboard-band--summary">
                            <div class="lst-dashboard-summaryrow" id="dash_listeners_card">
                                <span class="lst-dashboard-summaryrow__title"><?php DMsg::EchoUIStr('menu_sl'); ?></span>
                                <span class="lst-dashboard-summaryrow__item lst-dashboard-summaryrow__item--success"><span id="dash_listener_running">0</span> <?php DMsg::EchoUIStr('service_running'); ?></span>
                                <span class="lst-dashboard-summaryrow__item lst-dashboard-summaryrow__item--danger"><span id="dash_listener_broken">0</span> <?php DMsg::EchoUIStr('service_notrunning'); ?></span>
                            </div>
                            <div class="lst-dashboard-summaryrow" id="dash_vhosts_card">
                                <span class="lst-dashboard-summaryrow__title"><?php DMsg::EchoUIStr('menu_vh'); ?></span>
                                <span class="lst-dashboard-summaryrow__item lst-dashboard-summaryrow__item--success"><span id="dash_vh_running">0</span> <?php DMsg::EchoUIStr('service_active'); ?></span>
                                <span class="lst-dashboard-summaryrow__item lst-dashboard-summaryrow__item--danger"><span id="dash_vh_error">0</span> <?php DMsg::EchoUIStr('service_error'); ?></span>
                                <span class="lst-dashboard-summaryrow__item lst-dashboard-summaryrow__item--warning"><span id="dash_vh_disabled">0</span> <?php DMsg::EchoUIStr('service_disabled'); ?></span>
                            </div>
                            <div class="lst-dashboard-summaryrow" id="dash_card_blocked">
                                <span class="lst-dashboard-summaryrow__title"><?php DMsg::EchoUIStr('service_blockedips'); ?></span>
                                <button type="button" id="dash_blocked_link" class="lst-dashboard-summaryrow__item lst-dashboard-summaryrow__item--blocked" disabled aria-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_view') . ' ' . DMsg::UIStr('service_blockedips')); ?>">
                                    <span id="dash_blocked_value">0</span> <?php DMsg::EchoUIStr('service_total'); ?>
                                </button>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </article>

    <article>
        <div class="lst-widget lst-log-summary-widget" id="log_widget">
            <header role="heading" class="lst-widget-header">
                <span class="lst-widget-icon"><i class="lst-icon" data-lucide="file-text"></i></span>
                <h2><?php DMsg::EchoUIStr('service_servererrlog'); ?></h2>
                <div class="lst-log-header-statusbar" role="status" aria-live="polite">
                    <span class="lst-log-debug-status-label"><?php DMsg::EchoUIStr('service_debuglogstatus'); ?></span>
                    <span id="debuglogstatus" class="lst-log-debug-status-value"></span>
                    <button type="button" id="toggledebug" class="lst-btn lst-btn--secondary lst-log-toggle" aria-haspopup="dialog">
                        <i class="lst-icon" data-lucide="bug" aria-hidden="true"></i>
                        <?php DMsg::EchoUIStr('btn_toggle'); ?>
                    </button>
                </div>
                <div class="lst-widget-toolbar" role="menu">
                    <button type="button" id="dash_log_refresh_btn" class="lst-card-link lst-dashboard-refreshbtn" rel="tooltip" data-original-title="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_refresh')); ?>" aria-label="<?php echo UIBase::EscapeAttr(DMsg::UIStr('btn_refresh')); ?>">
                        <i class="lst-icon" data-lucide="refresh-cw" aria-hidden="true"></i>
                        <span><?php DMsg::EchoUIStr('btn_refresh'); ?></span>
                    </button>
                    <a class="lst-card-link lst-log-viewer-link" href="index.php?view=logviewer">
                        <?php DMsg::EchoUIStr('service_logviewer'); ?>
                        <i class="lst-icon" data-lucide="chevron-right"></i>
                    </a>
                </div>
                <div class="lst-widget-toolbar lst-spin-icon" role="menu"></div>
            </header>

            <div role="content" class="lst-widget-content">
                <div class="lst-widget-body lst-table-body">
                    <div class="lst-dashboard-log-toolbar" aria-live="polite">
                        <div id="dash_logsummary" class="lst-log-summary"></div>
                        <div id="dash_logmeta" class="lst-log-meta"></div>
                    </div>

                    <div id="dash_logempty" class="lst-dashboard-log-empty" hidden>
                        <i class="lst-icon" data-lucide="circle-check-big"></i>
                        <span><?php DMsg::EchoUIStr('service_norecenterrors'); ?></span>
                    </div>

                    <table id="dash_logtbl" class="table lst-data-grid-table lst-data-grid-table--interactive lst-log-data-grid" width="100%">
                        <thead>
                        <tr>
                            <th width="150"><?php DMsg::EchoUIStr('service_time'); ?></th>
                            <th width="60"><?php DMsg::EchoUIStr('service_level'); ?></th>
                            <th><?php DMsg::EchoUIStr('service_mesg'); ?></th>
                        </tr>
                        </thead>
                        <tbody id="dash_logbody"></tbody>
                    </table>
                </div>
            </div>
        </div>
    </article>
</section>

<script type="text/javascript">
    pageSetUp();

    var dashboardTableText = {
        debugOn: <?php echo json_encode(DMsg::UIStr('o_on')); ?>,
        debugOff: <?php echo json_encode(DMsg::UIStr('o_off')); ?>,
        debugUnavailable: <?php echo json_encode(DMsg::UIStr('note_unavailable')); ?>,
        close: <?php echo json_encode(DMsg::UIStr('btn_close')); ?>,
        blockedIps: <?php echo json_encode(DMsg::UIStr('service_blockedips')); ?>,
        noBlockedIps: <?php echo json_encode(DMsg::UIStr('service_noblockedips')); ?>,
        ipSamples: <?php echo json_encode(DMsg::UIStr('service_ipsamples')); ?>,
        totalLabel: <?php echo json_encode(DMsg::UIStr('service_total')); ?>,
        viewFullDetails: <?php echo json_encode(DMsg::UIStr('btn_viewfulldetails')); ?>,
        logWindow: <?php echo json_encode(DMsg::UIStr('service_loglastsize', ['%%size%%' => '20 KB'])); ?>,
        logCompact: <?php echo json_encode(DMsg::UIStr('service_logcompactnote')); ?>,
        logLevels: {
            1: 'ERROR',
            2: 'WARNING',
            3: 'NOTICE',
            4: 'INFO',
            5: 'DEBUG'
        },
        running: <?php echo json_encode(DMsg::UIStr('service_running')); ?>,
        unavailable: <?php echo json_encode(DMsg::UIStr('note_unavailable')); ?>
    };

    function dashboardBlockedPreviewUrl() {
        return "view/ajax_data.php?id=blockedipspreview";
    }

    function dashboardBlockedFullUrl(fallbackUrl) {
        return fallbackUrl || "index.php?view=blockedips";
    }

    function getDashboardBlockedCount(summary) {
        var count = parseInt(summary.blocked_ip_count, 10);

        if (isNaN(count) || count < 0) {
            count = 0;
        }

        return count;
    }

    function updateDashboardBlockedLink(count) {
        $("#dash_blocked_link").prop("disabled", count < 1);
    }

    function buildDashboardBlockedCategorySummary(categories) {
        var html = "",
            i;

        if (!categories || !categories.length) {
            return "";
        }

        html += '<div class="lst-dashboard-preview-categories">';
        for (i = 0; i < categories.length; i++) {
            html += '<div class="lst-dashboard-preview-category">'
                    + '<span class="lst-dashboard-preview-category__reason">' + $('<div>').text(categories[i].reason).html() + '</span>'
                    + '<span class="lst-dashboard-preview-category__count">' + $('<div>').text(formatDashboardCount(categories[i].count)).html() + '</span>'
                    + '</div>';
        }
        html += '</div>';

        return html;
    }

    function buildDashboardBlockedPreviewHeader(payload) {
        var totalCount = payload && payload.total_count ? payload.total_count : 0,
            totalText = dashboardTableText.totalLabel + ' ' + formatDashboardCount(totalCount),
            html = '<div class="lst-dashboard-preview-header">'
                + '<div class="lst-dashboard-preview-header__title">'
                + '<span class="lst-dialog-title-icon lst-dialog-title-icon--danger"><i class="lst-icon" data-lucide="shield-alert"></i></span>'
                + '<div class="lst-dashboard-preview-header__text">'
                + '<strong>' + $('<div>').text(dashboardTableText.blockedIps).html() + '</strong>'
                + '<span class="lst-dashboard-preview-header__meta">(' + $('<div>').text(totalText).html() + ')</span>'
                + '</div>'
                + '</div>'
                + '<div class="lst-dashboard-preview-header__actions">';

        if (payload.has_more) {
            html += '<button type="button" class="lst-dashboard-preview-header__link" data-lst-blocked-preview-details="1">'
                + $('<div>').text(dashboardTableText.viewFullDetails).html()
                + '</button>';
        }

        html += '<button type="button" class="lst-dashboard-preview-header__close" data-lst-blocked-preview-close="1" aria-label="' + $('<div>').text(dashboardTableText.close).html() + '">'
            + '<i class="lst-icon" data-lucide="x" aria-hidden="true"></i>'
            + '</button>'
            + '</div>'
            + '</div>';

        return html;
    }

    function buildDashboardBlockedPreviewContent(payload) {
        var items = payload && payload.items ? payload.items : [],
            categories = payload && payload.categories ? payload.categories : [],
            html = '<div class="lst-dashboard-preview">'
                + buildDashboardBlockedCategorySummary(categories),
            i;

        if (!items.length) {
            html += '<div class="lst-dashboard-preview-empty">'
                    + '<i class="lst-icon" data-lucide="circle-check-big" aria-hidden="true"></i>'
                    + '<span>' + $('<div>').text(dashboardTableText.noBlockedIps).html() + '</span>'
                    + '</div></div>';
            return html;
        }

        html += '<div class="lst-dashboard-preview-sectionlabel">' + $('<div>').text(dashboardTableText.ipSamples).html() + '</div>';
        html += '<ul class="lst-dashboard-preview-list">';
        for (i = 0; i < items.length; i++) {
            html += '<li class="lst-dashboard-preview-item">'
                    + '<span class="lst-dashboard-preview-ip">' + $('<div>').text(items[i].ip).html() + '</span>'
                    + '<span class="lst-dashboard-preview-item__reason">' + $('<div>').text(items[i].reason).html() + '</span>'
                    + '</li>';
        }
        html += '</ul></div>';

        return html;
    }

    function openDashboardBlockedPreviewModal(payload) {
        var modalId,
            markup,
            $modal;

        lstEnsureNotificationHosts();

        modalId = 'lst-dialog-' + (++lstDialogCounter);
        markup = ''
                + '<div class="lst-dialog-modal" id="' + modalId + '" tabindex="-1" role="dialog" aria-modal="true" aria-hidden="true">'
                + '<div class="lst-dialog-modal__backdrop" data-lst-dialog-backdrop="1"></div>'
                + '<div class="lst-dialog-modal__dialog lst-dashboard-blocked-dialog" role="document">'
                + '<div class="lst-dialog-modal__content">'
                + '<div class="lst-dialog-modal__body lst-dashboard-blocked-dialog__body">'
                + buildDashboardBlockedPreviewHeader(payload)
                + buildDashboardBlockedPreviewContent(payload)
                + '</div>'
                + '</div>'
                + '</div>'
                + '</div>';

        $modal = $(markup).appendTo('#lst-dialog-root');
        $modal.data('lstDialogReturnFocus', document.activeElement || null);

        function lstTrapDialogFocus(event) {
            var $focusables,
                first,
                last;

            if (event.key === 'Escape') {
                event.preventDefault();
                lstCloseDialog();
                return;
            }

            if (event.key !== 'Tab') {
                return;
            }

            $focusables = $modal.find('button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])')
                    .filter(':visible:not([disabled])');

            if (!$focusables.length) {
                event.preventDefault();
                return;
            }

            first = $focusables.get(0);
            last = $focusables.get($focusables.length - 1);

            if (event.shiftKey && document.activeElement === first) {
                event.preventDefault();
                last.focus();
            }
            else if (!event.shiftKey && document.activeElement === last) {
                event.preventDefault();
                first.focus();
            }
        }

        function lstDestroyDialog() {
            var returnFocus = $modal.data('lstDialogReturnFocus');

            $modal.remove();

            if (!$('#lst-dialog-root').children().length) {
                $('body').removeClass('lst-dialog-open');
            }

            if (returnFocus && returnFocus.focus) {
                try {
                    returnFocus.focus();
                } catch (ignore) {
                }
            }
        }

        function lstCloseDialog() {
            if ($modal.data('lstDialogClosing')) {
                return;
            }

            $modal.data('lstDialogClosing', true)
                    .removeClass('is-open')
                    .attr('aria-hidden', 'true');

            window.setTimeout(lstDestroyDialog, 180);
        }

        $modal.on('click', '[data-lst-blocked-preview-close]', function () {
            lstCloseDialog();
        });

        $modal.on('click', '[data-lst-blocked-preview-details]', function () {
            window.location = dashboardBlockedFullUrl(payload.full_url);
        });

        $modal.on('click', '[data-lst-dialog-backdrop]', function () {
            lstCloseDialog();
        });

        $modal.on('mousedown', '[data-lst-dialog-backdrop]', function (event) {
            event.preventDefault();
        });

        $modal.on('keydown', function (event) {
            lstTrapDialogFocus(event);
        });

        $('body').addClass('lst-dialog-open');

        window.setTimeout(function () {
            var $initial = $modal.find('[data-lst-blocked-preview-details], [data-lst-blocked-preview-close]').first();

            $modal.addClass('is-open').attr('aria-hidden', 'false');
            lstRenderLucide($modal[0]);

            if ($initial.length) {
                $initial.trigger('focus');
                $initial[0].focus();
            }
            else {
                $modal.trigger('focus');
            }
        }, 16);
    }

    function openDashboardBlockedPreview() {
        var $link = $("#dash_blocked_link");

        if ($link.prop("disabled")) {
            return;
        }

        $link.prop("disabled", true);

        function showPreview(payload) {
            openDashboardBlockedPreviewModal(payload);
        }

        $.ajax({
            url: dashboardBlockedPreviewUrl(),
            type: "GET",
            dataType: "json"
        })
        .done(function (payload) {
            showPreview(payload);
        })
        .fail(function () {
            window.location = dashboardBlockedFullUrl();
        })
        .always(function () {
            $link.prop("disabled", false);
        });
    }

    var lastDebugState = null;

    function setDebugLogStatus($statusRoot, state) {
        var normalizedState = parseInt(state, 10);

        if (isNaN(normalizedState)) {
            normalizedState = -1;
        }

        if (normalizedState === lastDebugState) {
            return;
        }
        lastDebugState = normalizedState;

        var statusValue = $statusRoot.find("#debuglogstatus");
        statusValue.removeClass("is-on is-off is-unavailable");

        if (normalizedState === 1) {
            statusValue.text(dashboardTableText.debugOn).addClass("is-on");
        }
        else if (normalizedState === 0) {
            statusValue.text(dashboardTableText.debugOff).addClass("is-off");
        }
        else {
            statusValue.text(dashboardTableText.debugUnavailable).addClass("is-unavailable");
        }
    }

    function formatDashboardCount(value, fallback) {
        var count = parseFloat(value);

        if (isNaN(count)) {
            return fallback || "0";
        }

        if (Math.floor(count) === count) {
            return count.toLocaleString();
        }

        return count.toLocaleString(undefined, {
            minimumFractionDigits: 0,
            maximumFractionDigits: 1
        });
    }

    function metricStateByRatio(currentValue, maxValue) {
        var ratio;

        if (!maxValue || maxValue <= 0) {
            return "";
        }

        ratio = currentValue / maxValue;
        if (ratio >= 0.95) {
            return "is-danger";
        }
        if (ratio >= 0.8) {
            return "is-warning";
        }

        return "is-success";
    }

    function applyCardState($card, stateClass) {
        $card.removeClass("is-success is-warning is-danger is-attention");
        if (stateClass) {
            $card.addClass(stateClass);
        }
    }

    function setDashboardRefreshBusy(selector, busy) {
        var button = $(selector);

        if (!button.length) {
            return;
        }

        button.prop("disabled", !!busy);
        button.find(".lst-icon").toggleClass("lst-spin", !!busy);
    }

    function updateDashboardStatusbar(data) {
        var pidValue,
            running,
            uptime,
            loadValue;

        if (!data) {
            return;
        }

        pidValue = data.pid ? data.pid : "—";
        running = (typeof data.running === "undefined") ? !!data.pid : !!data.running;
        uptime = data.uptime ? data.uptime : "";
        loadValue = data.load_average_display || data.serverload || "N/A";

        $("#dash_pid_value").text(pidValue);
        if (typeof data.uptime !== "undefined") {
            $("#dash_pid_state").text(running && uptime ? ("Up for " + uptime) : dashboardTableText.unavailable);
        }
        $("#dash_pid_dot").toggleClass("is-on", running).toggleClass("is-off", !running);
        $("#dash_load_value").text(loadValue);

        if (data.version_display || data.product_name) {
            $("#dash_version_value").text(data.version_display || data.product_name || "—");
        }
    }

    function refreshDashboardSummary() {
        setDashboardRefreshBusy("#dash_summary_refresh_btn", true);

        return $.ajax({
            url: "view/ajax_data.php?id=dashsummary",
            type: "GET",
            dataType: "json"
        })
        .done(function (summary) {
            var blockedIpCount = getDashboardBlockedCount(summary),
                throughput = [summary.throughput_in_display, summary.throughput_out_display]
                    .filter(function (part) { return !!part; })
                    .join(" / ");

            updateDashboardStatusbar(summary);

            $("#dash_req_value").text(summary.req_per_sec_display || "0");
            $("#dash_req_processing").text(formatDashboardCount(summary.req_processing));
            $("#dash_conn_value").text(formatDashboardCount(summary.plain_conn));
            $("#dash_conn_max").text(formatDashboardCount(summary.max_conn));
            $("#dash_ssl_value").text(formatDashboardCount(summary.ssl_conn));
            $("#dash_ssl_max").text(formatDashboardCount(summary.max_ssl_conn));
            $("#dash_throughput_value").text(throughput || "—");
            $("#dash_cache_value").text(formatDashboardCount(summary.total_cache_per_sec, "0"));
            $("#dash_cache_public").text(formatDashboardCount(summary.pub_cache_per_sec, "0"));
            $("#dash_cache_private").text(formatDashboardCount(summary.priv_cache_per_sec, "0"));
            $("#dash_static_value").text(formatDashboardCount(summary.static_hits_per_sec, "0"));
            $("#dash_blocked_value").text(formatDashboardCount(blockedIpCount));

            $("#dash_listener_running").text(formatDashboardCount(summary.l_running));
            $("#dash_listener_broken").text(formatDashboardCount(summary.l_broken));
            $("#dash_vh_running").text(formatDashboardCount(summary.v_running));
            $("#dash_vh_disabled").text(formatDashboardCount(summary.v_disabled));
            $("#dash_vh_error").text(formatDashboardCount(summary.v_err));
            updateDashboardBlockedLink(blockedIpCount);

            applyCardState($("#dash_card_connections"), metricStateByRatio(parseInt(summary.plain_conn, 10), parseInt(summary.max_conn, 10)));
            applyCardState($("#dash_card_ssl"), metricStateByRatio(parseInt(summary.ssl_conn, 10), parseInt(summary.max_ssl_conn, 10)));
            applyCardState($("#dash_card_cache"), parseFloat(summary.total_cache_per_sec) > 0 ? "is-success" : "");
            applyCardState($("#dash_card_static"), parseFloat(summary.static_hits_per_sec) > 0 ? "is-success" : "");
            applyCardState($("#dash_card_requests"), parseInt(summary.req_processing, 10) > 0 ? "is-success" : "");
            applyCardState($("#dash_listeners_card"), parseInt(summary.l_broken, 10) > 0 ? "is-attention" : "");
            applyCardState($("#dash_vhosts_card"), (parseInt(summary.v_disabled, 10) > 0 || parseInt(summary.v_err, 10) > 0) ? "is-attention" : "");
            applyCardState($("#dash_card_blocked"), blockedIpCount > 0 ? "is-attention" : "");

            lst_refreshFooterTime();
        })
        .always(function () {
            setDashboardRefreshBusy("#dash_summary_refresh_btn", false);
        });
    }

    function refreshDashboardStatusbar() {
        return $.ajax({
            url: "view/ajax_data.php?id=pid_load",
            type: "GET",
            dataType: "json"
        })
        .done(function (data) {
            if (data && data.hasOwnProperty && data.hasOwnProperty("login_timeout")) {
                window.location.href = "/login.php?timedout=1";
                return;
            }

            updateDashboardStatusbar(data);
            lst_refreshFooterTime();
        });
    }

    function reverseLogRows(html) {
        return $("<tbody>" + (html || "") + "</tbody>").children("tr").get().reverse();
    }

    function formatLogMeta(log) {
        var levelText = dashboardTableText.logLevels[parseInt(log.loglevel, 10)] || dashboardTableText.logLevels[1];

        return dashboardTableText.logWindow + " • "
            + dashboardTableText.logCompact
                .replace("%%outlines%%", formatDashboardCount(log.logfound))
                .replace("%%totallines%%", formatDashboardCount(log.logsearched))
                .replace("%%level%%", levelText);
    }

    var lastDashboardLogBody = null;

    function refreshDashboardLog() {
        var logdiv = $("#log_widget"),
            emptyState = $("#dash_logempty"),
            logTable = $("#dash_logtbl"),
            logBody = $("#dash_logbody");

        setDashboardRefreshBusy("#dash_log_refresh_btn", true);

        return $.ajax({
            url: "view/ajax_data.php?id=dashlog",
            type: "GET",
            dataType: "json",
            data: {level: "error"}
        })
        .done(function (log) {
            var hasErrors = parseInt(log.logfound, 10) > 0,
                bodyChanged = (log.log_body !== lastDashboardLogBody);

            setDebugLogStatus(logdiv, log.debuglog);
            $("#dash_logsummary").text(formatDashboardCount(log.logfound) + " ERROR");
            $("#dash_logmeta").text(formatLogMeta(log));

            if (bodyChanged) {
                var rows = reverseLogRows(log.log_body);
                logBody.empty().append(rows);
                lastDashboardLogBody = log.log_body;
            }
            emptyState.prop("hidden", hasErrors);
            logTable.toggle(hasErrors);
            $(".lst-log-viewer-link").toggleClass("lst-card-link--primary", hasErrors);

            lst_refreshFooterTime();
        })
        .always(function () {
            setDashboardRefreshBusy("#dash_log_refresh_btn", false);
        });
    }

    var dashboardStatusTimer = null;

    function stopDashboardPolling() {
        if (dashboardStatusTimer !== null) {
            clearInterval(dashboardStatusTimer);
            dashboardStatusTimer = null;
        }
    }

    function startDashboardPolling() {
        if (dashboardStatusTimer === null) {
            dashboardStatusTimer = setInterval(refreshDashboardStatusbar, 60000);
        }
    }

    function syncDashboardPolling() {
        if (document.visibilityState === "hidden") {
            stopDashboardPolling();
        }
        else {
            refreshDashboardStatusbar();
            startDashboardPolling();
        }
    }

    var pagefunction = function () {
        refreshDashboardSummary();
        refreshDashboardLog();
        startDashboardPolling();

        $("#toggledebug").on("click", function (event) {
            event.preventDefault();

            lstShowConfirmPopover(this, {
                title: "<i class='lst-icon lst-dialog-title-icon lst-dialog-title-icon--danger' data-lucide='bug'></i> <span class='lst-dialog-title-text'><strong><?php DMsg::EchoUIStr('service_toggledebug') ?></strong></span>",
                content: "<?php DMsg::EchoUIStr('service_toggledebugmsg') ?>",
                buttons: '<?php echo '[' . DMsg::UIStr('btn_cancel') . '][' . DMsg::UIStr('btn_go') . ']' ; ?>',
                placement: "bottom"
            }, function (buttonPressed) {
                if (buttonPressed !== "<?php DMsg::EchoUIStr('btn_go') ?>") {
                    return;
                }

                $.ajax({
                    type: "POST",
                    url: "view/serviceMgr.php",
                    data: {"act": "toggledebug"},
                    beforeSend: function () {
                        lstNotifyToast({
                            title: "<?php DMsg::EchoUIStr('service_requesting') ?>",
                            content: "<i class='lst-icon' data-lucide='clock'></i> <i><?php DMsg::EchoUIStr('service_willrefresh') ?></i>",
                            color: "#659265",
                            timeout: 2200
                        });
                    },
                    success: function () {
                        window.setTimeout(refreshDashboardLog, 1600);
                    }
                });
            });
        });

        $("#dash_blocked_link")
            .off("click.dashboard")
            .on("click.dashboard", function () {
                openDashboardBlockedPreview();
            });

        $("#dash_summary_refresh_btn")
            .off("click.dashboard")
            .on("click.dashboard", function () {
                refreshDashboardSummary();
            });

        $("#dash_log_refresh_btn")
            .off("click.dashboard")
            .on("click.dashboard", function () {
                refreshDashboardLog();
            });

        $(document)
            .off("visibilitychange.dashboard")
            .on("visibilitychange.dashboard", syncDashboardPolling);
        $(window)
            .off("beforeunload.dashboard")
            .on("beforeunload.dashboard", stopDashboardPolling);
    };

    pagefunction();
</script>
