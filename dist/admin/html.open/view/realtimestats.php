<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\RealTimeStats;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\UI\RealtimeStatsRenderer;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

echo UI::content_header('activity', DMsg::UIStr('menu_rtstats'));

$label_reqprocessing = DMsg::UIStr('service_reqprocessing');
$label_reqpersec = DMsg::UIStr('service_reqpersec');
$label_maxconn = DMsg::UIStr('service_maxconn');
$label_sslconn = DMsg::UIStr('service_sslconn');
$label_plainconn = DMsg::UIStr('service_plainconn');
$label_totalreq = DMsg::UIStr('service_totalreq');
$label_eapcount = DMsg::UIStr('service_eapcount');
$label_eapinuse = DMsg::UIStr('service_eapinuse');
$label_eapidle = DMsg::UIStr('service_eapidle');
$label_eapwaitq = DMsg::UIStr('service_eapwaitq');
$label_eapreqpersec = DMsg::UIStr('service_eapreqpersec');
$label_pubcachepersec = DMsg::UIStr('service_pubcachepersec');
$label_privcachepersec = DMsg::UIStr('service_privcachepersec');
$label_statichitspersec = DMsg::UIStr('service_statichitspersec');
$label_totalcachehits = DMsg::UIStr('service_totalcachehits');
$label_name = DMsg::UIStr('l_name');
$blockedIpCountLabel = '<a class="lst-stats-inline-link" href="index.php?view=blockedips">' . UIBase::Escape(DMsg::UIStr('service_blockedipcnt')) . '</a>';
$requestProcessingLabel = UI::GetRequestProcessingLabel($label_reqprocessing);
$th = '</th><th>';

$servstatbottom = [
		[
				[RealTimeStats::FLD_UPTIME, DMsg::UIStr('service_uptime'), 'success', ''],
				[RealTimeStats::FLD_S_TOT_REQS, $label_totalreq, 'muted', ''],
				[RealTimeStats::FLD_BLOCKEDIP_COUNT, $blockedIpCountLabel, '', 'pinkDark']
		],
		[
				[RealTimeStats::FLD_AVAILCONN, DMsg::UIStr('service_availconn'), 'success', ''],
				[RealTimeStats::FLD_PLAINCONN, $label_plainconn, 'orange', 'pinkDark'],
				[RealTimeStats::FLD_MAXCONN, $label_maxconn, 'muted', '']
		],
		[
				[RealTimeStats::FLD_AVAILSSL, DMsg::UIStr('service_availssl'), 'success', ''],
				[RealTimeStats::FLD_SSLCONN, $label_sslconn, 'orange', 'pinkDark'],
				[RealTimeStats::FLD_MAXSSL_CONN, DMsg::UIStr('service_maxsslconn'), 'muted', '']
		],
		[
				[RealTimeStats::FLD_S_REQ_PROCESSING, $requestProcessingLabel, 'success', 'pinkDark'],
				[RealTimeStats::FLD_S_REQ_PER_SEC, $label_reqpersec, 'success', 'green']
		],
		[
				[RealTimeStats::FLD_S_PUB_CACHE_HITS_PER_SEC, $label_pubcachepersec, 'success', 'green'],
				[RealTimeStats::FLD_S_PRIV_CACHE_HITS_PER_SEC, $label_privcachepersec, 'orange', 'pinkDark'],
				[RealTimeStats::FLD_S_STATIC_HITS_PER_SEC, $label_statichitspersec, 'success', '']
		]
];

$servstatplot = [
			[RealTimeStats::FLD_BPS_IN, DMsg::UIStr('service_bpsin'), false],
			[RealTimeStats::FLD_BPS_OUT, DMsg::UIStr('service_bpsout'), false],
			[RealTimeStats::FLD_SSL_BPS_IN, DMsg::UIStr('service_sslbpsin'), true],
			[RealTimeStats::FLD_SSL_BPS_OUT, DMsg::UIStr('service_sslbpsout'), true],
			[RealTimeStats::FLD_PLAINCONN, $label_plainconn, true],
			[RealTimeStats::FLD_IDLECONN, DMsg::UIStr('service_idleconn'), false],
			[RealTimeStats::FLD_SSLCONN, $label_sslconn, true],
		[RealTimeStats::FLD_S_REQ_PROCESSING, $label_reqprocessing, false],
		[RealTimeStats::FLD_S_REQ_PER_SEC, $label_reqpersec, true],
		[RealTimeStats::FLD_S_PUB_CACHE_HITS_PER_SEC, $label_pubcachepersec, false],
		[RealTimeStats::FLD_S_PRIV_CACHE_HITS_PER_SEC, $label_privcachepersec, false],
		[RealTimeStats::FLD_S_STATIC_HITS_PER_SEC, $label_statichitspersec, false]
];

$vhstatbottom = [
		[
				[RealTimeStats::FLD_VH_TOT_REQS, $label_totalreq, 'muted', ''],
				[RealTimeStats::FLD_VH_EAP_COUNT, $label_eapcount, '', 'pinkDark']
		],
		[
				[RealTimeStats::FLD_VH_EAP_IDLE, $label_eapidle, 'success', ''],
				[RealTimeStats::FLD_VH_EAP_INUSE, $label_eapinuse, 'orange', 'pinkDark'],
		],
		[
				[RealTimeStats::FLD_VH_EAP_WAITQUE, $label_eapwaitq, 'orange', 'red'],
				[RealTimeStats::FLD_VH_EAP_REQ_PER_SEC, $label_eapreqpersec, 'success', 'green'],
		],
		[
				[RealTimeStats::FLD_VH_REQ_PROCESSING, $label_reqprocessing, 'success', 'pinkDark'],
				[RealTimeStats::FLD_VH_REQ_PER_SEC, $label_reqpersec, 'success', 'green']
		],
		[
				[RealTimeStats::FLD_VH_PUB_CACHE_HITS_PER_SEC, $label_pubcachepersec, 'success', 'green'],
				[RealTimeStats::FLD_VH_PRIV_CACHE_HITS_PER_SEC, $label_privcachepersec, 'orange', 'pinkDark'],
				[RealTimeStats::FLD_VH_STATIC_HITS_PER_SEC, $label_statichitspersec, 'success', '']
		]
];

$vhstatplot = [
		[RealTimeStats::FLD_VH_EAP_INUSE, $label_eapinuse, true],
		[RealTimeStats::FLD_VH_EAP_WAITQUE, $label_eapwaitq, true],
		[RealTimeStats::FLD_VH_EAP_IDLE, $label_eapidle, true],
		[RealTimeStats::FLD_VH_EAP_REQ_PER_SEC, $label_eapreqpersec, true],
		[RealTimeStats::FLD_VH_REQ_PROCESSING, $label_reqprocessing, true],
		[RealTimeStats::FLD_VH_REQ_PER_SEC, $label_reqpersec, true],
		[RealTimeStats::FLD_VH_PUB_CACHE_HITS_PER_SEC, $label_pubcachepersec, false],
		[RealTimeStats::FLD_VH_PRIV_CACHE_HITS_PER_SEC, $label_privcachepersec, false],
		[RealTimeStats::FLD_VH_STATIC_HITS_PER_SEC, $label_statichitspersec, false]
];

