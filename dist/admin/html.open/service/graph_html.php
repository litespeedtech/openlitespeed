<?php

require_once("../includes/auth.php");


$graph_title = DUtil::getGoodVal(DUtil::grab_input("get","gtitle"));
$vhost = DUtil::getGoodVal(DUtil::grab_input("get","vhost"));
$extapp = DUtil::getGoodVal(DUtil::grab_input("get","extapp"));
$items = DUtil::getGoodVal(DUtil::grab_input("get","items"));
$titles = DUtil::getGoodVal(DUtil::grab_input("get","titles"));
$colors = DUtil::getGoodVal(DUtil::grab_input("get","colors"));
$yaxis = DUtil::getGoodVal(DUtil::grab_input("get","yaxis"));

$xmlsrc = 'graph_xml.php?vhost=' . urlencode($vhost)
	. '&extapp=' . urlencode($extapp)
	. '&items=' . urlencode($items)
	. '&titles=' . urlencode($titles)
	. '&colors=' . urlencode($colors)
	. '&yaxis=' . urlencode($yaxis);
$xmlsrc = urlencode($xmlsrc);
echo GUI::header();
?>

<table class="xtbl" width="100%" border="0" cellpadding="4" cellspacing="1">
<tr height="15"><td class="xtbl_title"><?php echo htmlspecialchars($graph_title,ENT_QUOTES)?> (interval = 10 secs)</td></tr>

<tr class="xtbl_value"><td>
<OBJECT classid="clsid:D27CDB6E-AE6D-11cf-96B8-444553540000"
	codebase="http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,0,0"
	WIDTH="100%"
	HEIGHT="220"
	id="charts"
	ALIGN="">
<PARAM NAME=movie VALUE="/static/flash/charts.swf?license=D1XIRHMWKRL.HSK5T4Q79KLYCK07EK&library_path=/static/flash/charts_library&xml_source=<? echo $xmlsrc;?>">

<PARAM NAME=quality VALUE=high>


<EMBED src="/static/flash/charts.swf?license=D1XIRHMWKRL.HSK5T4Q79KLYCK07EK&library_path=/static/flash/charts_library&xml_source=<? echo $xmlsrc;?>"
       quality=high
       WIDTH="100%"
       HEIGHT="220"
       NAME="charts"
       ALIGN=""
       TYPE="application/x-shockwave-flash"
       PLUGINSPAGE="http://www.macromedia.com/go/getflashplayer">
</EMBED>
</OBJECT>
</td></tr>
</table>

<?php
echo GUI::footer();
?>
