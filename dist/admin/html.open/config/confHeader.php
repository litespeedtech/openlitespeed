<?php

$buf = GUI::header("{$confCenter->_serv->_id}: $page->_title");
$buf .= GUI::top_menu();
$buf .= GUI::left_menu();

$buf .= '<form name="confform" method="post" action="confMgr.php">'; // to avoid warning, put in echo

if(strlen($disp->_titleLabel)) {
	$buf .= '<div><h2>'.$disp->_titleLabel . '</h2></div>'."\n";
}

$buf .= '<div>
<!-- START TABS -->
<div class="xtab" ><ul>';

$uri = $disp->_ctrlUrl . 'm=' . $disp->_mid . '&p=';

foreach ( $disp->_tabs as $pid => $tabName )
{
	$on = '';
	if ( $page->_id == $pid ){
		$on = 'class="on"';
	}

	$buf .= "<li $on><a href='{$uri}{$pid}'>{$tabName}</a></li>";

}

$buf .= '</ul></div>
<!-- END TABS -->
';

echo $buf;

