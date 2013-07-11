<?php

class DFileSect
{
	private $_tids;

	private $_layerId;
	private $_dataLoc;
	private	 $_holderIndex;

	public function __construct($tids, $layerId=NULL, $dataLoc=NULL, $holderIndex=NULL)
	{
		$this->_tids = $tids;
		$this->_layerId = $layerId;
		$this->_dataLoc = $dataLoc;
		$this->_holderIndex = $holderIndex;
	}
	
	public function GetTids()
	{
		return $this->_tids;
	}
	
	public function GetLayerId()
	{
		return $this->_layerId;
	}
	
	public function GetDataLoc()
	{
		return $this->_dataLoc;
	}
	
	public function GetHolderIndex()
	{
		return $this->_holderIndex;
	}
}
