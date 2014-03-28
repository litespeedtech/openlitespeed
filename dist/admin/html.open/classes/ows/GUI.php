<?php

class GUI extends GUIBase
{

	public static function top_menu($title = NULL)
	{

		$dropdown = '<ul>
    <li><a href="/" class="mainlevel">Home</a></li>
    <li><a href="/service/serviceMgr.php" class="mainlevel">Actions</a>
      <ul>
 	<li><a href="javascript:go(\'restart\',\'\');">Graceful Restart</a></li>
	<li><a href="javascript:toggle();">Toggle Debug Logging</a></li>
	<li><a href="/service/serviceMgr.php?vl=1">Server Log Viewer</a></li>
	<li><a href="/service/serviceMgr.php?vl=2">Real-Time Stats</a></li>
	<li><a href="/utility/build_php/buildPHP.php">Compile PHP</a></li>
      </ul>
    </li>
    <li><a href="/config/confMgr.php?m=serv"  class="mainlevel">Configuration</a>
      <ul>
      	<li><a href="/config/confMgr.php?m=serv">Server</a></li>
        <li><a href="/config/confMgr.php?m=sltop">Listeners</a></li>
        <li><a href="/config/confMgr.php?m=vhtop">Virtual Hosts</a></li>
        <li><a href="/config/confMgr.php?m=tptop">Virtual Host Templates</a></li>
      </ul>
    </li>
    <li><a href="/config/confMgr.php?m=admin" class="mainlevel">Web Console</a>
      <ul>
        <li><a href="/config/confMgr.php?m=admin">General</a></li>
        <li><a href="/config/confMgr.php?m=altop">Listeners</a></li>
      </ul>
    </li>
    <li><a href="/docs/"  class="mainlevel" target=_new>Help</a></li>
  </ul>';

		$buf = parent::gen_top_menu($dropdown);
		return $buf;
	}

	public static function footer()
	{
		$year = '2013-2014';
		$extra = '
    Open LiteSpeed is an open source HTTP server.
    Copyright (C) 2013-2014  Lite Speed Technologies, Inc.

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
		$buf = parent::gen_footer($year, $extra);
		return $buf;
	}

	public static function left_menu()
	{
		$confCenter = ConfCenter::singleton();

		$conftype = $confCenter->_confType;

		if($conftype == "admin") {

			$listeners = count($confCenter->_serv->_data['listeners']);

			$tabs = array('Admin' => 'admin', "Listeners ($listeners)" => 'altop');
		}
		else {

			$vhosts = count($confCenter->_info['VHS']);
			$listeners = count($confCenter->_info['LNS']);
			$templates = isset($confCenter->_serv->_data['tpTop']) ? count($confCenter->_serv->_data['tpTop']) : 0;

			$tabs = array('Server' => 'serv', "Listeners ($listeners)" => 'sltop',
					"Virtual Hosts ($vhosts)" => 'vhtop', "Virtual Host Templates ($templates)" => 'tptop');
		}

		return parent::gen_left_menu($tabs);
	}



}

