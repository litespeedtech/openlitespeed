<?php

include_once('includes/auth.php');

$service = new Service();

echo GUI::header();
echo GUI::top_menu();
?>

<h2 class="bottom_bar">Home</h2>

<table width="100%" class="xtbl" border="0" cellpadding="10" cellspacing="1">
<tr class="xtbl_header"><td class="xtbl_title" colspan="2">Main Areas</td></tr>
<tr>
	<td width="120px" class="xtbl_label" align="center"><a href="/service/serviceMgr.php"><img src="/static/images/icons/controlpanel.gif"></a><div>General</div></td>
	<td class="xtbl_value">Perform server restart, check server status, view real-time statistics, and more.</td>
</tr>
<tr>
	<td width="120px" class="xtbl_label" align="center"><a href="/config/confMgr.php?m=serv"><img src="/static/images/icons/serverconfig.gif"></a><div>Configuration</div></td>
	<td class="xtbl_value">Add, modify, or delete server configuration settings.</td>
</tr>
<tr>
	<td width="120px" class="xtbl_label" align="center"><a href="/config/confMgr.php?m=admin"><img src="/static/images/icons/adminconfig.gif"></a><div>WebAdmin Console</div></td>
	<td class="xtbl_value">Manage web administration interface settings.</td>
</tr>
</table>

<?
$log = array();
$res = $service->showErrLog($log);

if ( $res !== 0  )
{
	$buf = '<table class="xtbl log_tbl" border="0" cellpadding="5" cellspacing="1">
<caption>Found ' . $res . ' warning/error messages in the log: <a href="/service/serviceMgr.php?vl=1" title="Go to log viewer">see more</a></caption>
<tr class="xtbl_label"><td class="col_time">Time</td><td class="col_level">Level</td><td class="col_mesg">Message</td></tr>
		';

	foreach($log as $key => $entry) {
		$buf .= $entry;
	}
	$buf .= '</table>';
	echo $buf;
}

echo GUI::footer();

?>
