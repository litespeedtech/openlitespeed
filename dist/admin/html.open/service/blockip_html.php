<?php
require_once('../includes/auth.php');

$stats = new STATS();
$stats->parse_all();
$blocked_count = count($stats->blocked_ip);

echo GUI::header();

?>
<table class="xtbl" width="100%" border="0" cellpadding="4" cellspacing="1">
<tr height="15"><td class="xtbl_title">Current Blocked IP List (Total <? echo $blocked_count; ?>)</tr>

<tr class="xtbl_value"><td>

<?php
	echo join(', ', $stats->blocked_ip);
?>			

</td></tr>
</table>

<?php

echo GUI::footer();

?>