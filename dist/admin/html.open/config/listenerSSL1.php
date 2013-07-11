<?php
$listener = &$customizedData;

$ciphers = array();

if(isset($listener['ciphers'])) {
	$ciphers = explode(':',$listener['ciphers']->GetVal());
}

$c = array( 'SSLv3' => ' ', 'TLSv1' => ' ',
	'HIGH' => ' ', 'MEDIUM' => ' ', 'LOW' => ' ', 'EXPORT56' => ' ', 'EXPORT40' => ' ');
foreach( $ciphers as $ci )
{
	if ( isset($c[$ci]) )
	{
		$c[$ci] = 'checked';
	}
	else
	{ 	// for backword compatibility
		$a = substr($ci,1);
		if ( isset($c[$a]) )
		{
			if ( $ci{0} == '+' )
				$c[$a] = 'checked';
		}
	}
}

?>
			<tr class="xtbl_label"><td>Version 
<?
	if (isset($listener['ciphers']) && $listener['ciphers']->HasErr()) {
		echo '<span class="field_error">*' . $listener['ciphers']->GetVal() . '</span>';
	}
?>			 
			</td></tr>
			<tr class="xtbl_value"><td><table border="0" width="100%" cellpadding="3" cellspacing="0">
					<tr>
						<td><input type="checkbox" name="ck1" value="SSLv3" <?php echo $c['SSLv3'];?>>SSL v3.0</td>
						<td><input type="checkbox" name="ck2" value="TLSv1" <?php echo $c['TLSv1'];?>>TLS v1.0</td>
					</tr>
					</table>
            </td></tr>
			<tr class="xtbl_label"><td>Encryption Level</td></tr>
			<tr class="xtbl_value"><td><table border="0" cellpadding="3" cellspacing="0">
				<tr><td><input type="checkbox" name="ck3" value="HIGH" <?php echo $c['HIGH'];?>></td>
					<td width="80">HIGH</td><td>Triple-DES encryption</td></tr>
				<tr><td><input type="checkbox" name="ck4" value="MEDIUM" <?php echo $c['MEDIUM'];?>></td>
					<td>MEDIUM</td><td>128-bit encryption</td></tr>
				<tr><td><input type="checkbox" name="ck5" value="LOW" <?php echo $c['LOW'];?>></td>
					<td>LOW</td><td>64 or 56 bit encryption but exclude Export cipher suites</td></tr>
				<tr><td><input type="checkbox" name="ck6" value="EXPORT56" <?php echo $c['EXPORT56'];?>></td>
					<td>EXPORT56</td><td>56-bit Export encryption</td></tr>
				<tr><td><input type="checkbox" name="ck7" value="EXPORT40" <?php echo $c['EXPORT40'];?>></td>
					<td>EXPORT40</td><td>40-bit Export encryption</td></tr>
				</table>
			</td></tr>
