<?php

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
	"icon" => "fa-home",
	"label_htm" => "<span>Add your custom label/badge html here</span>",
	"sub" => [] //contains array of sub items with the same format as the parent
]

*/

$page_nav = [
		'dashboard' => [
				'title' => DMsg::UIStr('menu_dashboard'),
				'url' => '#view/dashboard.php',
				'icon' => 'fa-home'],
		'serv' => [
				'title' => DMsg::UIStr('menu_serv'),
				'icon' => 'fa-globe',
				'url' => '#view/confMgr.php?m=serv'],
		'sl' => [
				'title' => DMsg::UIStr('menu_sl'),
				'icon' => 'fa-chain',
				'url' => '#view/confMgr.php?m=sl'],
		'vh' => [
				'title' => DMsg::UIStr('menu_vh'),
				'icon' => 'fa-cubes',
				'url' => '#view/confMgr.php?m=vh'],
		'tp' => [
				'title' => DMsg::UIStr('menu_tp'),
				'url' => '#view/confMgr.php?m=tp',
				'icon' => 'fa-files-o'],
		'tools' => [
				'title' => DMsg::UIStr('menu_tools'),
				'icon' => 'fa-th',
				'sub' => [
						'buildphp' => [
								'title' => DMsg::UIStr('menu_compilephp'),
								'url' => '#view/compilePHP.php'],
						'logviewer' => [
								'title' => DMsg::UIStr('menu_logviewer'),
								'url' => '#view/logviewer.php'],
						'stats' => [
								'title' => DMsg::UIStr('menu_rtstats'),
								'url' => '#view/realtimestats.php'],
				]
		],
		'webadmin' => [
				'title' => DMsg::UIStr('menu_webadmin'),
				'icon' => 'fa-gear',
				'sub' => [
						'lg' => [
								'title' => DMsg::UIStr('menu_general'),
								'url' => '#view/confMgr.php?m=admin'],
						'al' => [
								'title' => DMsg::UIStr('menu_sl'),
								'url' => '#view/confMgr.php?m=al']
				]
		],
		'help' => [
				'title' => DMsg::UIStr('menu_help'),
				'icon' => 'fa-book',
				'sub' => [
						'docs' => [
								'title' => DMsg::UIStr('menu_docs'),
								'url_target' => '_blank',
								'url' => DMsg::DocsUrl()],
						'guides' => [
								'title' => DMsg::UIStr('menu_guides'),
								'url' => 'https://openlitespeed.org/kb/?utm_source=Open&utm_medium=WebAdmin',
								'url_target' => '_blank'],
						'devgroup' => [
								'title' => DMsg::UIStr('menu_devgroup'),
								'url' => 'https://groups.google.com/forum/#!forum/openlitespeed-development',
								'url_target' => '_blank'],
						'releaselog' => [
								'title' => DMsg::UIStr('menu_releaselog'),
								'url' => 'https://openlitespeed.org/release-log/?utm_source=Open&utm_medium=WebAdmin',
								'url_target' => '_blank'],
						'forum' => [
								'title' => DMsg::UIStr('menu_forum'),
								'url' => 'https://forum.openlitespeed.org/?utm_source=Open&utm_medium=WebAdmin',
								'url_target' => '_blank'],
						'slack' => [
								'title' => DMsg::UIStr('menu_slack'),
								'url' => 'https://www.litespeedtech.com/slack',
								'url_target' => '_blank'],
						'cloudimage' => [
								'title' => DMsg::UIStr('menu_cloudimage'),
								'url' => 'https://docs.litespeedtech.com/cloud/images/?utm_source=Open&utm_medium=WebAdmin',
								'url_target' => '_blank'],
				]
		]

];

//configuration variables
$page_title = '';
$no_main_header = false; //set true for lock.php and login.php

$footer_lic_info = '
		Open LiteSpeed is an open source HTTP server.
				Copyright (C) 2013-2024  Lite Speed Technologies, Inc.

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
