<?php

require_once('blowfish.php');

class Service
{
	var $cmd = NULL;
	var $cmdStatus = NULL;
	var $act = NULL;
	var $actId = NULL;
	var $FNAME = NULL;
	var $FSTATUS = NULL;
	var $FPID = NULL;
	var $serverLog = NULL;
	var $serverLastModTime = 100;
	var $listeners = array();
	var $adminL = array();
	var $vhosts = array();
	var $serv = array();


	function __construct() {
		$this->FNAME = '/tmp/lshttpd/.admin';
		$this->FSTATUS = '/tmp/lshttpd/.status';
		$this->FPID = '/tmp/lshttpd/lshttpd.pid';
	}

	function init()
	{
		clearstatcache();
		$this->serverLastModTime = filemtime($this->FSTATUS);
		$this->readStatus();
	}

	function refreshConf($data)
	{
		$listeners = $data['listener'];
		foreach( $listeners as $lname => $addr )
		{
			if ( !isset($this->listeners[$lname]) )
			{
				$this->listeners[$lname] = array();
				$this->listeners[$lname]['addr'] = $addr;
				$this->listeners[$lname]['status'] = 'Error';
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

		$fd = fopen($this->FPID, 'r');
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
			unset($this->cmd);
			$this->cmd[] = 'toggledbg';
			$this->issueCmd();
		}
		return true;
	}

	function waitForChange()
	{
		for( $count = 0; $count < 5; ++$count)
		{
			if ( !$this->checkLastMod() )
				sleep(1);
			else
				return true;
		}
		return false;

	}

	function readStatus()
	{
		$this->listeners = array();
		$this->adminL = array();
		$this->vhosts = array();
		$fd = fopen($this->FSTATUS, 'r');
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

	function readListener(&$l, $buffer, &$fd)
	{
		if ( preg_match( "/\[(.+)\] (.+)$/", $buffer, $matches ))
		{
			$lname = $matches[1];
			$l[$lname]['addr'] = $matches[2];
			$l[$lname]['status'] = 'Running';
			$tmp = fgets($fd, 512);
			while ( strncmp($tmp, 'ENDL' ,4) != 0 )
			{
				if ( strncmp( $tmp, 'LVMAP', 5 ) == 0)
				{
					if ( preg_match( "/\[(.+)\] (.+)$/", $tmp, $tm ))
					{
						$l[$lname]['map'][$tm[1]][] = $tm[2];
					}
				}
				$tmp = fgets($fd, 512);
			}
		}
	}

	function readVh($buffer, &$fd)
	{
		if ( preg_match( "/\[(.+)\] ([01])/", $buffer, $m ))
		{
			$vname = $m[1];
			if ( $vname != '_AdminVHost' )
				$this->vhosts[$m[1]] = $m[2];
		}
	}


	function checkLastMod()
	{
		clearstatcache();
		$mt = filemtime($this->FSTATUS);
		if ( $this->serverLastModTime != $mt )
		{
			$this->serverLastModTime = $mt;
			return true;
		}
		else
		{
			return false;
		}
	}

	function restartServer()
	{
		unset($this->cmd);
		$this->cmd[] = 'restart';

		$client = CLIENT::singleton();
		$client->changed = FALSE;
		$this->issueCmd();
	}


	function vhostControl()
	{
		unset( $this->cmd );
		$this->cmd[] = "$this->act:vhost:$this->actId";
		$this->issueCmd();
		$this->waitForChange();
		$this->readStatus();
	}

	function install( $app )
	{
	    unset( $this->cmd );
	    $this->cmd[]= 'install:' . $app;
	    $this->issueCmd();
	}

	public static function getCommandSocket($cmd)
	{
		$client = CLIENT::singleton();
		if ( strncmp( $_SERVER['LSWS_ADMIN_SOCK'], 'uds://', 6 ) == 0 )
		{
		    $sock = socket_create( AF_UNIX, SOCK_STREAM, 0 );
		    $addr = substr( $_SERVER["LSWS_ADMIN_SOCK"], 5 );
		    if ( socket_connect( $sock, $addr ) == false )
		    {
				error_log( 'failed to connect to server! socket_connect() failed: ' . socket_strerror(socket_last_error()) . "\n" );
				return false;
		    }

		}
		else
		{
		    $sock = socket_create( AF_INET, SOCK_STREAM, SOL_TCP );
		    $addr = explode( ":", $_SERVER['LSWS_ADMIN_SOCK'] );
		    if ( socket_connect( $sock, $addr[0], intval( $addr[1] ) ) == false )
		    {
				error_log( 'failed to connect to server! socket_connect() failed: ' . socket_strerror(socket_last_error()) . "\n" );
				return false;
		    }
		}
		$uid = PMA_blowfish_decrypt( $client->id, $client->secret[0]);
		$password = PMA_blowfish_decrypt( $client->pass, $client->secret[1]);
		$outBuf = "auth:" . $uid . ':' . $password . "\n";
		$outBuf .= $cmd . "\n" . 'end of actions';
		socket_write( $sock, $outBuf );
		socket_shutdown( $sock, 1 );
		return $sock;
	}

	function issueCmd()
	{
		$commandline = '';
		foreach( $this->cmd as $line )
		{
			$commandline .= $line . "\n";
		}
		$sock = Service::getCommandSocket($commandline);
		if ($sock != FALSE) {
			$res = socket_recv( $sock, $buffer, 1024, 0 );
			socket_close( $sock );
			return (( $res > 0 )&&(strncasecmp( $buffer, 'OK', 2 ) == 0 ));
		}
		else
			return FALSE;
	}

	public static function retrieveCommandData($cmd)
	{
		$sock = Service::getCommandSocket($cmd);
		$buffer = '';
		if ($sock != FALSE) {
			$read   = array($sock);
			$write  = NULL;
			$except = NULL;
			$num_changed_sockets = socket_select($read, $write, $except, 3); //wait for max 3 seconds
			if ($num_changed_sockets === FALSE) {
				error_log("socket_select failed: " . socket_strerror(socket_last_error()));
			}
			else if ($num_changed_sockets > 0) {
				while (socket_recv($sock, $data, 8192, 0)) {
		            $buffer .= $data;
		        }
			}
			socket_close( $sock );
		}
		return $buffer;

	}


	function isPending()
	{
		if ( file_exists($this->FNAME) )
		{
			$this->cmdStatus = 'IS_PENDING';
			return true;
		}
		else
			return false;
	}

	function &getLogData()
	{
		$data = array();
		$data['filename'] = $this->getServerLog();
		$level = DUtil::getGoodVal(DUtil::grab_input("get",'sel_level'));
		if ( $level == NULL )
			$level = 'I';
		if (!in_array($level, array('E', 'W', 'N', 'I', 'D'))) {
			return NULL;
		}

		$data['level'] = $level;

		$fd = fopen($data['filename'], 'r');
		if ( !$fd )
			return NULL;

		fseek( $fd, 0, SEEK_END );
		$endpos = ftell($fd);
		fclose($fd);
		$data['logSize'] = number_format( $endpos/1024, 2);

		$searchSize = (int) DUtil::getGoodVal(DUtil::grab_input("get",'searchSize'));
		if ( $searchSize <= 0 )
			$searchSize = 20;
		else if ($searchSize > 512)
			$searchSize = 512;

		$data['searchSize'] = $searchSize;

		$searchFrom = (int) DUtil::getGoodVal(DUtil::grab_input("get",'searchFrom'));

		if ( isset($_GET['end']) )
		{
			$searchFrom = $endpos;
		}
		else
		{
			if ( isset($_GET['prev']) )
				$searchFrom -= $searchSize;
			elseif ( isset($_GET['next']) )
				$searchFrom += $searchSize;
			elseif ( isset($_GET['begin']) )
				$searchFrom = 0;

			if ( $searchFrom < 0 )
				$searchFrom = 0;

			$data['searchFrom'] = $searchFrom;
			$searchFrom *= 1024;
		}

		$searchSize *= 1024;

		if ( $searchFrom >= $endpos )
		{
			$searchFrom = $endpos - $searchSize;
			if ( $searchFrom < 0 )
				$searchFrom = 0;
			$data['searchFrom'] = number_format( $searchFrom/1024, 2, '.', '');
		}
		if ( $searchFrom + $searchSize < $endpos )
			$endpos = $searchFrom + $searchSize;

		$data['fromPos'] = $searchFrom;
		$data['endPos'] = $endpos;
		return $data;
	}

	function showErrLog(&$buf, $len=20480)
	{
		$buf = array();
		$data = array();
		$data['filename'] = $this->getServerLog();
		$data['level'] = 'W';

		$fd = fopen($data['filename'], 'r');
		if ( !$fd )
		{
			$buf[] = 'Failed read server log file from '.$data['filename'];
			return 0;
		}

		fseek( $fd, 0, SEEK_END );
		$data['endPos'] = ftell($fd);
		if ( $data['endPos'] > $len )
			$data['fromPos'] = $data['endPos'] - $len;
		else
			$data['fromPos'] = 0;
		fclose($fd);
		$res = &$this->getLog($data);

		if ( $res[0] == 0 )
			return 0;

		if ( $res[0] > 10 )
		{
			$r = explode("\n", $res[2]);
			$i = count($r) - 11;
			for ( $j = 0 ; $j < 10 ; ++$j )
			{
				$buf[] = $r[$i+$j];
			}
			$res[0] = 'last 10';
		}
		else
		    $buf[] = $res[2];


		return $res[0];

	}


	function &getLog($data)
	{
		$newlineTag = '[ERR[WAR[NOT[INF[DEB';
		$levels = array( 'E' => 1, 'W' => 2, 'N' => 3, 'I' => 4, 'D' => 5 );
		$level = $levels[$data['level']{0}];

		$fd = fopen($data['filename'], 'r');
		if ( !$fd )
		{
			echo '<tr><td class="message_error" colspan=3>Failed to read server log from file '.$data['filename'].'</td></tr>';
			exit;
		}
		$endpos = $data['endPos'];
		fseek( $fd, $data['fromPos'] );
		$start = 0;
		$result = '';
		$totalLine = 0;
		$line = 0;
		$cutline = 0;
		$buffer = fgets($fd);
		while ( !preg_match("/^\d{4}-\d{2}-\d{2} /", $buffer) )
		{
			$buffer = fgets($fd);
			if ( $buffer === false )
				break;
			$curpos = ftell($fd);
			if ( $curpos >= $endpos )
				break;
		}

		do
		{
			$buffer = chop($buffer);
			// check if new line
			$c25 = substr($buffer, 25 , 3);
			if ( $c25 && strstr($newlineTag, $c25) )
			{
				// is new line
				$totalLine ++;
				if ( $start )
				{
					// finish prior line
					$result .= '</td></tr>' . "\n";
					$start = 0;
				}
				$b25 = $c25{0};
				if ( $levels[$b25] <= $level )
				{
					// start a new line
					$start = 1;
					$line ++;
					$style = 'log_' . $b25;
					$result .= '<tr><td class="col_time '. $style . '0">'. substr($buffer, 0, 23);
					$result .= '</td><td class="col_level '. $style . '1">';
					$i = strpos($buffer, ']', 24);
					$result .= ( substr($buffer, 25, $i - 25) );
					$result .= '</td><td class="col_mesg '. $style . '2">';
					$result .= htmlspecialchars( substr($buffer, $i+2) );
				}
			}
			elseif ( $start )
			{
				// multi-line output
				$result .= '<br>'.$buffer;
			}

			$curpos = ftell($fd);
			if ( $curpos >= $endpos )
				break;

		} while ( $buffer = fgets($fd) );

		fclose($fd);
		if ( $start )
			$result .= '</td></tr>'."\n";
		$res = array();
		$res[] = $line;
		$res[] = $totalLine;
		$res[] = &$result;

		return $res;
	}

	function getServerLog()
	{
		if ( $this->serverLog == null )
		{
			require_once('ConfigFileEx.php');

			$confpath = $_SERVER['LS_SERVER_ROOT'] . "conf/httpd_config.xml" ; //fixed location
			$logpath = ConfigFileEx::grepTagValue($confpath, 'logging.log.fileName');
			$this->serverLog = str_replace('$SERVER_ROOT', $_SERVER['LS_SERVER_ROOT'], $logpath);
		}
		return $this->serverLog;
	}

	function loadParam(&$holder, $line)
	{
		$t = preg_split('/[\s,:]/', $line, -1, PREG_SPLIT_NO_EMPTY);
		$c = count($t)/2;

		for ( $i = 0 ; $i < $c ; ++$i )
		{
			if(is_array($holder) && array_key_exists($t[2*$i],$holder)) {
				$holder[$t[2*$i]] += $t[2*$i+1];
			}
			else {
				$holder[$t[2*$i]] = $t[2*$i+1];
			}
		}
	}

	public static function GetLoadAvg()
	{
		$load_avg = 'N/A';
		$UPTIME = '';
		$UPTIME = exec("uptime");
		$UPTIME = strtolower( $UPTIME );
		$pos = strpos($UPTIME, "load average");
		if ( $pos > 0 )
		{
			$pos += 13;
            if ( $UPTIME[$pos] == 's' )
	            ++$pos;

			if ( $UPTIME[$pos] == ':' )
				++$pos;
			$load_avg = trim(substr($UPTIME, $pos ));
		}
		return $load_avg;
	}

}
