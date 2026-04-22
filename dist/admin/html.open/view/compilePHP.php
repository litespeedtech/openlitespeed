<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\Tool\BuildPhp\CompilePHPUI;

require_once __DIR__ . '/inc/auth.php';

echo UI::content_header('list', DMsg::ALbl('menu_tools'), DMsg::ALbl('menu_compilephp'));

echo '<section class="lst-build-page">';
$ui = new CompilePHPUI();
$ui->PrintPage();
echo '</section>';
?>


<script type="text/javascript">
/* DO NOT REMOVE : GLOBAL FUNCTIONS!

	 * pageSetUp() is needed whenever you load a page.
	 * It initializes and checks for all basic elements of the page
	 * and makes rendering easier.
	 *
	 */

	 pageSetUp();

	 // PAGE RELATED SCRIPTS


	function refreshStatus()
	{
		var statuszone = $("#statuszone"), logzone = $("#logzone");

		function scrollLogToBottom()
		{
			if (!logzone.length) {
				return;
			}

			logzone.scrollTop(logzone[0].scrollHeight);
		}

		function setBuildStatusIndicator(kind, iconName, message)
		{
			var statusGraphZone = $("#statusgraphzone");

			statusGraphZone.html(
				'<span class="lst-status-pill lst-build-status-pill lst-status-pill--' + kind + '">'
				+ '<i class="lst-icon" data-lucide="' + iconName + '"></i>'
				+ '<span>' + message + '</span>'
				+ '</span>'
			);

			if (typeof lstRenderLucide === 'function' && statusGraphZone.length) {
				lstRenderLucide(statusGraphZone[0]);
			}
		}

		$.ajax({
			url: "view/ajax_data.php?id=buildprogress",
			dataType: "text",
			async : true
		})
		.done(function(log) {
			var pos = log.indexOf("\n**LOG_DETAIL**");
			statuszone.text(log.substring(0, pos));
			logzone.text(log.substring(pos));
			scrollLogToBottom();
			lst_refreshFooterTime();

			if (log.indexOf("\n**DONE**") >= 0) {
				setBuildStatusIndicator('success', 'check', <?php echo json_encode(DMsg::UIStr('buildphp_finishsuccess')); ?>);
				if ($("#nextbtn").length) {
					$("#nextbtn").removeClass('disabled').prop('disabled', false);
					$("#prevbtn").removeClass('disabled').prop('disabled', false);
				}
         	}
         	else if (log.indexOf("\n**ERROR**") >= 0) {
                setBuildStatusIndicator('danger', 'triangle-alert', <?php echo json_encode(DMsg::UIStr('buildphp_stopduetoerr')); ?>);
				if ($("#prevbtn").length)
	        			$("#prevbtn").removeClass('disabled').prop('disabled', false);
				else
					setTimeout(refreshStatus, 15000);
         	}
        	else {
            	setTimeout(refreshStatus, 3000);
			}

		})
		.fail(function(jqXHR, textStatus) {
			statuszone.text('Status refresh error: ' + textStatus + ' ... please wait');
			scrollLogToBottom();
			setTimeout(refreshStatus, 5000);
		});
	}

	 // pagefunction

	var pagefunction = function() {
		if ($("#statuszone").length) {
			refreshStatus();
		}
	};

	pagefunction();

</script>
