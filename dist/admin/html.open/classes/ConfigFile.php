<?php

class ConfigFile
{
	private $pageDef;
	private $tblDef;
	//TAG=0, VALUE=1, ELEMENTS=2

	function __construct()
	{}

	//type = ('serv','vh','admin','tp')
	function load(&$confData, &$err)
	{
		$xmltree = new XmlTreeBuilder();
		$rootNode = &$xmltree->ParseFile($confData->_path, $err);
		if ( $rootNode != NULL )
		{
			$this->pageDef = DPageDef::GetInstance();
			$this->tblDef = DTblDef::GetInstance();

			$this->load_data($rootNode, $confData);
			if ( $confData->_type == 'serv' )
				$this->fill_serv($confData);
			elseif ( $confData->_type == 'admin' )
				$this->fill_listener($confData);
			else
				$this->fill_vh($confData);

			return TRUE;
		}
		error_log($err);
		$confData->_data = array();
		return FALSE;
	}

	function save(&$confData)
	{
		$fd = fopen($confData->_path . ".new", 'w');
		if ( !$fd )
		{
			error_log("failed to open in write mode for " . $confData->_path . ".new");
			return FALSE;
		}

		if ( !isset($this->pageDef))
		{
			$this->pageDef = DPageDef::GetInstance();
			$this->tblDef = DTblDef::GetInstance();
		}

		$data = &$this->convert_data($confData);

		
		$level = 0;
		$result = '<?xml version="1.0" encoding="UTF-8"?>'."\n";
		$this->writeStruct( $result, $level, $data);
		
		if(fwrite($fd, $result) === FALSE) {
			error_log("failed to write temp config for " . $confData->_path . ".new");
			return FALSE;
		}
		fclose($fd);
		
		@unlink($confData->_path . ".bak");
		if(!rename($confData->_path, $confData->_path . ".bak")) {
			error_log("failed to rename " . $confData->_path . " to " . $confData->_path . ".bak");
			return FALSE;
		}
		
		if(!rename($confData->_path . ".new", $confData->_path)) {
			error_log("failed to rename " . $confData->_path . ".new to " . $confData->_path);
			return FALSE;
		}
	
		return TRUE;
	}

	function &convert_data($confData)
	{
		$type = $confData->_type;
		$from = $confData->_data;
		$to0 = array();

		if ( $type == 'serv' ) {
		    $client = CLIENT::singleton();
		    $t = $client->type;
		    if (  $t == 'LSWS' )
		    {
			$to = &$to0['httpServerConfig'];
		    }
		    else
		    {
			$to = &$to0['loadBalancerConfig'];
		    }
		} elseif ( $type == 'vh' ) {
			$to = &$to0['virtualHostConfig'];
		} elseif ( $type == 'tp' ) {
			$to = &$to0['virtualHostTemplate'];
		} elseif ( $type == 'admin' ) {
			$to = &$to0['adminConfig'];
		}

		$pages = $this->pageDef->GetFileDef($type);
		$num = count($pages);
		for ( $c = 0 ; $c < $num ; ++$c )
		{
			$page = $pages[$c];
			if ( $page->GetLayerId() == NULL ){
				$this->convertStruct( $from, $to, $page->GetTids() );
			}
			else {
				$from1 = &DUtil::locateData( $from, $page->GetDataLoc() );
				if ( $from1 == NULL || count($from1) == 0 ) {
					continue;
				}

				$to1 = &DUtil::locateData( $to, $page->GetLayerId() );

				if ( $page->GetHolderIndex() == NULL )	{
					$this->convertStruct( $from1, $to1, $page->GetTids() );
				} else {
					$keys = array_keys($from1);
					foreach( $keys as $key ) {
						$to2 = array();
						$this->convertStruct( $from1[$key], $to2, $page->GetTids() );
						$to1[] = $to2;
					}
				}
			}
		}
		return $to0;
	}

