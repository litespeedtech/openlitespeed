<?php

$stats = new STATS;
$stats->parse_all();

function getSelectOptions($selType, $selValue)
{
	if ($selType == 'REFRESH')
	{
		$options = array('0'=>'Stop',
			'10'=>'10 Seconds','15'=>'15 Seconds',
			'30'=>'30 Seconds','60'=>'60 Seconds',
			'120' => '2 minutes', '300' => '5 minutes');
	}
	else if ($selType == 'SHOW_TOP')
	{
		$options = array('5'=>'Top 5',
			'10'=>'Top 10', '20'=>'Top 20',
			'50'=>'Top 50', '0'=>'All');
	}
	else if ($selType == 'VH_SHOW_SORTBY')
	{
		$options = array(
			'vhname'=>'Virtual Host Name',
			'req_processing'=>'Requests in Processing',
			'req_per_sec'=>'Request/Second',
			'output_bankdwidth'=>'Output Bandwidth',
			'eap_process'=>'ExtApp Processes',
			'eap_inuse'=>'EAProc In Use',
			'eap_idle'=>'EAProc Idle',
			'eap_waitQ'=>'EAProc WaitQ',
			'eap_req_per_sec'=>'EAProc Req/Sec');
	}
	else if ($selType == 'EAP_SHOW_SORTBY')
	{
		$options = array(
			'vhost'=>'Scope',
			'type'=>'Type',
			'extapp'=>'Name',
			'config_max_conn'=>'Max CONN',
			'effect_max_conn'=>'Eff Max',
			'pool_size'=>'Pool',
			'inuse_conn'=>'In Use',
			'idle_conn'=>'Idle',
			'waitqueue_depth'=>'WaitQ',
			'req_per_sec'=>'Req/Sec');
	}

	return GuiBase::genOptions($options, $selValue);

}

$refresh = GUIBase::GrabGoodInput("request","refresh","int");
$vh_show_ind = GUIBase::GrabGoodInput("request","vh_show_ind");
$vh_show_top = GUIBase::GrabGoodInput("request","vh_show_top");
$vh_show_filter = GUIBase::GrabGoodInput("request","vh_show_filter","string");
$vh_show_sort = GUIBase::GrabGoodInput("request","vh_show_sort","string");
$eap_show_ind = GUIBase::GrabGoodInput("request","eap_show_ind");
$eap_show_top = GUIBase::GrabGoodInput("request","eap_show_top");
$eap_show_filter = GUIBase::GrabGoodInput("request","eap_show_filter","string");
$eap_show_sort = GUIBase::GrabGoodInput("request","eap_show_sort","string");
$cur_time = gmdate("D M j H:i:s T");
$server_info = "server {$service->serv['name']} snapshot at $cur_time";


// setting defaults
if ($vh_show_ind == '') {
	$vh_show_ind = 'Show';
}

if ($vh_show_top === '') {
	$vh_show_top = '5';
}

if ($vh_show_sort == '') {
	$vh_show_sort = 'req_per_sec';
}

if ($eap_show_ind == '') {
	$eap_show_ind = 'Show';
}

if ($eap_show_top === '') {
	$eap_show_top = '5';
}

if ($eap_show_sort == '') {
	$eap_show_sort = 'req_per_sec';
}

if ($refresh <= 1 && $refresh != 0) {
	$refresh = 5;
}

if($refresh >= 2) {
	echo '<META http-equiv="Refresh" content=' . $refresh . '>';
}


?>
<form name="rpt" method="get"><input type="hidden" name="vl" value="2">
<input type="hidden" name="vh_show_ind" value="<?echo $vh_show_ind;?>">
<input type="hidden" name="eap_show_ind" value="<?echo $eap_show_ind;?>">

<div class="bottom_bar">
<span  class="h2_font">Real-Time Statistics</span>&nbsp;&nbsp;&nbsp; <? echo $server_info;?>
<span style="float:right">Refresh Interval: <select onChange='document.rpt.submit();' name="refresh"  class="th-clr">
			<?
			echo( getSelectOptions('REFRESH', $refresh) );
			?>
		</select></span>
</div>

