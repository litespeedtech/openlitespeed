<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Log\LogFilter;
use LSWebAdmin\Log\LogViewer;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\Runtime\SInfo;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

$modeInput = strtolower((string) UIBase::GrabGoodInput('get', 'm'));
$mode = ($modeInput === 'search') ? 'search' : 'browse';

$browseFilter = Service::ServiceData(SInfo::DATA_VIEW_LOG);
$currentFile = (string) $browseFilter->Get(LogFilter::FLD_LOGFILE);
$currentSize = (string) $browseFilter->Get(LogFilter::FLD_FILE_SIZE);
$browseLevel = (int) $browseFilter->Get(LogFilter::FLD_LEVEL);
if ($browseLevel < 1 || $browseLevel > 5) {
    $browseLevel = LogFilter::LEVEL_NOTICE;
}
$searchLevel = LogFilter::LEVEL_INFO;

$currentStartPos = (string) $browseFilter->Get(LogFilter::FLD_FROMPOS);
$currentBlockSize = (string) LogViewer::NormalizeBrowseBlockSize($browseFilter->Get(LogFilter::FLD_BLKSIZE));
$browseDefaultBlockSize = (string) LogViewer::BROWSE_DEFAULT_BLOCK_KB;
$browseMaxBlockSize = (string) LogViewer::BROWSE_MAX_BLOCK_KB;

$browseUrl = 'index.php?view=logviewer&m=browse';
$searchUrl = 'index.php?view=logviewer&m=search';

echo UI::content_header('list', DMsg::UIStr('menu_logviewer'));

?>

<section class="lst-logviewer-page">

