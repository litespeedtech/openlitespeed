<?php

class STATS {

	var $product = null;
	var $edition = null;
	var $version = null;
	var $uptime = null;
	var $load_avg = NULL;
	
	var $bps_in = 0;
	var $bps_out = 0;
	var $ssl_bps_in = 0;
	var $ssl_bps_out = 0;

	var $max_conn = 0;
	var $max_ssl_conn = 0;

	var $plain_conn = 0;
	var $avail_conn = 0;
	var $idle_conn = 0;

	var $ssl_conn = 0;
	var $avail_ssl_conn = 0;

	var $blocked_ip;
	var $vhosts = array();
	var $serv = NULL;

	//misc settings

	//full path to .rtreports files. different products have different paths
	var $report_path = "/tmp/lshttpd/";

	var $processes = 1;

	function __construct(){
		$product = PRODUCT::GetInstance();

		$this->report_path = $product->tmp_path;
		$this->processes = $product->processes;
	}
	
	function parse_all() {
		$this->parse_litespeed();
		$this->parse_sysstat();
		
	}

	function parse_sysstat() {

		$this->load_avg = Service::GetLoadAvg();
	}

	function parse_litespeed() {

		for ( $i = 1 ; $i <= $this->processes ; $i++ )
		{
			if ( $i > 1 ) {
				$content .= file_get_contents("{$this->report_path}.rtreport.{$i}");
			}
			else {
				$content = file_get_contents("{$this->report_path}.rtreport");
			}
			
		}
		$result = array();
		$this->blocked_ip = array();
		$found = 0;
		$found = preg_match_all("/VERSION: ([a-zA-Z0-9\ ]+)\/([a-zA-Z]*)\/([a-zA-Z0-9\.]+)\nUPTIME: ([0-9A-Za-z\ \:]+)\nBPS_IN:([0-9\ ]+), BPS_OUT:([0-9\ ]+), SSL_BPS_IN:([0-9\ ]+), SSL_BPS_OUT:([0-9\ ]+)\nMAXCONN:([0-9\ ]+), MAXSSL_CONN:([0-9\ ]+), PLAINCONN:([0-9\ ]+), AVAILCONN:([0-9\ ]+), IDLECONN:([0-9\ ]+), SSLCONN:([0-9\ ]+), AVAILSSL:([0-9\ ]+)/i", $content, $result);

		for($f = 0; $f < $found; $f++) {
			$this->product = trim($result[1][$f]);
			$this->edition = trim($result[2][$f]);
			$this->version = trim($result[3][$f]);
			$this->uptime = trim($result[4][$f]);
			$this->bps_in += (int) $result[5][$f];
			$this->bps_out += (int) $result[6][$f];
			$this->ssl_bps_in += (int) $result[7][$f];
			$this->ssl_bps_out += (int) $result[8][$f];

			$this->max_conn += (int) $result[9][$f];
			$this->max_ssl_conn += (int) $result[10][$f];

			$this->plain_conn += (int) $result[11][$f];
			$this->avail_conn += (int) $result[12][$f];
			$this->idle_conn += (int) $result[13][$f];

			$this->ssl_conn += (int) $result[14][$f];
			$this->avail_ssl_conn += (int) $result[15][$f];
		}
		
		$result = array();
		$found = 0;
		$found = preg_match_all("/BLOCKED_IP: ([0-9 \[\]\.,]*)/", $content, $result);

		for($f = 0; $f < $found; $f++) {
			$ips = trim($result[1][$f]);
			if ($ips != "") {
				$iplist = preg_split("/[\s,]+/", $ips, -1, PREG_SPLIT_NO_EMPTY);
				$this->blocked_ip = array_merge($this->blocked_ip, $iplist);
			}
		}
		$result = array();
		$found = 0;
		$found = preg_match_all("/REQ_RATE \[([^\]]*)\]: REQ_PROCESSING: ([0-9]+), REQ_PER_SEC: ([0-9]+), TOT_REQS: ([0-9]+)/i",$content,$result);

		for($f = 0; $f < $found; $f++) {
			$vhost = trim($result[1][$f]);

			if($vhost == '') {
				$vhost = '_Server';
			}

			if(!array_key_exists($vhost,$this->vhosts)) {
				$this->vhosts[$vhost] = new STATS_VHOST($vhost);
			}

			$temp = &$this->vhosts[$vhost];

			$temp->req_processing += (int) $result[2][$f];
			$temp->req_per_sec += doubleval($result[3][$f]);
			$temp->req_total += doubleval($result[4][$f]);
		}

		//reset
		$result = array();
		$found = 0;

		$found = preg_match_all("/EXTAPP \[([^\]]*)\] \[([^\]]*)\] \[([^\]]*)\]: CMAXCONN: ([0-9]+), EMAXCONN: ([0-9]+), POOL_SIZE: ([0-9]+), INUSE_CONN: ([0-9]+), IDLE_CONN: ([0-9]+), WAITQUE_DEPTH: ([0-9]+), REQ_PER_SEC: ([0-9]+), TOT_REQS: ([0-9]+)/i",$content,$result);

		for($f = 0; $f < $found; $f++) {
			$vhost = trim($result[2][$f]);
			$extapp = trim($result[3][$f]);

			if($vhost == '') {
				$vhost = '_Server';
			}

			if(!array_key_exists($vhost,$this->vhosts)) {
				$this->vhosts[$vhost] = new STATS_VHOST($vhost);
			}

			if(!array_key_exists($extapp,$this->vhosts[$vhost]->extapps)) {
				$this->vhosts[$vhost]->extapps[$extapp] = new STATS_VHOST_EXTAPP($extapp, $vhost);
				$this->vhosts[$vhost]->eap_count ++;
				$this->vhosts[$vhost]->eap_process ++;
			}

			$temp = &$this->vhosts[$vhost]->extapps[$extapp];

			$temp->type = trim($result[1][$f]);
			$temp->config_max_conn += (int) $result[4][$f];
			$temp->effect_max_conn += (int) $result[5][$f];
			$temp->pool_size += (int) $result[6][$f];
			$temp->inuse_conn += (int) $result[7][$f];
			$temp->idle_conn += (int) $result[8][$f];
			$temp->waitqueue_depth += (int) $result[9][$f];
			$temp->req_per_sec += (int) $result[10][$f];
			$temp->req_total += (int) $result[11][$f];

			$this->vhosts[$vhost]->eap_inuse += (int) $result[7][$f];
			$this->vhosts[$vhost]->eap_idle += (int) $result[8][$f];
			$this->vhosts[$vhost]->eap_waitQ += (int) $result[9][$f];
			$this->vhosts[$vhost]->eap_req_per_sec += (int) $result[10][$f];
			$this->vhosts[$vhost]->eap_req_total += (int) $result[11][$f];
			
		}

		if (count($this->blocked_ip) > 2) {
			sort($this->blocked_ip);
		}
		$this->serv = $this->vhosts['_Server'];
	}
	