$label_sec = DMsg::UIStr('service_seconds');
$label_min = DMsg::UIStr('service_minutes');

$refreshOptions = [
		0 => DMsg::UIStr('service_stop'),
		10 => "10 $label_sec",
		15 => "15 $label_sec",
		30 => "30 $label_sec",
		60 => "60 $label_sec",
		120 => "2 $label_min",
		300 => "5 $label_min"
];

$topOptions = [
		10 => 'Top 10', 20 => 'Top 20', 50 => 'Top 50', 100 => 'Top 100', 200 => 'Top 200'
];

$vhSortOptions = [
		RealTimeStats::FLD_VH_REQ_PER_SEC => $label_reqpersec,
		RealTimeStats::FLD_VH_REQ_PROCESSING => $label_reqprocessing,
		RealTimeStats::FLD_VH_TOT_REQS => $label_totalreq,
		RealTimeStats::FLD_VH_EAP_COUNT => $label_eapcount,
		RealTimeStats::FLD_VH_EAP_INUSE => $label_eapinuse,
		RealTimeStats::FLD_VH_EAP_WAITQUE => $label_eapwaitq,
		RealTimeStats::FLD_VH_EAP_IDLE => $label_eapidle,
		RealTimeStats::FLD_VH_EAP_REQ_PER_SEC => $label_eapreqpersec,
		RealTimeStats::FLD_VH_PUB_CACHE_HITS_PER_SEC => $label_pubcachepersec,
		RealTimeStats::FLD_VH_STATIC_HITS_PER_SEC => $label_statichitspersec
];

?>

<section class="lst-stats-page lst-rtstats-page">
		<article>

				<div class="lst-widget lst-live-widget">
				<header class="lst-widget-header">
				<span class="lst-widget-icon"><i class="lst-icon" data-lucide="bar-chart-2"></i></span>
				<h2><?php DMsg::EchoUIStr('service_livefeeds')?></h2>
				<div class="lst-widget-toolbar lst-live-toolbar" role="menu">
					<span class="lst-switch-title"><?php DMsg::EchoUIStr('service_refreshinterval')?></span>
					<select id="refresh-interval" class="lst-control-compact lst-live-interval">
<?php echo UIBase::genOptions($refreshOptions, 15); ?>
					</select>
				</div>
				</header>
				<div class="lst-widget-frame">


					<div class="lst-widget-body">
					<ul class="lst-tabs lst-tabs--compact lst-tabs--config-hover lst-live-tabs" id="liveTab">
							<li class="active lst-live-tab lst-live-tab--base" data-vn="serv"><a data-lst-tab href="#serv"><i
									class="lst-icon" data-lucide="server-cog"></i> <span
									class="lst-hide-compact"><?php DMsg::EchoUIStr('service_server')?></span> </a>
							</li>
						</ul>
						<div class="lst-live-content lst-tab-panel-group">

<?php
	echo '<div id="vhhide" hidden>' . RealtimeStatsRenderer::renderPlotTab('vh', $vhstatbottom, $vhstatplot, 0) . "</div>\n";
	echo RealtimeStatsRenderer::renderPlotTab('serv', $servstatbottom, $servstatplot, 1);
?>


</div>

</div>
</div></div>

</article>

<hr class="simple">
<article>
		<div class="lst-widget lst-snapshot-widget">
		<header class="lst-widget-header">
			<span class="lst-widget-icon"><i class="lst-icon" data-lucide="camera"></i>
			</span>
			<h2>
				Snapshot
				<span class="lst-rtstats-header-meta">
					<small class="lst-refresh-stamp lst-rtstats-refresh-stamp" id="refresh_stamp"></small>
					<button type="button" id="snapshot_refresh_btn" class="lst-btn lst-btn--secondary lst-btn--sm lst-rtstats-refreshbtn" data-lst-call="refreshVh" rel="tooltip" data-original-title="<?php DMsg::EchoUIStr('btn_refreshnow')?>">
						<i class="lst-icon" data-lucide="refresh-cw"></i>
					</button>
				</span>
			</h2>
			<ul class="lst-tabs lst-tabs--compact lst-tabs--config-hover lst-tab-trailing-nav">
				<li class="active"><a href="#vhoststab" data-lst-tab><i
						class="lst-icon" data-lucide="server"></i> <?php DMsg::EchoUIStr('menu_vh')?> </a>
				</li>
				<li><a href="#expstab" data-lst-tab><i
						class="lst-icon" data-lucide="external-link"></i> <?php DMsg::EchoUIStr('tab_ext')?></a>
				</li>
			</ul>
			<div class="lst-widget-toolbar lst-snapshot-controls" role="menu">
				<div class="lst-snapshot-controlgroup lst-snapshot-controlgroup--fetch">
					<span class="lst-snapshot-control-label"><?php DMsg::EchoUIStr('service_retrieve')?></span>
					<select id="vh_topn" name="vh_topn" class="lst-control-compact">
<?php echo UIBase::genOptions($topOptions, 10); ?>
					</select>
					<span class="lst-snapshot-control-label"><?php DMsg::EchoUIStr('service_by')?></span>
					<select name="vh_sort" id="vh_sort" class="lst-control-compact">