<article>
	<div class="lst-logviewer-card lst-logviewer-controls">
	<form class="lst-form lst-logviewer-form" name="logform" id="logform" data-lst-log-mode="<?php echo UI::EscapeAttr($mode); ?>" data-lst-browse-default-kb="<?php echo UI::EscapeAttr($browseDefaultBlockSize); ?>" data-lst-browse-max-kb="<?php echo UI::EscapeAttr($browseMaxBlockSize); ?>">
			<div class="lst-logviewer-formbody">
			<div class="lst-logviewer-headerbar">
			<span class="lst-logviewer-fileline">
				<span class="lst-logviewer-filelabel"><?php DMsg::EchoUIStr('service_filelocation')?>:</span>
				<code id="cur_log_file" class="lst-logviewer-filepath"><?php echo UI::Escape($currentFile); ?></code>
				<div class="lst-logviewer-downloadgroup">
					<div class="lst-dropdown-group" role="group" aria-label="<?php echo UI::Escape(DMsg::UIStr('btn_download')); ?>">
						<button type="button" id="downloadlog_toggle" class="lst-btn lst-btn--secondary lst-btn--sm lst-dropdown-toggle" data-lst-dropdown="true" aria-haspopup="true" aria-expanded="false">
							<i class="lst-icon" data-lucide="download" aria-hidden="true"></i>
							<span><?php DMsg::EchoUIStr('btn_download'); ?></span>
							<i class="lst-icon" data-lucide="chevron-down" aria-hidden="true"></i>
						</button>
						<ul class="lst-dropdown-menu lst-dropdown-menu--right">
							<li><a id="downloadlog_current" href="#"><?php DMsg::EchoUIStr('btn_downloadcurrentselection'); ?></a></li>
							<li><a id="downloadlog_full" href="#"><?php DMsg::EchoUIStr('btn_downloadfulllog'); ?></a></li>
						</ul>
					</div>
				</div>
			</span>
			<span class="lst-logviewer-meta">
				<span class="lst-logviewer-filelabel"><?php DMsg::EchoUIStr('service_size')?>:</span>
				<span id="cur_log_size" class="lst-logviewer-size"><?php echo UI::Escape($currentSize); ?></span> KB
			</span>
			</div>

			<ul class="lst-tabs lst-tabs--compact lst-tabs--config-hover lst-logviewer-mode-tabs" role="tablist">
				<li role="presentation"<?php if ($mode === 'browse') { echo ' class="active"'; } ?>>
					<a href="#logviewer_browse_pane" data-lst-tab data-lst-log-mode-target="browse" data-lst-history-url="<?php echo UI::EscapeAttr($browseUrl); ?>" aria-selected="<?php echo ($mode === 'browse') ? 'true' : 'false'; ?>">
						<i class="lst-icon" data-lucide="book-open-text" aria-hidden="true"></i>
						<span><?php DMsg::EchoUIStr('tab_browse'); ?></span>
					</a>
				</li>
				<li role="presentation"<?php if ($mode === 'search') { echo ' class="active"'; } ?>>
					<a href="#logviewer_search_pane" data-lst-tab data-lst-log-mode-target="search" data-lst-history-url="<?php echo UI::EscapeAttr($searchUrl); ?>" aria-selected="<?php echo ($mode === 'search') ? 'true' : 'false'; ?>">
						<i class="lst-icon" data-lucide="search" aria-hidden="true"></i>
						<span><?php DMsg::EchoUIStr('tab_search'); ?></span>
					</a>
				</li>
			</ul>

			<div class="lst-tab-panel-group lst-logviewer-mode-content">
				<div id="logviewer_browse_pane" class="lst-tab-view lst-logviewer-mode-pane<?php if ($mode === 'browse') { echo ' is-active'; } ?>" aria-hidden="<?php echo ($mode === 'browse') ? 'false' : 'true'; ?>">
					<div class="lst-logviewer-searchrow lst-logviewer-searchrow--browse">
						<div class="lst-logviewer-field lst-logviewer-field--level">
							<label class="lst-logviewer-fieldlabel" for="browse_sellevel"><?php DMsg::EchoUIStr('service_displevel')?></label>
							<select class="lst-choice-control" id="browse_sellevel" name="browse_sellevel">
								<option value="1"<?php echo ($browseLevel == 1) ? ' selected' : ''; ?>>ERROR</option>
								<option value="2"<?php echo ($browseLevel == 2) ? ' selected' : ''; ?>>WARNING</option>
								<option value="3"<?php echo ($browseLevel == 3) ? ' selected' : ''; ?>>NOTICE</option>
								<option value="4"<?php echo ($browseLevel == 4) ? ' selected' : ''; ?>>INFO</option>
								<option value="5"<?php echo ($browseLevel == 5) ? ' selected' : ''; ?>>DEBUG</option>
							</select>
						</div>
						<div class="lst-logviewer-field lst-logviewer-field--offset">
							<label class="lst-logviewer-fieldlabel" for="browse_startpos"><?php DMsg::EchoUIStr('service_searchfrom')?> (KB)</label>
							<input name="browse_startpos" id="browse_startpos" class="lst-choice-control" type="text" value="<?php echo UI::EscapeAttr($currentStartPos); ?>">
						</div>
						<div class="lst-logviewer-field lst-logviewer-field--length">
							<label class="lst-logviewer-fieldlabel" for="browse_blksize"><?php DMsg::EchoUIStr('service_length')?> (KB)</label>
							<input name="browse_blksize" id="browse_blksize" class="lst-choice-control" type="text" inputmode="decimal" value="<?php echo UI::EscapeAttr($currentBlockSize); ?>" max="<?php echo UI::EscapeAttr($browseMaxBlockSize); ?>">
						</div>
					</div>
					<div class="lst-logviewer-navrow">
						<div class="lst-logviewer-navgroup">
							<div class="lst-button-group lst-logviewer-navcluster" role="group" aria-label="<?php echo UI::Escape(DMsg::UIStr('menu_logviewer')); ?>">
								<button type="button" class="lst-btn lst-btn--secondary lst-btn--sm lst-logviewer-navbtn lst-logviewer-navbtn--jump" data-log-nav="begin" data-lst-call="refreshLog" data-lst-call-arg="begin" rel="tooltip" data-original-title="<?php echo UI::EscapeAttr(DMsg::UIStr('btn_first')); ?>"><i class="lst-icon" data-lucide="chevrons-left"></i><span class="lst-logviewer-navlabel lst-logviewer-navlabel--compact"><?php DMsg::EchoUIStr('btn_first'); ?></span></button>
								<button type="button" class="lst-btn lst-btn--secondary lst-btn--sm lst-logviewer-navbtn" data-log-nav="prev" data-lst-call="refreshLog" data-lst-call-arg="prev"><i class="lst-icon" data-lucide="chevron-left"></i><span class="lst-logviewer-navlabel"><?php DMsg::EchoUIStr('btn_prev'); ?></span></button>
								<button type="button" class="lst-btn lst-btn--secondary lst-btn--sm lst-logviewer-navbtn" data-log-nav="next" data-lst-call="refreshLog" data-lst-call-arg="next"><span class="lst-logviewer-navlabel"><?php DMsg::EchoUIStr('btn_next'); ?></span><i class="lst-icon" data-lucide="chevron-right"></i></button>
								<button type="button" class="lst-btn lst-btn--secondary lst-btn--sm lst-logviewer-navbtn lst-logviewer-navbtn--jump" data-log-nav="end" data-lst-call="refreshLog" data-lst-call-arg="end" rel="tooltip" data-original-title="<?php echo UI::EscapeAttr(DMsg::UIStr('btn_last')); ?>"><span class="lst-logviewer-navlabel lst-logviewer-navlabel--compact"><?php DMsg::EchoUIStr('btn_last'); ?></span><i class="lst-icon" data-lucide="chevrons-right"></i></button>
							</div>
							<button type="button" id="refreshlog_btn" class="lst-btn lst-btn--secondary lst-btn--sm lst-logviewer-navbtn lst-logviewer-navbtn--refresh" data-log-nav="refresh" data-lst-call="refreshLog" data-lst-call-arg=""><span id="refreshlog_icon"><i class="lst-icon" data-lucide="refresh-cw"></i></span><span class="lst-logviewer-navlabel"><?php DMsg::EchoUIStr('btn_refresh'); ?></span></button>
						</div>
					</div>
					<div class="lst-logviewer-range-row">
						<div class="lst-logviewer-range" aria-live="polite"></div>
					</div>
				</div>
				<div id="logviewer_search_pane" class="lst-tab-view lst-logviewer-mode-pane<?php if ($mode === 'search') { echo ' is-active'; } ?>" aria-hidden="<?php echo ($mode === 'search') ? 'false' : 'true'; ?>">
					<div class="lst-logviewer-searchrow lst-logviewer-searchrow--search">
						<div class="lst-logviewer-field lst-logviewer-field--level">
							<label class="lst-logviewer-fieldlabel" for="search_sellevel"><?php DMsg::EchoUIStr('service_displevel')?></label>
							<select class="lst-choice-control" id="search_sellevel" name="search_sellevel">
								<option value="1"<?php echo ($searchLevel == 1) ? ' selected' : ''; ?>>ERROR</option>
								<option value="2"<?php echo ($searchLevel == 2) ? ' selected' : ''; ?>>WARNING</option>
								<option value="3"<?php echo ($searchLevel == 3) ? ' selected' : ''; ?>>NOTICE</option>
								<option value="4"<?php echo ($searchLevel == 4) ? ' selected' : ''; ?>>INFO</option>
								<option value="5"<?php echo ($searchLevel == 5) ? ' selected' : ''; ?>>DEBUG</option>
							</select>
						</div>
						<div class="lst-logviewer-field lst-logviewer-field--search">
							<label class="lst-logviewer-fieldlabel" for="searchterm"><?php DMsg::EchoUIStr('service_searchcurrentlog')?></label>
							<input name="searchterm" id="searchterm" class="lst-choice-control" type="text" autocomplete="off" maxlength="<?php echo (int) \LSWebAdmin\Log\LogViewer::SEARCH_MAX_LENGTH; ?>" placeholder="<?php echo UI::EscapeAttr(DMsg::UIStr('service_logsearchplaceholder')); ?>">
						</div>
						<div class="lst-logviewer-searchactions">
							<button type="button" id="searchlog_btn" class="lst-btn lst-btn--primary lst-btn--sm" data-lst-call="searchLog"><?php DMsg::EchoUIStr('btn_search')?></button>
							<button type="button" id="clearlogsearch_btn" class="lst-btn lst-btn--secondary lst-btn--sm" data-lst-call="clearLogSearch"><?php DMsg::EchoUIStr('btn_clear')?></button>
						</div>
					</div>
					<p class="lst-logviewer-mode-note"><?php DMsg::EchoUIStr('service_logsearchbackendnote'); ?></p>
				</div>

					<div class="lst-logviewer-grid">
						<div class="lst-logviewer-pane-toolbar lst-table-toolbar">
							<div class="lst-logviewer-summary lst-table-toolbar__summary" aria-live="polite"></div>
							<div class="lst-logviewer-search-slot lst-table-toolbar__search"></div>
							<div class="lst-logviewer-meta-note lst-table-toolbar__meta" aria-live="polite"></div>
							<div class="lst-logviewer-rows-slot lst-table-toolbar__rows"></div>
						</div>
						<p id="dash_logfoundmesg" class="lst-logviewer-statusnote" aria-live="polite" hidden></p>
						<table id="dash_logtbl" class="table lst-data-grid-table lst-data-grid-table--interactive lst-log-data-grid" width="100%">
							<thead><tr><th width="150"><?php DMsg::EchoUIStr('service_time')?></th><th width="60"><?php DMsg::EchoUIStr('service_level')?></th><th><?php DMsg::EchoUIStr('service_mesg')?></th></tr></thead>
							<tbody id="dash_logbody" class="font-lstlog"></tbody>
						</table>
					</div>
				</div>
			</div>

			<input id="filename" name="filename" type="hidden" value="<?php echo UI::EscapeAttr($currentFile); ?>">
			<input id="act" name="act" type="hidden">
			</div>
	</form>
	</div>
