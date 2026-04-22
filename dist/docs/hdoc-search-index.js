window.HDOC_SEARCH_INDEX = [
    {
        "title": "Home",
        "url": "index.html",
        "text": "Home OpenLiteSpeed Web Server 1.9 Users' Manual — Rev. 3 Table of Contents License Introduction Installation/Uninstallation Administration Security Configurations For more information, visit our OpenLiteSpeed Knowledge Base"
    },
    {
        "title": "License",
        "url": "license.html",
        "text": "License GNU GENERAL PUBLIC LICENSE v3 GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 Copyright (C) 2007 Free Software Foundation, Inc. Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed. Preamble The GNU General Public License is a free, copyleft license for software and other kinds of works. The licenses for most software and other practical work..."
    },
    {
        "title": "Introduction",
        "url": "intro.html",
        "text": "Introduction Introduction OpenLiteSpeed (OLS) is a high-performance, open-source web server. It serves as a free alternative to LiteSpeed Web Server Enterprise and provides compatibility with Apache rewrite rules. OLS uses an event-driven architecture and includes a built-in caching engine (LSCache). Overview OpenLiteSpeed supports modern protocols including HTTP/2 and HTTP/3 (QUIC). It is suitable for standalone ser..."
    },
    {
        "title": "Installation",
        "url": "install.html",
        "text": "Installation Installation/Uninstallation Minimum system requirements Operating System: Linux(i386): kernel 2.4 and up, glibc-2.2 and up CentOS: 5 and up Ubuntu: 8.04 and up Debian: 4 and up FreeBSD(i386): 4.5 and up MacOSX: 10.3 and up CPU: Intel: 80486 and up PowerPC: PowerPC G4 Memory: 32MB and up Disk: Installation: 200MB Runtime: 300MB and up, swapping space depends on usage. Installation Installation must be per..."
    },
    {
        "title": "Administration",
        "url": "admin.html",
        "text": "Administration Administration LiteSpeed Web Server can be controlled in three ways: through the WebAdmin console, from the command line, or by sending signals. Through the WebAdmin console: WebAdmin console is a centralized control panel to control and configure all LiteSpeed Web Server settings. Log on to the WebAdmin console (by default http://[your site's address]:7080/). Select \"Service Manager\". Here you will fi..."
    },
    {
        "title": "Security",
        "url": "security.html",
        "text": "Security Security LiteSpeed Web Server is designed with security as a top consideration. LSWS supports SSL, has access control at server and virtual host levels, and context-specific realm protection. Besides these standard features, LSWS also has the following special security features: Connection level limits: IP-level throttling limits network bandwidth to and from a single IP address regardless of the number of c..."
    },
    {
        "title": "Configuration",
        "url": "config.html",
        "text": "Configuration Configuration Concepts Here are some basic concepts you should know before going into the detail of the configuration. Virtual Hosts LiteSpeed Web Server can host multiple web sites (virtual hosts) with one server instance. Traditionally, virtual hosts are classified into two types: IP-based virtual hosts and name-based virtual hosts. IP-based virtual hosts are web sites that have their own unique IP ad..."
    },
    {
        "title": "Web Console",
        "url": "webconsole.html",
        "text": "Web Console Web Console The Web Console section controls the settings for the WebAdmin Console. Some of these settings include: Session Timeout Log Settings Access Control Create/Delete Admin Users Reset Admin Passwords WebAdmin Listeners & SSL Settings"
    },
    {
        "title": "Compile PHP",
        "url": "CompilePHP_Help.html",
        "text": "Compile PHP Extra PATH Environment Variables Additional PATH values that will be appended to the current PATH environment variables for build scripts. Installation Path Prefix Sets the value for the \"--prefix\" configure option. The default installation location is under LiteSpeed Web Server's install directory. LiteSpeed Web Server can use multiple PHP versions at the same time. If you are installing multiple version..."
    },
    {
        "title": "Server General",
        "url": "ServGeneral_Help.html",
        "text": "Server General General settings for the whole server. When path information is required in a setting, it can be either absolute or relative to $SERVER_ROOT. $SERVER_ROOT is the location where LiteSpeed web server has been installed (your_home_dir/lsws or /opt/lsws, for example). The server executable is under $SERVER_ROOT/bin. Server Name A unique name for this server. If empty, server hostname will be used by defaul..."
    },
    {
        "title": "Server Log",
        "url": "ServLog_Help.html",
        "text": "Server Log File Name Specifies the path for the log file. Place the log file on a separate disk. Log Level Specifies the level of logging to include in your log file. Available levels (from high to low) are: ERROR{/}, WARNING{/}, NOTICE{/}, INFO{/} and DEBUG{/}. Only messages with level higher or equal to the current setting will be logged. Using DEBUG{/} log level does not have any performance impact, unless is set ..."
    },
    {
        "title": "Server Tuning",
        "url": "ServTuning_Help.html",
        "text": "Server Tuning Default SHM Directory Changes shared memory's default directory to the specified path. If the directory does not exist, it will be created. All SHM data will be stored in this directory unless otherwise specified. PROXY Protocol List of IPs/subnets for front-end proxies that communicate with this server using PROXY protocol. Once set, the server will use PROXY protocol for incoming connections from list..."
    },
    {
        "title": "Server Security",
        "url": "ServSecurity_Help.html",
        "text": "Server Security Follow Symbolic Link Specifies the server-level default setting of following symbolic links when serving static files. Choices are Yes{/}, If Owner Match{/} and No{/}. Yes{/} sets the server to always follow symbolic links. If Owner Match{/} sets the server to follow a symbolic link only if the owner of the link and of the target are same. No{/} means the server will never follow a symbolic link. This..."
    },
    {
        "title": "External Apps",
        "url": "ExtApp_Help.html",
        "text": "External Apps LiteSpeed Web Server can forward requests to external applications to process and generate dynamic content. Since 2.0, LiteSpeed Web Server has supported seven types of external applications: CGI, FastCGI, web server, servlet engine, LiteSpeed SAPI application, load balancer, and piped logger. CGI stands for Common Gateway Interface. The current standard is CGI/1.1. CGI applications run in standalone pr..."
    },
    {
        "title": "Script Handler",
        "url": "ScriptHandler_Help.html",
        "text": "Script Handler LiteSpeed Web Server supports all scripting languages including Perl, PHP, Ruby, Python, Java, etc. Scripts written in these different languages must be brought to the appropriate external application to be processed. LiteSpeed Web Server uses script handlers to decide which external application a script should go to. These script handlers map files to external applications by using the file suffix. (A..."
    },
    {
        "title": "App Server Settings",
        "url": "App_Server_Help.html",
        "text": "App Server Settings Rack/Rails Default Settings Default configurations for Rack/Rails applications. These settings can be overridden at the context level. Ruby Path Path to Ruby executable. Generally, it is /usr/bin/ruby or /usr/local/bin/ruby depending on where Ruby has been installed to. Run-Time Mode Specifies which mode the application will be running as: \"Development\", \"Production\", or \"Staging\". The default is ..."
    },
    {
        "title": "Module Configuration",
        "url": "Module_Help.html",
        "text": "Module Configuration Module support is available in OpenLiteSpeed 1.3 and LSWS Enterprise 5.0 and greater. All required modules must be registered under the Server Modules Configuration tab. Module files must be located in the server root/modules folder to be available for registering. On start up, the server loads all registered modules. The server must be restarted after new modules are registered. Modules can be c..."
    },
    {
        "title": "Listeners General",
        "url": "Listeners_General_Help.html",
        "text": "Listeners General Listener Name A unique name for this listener. IP Address Specifies the IP of this listener. All available IP addresses are listed. IPv6 addresses are enclosed in \"[ ]\". To listen on all IPv4 IP addresses, select ANY{/}. To listen on all IPv4 and IPv6 IP addresses, select [ANY]{/}. In order to serve both IPv4 and IPv6 clients, an IPv4-mapped IPv6 address should be used instead of a plain IPv4 addres..."
    },
    {
        "title": "Listeners SSL",
        "url": "Listeners_SSL_Help.html",
        "text": "Listeners SSL SSL Private Key & Certificate Every SSL listener requires a paired SSL private key and SSL certificate. Multiple SSL listeners can share the same key and certificate. You can generate SSL private keys yourself using an SSL software package, such as OpenSSL. SSL certificates can also be purchased from an authorized certificate issuer like VeriSign or Thawte. You can also sign the certificate yourself. Se..."
    },
    {
        "title": "Virtual Host Templates",
        "url": "Templates_Help.html",
        "text": "Virtual Host Templates Virtual host templates make it easy to create many new virtual hosts with similar configurations. Each template contains a template configuration file, a list of mapped listeners, and a list of member virtual hosts. To add a template-based virtual host, the administrator only needs to add a member with a unique virtual host name and a qualified domain name to the template member list. Listener-..."
    },
    {
        "title": "Virtual Host Registration",
        "url": "VirtualHosts_Help.html",
        "text": "Virtual Host Registration This page lists all defined virtual hosts. From here you can add/delete a virtual host or make changes to an existing one. Before you add a virtual host, make sure the virtual host root directory exists. Virtual Host Name A unique name for a virtual host. It is recommended to use the virtual host's domain name as the Virtual Host Name. Virtual Host Name can be referred to using the variable ..."
    },
    {
        "title": "Virtual Host General",
        "url": "VHGeneral_Help.html",
        "text": "Virtual Host General Document Root Specifies the document root for this virtual host. $VH_ROOT/html{/} is recommended. This directory is referred to as $DOC_ROOT in contexts. Administrator Email Specifies email address(es) of the administrator(s) of this virtual host. Enable Compression Specifies whether to enable GZIP/Brotli compression for this virtual host. This setting is only effective when compression is enable..."
    },
    {
        "title": "Virtual Host Security",
        "url": "VHSecurity_Help.html",
        "text": "Virtual Host Security CAPTCHA Protection CAPTCHA Protection is a service provided as a way to mitigate heavy server load. CAPTCHA Protection will activate after one of the below situations is hit. Once active, all requests by NON TRUSTED(as configured) clients will be redirected to a CAPTCHA validation page. After validation, the client will be redirected to their desired page. The following situations will activate ..."
    },
    {
        "title": "Virtual Host SSL",
        "url": "VHSSL_Help.html",
        "text": "Virtual Host SSL SSL Private Key & Certificate Every SSL listener requires a paired SSL private key and SSL certificate. Multiple SSL listeners can share the same key and certificate. You can generate SSL private keys yourself using an SSL software package, such as OpenSSL. SSL certificates can also be purchased from an authorized certificate issuer like VeriSign or Thawte. You can also sign the certificate yourself...."
    },
    {
        "title": "Rewrite",
        "url": "Rewrite_Help.html",
        "text": "Rewrite Enable Rewrite Specifies whether to enable LiteSpeed's URL rewrite engine. This option can be customized at the virtual host or context level, and is inherited along the directory tree until it is explicitly overridden. Auto Load from .htaccess Autoload rewrite rules contained in a directory's .htaccess file when first accessing that directory if an HttpContext for that directory using the rewritefile directi..."
    },
    {
        "title": "Context",
        "url": "Context_Help.html",
        "text": "Context In LiteSpeed Web Server terminology, a \"context\" is a virtual location, a common parent URL, that identifies a group of resources. Contexts can be thought of as different directories in your website's directory tree. For example, \"/\" is the root context mapped to the document root of a website. \"/cgi-bin/\" is a context farther up on the tree, dedicated to the CGI applications for this site. A context can be e..."
    },
    {
        "title": "Web Socket Proxy",
        "url": "VHWebSocket_Help.html",
        "text": "Web Socket Proxy WebSocket is a protocol that can be used instead of HTTP to deliver real-time, bidirectional communication over the Internet. Starting with version 1.1.1, OpenLiteSpeed supports WebSocket backends through the use of WebSocket proxies. These proxies send the WebSocket communication to the appropriate backend stipulated in the field. URI Specifies the URI(s) that will use this WebSocket backend. Traffi..."
    },
    {
        "title": "WebAdmin Console - Service Manager",
        "url": "ServerStat_Help.html",
        "text": "WebAdmin Console - Service Manager The Service Manager acts as a control room for monitoring the server and controlling certain top-level functions. It provides the following features: (The Service Manager can be accessed by clicking on the Actions menu or from home page.) Monitor the current status of the server, listeners, and virtual hosts. Apply configuration changes with a graceful restart. Enable or disable a p..."
    },
    {
        "title": "Real-Time Statistics",
        "url": "Real_Time_Stats_Help.html",
        "text": "Real-Time Statistics Anti-DDoS Blocked IP A comma-separated list of IP addresses blocked by Anti-DDoS protection, each ending with a semi-colon and reason code indicating why the IP address was blocked. Potential Reason Codes: A{/}: BOT_UNKNOWN B{/}: BOT_OVER_SOFT C{/}: BOT_OVER_HARD D{/}: BOT_TOO_MANY_BAD_REQ E{/}: BOT_CAPTCHA F{/}: BOT_FLOOD G{/}: BOT_REWRITE_RULE H{/}: BOT_TOO_MANY_BAD_STATUS I{/}: BOT_BRUTE_FORCE..."
    },
    {
        "title": "LiteSpeed SAPI App",
        "url": "External_LSAPI.html",
        "text": "LiteSpeed SAPI App Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Address A unique socket address used by the external application. IPv4/IPv6 sockets and Unix Domain Sockets (UDS) are supported. IPv4/IPv6 sockets can be used for communication over the network. UDS can only be used when the external application resides on the sam..."
    },
    {
        "title": "Web Server (Proxy)",
        "url": "External_WS.html",
        "text": "Web Server (Proxy) Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Address HTTP,HTTPS, or Unix Domain Sockets (UDS) address used by the external web server. If you proxy to another web server running on the same machine using an IPv4/IPv6 address, set the IP address to localhost{/} or 127.0.0.1{/}, so the external application is ..."
    },
    {
        "title": "Fast CGI App",
        "url": "External_FCGI.html",
        "text": "Fast CGI App Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Address A unique socket address used by the external application. IPv4/IPv6 sockets and Unix Domain Sockets (UDS) are supported. IPv4/IPv6 sockets can be used for communication over the network. UDS can only be used when the external application resides on the same mach..."
    },
    {
        "title": "Fast CGI Authorizer",
        "url": "External_FCGI_Auth.html",
        "text": "Fast CGI Authorizer Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Address A unique socket address used by the external application. IPv4/IPv6 sockets and Unix Domain Sockets (UDS) are supported. IPv4/IPv6 sockets can be used for communication over the network. UDS can only be used when the external application resides on the sa..."
    },
    {
        "title": "Simple CGI App",
        "url": "External_SCGI.html",
        "text": "Simple CGI App Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Address A unique socket address used by the external application. IPv4/IPv6 sockets and Unix Domain Sockets (UDS) are supported. IPv4/IPv6 sockets can be used for communication over the network. UDS can only be used when the external application resides on the same ma..."
    },
    {
        "title": "Servlet Engine",
        "url": "External_Servlet.html",
        "text": "Servlet Engine Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Address A unique socket address used by the external application. IPv4/IPv6 sockets and Unix Domain Sockets (UDS) are supported. IPv4/IPv6 sockets can be used for communication over the network. UDS can only be used when the external application resides on the same ma..."
    },
    {
        "title": "Piped Logger",
        "url": "External_PL.html",
        "text": "Piped Logger Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Remote Logger Address (Optional) Specifies the optional socket address for this piped logger. Set this when the logger is reached through a network socket or Unix Domain Socket. Leave this blank for a local piped logger started from the configured command path. Max Conn..."
    },
    {
        "title": "Load Balancer",
        "url": "External_LB.html",
        "text": "Load Balancer Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Workers List of worker groups previously defined in the external load balancer."
    },
    {
        "title": "uWSGI",
        "url": "External_UWSGI.html",
        "text": "uWSGI Name A unique name for this external application. You will refer to it by this name when you use it in other parts of the configuration. Address A unique socket address used by the external application. IPv4/IPv6 sockets and Unix Domain Sockets (UDS) are supported. IPv4/IPv6 sockets can be used for communication over the network. UDS can only be used when the external application resides on the same machine as ..."
    },
    {
        "title": "Static Context",
        "url": "Static_Context.html",
        "text": "Static Context Static Context Context settings are used to specify special settings for files in a certain location. These settings can be used to bring in files outside of the document root (like Apache's Alias or AliasMatch directives), to protect a particular directory using authorization realms, or to block or restrict access to a particular directory within the document root. URI Specifies the URI for this conte..."
    },
    {
        "title": "Java Web App Context",
        "url": "Java_Web_App_Context.html",
        "text": "Java Web App Context Java Web App Context Many people running Java applications use the servlet engine to serve static content as well. But no servlet engine is nearly as efficient as LiteSpeed Web Server for these processes. In order to improve the overall performance, LiteSpeed Web Server can be configured as a gateway server, which serves static content and forwards dynamic Java page requests to the servlet engine..."
    },
    {
        "title": "Servlet Context",
        "url": "Servlet_Context.html",
        "text": "Servlet Context Servlet Context Servlets can be imported individually through Servlet Contexts. A Servlet Context just specifies the URI for the servlet and the name of the servlet engine. You only need to use this when you do not want to import the whole web application or you want to protect different servlets with different authorization realms. This URI has the same requirements as for a . URI Specifies the URI f..."
    },
    {
        "title": "Fast CGI Context",
        "url": "FCGI_Context.html",
        "text": "Fast CGI Context FastCGI Context FastCGI applications cannot be used directly. A FastCGI application must be either configured as a script handler or mapped to a URL through FastCGI context. A FastCGI context will associate a URI with a FastCGI application. URI Specifies the URI for this context. FastCGI App Specifies the name of the FastCGI application. This application must be defined in the section at the server o..."
    },
    {
        "title": "Simple CGI Context",
        "url": "SCGI_Context.html",
        "text": "Simple CGI Context SCGI Context SCGI applications cannot be used directly. An SCGI application must be either configured as a script handler or mapped to a URL through an SCGI context. An SCGI context will associate a URI with an SCGI application. URI Specifies the URI for this context. SCGI App Specifies the name of the SCGI application. This application must be defined in the section at the server or virtual host l..."
    },
    {
        "title": "LiteSpeed SAPI Context",
        "url": "LSAPI_Context.html",
        "text": "LiteSpeed SAPI Context LiteSpeed SAPI Context External applications cannot be used directly. They must be either configured as a script handler or mapped to a URL through a context. An LiteSpeed SAPI Context will associate a URI with an LSAPI (LiteSpeed Server Application Programming Interface) application. Currently PHP, Ruby and Python have LSAPI modules. LSAPI, as it is developed specifically for LiteSpeed Web Ser..."
    },
    {
        "title": "Proxy Context",
        "url": "Proxy_Context.html",
        "text": "Proxy Context Proxy Context A Proxy Context enables this virtual host as a transparent reverse proxy server. This proxy server can run in front of any web servers or application servers that support HTTP protocol. The External web server that this virtual host proxies for has to be defined in before you can set up a Proxy Context. URI Specifies the URI for this context. Web Server Specifies the name of the external w..."
    },
    {
        "title": "CGI Context",
        "url": "CGI_Context.html",
        "text": "CGI Context CGI Context A CGI context defines scripts in a particular directory as CGI scripts. This directory can be inside or outside of the document root. When a file under this directory is requested, the server will always try to execute it as a CGI script, no matter if it's executable or not. In this way, file content under a CGI Context is always protected and cannot be read as static content. It is recommende..."
    },
    {
        "title": "Load Balancer Context",
        "url": "LB_Context.html",
        "text": "Load Balancer Context Load Balancer Context Like other external applications, load balancer worker applications cannot be used directly. They must be mapped to a URL through a context. A Load Balancer Context will associate a URI to be load balanced by the load balancer workers. URI Specifies the URI for this context. Load Balancer Specifies the name of the load balancer to be associated to this context. This load ba..."
    },
    {
        "title": "Redirect Context",
        "url": "Redirect_Context.html",
        "text": "Redirect Context Redirect Context A Redirect Context can be used to forward one URI or a group of URIs to another location. The destination URI can be either on the same web site (an internal redirect) or an absolute URI pointing to another web site (an external redirect). URI Specifies the URI for this context. External Redirect Specifies whether this redirect is external. For external redirection, may be specified ..."
    },
    {
        "title": "App Server Context",
        "url": "App_Server_Context.html",
        "text": "App Server Context App Server Context An App Server Context provides an easy way to configure a Ruby Rack/Rails, WSGI, or Node.js application. To add a an application through an App Server Context, only mounting the URL and the application's root directory is required. There is no need to go through all the trouble to define an external application, add a 404 handler, and rewrite rules, etc. URI Specifies the URI for..."
    },
    {
        "title": "uWSCGI Context",
        "url": "UWSGI_Context.html",
        "text": "uWSCGI Context uWSGI Context uWSGI applications cannot be used directly. A uWSGI application must be either configured as a script handler or mapped to a URL through a uWSGI context. A uWSGI context will associate a URI with an uWSGI application. URI Specifies the URI for this context. uWSGI App Specifies the name of the uWSGI application. This application must be defined in the section at the server or virtual host ..."
    },
    {
        "title": "Module Handler Context",
        "url": "Module_Context.html",
        "text": "Module Handler Context Module Handler Context A module handler context will associate a URI with a registered module. Modules need to be registered at Server Module Configuration tab. URI Specifies the URI for this context. Module Name of the module. The module must be registered under the Server Module Configuration tab. Once it is registered, the module name will be available in the drop down box for the Listener a..."
    },
    {
        "title": "Admin Console General",
        "url": "AdminGeneral_Help.html",
        "text": "Admin Console General Admin Server is a special virtual host dedicated to the WebAdmin console. It is very important to make sure Admin Server is securely protected either by only allowing access from the administrator's machines or by using an encrypted SSL connection. Enable Core Dump Specifies whether to enable core dump when the server is started by \"root\" user. For most modern Unix systems, processes that change..."
    },
    {
        "title": "Admin Console Security",
        "url": "AdminSecurity_Help.html",
        "text": "Admin Console Security Access Control Specifies what sub networks and/or IP addresses can access the server. At the server level, this setting will affect all virtual hosts. You can also set up access control unique to each virtual host at the virtual host level. Virtual host level settings will NOT override server level settings. Blocking/Allowing an IP is determined by the combination of the allowed list and the de..."
    },
    {
        "title": "Admin Listeners General",
        "url": "AdminListeners_General_Help.html",
        "text": "Admin Listeners General Admin Listeners are dedicated to the Admin Server. Secure (SSL) listeners are recommended for the Admin Server. Listener Name A unique name for this listener. IP Address Specifies the IP of this listener. All available IP addresses are listed. IPv6 addresses are enclosed in \"[ ]\". To listen on all IPv4 IP addresses, select ANY{/}. To listen on all IPv4 and IPv6 IP addresses, select [ANY]{/}. I..."
    },
    {
        "title": "Admin Listeners SSL",
        "url": "AdminListeners_SSL_Help.html",
        "text": "Admin Listeners SSL Admin Listeners are dedicated to the Admin Server. Secure (SSL) listeners are recommended for the Admin Server. SSL Private Key & Certificate Every SSL listener requires a paired SSL private key and SSL certificate. Multiple SSL listeners can share the same key and certificate. You can generate SSL private keys yourself using an SSL software package, such as OpenSSL. SSL certificates can also be p..."
    }
];