<div style="margin-top:20px;">
	<div style="width:350px;display:inline-block;vertical-align:top;margin-right:20px;">
	<table class="xtbl" width="100%" border="0" cellpadding="3"	cellspacing="1">
			<tr>
				<td class="xtbl_title" colspan=2>Server Health</td>
			</tr>
			<?
			$buf = '<tr><td width=120 class="xtbl_label_vert">Uptime</td><td class="xtbl_value">'. ucwords($stats->uptime)
			.'</td></tr>';
			$buf .= '<tr><td width=120 class="xtbl_label_vert">Load</td><td class="xtbl_value">' . $stats->load_avg
			.'</td></tr>' ."\n";
			$blocked_count = count($stats->blocked_ip);
			$blocked_sample = 'NONE';
			if ($blocked_count > 14) {
				$blocked_sample = join(', ', array_slice($stats->blocked_ip, 0, 14) );
				$blocked_sample .= '<br> ... <br>Total ' . $blocked_count . ' blocked &nbsp;&nbsp;&nbsp;<a target=_new href="blockip_html.php">Show All</a>';
			}
			else if ($blocked_count > 0) {
				$blocked_sample = join(', ', $stats->blocked_ip);
			}

			$buf .= '<tr><td width=120 class="xtbl_label_vert">Anti-DDoS Blocked IP</td><td class="xtbl_value">' . $blocked_sample
			.'</td></tr>' ."\n";
			echo $buf;
			?>
	</table></div>
	<div style="width:560px;display:inline-block;">
		<table class="xtbl" width="100%" border="0" cellpadding="3"
			cellspacing="1">
			<tr>
				<td class="xtbl_title" colspan=8>Server</td>
			</tr>
			<?
			$buf = '<tr><td class="xtbl_label_vert">Network Throughput</td>'
			. '<td width=1 class="xtbl_label_vert"><a target=_new href="graph_html.php?gtitle=Plain Traffic&vhost=&extapp=&items=bps_in,bps_out&titles=IN Plain KB/s,OUT Plain KB/s&colors=ff6600,8ad688&yaxis=KBytes / Second"><img src="/static/images/icons/graph.gif" border=0></a></td>'
			. '<td colspan=2 class="xtbl_value" >Http In</td><td class="xtbl_value">'
			. number_format($stats->bps_in)	. 'KB</td>'
			. '<td colspan=2 class="xtbl_value" >Http Out</td><td class="xtbl_value">'
			. number_format($stats->bps_out)	. 'KB</td></tr>'."\n";
			$buf .= '<tr><td class="xtbl_value">&nbsp;</td>'
			. '<td width=1 class="xtbl_label_vert"><a target=_new href="graph_html.php?gtitle=SSL Traffic&vhost=&extapp=&items=ssl_bps_in,ssl_bps_out&titles=IN SSL KB/s,OUT SSL KB/s&colors=ff6600,8ad688&yaxis=KBytes / Second"><img src="/static/images/icons/graph.gif" border=0></a></td>'
			. '<td colspan=2 class="xtbl_value" >Https In</td><td class="xtbl_value">'
			. number_format($stats->ssl_bps_in)	. 'KB</td>'
			. '<td colspan=2 class="xtbl_value" >Https Out</td><td class="xtbl_value">'
			. number_format($stats->ssl_bps_out)	. 'KB</td></tr>'."\n";
			$buf .= '<tr><td class="xtbl_label_vert">Connections</td>'
			. '<td width=1 class="xtbl_label_vert"><a target=_new href="graph_html.php?gtitle=Connections General&vhost=&extapp=&items=max_conn,idle_conn&titles=Max Connections,Idle Connections&colors=ff6600,8ad688&yaxis=Connections"><img src="/static/images/icons/graph.gif" border=0></a></td>'
			. '<td colspan=2 class="xtbl_value" >Max</td><td class="xtbl_value">'
			. number_format($stats->max_conn) . '</td>'
			. '<td colspan=2 class="xtbl_value" >Idle</td><td class="xtbl_value">'
			. number_format($stats->idle_conn) . '</td></tr>'."\n";
			$buf .= '<tr><td class="xtbl_value">&nbsp;</td>'
			. '<td width=1 class="xtbl_label_vert"><a target=_new href="graph_html.php?gtitle=Plain Connections&vhost=&extapp=&items=plain_conn,avail_conn&titles=Used Plain Connections,Free Plain Connections&colors=ff6600,8ad688&yaxis=Connections"><img src="/static/images/icons/graph.gif" border=0></a></td>'
			. '<td colspan=2 class="xtbl_value" >Http Used</td><td class="xtbl_value">'
			. number_format($stats->plain_conn)	. '</td>'
			. '<td colspan=2 class="xtbl_value" >Http Free</td><td class="xtbl_value">'
			. number_format($stats->avail_conn)	. '</td></tr>'."\n";
			$buf .= '<tr><td class="xtbl_value">&nbsp;</td>'
			. '<td width=1 class="xtbl_label_vert"><a target=_new href="graph_html.php?gtitle=SSL Connections&vhost=&extapp=&items=ssl_conn,avail_ssl_conn&titles=Used SSL Connections,Free SSL Connections&colors=ff6600,8ad688&yaxis=Connections"><img src="/static/images/icons/graph.gif" border=0></a></td>'
			. '<td colspan=2 class="xtbl_value" >Https Used</td><td class="xtbl_value">'
			. number_format($stats->ssl_conn)	. '</td>'
			. '<td colspan=2 class="xtbl_value" >Https Free</td><td class="xtbl_value">'
			. number_format($stats->avail_ssl_conn)	. '</td></tr>'."\n";
			$buf .= '<tr><td class="xtbl_label_vert">Requests</td>'
			. '<td width=1 class="xtbl_label_vert"><a target=_new href="graph_html.php?gtitle=VHOST Requests:Server&vhost=_Server&extapp=&items=req_processing,req_per_sec&titles=Requests In-Processing,Requests Per Second&colors=ff6600,8ad688&yaxis=Requests"><img src="/static/images/icons/graph.gif" border=0></a></td>'
			. '<td colspan=2 class="xtbl_value" >In Processing</td><td class="xtbl_value">'
			. number_format($stats->serv->req_processing). '</td>'
			. '<td colspan=2 class="xtbl_value" >Req/Sec</td><td class="xtbl_value">'
			. number_format($stats->serv->req_per_sec, 1)	. '</td></tr>'."\n";
			$buf .= '<tr><td class="xtbl_value">&nbsp;</td>'
			. '<td width=1 class="xtbl_label_vert"></td>'
			. '<td colspan=2 class="xtbl_value" >Total Req</td><td colspan=4 class="xtbl_value">'
			. number_format($stats->serv->req_total). '</td></tr>'."\n";

			echo $buf;

			?>
	</table></div>
