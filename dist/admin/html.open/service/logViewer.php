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

<h2 class="bottom_bar">Server Log Viewer</h2>

<form name="logform" method="get">
<input type="hidden" name="vl" value="1">
<div class="log_filter">
	<span><select name="sel_level" class="th-clr">
			<? echo( genOptions($options, $data['level']) ); ?>
		</select></span>
	<span>Log Size: <? echo $data['logSize'];?> KB</span>

	<span>Search from <input name="searchFrom" type="text" size="6" value="<? echo $data['searchFrom'];?>">KB</span>

	<span>Length <input name="searchSize" type="text" size="6" value="<? echo $data['searchSize'];?>">KB</span>
</div>

<div style="text-align:center; margin-bottom:15px">
<input type="submit" name="begin" value="&lt;&lt;" title="Begin">
<input type="submit" name="prev" value="&lt;" title="Prev">
<input type="submit" name="refresh" value="Refresh">
<input type="submit" name="next" value="&gt;" title="Next">
<input type="submit" name="end" value="&gt;&gt;" title="End">
</div>

<table class="xtbl log_tbl" border="0" cellpadding="5" cellspacing="1">
<?

$res = &$service->getLog($data);
echo '<caption>Retrieved '. number_format($res[0]) . ' of total ' . number_format($res[1]) . ' log entries.</caption>';

echo '<tr class="xtbl_title"><td class="col_time">Time</td><td class="col_level">Level</td><td class="col_mesg">Message</td></tr>';
echo $res[2];

?>
</table>

<div style="text-align:center; margin:20px auto">
<input type="submit" name="begin" value="&lt;&lt;" title="Begin">
<input type="submit" name="prev" value="&lt;" title="Prev">
<input type="submit" name="refresh" value="Refresh">
<input type="submit" name="next" value="&gt;" title="Next">
<input type="submit" name="end" value="&gt;&gt;" title="End">
</div>
</form>