<?php echo UIBase::genOptions($vhSortOptions, RealTimeStats::FLD_VH_REQ_PER_SEC); ?>
					</select>
				</div>
				<div class="lst-snapshot-controlgroup lst-snapshot-controlgroup--filter">
					<label class="lst-snapshot-control-label" for="vh_filter"><?php DMsg::EchoUIStr('service_filterbyvn')?></label>
					<input class="lst-filter-input lst-control-compact lst-snapshot-filter-input" type="text"
						name="vh_filter" id="vh_filter" placeholder="<?php DMsg::EchoUIStr('service_nameallowexp')?>" value="">
				</div>
			</div>
		</header>

		<div class="lst-tab-panel-group">
			<div class="lst-tab-view is-active" id="vhoststab" aria-hidden="false">
				<table id="dt_vhstatus" class="table lst-data-grid-table lst-data-grid-table--interactive lst-snapshot-table" width="100%">
					<thead>
					<tr>
						<th class="lst-snapshot-actioncol"></th>
						<th class="lst-snapshot-namecol"><?php echo $label_name; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_reqprocessing; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_reqpersec; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_totalreq; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_eapcount; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_eapinuse; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_eapidle; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_eapwaitq; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_eapreqpersec; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_pubcachepersec; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_statichitspersec; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_totalcachehits; ?></th>
					</tr>
					</thead>
					<tbody id="vh_body">
					</tbody>
				</table>
			</div>
			<div class="lst-tab-view" id="expstab" aria-hidden="true">
		        <table id="dt_expstatus" class="table lst-data-grid-table lst-data-grid-table--interactive lst-snapshot-table" width="100%">
					<thead>
					<tr>
						<th><?php DMsg::EchoUIStr('service_scope')?></th>
						<th><?php DMsg::EchoUIStr('l_type')?></th>
						<th class="lst-snapshot-namecol"><?php echo $label_name; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_maxconn; ?></th>
						<th class="lst-snapshot-num"><?php DMsg::EchoUIStr('service_effmax')?></th>
						<th class="lst-snapshot-num"><?php DMsg::EchoUIStr('service_pool')?></th>
						<th class="lst-snapshot-num"><?php DMsg::EchoUIStr('service_inuse')?></th>
						<th class="lst-snapshot-num"><?php DMsg::EchoUIStr('service_idle')?></th>
						<th class="lst-snapshot-num"><?php DMsg::EchoUIStr('service_waitq')?></th>
						<th class="lst-snapshot-num"><?php echo $label_reqpersec; ?></th>
						<th class="lst-snapshot-num"><?php echo $label_totalreq; ?></th>
					</tr>
					</thead>
					<tbody id="exp_body">
					</tbody>
				</table>
			</div>
		</div>
		</div>
</article>

<hr class="simple">
	<article>
		<div class="lst-widget lst-service-status-widget" id="listener_status_widget">
			<header class="lst-widget-header">
				<span class="lst-widget-icon"><i class="lst-icon" data-lucide="plug-zap"></i></span>
				<h2><?php DMsg::EchoUIStr('menu_sl')?>
					<span class="lst-rtstats-header-meta">
						<small class="lst-refresh-stamp lst-rtstats-refresh-stamp" id="listener_status_stamp"></small>
						<button type="button" id="listener_status_refresh_btn" class="lst-btn lst-btn--secondary lst-btn--sm lst-rtstats-refreshbtn" data-lst-call="refreshStatusTables" rel="tooltip" data-original-title="<?php DMsg::EchoUIStr('btn_refreshnow')?>">
							<i class="lst-icon" data-lucide="refresh-cw"></i>
						</button>
					</span>
				</h2>
				<div class="lst-widget-toolbar lst-service-status-pills" role="status" aria-live="polite">
					<span class="lst-status-pill lst-status-pill--success">
						<span id="listener_running" class="lst-status-count">0</span>
						<span class="lst-status-text"><?php DMsg::EchoUIStr('service_running')?></span>
					</span>
					<span class="lst-status-pill lst-status-pill--danger">
						<span id="listener_broken" class="lst-status-count">0</span>
						<span class="lst-status-text"><?php DMsg::EchoUIStr('service_notrunning')?></span>
					</span>
				</div>
			</header>
			<div class="lst-widget-content">
				<div class="lst-widget-body lst-table-body">
					<div class="lst-status-table-toolbar lst-table-toolbar">
						<div class="lst-listener-status-summary lst-table-toolbar__summary" aria-live="polite"></div>
						<div class="lst-listener-status-search lst-table-toolbar__search"></div>
						<div class="lst-listener-status-rows lst-table-toolbar__rows"></div>
					</div>
					<table id="dt_listenerstatus" class="table lst-data-grid-table lst-data-grid-table--interactive" width="100%">
						<thead>
						<tr>
							<th><?php DMsg::EchoUIStr('l_name')?></th>
							<th><?php DMsg::EchoUIStr('l_address')?></th>
							<th><?php DMsg::EchoUIStr('l_vhmappedvhosts')?></th>
							<th><?php DMsg::EchoUIStr('l_vhmappedcount')?></th>
							<th class="lst-state-actioncol"><?php DMsg::EchoUIStr('l_state')?></th>
						</tr>
						</thead>
						<tbody id="listener_status_body"></tbody>
					</table>
				</div>
			</div>
		</div>
	</article>

	<article>
		<div class="lst-widget lst-service-status-widget" id="vhost_status_widget">
			<header class="lst-widget-header">
				<span class="lst-widget-icon"><i class="lst-icon" data-lucide="server"></i></span>
				<h2><?php DMsg::EchoUIStr('menu_vh')?>
					<span class="lst-rtstats-header-meta">
						<small class="lst-refresh-stamp lst-rtstats-refresh-stamp" id="vhost_status_stamp"></small>
						<button type="button" id="vhost_status_refresh_btn" class="lst-btn lst-btn--secondary lst-btn--sm lst-rtstats-refreshbtn" data-lst-call="refreshStatusTables" rel="tooltip" data-original-title="<?php DMsg::EchoUIStr('btn_refreshnow')?>">
							<i class="lst-icon" data-lucide="refresh-cw"></i>
						</button>
					</span>
				</h2>
				<div class="lst-widget-toolbar lst-service-status-pills" role="status" aria-live="polite">
					<span class="lst-status-pill lst-status-pill--success">
						<span id="vh_running" class="lst-status-count">0</span>
						<span class="lst-status-text"><?php DMsg::EchoUIStr('service_active')?></span>
					</span>
					<span class="lst-status-pill lst-status-pill--warning">
						<span id="vh_disabled" class="lst-status-count">0</span>
						<span class="lst-status-text"><?php DMsg::EchoUIStr('service_disabled')?></span>
					</span>
					<span class="lst-status-pill lst-status-pill--danger">
						<span id="vh_error" class="lst-status-count">0</span>
						<span class="lst-status-text"><?php DMsg::EchoUIStr('service_error')?></span>
					</span>
				</div>
			</header>
			<div class="lst-widget-content">
				<div class="lst-widget-body lst-table-body">
					<div class="lst-status-table-toolbar lst-table-toolbar">
						<div class="lst-vhost-status-summary lst-table-toolbar__summary" aria-live="polite"></div>
						<div class="lst-vhost-status-search lst-table-toolbar__search"></div>
						<div class="lst-vhost-status-rows lst-table-toolbar__rows"></div>
					</div>
					<table id="dt_vhservicestatus" class="table lst-data-grid-table lst-data-grid-table--interactive" width="100%">
						<thead>
						<tr>
							<th><?php DMsg::EchoUIStr('l_name')?></th>
							<th><?php DMsg::EchoUIStr('service_intemplate')?></th>
							<th><?php DMsg::EchoUIStr('l_domains')?></th>
							<th class="lst-state-actioncol"><?php DMsg::EchoUIStr('l_state')?></th>
						</tr>
						</thead>
						<tbody id="vhost_status_body"></tbody>
					</table>
				</div>
			</div>
		</div>
	</article>
