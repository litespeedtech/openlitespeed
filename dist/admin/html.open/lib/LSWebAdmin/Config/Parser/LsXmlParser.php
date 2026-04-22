<?php

namespace LSWebAdmin\Config\Parser;

use LSWebAdmin\Config\CNode;

class LsXmlParser
{
    private $node_stack;
    private $cur_node;
    private $cur_val;

    public function Parse($filename)
    {
        $root = new CNode(CNode::K_ROOT, $filename, CNode::T_ROOT);

        $filename = $root->Get(CNode::FLD_VAL);
        $xmlstring = file_get_contents($filename);
        if ($xmlstring === false) {
            $root->SetErr("failed to read file $filename", CNode::E_FATAL);
            return $root;
        }

        $parser = xml_parser_create();
        xml_set_object($parser, $this);
        xml_parser_set_option($parser, XML_OPTION_CASE_FOLDING, false);
        xml_set_element_handler($parser, 'startElement', 'endElement');
        xml_set_character_data_handler($parser, 'characterData');

        $this->node_stack = [];
        $this->cur_node = $root;

        $result = xml_parse($parser, $xmlstring);
        if (!$result) {
            $err = 'XML error: ' . xml_error_string(xml_get_error_code($parser))
                . ' at line ' . xml_get_current_line_number($parser);
            $root->SetErr("failed to parse file $filename, $err", CNode::E_FATAL);
        }
        xml_parser_free($parser);

        return $root;
    }

    private function startElement($parser, $name, $attrs)
    {
        if ($this->cur_node != null) {
            $this->node_stack[] = $this->cur_node;
        }

        $this->cur_node = new CNode($name, '');
        $this->cur_val = '';
    }

    private function endElement($parser, $name)
    {
        $this->cur_node->SetVal(trim($this->cur_val));
        $this->cur_val = '';
        $node = $this->cur_node;
        $this->cur_node = array_pop($this->node_stack);
        $this->cur_node->AddChild($node);
    }

    private function characterData($parser, $data)
    {
        $this->cur_val .= $data;
    }
}
