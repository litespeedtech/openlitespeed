<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/view/inc/auth.php';

$view = isset($_GET['view']) && is_scalar($_GET['view']) ? (string) $_GET['view'] : 'dashboard';
$viewRoutes = [
    'dashboard' => __DIR__ . '/view/dashboard.php',
    'confMgr' => __DIR__ . '/view/confMgr.php',
    'compilePHP' => __DIR__ . '/view/compilePHP.php',
    'blockedips' => __DIR__ . '/view/blockedips.php',
    'listenervhmap' => __DIR__ . '/view/listenervhmap.php',
    'loginhistory' => __DIR__ . '/view/loginhistory.php',
    'opsauditlog' => __DIR__ . '/view/opsauditlog.php',
    'logviewer' => __DIR__ . '/view/logviewer.php',
    'realtimestats' => __DIR__ . '/view/realtimestats.php',
];

if (!isset($viewRoutes[$view]) || !is_file($viewRoutes[$view])) {
    $view = 'dashboard';
}

$currentViewFile = $viewRoutes[$view];

if ($view === 'confMgr') {
    Service::ConfigDispData();
}

//require UI configuration (nav, ribbon, etc.)
require_once __DIR__ . '/view/inc/configui.php';

require_once __DIR__ . '/view/inc/header.php';
require_once __DIR__ . '/view/inc/nav.php';

?>
<div id="main" role="main">

	<div id="ribbon">
		<span id="restartnotice" class="lst-ribbon-notice lst-ribbon-notice--warning" hidden><i class="lst-icon" data-lucide="bell"></i> <span class="lst-ribbon-notice-text"><?php DMsg::EchoUIStr('note_configmodified'); ?></span> <a class="lst-ribbon-notice-action" href="#" data-lst-call="lst_restart"><?php echo UIBase::Escape(DMsg::UIStr('menu_restart')); ?></a></span>
        <span id="readonlynotice" class="lst-ribbon-notice lst-ribbon-notice--warning" hidden></span>
	</div>

	<div id="content">
		<?php
		require_once __DIR__ . '/view/inc/scripts.php';
		require $currentViewFile;
		?>
	</div>

</div>


<!--
<?php
echo $footer_lic_info;
?>
-->

<div class="page-footer">
	<div class="lst-footer-bar">
		<div class="lst-footer-copywrap">
			<span class="lst-text-muted lst-footer-copy">OpenLiteSpeed WebAdmin Console © <?php echo date('Y') . ' '; DMsg::EchoUIStr('note_copyrightreserved'); ?></span>
		</div>
		<div class="lst-footer-meta lst-hide-phone">
			<span class="lst-text-muted lst-footer-stamp"><i class="lst-icon" data-lucide="clock"></i><?php DMsg::EchoUIStr('note_dataretrievedat'); ?> <span id="lst_UpdateStamp"></span></span>
		</div>
	</div>
</div>

	</body>
</html>
