<?php

class ConfigFileEx
{

	// grep logging.log.fileName, no need from root tag, any distinctive point will do
	public static function grepTagValue($filename, $tags)
	{
		$contents = file_get_contents($filename);
		if (is_array($tags))
		{
			$values = array();
			foreach ($tags as $tag)
			{
				$values[$tag] = ConfigFileEx::grepSingleTagValue($tag, $contents);
			}
			return $values;
		}
		
		return ConfigFileEx::grepSingleTagValue($tags, $contents);
	}

	public static function grepSingleTagValue($tag, &$contents)
	{
		$singleTags = explode('.', $tag);
		$cur_pos = 0;
		$end_tag = '';
		foreach($singleTags as $singletag)
		{
			$findtag = "<$singletag>";
			$cur_pos = strpos($contents, $findtag, $cur_pos);
			if ($cur_pos === FALSE)
				break;
			$cur_pos += strlen($findtag);
			$end_tag = "</$singletag>";
		}
		if(!strlen($contents) || !strlen($end_tag)) {
			$last_pos = FALSE;
		}
		else {
			$last_pos = strpos($contents, $end_tag, $cur_pos);
		}
		
		if ( $last_pos !== FALSE) {
			return substr($contents, $cur_pos, $last_pos - $cur_pos);
		}
		else
			return NULL;
	}

	// other files
	public static function &loadMime($filename)
	{
		$lines = file($filename);
		if ( $lines == FALSE ) {
			return FALSE;
		}

		$mime = array();
		foreach( $lines as $line )
		{
			$c = strpos($line, '=');
			if ( $c > 0 )
			{
				$suffix = trim(substr($line, 0, $c));
				$type = trim(substr($line, $c+1 ));
				$mime[$suffix] = array('suffix' => new CVal($suffix),
										'type' => new CVal($type));
			}
		}
		ksort($mime, SORT_STRING);
		reset($mime);

		return $mime;
	}

	public static function saveMime($filename, &$mime)
	{
		$fd = fopen($filename, 'w');
		if ( !$fd ) {
			return FALSE;
		}
		ksort($mime, SORT_STRING);
		reset($mime);
		foreach( $mime as $key => $entry )
		{
			if ( strlen($key) < 8 ) {
				$key = substr($key . '        ', 0, 8);
			}
			$line = "$key = " . $entry['type']->GetVal() . "\n";
			fputs( $fd, $line );
		}
		fclose($fd);

		return TRUE;
	}

	public static function &loadUserDB($filename)
	{
		if ( PathTool::isDenied($filename) ) {
			return FALSE;
		}

		$lines = file($filename);
		$udb = array();
		if ( $lines == FALSE ) {
			error_log('failed to read from ' . $filename);
			return $udb;
		}

		foreach( $lines as $line )
		{
			$line = trim($line);

			$parsed = explode(":",$line);

			if(is_array($parsed)) {

				$size = count($parsed);

				if($size != 2 && $size !=3) {
					continue;
				}

				if(!strlen($parsed[0]) || !strlen($parsed[1])) {
					continue;
				}

				$user = array();

				if($size >= 2) {
					$user['name'] = new CVal(trim($parsed[0]));
					$user['passwd'] = new CVal(trim($parsed[1]));
				}

				if($size == 3 && strlen($parsed[2])) {
					$user['group'] = new CVal(trim($parsed[2]));
				}

				$udb[$user['name']->GetVal()] = $user;
			}
		}

		ksort($udb);
		reset($udb);
		return $udb;
	}

	public static function saveUserDB($filename, &$udb)
	{
		if ( PathTool::isDenied($filename) ) {
			return FALSE;
		}

		$fd = fopen($filename, 'w');
		if ( !$fd ) {
			return FALSE;
		}

		ksort($udb);
		reset($udb);
		foreach( $udb as $name => $user ) {
			$line = $name . ':' . $user['passwd']->GetVal();
			if (isset($user['group']) ) {
				$line .= ':' . $user['group']->GetVal();
			}
			fputs( $fd, "$line\n" );
		}
		fclose($fd);
		return TRUE;
	}

	public static function &loadGroupDB($filename)
	{
		if ( PathTool::isDenied($filename) ) {
			return FALSE;
		}

		$gdb = array();
		$lines = file($filename);
		if ( $lines == FALSE ) {
			return $gdb;
		}

		foreach( $lines as $line ) {
			$line = trim($line);

			$parsed = explode(':', $line);

			if(is_array($parsed) && count($parsed) == 2) {
				$nameval = trim($parsed[0]);
				$group = array(
					'name' => new CVal($nameval),
					'users' => new CVal(trim($parsed[1])));
				$gdb[$nameval] = $group;
			}
		}
		ksort($gdb);
		reset($gdb);
		return $gdb;
	}

	public static function saveGroupDB($filename, &$gdb)
	{
		if ( PathTool::isDenied($filename) ) {
			return FALSE;
		}
		$fd = fopen($filename, 'w');
		if ( !$fd ) {
			return FALSE;
		}
		ksort($gdb);
		reset($gdb);
		foreach( $gdb as $name => $entry )
		{
			$line = $name . ':' . $entry['users']->GetVal() . "\n";
			fputs( $fd, $line );
		}
		fclose($fd);
		return TRUE;
	}

}