</section>


<script type="text/javascript">
	/* DO NOT REMOVE : GLOBAL FUNCTIONS!
	 *
	 * pageSetUp(); WILL CALL THE FOLLOWING FUNCTIONS
	 *
	 * // initialize shared tooltip/popover runtime
	 * lstInitTooltips(document);
	 * lstInitPopovers(document);
	 *
	 * // activate inline charts
	 * runAllCharts();
	 *
	 * // run form elements
	 * runAllForms();
	 *
	 ********************************
	 *
	 * pageSetUp() is needed whenever you load a page.
	 * It initializes and checks for all basic elements of the page
	 * and makes rendering easier.
	 *
	 */

	pageSetUp();

	// PAGE RELATED SCRIPTS

	var statusTableText = {
		showing: <?php echo json_encode(DMsg::UIStr('note_showing')); ?>,
		rows: <?php echo json_encode(DMsg::UIStr('note_rows')); ?>,
		filter: <?php echo json_encode(DMsg::UIStr('note_filter')); ?>,
		refresh: <?php echo json_encode(DMsg::UIStr('btn_refresh')); ?>,
		refreshNow: <?php echo json_encode(DMsg::UIStr('btn_refreshnow')); ?>,
		lastLoadedAt: <?php echo json_encode(DMsg::UIStr('service_lastloadedat')); ?>,
		localTime: <?php echo json_encode(DMsg::UIStr('service_localtime')); ?>,
		highestSince: <?php echo json_encode(DMsg::UIStr('service_highestsince')); ?>,
		prev: <?php echo json_encode(DMsg::UIStr('btn_prev')); ?>,
		next: <?php echo json_encode(DMsg::UIStr('btn_next')); ?>
	};

	// vh variables
	var snapshotRefreshButton = $("#snapshot_refresh_btn"),
	refreshstamp = $("#refresh_stamp"),
	dt_vhstatus = $("#dt_vhstatus"),
	dt_expstatus = $("#dt_expstatus"),
	vh_body = $("#vh_body"),
	exp_body = $("#exp_body"),
		listenerStatusWidget = $("#listener_status_widget"),
		vhostStatusWidget = $("#vhost_status_widget"),
		listenerStatusStamp = $("#listener_status_stamp"),
		vhostStatusStamp = $("#vhost_status_stamp"),
		listenerStatusRefreshButton = $("#listener_status_refresh_btn"),
		vhostStatusRefreshButton = $("#vhost_status_refresh_btn"),
		dt_listenerstatus = $("#dt_listenerstatus"),
		dt_vhservicestatus = $("#dt_vhservicestatus"),
		listener_status_body = $("#listener_status_body"),
		vhost_status_body = $("#vhost_status_body"),
		listenerStatusSlots = {
			summary: ".lst-listener-status-summary",
			search: ".lst-listener-status-search",
			rows: ".lst-listener-status-rows"
		},
		vhostStatusSlots = {
			summary: ".lst-vhost-status-summary",
			search: ".lst-vhost-status-search",
			rows: ".lst-vhost-status-rows"
		},
		vh_filter = $("#vh_filter"),
		vh_topn = $("#vh_topn"),
		vh_sort = $("#vh_sort"),
		livetabs = $("#liveTab"),
	allow_monitor_max = 3,
	dataset = [[]],
	liveWindowMinutes = 15,
	liveWindowMs = 15 * 60 * 1000,
	realtimePollIntervalId = null,
	snapshotRefreshInFlight = false,
	statusRefreshPending = false,
	statusRefreshInFlight = false,
	statusRefreshViewportTimer = null;

	function syncLiveTabStrip()
	{
		var hasMonitorTabs = livetabs.children("li").not(".lst-live-tab--base").length > 0;
		livetabs.toggleClass("lst-live-tabs--empty", !hasMonitorTabs);
	}

	function formatRealtimeStamp(date)
	{
		return date.toLocaleTimeString([], {
			hour: "2-digit",
			minute: "2-digit",
			second: "2-digit"
		});
	}

	function setRealtimeStamp(target, date)
	{
		var stamp = $(target),
			value = date || new Date(),
			zoneLabel = "";

		try {
			zoneLabel = Intl.DateTimeFormat().resolvedOptions().timeZone || "";
		} catch (ignore) {
			zoneLabel = "";
		}

		stamp.text(formatRealtimeStamp(value));
		stamp.attr("data-original-title", statusTableText.lastLoadedAt + " " + value.toLocaleString() + " (" + (zoneLabel || statusTableText.localTime) + ")");
	}

	function setRealtimeRefreshButtonState(target, isLoading)
	{
		var button = $(target),
			icon = button.find(".lst-icon");

		button.toggleClass("is-loading", !!isLoading);
		button.prop("disabled", !!isLoading);
		button.attr("aria-busy", isLoading ? "true" : null);
		icon.toggleClass("lst-spin", !!isLoading);
	}

	function getSnapshotTableOptions()
	{
		return {
			autoWidth: false,
			paging: false,
			searching: false,
			info: false,
			ordering: false
		};
	}

	function updateSnapshotTableRows(tableSelector, body, rowMarkup)
	{
		var api = lstGetDataTableApi(tableSelector),
			rows = $("<tbody>").html(rowMarkup || "").children("tr");

		if (!api) {
			body.html(rowMarkup);
			return;
		}

		api.clear();
		if (rows.length) {
			api.rows.add(rows.toArray());
		}
		api.draw(false);
	}

	function ensureSnapshotTable(tableSelector)
	{
		if (!$.fn.dataTable || !$.fn.dataTable.isDataTable(tableSelector)) {
			$(tableSelector).DataTable(getSnapshotTableOptions());
		}
	}

	function renderMonitorConfirmPopover(vhname)
	{
		var escapeHtml = window.lstEscapeHtml || function(value) {
			return String(value == null ? '' : value);
		};
		var escapeAttr = window.lstEscapeHtmlAttr || escapeHtml;

		return '<div class="lst-monitor-popover">'
				+ '<p class="lst-monitor-popover__text">' + <?php echo json_encode(DMsg::UIStr('service_confirmmonitor') . ' '); ?> + '<strong>' + escapeHtml(vhname) + '</strong>?</p>'
				+ '<div class="lst-monitor-popover__actions">'
				+ '<button type="button" class="lst-btn lst-btn--primary lst-btn--sm" data-lst-call="addMonitorTab" data-lst-call-arg="' + escapeAttr(vhname) + '">' + <?php echo json_encode(DMsg::UIStr('service_addtomonitor')); ?> + '</button>'
				+ '<button type="button" class="lst-btn lst-btn--secondary lst-btn--sm" data-lst-call="dismissMonitorConfirm">' + <?php echo json_encode(DMsg::UIStr('btn_cancel')); ?> + '</button>'
				+ '</div>'
				+ '</div>';
	}

	function dismissMonitorConfirm()
	{
		if (typeof window.lstHidePopover === "function") {
			window.lstHidePopover();
		}
	}

	function addMonitorTab(vhname) {
		var vhid = vhname.replace(/\./g, '_');
		var escapeHtml = window.lstEscapeHtml || function(value) {
			return String(value == null ? '' : value);
		};
		var escapeAttr = window.lstEscapeHtmlAttr || escapeHtml;
		var shortName = vhname.substring(0, 30);
		var tabname = (vhname.length < 30)
			? escapeHtml(vhname)
			: '<abbr rel="tooltip" data-placement="bottom" data-original-title="' + escapeAttr(vhname) + '">' + escapeHtml(shortName) + '</abbr>';
		var existingTab = getMonitorTabItem(vhname);
		var newtabli;

		dismissMonitorConfirm();

		if (existingTab.length) {
			syncSnapshotMonitorButtons();
			focusMonitorTab(vhname);
			return false;
		}

		if (getMonitorTabCount() >= allow_monitor_max) {
		lstNotifyToast({
			title : <?php echo json_encode(DMsg::UIStr('service_monitormaxallowed') . ' '); ?> + allow_monitor_max + " " + <?php echo json_encode(DMsg::UIStr('menu_vh')); ?>,
			content : "",
			color : "#A65858",
				timeout : 5000
			});
			return false;
		}

		livetabs.append('<li class="lst-live-tab" data-vn="' + escapeAttr(vhname) + '"><a data-lst-tab href="#' + vhid
				+ '"><i class="lst-icon" data-lucide="server" rel="tooltip" data-placement="bottom" data-original-title="' + escapeAttr(vhname) + '"></i><span class="lst-hide-compact">'
				+ tabname + '</span><span class="lst-live-tab-close" role="button" tabindex="0" aria-label="' + <?php echo json_encode(UIBase::EscapeAttr(DMsg::UIStr('btn_close'))); ?> + '"><i class="lst-icon" data-lucide="x"></i></span></a></li>');

		var tabcontent = livetabs.closest(".lst-widget-body").find(".lst-tab-panel-group");
		var newcontent = tabcontent.find("#vhhide").html().replace(/id=\"vh\"/, 'id="' + vhid + '"');
		tabcontent.append(newcontent);

		var newtab = tabcontent.find("#" + vhid);
		initDataSet(newtab);
		syncLiveTabStrip();
		newtabli = livetabs.children().last();
		if (typeof window.lstRenderLucide === "function") {
			window.lstRenderLucide(newtabli[0]);
		}
		lstInitTooltips(newtabli);
		newtabli.on("click", ".lst-live-tab-close", function(e) {
			newtab.remove();
			var li = $(this).closest("li");
			if (li.hasClass("active")) {
				livetabs.children().first().find('a[data-lst-tab]').tab('show');
			}
			li.remove();
			syncLiveTabStrip();
			syncSnapshotMonitorButtons();
			e.stopPropagation();
			e.preventDefault();
		});
		newtabli.on("keydown", ".lst-live-tab-close", function(e) {
			if (e.key === "Enter" || e.key === " ") {
				e.preventDefault();
				$(this).trigger("click");
			}
		});

		syncSnapshotMonitorButtons();
		focusMonitorTab(vhname);
		return true;
	}

	function getMonitorTabCount()
	{
		return livetabs.children('li').not('.lst-live-tab--base').length;
	}

	function getMonitorTabItem(vhname)
	{
		return livetabs.children('li[data-vn]').not('.lst-live-tab--base').filter(function() {
			return $(this).attr('data-vn') === vhname;
		}).first();
	}

	function updateSnapshotMonitorButton(button, vhname, isMonitored)
	{
		var monitorLabel = isMonitored
			? vhname + ' ' + <?php echo json_encode(DMsg::UIStr('service_ismonitored')); ?>
			: <?php echo json_encode(DMsg::UIStr('service_addtomonitor')); ?>;
		var iconName = isMonitored ? 'monitor-check' : 'monitor-dot';
		var btn = $(button);

		if (!btn.length) {
			return;
		}

		btn.toggleClass('is-monitored', isMonitored)
			.attr('data-lst-monitored', isMonitored ? '1' : '0')
			.attr('data-original-title', monitorLabel)
			.attr('aria-label', monitorLabel)
			.html('<i class="lst-icon" data-lucide="' + iconName + '"></i>');

		if (typeof window.lstRenderLucide === 'function') {
			window.lstRenderLucide(btn[0]);
		}
	}

	function syncSnapshotMonitorButtons()
	{
		vh_body.find('tr').each(function() {
			var row = $(this);
			var vhname = (window.lstTrimText)
				? window.lstTrimText(row.find('.lst-vhname').text())
				: String(row.find('.lst-vhname').text() || '').trim();

			if (!vhname.length) {
				return;
			}

			updateSnapshotMonitorButton(row.find('[data-lstmonitor="vh"]').first(), vhname, getMonitorTabItem(vhname).length > 0);
		});
	}

	function focusMonitorTab(vhname)
	{
		var targetTab = getMonitorTabItem(vhname);
		var targetLink = targetTab.find('a[data-lst-tab]').first();
		var liveWidget = livetabs.closest('.lst-widget');

		if (!targetLink.length) {
			return false;
		}

		targetLink.tab('show');

		if (liveWidget.length && liveWidget.get(0) && typeof liveWidget.get(0).scrollIntoView === 'function') {
			liveWidget.get(0).scrollIntoView({block: 'start', behavior: 'smooth'});
		}

		return true;
	}

	function getThemeValue(name, fallback)
	{
		var value = window.getComputedStyle(document.documentElement).getPropertyValue(name);
		if (value) {
			value = (window.lstTrimText) ? window.lstTrimText(value) : String(value).trim();
		}
		return value || fallback;
	}

	function getLiveWindowCutoff(nowMs)
	{
		return nowMs - liveWindowMs;
	}

	function trimTabSeriesByWindow(tabid, nowMs)
	{
		var statKeys = Object.keys(dataset[tabid] || []),
			cutoff = getLiveWindowCutoff(nowMs),
			i,
			statIdx,
			points,
			trimIndex;

		for (i = 0; i < statKeys.length; i++) {
			statIdx = statKeys[i];
			if (!dataset[tabid][statIdx] || !Array.isArray(dataset[tabid][statIdx].data)) {
				continue;
			}

			points = dataset[tabid][statIdx].data;
			trimIndex = 0;

			while (trimIndex < points.length && points[trimIndex][0] < cutoff) {
				trimIndex++;
			}

			if (trimIndex > 0) {
				points.splice(0, trimIndex);
			}
		}
	}

	function trimAllLiveSeries(nowMs)
	{
		livetabs.find("li[data-vn]").each(function() {
			trimTabSeriesByWindow($(this).data("vn").replace(/\./g, '_'), nowMs);
		});
	}

	function ensureTabSessionState(tab)
	{
		var state = tab.data("lstSessionState"),
			startedAt;

		if (state) {
			return state;
		}

		startedAt = new Date();
		state = {
			startedAt: startedAt,
			startedAtLabel: startedAt.toLocaleString(),
			highs: {}
		};

		tab.data("lstSessionState", state);
		return state;
	}

	function updateTabSessionHighs(tab, data, sampleLabel)
	{
		var state = ensureTabSessionState(tab);

		tab.find("span.lst-stat-max").each(function(){
			var stat = $(this),
				idx = stat.data("lstStatIdx"),
				newval = Number(data[idx]),
				record = state.highs[idx],
				tooltip;

			if (!isFinite(newval) || newval <= 0) {
				return;
			}

			if (!record || newval > record.value) {
				tooltip = statusTableText.highestSince + " " + state.startedAtLabel + "<br>" + highest + " " + sampleLabel;
				state.highs[idx] = {
					value: newval,
					at: sampleLabel
				};
				stat.prop("hidden", false)
					.html(up + newval)
					.attr("data-original-title", tooltip)
					.attr("data-html", "true")
					.attr("rel", "tooltip")
					.data("curval", newval);
			}
		});
	}

	var seriesColors = (window.lstGetChartSeriesPalette)
		? window.lstGetChartSeriesPalette()
		: ["#d65a5f", "#2f7fe0", "#2fa36b", "#e08a2e", "#8a63d2", "#c65f8d", "#1f97b5", "#8e6b3b", "#5d6d85"];

	function getPlotTheme() {
		var isDark = document.documentElement.getAttribute("data-lst-theme") === "dark";
		return {
			gridColor: getThemeValue("--lst-chart-grid", isDark ? "rgba(130, 159, 199, 0.14)" : "rgba(19, 39, 67, 0.10)"),
			axisColor: getThemeValue("--lst-chart-axis", isDark ? "#9eb1cf" : "#5d6d85"),
			surface: getThemeValue("--lst-chart-surface", isDark ? "#0f1826" : "#fbfcff")
		};
	}

		function plotAccordingToChoices(tab)
		{
			var tabid = tab.attr("id");
			var plotdata = [],
				chart = tab.find(".chart-large");
			tab.find("input:checked").each(function () {
				var key = $(this).data("lstStatIdx");
				plotdata.push(dataset[tabid][key]);

			});
			if (plotdata.length) {
				window.lstRenderTimeSeriesChart(chart, plotdata, {
					windowMs: liveWindowMs,
					rangeEnd: new Date().getTime() - timezoneoffset,
					theme: getPlotTheme()
				});
			} else {
				window.lstDestroyTimeSeriesChart(chart);
			}
		}

	var up = '<i class="lst-icon" data-lucide="chevron-up"></i> ';
	var highest = <?php echo json_encode(DMsg::UIStr('service_higheston') . ' '); ?>;
		var serv_tab = $("#serv");
		initDataSet(serv_tab);
		syncLiveTabStrip();

		$(document).off('lst:themechange.realtimeStatsLive').on('lst:themechange.realtimeStatsLive', function() {
			livetabs.find('li[data-vn]').each(function() {
				var tabid = $(this).data('vn').replace(/\./g, '_');
				var tab = $('#' + tabid);
				if (tab.length) {
					plotAccordingToChoices(tab);
				}
			});
		});

	var updateInterval = 15000;
	var timezoneoffset = (new Date()).getTimezoneOffset() * 60000;

	function isLivePaneVisible(tab)
	{
		var pane = $(tab);

		return pane.length && (pane.hasClass("active") || pane.is(":visible"));
	}

	function onPlotDataReceived(series)
	{
		var curtime = new Date();
		var curlocaltime = curtime.getTime() - timezoneoffset;
		var curlocaltimestr = curtime.toLocaleString();

		livetabs.find("li").each(function() {

			var vn = $(this).data("vn");
			var data = series[vn];
			if (data == undefined)
				return;

			var tabid = vn.replace(/\./g, '_');
			var statKeys = Object.keys(dataset[tabid] || []);
			var tab = $("#" + tabid);
			ensureTabSessionState(tab);

			tab.find("span.lst-stat-val").each(function(){
				var idx = $(this).data("lstStatIdx");
				$(this).text(data[idx]);
			});
			updateTabSessionHighs(tab, data, curlocaltimestr);

			for (i = 0; i < statKeys.length ; i++) {
				var statIdx = statKeys[i];
				if (!dataset[tabid][statIdx]) {
					continue;
				}
				dataset[tabid][statIdx].data.push([curlocaltime, data[statIdx]]);
			}

			trimTabSeriesByWindow(tabid, curlocaltime);

			if (isLivePaneVisible(tab)) {
				plotAccordingToChoices(tab);
			}
		});

		lstInitTooltips(livetabs.closest(".lst-widget").find(".lst-stat-max"));


		lst_refreshFooterTime();
	}

	function initDataSet(tab)
	{
			var tabid = tab.attr("id");
			dataset[tabid] = [];
			ensureTabSessionState(tab);
			function syncToggleStates() {
				tab.find('.lst-plot-toggle').each(function () {
					var toggle = $(this),
						checked = toggle.find('input[type="checkbox"]').is(':checked');

					toggle.toggleClass('is-active', checked)
						.attr('aria-pressed', checked ? 'true' : 'false');
				});
			}
			tab.find('input[type="checkbox"]').each(function (seriesOrder) {
				var toggle = $(this).closest(".lst-plot-toggle"),
					idx = $(this).data("lstStatIdx"),
					label = (window.lstTrimText) ? window.lstTrimText(toggle.text()) : String(toggle.text()).trim(),
					color = seriesColors[seriesOrder % seriesColors.length],
					mutedColor = (window.lstColorToSoft)
						? window.lstColorToSoft(color, 0.34)
						: color,
					softColor = (window.lstColorToSoft)
						? window.lstColorToSoft(color, 0.14)
						: color;

				dataset[tabid][idx] = {"label":label, "data":[], "color": color};
				toggle.css({
					"--lst-series-color": color,
					"--lst-series-muted": mutedColor,
					"--lst-series-soft": softColor
				});
			});
			syncToggleStates();
		tab.find('input[type="checkbox"]').click(function() {
			syncToggleStates();
			plotAccordingToChoices(tab);
		});
	}

	function registerRealtimePollInterval(intervalId)
	{
		if (intervalId == null || $.inArray(intervalId, $.intervalArr) !== -1) {
			return;
		}

		$.intervalArr.push(intervalId);
	}

	function unregisterRealtimePollInterval(intervalId)
	{
		var index = $.inArray(intervalId, $.intervalArr);

		if (index !== -1) {
			$.intervalArr.splice(index, 1);
		}
	}

	function stopRealtimePolling()
	{
		if (realtimePollIntervalId === null) {
			return;
		}

		window.clearInterval(realtimePollIntervalId);
		unregisterRealtimePollInterval(realtimePollIntervalId);
		realtimePollIntervalId = null;
	}

	function startRealtimePolling()
	{
		if (realtimePollIntervalId !== null || updateInterval <= 0) {
			return;
		}

		realtimePollIntervalId = window.setInterval(function () {
			updateChart();
			refreshStatusTables(false);
		}, updateInterval);
		registerRealtimePollInterval(realtimePollIntervalId);
	}

	function restartRealtimePolling()
	{
		stopRealtimePolling();
		trimAllLiveSeries(new Date().getTime() - timezoneoffset);
		startRealtimePolling();
	}

	function updateChart()
	{
		var vnmonitor = [];
		livetabs.find("li[data-vn]").each(function() {
			vnmonitor.push($(this).data("vn"));
		});
		$.ajax({
			url: "view/ajax_data.php?id=plotstat",
			type: "POST",
			dataType: "json",
			data: {"vhnames": vnmonitor},
			success: onPlotDataReceived
		});

	}

	function updateStatusPill(counterSelector, value)
	{
		var count = parseInt(value, 10) || 0,
			counter = $(counterSelector),
			pill = counter.closest(".lst-status-pill"),
			previous = parseInt(counter.attr("data-lst-count"), 10),
			hadValue = counter.attr("data-lst-count") != null;

		if (!hadValue || previous !== count) {
			counter.text(count);
			counter.attr("data-lst-count", count);
		}
		pill.toggleClass("is-empty", count === 0);
	}

	function formatStatusSummary(api)
	{
		var info = api.page.info();

		if (!info || !info.recordsTotal) {
			return statusTableText.showing + " 0 / 0";
		}

		if (!info.recordsDisplay) {
			return statusTableText.showing + " 0 / " + info.recordsTotal;
		}

		if (info.start + 1 === info.end) {
			return statusTableText.showing + " " + info.end + " / " + info.recordsTotal;
		}

		return statusTableText.showing + " " + (info.start + 1) + "-" + info.end + " / " + info.recordsTotal;
	}

	function getStatusTableOptions()
	{
		return {
			autoWidth: false,
			info: false,
			language: lstDataTableLanguage(statusTableText)
		};
	}

	function syncStatusWidgetControls(widgetSelector, tableSelector, slots)
	{
		var widget = $(widgetSelector),
			wrapper = $(tableSelector).closest(".dt-container"),
			searchSlot = widget.find(slots.search),
			rowsSlot = widget.find(slots.rows),
			summarySlot = widget.find(slots.summary),
			filter = wrapper.find(".dt-search").first(),
			length = wrapper.find(".dt-length").first();

		if (filter.length && filter.parent()[0] !== searchSlot[0]) {
			searchSlot.append(filter);
		}

		if (length.length && length.parent()[0] !== rowsSlot[0]) {
			rowsSlot.append(length);
		}

		if ($.fn.dataTable && $.fn.dataTable.isDataTable(tableSelector)) {
			summarySlot.text(formatStatusSummary($(tableSelector).DataTable()));
		}
	}

	function updateStatusTableRows(tableSelector, body, rowMarkup)
	{
		var api = lstGetDataTableApi(tableSelector),
			rows = $("<tbody>").html(rowMarkup || "").children("tr");

		if (!api) {
			body.html(rowMarkup);
			return;
		}

		api.clear();
		if (rows.length) {
			api.rows.add(rows.toArray());
		}
		api.draw(false);
	}

	function ensureStatusTable(tableSelector, widgetSelector, slots)
	{
		if (!$.fn.dataTable || !$.fn.dataTable.isDataTable(tableSelector)) {
			$(tableSelector).DataTable(getStatusTableOptions());
			lstTuneDataTableChrome(tableSelector, {
				placeholder: statusTableText.filter,
				lengthLabel: statusTableText.rows,
				bindDraw: true,
				onUpdate: function () {
					syncStatusWidgetControls(widgetSelector, tableSelector, slots);
				}
			});
		}

		syncStatusWidgetControls(widgetSelector, tableSelector, slots);
	};

	function isNearViewport(target, threshold)
	{
		var element = $(target).get(0),
			rect,
			viewportHeight;

		if (!element) {
			return false;
		}

		rect = element.getBoundingClientRect();
		viewportHeight = window.innerHeight || document.documentElement.clientHeight || 0;
		threshold = threshold || 0;

		return rect.bottom >= (0 - threshold) && rect.top <= (viewportHeight + threshold);
	}

	function shouldRefreshStatusTables()
	{
		return isNearViewport(listenerStatusWidget, 220) || isNearViewport(vhostStatusWidget, 220);
	}

	function schedulePendingStatusRefresh()
	{
		if (!statusRefreshPending || statusRefreshInFlight) {
			return;
		}

		window.clearTimeout(statusRefreshViewportTimer);
		statusRefreshViewportTimer = window.setTimeout(function () {
			if (shouldRefreshStatusTables()) {
				refreshStatusTables(true);
			}
		}, 120);
	}

	function captureStatusRefreshLayout()
	{
		var listenerBody = listenerStatusWidget.find(".lst-table-body").first(),
			vhostBody = vhostStatusWidget.find(".lst-table-body").first();

		if (listenerBody.length) {
			listenerBody.css("min-height", listenerBody.outerHeight() + "px");
		}

		if (vhostBody.length) {
			vhostBody.css("min-height", vhostBody.outerHeight() + "px");
		}

		return {
			scrollTop: window.pageYOffset || document.documentElement.scrollTop || document.body.scrollTop || 0,
			listenerBody: listenerBody,
			vhostBody: vhostBody
		};
	}

	function restoreStatusRefreshLayout(layoutState)
	{
		if (!layoutState) {
			return;
		}

		window.requestAnimationFrame(function () {
			window.scrollTo(window.pageXOffset || 0, layoutState.scrollTop || 0);

			window.requestAnimationFrame(function () {
				layoutState.listenerBody.css("min-height", "");
				layoutState.vhostBody.css("min-height", "");
			});
		});
	}

	function refreshStatusTables(force)
	{
		force = (force !== false);

		if (statusRefreshInFlight) {
			statusRefreshPending = true;
			return;
		}

		if (document.visibilityState === "hidden") {
			statusRefreshPending = true;
			return;
		}

		if (!force && !shouldRefreshStatusTables()) {
			statusRefreshPending = true;
			return;
		}

		statusRefreshPending = false;
		statusRefreshInFlight = true;

		$.ajax({
			url: "view/ajax_data.php?id=dashstatus",
			type: "GET",
			dataType: "json",
			beforeSend: function () {
				setRealtimeRefreshButtonState(listenerStatusRefreshButton, true);
				setRealtimeRefreshButtonState(vhostStatusRefreshButton, true);
			}
		})
		.done(function(status) {
			var layoutState = captureStatusRefreshLayout(),
				refreshedAt = new Date();

			updateStatusPill("#listener_running", status.l_running);
			updateStatusPill("#listener_broken", status.l_broken);
			updateStatusPill("#vh_running", status.v_running);
			updateStatusPill("#vh_disabled", status.v_disabled);
			updateStatusPill("#vh_error", status.v_err);

			updateStatusTableRows("#dt_listenerstatus", listener_status_body, status.l_body);
			updateStatusTableRows("#dt_vhservicestatus", vhost_status_body, status.v_body);

			ensureStatusTable("#dt_listenerstatus", "#listener_status_widget", listenerStatusSlots);
			ensureStatusTable("#dt_vhservicestatus", "#vhost_status_widget", vhostStatusSlots);
			setRealtimeStamp(listenerStatusStamp, refreshedAt);
			setRealtimeStamp(vhostStatusStamp, refreshedAt);

			if (typeof window.lstRenderLucide === "function") {
				window.lstRenderLucide(listener_status_body[0]);
				window.lstRenderLucide(vhost_status_body[0]);
			}
			lstInitTooltips(listener_status_body);
			lstInitTooltips(vhost_status_body);
			lst_refreshFooterTime();
			restoreStatusRefreshLayout(layoutState);
		})
		.always(function() {
			setRealtimeRefreshButtonState(listenerStatusRefreshButton, false);
			setRealtimeRefreshButtonState(vhostStatusRefreshButton, false);
			statusRefreshInFlight = false;

			if (statusRefreshPending) {
				schedulePendingStatusRefresh();
			}
		});
	}

	function refreshVh()
	{
		if (snapshotRefreshInFlight) {
			return;
		}

		snapshotRefreshInFlight = true;

		$.ajax({
			url: "view/ajax_data.php?id=vhstat",
			type: "GET",
			dataType: "json",
			data: {vh_filter: vh_filter.val(),
					vh_topn: vh_topn.val(),
					vh_sort: vh_sort.val()},
			beforeSend : function() {
				setRealtimeRefreshButtonState(snapshotRefreshButton, true);
			}
		})
		.done(function(status) {
			var refreshedAt = new Date();

			updateSnapshotTableRows("#dt_vhstatus", vh_body, status.vbody);
			updateSnapshotTableRows("#dt_expstatus", exp_body, status.ebody);
			ensureSnapshotTable("#dt_vhstatus");
			ensureSnapshotTable("#dt_expstatus");

			setRealtimeStamp(refreshstamp, refreshedAt);
			if (typeof window.lstRenderLucide === "function") {
				window.lstRenderLucide(vh_body[0]);
			}
			syncSnapshotMonitorButtons();
			lstInitTooltips(vh_body);
			})
		.always(function() {
			setRealtimeRefreshButtonState(snapshotRefreshButton, false);
			snapshotRefreshInFlight = false;
		})
		;

	}

	window.addMonitorTab = addMonitorTab;
	window.dismissMonitorConfirm = dismissMonitorConfirm;
	window.refreshVh = refreshVh;
	window.refreshStatusTables = refreshStatusTables;

	// pagefunction

	var pagefunction = function()
	{
		startRealtimePolling();
		$(window).off("scroll.realtimeStatus resize.realtimeStatus").on("scroll.realtimeStatus resize.realtimeStatus", function() {
			schedulePendingStatusRefresh();
		});
		$(document).off("visibilitychange.realtimeStatus").on("visibilitychange.realtimeStatus", function() {
			if (document.visibilityState === "visible") {
				schedulePendingStatusRefresh();
			}
		});
		livetabs.off("shown.lstTab.realtimeStats").on("shown.lstTab.realtimeStats", 'a[data-lst-tab]', function () {
			var target = $($(this).attr("href"));

			if (target.length) {
				plotAccordingToChoices(target);
			}
		});

		$("#refresh-interval").change(function() {
			updateInterval = $(this).val() * 1000;
			restartRealtimePolling();
		});

		updateChart();

		refreshVh();
		refreshStatusTables(true);

		$('#dt_vhservicestatus').on('click', '[data-action="lstvhcontrol"]', function (e) {
			var trigger = this,
				vn = $(this).closest("tr").data("vn"),
				act = $(this).data("lstact"),
				atitle;
			if (act == 'disable') {
				atitle = <?php echo json_encode(DMsg::UIStr('service_suspendvhconfirm')); ?>;
			}
			else if (act == 'enable') {
				atitle = <?php echo json_encode(DMsg::UIStr('service_resumevhconfirm')); ?>;
			}
			vhcontrol(trigger, act, atitle, vn);
			e.preventDefault();
		});

		function vhcontrol(trigger, act, acttitle, vn) {
			lstShowConfirmPopover(trigger, {
				title: "<i class='lst-icon lst-dialog-title-icon lst-dialog-title-icon--accent' data-lucide='server'></i> <span class='lst-dialog-title-text'><strong>" + acttitle + "</strong></span>",
				content: vn,
				buttons: <?php echo json_encode('[' . DMsg::UIStr('btn_cancel') . '][' . DMsg::UIStr('btn_go') . ']'); ?>,
				placement: "left"
			}, function (buttonPressed) {
				if (buttonPressed !== <?php echo json_encode(DMsg::UIStr('btn_go')); ?>) {
					return;
				}

				$.ajax({
					type: "POST",
					url: "view/serviceMgr.php",
					data: {"act": act, "actId": vn},
					beforeSend: function () {
					lstNotifyToast({
						title: <?php echo json_encode(DMsg::UIStr('service_requesting')); ?>,
						content: "<i class='lst-icon' data-lucide='clock'></i> <i>" + <?php echo json_encode(DMsg::UIStr('service_willrefresh')); ?> + "</i>",
						color: "#659265",
						timeout: 2200
						});
					},
					success: function () {
					setTimeout(refreshStatusTables, 2000);
				}
			});
		});
	}
}


// end pagefunction

// load related plugins

lstLoadStatsPageAssets(pagefunction);


</script>
