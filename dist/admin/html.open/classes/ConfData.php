<?php

class CVal
{
	private $_v = NULL; //value
	private $_e = NULL; //err
	public function __construct($v, $e=NULL)
	{
		$this->_v = $v;
		$this->_e = $e;
	}
	
	public function GetVal()
	{
		return $this->_v;
	} 
	
	public function SetVal($v)
	{
		$this->_v = $v;
	}
	
	public function HasVal()
	{
		// 0 is valid
		return ($this->_v !== NULL && $this->_v !== '');
	}
	
	public function GetErr()
	{
		return $this->_e;
	}
	
	public function SetErr($e)
	{
		$this->_e = $e;
	}
	
	public function HasErr()
	{
		return $this->_e != NULL;
	}
	
}

class ConfData
{
	var $_data;
	var $_path;
	var $_type; //{'serv','admin','vh','tp'}
	var $_id;

	public function __construct($type, $path, $id=NULL)
	{
		$this->_data = array();
		$this->_type = $type;
		$this->_path = $path;
		$this->_id = $id;
	}

	public function setId($id)
	{
		$this->_id = $id;
	}

}
