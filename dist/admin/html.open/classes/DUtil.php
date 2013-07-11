<?php

class DUtil
{
	
	public static function object_sort(&$array_data, $column, $direction, $type) {
                // get a list of columns
                $columns = array();

                foreach ($array_data as $key => $row) {
                   $columns[$key]  = $row->$column;

                }
                
                array_multisort($columns, $direction, $type, $array_data);
	}

	public static function grab_input($origin = "",$name = "",$type = "") {
		global $_REQUEST, $_COOKIE, $_GET, $_POST;
		$temp = NULL;

		$origin = strtoupper($origin);

		if($name == "" || $origin == "") {
			die("input error");
		}

		switch($origin) {
			case "REQUEST":
			case "ANY":
			$temp = $_REQUEST;
			break;
			case "GET":
			$temp = $_GET;
			break;
			case "POST":
			$temp = $_POST;
			break;
			case "COOKIE":
			$temp = $_COOKIE;
			break;
			case "FILE":
			$temp = $_FILES;
			break;
			case "SERVER":
			$temp = $_SERVER;
			break;
			default:
			die("input extract error.");
			break;
		}


		if(array_key_exists($name,$temp)) {
			$temp =  $temp[$name];
		}
		else {
			$temp = NULL;
		}

		switch($type) {
			case "int":	
				return (int) $temp;
				break;
			case "float":
				return (float) $temp;
				break;
			case "string":
				return trim((string) $temp);
				break;
			case "array":
				return (is_array($temp) ?  $temp : NULL);
				break;
			case "object":
				return (is_object($temp) ?  $temp : NULL);
				break;
			default:
				return trim((string) $temp); //default string
				break;

		}

	}

	public static function genOptions($options, $selValue)
	{
		$o = '';
		if ( $options )
		{
			foreach ( $options as $key => $value )
			{
				$o .= '<option value="' . $key .'"';
				if ( $key == $selValue )
					$o .= ' selected';
				$o .= ">$value</option>\n";
			}
		}
		return $o;
	}	

	public static function getGoodVal($val)
	{
		if ( $val != NULL && strpos($val, '<') !== FALSE )
		{
			return NULL;
		}
		else
			return $val;
	}

	public static function getGoodVal1(&$val)
	{
		if ( $val != NULL && strpos($val, '<') !== FALSE )
		{
			$val = NULL;
			return NULL;
		}
		else
			return $val;
	}

	public static function getLastId($id)
	{
		if ( $id == NULL )
			return NULL;

		$pos = strrpos($id, '`');
		if ( $pos === FALSE )
			return $id;
		else
			return substr($id, $pos+1);
	}

	public static function trimLastId(&$id)
	{
		$pos = strrpos($id, '`');
		if ( $pos !== FALSE )
			$id = substr($id, 0, $pos);
		else
			$id = NULL;
	}

	public static function switchLastId(&$curId, $newId)
	{
		$pos = strrpos($curId, '`');
		if ( $pos !== FALSE )
			$curId = substr($curId, 0, $pos+1) . $newId;
		else
			$curId = $newId;
	}

	public static function &locateData0(&$holder, $dataloc, $ref=NULL)
	{
		$data = &$holder;

		if ( $dataloc != NULL )
		{
			$datalocs = explode(':', $dataloc);

			foreach ( $datalocs as $loc )
			{
				$data = &$data[$loc];
			}
			if ( $ref != NULL )
				$data = &$data[$ref];
		}
		return $data;
	}

	public static function &locateData(&$holder, $dataloc, $ref=NULL)
	{
		$data = &$holder;
		if ( $ref != NULL ) {
			$refs = explode('`', $ref);
		}

		if ( $dataloc != NULL )
		{
			$datalocs = explode(':', $dataloc);
			foreach ( $datalocs as $loc )
			{
				$r = strpos($loc, '!$');
				
				if ( $r > 0 )
				{
					$a = substr($loc, $r+2);
					$loc = substr($loc, 0, $r);
					$data = &$data[$loc][$refs[$a]];
				}
				else
					$data = &$data[$loc];
			}

		}
		return $data;
	}

	public static function getSubTid(&$subTbls, &$data)
	{
		$key = $subTbls[0];

		if ( !isset($data[$key]) )
			return NULL;
		$key = $data[$key]->GetVal();
		if ( !isset($subTbls[$key]) )
			return $subTbls[1];
		else
			return $subTbls[$key];
	}
	
	public static function splitMultiple($val)
	{
		return preg_split("/, /", $val, -1, PREG_SPLIT_NO_EMPTY);
	}	

	public static function array_string_keys($input) 
	{
		$output = array();
		foreach($input as $k => $v) {
			$output[] = (string)$k;
		}
		return $output;
			
	}
	
	public static function dbg_out($tag, &$obj)
	{
		echo "<!-- $tag --\n";
		var_dump($obj);
		echo "-->\n";
	}
	public static function dbg_tag($tag)
	{
		echo "<!-- $tag -->\n";
	}

 
}
