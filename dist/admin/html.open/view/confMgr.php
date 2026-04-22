<?php

use LSWebAdmin\I18n\DMsg;
use LSWebAdmin\Product\Current\DPageDef;
use LSWebAdmin\Product\Current\Service;
use LSWebAdmin\Product\Current\UI;
use LSWebAdmin\UI\ConfPageResolver;
use LSWebAdmin\UI\ConfUiContext;
use LSWebAdmin\UI\UIBase;

require_once __DIR__ . '/inc/auth.php';

$disp = Service::ConfigDispData();
$uiContext = ConfUiContext::fromDisplay($disp);

$page = ConfPageResolver::resolve($uiContext, DPageDef::class);
UI::PrintConfPage($uiContext, $page);

?>

<script type="text/javascript">

pageSetUp();

var pagefunction = function() {
	// clears memory even if nothing is in the function
   	var rospan = $("#readonlynotice");
    <?php
    if (defined('_CONF_READONLY_')) {
        $alert_str = '<i class=\"lst-icon\" data-lucide=\"bell\"></i> ' . DMsg::UIStr('note_readonly_mode');
    ?>
        rospan.html(<?php echo UIBase::EscapeJs($alert_str); ?>);
        if (rospan.prop("hidden"))
            rospan.prop("hidden", false);

    <?php } else { ?>
        rospan.html("");
        if (!rospan.prop("hidden"))
            rospan.prop("hidden", true);

<?php }

   if (Service::HasChanged()) { ?>
	var span = $("#restartnotice");
	if (span.prop("hidden"))
		span.prop("hidden", false);
<?php } ?>

    if (typeof syncRibbonState === 'function') {
        syncRibbonState();
    }
};

// end pagefunction

// run pagefunction on load
pagefunction();

</script>
