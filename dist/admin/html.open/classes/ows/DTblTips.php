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
			$tips[] = 'MIME settings can be edited from previous page. You can specify the mime configuration file location which can be either be an absolute path or relative to $SERVER_ROOT.';
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
		elseif ( $tid == 'SERV_EXPIRES' || $tid == 'VH_EXPIRES' )
		{
			$tips[] = 'Expires can be set at server, virtual host or context level. Lower level will override higher level settings: virtual host setting will override server settings.';
			$tips[] = 'Expires syntax, "A|Mseconds" means after base time (A or M) plus the specified time in seconds, the file will expire. "A" means client access time, "M" means file modified time. You can override this default setting by different MIME type: A86400 means the file will expire after 1 day based on client access time.';
			$tips[] = 'Expires By Type will override default settings. Each entry is in the format of "MIME-type=A|Mseconds" with no space in between. You can input multiple entries separated by comma.';
			$tips[] = 'Here are some common numbers: 1 hour = 3600 sec, 1 day = 86400 sec, 1 week = 604800 sec, 1 month = 2592000 sec, 1 year = 31536000 sec.';
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
			$tips[] = 'Dynamic GZIP compression control will be effective only if GZIP Compression is enabled.';
			$tips[] = 'GZIP Compression level ranges from 1 (Minimum) to 9 (Maximum).';
			$tips[] = 'Compressible Types is a list of MIME types that are compressible, separated by commas. You can use wildcard "*" for MIME types, like */*, text/*. You can put "!" in front to exclude certain types. The order of the list is important if you use "!". For e.g., a list like "text/*, !text/css, !text/js" will compress all text file except for css and js.';
		}
		elseif ( $tid == 'SERV_SEC_FILE')
		{
			$tips[] = 'If Follow-Symbolic-Link is enabled, you can still disable it at virtual host level.';
			$tips[] = 'Check-Symbolic-Link control will take effect only if Follow-Symbolic-Link is turned on. This controls whether symbolic links are checked against Access Denied Directories.';
		}
		elseif ( $tid == 'SERV_SEC_CGI')
		{
			$tips[] = 'Limits resources that CGI program can use. This helps against DoS attack.';
			$tips[] = 'Max CGI Instances controls how many CGI processes the web server can launch.';
			$tips[] = 'Process soft/hard limit controls how many processes are allowed for one user. This includes all the processes spawned by CGI application. This is OS level limit.';
			$tips[] = 'Set to 0 or empty will use operation system default value for all soft/hard limits.';
			$tips[] = 'The soft limit is the value that the kernel enforces for the corresponding resource. The hard limit acts as a ceiling for the soft limit';
		}
		elseif ( $tid == 'SERV_SEC_CONN')
		{
			$tips[] = 'Set IP level throttle limit here. The number will be rounded up to 4K units. Set to "0" to disable throttling.';
			$tips[] = 'Set concurrent connection Limits coming from one client (per IP address). This helps against DoS attack.';
			$tips[] = 'Number of connections can temporarily exceed Soft Limit during Grace Period as long as under Hard Limit. After Grace Period, if it is still above Soft Limit, then no more connections will be allowed from that IP for time of Banned Period.';
			$tips[] = 'See help for more details.';
		}
		elseif ( $tid == 'SERV_SEC_DENY')
		{
			$tips[] = 'Enter a full path if you want to deny access for specific directory; entering a path followed by * will disable all the sub directories.';
			$tips[] = 'Path can be either absolute or relative to $SERVER_ROOT, use comma to separate.';
			$tips[] = 'If both Follow Symbolic Link and Check Symbolic Link are enabled, symbolic links will be checked against the denied directories';
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
		elseif ( $tid == 'A_EXT_SEL' )
		{
			$tips[] = 'You can set up external Fast CGI application and AJPv13 (Apache JServ Protocol v1.3) compatible servlet engine.';
		}
		elseif ( $tid == 'A_EXT_FCGI' || $tid == 'A_EXT_FCGIAUTH' )
		{
			$tips[] = 'Give a name that easy to remember, other places will refer to this app by its name.';
			$tips[] = 'Address can be IPv4 socket address "IP:PORT", like 192.168.1.3:7777 and localhost:7777 or Unix domain socket address "UDS://path" like UDS://tmp/lshttpd/myfcgi.sock.';
			$tips[] = 'UDS is chrooted in chroot environment.';
			$tips[] = 'For local FCGI, Unix domain socket is preferred due to security and better performance. If you have to use IPv4 socket, set the IP part to localhost or 127.0.0.1, thus the FCGI is inaccessible from other machines.';
			$tips[] = 'Local FCGI can be started by the web server. In this case, you need to specify path, backlog and number of instances.';
		}
		elseif ( $tid == 'A_EXT_LOADBALANCER' )
		{
			$tips[] = 'Load balancing workers must be previously defined.';
			$tips[] = 'Format is a comma-separated list of <extAppType>::<extAppName>.';
		   	$tips[] = 'Available ExtApp Types are fcgi(Fast CGI App), fcgiauth(Fast CGI Authorizer), lsapi(LSAPI App), servlet(Servlet/JSP Engine), proxy(Web Server)';
			$tips[] = 'Different types of external applications can be mixed in one load balancing cluster.';
			$tips[] = 'Example: fcgi::localPHP, proxy::backend1';
		}
		elseif ( $tid == 'A_SCRIPT')
		{
			$tips[] = 'Script handler can be a CGI, a FCGI app, a module handler, a Servlet engine, or a proxy to Web server.';
			$tips[] = 'Except CGI, other handlers need to be predefined in "External App" section.';
			$tips[] = 'If the web site supports php or jsp, please set up here.';
		}
		elseif ( $tid == 'S_RAILS')
		{
			$tips[] = 'Ruby Path is the path of ruby executable. For e.g., /usr/local/bin/ruby. It has to be an absolute path.';
		}
		elseif ( $tid == 'L_GENERAL')
		{
			$tips[] = "Give listener a name that is easy to understand and remember.";
			$tips[] = "Select an IP address from the list, if you don't specify a particular address, the system will bind to all the available IP address on this machine.";
			$tips[] = "Input a unique port number on this IP for this listener. Only super user (root) can use port lower than 1024. Port 80 is the default HTTP port; port 443 is the default HTTPS port.";
			$tips[] = 'Check "secure" will use https, then you can configure SSL settings.';
		}
		elseif ( $tid == 'L_VHMAP')
		{
			$tips[] = 'Select the virtual hosts that you want to map to this listener.';
			$tips[] = 'Enter all the domains that you want this listener to response. Use comma "," to separate individual domain.';
			$tips[] = 'You can choose only one virtual host to handle all unspecified domains, put "*" in domains.';
			$tips[] = 'If you have not set up the virtual host you want to map, you can skip this step and come back later';
		}
		elseif ( $tid == 'L_SSL_CERT' )
		{
			$tips[] = '"secure" checkbox on Listener General - Address Settings needs to be enabled.';
		}
		elseif ( $tid == 'L_SSL')
		{
			$tips[] = '"secure" checkbox on Listener General - Address Settings needs to be enabled.';
		}
		elseif ( $tid == 'VH_REALM_SEL' )
		{
			$tips[] = 'Define your HT Access realm here, this can be used for contexts.';
			$tips[] = 'Currently password file and LDAP are supported for user DB and group DB.';
		}
		elseif ( $tid == 'VH_REALM_FILE' )
		{
		}
		elseif ( $tid == 'VH_REALM_LDAP' )
		{
		}
		elseif ( $tid == 'VH_CTX_SEL' )
		{
			$tips[] = 'Static context can be used to map a URI to a directory either outside document root or within it.';
			$tips[] = 'Java Web App context is used to automatically import a predefined Java Application in an AJPv13 compilant Java servlet engine.';
			$tips[] = 'Servlet context is used to import a specific servlet under a web application.';
			$tips[] = 'Fast CGI context is a mount point of Fast CGI application.';
			$tips[] = 'Proxy context enables this virtual host serving as a transparant reverse proxy server to an external web server or application server.';
			$tips[] = 'CGI context can be used to specify a directory only contains CGI scripts.';
			$tips[] = 'Redirect context can set up an internal or external redirect URI.';
			$tips[] = 'Rack/Rails context is specifically used for Rack/Rails application';
			$tips[] = 'Module handler context is a mount point of hander type modules.';

		}
		if ( substr($tid, 0, -1) == 'VH_CTX' )
		{
			$tipExpire = 'Expires settings will override virtual host and server level settings. Expires Default syntax is A|Mseconds, means after base time (A or M) plus the specified time in seconds, the file will expire. "A" means client access time, "M" means file modified time.';

			$tipOverride = 'Allow Override will be effective only if server/vhost level setting is enabled. Context will inherit virtual host settings by default when nothing is checked. When nothing but "None" is checked, access control file will be ignored. When "Access Control" is checked, directives "Order", "Allow" and "Deny" are allowed. When "Authentication" is checked, directives "AuthGroupFile", "AuthName", "AuthType", "AuthUserFile", "Require" are allowed.';
			$tipRealm = 'A Context can be protected with predefined realm, which is set up in the virtual host security section. Optionally, an alternative name and additional requirements can be specified.';

			if ( $tid == 'VH_CTXG')
			{
				$tips[] = 'Static context can be used to map a URI to a directory either outside document root or within it. The directory can be absolute path or relative to document root(default), $VH_ROOT or $SERVER_ROOT.';
				$tips[] = 'Check "Accessible" will allow browsing static files in this context. You may want to disable it to prevent viewing static files, for e.g. when you update the content.';
				$tips[] = &$tipExpire;
				$tips[] = &$tipOverride;
				$tips[] = &$tipRealm;
				$tips[] = &$tipAC;
			}
			elseif ( $tid == 'VH_CTXJ')
			{
				$tips[] = 'Java web app context is used to automatically import a predefined Java Application in an AJPv13 compilant Java servlet engine, the servlet engine should be set up in external app section (either server or virtual host level).';
				$tips[] = 'Location is the directory that contains web application files, which includes WEB-INF/ sub directory.';
				$tips[] = 'The web server will automatically import configuration file of web application, which usually is WEB-INF/web.xml under the driectory specified by "location".';
				$tips[] = 'If the servlet engine runs on a different machine, it is recommended to make a copy of webapps directory locally. Otherwise you must put the files in a common accessible network drive, which may affect performance. ';
				$tips[] = &$tipExpire;
				$tips[] = &$tipOverride;
				$tips[] = &$tipRealm;
				$tips[] = &$tipAC;
			}
			elseif ( $tid == 'VH_CTXS')
			{
				$tips[] = 'Servlet context is used to import a specific servlet under a web application, whereas a "Java Web App" context will import every thing automatically. You do not need to set up individual Servlet Context for each servlet defined in a web application.';
				$tips[] = &$tipOverride;
				$tips[] = &$tipRealm;
				$tips[] = &$tipAC;
			}
			elseif ( $tid == 'VH_CTXF')
			{
				$tips[] = 'Fast CGI context is a mount point of Fast CGI application. The Fast CGI Application must be pre-defined at server level or virtual host level.';
				$tips[] = &$tipOverride;
				$tips[] = &$tipRealm;
				$tips[] = &$tipAC;
			}
			elseif ( $tid == 'VH_CTXP')
			{
				$tips[] = 'Proxy context enables this virtual host serving as a transparant reverse proxy server to an external web server or application server.';
				$tips[] = 'External web server must be pre-defined under External App at server or virtual host level.';
			}
			elseif ( $tid == 'VH_CTXC')
			{
				$tips[] = 'A CGI context can be used to specify a directory only contains CGI scripts. Path can be absolute path or relative to $SERVER_ROOT, $VH_ROOT or $DOC_ROOT(default). Path and URI must be ended with "/" for a cgi-bin directory.';
				$tips[] = 'If only a specific script is needed in that directory, it is recommended to create a CGI context for that script only. In this case, path and URI need not be a directory. For e.g., path can be ~/myapp/myscript.pl, URI can be /myapp/myscript.pl. All other files will not be served as CGI.';
				$tips[] = &$tipOverride;
				$tips[] = &$tipRealm;
				$tips[] = &$tipAC;
			}
			elseif ( $tid == 'VH_CTXR')
			{
				$tips[] = 'Set up redirect URI here. If it is an external redirect, you can specify the status code. Internal redirect has to start with "/", external redirect can either start with "/" or with "http(s)://".';
				$tips[] = &$tipOverride;
				$tips[] = &$tipRealm;
				$tips[] = &$tipAC;
			}
			elseif ( $tid == 'VH_CTXRL')
			{
				$tips[] = 'Rack/Rails context is for easy configuration of running Rack/Rails application. You only need to specify the root location of your rack/rails application in the "Location" field.';
				$tips[] = &$tipOverride;
				$tips[] = &$tipRealm;
				$tips[] = &$tipAC;
			}
		}
		elseif ( $tid == 'VH_ERRPG' )
		{
			$tips[] = 'You can set up your customized error pages for different error codes.';
		}
		elseif ( $tid == 'VH_TOP_D' )
		{
			$tips[] = 'All directories must preexist. This web interface will not create directory for you. If you are creating a new virtual host, you can create empty root directory you and set up from beginning; or you can copy the DEFAULT virtual root that shipped with the package to this virtual host root and then modify it.';
			$tips[] = 'Virtual host root ($VH_ROOT) can be absolute path or relative to $SERVER_ROOT. Config File can be relative to $VH_ROOT.';
			$tips[] = 'Turn on Restrained in shared hosting enviroment.';
		}
		elseif ( $tid == 'VH_GENERAL')
		{
			$tips[] = 'Set up your document root here, which can be absolute path or relative to $SERV_ROOT or $VH_ROOT';
			$tips[] = 'Document root is referred as $DOC_ROOT in this virtual host, which can be used in other path configuration.';
			$tips[] = 'You can enter multiple admin emails, separated by comma.';
		}
		elseif ( $tid == 'VH_LOG')
		{
			$tips[] = 'If you use server setting, the log will write to the server file which is set up at server level.';
			$tips[] = 'Log file path can be either absolute path or relative to $SERVER_ROOT, $VH_ROOT.';
			$tips[] = 'If you want to set Log Level to DEBUG, you must set server log level to DEBUG as well. The level of debugging is solely controlled by Server DEBUG Level. Use DEBUG only if you have to since it has great impact on server performance and may fill up disk space quickly.';
			$tips[] = 'A new file will be created if current log file exceeds the rolling size. The size is in bytes, you can put 10240, 10K or 1M.';
		}
		elseif ( $tid == 'VH_ACLOG')
		{
			$tips[] = 'When required, you can disable access logging for this virtual host save disk i/o.';
			$tips[] = 'Log file path can be either an absolute path or a relative path to $SERVER_ROOT, $VH_ROOT.';
			$tips[] = 'A new log file will be created if current log file exceeds the rolling size. File size is in bytes and you use multiple formats: 10240, 10K or 1M.';
		}
		elseif ( $tid == 'VH_INDXF')
		{
			$tips[] = 'You can use default server level settings for index files or use your own ones.';
			$tips[] = 'You can use your settings in addition to the server settings.';
			$tips[] = 'You can disable index file by not using server settings and blanking vhost settings.';
			$tips[] = 'You can enable/disable "auto index" at context level.';
		}
		elseif ( $tid == 'VH_FRONTPG')
		{
			$tips[] = 'Domain names are comma-dilimited list of domain:port. For e.g. www.domain1.com:80, www.domain2.com:443 ';
		}
		elseif ( $tid == 'VH_REWRITE_MAP')
		{
			$tips[] = 'Enter URI for location. URI must start with "/".';
		}
		elseif ( $tid == 'VH_UDB' )
		{
			$tips[] = 'If you enter group information here, the group DB will not be checked.';
			$tips[] = 'You can enter multiple groups, use comma to separate. Space will be treated as part of a group name.';
		}
		elseif ( $tid == 'VH_GDB' )
		{
			$tips[] = 'Group DB will be checked only if the user in the user DB does not contain group information.';
			$tips[] = 'Use comma to separate multiple users.';
		}
		elseif ( $tid == 'ADMIN_USR')
		{
			$tips[] = 'You need to know the old password in order to change it.';
			$tips[] = 'If you forget the admin password, you cannot change it from here for security reason. Please use the following command from shell:
[your install dir]/admin/misc/admpass.sh. This script will override all the user ids and reset to one admin user';
		}
		return $tips;
	}

}
