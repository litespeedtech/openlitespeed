<?php

class GUIBase
{

	public static function message($title="", $msg="", $type = "normal")
	{
		return "<div class=panel_{$type} align=left><span class='gui_{$type}'>" . (strlen($title) ? "$title<hr size=1 noshade>" : "") . "$msg</span></div>";
	}

	public static function header()
	{
		//charset need to use utf-8 for international chars support. replaced iso-8859-1
		$tk = CAuthorizer::GetToken();
		return "<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.0 Transitional//EN' 'http://www.w3.org/TR/html4/loose.dtd'>
		<html>
		<head>
		<title>LiteSpeed WebAdmin Console</title>
		<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>
		<meta HTTP-EQUIV='Cache-control' CONTENT='no-cache'>
		<meta HTTP-EQUIV='Pragma' CONTENT='no-cache'>
		<meta HTTP-EQUIV='Expires' CONTENT='-1'>
		<link rel='Shortcut Icon' type='image/x-icon' href='/static/images/icons/favicon.ico' />
		<link rel='stylesheet' type='text/css' href='/static/styles/style.css'>
		<script type='text/javascript' src='/static/scripts/general.js'></script>
		</head>
		<body >
		<form name='mgrform' method='get' action='/service/serviceMgr.php'>
		<input type='hidden' name='act'><input type='hidden' name='actId'><input type='hidden' name='vl'><input type='hidden' name='tk' value='{$tk}'>
		</form>
		<div id=\"main-wrapper\">";
	}

	public static function gen_top_menu($dropdown_ul, $ver_link='') {

		global $_SESSION;

		$product = PRODUCT::GetInstance();

		$state = self::GrabGoodInput("get","state");
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

		if(CClient::HasChanged()) {
			$buf .= GUIBase::message('','Configuration has been modified. To apply changes, please perform a <a href="javascript:go(\'restart\', \'\');">Graceful Restart</a>.');
		}

		return $buf;

	}

	public static function gen_footer($year, $extra) {
		$buf = '
<!-- END CONTENT -->
</div></div>
<div id="copyright" align="center">Copyright &copy; ' . $year
	. ' <a href="http://www.litespeedtech.com">LiteSpeed Technologies, Inc.</a> All Rights Reserved.</div>'
	. "<!-- $extra -->\n"
	. '</body></html>';

		return $buf;
	}

	public static function genOptions($options, $selValue)
	{
		$o = '';
		if ( $options )
		{
			foreach ( $options as $key => $value )
			{
				$o .= '<option value="' . $key .'"';
				if ( $key == $selValue ) {
					if (!($selValue === '' && $key === 0)
					&& !($selValue === NULL && $key === 0)
					&& !($selValue === '0' && $key === '')
					&& !($selValue === 0 && $key === ''))
						$o .= ' selected';
				}
				$o .= ">$value</option>\n";
			}
		}
		return $o;
	}

	public static function GrabInput($origin, $name, $type = '')
	{
		if($name == '' || $origin == '')
			return NULL;

		global $_REQUEST, $_COOKIE, $_GET, $_POST;
		$temp = NULL;

		switch(strtoupper($origin)) {
			case "REQUEST":
			case "ANY":	$temp = $_REQUEST;
			break;
			case "GET": $temp = $_GET;
			break;
			case "POST": $temp = $_POST;
			break;
			case "COOKIE": $temp = $_COOKIE;
			break;
			case "FILE": $temp = $_FILES;
			break;
			case "SERVER": $temp = $_SERVER;
			break;
			default:
				die("input extract error.");
		}

		if(array_key_exists($name, $temp))
			$temp =  $temp[$name];
		else
			$temp = NULL;

		switch($type) {
			case "int": return (int) $temp;
			case "float": return (float) $temp;
			case "string": return trim((string) $temp);
			case "array": return (is_array($temp) ?  $temp : NULL);
			case "object": return (is_object($temp) ?  $temp : NULL);
			default: return trim((string) $temp); //default string
		}

	}

	public static function GrabGoodInput($origin, $name, $type='')
	{
		$val = self::GrabInput($origin, $name, $type);
		if ( $val != NULL && strpos($val, '<') !== FALSE )	{
			$val = NULL;
		}

		return $val;
	}

	public static function GrabGoodInputWithReset($origin, $name, $type='')
	{
		$val = self::GrabInput($origin, $name, $type);
		if ( $val != NULL && strpos($val, '<') !== FALSE )
		{
			switch(strtoupper($origin)) {
				case "REQUEST":
				case "ANY":	$_REQUEST[$name] = NULL;
				break;
				case "GET": $_GET[$name] = NULL;
				break;
				case "POST": $_POST[$name] = NULL;
				break;
				case "COOKIE": $_COOKIE[$name] = NULL;
				break;
				case "FILE": $_FILES[$name] = NULL;
				break;
				case "SERVER": $_SERVER[$name] = NULL;
				break;
			}
			$val = NULL;
		}
		return $val;
	}

}
