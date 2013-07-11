<?php

class XmlTreeBuilder
{
	private $parser;
	private $node_stack;
	private $cur_node;
	private $cur_val;
	//TAG=0, VALUE=1, ELEMENTS=2

	public function &ParseFile($filename, &$err)
	{
		if ( !is_file($filename) )
		{
			$err = $filename . ' is not a valid file.';
			return NULL;
		}
		$fd = fopen($filename, 'r');
		if ( !$fd )
		{
			$err = 'failed to open file ' . $filename;
			return NULL;
		}
		$len = filesize($filename);
		if ( $len > 0 )
		{
			$contents = fread($fd, $len);
			fclose($fd);
			$root = $this->parseString($contents, $err);
		}
		else
		{
			$root = array();
			fclose($fd);
		}
		
		if ( $err != NULL )
		{
			$err .= '<br> Failed to parse file ' . $filename;
			return NULL;
		}
		
		return $root;
	}

	private function parseString($xmlstring, &$err)
	{
		$this->parser = xml_parser_create();
		xml_set_object($this->parser, $this);
		xml_parser_set_option($this->parser, XML_OPTION_CASE_FOLDING, false);
		xml_set_element_handler($this->parser, 'startElement', 'endElement');
		xml_set_character_data_handler($this->parser, 'characterData');

		// Build a Root node and initialize the node_stack...
		$this->node_stack = array();
		$this->cur_node = NULL;
		$this->startElement(null, 'root', array());

		// parse the data and free the parser...
		$result = xml_parse($this->parser, $xmlstring);
		if ( !$result )
		{
			$err = 'XML error: '. xml_error_string(xml_get_error_code($this->parser))
				. ' at line ' . xml_get_current_line_number($this->parser);
		}
		xml_parser_free($this->parser);
		if ( !$result )
			return NULL;

		// return the root node...

		return $this->cur_node[2][0];
	}

	private function startElement($parser, $name, $attrs)
	{
		if ( $this->cur_node != NULL )
		{
			$this->node_stack[] = $this->cur_node;
		}
		$this->cur_node = array();
		$this->cur_node[0] = $name; //'TAG'=0

//		foreach ($attrs as $key => $value) 
//		{
//			$node[$key] = $value;
//		}  //no attributes used in xml

		$this->cur_val = '';
		$this->cur_node[2] = array();//'ELEMENTS'=2

	}

	private function endElement($parser, $name)
	{
		$this->cur_node[1] = trim($this->cur_val);//'VALUE'=1
		$node = $this->cur_node;
		// and add it an element of the last node in the stack...
		$this->cur_node = array_pop($this->node_stack);
		$this->cur_node[2][] = $node;

	}

	private function characterData($parser, $data)
	{
		// add this data to the last node in the stack...
		$this->cur_val .= $data;
	}

	private function xmlNodeOutput(&$node, $space)
	{
		$output = str_repeat('  ', $space);
		$attrs = array();
		foreach( array_keys($node) as $key )
		{
			if ( $key == 0 ) //'TAG'
			{
				$name = &$node[0];
			}
			elseif ( $key == 1 ) //'VALUE'
			{
				$data = &$node[1];
			}
			elseif ( $key == 2 ) //'ELEMENTS'
			{
				$elements = &$node[2];
			}
			else
			{
				$attrs[$key] = $node[$key];
			}

		}
		$name = $node[0]; //'TAG'
		$output .= "<$name";
		if ( count($attrs) > 0 )
		{
			foreach( $attrs as $key => $value )
			{
				$output .= " $key=\"$value\"";
			}
		}
		$output .= '>';

		if ( count($elements) == 0 )
		{
			$output .= "$data</$name>\n";
		}
		else
		{
			$output .= "\n";
			if ( $data )
			{
				$output .= str_repeat('  ', $space+1);
				$output .= $data . "\n";
			}

			foreach( array_keys($elements) as $key )
			{
				$output .= $this->xmlNodeOutput($elements[$key], $space+1);
			}

			$output .= str_repeat('  ', $space);
			$output .= "</$name>\n";

		}
		return $output ;
	}


}