	function load_data(&$rootNode, &$confData)
	{

		$holder = &$confData->_data;
		$el = &$rootNode[2];
		$pages = $this->pageDef->GetFileDef($confData->_type);
		$num = count($pages);
		for ( $c = 0 ; $c < $num ; ++$c )
		{
			$page = &$pages[$c];
			if ( $page->GetLayerId() == NULL )
				$this->extractSection( $el, $holder, $page->GetTids() );
			else
			{
				$isRepeated = ($page->GetHolderIndex() != NULL);

				$els = &$this->locateXmlElement( $el, $page->GetLayerId(), $isRepeated );
				if ( count($els) == 0 )
					continue;

				$holder1 = &DUtil::locateData( $holder, $page->GetDataLoc() );
				if ( !$isRepeated )
				{
					$this->extractSection( $els[0], $holder1, $page->GetTids() );
				}
				else
				{
					$innercount = count($els);
					for ( $j = 0 ; $j < $innercount ; ++$j )
					{
						$holder2 = array();
						$this->extractSection( $els[$j], $holder2, $page->GetTids() );
						$holder1[$holder2[$page->GetHolderIndex()]->GetVal()] = $holder2;
					}
				}
			}
		}
	}

	function fill_serv(&$confData)
	{
		$holder = &$confData->_data;
		$runningAs = 'user('. $holder['general']['user']->GetVal() .
			') : group(' . $holder['general']['group']->GetVal() .')' ;
		$holder['general']['runningAs'] = new CVal($runningAs);

		$this->fill_listener($confData);
	}

	function fill_listener(&$confData)
	{
		$listeners = &$confData->_data['listeners'];

		if ( $listeners == NULL || count($listeners) == 0 )
			return;
		$lnames = array_keys($listeners);
		foreach ( $lnames as $ln )
		{
			$l = &$listeners[$ln];
			$addr = $l['address']->GetVal();
			$pos = strrpos($addr,':');
			if ( $pos )
			{
				$ip = substr($addr, 0, $pos);
				if ( $ip == '*' )
					$ip = 'ANY';
				$l['ip'] = new CVal($ip);
				$l['port'] = new CVal(substr($addr, $pos+1));
			}
		}
	}

	function fill_vh(&$confData)
	{
		$ctxs = &$confData->_data['context'];
		if ( $ctxs == NULL || count($ctxs) == 0 )
			return;
		$keys = array_keys($ctxs);
		$i = 1;
		foreach ( $keys as $k )
		{
			$ctxs[$k]['order'] = new CVal($i);
			++$i;
		}
	}

	function extractSection( &$el, &$holder, $tids)
	{
		foreach( $tids as $tid ) {
			$tbl = $this->tblDef->GetTblDef($tid);
			$holderIndex = $tbl->_holderIndex;

			$isRepeated = ($holderIndex != NULL);
			$els = &$this->locateXmlElement($el, $tbl->_layerId, $isRepeated);

			if ( count($els) == 0 ) continue;

			$data = &DUtil::locateData( $holder, $tbl->_dataLoc );
			$tags = $tbl->GetTags();

			foreach ($els as $e) {
				if ( $isRepeated ) {
					$data1 = array();
					$data2 = &DUtil::locateData( $data1, $tbl->_dataLoc );
					$this->extractElement('' , $tags, $e, $data2);
					if ( $tbl->_subTbls != NULL ) {
						$this->extractSubTbl($e, $data1, $tbl);
					}
					$data[ $data2[$holderIndex]->GetVal() ] = $data2;
				} else {
					$this->extractElement('' , $tags, $e, $data);
				}
			}
		}
	}

	function extractSubTbl($el, &$holder, $tbl)
	{
		$holder1 = &DUtil::locateData( $holder, $tbl->_dataLoc );
		$tid = DUtil::getSubTid($tbl->_subTbls, $holder1);
		if ( $tid == NULL )
			return;

		$tids = array($tid);
		$holder2 = array();
		$this->extractSection($el, $holder2, $tids);

		$holder3 = &DUtil::locateData( $holder2, $tbl->_dataLoc );
		$keys = array_keys($holder3);
		foreach( $holder3[$keys[0]] as $key=>$val )
		{
			$holder1[$key] = $val;
		}
	}

	function &locateXmlElement(&$el, $layerId, $isRepeated)
	{
		$els = array();
		if ( $layerId == NULL )
		{
			$els[] = &$el;
		}
		else
		{
			$layerIds = explode(':', $layerId);
			$depth = count($layerIds);
			$el1 = &$el;

			for ( $layer = 0 ; $layer < $depth ; ++$layer )
			{
				$innercount = count($el1);
				$tag = $layerIds[$layer];

				for ( $i = 0 ; $i < $innercount ; ++$i )
				{
					if ( $el1[$i][0] == $tag )
					{
						if ( ($layer+1) == $depth )
						{
							$els[] = &$el1[$i][2];
							if ( !$isRepeated )
								break;
						}
						else
						{
							$el1 = &$el1[$i][2];
							break;
						}
					}
				}
			}
		}

		return $els;
	}

