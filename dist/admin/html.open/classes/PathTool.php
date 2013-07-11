<?php

class PathTool
{
	public static function getAbsolutePath($root, $path)
	{
		if ( $path{-1} != '/' )
			$path .= '/';

		$newPath = $this->getAbsoluteFile($root, $path);
		return $newPath;
	}

	public static function getAbsoluteFile($root, $path)
	{
		if ( $path{0} != '/' )
			$path = $root . '/' . $path;

		$newPath = $this->clean($path);
		return $newPath;
	}

	public static function hasSymbolLink($path)
	{
		if ( $path != realpath($path) )
			return true;
		else
			return false;
	}

	public static function clean($path)
	{
		do {
			$newS1 = $path;
			$newS = str_replace('//', '/',  $path);
			$path = $newS;
		} while ( $newS != $newS1 );
		do {
			$newS1 = $path;
			$newS = str_replace('/./', '/',  $path);
			$path = $newS;
		} while ( $newS != $newS1 );
		do {
			$newS1 = $path;
			$newS = preg_replace('/\/[^\/^\.]+\/\.\.\//', '/',  $path); 
			$path = $newS;
		} while ( $newS != $newS1 );

		return $path;
	}

	public static function createFile($path, &$err, $htmlname)
	{
		if ( file_exists($path) )
		{
			if ( is_file($path) )
			{
				$err = 'Already exists '.$path;
				return false;
			}
			else
			{
				$err = 'name conflicting with an existing directory '.$path;
				return false;
			}
		}
		$dir = substr($path, 0, (strrpos($path, '/')));
		if ( PathTool::createDir($dir, 0700, $err) )
		{
			if ( touch($path) )
			{
				chmod($path, 0600);
				//populate vhconf tags
				$type = 'vh';
				if ($htmlname == 'templateFile')
					$type = 'tp';
				$newconf = new ConfData($type, $path, 'newconf');
				$config = new ConfigFile();
				$res = $config->save($newconf);
				if ( !$res )
				{
					$err = 'failed to save to file ' . $path;
					return false;
				}
				
				return true;
			}
			else
				$err = 'failed to create file '. $path;
		}

		return false;
	}

	public static function createDir($path, $mode, &$err)
	{
		if ( file_exists($path) )
		{
			if ( is_dir($path) )
				return true;
			else
			{
				$err = "$path is not a directory";
				return false;
			}
		}
		$parent = substr($path, 0, (strrpos($path, '/')));
		if ( strlen($parent) <= 1 )
		{
			$err = "invalid path: $path";
			return false;
		}
		if ( !file_exists($parent) 
			 && !PathTool::createDir($parent, $mode, $err) )
			return false;

		if ( mkdir($path, $mode) )
			return true;
		else
		{
			$err = "fail to create directory $path";
			return false;
		}
		
	}

	public static function isDenied($path)
	{
		$absname = realpath($path);
		if ( strncmp( $absname, '/etc/', 5 ) == 0 )
			return true;
		else
			return false;
	}



}

