<?php

require_once('../includes/auth.php');

$disp = ConfControl::GetDInfo();

$buf = GUI::top_menu();
$buf .= $disp->GetTabs();

echo $buf;

$page = DPageDef::GetPage($disp);
$page->PrintHtml($disp);

$buf = $disp->PrintEndForm();
$buf .= GUI::footer();
echo $buf;