</article>
</section>

<script type="text/javascript">
	/* DO NOT REMOVE : GLOBAL FUNCTIONS!
	 *
	 * pageSetUp() is needed whenever you load a page.
	 * It initializes and checks for all basic elements of the page
	 * and makes rendering easier.
	 */

	pageSetUp();

        function syncLogViewerRibbon() {
            var ribbon = $("#ribbon"),
                    breadcrumb = ribbon.find(".lst-breadcrumb"),
                    hasNotice = ribbon.find("#restartnotice:visible, #readonlynotice:visible").length > 0;

            breadcrumb.empty();
            ribbon.toggleClass("lst-ribbon-empty", !hasNotice);
        }

	// PAGE RELATED SCRIPTS

        var logViewerText = {
            showing: <?php echo json_encode(DMsg::UIStr('note_showing')); ?>,
            rows: <?php echo json_encode(DMsg::UIStr('note_rows')); ?>,
            filter: <?php echo json_encode(DMsg::UIStr('note_filter')); ?>,
            browseFirst: <?php echo json_encode(DMsg::UIStr('service_logviewfirst')); ?>,
            browseFull: <?php echo json_encode(DMsg::UIStr('service_logviewfull')); ?>,
            browseLast: <?php echo json_encode(DMsg::UIStr('service_logviewlast')); ?>,
            browseRange: <?php echo json_encode(DMsg::UIStr('service_logviewrange')); ?>,
            enterSearch: <?php echo json_encode(DMsg::UIStr('service_logsearchenterterm')); ?>,
            invalidSearch: <?php echo json_encode(DMsg::UIStr('service_logsearchinvalidterm')); ?>,
            searchMinLength: <?php echo (int) \LSWebAdmin\Log\LogViewer::SEARCH_MIN_LENGTH; ?>,
            searchMaxLength: <?php echo (int) \LSWebAdmin\Log\LogViewer::SEARCH_MAX_LENGTH; ?>,
            searchSummary: <?php echo json_encode(DMsg::UIStr('service_logsearchsummary')); ?>,
            searchSummaryOrHigher: <?php echo json_encode(DMsg::UIStr('service_logsearchsummaryorhigher')); ?>,
            searchLimited: <?php echo json_encode(DMsg::UIStr('service_logsearchsummarylimited')); ?>,
            searchLimitedOrHigher: <?php echo json_encode(DMsg::UIStr('service_logsearchsummarylimitedorhigher')); ?>,
            searchEmpty: <?php echo json_encode(DMsg::UIStr('service_logsearchnomatch')); ?>,
            searchDefaultLevel: <?php echo (int) LogFilter::LEVEL_INFO; ?>,
            logCompact: <?php echo json_encode(DMsg::UIStr('service_logcompactnote')); ?>,
            logCompactOrHigher: <?php echo json_encode(DMsg::UIStr('service_logcompactnoteorhigher')); ?>,
            loadFailed: <?php echo json_encode(DMsg::Err('err_failreadfile')); ?>,
            downloadSelectionEmpty: <?php echo json_encode(DMsg::UIStr('btn_downloadcurrentselection') . ' unavailable'); ?>,
            logLevels: {
                1: 'ERROR',
                2: 'WARNING',
                3: 'NOTICE',
                4: 'INFO',
                5: 'DEBUG'
            }
        };

        function getLogViewerMode() {
            return $("#logform").data("lstLogMode") || "browse";
        }

        function setLogViewerMode(mode) {
            $("#logform").data("lstLogMode", mode === "search" ? "search" : "browse");
        }

        function isLogViewerSearchMode() {
            return getLogViewerMode() === "search";
        }

        function getStoredLogSearchTerm() {
            return lstTrimText($("#searchterm").val() || "");
        }

        function setStoredLogSearchTerm(term) {
            $("#searchterm").val(term || "");
        }

        function setDisplayedLogSearchLevel(level) {
            $("#search_sellevel").val(level || String(logViewerText.searchDefaultLevel));
        }

        function getDisplayedLogSearchLevel() {
            return $("#search_sellevel").val() || "";
        }

        function getBrowseLogLevel() {
            return $("#browse_sellevel").val() || "";
        }

        function getBrowseStartPos() {
            return $("#browse_startpos").val() || "";
        }

        function getBrowseBlockSize() {
            return normalizeLogViewerBrowseBlockSize($("#browse_blksize").val());
        }

        function getLogViewerResponseCache() {
            var cache = $("#logform").data("lstResponseCache");

            if (!cache) {
                cache = {
                    browse: null,
                    search: null
                };
                $("#logform").data("lstResponseCache", cache);
            }

            return cache;
        }

        function getCachedLogViewerResponse(mode) {
            var cache = getLogViewerResponseCache();

            return cache[mode] || null;
        }

        function setCachedLogViewerResponse(mode, log) {
            var cache = getLogViewerResponseCache();

            cache[mode] = log || null;
        }

        function clearCachedLogViewerResponse(mode) {
            setCachedLogViewerResponse(mode, null);
        }

        function updateLogViewerHistory(mode) {
            var $tab = $('.lst-logviewer-mode-tabs a[data-lst-log-mode-target="' + mode + '"]'),
                    historyUrl = $tab.attr("data-lst-history-url") || "";

            if (!historyUrl || !window.history || typeof window.history.replaceState !== "function") {
                return;
            }

            window.history.replaceState(null, document.title, historyUrl);
        }

        function getLogViewerBrowseDefaultKb() {
            var value = Number($("#logform").data("lstBrowseDefaultKb"));

            if (!isFinite(value) || value <= 0) {
                return 20;
            }

            return value;
        }

        function getLogViewerBrowseMaxKb() {
            var value = Number($("#logform").data("lstBrowseMaxKb"));

            if (!isFinite(value) || value <= 0) {
                return getLogViewerBrowseDefaultKb();
            }

            return value;
        }

        function normalizeLogViewerBrowseBlockSize(value) {
            var parsed = Number(value),
                    max = getLogViewerBrowseMaxKb();

            if (!isFinite(parsed) || parsed <= 0) {
                return getLogViewerBrowseDefaultKb();
            }

            return Math.min(parsed, max);
        }

        function setLogViewerHasSelection(hasSelection) {
            $("#logform").data("lstHasSelection", hasSelection ? 1 : 0);
        }

        function hasLogViewerSelection() {
            return !!$("#logform").data("lstHasSelection");
        }

        function setLogViewerBrowseState(state) {
            $("#logform").data("lstBrowseState", state || null);
        }

        function getLogViewerBrowseState() {
            return $("#logform").data("lstBrowseState") || null;
        }

        function setLogViewerNavDisabled(action, disabled) {
            var button = $('.lst-logviewer-navbtn[data-log-nav="' + action + '"]');

            if (!button.length) {
                return;
            }

            button.toggleClass("disabled", !!disabled)
                    .attr("aria-disabled", disabled ? "true" : null)
                    .attr("tabindex", disabled ? "-1" : null);
        }

        function formatLogViewerRangeValue(value) {
            var rounded;

            if (!isFinite(value)) {
                value = 0;
            }

            value = Math.max(0, Number(value) || 0);
            rounded = Math.round(value * 10) / 10;

            if (Math.abs(rounded - Math.round(rounded)) < 0.005) {
                return Math.round(rounded).toLocaleString();
            }

            return rounded.toLocaleString(undefined, {
                minimumFractionDigits: 1,
                maximumFractionDigits: 1
            });
        }

        function syncLogViewerUiState() {
            var browseState = getLogViewerBrowseState(),
                    range = $(".lst-logviewer-range"),
                    clearButton = $("#clearlogsearch_btn"),
                    searchButton = $("#searchlog_btn"),
                    clearDisabled = false,
                    searchTerm = lstTrimText($("#searchterm").val()),
                    searchValidationError = validateLogSearchTerm(searchTerm),
                    searchDisabled = searchValidationError !== "";

            if (clearButton.length) {
                clearDisabled = searchTerm === ""
                        && getStoredLogSearchTerm() === ""
                        && !hasLogViewerSelection();

                clearButton
                        .toggleClass("disabled", clearDisabled)
                        .attr("aria-disabled", clearDisabled ? "true" : null)
                        .prop("disabled", clearDisabled);
            }

            if (searchButton.length) {
                searchButton
                        .toggleClass("disabled", searchDisabled)
                        .attr("aria-disabled", searchDisabled ? "true" : null)
                        .prop("disabled", searchDisabled);
            }

            if (isLogViewerSearchMode()) {
                if (range.length) {
                    range.text("").prop("hidden", true);
                }
                return;
            }

            setLogViewerNavDisabled("begin", !browseState || browseState.atStart);
            setLogViewerNavDisabled("prev", !browseState || browseState.atStart);
            setLogViewerNavDisabled("refresh", false);
            setLogViewerNavDisabled("next", !browseState || browseState.atEnd);
            setLogViewerNavDisabled("end", !browseState || browseState.atEnd);

            if (!range.length) {
                return;
            }

            if (browseState) {
                range.text(formatLogViewerRangeMessage(browseState)).prop("hidden", false);
            } else {
                range.text("").prop("hidden", true);
            }
        }

        function formatLogViewerRangeMessage(browseState) {
            var start = Math.max(0, Number(browseState.startKb) || 0),
                    end = Math.max(start, Number(browseState.endKb) || 0),
                    total = Math.max(end, Number(browseState.totalKb) || 0),
                    shown = Math.max(0, end - start),
                    epsilon = 0.05;

            if (total <= epsilon || shown >= (total - epsilon)) {
                return logViewerText.browseFull
                        .replace("%%total%%", formatLogViewerRangeValue(total));
            }

            if (start <= epsilon) {
                return logViewerText.browseFirst
                        .replace("%%size%%", formatLogViewerRangeValue(shown))
                        .replace("%%total%%", formatLogViewerRangeValue(total));
            }

            if (Math.abs(end - total) <= epsilon) {
                return logViewerText.browseLast
                        .replace("%%size%%", formatLogViewerRangeValue(shown))
                        .replace("%%total%%", formatLogViewerRangeValue(total));
            }

            return logViewerText.browseRange
                    .replace("%%start%%", formatLogViewerRangeValue(start))
                    .replace("%%end%%", formatLogViewerRangeValue(end))
                    .replace("%%total%%", formatLogViewerRangeValue(total));
        }

        function validateLogSearchTerm(term) {
            if (term.length < logViewerText.searchMinLength || term.length > logViewerText.searchMaxLength) {
                return "length";
            }

            if (!/^[\x20-\x7E]+$/.test(term)) {
                return "invalid";
            }

            if (/[;&|`${}]/.test(term)) {
                return "invalid";
            }

            if (/\\[xXuU][0-9a-fA-F]{2}|%[0-9a-fA-F]{2}|[A-Za-z0-9+/=]{40,}/i.test(term)) {
                return "invalid";
            }

            return "";
        }

        function buildLogDownloadUrl(scope) {
            var filename = $("#cur_log_file").text(),
                    searchTerm = isLogViewerSearchMode() ? getStoredLogSearchTerm() : "",
                    browseBlockSize = getBrowseBlockSize(),
                    selectedLevel = isLogViewerSearchMode() ? getDisplayedLogSearchLevel() : getBrowseLogLevel(),
                    query = [];

            if (!filename) {
                return "#";
            }

            query.push("id=downloadlog");
            query.push("scope=" + encodeURIComponent(scope));
            query.push("filename=" + encodeURIComponent(filename));
            query.push("sellevel=" + encodeURIComponent(selectedLevel));
            query.push("startpos=" + encodeURIComponent(getBrowseStartPos()));
            query.push("blksize=" + encodeURIComponent(browseBlockSize));
            if (searchTerm !== "") {
                query.push("searchterm=" + encodeURIComponent(searchTerm));
            }

            return "view/ajax_data.php?" + query.join("&");
        }

        function sanitizeLogDownloadFilename(filename) {
            var safeName = String(filename || "server_log.txt").replace(/[\r\n]+/g, "").replace(/[^A-Za-z0-9._-]+/g, "_");

            if (!safeName || safeName === "." || safeName === "..") {
                return "server_log.txt";
            }

            return safeName;
        }

        function buildLogCurrentSelectionFilename() {
            var filename = $("#cur_log_file").text() || "server.log",
                    baseName = filename.split(/[\\/]/).pop() || "server.log",
                    parts = baseName.split("."),
                    extension = parts.length > 1 ? parts.pop() : "txt",
                    stem = parts.length ? parts.join(".") : baseName,
                    timestamp = new Date().toISOString().replace(/[\-:]/g, "").replace(/\.\d+Z$/, "").replace("T", "_");

            if (!stem) {
                stem = "server_log";
            }

            return sanitizeLogDownloadFilename(stem + "_selection_" + timestamp + "." + (extension || "txt"));
        }

        function buildLogCurrentSelectionText(rows) {
            return $.map(rows || [], function (row) {
                var time = (row && row.time ? String(row.time) : "").replace(/\t/g, " "),
                        level = (row && row.level ? String(row.level) : "").replace(/\t/g, " "),
                        message = (row && row.message ? String(row.message) : "").replace(/\r\n/g, "\n").replace(/\r/g, "\n");

                return time + "\t" + level + "\t" + message;
            }).join("\n") + ((rows && rows.length) ? "\n" : "");
        }

        function downloadLogCurrentSelection() {
            var rows = $("#logform").data("lstDownloadRows") || [],
                    text,
                    blob,
                    url,
                    link;

            if (!rows.length) {
                setLogViewerStatus(logViewerText.downloadSelectionEmpty, false);
                syncLogViewerUiState();
                return false;
            }

            text = buildLogCurrentSelectionText(rows);
            blob = new Blob([text], {type: "text/plain;charset=utf-8"});
            url = window.URL.createObjectURL(blob);
            link = document.createElement("a");
            link.href = url;
            link.download = buildLogCurrentSelectionFilename();
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
            window.setTimeout(function () {
                window.URL.revokeObjectURL(url);
            }, 0);

            return false;
        }

        function syncLogDownloadLinks() {
            var filename = $("#cur_log_file").text(),
                    hasFile = !!filename,
                    hasSelection = hasLogViewerSelection(),
                    $toggle = $("#downloadlog_toggle"),
                    $current = $("#downloadlog_current"),
                    $full = $("#downloadlog_full"),
                    $links = $current.add($full);

            if (!hasFile) {
                $toggle.addClass("disabled").attr("aria-disabled", "true");
                $links.attr("href", "#").addClass("disabled").attr("aria-disabled", "true").attr("tabindex", "-1");
                return;
            }

            $toggle.removeClass("disabled").removeAttr("aria-disabled");
            $full
                    .attr("href", buildLogDownloadUrl("full"))
                    .removeClass("disabled")
                    .removeAttr("aria-disabled")
                    .removeAttr("tabindex");

            $current.attr("href", buildLogDownloadUrl("current"));
            if (hasSelection) {
                $current.removeClass("disabled")
                        .removeAttr("aria-disabled")
                        .removeAttr("tabindex");
            } else {
                $current.addClass("disabled")
                        .attr("aria-disabled", "true")
                        .attr("tabindex", "-1");
            }
        }

        function formatLogViewerCount(value) {
            var count = parseInt(value, 10);

            if (isNaN(count) || count < 0) {
                count = 0;
            }

            return count.toLocaleString();
        }

        function formatLogViewerSummary(api) {
            var info = api.page.info(),
                    visible,
                    total;

            if (!info) {
                return logViewerText.showing + " 0 / 0";
            }

            visible = Math.max(info.end - info.start, 0);
            total = Math.max(parseInt(info.recordsTotal, 10) || 0, 0);

            return logViewerText.showing + " " + visible + " / " + total;
        }

        function formatLogViewerMeta(log) {
            var levelText = logViewerText.logLevels[parseInt(log.loglevel, 10)] || logViewerText.logLevels[3],
                    isExactLevel = parseInt(log.loglevel, 10) === 1,
                    searchTotal,
                    displayed,
                    template;

            if ((log.mode || "browse") === "search") {
                searchTotal = Math.max(parseInt(log.search_total, 10) || 0, 0);
                displayed = Math.max(parseInt(log.search_displayed, 10) || 0, 0);

                if (searchTotal === 0) {
                    return logViewerText.searchEmpty
                            .replace("%%term%%", log.searchterm || "");
                }

                if (parseInt(log.search_limited, 10)) {
                    template = isExactLevel ? logViewerText.searchLimited : logViewerText.searchLimitedOrHigher;
                    return template
                            .replace("%%matches%%", formatLogViewerCount(searchTotal))
                            .replace("%%shown%%", formatLogViewerCount(displayed))
                            .replace("%%term%%", log.searchterm || "")
                            .replace("%%level%%", levelText);
                }

                template = isExactLevel ? logViewerText.searchSummary : logViewerText.searchSummaryOrHigher;
                return template
                        .replace("%%matches%%", formatLogViewerCount(searchTotal))
                        .replace("%%term%%", log.searchterm || "")
                        .replace("%%level%%", levelText);
            }

            template = isExactLevel ? logViewerText.logCompact : logViewerText.logCompactOrHigher;
            return template
                    .replace("%%outlines%%", formatLogViewerCount(log.logfound))
                    .replace("%%totallines%%", formatLogViewerCount(log.logsearched))
                    .replace("%%level%%", levelText);
        }

        function syncLogViewerControls() {
            var searchSlot = $(".lst-logviewer-search-slot"),
                    rowsSlot = $(".lst-logviewer-rows-slot"),
                    summarySlot = $(".lst-logviewer-summary"),
                    metaSlot = $(".lst-logviewer-meta-note"),
                    wrapper = $("#dash_logtbl").closest(".dt-container"),
                    filter = wrapper.find(".dt-search").first(),
                    length = wrapper.find(".dt-length").first();

            if (filter.length && filter.parent()[0] !== searchSlot[0]) {
                searchSlot.append(filter);
            }

            if (length.length && length.parent()[0] !== rowsSlot[0]) {
                rowsSlot.append(length);
            }

            if ($.fn.dataTable.isDataTable("#dash_logtbl")) {
                summarySlot.text(formatLogViewerSummary($("#dash_logtbl").DataTable()));
            }
            else {
                summarySlot.text(logViewerText.showing + " 0 / 0");
            }

            metaSlot.text($("#logform").data("lstLogMeta") || "");
        }

        function setLogViewerStatus(message, isError) {
            var status = $("#dash_logfoundmesg");

            if (!message) {
                status.prop("hidden", true)
                        .removeClass("is-error")
                        .text("");
                return;
            }

            status.text(message)
                    .prop("hidden", false)
                    .toggleClass("is-error", !!isError);
        }

        function getLogViewerTableOptions() {
            return {
                autoWidth: false,
                ordering: true,
                order: [[0, "asc"]],
                lengthMenu: [[30, 50, 100, 200], [30, 50, 100, 200]],
                language: lstDataTableLanguage(logViewerText)
            };
        }

        function updateLogViewerTableRows(rowMarkup) {
            var api = lstGetDataTableApi("#dash_logtbl"),
                    rows = $("<tbody>").html(rowMarkup || "").children("tr");

            if (!api) {
                $("#dash_logbody").html(rowMarkup || "");
                return;
            }

            api.clear();
            if (rows.length) {
                api.rows.add(rows.toArray());
            }

            api.search("");
            api.order([[0, "asc"]]);
            api.page(0).draw(false);
        }

        function ensureLogViewerTable() {
            if (!$.fn.dataTable.isDataTable("#dash_logtbl")) {
                $("#dash_logtbl").DataTable(getLogViewerTableOptions());
                lstTuneDataTableChrome("#dash_logtbl", {
                    placeholder: logViewerText.filter,
                    lengthLabel: logViewerText.rows,
                    bindDraw: true,
                    onUpdate: syncLogViewerControls
                });
            }

            syncLogViewerControls();
        }

        function resetLogViewerResults() {
            updateLogViewerTableRows("");
            setLogViewerHasSelection(false);
            setLogViewerBrowseState(null);
            $("#logform").data("lstDownloadRows", []);
            $("#logform").data("lstLogMeta", "");
            setLogViewerStatus("");
            ensureLogViewerTable();
            syncLogDownloadLinks();
            syncLogViewerUiState();
        }

        function applyLogViewerModeInputs(log) {
            if ((log.mode || "browse") === "search") {
                $("#searchterm").val(log.searchterm || "");
                setDisplayedLogSearchLevel(log.sellevel);
            } else {
                $("#browse_sellevel").val(log.sellevel || "");
                $("#browse_startpos").val(log.startpos || "");
                $("#browse_blksize").val(normalizeLogViewerBrowseBlockSize(log.blksize));
            }
        }

        function renderLogViewerState(log) {
            var logform = $("#logform"),
                    mode = (log && log.mode) ? log.mode : getLogViewerMode();

            setLogViewerMode(mode);
            $("#cur_log_file").text(log && log.cur_log_file ? log.cur_log_file : "");
            $("#cur_log_size").text(log && log.cur_log_size ? log.cur_log_size : "0.00");
            logform.find("#filename").val(log && log.cur_log_file ? log.cur_log_file : "");
            updateLogViewerTableRows(log && log.log_body ? log.log_body : "");
            logform.data("lstDownloadRows", log && Array.isArray(log.download_rows) ? log.download_rows : []);

            if (log) {
                applyLogViewerModeInputs(log);
            }

            if (log && mode === "browse") {
                setLogViewerBrowseState({
                    startKb: Number(log.browse_start_kb) || 0,
                    endKb: Number(log.browse_end_kb) || 0,
                    totalKb: Number(log.browse_total_kb) || 0,
                    atStart: !!log.browse_at_start,
                    atEnd: !!log.browse_at_end
                });
            } else {
                setLogViewerBrowseState(null);
            }

            setLogViewerHasSelection(!!(log && (parseInt(log.logfound, 10) || 0) > 0));
            logform.data("lstLogMeta", log ? formatLogViewerMeta(log) : "");

            ensureLogViewerTable();
            syncLogDownloadLinks();
            syncLogViewerUiState();
            lst_refreshFooterTime();
        }

        function showCachedLogViewerMode(mode) {
            var cached = getCachedLogViewerResponse(mode);

            setLogViewerMode(mode);
            updateLogViewerHistory(mode);

            if (mode === "search") {
                setDisplayedLogSearchLevel(getDisplayedLogSearchLevel() || "");
            }

            if (cached) {
                renderLogViewerState(cached);
            } else if (mode === "browse") {
                $("#browse_blksize").val(getBrowseBlockSize());
                loadLogViewer("browse", "");
            } else {
                resetLogViewerResults();
                if (mode === "search") {
                    setStoredLogSearchTerm("");
                    setDisplayedLogSearchLevel("");
                }
            }
        }

        function applyLogViewerResponse(log) {
            var mode = (log && log.mode) ? log.mode : getLogViewerMode();

            setCachedLogViewerResponse(mode, log);
            updateLogViewerHistory(mode);
            renderLogViewerState(log);
        }

        function loadLogViewer(mode, act) {
            var spinicon = $("#refreshlog_icon"),
                    logform = $("#logform"),
                    searchButton = $("#searchlog_btn"),
                    requestData = {
                        filename: $("#cur_log_file").text() || logform.find("#filename").val() || "",
                        act: mode === "search" ? "" : (act || "")
                    };

            if (mode === "search") {
                requestData.searchterm = getStoredLogSearchTerm();
                requestData.sellevel = getDisplayedLogSearchLevel();
            } else {
                requestData.sellevel = getBrowseLogLevel();
                requestData.startpos = getBrowseStartPos();
                requestData.blksize = getBrowseBlockSize();
                $("#browse_blksize").val(requestData.blksize);
            }

            $.ajax({
                url: "view/ajax_data.php?id=" + (mode === "search" ? "searchlog" : "viewlog"),
                data: requestData,
                dataType: "json",
                beforeSend: function () {
                    setLogViewerStatus("");

                    if (mode === "search" && searchButton.length) {
                        searchButton.prop("disabled", true);
                    } else if (spinicon.length) {
                        spinicon.html('<i class="lst-icon lst-spin" data-lucide="loader-2"></i>');
                    }
                }
            })
            .done(function (log) {
                applyLogViewerResponse(log);
                if (mode === "search" && searchButton.length) {
                    searchButton.prop("disabled", false);
                } else if (spinicon.length) {
                    spinicon.html('<i class="lst-icon" data-lucide="refresh-cw"></i>');
                }
            })
            .fail(function () {
                if (mode === "search" && searchButton.length) {
                    searchButton.prop("disabled", false);
                } else if (spinicon.length) {
                    spinicon.html('<i class="lst-icon" data-lucide="refresh-cw"></i>');
                }
                setLogViewerStatus(logViewerText.loadFailed, true);
                syncLogViewerUiState();
                syncLogDownloadLinks();
            });
        }

        syncLogViewerRibbon();

        function refreshLog(act) {
            if (isLogViewerSearchMode()) {
                return false;
            }

            $("#browse_blksize").val(getBrowseBlockSize());

            loadLogViewer("browse", act || "");
            return false;
        }

        function searchLog() {
            var term = lstTrimText($("#searchterm").val()),
                    validationError = validateLogSearchTerm(term);

            if (validationError !== "") {
                setLogViewerStatus(validationError === "length" ? logViewerText.enterSearch : logViewerText.invalidSearch, false);
                syncLogViewerUiState();
                return false;
            }

            setStoredLogSearchTerm(term);
            loadLogViewer("search", "");
            return false;
        }

        function clearLogSearch() {
            $("#searchterm").val("");
            setDisplayedLogSearchLevel("");
            clearCachedLogViewerResponse("search");
            if (isLogViewerSearchMode()) {
                resetLogViewerResults();
            }
            return false;
        }
	// pagefunction


	var pagefunction = function() {
        $(".lst-logviewer-mode-tabs").on("shown.lstTab.logviewer", 'a[data-lst-log-mode-target]', function () {
            var mode = $(this).attr("data-lst-log-mode-target") || "browse";

            showCachedLogViewerMode(mode);
        });

        $("#downloadlog_toggle").on("click", function (event) {
            if ($(this).attr("aria-disabled") === "true") {
                event.preventDefault();
                event.stopPropagation();
            }
        });

        $("#downloadlog_current, #downloadlog_full").on("click", function (event) {
            if ($(this).hasClass("disabled") || $(this).attr("aria-disabled") === "true") {
                event.preventDefault();
                return;
            }

            if (this.id === "downloadlog_current") {
                event.preventDefault();
                downloadLogCurrentSelection();
            }
        });

        $("#browse_blksize").on("change blur", function () {
            $(this).val(normalizeLogViewerBrowseBlockSize($(this).val()));
        });

        $("#searchterm").on("input", function () {
            syncLogViewerUiState();
        });

        $("#searchterm").on("keydown", function (event) {
            if (event.key === "Enter") {
                event.preventDefault();
                searchLog();
            }
        });

        setLogViewerMode("<?php echo UI::Escape($mode); ?>");
        setDisplayedLogSearchLevel($("#search_sellevel").val() || "");
        setLogViewerBrowseState(null);
        setLogViewerHasSelection(false);
        syncLogDownloadLinks();
        syncLogViewerUiState();

        if (isLogViewerSearchMode()) {
            showCachedLogViewerMode("search");
        } else {
            refreshLog('');
        }
	};

	// end pagefunction

	// load related plugins

	lstLoadDataTableAssets(pagefunction);


</script>
