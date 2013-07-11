<?php

$ciphers = array();

if ($customizedData != NULL) {
	$listener = &$customizedData;
	if(array_key_exists("ciphers",$listener)){
		$ciphers = explode(':',$listener['ciphers']->GetVal());
	}
}

$c = array( 'SSLv3' => 0, 'TLSv1' => 0,
	'HIGH' => 0, 'MEDIUM' => 0, 'LOW' => 0, 'EXPORT56' => 0, 'EXPORT40' => 0);
foreach( $ciphers as $ci )
{
	if ( isset($c[$ci]) )
	{
		$c[$ci] = 1;
	}
	else
	{ // for backword compatibility
		$a = substr($ci,1);
		if ( isset($c[$a]) )
		{
			if ( $ci{0} == '+' )
				$c[$a] = 1;
		}
	}
}

function showCheckBox($val)
{
	$gif = ( $val )? 'checked.gif':'unchecked.gif';
	echo '<img src="/static/images/graphics/'.$gif.'" width="12" height="12">';
}
?>
			<tr class="xtbl_label"><td>Version</td></tr>
			<tr class="xtbl_value"><td><table border="0" cellpadding="3" width="100%" cellspacing="0">
				    <tr class="xtbl_value">
					<td><?php showCheckBox($c['SSLv3']);?> SSL v3.0</td>
					<td><?php showCheckBox($c['TLSv1']);?> TLS v1.0</td>
					</tr>
					</table>
            </td></tr>
			<tr class="xtbl_label"><td>Encryption Level</td></tr>
			<tr class="xtbl_value"><td><table border="0" cellpadding="3" cellspacing="0">
				<tr><td><?php showCheckBox($c['HIGH']);?></td>
					<td width="80">HIGH</td><td>Triple-DES encryption</td></tr>
				<tr><td><?php showCheckBox($c['MEDIUM']);?></td>
					<td>MEDIUM</td><td>128-bit encryption</td></tr>
				<tr><td><?php showCheckBox($c['LOW']);?></td>
					<td>LOW</td><td>64 or 56 bit encryption but exclude Export cipher suites</td></tr>
				<tr><td><?php showCheckBox($c['EXPORT56']);?></td>
					<td>EXPORT56</td><td>56-bit Export encryption</td></tr>
				<tr><td><?php showCheckBox($c['EXPORT40']);?></td>
					<td>EXPORT40</td><td>40-bit Export encryption</td></tr>
				</table>
			</td></tr>
