<?php

class Service extends ServiceBase
{
	function refreshConf()
	{
		$data = ConfControl::ExportConf();
		$conflisteners = $data['listener'];
		foreach( $conflisteners as $lname => $addr ) {
			if ( !isset($this->listeners[$lname]) )	{
				$this->listeners[$lname] = array('addr' => $addr, 'status' => 'Error');
			}
		}

		$vhnames = $data['vhost'];
		foreach( $this->vhosts as $key => $value )
		{
			$this->vhosts[$key] = $value{0} . 'N';
		}
		foreach( $vhnames as $vhname )
		{
			if ( !isset($this->vhosts[$vhname]) )
				$this->vhosts[$vhname] = '0A';
		}

		$this->serv['name'] = $data['serv'];

		$fd = fopen(self::FPID, 'r');
		if ( $fd )
		{
			$this->serv['pid'] = trim(fgets($fd));
			fclose($fd);
		}
		$this->serverLog = $data['servLog'];
	}

	function process($act, $actId)
	{
		$this->act = $act;
		$this->actId = $actId;

		if ( $this->isPending() ) {
			return false;
		}

		$this->checkLastMod();

		if ( $actId ) {
			$this->vhostControl();
		}
		elseif ( $act == 'restart' ) {
			$this->restartServer();
		}
		elseif ( $act == 'toggledbg' )
		{
			$cmd = array('toggledbg');
			$this->issueCmd($cmd);
		}
		return true;
	}

	function readStatus()
	{
		$this->listeners = array();
		$this->adminL = array();
		$this->vhosts = array();
		$fd = fopen(self::FSTATUS, 'r');
		if ( !$fd )
			return false;
		while ( !feof($fd) )
		{
			$buffer = fgets($fd, 512);
			if ( strncmp($buffer, 'LISTENER0', 9) == 0 )
			{
				$this->readListener($this->listeners, $buffer, $fd);
			}
			elseif ( strncmp($buffer, 'LISTENER1', 9) == 0 )
			{
				$this->readListener($this->adminL, $buffer, $fd);
			}
			elseif ( strncmp($buffer, 'VHOST', 5) == 0 )
			{
				$this->readVh($buffer, $fd);
			}
			elseif ( strncmp($buffer, 'EOF', 3) == 0 )
				break;
		}
		fclose($fd);
		return true;
	}

}
