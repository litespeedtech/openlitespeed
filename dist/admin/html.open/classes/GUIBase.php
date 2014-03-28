<?php

class GUIBase
{

	public static function message($title="", $msg="", $type = "normal")
	{
		return "<div class=panel_{$type} align=left><span class='gui_{$type}'>" . (strlen($title) ? "$title<hr size=1 noshade>" : "") . "$msg</span></div>";
	}

	public static function header($title = NULL)
	{
		//charset need to use utf-8 for international chars support. replaced iso-8859-1
		$client = CLIENT::singleton();
		$tk = $client->token;
		return "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0 Transitional//EN' 'http://www.w3.org/TR/html4/loose.dtd'>
		<html>
		<head>
		<title>{$title} - LiteSpeed WebAdmin Console</title>
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
		<input type='hidden' name='act'><input type='hidden' name='actId'><input type='hidden' name='vl'><input type='hidden' name='tk' value='{$tk}'>
		</form>
		<div id=\"main-wrapper\">";
	}

	public static function gen_top_menu($dropdown_ul, $ver_link='') {

		global $_SESSION;

		$client = CLIENT::singleton();
		$product = PRODUCT::GetInstance();

		$state = DUtil::getGoodVal(DUtil::grab_input("get","state"));
		if($state == 'restarted') {
			$product->refreshVersion();
		}

		$buf = '<div><a href="/index.php"><img src="/static/images/logo/product_logo.gif"></a><span style="float:right;margin-top:50px">'
				. ucwords(strtolower($product->product)). ' ' . ucwords(strtolower($product->edition)) . ' ';

		if ($ver_link != '')
			$buf .= "<a href=\"$ver_link\" title=\"Go to version management\">{$product->version}</a>";
		else
			$buf .= $product->version;

		$buf .= '&nbsp;&nbsp;&nbsp;&nbsp;<a href="/login.php?logoff=1">Log Off</a></span></div>';

		$buf .= '<div id="nav" class="xui_menu">' . $dropdown_ul;

		$buf .= '</div>
<div id="content-wrapper">
<!-- START CONTENT -->';

		if($client->changed === TRUE) {
			$buf .= GUIBase::message("",'Configuration has been modified. To apply changes, please perform a <a href="javascript:go(\'restart\', \'\');">Graceful Restart</a>.');
		}

		return $buf;

	}

	public static function gen_footer($year, $extra) {
		$buf = '
<!-- END CONTENT -->
</div>
<div id="copyright" align="center">Copyright &copy; ' . $year
	. ' <a href="http://www.litespeedtech.com">LiteSpeed Technologies, Inc.</a> All Rights Reserved.</div>'
	. "<!-- $extra -->\n"
	. '</body></html>';

		return $buf;
	}

	public static function gen_left_menu($tabs)
	{
		$area = DUtil::getGoodVal(DUtil::grab_input("request","m"));

		$buf = '<!-- START TABS -->
<div class="xtab" style="margin-left:-8px;"><ul>';

		foreach ( $tabs as $tabName => $tabKey ) {
			$on = '';
			if ( substr($area,0,strlen($tabKey)) == $tabKey ) {
				$on = 'class="on"';
			}

			$buf .= "<li $on><a href=\"/config/confMgr.php?m={$tabKey}\">{$tabName}</a></li>\n";
		}

		$buf .= '</ul></div>
	<!-- END TABS -->';

		return $buf;
	}



}
