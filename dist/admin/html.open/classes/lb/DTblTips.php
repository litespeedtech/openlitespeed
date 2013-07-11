<?php

class DTblTips
{

	public static function getTips($tid)
	{
		$tips = NULL;

		if ( $tid == 'SERV_PROCESS' )
		{
			$tips[] = 'The user and group setting of the server process cannot be modified. This was set up during installation. You have to reinstall to change this option.';
			$tips[] = 'Swapping directory is recommended to be placed on a local disk such as /tmp. Network drive should be avoided at all cost. Swap will be when configured memory i/o buffer is exhausted.';
			$tips[] = 'When you enable "Auto Restart" the monitor process will automatically launch a new server process and service will resume instantly if a crash is detected.';
		}
		elseif ( $tid == 'SERV_GENERAL' )
		{
			$tips[] = 'You can enter multiple admin emails: use comma to separate.';
		}
		elseif ( $tid == 'SERV_LOG' )
		{
			$tips[] = 'Log file path can be either an absolute path or relative to $SERVER_ROOT.';
			$tips[] = 'A new log file will be created if current log file exceeds the rolling size. File size is in bytes and you can use multiple input formats: 10240, 10K or 1M.';
			$tips[] = 'Stderr Log is located in the same directory as the Server Log. If enabled, all External Application output to stderr will be logged in this file.';
		}
		elseif ( $tid == 'SERV_ACLOG' )
		{
			$tips[] = 'Log file path can be either be an absolute path or relative to $SERVER_ROOT.';
			$tips[] = 'A new log file will be created if current log file exceeds the rolling size. File size is in bytes and you can use multiple input formats: 10240, 10K or 1M.';
		}
		elseif ( $tid == 'SERV_TUNING_CONN' )
		{
			$tips[] = 'Numbers can be represented as 10240, 10K or 1M.';
			$tips[] = 'If send/receive buffer size is 0, default TCP buffer size will be used.';
		}
		elseif ( $tid == 'SERV_TUNING_REQ' )
		{
			$tips[] = 'Numbers can be represented like 10240, 10K or 1M.';
		}
		elseif ( $tid == 'SERV_TUNING_GZIP')
		{
			$tips[] = 'GZIP Compression level ranges from 1 (Minimum) to 9 (Maximum).';
			$tips[] = 'Compressible Types is a list of MIME types that are compressible, separated by commas. You can use wildcard "*" for MIME types, like */*, text/*. You can put "!" in front to exclude certain types. The order of the list is important if you use "!". For e.g., a list like "text/*, !text/css, !text/js" will compress all text file except for css and js.';
		}
		elseif ( $tid == 'SERV_SEC_CONN')
		{
			$tips[] = 'Set IP level throttle limit here. The number will be rounded up to 4K units. Set to "0" to disable throttling.';
			$tips[] = 'Set concurrent connection Limits coming from one client (per IP address). This helps against DoS attack.';
			$tips[] = 'Number of connections can temporarily exceed Soft Limit during Grace Period as long as under Hard Limit. After Grace Period, if it is still above Soft Limit, then no more connections will be allowed from that IP for time of Banned Period.';
			$tips[] = 'See help for more details.';
		}
		elseif ( $tid == 'A_SECAC')
		{
			$tips[] = 'You can set up access control at server, virtual host and context levels. If there is access control at server level, the virtual host rules will be applied after the server rules are satisfied.';
			$tips[] = 'To deny access from certain address, put "ALL" in allowed list, and put subnet or IP in denied list. To allow only certain IP or subnet to access the site, put "ALL" in denied list and specify the address in the allowed list.';
			$tips[] = 'Input format can be either IP like 192.168.0.2, or sub-network like 192.168.* or subnet/netmask like 192.168.128.5/255.255.128.0.';
			$tips[] = 'If you have trusted IP or sub-network, then you must specify them in allowed list by adding a trailing "T" such as 192.168.1.*T. Trusted IP or sub-network is not limited by connection/throttling limit.';

//admin interface tips
			/*$tips[] = 'If there is access control at regular server level, this settings will be applied after the server rules are satisfied.';

			$tips[] = 'If you want to deny access from certain address, put "ALL" in allowed list, and put subnet or IP in denied list. You can also deny "ALL" but allow certain address.';
			$tips[] = 'You can either enter IP like 192.168.0.2, or subnet like 192.168.* or subnet/netmask like 192.168.128.5/255.255.128.0.';
			$tips[] = 'For security purpose, it is recommended here to put deny "ALL" and allow only those IP that administrator can access.';
			*/
		}
		elseif ( $tid == 'A_EXT_FCGI' || $tid == 'A_EXT_FCGIAUTH' )
		{
			$tips[] = 'Give a name that easy to remember, other places will refer to this app by its name.';
			$tips[] = 'Address can be IPv4 socket address "IP:PORT", like 192.168.1.3:7777 and localhost:7777 or Unix domain socket address "UDS://path" like UDS://tmp/lshttpd/myfcgi.sock.';
			$tips[] = 'UDS is chrooted in chroot environment.';
			$tips[] = 'For local FCGI, Unix domain socket is preferred due to security and better performance. If you have to use IPv4 socket, set the IP part to localhost or 127.0.0.1, thus the FCGI is inaccessible from other machines.';
			$tips[] = 'Local FCGI can be started by the web server. In this case, you need to specify path, backlog and number of instances.';
		}
		elseif ( $tid == 'L_GENERAL')
		{
			$tips[] = "Give listener a name that is easy to understand and remember.";
			$tips[] = "Select an IP address from the list, if you don't specify a particular address, the system will bind to all the available IP address on this machine.";
			$tips[] = "Input a unique port number on this IP for this listener. Only super user (root) can use port lower than 1024. Port 80 is the default HTTP port; port 443 is the default HTTPS port.";
			$tips[] = 'Check "secure" will use https, then you must configure SSL settings.';
		}
		elseif ( $tid == 'L_VHMAP')
		{
			$tips[] = 'Select the virtual hosts that you want to map to this listener.';
			$tips[] = 'Enter all the domains that you want this listener to response. Use comma "," to separate individual domain.';
			$tips[] = 'You can choose only one virtual host to handle all unspecified domains, put "*" in domains.';
			$tips[] = 'If you have not set up the virtual host you want to map, you can skip this step and come back later';
		}
		elseif ( $tid == 'L_CERT' )
		{
			$tips[] = '"secure" checkbox on General:Address Settings needs to be enabled.';
			$tips[] = 'File path can be absolute path or relative to $SERVER_ROOT.';
		}
		elseif ( $tid == 'L_SSL')
		{
			$tips[] = '"secure" checkbox on General:Address Settings needs to be enabled.';
			$tips[] = 'For SSL versions and encryption levels, please select all you want to accept.';
		}
		elseif ( $tid == 'VH_SECG' )
		{
		}
		elseif ( $tid == 'VH_SECHL' )
		{
			$tips[] = 'Set "Allow Direct Access" to "Yes" if it is preferred to allow direct access without a referrer. For e.g., there is no referrer when a user key in a url in a browser and some proxy will suppress "Referrer" header.';
			$tips[] = 'If "Redirect URL" is set, a hotlink attempt will be redirected to the url specified; otherwise, access denied status code will be returned.';
			$tips[] = 'Set "Only Self Reference" to "Yes" will only allow links from same web site and there is no need to specify "Allowed Domains". It is more convinent when multiple domains are parked. ';
			$tips[] = 'If "Only Self Reference" set to "No", "Allowed Domains" must be specified.';
		}
		elseif ( $tid == 'VH_CTX_SEL' )
		{
			$tips[] = 'Redirect context can set up an internal or external redirect URI.';
		}
		if ( substr($tid, 0, -1) == 'VH_CTX' )
		{

			if ( $tid == 'VH_CTXR')
			{
				$tips[] = 'Set up redirect URI here. If it is an external redirect, you can specify the status code. Internal redirect has to start with "/", external redirect can either start with "/" or with "http(s)://".';
				$tips[] = &$tipAC;
			}
		}
		elseif ( $tid == 'VH_TOP_D' )
		{
			$tips[] = 'Set up the default cluster here, all traffic will be served by this cluster unless contexts has been defined';
		}
		elseif ( $tid == 'VH_GENERAL')
		{
			$tips[] = 'You can enter multiple admin emails, separated by comma.';
		}
		elseif ( $tid == 'VH_ACLOG')
		{
			$tips[] = 'When required, you can disable access logging for this virtual host save disk i/o.';
			$tips[] = 'A new log file will be created if current log file exceeds the rolling size. File size is in bytes and you use multiple formats: 10240, 10K or 1M.';
		}
		elseif ( $tid == 'VH_REWRITE_CTRL')
		{
		}
		elseif ( $tid == 'VH_REWRITE_RULE')
		{
		}
		elseif ( $tid == 'VH_REWRITE_MAP')
		{
			$tips[] = 'Enter URI for location. URI must start with "/".';
		}
		elseif ( $tid == 'ADMIN_PHP')
		{
		}
		return $tips;
	}

}