	function extractElement($prefix, $tags, $el, &$data)
	{
		$ids = array_keys($tags);

		foreach ($el as $e) 
		{
			$name = $e[0];
			if ( in_array($name, $ids) )
			{
				if ( is_array($tags[$name]) )
				{
					$prefix1 = $prefix . $name . ':';
					$this->extractElement( $prefix1, $tags[$name], $e[2], $data );
				}
				else
				{
					$index = $prefix . $name;
					if ( $tags[$name] == 2 )
						$data[$index][] = new CVal($e[1]);
					else
						$data[$index] = new CVal($e[1]);
				}
			}
		}
	}

	
	
	function writeStruct( &$result, &$level, &$data)
	{
		$keys = array_keys($data);
		foreach( $keys as $key )
		{
			$data1 = &$data[$key];
			if ( is_array($data1) )
			{
				if ( isset($data1[0]) ) //numeric
				{
					$c = count($data1);
					for ( $i = 0 ; $i < $c ; ++$i )
					{
						if ( is_array($data1[$i]) )
						{
							$result .= $this->xmlTagS( $level, $key );
							$this->writeStruct( $result, $level, $data1[$i] );
							$result .= $this->xmlTagE( $level, $key );
						}
						else
							$result .= $this->xmlTag( $level, $key, $data1[$i] );
					}
				}
				else
				{
					$result .= $this->xmlTagS( $level, $key );
					$this->writeStruct( $result, $level, $data1 );
					$result .=  $this->xmlTagE( $level, $key );
				}
			}
			else
			{
				if ( isset($data1) )
					$result .= $this->xmlTag( $level, $key, $data1 );
			}
		}
	}

	function convertStruct( &$from, &$to, $tids )
	{
		foreach( $tids as $tid ) {
			
			$tbl = $this->tblDef->GetTblDef($tid);
			$fholder = &DUtil::locateData($from, $tbl->_dataLoc);
			
			if ( !isset($fholder) )	continue;

			$holder = &DUtil::locateData($to, $tbl->_layerId);

			if ( $tbl->_holderIndex != NULL ) {
				$fkeys = array_keys( $fholder );
				foreach ( $fkeys as $fkey )	{
					$data = array();
					$from1 = &$fholder[$fkey];

					$this->convertOneLevel($from1, $data, $tbl->_dattrs);

					if ( $tbl->_subTbls != NULL ) {
						$this->convertSubTbl($from1, $data, $tbl);
					}

					$holder[] = $data;
				}
			} else {
				$this->convertOneLevel($fholder, $holder, $tbl->_dattrs);
			}

		}
	}

	function convertSubTbl($from, &$to, $tbl)
	{
		$tid1 = DUtil::getSubTid($tbl->_subTbls, $from);
		if ( $tid1 == NULL ) return;
		$tbl1 = $this->tblDef->GetTblDef($tid1);
		$this->convertOneLevel($from, $to, $tbl1->_dattrs);
	}

	function convertOneLevel($from, &$to, $attrs)
	{
		foreach ( $attrs as $attr )
		{
			if ( $attr->_type != 'action' && $attr->_FDE[0] == 'Y' )
			{
				$this->convertCopy($from, $to, $attr);
			}
		}
	}

	function convertCopy( $from, &$to, $attr )
	{
		$attr_key = $attr->_key;
		if ( !isset($from[$attr_key]) && $attr->_allowNull ) {
			return;
		}

		$prepared = NULL;
		if (isset($from[$attr_key]) && is_array($from[$attr_key]) )	{
			foreach ( $from[$attr_key] as $i ) {
				$prepared[] = $i->GetVal();
			}
		}
		elseif (isset($from[$attr_key]) && $from[$attr_key]->HasVal()) {
			$prepared = $from[$attr_key]->GetVal();
		}

		if ( strpos($attr_key, ':') === FALSE ) {
			$to[$attr_key] = $prepared;
		} else {
			$to1 = &DUtil::locateData($to, $attr_key);
			$to1 = $prepared;
		}

	}

	function xmlTag($level, $tag, $value)
	{
		$val = htmlspecialchars($value);
		return str_repeat('  ', $level) . "<$tag>$val</$tag>\n";
	}

	function xmlTagS(&$level, $tag)
	{
		return str_repeat('  ', $level++) . "<$tag>\n";
	}

	function xmlTagE(&$level, $tag)
	{
		return str_repeat('  ', --$level) . "</$tag>\n";
	}


}

