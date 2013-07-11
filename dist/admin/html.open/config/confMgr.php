<?php

require_once('../includes/auth.php');

$confCenter = ConfCenter::singleton();

$disp = $confCenter->GetDispInfo();
$page = DPageDef::GetInstance()->GetPageDef($disp->_type, $disp->_pid);
$confErr = NULL;
$pageData = $confCenter->GetPageData($disp, $confErr);

require_once('confHeader.php');

if ( $confErr == NULL ) {
	$page->PrintHtml($pageData, $disp);
}
else {
	echo '<div class="message_error">' . $confErr . '</div>';
}

require_once('confFooter.php');