</div>
<div>
<table class="xtbl" width="100%" border="0" cellpadding="3"
	cellspacing="1">
	<tr class="xtbl_title">
		<td colspan="11">
		<table width="100%" border="0" cellpadding="3">
			<tr>
				<td>Virtual Host</td>
				<td>
<?
				$buf = '';
				if ($vh_show_ind == 'Show') {
					$buf .= '<button name="vh_show" type="button" onclick="document.rpt.vh_show_ind.value=\'Hide\';document.rpt.submit();">Hide</button>';
				} else {
					$buf .= '<button name="vh_show" type="button" onclick="document.rpt.vh_show_ind.value=\'Show\';document.rpt.submit();">Show</button>';
				}

				$buf .= '</td><td>Display: <select name="vh_show_top">'
				. getSelectOptions('SHOW_TOP', $vh_show_top)
				. '</select></td>'."\n";
				$buf .= '<td>Filter by Name (take regExp): <input type="text" name="vh_show_filter" value="'
				. $vh_show_filter . '"></td>' ."\n";
				$buf .= '<td>Sort by: <select name="vh_show_sort">'
				. getSelectOptions('VH_SHOW_SORTBY', $vh_show_sort)
				. '</select></td>'."\n";
				$buf .= '<td><input name="vh_show_apply" value="Apply" type="submit">';
				echo $buf;
?>
				</td>
			</tr>
		</table>
		</td>
	</tr>

	<?
	if ($vh_show_ind == 'Show') {

		$buf = '<tr class="xtbl_label_vert">'
		. '<td>VH Name</td><td>&nbsp;</td>'
		. '<td>Req in Processing</td><td>Req/Sec</td><td>Total Req</td>'
		. '<td>Output Bandwidth</td>'
		. '<td>ExtApp Processes</td><td>EAProc In Use</td><td>EAProc Idle</td>'
		. '<td>EAProc WaitQ</td><td>EAProc Req/Sec</td></tr>'
		. "\n";

		$vhlist = $stats->apply_vh_filter($vh_show_top, $vh_show_filter, $vh_show_sort);

		foreach( $vhlist as $vhname ) {
			if ($vhname == '_Server')
				continue;
			$vh = $stats->vhosts[$vhname];
			$buf .= '<tr class="xtbl_value"><td>' . $vhname . '</td>';
			$buf .= '<td width=1><a target=_new href="graph_html.php?gtitle=VHOST Requests: '
			. urlencode($vhname) . '&vhost=' . urlencode($vhname)
			. '&extapp=&items=req_processing,req_per_sec&titles=Requests In-Processing,Requests Per Second&colors=ff6600,8ad688&yaxis=Requests"><img src="/static/images/icons/graph.gif" border=0></a></td>';

			$buf .= '<td align="center">'.number_format($vh->req_processing).'</td>';
			$buf .= '<td align="center">'.number_format($vh->req_per_sec,1).'</td>';
			$buf .= '<td align="center">'.number_format($vh->req_total).'</td>';
			$buf .= '<td align="center">'.number_format($vh->output_bankdwidth).'</td>';
			$buf .= '<td  align="center">'.number_format($vh->eap_process).'</td>';
			$buf .= '<td  align="center">'.number_format($vh->eap_inuse).'</td>';
			$buf .= '<td  align="center">'.number_format($vh->eap_idle).'</td>';
			$buf .= '<td  align="center">'.number_format($vh->eap_waitQ).'</td>';
			$buf .= '<td  align="center">'.number_format($vh->eap_req_per_sec).'</td>';
			$buf .= '</tr>'."\n";

		}
	echo $buf;
	}
	?>
