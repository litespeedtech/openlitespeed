<?
if (!defined('LEGAL')) return;
echo '<h2 class="bottom_bar">' . TITLE . '</h2>';

if ( isset($check->pass_val['err']['bash'])) {
	echo '<div class="panel_error" align=left><span class="gui_error">'
		. $check->pass_val['err']['bash']
		. '</span></div>';
}


?>

<form name="buildphp" method="post">
<input type="hidden" name="step" value="1">

<table width="100%" class="xtbl" border="0" cellpadding="5" cellspacing="1">
	<tr class="xtbl_header">
		<td colspan="3" class="xtbl_title">
		 Step 1 : Select a PHP version
		</td>
	</tr>

<?

$basevers = array_keys($PHP_VER);
rsort($basevers);

$buf = '';
foreach($basevers as $base_ver) {

	$versions = $PHP_VER[$base_ver];

	$buf .= '<tr class="xtbl_value"><td class="xtbl_label">PHP ' . $base_ver . '</td><td class="icon"></td><td>';

	$buf .= '<select name="php_version'. $base_ver . '">';
	foreach( $versions as $o ) {
		$buf .= "<option value=\"$o\">$o</option>";
	}
	$buf .= '</select>&nbsp;&nbsp;&nbsp;&nbsp;';

	$buf .=	'<input type="submit" name="build' . $base_ver . '" value="Next">'
			. "</td></tr>\n";

}

echo $buf;

?>
</table>
</form>
<p class="field_note"> * If you want to use a version not listed here, you can manually update the settings in /usr/local/lsws/admin/html/utility/build_php/buildconf.inc.php.</p>
<p class="field_note"> ** For more information regarding LSPHP, please visit <a href="https://www.litespeedtech.com/support/wiki/doku.php/litespeed_wiki:php" target="_blank">LiteSpeed wiki</a>.</p>
