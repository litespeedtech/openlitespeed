<?

echo GUI::header("{$confCenter->_serv->_id}: $page->_title");
echo GUI::top_menu();
echo GUI::left_menu();
echo '<form name="confform" method="post" action="confMgr.php">'; // to avoid warning, put in echo
?>

<div style='height:10px;'></div>

<? 

if(strlen($disp->_titleLabel)) { 
	echo  '<div style="width:100%;"><h2>'.$disp->_titleLabel . '</h2></div>'."\n"; 
} 

echo '<div style="padding-left:10px;">'; // avoid warning, put in echo

?>

<!-- START TABS -->
<div id="xtab" >
<ul>
<?
$uri = $disp->_ctrlUrl . 'm=' . $disp->_mid .'&p=';


foreach ( $disp->_tabs as $pid => $tabName )
{
	$class= "";

	if ( $page->_id == $pid )
	{
		$class = "on";

	}

	echo "\t<li class='{$class}'><div><a href='{$uri}{$pid}'>{$tabName}</a></div></li>\n";

}

?>

</ul>
</div>
<!-- END TABS -->
<br>


