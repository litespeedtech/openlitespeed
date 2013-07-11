<?
function genOptions(&$options, $selValue)
{
	$output = '';
	foreach ( $options as $key => $value )
	{
		$output .= '<option value="' . $key .'"';
		if ( $key === $selValue )
			$output .= ' selected';
		$output .= ">Display Level: $value</option>\n";
	}
	return $output;
}
$options = array('E'=>'ERROR', 'W'=>'WARNING',
			   'N'=>'NOTICE', 'I'=>'INFO', 'D'=>'DEBUG');
$service = new Service();
$data = &$service->getLogData();
if ($data == NULL)
	return;
?>

<h2>Server Log Viewer</h2>

<form name="logform" method="get">
<input type="hidden" name="vl" value="1">
<table width="100%" border="0" cellpadding="0" cellspacing="4">
	<tr>
		<td>
		<select name="sel_level">
			<? echo( genOptions($options, $data['level']) ); ?>
		</select>
		</td>
		<td>
		Log Size: <? echo $data['logSize'];?> KB
		</td>
		<td>
		Search from <input name="searchFrom" type="text" size="6" value="<? echo $data['searchFrom'];?>">KB
		</td>
		<td>
		Length <input name="searchSize" type="text" size="6" value="<? echo $data['searchSize'];?>">KB

		</td>
	</tr>
</table>


<table  width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td align="center" nowrap>
		<input type="image" name="begin" height="22" width="55" src="/static/images/buttons/begin.gif">&nbsp;&nbsp;
		<input type="image" name="prev" height="22" width="55" src="/static/images/buttons/prev.gif">&nbsp;&nbsp;
		<input type="image" name="refresh" height="22" width="55" src="/static/images/buttons/refresh.gif">&nbsp;&nbsp;
		<input type="image" name="next" height="22" width="55" src="/static/images/buttons/next.gif">&nbsp;&nbsp;
		<input type="image" name="end" height="22" width="55" src="/static/images/buttons/end.gif">
		</td>
	</tr>
</table>

<table class=xtbl width="100%" border="0" cellpadding="5" cellspacing="1">
<?

$res = &$service->getLog($data);
echo '<tr class="xtbl_title"><td colspan="3">Retrieved '. number_format($res[0]) . ' of total ' . number_format($res[1]) . ' log entries.</td></tr>';
echo '<tr  class=xtbl_label><td width="170">Time</td><td width="50">Level</td><td>Message</td></tr>';
echo $res[2];

?>
</table>

<table  width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td align="center" nowrap>
		<input type="image" name="begin" height="22" width="55" src="/static/images/buttons/begin.gif">&nbsp;&nbsp;
		<input type="image" name="prev" height="22" width="55" src="/static/images/buttons/prev.gif">&nbsp;&nbsp;
		<input type="image" name="refresh" height="22" width="55" src="/static/images/buttons/refresh.gif">&nbsp;&nbsp;
		<input type="image" name="next" height="22" width="55" src="/static/images/buttons/next.gif">&nbsp;&nbsp;
		<input type="image" name="end" height="22" width="55" src="/static/images/buttons/end.gif">
		</td>
	</tr>
</table>
</form>

