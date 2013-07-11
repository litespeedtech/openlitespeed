<?php

class DATTR_HELP_ITEM
{
	private $name;
	private $desc;
	private $tips; 
	private $syntax;
	private $example;
	
	function __construct($name, $desc, $tips = NULL, $syntax = NULL, $example = NULL) {
		
		$this->name = $name;
		$this->desc = $desc;
		$this->tips = $tips;
		$this->syntax = $syntax;
		$this->example = $example;
	}
	

	function render($key, $blocked_version=0) 
	{ 
		$key = str_replace(array(':','_'), '', $key );
		$buf = '<img class="xtip-hover-' . $key . '" src="/static/images/icons/info.gif">' 
			. '<div id="xtip-note-' . $key . '" class="snp-mouseoffset notedefault"><b>' 
			. $this->name
			. '</b>' ;
		switch ($blocked_version) {
			case 0: break; 
			case 1:  // LSWS ENTERPRISE; 
				$buf .= ' <i>This feature is available in Enterprise Edition</i>';
				break;
			case 2: // LSWS 2CPU +
			case 3: // LSLB 2CPU +
				$buf .= ' <i>This feature is available for Multiple-CPU license</i>';
				break;
		}
		$buf .= '<hr size=1 color=black>'
			. $this->desc
			. '<br><br>';
		if ($this->syntax) {
			$buf .= 'Syntax: ' 
				. $this->syntax
				. '<br><br>';
		}
		if ($this->example) {
			$buf .= 'Example: ' 
				. $this->example
				. '<br><br>';
		}
		if ($this->tips) {
			$buf .= 'Tip(s):<ul type=circle>';
			if (strpos($this->tips, '<br>') !== FALSE) {
				$tips = explode('<br>', $this->tips);
				foreach($tips as $ti) {
					$ti = trim($ti);
					if ($ti != '')
						$buf .= '<li>' . $ti . '</li>';	
				}
			}
			else {
				$buf .= '<li>' . $this->tips . '</li>';
			}
			$buf .= '</ul>';
		}

		$buf .= '</div>';
		return $buf;
	}
	
}

