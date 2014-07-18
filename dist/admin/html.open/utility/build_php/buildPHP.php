<?php

require_once('../../includes/auth.php');

include_once( 'buildconf.inc.php' );

if (!CAuthorizer::HasSetTimeout()) {
	$control = ConfControl::singleton(); //will set timeout
}


echo GUI::top_menu();

$check = new BuildCheck();

switch($check->GetNextStep()) {

	case "1":
		include("buildStep1.php");
		break;
	case "2":
		include("buildStep2.php");
		break;
	case "3":
		include("buildStep3.php");
		break;
	case "4":
		include("buildStep4.php");
		break;

	case "0":
	default: // illegal
		echo "ERROR";
}

echo GUI::footer();
