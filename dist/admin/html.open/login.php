<?

require_once('includes/global.php');

require_once('blowfish.php');


$client = CLIENT::singleton();
$is_https = (isset($_SERVER['HTTPS']) && ($_SERVER['HTTPS'] == 'on'));

if(isset($_GET['generateKeypair'])) {
	$keyLength = 512;
	$cc = ConfCenter::singleton();
	$keyfile = $cc->GetConfFilePath('admin', 'key');
	$mykeys = null;
	if (file_exists($keyfile)) {
		$str = file_get_contents($keyfile);
		if ($str != '') {
			$mykeys = unserialize($str);
		}
	}
	if ($mykeys == null) {
		$jCryption = new jCryption();
 		$keys = $jCryption->generateKeypair($keyLength);
		$e_hex = $jCryption->dec2string($keys['e'],16);
		$n_hex = $jCryption->dec2string($keys['n'],16);
		$mykeys = array( 'e_hex' => $e_hex, 'n_hex' => $n_hex, 'd_int' => $keys['d'], 'n_int' => $keys['n']);
		$serialized_str = serialize($mykeys);
		file_put_contents($keyfile, $serialized_str);
		chmod($keyfile, 0600);
	}
	$_SESSION['d_int'] = $mykeys['d_int'];
	$_SESSION['n_int'] = $mykeys['n_int'];
	
	echo '{"e":"'.$mykeys['e_hex'].'","n":"'.$mykeys['n_hex'].'","maxdigits":"'.intval($keyLength*2/16+3).'"}';
	exit;
} 


$timedout = DUtil::grab_input('get','timedout','int');
$logoff = DUtil::grab_input('get','logoff','int');


$msg = "";



if($timedout == 1 || $logoff  == 1) {
	$client->clear();

	if($timedout == 1) {
		$msg = 'Your session has timed out.';
	}
	else {
		$msg = 'You have logged off.';
	}
}


if($client->isValid()) {
	header('location:/index.php');
	exit();
}

$userid = null;
$pass = null;

if ( isset($_POST['jCryption']) )
{
	$jCryption = new jCryption();
	$var = $jCryption->decrypt($_POST['jCryption'], $_SESSION['d_int'], $_SESSION['n_int']);
	unset($_SESSION['d_int']);
	unset($_SESSION['n_int']);
	parse_str($var,$result);
	$userid = $result['userid'];
	$pass = $result['pass'];
}
else if ($is_https && isset($_POST['userid'])) {
	$userid = DUtil::grab_input('POST','userid');
	$pass = DUtil::grab_input('POST','pass');
}

if ($userid != null) {
	if ( $client->authenticate($userid, $pass) === TRUE ) {
		$temp=gettimeofday();
		$start=(int)$temp['usec'];
		$secretKey0 = mt_rand(). $start . mt_rand();
		$secretKey1 = mt_rand(). mt_rand() . $start;
	
		$client->secret = array($secretKey0, $secretKey1);
		$client->store(PMA_blowfish_encrypt($userid, $secretKey0), PMA_blowfish_encrypt($pass, $secretKey1));
		$client->updateAccessTime();
		$client->valid  = TRUE;

		header('location:/index.php');
		exit();
	}
	else {
		$msg = 'Invalid credentials.';
	}
}


echo GUI::header();

if (!$is_https) {
?>
<script type="text/javascript" src="/static/scripts/jcryption/jquery.min.js" ></script>
<script type="text/javascript" src="/static/scripts/jcryption/jquery.jcryption.js" ></script>
 
<script type="text/javascript">

$(document).ready(function() {
	
	
	$("#login").jCryption()
	
	$("input,select,textarea").removeAttr("disabled");
	
});
</script>

<? } ?>

<form id = "login" action="login.php" method="post">

<table align=center width="300" style='margin-top:150px;' class=xtbl border="0" cellpadding="6" cellspacing="1">
<tr id=top_menu><td style='background: rgb(52, 79, 115);color:white;'>Web Console</td></tr>
<tr class=xtbl_label><td align=center><img src="/static/images/logo/product_logo.gif" border="0"></td></tr>
<?
if (strlen($msg)) {
	echo "<tr class=xtbl_label align=center><td>{$msg}</td></tr>";
}
?>

<tr class=xtbl_value><td>Username<br><input id=uid type="text" style='width:98%' name="userid" size="25"></td></tr>
<tr class=xtbl_value><td>Password<br><input id=pass type="password" style='width:98%' name="pass" size="25"></td></tr>
<tr class=xtbl_value><td><input type="submit" class=button_full value="Login" style='padding:2px;background-color:white;border:1px outset black;'></td></tr>

</table>
</form>


<?



echo GUI::footer();

?>

