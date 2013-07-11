<?php

include_once('includes/auth.php');


$service = new Service();


echo GUI::header();
echo GUI::top_menu();

?>


<h2>Home</h2>

<table width="100%" class=xtbl border="0" cellpadding="5" cellspacing="1">
<tr class=xtbl_header><td class=xtbl_title colspan=2>Main Areas</td></tr>
<tr>
	<td width=120 class="xtbl_label" valign=middle align=center>General<br><a href="/service/serviceMgr.php"><img src="/static/images/icons/controlpanel.gif"></a></td>
	<td valign=middle class="xtbl_value">Perform server restart, manage upgrades, check server status, view
	real-time statistics, and more.</td>
</tr>
<tr>
	<td width=120 class="xtbl_label" valign=middle align=center>Configuration<br><a href="/config/confMgr.php?m=serv"><img src="/static/images/icons/serverconfig.gif"></a></td>
	<td valign=middle class="xtbl_value">Add, modify, or delete server configuration settings.</td>
</tr>
<tr>
	<td width=120 class="xtbl_label" align=center>Web Console<br><a href="/config/confMgr.php?m=admin"><img src="/static/images/icons/adminconfig.gif"></a></td>
	<td valign=middle class="xtbl_value">Manage web administration interface settings.</td>
</tr>
</table>

<!-- <a href="javascript:showReport()"> -->



<?
$buf = array();
$res = $service->showErrLog($buf);



if ( $res !== 0  )
{
	echo '<table width="100%" class="xtbl" border="0" cellpadding="5" cellspacing="1">
	<tr class="xtbl_header">
		<td class="xtbl_title" colspan="2" nowrap>Found ' . $res . 'warning/error messages in the log:</td>
		<td class="xtbl_edit"><a href="/service/serviceMgr.php?vl=1">More</a></td>
	</tr>
	<tr class="xtbl_label">
		<td width="170">Time</td>
		<td width="50">Level</td>
		<td>Message</td>
	</tr>';
	
	foreach($buf as $key => $entry) {
		echo $entry;
	}
	
	echo '</table>';

}



echo GUI::footer();

?>