</table>
</div>
<div>
<table class="xtbl" width="100%" border="0" cellpadding="3"
	cellspacing="1">
	<?
	$buf = '<tr class="xtbl_title"><td colspan=10>'
	. '<table width="100%" border="0" cellpadding="3">'
	. '<tr><td>External Application</td><td>';
	if ($eap_show_ind == 'Show') {
		$buf .= '<button name="eap_show" type="button" onclick="document.rpt.eap_show_ind.value=\'Hide\';document.rpt.submit();">Hide</button>';
	} else {
		$buf .= '<button name="eap_show" type="button" onclick="document.rpt.eap_show_ind.value=\'Show\';document.rpt.submit();">Show</button>';
	}

	$buf .= '</td><td>Display: <select name="eap_show_top">'
	. getSelectOptions('SHOW_TOP', $eap_show_top)
	. '</select></td>'."\n";
	$buf .= '<td>Filter by Name (take regExp): <input type="text" name="eap_show_filter" value="'
	. $eap_show_filter . '"></td>' ."\n";
	$buf .= '<td>Sort by: <select name="eap_show_sort">'
	. getSelectOptions('EAP_SHOW_SORTBY', $eap_show_sort)
	. '</select></td>'."\n";
	$buf .= '<td><input name="eap_show_apply" value="Apply" type="submit"></td>'
	. '</tr></table></td></tr>' . "\n";

	if ($eap_show_ind == 'Show') {

		$buf .= '<tr class="xtbl_label_vert">'
		. '<td>Scope</td><td>Type</td><td>Name</td><td>&nbsp;</td>'
		. '<td>Max CONN</td><td>Eff Max</td><td>Pool</td>'
		. '<td>In Use</td><td>Idle</td><td>WaitQ</td><td>Req/Sec</td>'
		. "</tr>\n";
		$exapps = $stats->apply_eap_filter($eap_show_top, $eap_show_filter, $eap_show_sort);

		foreach( $exapps as $eap ) {
			$buf .= '<tr class="xtbl_value"><td>' . $eap->vhost . '</td>';
			$buf .= '<td align="center">' . $eap->type . '</td>';
			$buf .= '<td align="center">' . $eap->extapp . '</td>';
			$buf .= '<td width=1><a target=_new href="graph_html.php?gtitle=VHOST ExtApp: '
			.urlencode($eap->vhost).'&vhost='.urlencode($eap->vhost)
			.'&extapp='.urlencode($eap->extapp).
					'&items=req_per_sec,inuse_conn,waitqueue_depth&titles=Requests Per Second,In-Use Connections,Wait-Queue Depth&colors=ff6600,8ad688,003366&yaxis=Request/Connection"><img src="/static/images/icons/graph.gif" border=0>'
					. '</a></td>';
					$buf .= '<td align="center">'.number_format($eap->config_max_conn).'</td>';
					$buf .= '<td align="center">'.number_format($eap->effect_max_conn).'</td>';
					$buf .= '<td align="center">'.number_format($eap->pool_size).'</td>';
					$buf .= '<td  align="center">'.number_format($eap->inuse_conn).'</td>';
					$buf .= '<td  align="center">'.number_format($eap->idle_conn).'</td>';
					$buf .= '<td  align="center">'.number_format($eap->waitqueue_depth).'</td>';
					$buf .= '<td  align="center">'.number_format($eap->req_per_sec).'</td>';
					$buf .= '</tr>'."\n";
			}
		}
		echo $buf;
?>
</table>
</div>


</form>
