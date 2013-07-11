<?php

define(FLD_IP, 0);
define(FLD_KAReqServed, 1);
define(FLD_Mode, 2);
define(FLD_ReqTime, 3);
define(FLD_InBytesCurrent, 4);
define(FLD_InBytesTotal, 5);
define(FLD_OutBytesCurrent, 6);
define(FLD_OutBytesTotal, 7);
define(FLD_VHost, 8);
define(FLD_Handler, 9);
define(FLD_ExtappProcessTime, 10);
define(FLD_ExtappSocket, 11);
define(FLD_ExtappPid, 12);
define(FLD_ExtappConnReqServed, 13);
define(FLD_Url, 14);

class ReqProbe {

	var $filters = null;
	var $req_detail = null;
	
		
	//127.0.0.1       0       WR      17.2    92      92      694     -1      Example example_lsphp   17      /tmp/lshttpd/example_lsphp.997  5390    0       "GET /sslow.php HTTP/1.1"
/*
 * 
127.0.0.1,  IP
0,   keep-alived requests
WR,  connection status
53.0, total processing time
513/513, in bytes/total bytes
469/-1,  out bytes/total bytes  (-1 don't know)
Example, vh name
example_lsphp, handler name (external app name, static file is "static")
.......
53,  extapp process time 
/tmp/lshttpd/example_lsphp.519,  extapp socket
17587,  extapp pid
0  reqs processed through extapp connection
"GET /sslow.php HTTP/1.1",  req_url
 */	
	
	function __construct(){
		$this->filters = null;
		$this->req_detail = null;
	}
	
	function retrieve($filters, &$total_count, &$filtered_count) {
		$this->filters = $filters;
		$vhname_filter = '';
		$ip_filter = '';
		$url_filter = '';
		$reqtime_filter = '';
		$extproctime_filter = '';
		$reqbodysize_filter = '';
		$outsize_filter = '';

		/*$filters = array('SHOW_TOP' => $req_show_top,
	'SHOW_SORT' => $req_show_sort,
	'VHNAME' => $vhname_filter,
	'IP' => $ip_filter,
	'URL' => $url_filter,
	'REQ_TIME' => $reqtime_filter,
	'PROC_TIME' => $extproctime_filter,
	'IN'=> $reqbodysize_filter,
	'OUT' => $outsize_filter);
		 * */

		$show_top = (int)$filters['SHOW_TOP'];
		$show_sort = $filters['SHOW_SORT'];
		$has_filter = FALSE;

		$sortBy = FLD_ProcessTime; // default
		if ($show_sort == 'vhname') {
			$sortBy = FLD_VHost;
		} elseif ($show_sort == 'client') {
			$sortBy = FLD_IP;
		} elseif ($show_sort == 'req_time') {
			$sortBy = FLD_ReqTime;
		} elseif ($show_sort == 'extproc_time') {
			$sortBy = FLD_ExtappProcessTime;
		} elseif ($show_sort == 'in') {
			$sortBy = FLD_InBytesTotal;
		} elseif ($show_sort == 'out') {
			$sortBy = FLD_OutBytesCurrent;
		} 
		if ($filters['VHNAME'] != NULL) {
			$vhname_filter = $filters['VHNAME'];
			$has_filter = TRUE;
		}
		if ($filters['IP'] != NULL) {
			$ip_filter = $filters['IP'];
			$has_filter = TRUE;
		}
		if ($filters['URL'] != NULL) {
			$url_filter = $filters['URL'];
			$has_filter = TRUE;
		}
		if ($filters['REQ_TIME'] != NULL) {
			$reqtime_filter = $filters['REQ_TIME'];
			$has_filter = TRUE;
		}
		if ($filters['PROC_TIME'] != NULL) {
			$extproctime_filter = $filters['PROC_TIME'];
			$has_filter = TRUE;
		}
		if ($filters['IN'] != NULL) {
			$reqbodysize_filter = 1024 * floatval($filters['IN']);
			$has_filter = TRUE;
		}
		if ($filters['OUT'] != NULL) {
			$outsize_filter = 1024 * floatval($filters['OUT']);
			$has_filter = TRUE;
		}
			
		$contents = $this->getStatusDetail();
		if ($contents == FALSE)
			return FALSE;

		$full_list = explode("\n", $contents);
		$count = count($full_list);
		//remove last empty line
		if (trim($full_list[$count-1]) == '') {
			$count --;
		}
		$total_count = $count;
		$filtered_list = array();
		$sorted_list = array();
		if ($has_filter) {
			$j = 0;
			for ($i = 0; $i < $count ; ++$i) {
				$s2 = explode("\t", $full_list[$i]);
				
				if ($reqtime_filter != '') {
					if ($s2[FLD_ReqTime] <= $reqtime_filter)
						continue;
				}
				if ($extproctime_filter != '') {
					if ($s2[FLD_ExtappProcessTime] <= $extproctime_filter)
						continue;
				}
				if ($reqbodysize_filter != '') {
					if ($s2[FLD_InBytesTotal] <= $reqbodysize_filter)
						continue;
				}
				if ($outsize_filter != '') {
					if ($s2[FLD_OutBytesCurrent] <= $outsize_filter)
						continue;
				}
				if ($ip_filter != '') {
					if (!preg_match("/$ip_filter/", $s2[FLD_IP])) {
						continue;
					}
				}
				if ($vhname_filter != '') {
					if (!preg_match("/$vhname_filter/i", $s2[FLD_VHost])) 
						continue;
				}
				if ($url_filter != '') {
					if (!preg_match("/$url_filter/i", $s2[FLD_Url])) 
						continue;
				}
				$filtered_list[$j] = $full_list[$i];
				$sorted_list["{$j}a"] = $s2[$sortBy];
				$j ++;
			}
		}
		else {
			$filtered_list = &$full_list;
			for ($i = 0; $i < $count ; ++$i) {
				$s2 = explode("\t", $full_list[$i]);
				$sorted_list["{$i}a"] = $s2[$sortBy];
			}
		}
				
		if ($sortBy == FLD_VHost || $sortBy == FLD_IP) {
			//reg sort
			asort($sorted_list);
		} 
		else {
			//reverse sort
			arsort($sorted_list, SORT_NUMERIC);
		} 
		if ($show_top == 0) {
			$show_top = count($sorted_list);
		}
		$cut = 0;
		$final_list = array();
		foreach ($sorted_list as $ia => $v) {
		 	$index = (int)(rtrim($ia, 'a'));
			$final_list[$cut++] = $filtered_list[$index];
			if ($cut == $show_top)
				break;
		}
		$filtered_count = count($final_list);
		return $final_list;
	}
	
	
	function getStatusDetail()
	{
		$cmd = 'status:rpt=detail';
		$service = new Service();
		return $service->retrieveCommandData($cmd);
	}
}
