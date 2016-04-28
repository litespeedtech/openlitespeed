<?

require_once('view/inc/global.php');

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

$no_main_header = true;
include 'view/inc/header.php';

if (!$is_https) {
?>
<script type="text/javascript" src="/res/js/jcryption/jquery.jcryption.min.js" ></script>

<script type="text/javascript">

$(document).ready(function() {
	$("#login").jCryption();
	$("input,select,textarea").removeAttr("disabled");
});
</script>

<? } ?>

<div class="container">
<div style="margin-top:50px"></div>
						<div class="col-xs-6 col-md-4 col-md-offset-4 padding-10">
						<div class="well no-padding">
							<form action="login.php"  id="login" method="post" class="smart-form client-form" novalidate="novalidate">
								<header><div class="text-center"><img src="/res/img/product_logo.gif" alt="LiteSpeed"></div></header>
								<fieldset>
								<?
if ($msg != '') {
	echo "<section><div class=\"note\">$msg</div></section>";
}
?>
								<section>
										<label class="label"><?php DMsg::EchoUIStr('l_username')?></label>
										<label class="input"> <i class="icon-append fa fa-user"></i>
											<input type="text" id="uid" name="userid" tabindex="1" required autofocus="autofocus">
											<b class="tooltip tooltip-top-right"><i class="fa fa-user txt-color-teal"></i> <?php DMsg::EchoUIStr('service_enteruser')?></b></label>
									</section>

									<section>
										<label class="label"><?php DMsg::EchoUIStr('l_pass')?></label>
										<label class="input"> <i class="icon-append fa fa-lock"></i>
											<input type="password" id="pass" name="pass"  tabindex="2" required >
											<b class="tooltip tooltip-top-right"><i class="fa fa-lock txt-color-teal"></i> <?php DMsg::EchoUIStr('service_enterpass')?></b> </label>
									</section>

								</fieldset>
								<footer>
									<button type="submit" class="btn btn-primary">
										Sign in
									</button>
								</footer>
							</form>
						</div>
					</div>
<br>
</div>
<div class="row" style="margin:40px"></div>
<div class="footer">
        <p class="text-center">Copyright &copy; 2014-2016 <a href="http://www.litespeedtech.com">LiteSpeed Technologies, Inc.</a> </p>
</div>

<?php


if (empty($_SERVER['HTTP_REFERER'])) {
	include("view/inc/scripts.php");
}
else {
?>
<script type="text/javascript">

if (typeof pageSetUp == 'function') {
	pageSetUp();

	var pagefunction = function() {
		// clears memory even if nothing is in the function
	};

	// end pagefunction

	// run pagefunction on load
	pagefunction();
}

</script>
<?php
}
?>


</body>
</html>
