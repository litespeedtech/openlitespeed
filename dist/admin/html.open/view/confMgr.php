<?php

require_once("inc/auth.php");

$disp = Service::ConfigDispData();

$page = DPageDef::GetPage($disp);
UI::PrintConfPage($disp, $page);

?>

<script type="text/javascript">

pageSetUp();

var pagefunction = function() {
	// clears memory even if nothing is in the function
   	var rospan = $("#readonlynotice");
    <?php
    if (defined('_CONF_READONLY_')) {
        $alert_str = '<i class=\"fa fa-bell\"></i> ' . DMsg::UIStr('note_readonly_mode');
    ?>
        rospan.html("<?php echo $alert_str;?>");
      	if (rospan.hasClass("hide"))
            rospan.removeClass("hide");

    <?php } else { ?>
        rospan.html("");
      	if (!rospan.hasClass("hide"))
            rospan.addClass("hide");

    <?php }

   if (Service::HasChanged()) { ?>
	var span = $("#restartnotice");
	if (span.hasClass("hide"))
		span.removeClass("hide");
<?php } ?>
};

// end pagefunction

// run pagefunction on load
pagefunction();

</script>