	function apply_vh_filter($top, $filter, $sort) {
		$list = array_keys($this->vhosts);
		
		if ($filter != "") {
			$filter = "/$filter/i";
			$list = preg_grep($filter, $list);
		}
		if (count($list) <= 1) {
			return $list; 
		}
		
		if ($sort != "") {
			if ($sort == 'vhname') {
				sort($list);
			}
			else { //second order by name
				foreach ($list as $vhname) {
					$sortDesc1[] = $this->vhosts[$vhname]->$sort;
					$sortAsc2[] = $vhname;
				}
				array_multisort($sortDesc1, SORT_DESC, SORT_NUMERIC, $sortAsc2, SORT_ASC, SORT_STRING);
				$list = $sortAsc2; 
			}
			
		}
		
		if ($top != 0 && count($list) > $top) {
			$list = array_slice($list, 0, $top);
		}
		return $list;
	}

	function apply_eap_filter($top, $filter, $sort) {
		
		$listAll = array();
		foreach($this->vhosts as $vh) {
			if (count($vh->extapps)>0) {
				$listAll = array_merge($listAll, $vh->extapps);
			}
		}
		
		$list = array_keys($listAll);
		
		if ($filter != "") {
			$filter = "/$filter/i";
			$list = preg_grep($filter, $list);
		}

		$c = count($list);
		$exapps = array();
		
		if ($c == 0) {
			return $exapps; 
		}
		else if ($c == 1) {
			$exapps[] = $listAll[$list[0]];
			return $exapps; 
		}
		
		if ($sort != "") {
			if ($sort == 'extapp') {
				sort($list);
			}
			else { //second order by name
				foreach ($list as $name) {
					$sortDesc1[] = $listAll[$name]->$sort;
					$sortAsc2[] = $name;
				}
				array_multisort($sortDesc1, SORT_DESC, SORT_NUMERIC, $sortAsc2, SORT_ASC, SORT_STRING);
				$list = $sortAsc2; 
			}
			
		}
		
		if ($top != 0 && count($list) > $top) {
			$list = array_slice($list, 0, $top);
		}
		foreach($list as $name) {
			$exapps[] = $listAll[$name]; 
		}
		return $exapps;
	}
	

}

class STATS_VHOST {

	var $vhost = null;
	var $req_processing = 0;
	var $req_per_sec = 0;
	var $req_total = 0;
	var $output_bankdwidth = 0;
	var $eap_count = 0;
	var $eap_process = 0;
	var $eap_inuse = 0;
	var $eap_idle = 0;
	var $eap_waitQ = 0;
	var $eap_req_per_sec = 0;
	var $eap_req_total = 0;
	var $extapps = array();

	function STATS_VHOST($vhost = null) {
		$this->vhost = trim($vhost);
	}

}

class STATS_VHOST_EXTAPP {

	function STATS_VHOST_EXTAPP($extapp = null, $vhost = null) {
		$this->extapp = trim($extapp);
		$this->vhost = trim($vhost);
	}

	var $vhost = null;
	var $type = null;
	var $extapp = null;
	var $config_max_conn = 0;
	var $effect_max_conn = 0;
	var $pool_size = 0;
	var $inuse_conn = 0;
	var $idle_conn = 0;
	var $waitqueue_depth = 0;
	var $req_per_sec = 0;
	var $req_total = 0;

}

