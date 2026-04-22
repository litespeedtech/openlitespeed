<?php

use LSWebAdmin\I18n\DMsg;

//ribbon breadcrumbs config
//["Display Name" => "URL"];
$breadcrumbs = [
	//"Home" => '/'
];

/*navigation array config

ex:
"dashboard" => [
	"title" => "Display Title",
	"url" => "http://yoururl.com",
	"url_target" => "_self",
	"icon" => "home",
	"label_htm" => "<span>Add your custom label/badge html here</span>",
	"sub" => [] //contains array of sub items with the same format as the parent
]

*/

$page_nav = [
		'dashboard' => [
				'title' => DMsg::UIStr('menu_dashboard'),
				'url' => 'index.php?view=dashboard',
				'icon' => 'layout-dashboard'],
		'serv' => [
				'title' => DMsg::UIStr('menu_serv'),
				'icon' => 'server-cog',
				'url' => 'index.php?view=confMgr&m=serv'],
		'sl' => [
				'title' => DMsg::UIStr('menu_sl'),
				'icon' => 'plug-zap',
				'url' => 'index.php?view=confMgr&m=sl'],
		'vh' => [
				'title' => DMsg::UIStr('menu_vh'),
				'icon' => 'server',
				'url' => 'index.php?view=confMgr&m=vh'],
		'tp' => [
				'title' => DMsg::UIStr('menu_tp'),
				'url' => 'index.php?view=confMgr&m=tp',
				'icon' => 'copy'],
		'livestats' => [
				'title' => DMsg::UIStr('menu_livestats'),
				'icon' => 'activity-square',
				'sub' => [
						'stats' => [
								'title' => DMsg::UIStr('menu_rtstats'),
								'url' => 'index.php?view=realtimestats'],
						'listenervhmap' => [
								'title' => DMsg::UIStr('service_listenervhmap'),
								'url' => 'index.php?view=listenervhmap'],
						'blockedips' => [
								'title' => DMsg::UIStr('service_blockedips'),
								'url' => 'index.php?view=blockedips'],
				]
		],
		'tools' => [
				'title' => DMsg::UIStr('menu_tools'),
				'icon' => 'wrench',
				'sub' => [
						'logviewer' => [
								'title' => DMsg::UIStr('menu_logviewer'),
								'url' => 'index.php?view=logviewer'],
					'loginhistory' => [
							'title' => DMsg::UIStr('menu_loginhistory'),
							'url' => 'index.php?view=loginhistory'],
					'opsauditlog' => [
							'title' => DMsg::UIStr('menu_opsauditlog'),
							'url' => 'index.php?view=opsauditlog'],
					'buildphp' => [
								'title' => DMsg::UIStr('menu_compilephp'),
								'url' => 'index.php?view=compilePHP'],
				]
		],
		'webadmin' => [
				'title' => DMsg::UIStr('menu_webadmin'),
				'icon' => 'settings',
				'sub' => [
						'lg' => [
								'title' => DMsg::UIStr('menu_general'),
								'url' => 'index.php?view=confMgr&m=admin'],
						'al' => [
								'title' => DMsg::UIStr('menu_sl'),
								'url' => 'index.php?view=confMgr&m=al']
				]
		],
		'help' => \LSWebAdmin\Product\Current\UI::GetHelpMenu()

];

//configuration variables
$page_title = '';
$no_main_header = false; //set true for lock.php and login.php

$footer_lic_info = '
		Open LiteSpeed is an open source HTTP server.
				Copyright (C) 2013-' . date('Y') . '  Lite Speed Technologies, Inc.

				This program is free software: you can redistribute it and/or modify
				it under the terms of the GNU General Public License as published by
				the Free Software Foundation, either version 3 of the License, or
				(at your option) any later version.

				This program is distributed in the hope that it will be useful,
				but WITHOUT ANY WARRANTY; without even the implied warranty of
				MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
				GNU General Public License for more details.

				You should have received a copy of the GNU General Public License
				along with this program.  If not, see http://www.gnu.org/licenses/ .

		';
