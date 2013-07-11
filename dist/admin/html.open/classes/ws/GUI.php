<?php

class GUI 
{

	public static function header($title = NULL) {
		//charset need to use utf-8 for international chars support. replaced iso-8859-1
		$client = CLIENT::singleton(); 
		$tk = $client->token;
		return "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0 Transitional//EN' 'http://www.w3.org/TR/html4/loose.dtd'>
<html>
<head>
	<title>{$title} - LiteSpeed Web Admin Console</title>
	<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>
	<meta HTTP-EQUIV='Cache-control' CONTENT='no-cache'>
	<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>
	<meta HTTP-EQUIV='Expires' CONTENT='-1'>
	<link rel='Shortcut Icon' type='image/x-icon' href='/static/images/icons/favicon.ico' />
	<link rel='stylesheet' type='text/css' href='/static/styles/style.css'>
	<script type='text/javascript'
	src='/static/scripts/general.js'></script>


</head>


<body >


<form name='mgrform' method='get' action='/service/serviceMgr.php'>
<input type='hidden' name='act'>
<input type='hidden' name='actId'>
<input type='hidden' name='vl'>
<input type='hidden' name='tk' value='{$tk}'>
</form>


<!-- START REAL BODY -->
<!-- START OUTER TABLE -->
<table width=980 border=0 cellspacing=0 cellpadding=0 align=center>
<tr><td>";

	}

	public static function top_menu($title = NULL) {

		global $_SESSION;

		$client = CLIENT::singleton();
		$product = PRODUCT::GetInstance();

		$state = DUtil::getGoodVal(DUtil::grab_input("get","state"));
		if($state == 'restarted') {
			$product->refreshVersion();
		}
		$buff = "";

		if ($title != NULL) {
			$buff .= "{$title}";
		}

		$buff .= "


<table width=100% style='margin-bottom:5px;' cellpadding=0 cellspacing=0>
<tr >
<td style='border-bottom:1px solid #cacaca;padding-bottom:2px;'><a href='/index.php'><img src='/static/images/logo/product_logo.gif' border=0></a></td>
<td style='border-bottom:1px solid #cacaca;padding-bottom:2px;' align=right valign=bottom>". ucwords(strtolower($product->product)). " " . ucwords(strtolower($product->edition)) . " v" . ucwords(strtolower($product->version)). " : <a href='/login.php?logoff=1'>Log Off</a></td>
</tr>
</table>
</td></tr>
<tr><td>
<table width='100%' border='0' cellpadding='0' cellspacing='1' id=top_menu >

<tr><td style='background: rgb(52, 79, 115);padding-left:10px; height: 28px;'>
<div id='nav' class='xui_menu'>
  <ul>
    <li><a href='/' class='mainlevel'>Home</a></li>
    <li><a href='/service/serviceMgr.php' class='mainlevel'>Actions</a>
      <ul>
 	<li><a href=\"javascript:go('restart','');\">Graceful Restart</a></li>
	<li><a href='javascript:toggle();'>Toggle Debug Logging</a></li>
	<li><a href='/service/serviceMgr.php?vl=1'>Server Log Viewer</a></li>
	<li><a href='/service/serviceMgr.php?vl=2'>Real-Time Stats</a></li>
        <li><a href='/service/serviceMgr.php?vl=3'>Version Manager</a></li>
	<li><a href='/utility/build_php/buildPHP.php'>Compile PHP</a></li>
      </ul>
    </li>
    <li><a href='/config/confMgr.php?m=serv'  class='mainlevel'>Configuration</a>
      <ul>
      	<li><a href='/config/confMgr.php?m=serv'>Server</a></li>
        <li><a href='/config/confMgr.php?m=sltop'>Listeners</a></li>
        <li><a href='/config/confMgr.php?m=vhtop'>Virtual Hosts</a></li>
        <li><a href='/config/confMgr.php?m=tptop'>Virtual Host Templates</a></li>
      </ul>
    </li>
    <li><a href='/' class='mainlevel'>Web Console</a>
      <ul>
        <li><a href='/config/confMgr.php?m=admin'>General</a></li>
        <li><a href='/config/confMgr.php?m=altop'>Listeners</a></li>
      </ul>
    </li>
    <li><a href='/docs/'  class='mainlevel' target=_new>Help</a></li>
  </ul>
</div>
</td></tr>
</table>
<table style='width:100%;border-left:1px solid #cacaca;border-right:1px solid #cacaca;border-bottom:1px solid #cacaca;'>
<tr><td style='padding:10px; '>
<!-- START CONTENT -->";

		if($client->changed === TRUE) {
			$buff .= XUI::message("","Configuration has been modified. To apply changes, please visit Control Panel and execute a Graceful Restart. <a href='/service/serviceMgr.php'>Apply Changes</a>");
		}


		return $buff;

	}

	public static function footer() {
		return "

<!-- END CONTENT -->

<!-- END REAL BODY -->



</td></tr></table>
<div id=copyright align=center>Copyright &copy; 2002-2013. <a href='http://www.litespeedtech.com'>Lite Speed Technologies, Inc.</a></div>



</body>
</html>";

	}


	public static function left_menu() {
		$confCenter = ConfCenter::singleton();

		$area = $confCenter->_confType;

		if($area == "admin") {
			return GUI::left_menu_admin();
		}


		$vhosts = $confCenter->_info['VHS'];
		$listeners = $confCenter->_info['LNS'];

		$templates = array();

		if(isset($confCenter->_serv->_data['tpTop'])) {
			$templates  = $confCenter->_serv->_data['tpTop'];
		}

		$area = DUtil::getGoodVal(DUtil::grab_input("request","m"));
		$stuff = array("Server" => "serv","Listeners (" . count($listeners) . ")" => "sl","Virtual Hosts (" . count($vhosts) . ")" => "vh", "Virtual Host Templates (" . count($templates) . ")" => "tp",);


		$buf = "<!-- START TABS -->
<div id=xtab>
<ul>";



		foreach ( $stuff as $tabName => $tabKey ) {
			$class= "";

			if ( substr($area,0,strlen($tabKey)) == $tabKey ) {
				$class = "on";

			}

			if($tabKey != "serv") {
				$tabKey = $tabKey . "top";
			}
			$buf .= "\t<li class='{$class}'><div><a href='/config/confMgr.php?m={$tabKey}'>{$tabName}</a></div></li>\n";

		}


		$buf .= "</ul>
	</div>
	<!-- END TABLS -->";

		return $buf;
	}

	public static function left_menu_admin() {
		$confCenter = ConfCenter::singleton();

		$listeners = $confCenter->_serv->_data['listeners'];

		$area = DUtil::getGoodVal(DUtil::grab_input("request","m"));
		$stuff = array("Admin" => "admin","Listeners (" . count($listeners) . ")" => "al");


		$buf = "<!-- START TABS -->
<div id=xtab>
<ul>";



		foreach ( $stuff as $tabName => $tabKey ) {
			$class= "";

			if ( substr($area,0,strlen($tabKey)) == $tabKey ) {
				$class = "on";

			}

			if($tabKey != "admin") {
				$tabKey = $tabKey . "top";
			}
			$buf .= "\t<li class='{$class}'><div><a href='/config/confMgr.php?m={$tabKey}'>{$tabName}</a></div></li>\n";

		}


		$buf .= "</ul>
</div>
<!-- END TABLS -->";

		return $buf;
	}

}

