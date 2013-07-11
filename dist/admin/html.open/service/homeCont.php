<?

$state = DUtil::getGoodVal(DUtil::grab_input("get","state"));

if($state == "restarted") {
	echo XUI::message("Server Restarted",ucwords(strtolower($product->product))." has been gracefully restarted.","success");
	$product->refreshVersion();
}

$product_info = ucwords(strtolower($product->product)) . ' - Version ' . (strtolower($product->version)) . '</a>';

?>

<h2><?echo $product_info;?></h2>


<table width="100%" class=xtbl border="0" cellpadding="5" cellspacing="1">
<tr class=xtbl_header><td class=xtbl_title colspan=3><?echo $service->serv['name'] . ' ( PID = ' . $service->serv['pid'] . ')';?></td></tr>
<tr>
	<td class="xtbl_label">Apply Changes / Graceful Restart</td>
	<td class="icon"><a href="javascript:go('restart','')"><img alt="Apply Changes / Graceful Restart" title="Apply Changes / Graceful Restart" src="/static/images/icons/refresh.gif"></a></td>
	<td class="xtbl_label">Server Log Viewer</td>
	<td class="icon"><a href="/service/serviceMgr.php?vl=1"><img alt="Server Log Viewer" title="Server Log Viewer" src="/static/images/icons/record.gif"></a></td>
</tr>

<tr>
	<td class="xtbl_label">Toggle Debug Logging</td>
	<td class="icon"><a href="javascript:toggle()"><img alt="Toggle Debug Logging" title="Toggle Debug Logging" src="/static/images/icons/refresh.gif"></a></td>
	<td class="xtbl_label">Real-Time Statistics</td>
	<td class="icon"><a href="/service/serviceMgr.php?vl=2"><img alt="Real-Time Statistics" title="Real-Time Statistics" src="/static/images/icons/report.gif"></a></td>
</tr>
</table>


		<table width="100%" class=xtbl border="0" cellpadding="5" cellspacing="1">
			<tr class=xtbl_header><td colspan="5" class=xtbl_title>Listeners</td></tr>
			<tr class="xtbl_title2"><td>&nbsp;</td><td>Status</td><td>Name</td><td>Address</td><td>Virtual Host Mappings</td></tr>
<?
foreach( $service->listeners as $name=>$l )
{
	echo '<tr class="xtbl_value"><td class=icon><img src="/static/images/icons/link.gif"></td>'."\n";
	if(strtoupper($l['status']) == "RUNNING") {
		echo "<td width=25 class=status_running>";
	}
	else {
		echo "<td width=25 class=status_stopped>";
	}

	echo $l['status'].'</td>';

	echo '<td width="120">'.$name . '</td><td>'. $l['addr'].'</td>';

	echo '<td >';

	if ( isset( $l['map'] ) )
	{
		echo '<table border="0" cellpadding="0" cellspacing="0">'."\n";
		foreach( $l['map'] as $vh => $lmap )
		{
			echo '<tr><td class="xtbl_value" style="font-size:11px;">['.$vh.'] ';
			echo( implode(' ', $lmap) );
			echo '</td></tr>';
		}
		echo '</table>' . "\n";
	}
	else
	{
		echo 'N/A';
	}
	echo "</td></tr>\n";
}
?>
		</table>

		<table width="100%" class=xtbl border="0" cellpadding="5" cellspacing="1">
			<tr class=xtbl_header><td colspan="4" class=xtbl_title>Virtual Hosts</td></tr>
			<tr class="xtbl_label">
				<td>&nbsp;</td><td>Status</td><td width="120">Name</td><td>Actions</td>
			</tr>
<? $vhName = array_keys($service->vhosts);
sort($vhName);
foreach( $vhName as $vh )
{
	echo '<tr class=xtbl_value><td class=icon><img src="/static/images/icons/web.gif"></td>';
	$value =  $service->vhosts[$vh];
	$canStart = 0;
	$canStop = 1;
	$canRestart = 1;
	
	if ( $value{1} == 'A' ) { //in config, but in status
		$canStop = 0;
		$canStart = 0;
		$canRestart = 0;
		echo '<td width=25 nowrap class=status_problem>Restart Required';
	}
	else if ( $value{0} == 1 ) { // active, default case
		echo '<td width=25 class=status_running>Running';
	}
	else {
		$canStart = 1;
		$canStop = 0;
		$canRestart = 0;
		echo '<td width=25 class=status_stopped>Stopped';
	}	

	echo '</td>';
	echo '<td >' . $vh . '</td>';
	echo '<td nowrap>';
	if ( $canStart ) {
		echo '<a href="javascript:go('."'enable','$vh'". ')"><img alt="start" title="Enable" src="/static/images/icons/play.gif"></a>';
	}
	if ( $canStop ) {
		echo '<a href="javascript:go('."'disable','$vh'".')"><img alt="stop" title="Disable" src="/static/images/icons/stop.gif"></a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;';
	}
	if ( $canRestart ) {
		echo '<a href="javascript:go('."'restart','$vh'". ')"><img alt="restart" title="Restart" src="/static/images/icons/refresh.gif"></a>';
	}
	echo "</td></tr>\n";
}
?>
        </table>
        


