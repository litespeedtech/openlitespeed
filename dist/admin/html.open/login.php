<?

require_once('includes/global.php');

$authorizer = CAuthorizer::singleton();

if(isset($_GET['generateKeypair'])) {
	$keypair = $authorizer->GenKeyPair();
	echo $keypair;
	exit;
}

$is_https = (isset($_SERVER['HTTPS']) && ($_SERVER['HTTPS'] == 'on'));
if (!$authorizer->ShowLogin($is_https, $msg)) {
	header('location:/index.php');
	exit();
}

echo GUI::header();

if (!$is_https) {
?>
<script type="text/javascript" src="/static/scripts/jcryption/jquery.min.js" ></script>
<script type="text/javascript" src="/static/scripts/jcryption/jquery.jcryption.js" ></script>

<script type="text/javascript">

$(document).ready(function() {
	$("#login").jCryption();
	$("input,select,textarea").removeAttr("disabled");
});
</script>

<? } ?>

<form id = "login" action="login.php" method="post">

<table align=center width="300px" style="margin-top:150px;background-color:#859bb1" border="0" cellpadding="10" cellspacing="1">
<tr id=top_menu><td style="font-weight:bold;color:white;">WebAdmin Console</td></tr>
<tr class=xtbl_label><td align=center><img src="/static/images/logo/product_logo.gif" border="0"></td></tr>
<?
if ($msg != '') {
	echo "<tr class=xtbl_label align=center><td>{$msg}</td></tr>";
}
?>

<tr class=xtbl_value><td>User Name<br><input id="uid" type="text" tabindex="1" required autofocus="autofocus" style="width:100%" name="userid"></td></tr>
<tr class=xtbl_value><td>Password<br><input id="pass" type="password" tabindex="1" required style="width:100%" name="pass"></td></tr>
<tr class=xtbl_value><td><input type="submit" style="width:100%" value="Login"></td></tr>

</table>
</form>
<br>
<br>

<?
echo GUI::footer();
?>
