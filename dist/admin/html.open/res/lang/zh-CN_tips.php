<?php 

global $_tipsdb;

$_tipsdb['CACertFile'] = new DAttrHelp("CA证书文件", '指定包含证书颁发机构（CA）证书的证书链文件。 按照优先顺序，此文件只是PEM编码的证书文件的串联。 这可以用作替代或 除了&quot;CA证书路径&quot;。 这些证书用于客户端证书身份验证和构建服务器证书链，除了服务器证书之外，这些证书还将发送到浏览器。', '', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['CACertPath'] = new DAttrHelp("CA证书路径", '指定证书颁发机构（CA）证书的目录。 这些证书用于客户端证书身份验证和构建服务器证书链，除了服务器证书之外，这些证书还将发送到浏览器。', '', 'path', '');

$_tipsdb['CGIPriority'] = new DAttrHelp("CGI优先级", '指定外部应用程序进程的优先级。数值范围从-20到20。数值越小，优先级越高。<br/><br/>CGI进程不能拥有比Web服务器更高的优先级。如果这个优先级数值被设置为低于 服务器的优先级数值，则将使用服务器优先级作为替代。', '', 'int', '');

$_tipsdb['CPUHardLimit'] = new DAttrHelp("CPU硬限制", '以秒为单位，指定CGI进程的CPU占用时间限制。 如果进程持续占用CPU时间，达到硬限制，则进程将被强制杀死。如果没有设置该限制，或者限制设为0， 操作系统的默认设置将被使用。', '', '整数', '');

$_tipsdb['CPUSoftLimit'] = new DAttrHelp("CPU软限制(秒)", '以秒为单位，指定CGI进程的CPU占用时间限制。当进程达到 软限制时，将收到通知信号。如果没有设置该限制，或者限制设为0， 将使用操作系统的默认设置。', '', '整数', '');

$_tipsdb['DHParam'] = new DAttrHelp("DH参数", '指定DH密钥交换所需的Diffie-Hellman参数文件的位置。', '', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['GroupDBLocation'] = new DAttrHelp("组数据库位置", '指定组数据库的位置。<br/>组信息可以在用户数据库或在这个独立的组数据库中设置。 用于用户验证时，将首先检查用户数据库。 如果用户数据库同样包含组信息，组数据库将不被检查。<br/>对于类型为Password File的数据库， 组数据库地址应当是到达包含有组定义的平面文件的路径。 你可以在WebAmin控制台中点击文件名来修改这个设置。<br/>每一行组文件应当包含一个组名， 组名后面跟一个冒号，并在冒号后面使用空格来分割组中的用户名。 例如: <blockquote><code>testgroup: user1 user2 user3</code></blockquote>', '', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT,$VH_ROOT的相对路径。', '');

$_tipsdb['HANDLER_RESTART'] = new DAttrHelp("Hook::HANDLER_RESTART Priority", 'Sets the priority for this module callback within the HTTP Handler Restart Hook.<br/>   The HTTP Handler Restart Hook is triggered when the web server needs to discard the current response and start processing from beginning, for example, when an internal redirect has been requested.<br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['HTTP_AUTH'] = new DAttrHelp("Hook::HTTP_AUTH Priority", 'Sets the priority for this module callback within the HTTP Authentication Hook.<br/>  The HTTP Authentication Hook is triggered after resource mapping and before handler processing.  It occurs after HTTP built-in authentication, and can be used to perform additional authentication checking.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['HTTP_BEGIN'] = new DAttrHelp("Hook::HTTP_BEGIN Priority", 'Sets the priority for this module callback within the HTTP Begin Hook.<br/>   The HTTP Begin Hook is triggered when the TCP/IP connection begins an HTTP Session.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['HTTP_END'] = new DAttrHelp("Hook::HTTP_END Priority", 'Sets the priority for this module callback within the HTTP Session End Hook. <br/><br/>The HTTP Session End Hook is triggered when the HTTP connection has ended.     <br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['L4_BEGINSESSION'] = new DAttrHelp("Hook::L4_BEGINSESSION Priority", 'Sets the priority for this module callback within the L4 Begin Session Hook.<br/>  The L4 Begin Session Hook is triggered when the TCP/IP connection begins.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['L4_ENDSESSION'] = new DAttrHelp("Hook::L4_ENDSESSION Priority", 'Sets the priority for this module callback within the L4 End Session Hook.<br/>   The L4 End Session Hook is triggered when the TCP/IP connection ends.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['L4_RECVING'] = new DAttrHelp("Hook::L4_RECVING Priority", 'Sets the priority for this module callback within the L4 Receiving Hook.<br/>   The L4 Receiving Hook is triggered when the TCP/IP connection receives data.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['L4_SENDING'] = new DAttrHelp("Hook::L4_SENDING Priority", 'Sets the priority for this module callback within the L4 Sending Hook.<br/>  The L4 Sending Hook is triggered when the TCP/IP connection sends data.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['MAIN_ATEXIT'] = new DAttrHelp("Hook::MAIN_ATEXIT Priority", 'Sets the priority for this module callback within the Main At Exit Hook. <br/><br/>The Main At Exit Hook is triggered by the main (controller) process just prior to exiting. It is the last hook point to be called by the main process.   <br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['MAIN_INITED'] = new DAttrHelp("Hook::MAIN_INITED Priority", 'Sets the priority for this module callback within the Main Initialized Hook. <br/><br/>The Main Initialized Hook is triggered once upon startup, after the server configuration and  initialization is completed by the main (controller) process, and before any requests are serviced.   <br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['MAIN_POSTFORK'] = new DAttrHelp("Hook::MAIN_POSTFORK Priority", 'Sets the priority for this module callback within the Main Postfork Hook. <br/><br/>The Main Postfork Hook is triggered by the main (controller) process immediately after  a new worker process has been started (forked). This is called for each worker, and may happen during  system startup, or if a worker has been restarted.   <br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['MAIN_PREFORK'] = new DAttrHelp("Hook::MAIN_PREFORK Priority", 'Sets the priority for this module callback within the Main Prefork Hook. <br/><br/>The Main Prefork Hook is triggered by the main (controller) process immediately before  a new worker process is started (forked). This is called for each worker, and may happen during  system startup, or if a worker has been restarted.   <br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['RCVD_REQ_BODY'] = new DAttrHelp("Hook::RCVD_REQ_BODY Priority", 'Sets the priority for this module callback within the HTTP Received Request Body Hook.  <br/><br/>The HTTP Received Request Body Hook is triggered when the web server finishes receiving request body data.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['RCVD_RESP_BODY'] = new DAttrHelp("Hook::RCVD_RESP_BODY Priority", 'Sets the priority for this module callback within the HTTP Received Response Body Hook.  <br/><br/>The HTTP Received Response Body Hook is triggered when the web server backend finishes receiving the response body.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['RECV_REQ_BODY'] = new DAttrHelp("Hook::RECV_REQ_BODY Priority", 'Sets the priority for this module callback within the HTTP Receive Request Body Hook.  <br/><br/>The HTTP Receive Request Body Hook is triggered when the web server receives request body data.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['RECV_REQ_HEADER'] = new DAttrHelp("Hook::RECV_REQ_HEADER Priority", 'Sets the priority for this module callback within the HTTP Receive Request Header Hook.<br/>   The HTTP Receive Request Header Hook is triggered when the web server receives a request header.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['RECV_RESP_BODY'] = new DAttrHelp("Hook::RECV_RESP_BODY Priority", 'Sets the priority for this module callback within the HTTP Receive Response Body Hook.  <br/><br/>The HTTP Receive Response Body Hook is triggered when the web server backend receives the response body.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['RECV_RESP_HEADER'] = new DAttrHelp("Hook::RECV_RESP_HEADER Priority", 'Sets the priority for this module callback within the HTTP Receive Response Header Hook.  <br/><br/>The HTTP Receive Response Header Hook is triggered when the web server creates the response header.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['SEND_RESP_BODY'] = new DAttrHelp("Hook::SEND_RESP_BODY Priority", 'Sets the priority for this module callback within the HTTP Send Response Body Hook. <br/><br/>The HTTP Send Response Body Hook is triggered when the web server is going to send the response body.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['SEND_RESP_HEADER'] = new DAttrHelp("Hook::SEND_RESP_HEADER Priority", 'Sets the priority for this module callback within the HTTP Send Response Header Hook. <br/><br/>The HTTP Send Response Header Hook is triggered when the web server is ready to send the response header.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['UDBgroup'] = new DAttrHelp("组", '该用户所属的组的逗号分隔列表。 用户将只能访问属于这些组的资源。 如果在此处添加了组信息，则此信息将用于资源授权，并且涉及该用户的任何组数据库设置都将被忽略。', '', '', '');

$_tipsdb['UDBpass'] = new DAttrHelp("新密码", '密码可以是任意长度，可以包含任何字符。', '', '', '');

$_tipsdb['UDBpass1'] = new DAttrHelp("重新输入密码", '密码可以是任意长度，可以包含任何字符。', '', '', '');

$_tipsdb['UDBusername'] = new DAttrHelp("用户名", '仅包含字母和数字的用户名。（无特殊字符）', '', '', '');

$_tipsdb['URI_MAP'] = new DAttrHelp("Hook::URI_MAP Priority", 'Sets the priority for this module callback within the HTTP URI Map Hook.<br/>  The HTTP URI Map Hook is triggered when the web server maps a URI request to a context.  <br/><br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['VHlsrecaptcha'] = new DAttrHelp("reCAPTCHA 保护", 'reCAPTCHA 保护是一种减轻服务器负载的服务。在下列情况发生后，reCAPTCHA保护将激活 激活后,所以不受信任的客户端(可自定)发出的请求将被重定向到reCAPTCHA验证页面 验证完成后客户端将被重定向到其所需的页面<br/><br/>下列情况将启用reCAPTCHA保护: 1. 服务器或虚拟主机并发请求计数超过连接限制。<br/>2. 启用了Anti-DDoS，并且客户端以可疑的方式访问了URL。 客户端将首先重定向到reCAPTCHA，而不是在触发时被拒绝。<br/>3. 提供了新的重写规则，以通过重写规则激活reCAPTCHA。可以设置“verifycaptcha”将客户端重定向到reCAPTCHA。可以设置一个特殊值&#039;:deny&#039;以在客户端失败太多次时拒绝它。例如，[E=verifycaptcha]将始终重定向到reCAPTCHA，直到通过验证。 [E=verifycaptcha: deny]将重定向到reCAPTCHA，如果客户端达到最大尝试次数,将被拒绝。', '', '', '');

$_tipsdb['WORKER_ATEXIT'] = new DAttrHelp("Hook::WORKER_ATEXIT Priority", 'Sets the priority for this module callback within the Worker At Exit Hook. <br/><br/>The Worker At Exit Hook is triggered by a worker process just prior to exiting. It is the last hook point to be called by a worker.   <br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['WORKER_POSTFORK'] = new DAttrHelp("Hook::WORKER_POSTFORK Priority", 'Sets the priority for this module callback within the Worker Postfork Hook. <br/><br/>The Worker Postfork Hook is triggered by a worker process after being created by the main (controller) process.  Note that a corresponding Main Postfork Hook may be called by the main process either before or after this callback.   <br/>It will only take effect if the module has a hook point here. If it is not set, the priority will be the default value defined in the module.', '', 'Integer value from -6000 to 6000. Lower value means higher priority.', '');

$_tipsdb['accessAllowed'] = new DAttrHelp("允许访问列表", '指定允许访问这个context下资源的IP地址或子网。综合 &quot;拒绝访问列表&quot;项的配置以及服务器/虚拟主机级别访问控制， 可访问性将以客户端IP所符合的最小范围来确定。', '', 'Comma-delimited list of IPs/sub-networks.', '网络可以写成192.168.1.0/255.255.255.0, 192.168.1 或192.168.1.*。');

$_tipsdb['accessControl'] = new DAttrHelp("登入限制", '指定哪些子网络和/或IP地址可以访问该服务器。 这是影响所有的虚拟主机的服务器级别设置。您还可以为每个虚拟主机设置登入限制。虚拟主机的设置不会覆盖服务器设置。<br/><br/>是否阻止/允许一个IP是由允许列表与阻止列表共同决定。 如果你想阻止某个特定IP或子网，请在&quot;允许列表&quot;中写入* 或 ALL，并在&quot;拒绝列表&quot;中写入需要阻止的IP或子网。 如果你想允许某个特定的IP或子网，请在&quot;拒绝列表&quot;中写入* 或 ALL，并在&quot;允许列表&quot;中写入需要允许的IP或子网。 单个IP地址是被允许访问还是禁止访问取决于该IP符合的最小限制范围。<br/><br/>信任的IP或子网络可以在&quot;允许列表&quot;列表中添加后缀“T”来指定。受信任的IP或子网不受连接数/流量限制。 只有服务器级别的登入限制才可以设置受信任的IP或子网。', '[安全建议] 用此项设置适用于所有虚拟主机的常规限制。', '', '');

$_tipsdb['accessControl_allow'] = new DAttrHelp("允许列表", '指定允许的IP地址或子网的列表。 可以使用{VAL}*或{VAL}ALL。', '[安全建议] 在服务器级别设置的受信任的IP或子网不受连接/节流限制。', '逗号分隔的IP地址或子网列表。 结尾加上“T”可以用来表示一个受信任的IP或子网，如{VAL}192.168.1.*T。', '子网: 192.168.1.0/255.255.255.0, 192.168.1.0/24, 192.168.1 或 192.168.1.*. <br/>IPv6 地址: ::1 或 [::1] <br/>IPv6 子网: 3ffe:302:11:2:20f:1fff:fe29:717c/64 或  [3ffe:302:11:2:20f:1fff:fe29:717c]/64.');

$_tipsdb['accessControl_deny'] = new DAttrHelp("拒绝列表", '指定不允许的IP地址或子网的列表。', '', '逗号分隔的IP地址或子网列表。 可以使用{VAL}*或{VAL}ALL。', '子网: 192.168.1.0/255.255.255.0, 192.168.1.0/24, 192.168.1 或 192.168.1.*. <br/>IPv6 地址: ::1 或 [::1] <br/>IPv6 子网: 3ffe:302:11:2:20f:1fff:fe29:717c/64 或  [3ffe:302:11:2:20f:1fff:fe29:717c]/64.');

$_tipsdb['accessDenied'] = new DAttrHelp("拒绝访问列表", '指定哪个IP地址或子网不被允许访问这个context下的资源。 综合&quot;允许访问列表&quot;项的配置以及服务器/虚拟主机级别访问控制， 可访问性将以客户端IP所符合的最小范围来确定。', '', 'Comma-delimited list of IPs/sub-networks.', '子网络可以写成192.168.1.0/255.255.255.0, 192.168.1 或192.168.1.*。');

$_tipsdb['accessDenyDir'] = new DAttrHelp("拒绝访问的目录", '指定应该拒绝访问的目录。 将包含敏感数据的目录加入到这个列表，以防止向客户端意外泄露敏感文件。 在路径后加一个“*”，可包含所有子目录。 如果&quot;跟随符号链接&quot;和&quot;检查符号链接&quot;都被启用， 符号链接也将被检查是否在被拒绝访问目录中。', '[安全建议] 至关重要: 此设置只能防止服务这些目录中的静态文件。 这不能防止外部脚本如PHP、Ruby、CGI造成的泄露。', 'Comma-delimited list of directories', '');

$_tipsdb['accessLog_bytesLog'] = new DAttrHelp("字节记录", '指定带宽字节日志文件的路径。设置后，将创建一份兼容cPanel面板的带宽日志。这将记录 一个请求传输的总字节数，包括请求内容和响应内容。', '[性能建议] 将日志文件放置在一个单独的磁盘上。', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['accessLog_fileName'] = new DAttrHelp("文件名", '指定访问日志文件的文件名。', '[性能建议] 将访问日志文件放置在一个单独的磁盘上。', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['accessLog_logFormat'] = new DAttrHelp("日志格式", ' 指定访问日志的格式。 设置之后，它将覆盖&quot;记录头部&quot; 的设定。', '', '字符串。日志格式的语法与Apache 2.0自定义 <a href="http://httpd.apache.org/docs/current/mod/mod_log_config.html#formats" target="_blank" rel="noopener noreferrer">log format</a>.', '<b>一般日志格式（CLF）</b><br/>    &quot;%h %l %u %t \&quot;%r\&quot; %>s %b&quot;<br/><br/><b>支持虚拟主机的一般日志格式</b><br/>    &quot;%v %h %l %u %t \&quot;%r\&quot; %>s %b&quot;<br/><br/><b>NCSA扩展/组合日志格式</b><br/>    &quot;%h %l %u %t \&quot;%r\&quot; %>s %b \&quot;%{Referer}i\&quot; \&quot;%{User-agent}i\&quot; <br/><br/><b>记录Foobar的cookie值</b><br/>    &quot;%{Foobar}C&quot;');

$_tipsdb['accessLog_logHeader'] = new DAttrHelp("记录头部", '指定是否记录HTTP请求头: Referer、 UserAgent和Host。', '[性能建议] 如果你不需要在访问日志中记录这些头部信息，关闭这个功能。', '从复选框中选择', '');

$_tipsdb['accessLog_pipedLogger'] = new DAttrHelp("管道日志记录", '指定外部应用程序，该应用程序将通过其STDIN流（文件句柄为0）上的pipe接收LiteSpeed发送的访问日志数据。 指定此字段后，访问日志将仅发送到记录器应用程序，而不发送到上一个条目中指定的访问日志文件。<br/><br/>首先必须在&quot;外部应用&quot;中定义记录器应用程序。 服务器级别的访问日志记录只能使用在服务器级别定义的外部记录器应用程序。 虚拟主机级别的访问日志记录只能使用在虚拟主机级别定义的记录器应用程序。<br/><br/>记录器进程的启动方式与其他外部进程相同 (CGI/FastCGI/LSAPI)过程。这意味着它将作为 在虚拟主机的&quot;外部应用程序设置UID模式&quot;设置中指定的用户ID，并且永远不会以特权用户运行。<br/><br/>如果配置了多个记录器的实例，LiteSpeed web server将在多个记录器之间执行简单的负载均衡。LiteSpeed服务器始终尝试将记录器应用程序的数量保持在尽可能低的水平。只有当一个记录器应用程序未能及时处理访问日志条目时，服务器才会尝试启动记录器的另一个实例。<br/><br/>如果记录器崩溃，Web服务器将启动另一个实例，但是流缓冲区中的日志数据将丢失。 如果外部记录器无法跟上日志流的速度和数量，则可能会丢失日志数据。', '', '从列表中选择', '');

$_tipsdb['aclogUseServer'] = new DAttrHelp("日志管理", '指定写入访问日志的地点。这里有三个选项： <ol> <li>写入到服务器的访问日志；</li> <li>为虚拟主机创建一个访问日志；</li> <li>禁用访问日志记录</li> </ol>', '', '从列表中选择', '');

$_tipsdb['addDefaultCharset'] = new DAttrHelp("添加默认的字符集", 'S指定当内容类型是&quot;text/html&quot;或&quot;text/plain&quot;且没有参数时，是否添加字符集标记到&quot;Content-Type&quot;响应报头中。当设置为Off时，该功能禁用。当设置为On时，将添加&quot;自定义默认字符集&quot;中指定的字符集，如果没有指定，将添加默认的&quot;iso-8859-1&quot;字符集。', '', '从单选框选择', '');

$_tipsdb['addMIMEType'] = new DAttrHelp("MIME类型", '为此context指定的其他MIME类型的映射.  新映射将覆盖此context及其子context下的现有映射。<br/>	   如果要将PHP脚本显示为文本文件而不是作为脚本执行，则只需覆盖.php映射到MIME类型&quot;text/plain&quot;', '', 'MIME-type1 extension extension ..., MIME-type2 extension ... 		Use comma to separate between MIME types, use space to 		separate multiple extensions.', 'image/jpg jpeg jpg, image/gif gif');

$_tipsdb['addonmodules'] = new DAttrHelp("附加模块", '选择您想使用的附加模块。 如果你想使用一个没有在这里列出的版本，你可以手动更新源代码。(源代码的位置会在PHP构建的这一步中显示出来。)', '', '从复选框中选择', '');

$_tipsdb['adminEmails'] = new DAttrHelp("管理员电子邮箱", '指定服务器管理员的电子邮箱地址。 如果设置了电子邮箱地址，管理员将收到重要事件的电子邮件通知（例如， LiteSpeed服务因崩溃而自动重启或者授权即将过期）。', '电子邮件提醒功能只有在服务器有活动的MX服务器（如postfix、exim或sendmail）的情况下才会生效。', 'Comma delimited list of email address.', '');

$_tipsdb['adminUser'] = new DAttrHelp("网络管理员用户", 'Change the username and password for the WebAdmin Console.  The old password must be entered and verified in order to save changes.', '', '', '');

$_tipsdb['allowBrowse'] = new DAttrHelp("访问权限", '指定是否可以访问此context 。 设置为否以拒绝访问。 您可以使用此功能来防止访问指定目录。 您可以在更新此context内容或此目录中有特殊数据时使用它。', '', '从单选框选择', '');

$_tipsdb['allowQuic'] = new DAttrHelp("打开HTTP3/QUIC (UDP) 端口", '允许对映射到该监听器的虚拟主机使用HTTP3/QUIC网络协议. 为了使此设置生效，还必须在服务器级别将&quot;启用HTTP3/QUIC&quot;设置为是。 默认值为是。', '当此设置设置为是时，仍可以通过&quot;Enable HTTP3/QUIC&quot;设置在虚拟主机级别禁用HTTP3/QUIC。', '', '');

$_tipsdb['allowSetUID'] = new DAttrHelp("允许设置UID", '指定CGI脚本是否允许设置UID。如果允许，并且为CGI脚本启用了设置UID，那么无论CGI脚本是代表哪个用户启动的，CGI进程的用户ID都将切换为CGI脚本所有者的用户ID。<br/>默认值是 &quot;关&quot', '[安全建议] 尽可能不要允许CGI脚本设置UID,因为这存在着安全风险', '从单选框选择', '');

$_tipsdb['allowSymbolLink'] = new DAttrHelp("跟随符号链接", '指定在这个虚拟主机内是否要跟随符号链接。 If Owner Match选项启用后，只有在链接和目标属主一致时才跟踪符号链接。 此设置将覆盖默认的服务器级设置。', '[性能和安全性建议] 为了更好的安全性，请禁用此功能。为了获得更好的性能，启用它。', '从列表中选择', '');

$_tipsdb['appServerContext'] = new DAttrHelp("App Server Context", 'An App Server Context provides an easy way to configure a Ruby Rack/Rails, WSGI, or Node.js application. To add a an application through an App Server Context, only mounting the URL and the application&#039;s root directory is required. There is no need to go through all the trouble to define an external application, add a 404 handler, and rewrite rules, etc.', '', '', '');

$_tipsdb['appType'] = new DAttrHelp("应用类型", '用于此context的应用程序类型。 支持Rack/Rails, WSGI, 和 Node.js。', '', '', '');

$_tipsdb['appserverEnv'] = new DAttrHelp("运行模式", '指定Rack/Rails的运行模式:&quot;发展&quot;,  &quot;生产&quot;或者是&quot;预发布&quot;. 默认为 &quot;生产&quot;.', '', '从列表中选择', '');

$_tipsdb['as_location'] = new DAttrHelp("Location", '指定此context在文件系统中的对应位置。<br/><br/>Default value: $DOC_ROOT + &quot;URI&quot;', '', 'It can be an absolute path or path relative to $SERVER_ROOT, $VH_ROOT, or $DOC_ROOT. $DOC_ROOT is the default relative path, and can be omitted.<br/><br/>If the &quot;URI&quot; is a regular expression, then the matched sub-string can be used to form the &quot;Root&quot; string. The matched sub-string can be referenced with the values &quot;$1&quot; - &quot;$9&quot;. &quot;$0&quot; and &quot;&&quot; can be used to reference the whole matched string. Additionally, a query string can be set by appending a &quot;?&quot; followed by the query string. Be careful. &quot;&&quot; should be escaped as &quot;\&&quot; in the query string.', 'A plain URI like /examples/ with &quot;位置&quot; set to /home/john/web_examples will map the request &quot;/examples/foo/bar.html&quot; to file &quot;/home/john/web_examples/foo/bar.html&quot;.<br/>To simulate Apache&#039;s mod_userdir, set URI to exp: ^/~([A-Za-z0-9]+)(.*), set &quot;位置&quot; to /home/$1/public_html$2. With these settings, a request of URI /~john/foo/bar.html will map to file /home/john/public_html/foo/bar.html.');

$_tipsdb['as_startupfile'] = new DAttrHelp("启动文件", '用于启动应用程序的文件,路径应相对于应用程序根目录。<br/><br/>默认的启动文件包括Rack/Rails的&#039;config.ru&#039;, WSGI的&#039;wsgi.py&#039;和&#039;passenger_wsgi.py&#039;, NodeJS的&#039;app.js&#039;.', '', 'Path relative to application root directory', '');

$_tipsdb['authName'] = new DAttrHelp("认证名称", '为当前context下的realm认证指定一个替代的名称。 如果没有指定，原realm名称将被使用。 认证名称将显示在浏览器登陆弹出窗口。', '', '', '');

$_tipsdb['autoFix503'] = new DAttrHelp("自动修复503错误", '指定是否尝试通过平滑重启LiteSpeed修复“503 服务不可用”错误。“503”错误通常是由 发生故障的外部应用程序引起的，Web服务器重新启动往往可以临时修复 错误。如果启用，当30秒内出现超过30次“503”错误时，服务器将自动 重新启动。此功能是默认启用的。', '', '从单选框选择', '');

$_tipsdb['autoIndex'] = new DAttrHelp("自动索引", '在目录中，当&quot;索引文件&quot;中所列的索引文件不可用时，指定运行时是否即时生成目录索引。<br/>此选项可以在虚拟主机级别和context级别中设置，并可以顺着目录树继承，直到被覆盖。 您可以自定义生成的索引页面。请访问在线百科了解如何操作。', '[安全建议] 建议关闭自动索引，从而尽可能防止泄露机密数据。', '从单选框选择', '');

$_tipsdb['autoIndexURI'] = new DAttrHelp("自动索引URI", '在目录中，当&quot;索引文件&quot;中所列出的索引文件（index）不可用时，指定用来生成索引页面的URI。 LiteSpeed Web服务器使用一个外部脚本来生成索引页面，从而为定制提供最大的灵活性。 默认的脚本生成一个类似于Apache的索引页面。 定制生成的索引页，请访问在线百科。 被索引的目录通过一个环境变量 &quot;LS_AI_PATH&quot;来传递给脚本。', '', 'URI', '');

$_tipsdb['autoLoadHtaccess'] = new DAttrHelp("自动加载.htaccess", '如果使用<b>rewritefile</ b>指令的目录的HttpContext不存在，则在初次访问该目录时自动加载.htaccess文件中包含的重写规则。 最初加载后，必须执行正常重启才能使对该.htaccess文件的修改生效。<br/><br/>虚拟主机级别设置将覆盖服务器级别设置。 默认值：<br/><br/><b>Server-level:</b> No<br/><br/><b>VH-Level:</b> Inherit Server-level setting', '', '从单选框选择', '');

$_tipsdb['autoStart'] = new DAttrHelp("开机自启", '指定是否要Web服务器自动启动应用程序。只有运行在同一台机器上的FastCGI和LSAPI应用程序才能自动启动。 &quot;地址&quot;中的IP必须是本地IP. 通过LiteSpeed CGI守护进程而不是主服务器进程启动,有助于减少系统开销。<br/><br/>Default value: Yes (Through CGI Daemon)', '', '从列表中选择', '');

$_tipsdb['backlog'] = new DAttrHelp("Back Log", 'Specifies the backlog of the listening socket.  Required if &quot;开机自启&quot; is enabled.', '', '整数', '');

$_tipsdb['banPeriod'] = new DAttrHelp("禁止期（秒）", '指定在&quot;宽限期（秒）&quot;之后，如果连接数仍然高于 &quot;连接软限制&quot;，来自该IP的新连接将被拒绝多长时间。如果IP 经常被屏蔽，我们建议您延长禁止期以更强硬地惩罚滥用.', '', '整数', '');

$_tipsdb['binPath'] = new DAttrHelp("二进制文件路径", 'App Server的二进制文件的位置。', '', '', '');

$_tipsdb['blockBadReq'] = new DAttrHelp("封锁坏请求", '封锁持续发送坏HTTP请求的IP&quot;禁止期（秒）&quot;所设置的时长。默认为{VAL}Yes。 这有助于封锁反复发送垃圾请求的僵尸网络攻击。', '', '从单选框选择', '');

$_tipsdb['brStaticCompressLevel'] = new DAttrHelp("Brotli 压缩等级 (静态文件)", '指定Brotli压缩静态内容的级别。 范围从1 (最低)到9 (最高)。<br/><br/>当设置为 0时, brotli压缩将在全局禁用。<br/><br/>Default value: 5', '[性能建议] 压缩可以用来节省网络带宽。  基于文本的响应（例如html，css和javascript文件）效果最好，平均可以将其压缩为原始大小的一半。', 'Number between 0 and 11.', '');

$_tipsdb['bubbleWrap'] = new DAttrHelp("Bubblewrap Container", 'Set to On if you wish to start CGI processes (including PHP programs) in a bubblewrap sandbox. See <a href="   https://wiki.archlinux.org/title/Bubblewrap " target="_blank" rel="noopener noreferrer">   https://wiki.archlinux.org/title/Bubblewrap </a> for details on using bubblewrap. Bubblewrap must be installed on your system prior to using this setting.<br/><br/>This setting cannot be turned on at the Virtual Host level if set to &quot;Disabled&quot; at the Server level.<br/><br/>Default values:<br/><b>Server level:</b> Disabled<br/><b>VH level:</b> Inherit Server level setting', '', '从列表中选择', '');

$_tipsdb['bubbleWrapCmd'] = new DAttrHelp("Bubblewrap Command", 'bubblewraps使用的完整的命令, 包括bubblewrap程序本身。 有关配置此命令的更多信息，请参见： <a href="   https://openlitespeed.org/kb/bubblewrap-in-openlitespeed/ " target="_blank" rel="noopener noreferrer">   https://openlitespeed.org/kb/bubblewrap-in-openlitespeed/ </a>. 如果未指定，将使用下面列出的默认命令。<br/><br/>默认值: /bin/bwrap --ro-bind /usr /usr --ro-bind /lib /lib --ro-bind-try /lib64 /lib64 --ro-bind /bin /bin --ro-bind /sbin /sbin --dir /var --dir /tmp --proc /proc --symlink../tmp var/tmp --dev /dev --ro-bind-try /etc/localtime /etc/localtime --ro-bind-try /etc/ld.so.cache /etc/ld.so.cache --ro-bind-try /etc/resolv.conf /etc/resolv.conf --ro-bind-try /etc/ssl /etc/ssl --ro-bind-try /etc/pki /etc/pki --ro-bind-try /etc/man_db.conf /etc/man_db.conf --ro-bind-try /home/$USER /home/$USER --bind-try /var/lib/mysql/mysql.sock /var/lib/mysql/mysql.sock --bind-try /home/mysql/mysql.sock /home/mysql/mysql.sock --bind-try /tmp/mysql.sock /tmp/mysql.sock  --unshare-all --share-net --die-with-parent --dir /run/user/$UID ‘$PASSWD 65534’ ‘$GROUP 65534’', '', '', '');

$_tipsdb['certChain'] = new DAttrHelp("证书链", '指定证书是否为证书链。 存储证书链的文件必须为PEM格式， 并且证书必须按照从最低级别（实际的客户端或服务器证书）到最高级别（Root）CA的链接顺序。', '', '从单选框选择', '');

$_tipsdb['certFile'] = new DAttrHelp("证书文件", 'SSL证书文件的文件名。', '[安全建议] 私钥文件应放在一个安全的目录中，该目录应 允许对运行服务器的用户具有只读的访问权限。', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['cgiContext'] = new DAttrHelp("CGI Context", 'A CGI context defines scripts in a particular directory as CGI scripts. This directory can be inside or outside of the document root. When a file under this directory is requested, the server will always try to execute it as a CGI script, no matter if it&#039;s executable or not. In this way, file content under a CGI Context is always protected and cannot be read as static content. It is recommended that you put all your CGI scripts in a directory and set up a CGI Context to access them.', '', '', '');

$_tipsdb['cgiResource'] = new DAttrHelp("CGI Settings", 'The following settings control CGI processes. Memory and process limits also serve as the default for other external applications if limits have not been set explicitly for those applications.', '', '', '');

$_tipsdb['cgi_path'] = new DAttrHelp("路径", '指定CGI脚本的位置.', '', 'The path can be a directory that contains a group of CGI scripts, like $VH_ROOT/myapp/cgi-bin/. In this case, the context &quot;URI&quot; must end with &quot;/&quot;, like /app1/cgi/. The Path can also specify only one CGI script, like $VH_ROOT/myapp/myscript.pl. This script should have the corresponding &quot;URI&quot; /myapp/myscript.pl.', '');

$_tipsdb['cgidSock'] = new DAttrHelp("CGI守护进程套接字", '用于与CGI守护进程沟通的唯一套接字地址。为了 最佳性能和安全性，LiteSpeed服务器使用一个独立的CGI 守护进程来产生CGI脚本的子进程。 默认套接字是“uds://$SERVER_ROOT/admin/conf/.cgid.sock”。 如果你需要放置在另一个位置，在这里指定一​​个Unix域套接字。', '', 'UDS://path', 'UDS://tmp/lshttpd/cgid.sock');

$_tipsdb['cgroups'] = new DAttrHelp("cgroups", '如果当前操作系统支持(目前支持RedHat/Centos Linux v7.5+和Ubuntu 18.04+)，则将cgroup设置应用于此CGI进程。 。 当前执行的用户将用于确定要应用的cgroup配置。<br/>在服务器级别将此设置为Disabled将在服务器范围内禁用此设置。 在其他情况下，可以在虚拟主机级别覆盖服务器级别的设置。<br/><br/>默认值:<br/><b>服务器级别:</b> Off<br/><b>虚拟主机级别:</b> 继承服务器级别设置', '', '从列表中选择', '');

$_tipsdb['checkSymbolLink'] = new DAttrHelp("检查符号链接", '指定在启用了&quot;跟随符号链接&quot;时，是否检查符号链接在不在&quot;拒绝访问的目录&quot;中。 如果启用检查，将检查网址对应的真正的资源路径是否在配置的禁止访问目录中。 如果在禁止访问目录中，访问将被禁止。', '[性能和安全] 要获得最佳的安全性，启用该选项。要获得最佳性能，禁用该选项。', '从单选框选择', '');

$_tipsdb['ciphers'] = new DAttrHelp("密码套件", 'Specifies the cipher suite to be used when negotiating the SSL handshake. LSWS supports cipher suites implemented in SSL v3.0, TLS v1.0, TLS v1.2, and TLS v1.3.', ' We recommend leaving this field blank to use our default cipher which follows SSL cipher best practices.', 'Colon-separated string of cipher specifications.', 'ECDHE-RSA-AES128-SHA256:RC4:HIGH:!MD5:!aNULL:!EDH');

$_tipsdb['clientVerify'] = new DAttrHelp("Client Verification", ' Specifies the type of client certifcate authentication. Available types are: <ul> <li><b>None:</b> No client certificate is required.</li> <li><b>Optional:</b> Client certificate is optional.</li> <li><b>Require:</b> The client must has valid certificate.</li> <li><b>Optional_no_ca:</b> Same as optional.</li> </ul> The default is &quot;None&quot;.', '&quot;None&quot; or &quot;Require&quot; are recommended.', '从列表中选择', '');

$_tipsdb['compilerflags'] = new DAttrHelp("编译器标志", '添加其他编译器标志，例如优化的编译器选项。', '', '支持的标志有CFLAGS, CXXFLAGS, CPPFLAGS, LDFLAGS.使用空格分隔不同的标志。 对于标志值，请使用单引号（而不是双引号）', 'CFLAGS=&#039;-O3 -msse2 -msse3 -msse4.1 -msse4.2 -msse4 -mavx&#039;');

$_tipsdb['compressibleTypes'] = new DAttrHelp("压缩类型", '指定允许压缩哪些MIME类型。 保留此设置不变，或输入default以使用服务器内置的默认列表，该列表已经涵盖了大多数mime类型。<br/>Default value: text/*,application/x-javascript,application/javascript,application/xml,image/svg+xml,application/rss+xml, application/json,application/vnd.ms-fontobject,application/x-font,application/x-font-opentype, application/x-font-truetype,application/x-font-ttf,font/eot,font/opentype,font/otf,font/ttf,image/x-icon, image/vnd.microsoft.icon,application/xhtml+xml', '[性能建议] 只允许特定类型进行GZIP压缩。 二进制文件如gif/png/jpeg图片文件及flash文件无法从压缩中获益。', 'MIME type list separated by commas. Wild card &quot;*&quot; and negate sign &quot;!&quot; are allowed, such as text/*, !text/js.', 'If you want to compress text/* but not text/css, you can have a rule like text/*, !text/css. &quot;!&quot; will exclude that MIME type.');

$_tipsdb['configFile'] = new DAttrHelp("配置文件", '指定虚拟主机的配置文件名称。 配置文件必须位于$SERVER_ROOT/conf/vhosts/目录下。', '推荐使用$SERVER_ROOT/conf/vhosts/$VH_NAME/vhconf.conf。', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['configureparams'] = new DAttrHelp("配置参数", '为PHP构建配置参数。当您单击下一步时，Apache特定的参数和“--prefix”值将被自动删除，并且“--with-litespeed”将自动追加。（前缀可以在上面的字段中设置）通过这种方式，您可以简单地复制并粘贴现有PHP的phpinfo()输出的配置参数。', '', '空格分隔多个选项(带或不带双引号）', '');

$_tipsdb['connTimeout'] = new DAttrHelp("连接超时时间(secs)", '指定一个请求允许的最大连接空闲时间。 如果在这段时间内连接一直处于空闲状态(即没有I/O活动），则它将被关闭。', '[安全建议] 将值设置得尽可能低，在可能的拒绝服务攻击中，这可以帮助释放无效连接所占用的连接数。', '整数', '');

$_tipsdb['consoleSessionTimeout'] = new DAttrHelp("会话超时时长（秒）", '自定义WebAdmin控制台会话超时时间。 如果未设置任何值，则默认值60秒生效。', '[安全建议] 在生产环境中一般设置一个不超过300秒的合适值。', '整数', '');

$_tipsdb['cpuAffinity'] = new DAttrHelp("处理器亲和性", 'CPU关联将进程绑定到一个或多个CPU（核心）。 始终使用同一CPU对进程来说是有益的，因为这样进程可以利用CPU缓存中剩余的数据。 如果进程移至其他CPU，则不会使用CPU缓存，并且会产生不必要的开销。<br/><br/>CPU Affinity设置控制一个服务器进程将与多少个CPU（核心）相关联。 最小值为0，它将禁用此功能。 最大值是服务器具有的核心数。 通常，1是最佳设置，因为它会最严格地使用CPU亲和力，从而最大程度地利用CPU缓存。<br/><br/>Default value: 0', '', 'Integer value from 0 to 64. (0 will disable this feature)', '');

$_tipsdb['crlFile'] = new DAttrHelp("客户端吊销文件", ' Specifies the file containing PEM-encoded CA CRL files enumerating revoked client certificates. This can be used as an alternative or in addition to &quot;客户端吊销路径&quot;.', '', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['crlPath'] = new DAttrHelp("客户端吊销路径", ' Specifies the directory containing PEM-encoded CA CRL files for revoked client certificates. The files in this directory have to be PEM-encoded. These files are accessed through hash filenames, hash-value.rN. Please refer to openSSL or Apache mod_ssl documentation regarding creating the hash filename.', '', 'path', '');

$_tipsdb['ctxType'] = new DAttrHelp("Context类型", 'The type of context created determines it&#039;s usage.<br><br><b>Static</b> context can be used to map a URI to a directory either outside document root or within it.<br> <b>Java Web App</b> context is used to automatically import a predefined Java Application in an AJPv13 compilant Java servlet engine.<br> <b>Servlet</b> context is used to import a specific servlet under a web application.<br> <b>Fast CGI</b> context is a mount point of Fast CGI application.<br> <b>LiteSpeed SAPI</b> context can be used to associate a URI with an LSAPI application.<br> <b>Proxy</b> context enables this virtual host to serve as a transparant reverse proxy server to an external web server or application server.<br> <b>CGI</b> context can be used to specify a directory only contains CGI scripts.<br> <b>Load Balancer</b> context can be used to assign a different cluster for that context.<br> <b>Redirect</b> context can set up an internal or external redirect URI.<br> <b>App Server</b> context is specifically used for Rack/Rails, WSGI, and Node.js applications.<br> <b>Module handler</b> context is a mount point of hander type modules.<br>', '', '', '');

$_tipsdb['defaultCharsetCustomized'] = new DAttrHelp("自定义默认字符集", '指定一个字符集当&quot;添加默认的字符集&quot;是On时使用。这是可选的。默认值是iso-8859-1。当&quot;添加默认的字符集&quot;是Off时本设置将不生效。', '', 'Name of a character set.', 'utf-8');

$_tipsdb['defaultType'] = new DAttrHelp("默认MIME类型", '定后，当MIME类型映射不能由文档的后缀确定或没有后缀时，将使用此类型。如果未指定，将使用默认值application/octet-stream。', '', 'MIME-type', '');

$_tipsdb['destinationuri'] = new DAttrHelp("目标URI", '指定重定向的目标位置。 如果被重定向的URI映射到另一个重定向URI时，将再次被重定向。', '', '这个URI可以是一个在同一个网站上以&quot;/&quot;开始的相对URI， 或者是一个指向其他网站以&quot;http(s): //&quot;开始的绝对URI。 如果&quot;URI&quot;包含正则表达式，目标地址可以匹配变量，如$1或$2。', '');

$_tipsdb['disableInitLogRotation'] = new DAttrHelp("禁用初始日志轮换", '指定在启动时是否启用/禁用服务器错误日志文件的轮换。 使用值为“未设置”时，默认启用初始日志轮转。', '', '从单选框选择', '');

$_tipsdb['docRoot'] = new DAttrHelp("文档根目录", '指定此虚拟主机的文档根目录。 推荐使用$VH_ROOT/html。在context中，此目录可以用$DOC_ROOT来引用。', '', '可以说绝对路径,也可以是相对于$SERVER_ROOT或$VH_ROOT的相对路径。', '');

$_tipsdb['domainName'] = new DAttrHelp("域名", '指定映射域名。域名不区分大小写,如果带有&quot;www.&quot;, &quot;www.&quot;则会移除,允许使用通配符&quot;*&quot;和&quot;?&quot; &quot;?&quot;仅代表一个字符,&quot;*&quot;代表任意数量的字符, 不允许出现重复域名', ' If a listener is dedicated to one virtual host, always use * for the domain name to avoid unnecessary checking. Domain names with wildcard characters  (other than the catchall domain) should be avoided whenever possible.', 'Comma-separated list.', 'www?.example.com<br/>&quot;*.mydomain.com&quot; will match all subdomains of mydomain.com.<br/>&quot;*&quot; by itself is the catchall domain and will match any unmatched domain names.');

$_tipsdb['dynReqPerSec'] = new DAttrHelp("动态请求/秒", '指定每秒可处理的来自单个IP的动态请求的数量（无论与该IP之间建立了多少个连接） 当达到此限制时，所有后来的请求将被延滞到下一秒。<br/><br/>静态内容的请求限制与此限制无关。 可以在服务器或虚拟主机级别设置每个客户端请求的限制。 虚拟主机级别的设置将覆盖服务器级别的设置。', '[安全] 受信任的IP或子网不受影响。', '整数', '');

$_tipsdb['enableCoreDump'] = new DAttrHelp("启用Core Dump", '指定当服务由root用户启动时是否启用core dump。 对大多数现代的Unix系统，会更改用户ID或组ID的进程出于安全考虑不被允许产生core文件。但是core dump文件对于排查故障非常有用。 这个选项只能在Linux Kernel 2.4或更高版本中可用。 Solaris用户应当使用coreadm命令来控制这个功能。', '[安全建议] 仅在当你在服务器日志中看到没有创建core文件时启用。当产生core文件后立即关闭。Core文件产生后请提交bug报告。', '从单选框选择', '');

$_tipsdb['enableDHE'] = new DAttrHelp("启用DH密钥交换", '允许使用Diffie-Hellman密钥交换进行进一步的SSL加密。', '[安全建议] DH密钥交换比仅使用RSA密钥更安全。 ECDH和DH密钥安全性相同。<br/><br/>[x性能] 启用DH密钥交换将增加CPU负载，并且比ECDH密钥交换和RSA都慢。 如果可用，则首选ECDH密钥交换。', '从单选框选择', '');

$_tipsdb['enableDynGzipCompress'] = new DAttrHelp("启用GZIP动态压缩", '控制动态HTTP回应的GZIP压缩。 &quot;启用GZIP压缩&quot;必须设置为Yes来开启动态GZIP压缩。<br/>Default value: Yes', '[性能建议] 压缩动态回应将增加CPU和内存的使用，但可以节省网络带宽。', '从单选框选择', '');

$_tipsdb['enableECDHE'] = new DAttrHelp("启用ECDH密钥交换", '允许使用Diffie-Hellman密钥交换进行进一步的SSL加密。', '[安全建议] ECDH密钥交换比仅使用RSA密钥更安全。 ECDH和DH密钥交换安全性相同。<br/><br/>[性能] 启用ECDH密钥交换会增加CPU负载，并且比仅使用RSA密钥要慢。', '从单选框选择', '');

$_tipsdb['enableExpires'] = new DAttrHelp("启用过期", '指定是否为静态文件生成Expires头。如果启用，将根据 &quot;默认过期&quot;和&quot;按类型过期&quot;生成Expires头。<br/><br/>这可以在服务器，虚拟主机和Context级别设置。低级别的设置将 覆盖高级别的设置。例如，Context级别的设置将覆盖虚拟主机级别的设置， 虚拟主机级别的设置将覆盖服务器级别的设置。', '', '从单选框选择', '');

$_tipsdb['enableGzipCompress'] = new DAttrHelp("启用GZIP压缩", '制静态或动态HTTP回应的GZIP压缩。<br/><br/>Default value: Yes', '[性能建议] 开启该功能可以节省网络带宽。 针对基于文本的回应如html、css和javascript文件最有效果，一般可以压缩到原文件大小的一半大小。', '从单选框选择', '');

$_tipsdb['enableIpGeo'] = new DAttrHelp("启用IP地理定位", '指定是否启用IP地理定位查找。 可以在服务器级别，虚拟主机级别，或context级别设置。', '', '从单选框选择', '');

$_tipsdb['enableLVE'] = new DAttrHelp("Cloud-Linux", '指定当CloudLinux存在时是否启用CloudLinux的轻量级虚拟 环境（LVE）。您可以搭配使用Litespeed与LVE实现更好的资源管理。 欲了解更多信息，请访问 <a href="http://www.cloudlinux.com" target="_blank" rel="noopener noreferrer">http://www.cloudlinux.com</a>。', '', '从列表中选择', '');

$_tipsdb['enableRecaptcha'] = new DAttrHelp("启用reCAPTCHA", '必须先在服务器级别将此设置设置为是,才能在当前级别启用并使用reCAPTCHA保护功能。 <br/><br/>默认值:<br/><b>服务器级别:</b> 是<br/><b>虚拟主机级别:</b> 继承服务器级别设置', '', '从单选框选择', '');

$_tipsdb['enableRewrite'] = new DAttrHelp("启用重写", '指定是否启用LiteSpeed的URL重写. 可以在虚拟主机或context级别上自定义此选项， 并且沿目录树继承该选项，直到被其他选项覆。', '', '从单选框选择', '');

$_tipsdb['enableScript'] = new DAttrHelp("启用脚本", '指定在这个虚拟主机中是否允许运行脚本（非静态页面）。 如果禁用，CGI, FastCGI, LSAPI, Servlet引擎 和其他脚本语言都将在这个虚拟机中不被允许使用。 因此如果你希望使用一个脚本处理程序，你需要启用本项。', '', '从单选框选择', '');

$_tipsdb['enableSpdy'] = new DAttrHelp("启用 SPDY/HTTP2/HTTP3", '有选择地启用HTTP/3，HTTP/2和SPDY HTTP网络协议。<br/><br/>如果要禁用SPDY，HTTP/2和HTTP3，请选中“无”，并取消选中所有其他框。<br/>Default value: All enabled', '可以在侦听器和虚拟主机级别上设置此设置。', '从复选框中选择', '');

$_tipsdb['enableStapling'] = new DAttrHelp("启用 OCSP 装订", '确定是否启用OCSP装订，这是一种更有效的验证公钥证书的方式。', '', '从单选框选择', '');

$_tipsdb['enableh2c'] = new DAttrHelp("启用 明文TCP上的HTTP/2", '指定是否通过非加密TCP连接启用HTTP/2。 默认设置为禁用。', '', '从单选框选择', '');

$_tipsdb['env'] = new DAttrHelp("环境", '为外部应用程序指定额外的环境变量。', '', 'Key=value. Multiple variables can be separated by &quot;ENTER&quot;', '');

$_tipsdb['errCode'] = new DAttrHelp("错误代码", '指定错误页面的HTTP状态码。 只有特定的HTTP状态码才可以自定义错误页面。', '', '从列表中选择', '');

$_tipsdb['errPage'] = new DAttrHelp("自定义错误页面", '当服务器在处理请求时遇到问题， 服务器将向网络客户端返回错误代码和html页面作为错误消息。 错误代码在HTTP协议中定义（请参阅RFC 2616）。 LiteSpeed Web服务器为每个错误代码都有一个内置的默认错误页面，但是也可以为每个错误代码配置一个自定义页面。 甚至可以进一步自定义这些错误页面，以使其对于每个虚拟主机都是唯独特的。', '', '', '');

$_tipsdb['errURL'] = new DAttrHelp("URL", '指定自定义错误页的URL。 当返回相应HTTP状态时服务器会将请求转发到该URL。 如果此URL指向一个不存在的地址，自带的错误页面将被使用。 该URL可以是一个静态文件，动态生成的页面，或者其他网站的页面 （网址开头为&quot;http(s): //&quot;）。 当转发到在其他网站上的页面时，客户端会收到一个重定向状态码 来替代原本的状态码。', '', 'URL', '');

$_tipsdb['expWSAddress'] = new DAttrHelp("地址", '外部网络服务器使用的HTTP,HTTPS或Unix域套接字(UDS)地址。', '[安全建议] 如果代理到同一台机器上运行的另一台Web服务器，请将IP地址设置为localhost或127.0.0.1，这样从其他机器上就无法访问外部应用程序。', 'IPv4 或 IPV6 地址(:端口),UDS://path或unix:path 如果外部Web服务器使用HTTPS，则在前面加上 &quot;https://&quot;。  如果外部Web服务器使用标准端口80或443，则端口是可选的。', '192.168.0.10<br/>127.0.0.1:5434<br/>https://10.0.8.9<br/>https://127.0.0.1:5438<br/>UDS://tmp/lshttpd/php.sock<br/>unix:/tmp/lshttpd/php.sock');

$_tipsdb['expiresByType'] = new DAttrHelp("按类型过期", '为各个MIME类型分别指定Expires头设置。', '', '逗号分隔的“MIME-类型=A|M秒数”的列表。 文件将在基准时间（A|M）加指定秒数的时间后失效。<br/><br/>“A”代表基准时间为客户端的访问时间，“M”代表文件的最后修改时间。 MIME-类型可使用通配符“*”，如image/*。', '');

$_tipsdb['expiresDefault'] = new DAttrHelp("默认过期", '指定生成Expires头的默认设置。该设置在&quot;启用过期&quot; 设为“启用”时有效。它可以被&quot;按类型过期&quot;覆盖。 除非必要，否则不要在服务器或虚拟主机级别设置该默认值。 因为它会为所有网页生成Expires头。大多数时候，应该是 为不常变动的某些目录在Context级别设置。如果没有默认设置，&quot;按类型过期&quot;中未指定的类型不会生成Expires头。', '', 'A|Mseconds<br/>文件将在基准时间（A|M）加指定秒数的时间后失效。 “A”代表基准时间为客户端的访问时间，“M”代表文件的最后修改时间。', '');

$_tipsdb['expuri'] = new DAttrHelp("URI", 'Specifies the URI for this context.', '', '指定此context下的URI。这个URI应该以&quot;/&quot;开始。 如果一个URI以&quot;/&quot;结束，那么该context将包含这个URI下的所有下级URI。如果context类型映射到系统目录上,则必须添加结尾的&quot;/&quot;', '');

$_tipsdb['extAppAddress'] = new DAttrHelp("地址", '外部应用程序使用的唯一套接字地址。 支持IPv4/IPv6套接字和Unix域套接字（UDS）。 IPv4/IPv6套接字可用于网络通信。 只有当外部应用程序与服务器在同一台机器上时，才能使用UDS。', '[安全建议] 如果外部应用程序在同一台机器上运行，则首选UDS。如果必须使用IPv4|IPV6, 将IP地址设置为localhost或127.0.0.1，这样外部应用就无法从其他机器上访问。 [性能建议] Unix域套接字一般比IPv4套接字拥有更高的性能。', 'IPv4 或 IPV6 address:port 或者 UDS://path', '127.0.0.1:5434<br/>UDS://tmp/lshttpd/php.sock.');

$_tipsdb['extAppName'] = new DAttrHelp("名称", '此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。', '', '', '');

$_tipsdb['extAppPath'] = new DAttrHelp("命令", '指定包含外部应用程序的执行参数在内的完整命令行。  需要&quot;开机自启&quot;的值为enable.如果参数中包含空格或制表符,应使用双引号或单引号将其引起。', '', 'Full path to the executable with optional parameters.', '');

$_tipsdb['extAppPriority'] = new DAttrHelp("优先级", '指定外部应用的优先级,值的范围是-20到20.  一个小的数字意味着高优先级.外部应用进程的优先级不能比Web服务器高. 如果该优先级设置一个比服务器的优先级小的数字，则将使用服务器的优先级。', '', 'int', '');

$_tipsdb['extAppType'] = new DAttrHelp("类型", '指定外部应用程序的类型。 应用程序类型根据它们提供的服务或与服务器通信所使用的协议而有所不同。 从中选择 <ul>   <li>FastCGI: a FastCGI application with a Responder role.</li>   <li>FastCGI Authorizer: a FastCGI application with an Authorizer role</li>   <li>Servlet Engine: a servlet engine with an AJPv13 connector, such as Tomcat.</li>   <li>Web Server: a web server or application server that supports HTTP protocol.</li>   <li>LiteSpeed SAPI App: an application that communicates with the web server using LSAPI protocol.</li>   <li>Load Balancer: a virtual application that can balance load among worker applications.</li>   <li>Piped Logger: an application that can process access log entries received on its STDIN stream.</li> </ul>', 'Most applications will use either LSAPI or FastCGI protocol. LSAPI supports PHP, Ruby, and Python. Perl can be used with FastCGI. (PHP, Ruby, and Python can also be set up to run using FastCGI, but they run faster using LSAPI.) Java uses servlet engines.', '从列表中选择', '');

$_tipsdb['extAuthorizer'] = new DAttrHelp("Authorizer", '指定可用于生成授权/未授权 decisions的外部应用程序。 目前,仅FastCGI授权器可用。 有关FastCGI授权者角色的更多详细信息， 请访问<a href="https://fastcgi-archives.github.io/" target="_blank" rel="noopener noreferrer">https://fastcgi-archives.github.io/</a>。', '', '从列表中选择', '');

$_tipsdb['extGroup'] = new DAttrHelp("以特定组运行", '外部应用程序将作为此指定的组名运行。如果未设置，将继承虚拟主机级别的设置。<br/><br/>Default value: Not Set', '', 'Valid group name.', '');

$_tipsdb['extMaxIdleTime'] = new DAttrHelp("最大空闲时间", '指定服务器停止外部应用之前的最大空闲时间，以释放空闲资源。 当设置为&quot;-1&quot;时，服务器将不会停止外部应用，除非在ProcessGroup模式下运行, 而在ProcessGroup模式下,闲置的外部应用将在30秒后停止.<br/><br/>默认值为: -1{/val}', ' This feature is especially useful in the mass hosting environment where, in order to prevent files owned by one virtual host from being accessed by the external application scripts of another virtual host, many different applications are run at the same time in SetUID mode. Set this value low to prevent these external applications from idling unnecessarily.', 'Integer number', '');

$_tipsdb['extUmask'] = new DAttrHelp("umask", 'Sets default umask for this external application&#039;s processes. See  man 2 umask  for details. The default value taken from the server-level &quot;umask&quot; setting.', '', 'value valid range [000]-[777].', '');

$_tipsdb['extUser'] = new DAttrHelp("Run As User", 'The external application will run as this specified user name. If not set, Virtual Host level settings will be inherited.<br/><br/>Default value: Not Set', '', 'Valid username.', '');

$_tipsdb['extWorkers'] = new DAttrHelp("Workers", 'List of worker groups previously defined in the external load balancer.', '', 'A comma-separated list in the form ExternalAppType::ExternalAppName', 'fcgi::localPHP, proxy::backend1');

$_tipsdb['externalredirect'] = new DAttrHelp("外部重定向", '指定此重定向是否为外部重定向。 对于外部重定向,可以指定&quot;状态码&quot;，并且&quot;目标URI&quot;可以以“/”或“ http(s)://”开头。 对于内部重定向, &quot;目标URI&quot;必须以&quot;/&quot;开始', '', '', '');

$_tipsdb['extraHeaders'] = new DAttrHelp("标头控制", '指定要添加的附加响应/请求头。可以添加每行一个来添加多个头指令。&quot;NONE &quot;可以用来禁止父头继承。如果没有提供，则假定为 &quot;Header&quot;。', ' 语法和用法类似于 <a href="https://httpd.apache.org/docs/2.2/mod/mod_headers.html#header" target="_blank" rel="noopener noreferrer">Apache&#039;s mod_headers directives</a><br/><br/> The &#039;Header&#039; directive is is optional and can be excluded or left in when copying rules from elsewhere without issue.', '[Header]|RequestHeader [condition] set|append|merge|add|unset header [value] [early|env=[!]variable]', 'set Cache-control no-cache<br/>append Cache-control no-store<br/>Header set My-header cust_header_val<br/>RequestHeader set My-req-header cust_req_header_val');

$_tipsdb['extrapathenv'] = new DAttrHelp("额外的PATH环境变量", '将其他用于构建脚本的路径附加到当前PATH环境变量中。', '', '多个路径以“:”分隔', '');

$_tipsdb['fcgiContext'] = new DAttrHelp("FastCGI Context", 'FastCGI applications cannot be used directly. A FastCGI application must be either configured as a script handler or mapped to a URL through FastCGI context. A FastCGI context will associate a URI with a FastCGI application.', '', '', '');

$_tipsdb['fcgiapp'] = new DAttrHelp("FastCGI 应用程序", '指定FastCGI应用程序的名称。 必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义此应用程序。', '', '从列表中选择', '');

$_tipsdb['fileETag'] = new DAttrHelp("文件ETag", '指定是否使用一个文件的索引节点、最后修改时间和大小属性 生成静态文件的ETag HTTP响应头。 所有这三个属性是默认启用的。 如果您打算使用镜像服务器服务相同的文件，您应该不勾选索引节点。 否则，为同一个文件生成的ETag在不同的服务器上是不同的。', '', '从复选框中选择', '');

$_tipsdb['fileUpload'] = new DAttrHelp("文件上传", '通过使用请求正文解析器将文件解析到服务器本地目录中来上传文件时，提供了附加的安全功能。第三方模块可以轻松地在文件本地目录中扫描文件是否存在危害。 当启用&quot;通过文件路径传递上传数据&quot;或模块在LSI_HKPT_HTTP_BEGIN级别调用LSIAPI的set_parse_req_body时，将使用请求正文解析器。 源代码包中提供的API示例。', '', '', '');

$_tipsdb['followSymbolLink'] = new DAttrHelp("跟随符号链接", '指定服务静态文件时跟踪符号链接的服务器级别默认设置。<br/><br/>选项有Yes、If Owner Match和No。<br/><br/>Yes设置服务器始终跟踪符号链接。 If Owner Match设置服务器只有在链接和目标属主一致时才跟踪符号链接。 No表示服务器永远不会跟踪符号链接。 该设置可以在虚拟主机配置中覆盖，但不能通过.htaccess文件覆盖。', '[性能和安全建议] 要获得最佳安全性，选择{VAL}No或If Owner Match。 要获得最佳性能，选择{VAL}Yes。', '从列表中选择', '');

$_tipsdb['forceGID'] = new DAttrHelp("强制GID", '指定一组ID，以用于所有在suEXEC模式下启动的外部应用程序。 当设置为非零值时，所有suEXEC的外部应用程序（CGI、FastCGI、 LSAPI）都将使用该组ID。这可以用来防止外部应用程序访问其他用 户拥有的文件。<br/><br/>例如，在共享主机环境，LiteSpeed以“www-data”用户、“www-data”组 身份运行。每个文件根目录是由用户帐户所有，属组为“www-data”，权限 为0750。如果强制GID被设置为“nogroup”（或“www-data”之外的任何一 个组），所有suEXEC外部应用程序都将以特定用户身份运行，但属组为 “nogroup”。这些外部应用程序的进程依然能够访问属于相应用户的文件（ 因为他们的用户ID），但没有组权限访问其他人的文件。另一方面，服务器 仍然可以服务在任何用户文件根目录下的文件（因为它的组ID）。', '[安全建议] 设置足够高的值以排除所有系统用户所在的组。', '整数', '');

$_tipsdb['forceStrictOwnership'] = new DAttrHelp("强制严格属主检查", '指定是否执行严格的文件所有权检查。 如果启用，Web服务器将检查正在服务的文件的所有者与虚拟主机的所有者是否相同。 如果不同，将返回403拒绝访问错误。 该功能默认是关闭的。', '[安全建议] 对于共享主机，启用此检查以得到更好的安全性。', '从单选框选择', '');

$_tipsdb['forceType'] = new DAttrHelp("强制MIME类型", '指定后，无论文件后缀如何， 此context下的所有文件都将用作具有指定MIME类型的静态文件。  设置为NONE时，将禁用强制MIME类型。', '', 'MIME type or NONE.', '');

$_tipsdb['gdb_groupname'] = new DAttrHelp("组名", '组名，仅包含字母和数字（无特殊字符）。', '', 'string', '');

$_tipsdb['gdb_users'] = new DAttrHelp("用户", '属于该组的用户,用空格分隔。', '', '', '');

$_tipsdb['generalContext'] = new DAttrHelp("静态Context", 'Context settings are used to specify special settings for files in a certain location. These settings can be used to bring in files outside of the document root (like Apache&#039;s Alias or AliasMatch directives), to protect a particular directory using authorization realms, or to block or restrict access to a particular directory within the document root.', '', '', '');

$_tipsdb['geoipDBFile'] = new DAttrHelp("数据库文件路径", '指定MaxMind GeoIP数据库路径。', '', '绝对路径', '/usr/local/share/GeoIP/GeoLite2-Country.mmdb');

$_tipsdb['geoipDBName'] = new DAttrHelp("数据库名称", 'MaxMind GeoIP数据库名称。从GeoIP2起，此设置是必需的。<br/><br/>从GeoIP升级到GeoIP2时，为此设置“ COUNTRY_DB”，“CITY_DB”或“ ASN_DB”的值会自动使用一些与GeoIP兼容的条目（在下面的“数据库名称”值中列出）填充PHP的$ _SERVER变量，以简化此操作的过渡。<br/><b>CITY_DB:</b> &#039;GEOIP_COUNTRY_CODE&#039;, &#039;GEOIP_COUNTRY_NAME&#039;, &#039;GEOIP_CONTINENT_CODE&#039;, &#039;GEOIP_COUNTRY_CONTINENT&#039;, &#039;GEOIP_DMA_CODE&#039;, &#039;GEOIP_METRO_CODE&#039;, &#039;GEOIP_LATITUDE&#039;, &#039;GEOIP_LONGITUDE&#039;, &#039;GEOIP_POSTAL_CODE&#039;, and &#039;GEOIP_CITY&#039;.<br/><b>COUNTRY_DB:</b> &#039;GEOIP_COUNTRY_CODE&#039;, &#039;GEOIP_COUNTRY_NAME&#039;, &#039;GEOIP_CONTINENT_CODE&#039;, and &#039;GEOIP_COUNTRY_CONTINENT&#039;.<br/><b>ASN_DB:</b> &#039;GEOIP_ORGANIZATION&#039; and &#039;GEOIP_ISP&#039;.', '', '', 'COUNTRY_DB');

$_tipsdb['geolocationDB'] = new DAttrHelp("IP地理定位数据库", '多个MaxMind地理定位数据库可以在这里指定。MaxMind有以下数据库类型: 国家，地区，城市，组织，ISP和NETSPEED。如果混合配置“国家”，“地区”，和“城市”类型数据库，则最后一项将会生效。', '', '', '');

$_tipsdb['gracePeriod'] = new DAttrHelp("宽限期（秒）", '指定来自一个IP的连接数超过&quot;连接软限制&quot;之后， 多长时间之内可以继续接受新连接。在此期间，如果总连接数仍然 低于&quot;连接硬限制&quot;，将继续接受新连接。之后，如果连接数 仍然高于&quot;连接软限制&quot;，相应的IP将被封锁&quot;禁止期（秒）&quot;里设置的时长。', '[性能与安全建议] 设置为足够大的数量，以便下载完整网页， 但也要足够低以防范蓄意攻击。', '整数', '');

$_tipsdb['gracefulRestartTimeout'] = new DAttrHelp("平滑重启超时时长(secs)", '平滑重启时，即使新的服务器实例已经启动，旧的实例仍将继续 处理现有的请求。此项超时设置定义了旧实例等待多长时间后中止。 默认值是300秒。 -1表示永远等待。 0表示不等待，立即中止。', '', 'int', '');

$_tipsdb['groupDBCacheTimeout'] = new DAttrHelp("组数据库缓存超时时长(secs)", '指定多长时间后台组数据库将检查一次变更。 查看更多详细信息查看&quot;用户数据库缓存超时 (secs)&quot;。', '', '整数', '');

$_tipsdb['groupDBMaxCacheSize'] = new DAttrHelp("组数据库最大缓存大小", '指定组数据库的最大缓存大小。', '[性能建议] 由于更大的缓存会消耗更多的内存， 更高的值可能会也可能不会提供更好的性能。 请根据你的用户数据库大小和网站使用情况来设置合适的大小。', '整数', '');

$_tipsdb['gzipAutoUpdateStatic'] = new DAttrHelp("自动更新静态文件", '指定是否由LiteSpeed自动创建/更新可压缩静态文件的GZIP压缩版本。 如果设置为Yes，当请求文件MIME属于&quot;压缩类型&quot;时， LiteSpeed会根据压缩的文件时间戳来创建/更新文件的压缩版本。 压缩的文档会创建在&quot;静态缓存目录&quot;目录下。 文件名称根据原文件的MD5散列创建。<br/><br/>默认值: Yes', '', '从单选框选择', '');

$_tipsdb['gzipCacheDir'] = new DAttrHelp("静态缓存目录", '指定目录路径来存储静态内容的压缩文件。默认是&quot;交换目录&quot;。', '', 'Directory Path', '');

$_tipsdb['gzipCompressLevel'] = new DAttrHelp("GZIP压缩级别（动态内容）", '指定压缩动态态内容的级别。 范围从1 (最低)到9 (最高)。<br/>改设置尽在&quot;启用GZIP压缩&quot;和&quot;启用GZIP动态压缩&quot;启用时生效<br/><br/>默认值: 6', '[性能建议] 更高的压缩级别将消耗更多的内存和CPU资源。 如果您的机器有额外的资源您可以设置更高的级别。 级别9与级别6没有太大的区别，但是级别9会占用多得多的CPU资源。', 'Number between 1 and 9.', '');

$_tipsdb['gzipMaxFileSize'] = new DAttrHelp("静态文件最大尺寸(bytes)", '指定LiteSpeed可以自动创建压缩文件的静态文件最大尺寸。<br/><br/>默认值为 10M', '[性能建议] 不建议使用LiteSpeed创建/更新较大文件的压缩文件。 压缩操作会占用整个服务器进程并且在压缩结束前新请求都无法被处理。', 'Number in bytes not less than 1K.', '');

$_tipsdb['gzipMinFileSize'] = new DAttrHelp("静态文件最小尺寸 (bytes)", '指定LiteSpeed创建相应压缩文件的静态文件最小尺寸。<br/><br/>Default value: 200', 'It is not necessary to compress very small files as the bandwidth saving is negligible.', 'Number in bytes not less than 200.', '');

$_tipsdb['gzipStaticCompressLevel'] = new DAttrHelp("GZIP压缩级别（静态内容）", '指定GZIP压缩静态内容的级别。 范围从1 (最低)到9 (最高)。。<br/><br/>该选项仅在 &quot;启用GZIP压缩&quot;和&quot;自动更新静态文件&quot; 启用后才会生效<br/><br/>默认值是6', '', 'Number between 1 and 9.', '');

$_tipsdb['hardLimit'] = new DAttrHelp("连接硬限制", '指定来自单个IP的并发连接的硬限制。 此限制是永远执行的，客户端将永远无法超过这个限制。 HTTP/1.0客户端通常会尝试建立尽可能多的连接，因为它们需要同时下载嵌入的内容。此限制应设置得足够高，以使HTTP/1.0客户端仍然可以访问相应的网站。 使用&quot;连接软限制&quot;设置期望的连接限制。<br/><br/>建议根据你的网页内容和流量负载，限制在20与50之间。', '[安全] 一个较低的数字将使得服务器可以服务更多独立的客户。<br/>[安全] 受信任的IP或子网不受影响。<br/>[性能] 使用大量并发客户端进行基准测试时，设置一个较高的值。', '整数', '');

$_tipsdb['httpdWorkers'] = new DAttrHelp("工具人(Workers)的数量", '指定httpd工作程序的数量.', '[性能] 设置适当的数字以满足您的需求。 增加更多的数量不一定意味着更好的效果。', 'Integer value between 1 and 16.', '');

$_tipsdb['inBandwidth'] = new DAttrHelp("入口带宽 (bytes/sec)", '指定对单个IP地址允许的最大传入吞吐量（无论与该IP之间建立了多少个连接）。 为提高效率，真正的带宽可能最终会略高于设定值。 带宽是按1KB单位分配。设定值为0可禁用限制。 每个客户端的带宽限制（字节/秒）可以在服务器或虚拟主机级别设置。 虚拟主机级别的设置将覆盖服务器级别的设置。', '[安全] 受信任的IP或子网不受影响。', '整数', '');

$_tipsdb['inMemBufSize'] = new DAttrHelp("最大的读写缓冲区大小", '指定用于存储请求内容和相应的动态响应的最大缓冲区大小。达到此限制时， 服务器将在&quot;交换目录&quot;中创建临时交换文件。', '[性能] 设置足够大的缓冲区，以容纳所有并发 请求/响应，避免内存和磁盘数据交换。如果交换目录（默认为/tmp/lshttpd/swap/）存在频繁的读写活动，说明缓冲区太小，LiteSpeed正在使用交换文件。', '整数', '');

$_tipsdb['indexFiles'] = new DAttrHelp("索引文件", '指定URL映射到目录时顺序查找的索引文件名称。 您可以在服务器、虚拟主机和Context级别对其进行自定义。', '[性能建议] 只设置你需要的索引文件。', 'Comma-delimited list of index filenames.', '');

$_tipsdb['indexUseServer'] = new DAttrHelp("使用服务器索引文件", '指定是否使用服务器的索引文件。 如果设置为Yes，那么只有服务器的设置将被使用。 如果设置为No，那么服务器的设置将不会被使用。 如果设置为Addition，那么附加的索引文件可以被添加到此虚拟主机服务器的索引文件列表中。 如果想要禁用此虚拟主机的索引文件，您可以将该值设置为No，并将索引文件栏留空。 field empty.', '', '从列表中选择', '');

$_tipsdb['initTimeout'] = new DAttrHelp("初次请求超时时间 (secs)", '指定服务器等待外部应用响应新建立的连接的第一个请求的最大时间  如果服务器在这个限制时间内没有收到外部应用的任何数据，它将把这个连接标记为坏(Bad)。这有助于识别与外部应用程序的通信问题。 这有助于尽快发现与外部应用的通信问题。如果某些请求的处理时间较长，则增加这个限制以避免503错误信息。', '', '整数', '');

$_tipsdb['installpathprefix'] = new DAttrHelp("安装路径前缀", '设置“--prefix”配置选项的值。 默认安装位置在LiteSpeed Web服务器的安装目录。', 'LiteSpeed Web Server可以同时使用多个PHP版本。 如果要安装多个版本，则 应该给他们不同的前缀。', '路径', '/usr/local/lsws/lsphp5');

$_tipsdb['instances'] = new DAttrHelp("实例数", '指定服务器创建的外部应用的最大实例数。这个选项需要&quot;开机自启&quot;的值为enable. 大多数FastCGI/LSAPI应用程序每个进程实例只能处理一个请求，对于这种类型的应用，实例数应与&quot;最大连接数&quot;的值相匹配。  而有些FastCGI/LSAPI应用程序可以生成多个子进程以同时处理多个请求. 对于这种类型的应用，应将实例设置为 &quot;1&quot;，并使用环境变量来控制应用可以生成多少个子进程。', '', '整数', '');

$_tipsdb['internalmodule'] = new DAttrHelp("内部的", '指定模块是否为内部模块（静态链接），而不是动态链接库。', '', '布尔值', '');

$_tipsdb['ip2locDBCache'] = new DAttrHelp("数据库缓存类型", '使用的缓存方法。 默认值为Memory。', '', '从列表中选择', '');

$_tipsdb['ip2locDBFile'] = new DAttrHelp("IP2Location数据库文件路径", '有效数据库文件的位置。', '', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['javaServletEngine'] = new DAttrHelp("Servlet Engine", '指定为该Web应用程序提供服务的Servlet Engine的名称。 Servlet引擎必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义。', '', '从列表中选择', '');

$_tipsdb['javaWebAppContext'] = new DAttrHelp("Java Web App Context", 'Many people running Java applications use the servlet engine to serve static content as well. But no servlet engine is nearly as efficient as LiteSpeed Web Server for these processes. In order to improve the overall performance, LiteSpeed Web Server can be configured as a gateway server, which serves static content and forwards dynamic Java page requests to the servlet engine.<br/><br/>LiteSpeed Web Server requires certain contexts to be defined in order to run a Java application. A Java Web App Context automatically creates all required contexts based on the Java web application&#039;s configuration file (WEB-INF/web.xml).<br/><br/>There are a few points you need to keep in mind when setting up a Java Web App Context:<br/><ul> <li>A Servlet Engine external application must be set up in &quot;外部应用&quot; before Java Web App Context can be set up.</li> <li>A &quot;脚本处理程序&quot; for .jsp files should be defined as well.</li> <li>If the web application is packed into a .war file, the .war file must be expanded. The server cannot access compressed archive files.</li> <li>For the same resources, the same URL should be used no matter whether it is accessed through LiteSpeed Web Server or through the servlet engine&#039;s built-in HTTP server.<br/>For example,   Tomcat 4.1 is installed under /opt/tomcat. Files for the &quot;examples&quot; web application are   located at /opt/tomcat/webapps/examples/. Through Tomcat&#039;s built-in HTTP server,   the &quot;examples&quot; web application is thus accessed with a URI like &quot;/examples/***&quot;.   The corresponding Java Web App Context should thus be configured:   &quot;URI&quot; = /examples/, &quot;伪造&quot; = /opt/tomcat/webapps/examples/.</li>   </ul>', '', '', '');

$_tipsdb['javaWebApp_location'] = new DAttrHelp("伪造", '指定包含此Web应用程序的目录。 应包含以下文件 &quot;WEB-INF/web.xml&quot;.<br/><br/>默认值：$DOC_ROOT + &quot;URI&quot;', '', 'path', '');

$_tipsdb['keepAliveTimeout'] = new DAttrHelp("持续连接超时时长", '指定持续连接请求的最长闲置时间。 如果在这段时间内没有接收到新的请求，该连接将被关闭。 这个设置只适用于HTTP/1.1连接。HTTP/2连接有较长的闲置时间，不受此设置影响。', '[安全和性能建议] 我们建议您将值设置得刚好足够处理单个页面 视图的所有请求。没有必要延长持续连接时间。较小的值可以减少闲置 连接，提高能力，以服务更多的用户，并防范拒绝服务攻击。2-5秒 对大多数应用是合理范围。Litespeed在非持续连接环境是非常高效的。', '整数', '');

$_tipsdb['keyFile'] = new DAttrHelp("私钥文件", 'SSL私钥文件的文件名。 密钥文件不应被加密。', '[安全建议] 私钥文件应放在一个安全的目录中，该目录应 允许对运行服务器的用户具有只读的访问权限。', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['lbContext'] = new DAttrHelp("负载均衡 Context", 'Like other external applications, load balancer worker applications cannot be used directly. They must be mapped to a URL through a context. A Load Balancer Context will associate a URI to be load balanced by the load balancer workers.', '', '', '');

$_tipsdb['lbapp'] = new DAttrHelp("负载均衡", '指定要与此context关联的负载均衡器。 此负载均衡是一个虚拟应用程序，必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义。', '', '从列表中选择', '');

$_tipsdb['listenerBinding'] = new DAttrHelp("绑定", '指定侦听器分配给哪个lshttpd子进程。 通过手动将侦听器与进程相关联，可以使用不同的子进程来处理对不同侦听器的请求。 默认情况下，将侦听器分配给所有子进程。', '', '从复选框中选择', '');

$_tipsdb['listenerIP'] = new DAttrHelp("IP Address", '指定此监听器的IP地址.所有可用的IP地址都列出了. IPv6地址应包含&quot;[ ]&quot;.<br/><br/>如果要监听所有IPV4地址, 选择 ANY. 要监听所有IPV4和IPV6地址,选择 [ANY].<br/><br/>为了同时为IPv4和IPv6客户端提供服务，应使用IPv4映射的IPv6地址代替纯IPv4地址。  IPv4映射的IPv6地址应写为[::FFFF:x.x.x.x].', '[安全建议] 如果您的计算机在不同的子网中具有多个IP, 您可以选择特定的IP以仅允许来自相应子网的流量。', '从列表中选择', '');

$_tipsdb['listenerModules'] = new DAttrHelp("Listener Modules", 'Listener module configuration data is, by default inherited from the Server module configuration.   The Listener Modules are limited to the TCP/IP Layer 4 hooks.', '', '', '');

$_tipsdb['listenerName'] = new DAttrHelp("侦听器名称", '此侦听器的唯一名称。', '', '', '');

$_tipsdb['listenerPort'] = new DAttrHelp("端口", '指定侦听器的TCP端口。 只有超级用户（root）可以使用低于1024的端口。 端口80是默认的HTTP端口。 端口443是默认的HTTPS端口。', '', '整数', '');

$_tipsdb['listenerSecure'] = new DAttrHelp("安全", '指定这是否是安全（SSL）侦听器。 对于安全的侦听器，需要正确设置其他SSL设置。', '', '从单选框选择', '');

$_tipsdb['lmap'] = new DAttrHelp("Virtual Hosts Mappings", 'Shows currently established mappings to virtual hosts from a particular listener.  The virtual host name appears in brackets and is followed by the matching domain name(s) for this listener.', 'If a virtual host has not been loaded successfully (fatal errors in the  virtual host configuration), the mapping to that virtual host will not be displayed.', '', '');

$_tipsdb['lname'] = new DAttrHelp("Name - Listener", 'The unique name that identifies this listener. This is the  &quot;侦听器名称&quot; you specified when setting up the listener.', '', '', '');

$_tipsdb['location'] = new DAttrHelp("位置", '指定此context在文件系统中的对应位置。<br/><br/>Default value: $DOC_ROOT + &quot;URI&quot;', '', 'It can be an absolute path or path relative to $SERVER_ROOT, $VH_ROOT, or $DOC_ROOT. $DOC_ROOT is the default relative path, and can be omitted.<br/><br/>If the &quot;URI&quot; is a regular expression, then the matched sub-string can be used to form the &quot;Root&quot; string. The matched sub-string can be referenced with the values &quot;$1&quot; - &quot;$9&quot;. &quot;$0&quot; and &quot;&&quot; can be used to reference the whole matched string. Additionally, a query string can be set by appending a &quot;?&quot; followed by the query string. Be careful. &quot;&&quot; should be escaped as &quot;\&&quot; in the query string.', 'A plain URI like /examples/ with &quot;位置&quot; set to /home/john/web_examples will map the request &quot;/examples/foo/bar.html&quot; to file &quot;/home/john/web_examples/foo/bar.html&quot;.<br/>To simulate Apache&#039;s mod_userdir, set URI to exp: ^/~([A-Za-z0-9]+)(.*), set &quot;位置&quot; to /home/$1/public_html$2. With these settings, a request of URI /~john/foo/bar.html will map to file /home/john/public_html/foo/bar.html.');

$_tipsdb['logUseServer'] = new DAttrHelp("使用服务器日志", '指定是否将虚拟主机的日志信息放置到服务器日志文件中，而不是创建独自的日志文件。', '', '从单选框选择', '');

$_tipsdb['log_compressArchive'] = new DAttrHelp("压缩存档", '指定是否压缩回滚日志以节省磁盘空间。', '日志文件是高度可压缩的，建议采取压缩以减少旧日志的磁盘占用量。', '从单选框选择', '');

$_tipsdb['log_debugLevel'] = new DAttrHelp("调试级别", '指定调试日志级别。 要使用此功能，&quot;日志级别&quot;必须设置为DEBUG。 在“调试级别”设置为NONE时，即使&quot;日志级别&quot; 设置为DEBUG，调试日志也是被禁用的。 在正在运行的服务器上，&quot;Toggle Debug Logging&quot;可以被用于 控制调试级别而无需重启。', '[性能建议] 重要！如果你不需要详细的调试日志记录， 应始终将其设置为NONE。启用调试日志记录将严重降低服务性能 ，且可能在很短时间耗尽磁盘空间。 调试日志记录包括每个请求和响应的详细信息。<br/><br/>我们推荐将日志级别设置为DEBUG，调试级别设置为NONE。 这些设置意味着你的磁盘不会被调试日志塞满， 但是你可以使用&quot;Toggle Debug Logging&quot; 控制调试输出。这个动作可以实时启用或关闭调试记录， 对于调试繁忙的生产服务器非常有用。', '从列表中选择', '');

$_tipsdb['log_enableStderrLog'] = new DAttrHelp("启用标准错误日志（stderr）", '指定在接受到服务器启动的进程输出的标准错误时，是否写入到日志。 如果启用，标准错误信息将记录到服务器日志所在目录内的固定名为“stderr.log”的文件。如果禁用，所有的标准错误输出都将被丢弃。 弃。<br/>诸如PHP的error_log()之类的函数不会直接写入标准错误日志(文件句柄2)，所以不会受到此设置的影响，它们会写入PHP ini指令&#039;error_log&#039;中设置的文件或带有标签“ error.log”的文件中 如果未设置该指令，则为“[STDERR]”。', '如果您需要调试配置的外部应用程序，如PHP、Ruby、Java、Python、Perl，请开启该功能。', '从单选框选择', '');

$_tipsdb['log_fileName'] = new DAttrHelp("文件名", '指定日志文件的路径。', '[性能建议] 将日志文件放置在一个单独的磁盘上。', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['log_keepDays'] = new DAttrHelp("保留天数", '指定访问日志文件将被保存在磁盘上多少天。 只有超出指定天数的回滚日志文件会被删除。 当前的日志文件不会被删除，无论它包含了多少天的数据。 如果你不想自动删除过时的、很旧的日志文件， 将该值设置为0。', '', '整数', '');

$_tipsdb['log_logLevel'] = new DAttrHelp("日志级别", '指定日志文件中记录的日志级别。 可用级别（由高到低）为: ERROR、 WARNING、NOTICE、INFO和DEBUG。 只有级别与当前设置相同或更高的消息将被记录（级别越低记录越详细）。 be logged.', '[性能建议] 使用DEBUG日志级别对 性能没有任何影响，除非&quot;调试级别&quot;没有被设置为NONE.。我们推荐将日志级别设置为DEBUG，将 调试级别值设置为NONE。这样设置意味着你的磁盘不会被调试日志塞满，但是你可以使用&quot;Toggle Debug Logging&quot; 控制调试输出。这个操作可以实时启用或关闭调试记录， 对于调试繁忙的生产服务器非常有用。', '从列表中选择', '');

$_tipsdb['log_rollingSize'] = new DAttrHelp("回滚大小 (bytes)", '指定何时日志文件需要回滚，也称为日志循环。 当文件大小超过回滚限制后，在使用的日志文件将在同一目录中被重命名 为log_name.mm_dd_yyyy(.sequence)，一个新的日志文件将被创建。 回滚的日志文件的实际大小有时会比限制值稍微大一些。 将值设置为0将禁用日志循环。', '请用“K”，“M”，“G”代表千字节，兆字节和千兆字节。', '整数', '');

$_tipsdb['lsapiContext'] = new DAttrHelp("LiteSpeed SAPI Context", 'External applications cannot be used directly. They must be either configured as a script handler or mapped to a URL through a context. An LiteSpeed SAPI Context will associate a URI with an LSAPI (LiteSpeed Server Application Programming Interface) application. Currently PHP, Ruby and Python have LSAPI modules. LSAPI, as it is developed specifically for LiteSpeed web server, is the most efficient way to communicate with LiteSpeed web server.', '', '', '');

$_tipsdb['lsapiapp'] = new DAttrHelp("LiteSpeed SAPI App", '指定要连接到此context的LiteSpeed SAPI应用程序。 此应用程序必须在服务器或虚拟主机级别的 &quot;外部应用&quot;中定义。', '', '从列表中选择', '');

$_tipsdb['lsrecaptcha'] = new DAttrHelp("reCAPTCHA保护", 'reCAPTCHA 保护是一种减轻服务器负载的服务。在下列情况发生后，reCAPTCHA保护将激活 激活后,所以不受信任的客户端(可自定)发出的请求将被重定向到reCAPTCHA验证页面 验证完成后客户端将被重定向到其所需的页面<br/><br/>下列情况将启用reCAPTCHA保护: 1. 服务器或虚拟主机并发请求计数超过连接限制。<br/>2. 启用了Anti-DDoS，并且客户端以可疑的方式访问了URL。 客户端将首先重定向到reCAPTCHA，而不是在触发时被拒绝。<br/>3. 提供了新的重写规则，以通过重写规则激活reCAPTCHA。可以设置“verifycaptcha”将客户端重定向到reCAPTCHA。可以设置一个特殊值&#039;:deny&#039;以在客户端失败太多次时拒绝它。例如，[E=verifycaptcha]将始终重定向到reCAPTCHA，直到通过验证。 [E=verifycaptcha: deny]将重定向到reCAPTCHA，如果客户端达到最大尝试次数,将被拒绝。', '', '', '');

$_tipsdb['lstatus'] = new DAttrHelp("Status - Listener", 'The current status of this listener. The status is either Running or Error.', 'If the listener is in the Error state, you can view the server log to find out why.', '', '');

$_tipsdb['mappedListeners'] = new DAttrHelp("映射的侦听器", 'Specifies the names of all listeners that this template maps to. A listener-to-virtual host mapping for this template&#039;s member virtual hosts will be added to the listeners specified in this field. This mapping will map listeners to virtual hosts based on the domain names and aliases set in the member virtual hosts&#039; individual configurations.', '', 'comma-separated list', '');

$_tipsdb['maxCGIInstances'] = new DAttrHelp("最大CGI实例数量", 'S指定服务器可以启动的CGI进程最大并发数量。 对于每个对CGI脚本的请求，服务器需要启动一个独立的CGI进程。 在Unix系统中，并发进程的数量是有限的。过多的并发进程会降 低整个系统的性能，也是一种进行拒绝服务攻击的方法。 LiteSpeed服务器将对CGI脚本的请求放入管道队列，限制并发 CGI进程数量，以确保最优性能和可靠性。 硬限制为2000。', '[安全和性能建议] 更高的数量并不一定转化为更快的性能。 在大多数情况下，更低的数量提供更好的性能和安全性。更高的数量 只在CGI处理过程中读写延迟过高时有帮助。', '整数', '');

$_tipsdb['maxCachedFileSize'] = new DAttrHelp("最大小文件缓存(bytes)", '指定预分配内存缓冲区中缓存的静态文件最大尺寸。静态文件 可以用四种不同的方式服务：内存缓存、内存映射缓存、直接读写和 sendfile()。 尺寸小于&quot;最大小文件缓存(bytes)&quot;的文件将使用内存缓存服务。尺寸大于该限制、但小于 &quot;最大MMAP文件大小(bytes)&quot;的文件，将使用内存映射缓存服务。 尺寸大于&quot;最大MMAP文件大小(bytes)&quot;的文件将通过直接读写或sendfile() 服务。使用内存缓存服务小于4K的文件是最佳做法。', '', '整数', '');

$_tipsdb['maxConnections'] = new DAttrHelp("最大连接数", '指定服务器可以接受的最大并发连接数。这包括纯TCP连接和SSL连接。 一旦达到此限制，服务器将在完成活动请求时关闭“Keep-Alive”的连接。', '当服务器由&quot;root&quot;用户启动时，服务器会尝试自动调整每个进程的文件描述符限制，但是，如果自动调整失败，你可能需要手动增加此限制。', '整数', '');

$_tipsdb['maxConns'] = new DAttrHelp("最大连接数", '指定服务器与外部应用之间可建立的最大并发连接数。该设置控制外部应用程序可同时处理多少个请求。 然而，真正的限制还取决于外部应用本身。如果外部的速度不够快，或者无法处理大量的并发请求，那么将这个值设置得更高也无济于事。', '[性能建议] 设置一个高值并不能直接变成高性能。设置为一个不会使外部应用程序过载的值，将提供最佳的性能/吞吐量。', '整数', '');

$_tipsdb['maxDynRespHeaderSize'] = new DAttrHelp("动态回应报头最大大小(bytes)", '指定动态回应的最大报头大小。硬限制为131072字节或128K.<br/><br/>Default value: 32768 or 32K', '[可靠性和性能建议] 设置一个合理的低值以帮助识别外部应用程序产生的 坏的动态回应。', '整数', '');

$_tipsdb['maxDynRespSize'] = new DAttrHelp("动态回应主内容最大大小(bytes)", '指定动态回应的最大主内容尺寸。硬限制是2047MB。', '[可靠性和性能建议] 设置一个合理的低值以帮助识别坏的响应。恶意脚本经常包含 无限循环而导致大尺寸回应。', '整数', '');

$_tipsdb['maxKeepAliveReq'] = new DAttrHelp("最大持续连接请求数", '指定通过持续连接（持久）会话处理的请求的最大数量。一旦达 到此限制，连接将被关闭。您也可以为每个虚拟主机单独设置限制。', '[性能建议] 设置为合理的较高的值。值为“1”或“0”时将禁用持续连接。', '整数', '');

$_tipsdb['maxMMapFileSize'] = new DAttrHelp("最大MMAP文件大小(bytes)", '指定使用内存映射（MMAP）的最大静态文件大小。 静态文件可以用四种不同的方式服务：内存缓存、内存映射缓存、直接读写和 sendfile()。 尺寸小于&quot;最大小文件缓存(bytes)&quot;的文件将使用内存缓存服务。尺寸大于该限制、但小于 &quot;最大MMAP文件大小(bytes)&quot;的文件，将使用内存映射缓存服务。 尺寸大于&quot;最大MMAP文件大小(bytes)&quot;的文件将通过直接读写或sendfile() 服务。 由于服务器有一个32位的地址空间（2GB），不建议使用内存映射非常大的文件。', '', '整数', '');

$_tipsdb['maxMindDBEnv'] = new DAttrHelp("环境变量", '将数据库查找的结果分配给环境变量。', '', 'Variable_Name mapped_DB_data<br/><br/>One entry per line. Path to data can use map keys or 0-based array indexes, both being separated by /.', 'COUNTRY_CODE COUNTRY_DB/country/iso_code<br/>REGION_CODE  CITY_DB/subdivisions/0/iso_code');

$_tipsdb['maxReqBodySize'] = new DAttrHelp("最大请求主内容大小(bytes)", '指定HTTP请求主内容最大尺寸。对于32位操作系统， 硬限制为2GB。对于64位操作系统，几乎是无限的。', '[安全建议] 为了防止拒绝服务攻击，尽量将限制值设定到实际需求的大小。 交换空间的剩余空间必须比这个限制值大。', '整数', '');

$_tipsdb['maxReqHeaderSize'] = new DAttrHelp("最大请求头大小(bytes)", '指定请求URL中包含的HTTP请求头最大值。  硬限制为131072 字节或128K.<br/><br/>Default value: 32768 or 32K', '[安全和性能建议] 设置合理的低值来减少内存的使用并帮助识别虚假请求和拒绝服务攻击。<br/>对于大多数网站来说4000-8000已经足够大。', '整数', '');

$_tipsdb['maxReqURLLen'] = new DAttrHelp("最大请求URL长度(bytes)", '指定请求URL的最大大小。 URL是一个纯文本的地址，包含查询字符串来请求服务器上的资源。 硬限制为65530个字节。 大于此值的值，例如64K（大6个字节），将被视为65530。<br/><br/>Default value: 8192 or 8K.', '[安全和性能建议] 将其设置合理的低值来以减少内存使用 并帮助识别虚假请求和拒绝服务攻击。<br/>对大多数网站2000-3000已经足够大，除非使用HTTP GET而不是POST来提交大型的查询字符串。', '整数', '');

$_tipsdb['maxSSLConnections'] = new DAttrHelp("最大SSL连接数", '指定服务器将接受的最大并发SSL连接数。 由于并发SSL和非SSL的连接总数不能超过&quot;最大连接数&quot;的限制，因此允许的最大SSL连接的数量须低于此限制。', '', '整数', '');

$_tipsdb['memHardLimit'] = new DAttrHelp("内存硬限制 (bytes)", '与&quot;内存软限制 (bytes)&quot;非常相同，但是在一个用户进程中，软限制 可以被放宽到硬限制的数值。硬限制可以在服务器级别或独立的外部应用程序级别设 置。如果未在独立的外部应用程序级别设定限制，将使用服务器级别的限制。<br/><br/>如果在两个级别都没有设置该限制，或者限制值设为0，将使用操 作系统的默认设置。', '[注意] 不要过度调整这个限制。如果您的应用程序需要更多的内存， 这可能会导致503错误。', '整数', '');

$_tipsdb['memSoftLimit'] = new DAttrHelp("内存软限制 (bytes)", '以字节为单位指定服务器启动的外部应用进程或程序的内存占用限制。<br/><br/>此限制的目的主要是为了防范软件缺陷或蓄意攻击造成的过度内存使用， 而不是限制正常使用。确保留有足够的内存，否则您的应用程序可能故障并 返回503错误。限制可以在服务器级别或独立的外部应用程序级别设置。如 果未在独立的外部应用程序级别设定限制，将使用服务器级别的限制。<br/><br/>如果在两个级别都没有设置该限制，或者限制值设为0，将使用操 作系统的默认设置。', '[注意] 不要过度调整这个限制。如果您的应用程序需要更多的内存， 这可能会导致503错误。', '整数', '');

$_tipsdb['memberVHRoot'] = new DAttrHelp("成员虚拟主机根目录", '指定此虚拟主机的根目录。 如果留空，将使用此模板的默认虚拟主机根目录.<br/><br/>Note: 这<b>不是</b>文档根。 建议将与虚拟主机相关的所有文件（如虚拟主机配置，日志文件，html文件，CGI脚本等）放置在此目录下。 虚拟主机根目录可以以变量$VH_ROOT来引用。', '', 'path', '');

$_tipsdb['mime'] = new DAttrHelp("MIME设置", '为此服务器指定包含MIME设置的文件。 在chroot模式中提供了绝对路径时，该文件路径总是相对于真正的根。 点击文件名可查看/编辑详细的MIME项。', 'Click the filename to edit the MIME settings.', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['mimesuffix'] = new DAttrHelp("后缀", '你可以列出相同MIME类型的多个后缀，用逗号分隔。', '', '', '');

$_tipsdb['mimetype'] = new DAttrHelp("MIME类型", 'MIME类型由类型和子类型组成，格式为“类型/子类型”.', '', '', '');

$_tipsdb['minGID'] = new DAttrHelp("最小的GID", '指定外部应用程序的最小组ID。 如果组ID比这里指定的值更小，其外部脚本的执行将被拒绝。 如果的LiteSpeed Web服务器是由“Root”用户启动，它可以在“suEXEC” 模式运行外部应用程序，类似Apache（可以切换到与Web服务器不同的用户/组ID）。', '[安全] 设置足够高的值以排除所有系统用户所属的组。', '整数', '');

$_tipsdb['minUID'] = new DAttrHelp("最小的UID", '指定外部应用程序的最小用户ID。 如果用户ID比这里指定的值更低。其外部脚本的执行将被拒绝。 如果的LiteSpeed Web服务器由“Root”用户启动，它可以在“suEXEC” 模式运行外部应用程序，类似Apache（可以切换到与Web服务器不同的用户/组ID）。', ' Set it high enough to exclude all system/privileged users.', '整数', '');

$_tipsdb['modParams'] = new DAttrHelp("模块参数", 'Set module parameters. The module parameters are defined by the module developer.<br/><br/>Set the value in the Server configuration to globally assign the default value.  The user can override this setting at the Listener, Virtual Host or Context levels. If the &#039;Not Set&#039; radio button is selected, it will be inherited from the upper level.', '', 'Specified by the module interface.', '');

$_tipsdb['moduleContext'] = new DAttrHelp("Module Handler Context", 'A module handler context will associate a URI with a registered module. Modules need to be registered at Server Module Configuration tab.', '', '', '');

$_tipsdb['moduleEnabled'] = new DAttrHelp("Enable Module", 'Enables module hooks globally. This setting can be overridden at the Listener and Virtual Host levels.<br/><br/>Default value: Yes', '', '从单选框选择', '');

$_tipsdb['moduleEnabled_lst'] = new DAttrHelp("Enable Module", 'Enables module hooks at the Listener level. This setting will only take effect if the module has TCP/IP level hooks (L4_BEGSESSION, L4_ENDSESSION, L4_RECVING, L4_SENDING).<br/><br/>Default value: Inherit Server level setting', '', '从单选框选择', '');

$_tipsdb['moduleEnabled_vh'] = new DAttrHelp("Enable Module", 'Enables module hooks at the Virtual Host level. This setting will only take effect if the module has HTTP level hooks.<br/><br/>Default value: Inherit Server level setting', '', '从单选框选择', '');

$_tipsdb['moduleNameSel'] = new DAttrHelp("模块", '模块名称。 该模块必须在“服务器模块配置”选项卡下配置。 配置后，模块名称将在侦听器和虚拟主机配置的下拉框中显示。', '', '从列表中选择', '');

$_tipsdb['modulename'] = new DAttrHelp("模块", '服务器启动时要加载的外部或内部模块的名称。<br/><br/><b>外部模块</b><br/>用于外部模块的值必须与 $SERVER_ROOT/modules/modulename.so 下的“.so”文件名匹配， 以便服务器应用程序加载该文件。 在注册新模块后需要重新启动服务器。<br/><br/><b>内部模块</b><br/>内部模块使用的值必须与构建模块时使用的名称匹配 例如，对于服务器附带的内部缓存模块，必须将其设置为“缓存”。', '', '字符串', '');

$_tipsdb['nodeBin'] = new DAttrHelp("Node路径", 'Node.js执行文件的路径.', '', '绝对路径', '');

$_tipsdb['nodeDefaults'] = new DAttrHelp("Node.js App Default Settings", 'Default configurations for Node.js applications. These settings can be overriden at the context level.', '', '', '');

$_tipsdb['note'] = new DAttrHelp("备注", '为你自己添加备注', '', '', '');

$_tipsdb['ocspCACerts'] = new DAttrHelp("OCSP CA证书", '指定存储OCSP证书颁发机构（CA）证书的文件的位置。 这些证书用于检查OCSP响应服务器的响应（并确保这些响应不被欺骗或以其他方式被破坏）。 该文件应包含整个证书链。 如果该文件不包含根证书，则LSWS无需将根证书添加到文件中就应该能够在系统目录中找到该根证书， 但是，如果此验证失败，则应尝试将根证书添加到此文件中。<br/><br/>This setting is optional. If this setting is not set, the server will automatically check &quot;CA证书文件&quot;.', '', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['ocspRespMaxAge'] = new DAttrHelp("OCSP响应最大有效时间（秒）", '此选项设置OCSP响应的最大有效时间。 如果OCSP响应早于该最大使用期限，则服务器将与OCSP响应服务器联系以获取新的响应。 默认值为86400 。 通过将此值设置为-1，可以关闭最大有效时间。', '', 'Integer of seconds', '');

$_tipsdb['ocspResponder'] = new DAttrHelp("OCSP响应服务器", '指定要使用的OCSP响应服务器的URL。 如果未设置，则服务器将尝试联系OCSP响应服务器 在证书颁发机构的颁发者证书中有详细说明。 某些颁发者证书可能未指定OCSP服务器URL。', '', 'URL starting with http://', 'http://rapidssl-ocsp.geotrust.com ');

$_tipsdb['outBandwidth'] = new DAttrHelp("出口带宽 (bytes/sec)", '指定对单个IP地址允许的最大传出吞吐量（无论与该IP之间建立了多少个连接）。 为提高效率，真正的带宽可能最终会略高于设定值。 带宽按4KB为单位分配。设定值为0可禁用限制。 每个客户端的带宽限制（字节/秒）可以在服务器或虚拟主机级别设置。 虚拟主机级别的设置将覆盖服务器级别的设置。', '[性能建议] 按8KB单位设置带宽可获得更好的性能。<br/>[安全建议] 受信任的IP或子网不受影响。', '整数', '');

$_tipsdb['pcKeepAliveTimeout'] = new DAttrHelp("持久连接超时时间", '指定保持持久连接的最大时间，以秒为单位。<br/><br/>如果设置成&quot;-1&quot;,连接将不会超时. 当设置成0或者更大, 连接将在这些秒数后被关闭。', '', 'int', '');

$_tipsdb['perClientConnLimit'] = new DAttrHelp("客户端流量限制", '这些是基于客户端IP的连接控制设置。 这些设置有助于缓解DoS（拒绝服务）和DDoS（分布式拒绝服务）攻击。', '', '', '');

$_tipsdb['persistConn'] = new DAttrHelp("持久连接", '指定在处理完请求后是否要保持连接 持久连接可以提高性能， 但某些FastCGI外部应用程序不完全支持持久连接。默认为 &quot;On&quot;。', '', '从单选框选择', '');

$_tipsdb['phpIniOverride'] = new DAttrHelp("php.ini 重写", '用于覆盖当前context（虚拟主机级别或context级别）中的php.ini设置。<br/><br/>支持的指令是：<br/>php_value<br/>php_flag<br/>php_admin_value<br/>php_admin_flag<br/><br/>所有其他行/指令将被忽略。', '', 'Override syntax is similar to Apache, a newline separated list of directives and their values with each directive being prepended by php_value, php_flag, php_admin_value, or php_admin_flag appropriately.', 'php_value include_path &quot;.:/usr/local/lib/php&quot;<br/>php_admin_flag engine on<br/>php_admin_value open_basedir &quot;/home&quot;');

$_tipsdb['pid'] = new DAttrHelp("PID", 'PID (Process ID) of the current server process.', 'The PID will change each time the server is restarted.', '', '');

$_tipsdb['procHardLimit'] = new DAttrHelp("进程硬限制", '与&quot;进程软限制&quot;非常相同，但是，在用户进程中软限制 可以被放宽到硬限制的数值。硬限制可以在服务器级别或独立的外部应用程序级别设 置。如果未在独立的外部应用程序级别设定限制，将使用服务器级别的限制。 如果在两个级别都没有设置该限制，或者限制值设为0，将使用操 作系统的默认设置。', '', '整数', '');

$_tipsdb['procSoftLimit'] = new DAttrHelp("进程软限制", '限制用户可以创建的进程总数.所有现有的进程都将被计算在这个限制之内,而不仅仅是要启动的新进程。<br/><br/>该限制可以在服务器级别或单个外部应用级别进行设置.如果未在应用级别设置，则将使用服务器级别的限制. 如果该值为0或服务器级和应用级都没有设置,将使用操作系统的默认设置', 'PHP scripts can call for forking processes. The main purpose of this limit is as a last line of defense to prevent fork bombs and other attacks caused by PHP processes creating other processes.<br/><br/>Setting this setting too low can severely hurt functionality. The setting will thus be ignored below certain levels.<br/><br/>When <b>Run On Start Up</b> is set to &quot;Yes (Daemon mode)&quot;, the actual process limit will be higher than this setting to make sure parent processes are not limited.', '整数', '');

$_tipsdb['proxyContext'] = new DAttrHelp("Proxy Context", 'A Proxy Context enables this virtual host as a transparent reverse proxy server. This proxy server can run in front of any web servers or application servers that support HTTP protocol. The External web server that this virtual host proxies for  has to be defined in &quot;外部应用&quot; before you can set up a Proxy Context.', '', '', '');

$_tipsdb['proxyWebServer'] = new DAttrHelp("Web服务器", '指定外部Web服务器的名称。 此外部Web服务器必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义', '', '从列表中选择', '');

$_tipsdb['quicBasePLPMTU'] = new DAttrHelp("PLPMTU 默认值", 'QUIC默认使用的PLPMTU (无报头的最大数据包大小,以字节为单位)的默认值. 设置为0将会允许QUIC设置大小.<br/><br/>这个设置必须低于 &quot;PLPMTU的最大值&quot; 的值.<br/><br/>Default value: 0', '', '0或1200至65527之间的整数', '');

$_tipsdb['quicCfcw'] = new DAttrHelp("流连接窗口控制", '为QUIC连接分配的缓冲区的初始大小。 默认值为 1.5M 。', '更大的窗口大小将使用更多的内存。', 'Number between 64K and 512M', '');

$_tipsdb['quicCongestionCtrl'] = new DAttrHelp("拥塞控制", '使用的拥塞控制算法。 可以手动设置，也可以选择“默认”选项，将其保留到的QUIC库中。<br/><br/>默认值:: Default', '', '从列表中选择', '');

$_tipsdb['quicEnable'] = new DAttrHelp("启用HTTP3/QUIC", '在整个服务器范围内启用HTTP3/QUIC网络协议。 默认值为是。', '当此设置设置为是时，仍然可以通过&quot;打开HTTP3/QUIC (UDP) 端口&quot;设置在侦听器级别 或通过&quot;Enable HTTP3/QUIC&quot;设置在虚拟主机级别禁用HTTP3/QUIC', '从单选框选择', '');

$_tipsdb['quicEnableDPLPMTUD'] = new DAttrHelp("启用 DPLPMTUD", '启用 Datagram Packetization Layer Path Maximum Transmission Unit Discovery (DPLPMTUD).<br/><br/><b><a href="https://tools.ietf.org/html/rfc8899" target="_blank" rel="noopener noreferrer">Background on DPLPMTUD (RFC 8899)</a></b><br/><br/>Default value: Yes', '', '从单选框选择', '');

$_tipsdb['quicHandshakeTimeout'] = new DAttrHelp("握手超时时间", '给出新的QUIC连接完成其握手的时间（以秒为单位），超过限制时间后连接将中止。 默认值为10。', '', 'Integer number between 1 and 15', '');

$_tipsdb['quicIdleTimeout'] = new DAttrHelp("空闲超时时间（秒）", '空闲的QUIC连接将被关闭的时间（以秒为单位）。 默认值为 30 。', '', 'Integer number between 10 and 30', '');

$_tipsdb['quicMaxCfcw'] = new DAttrHelp("最大连接流量窗口值设置", '指定由于连接流控制窗口缓冲区的最大大小 auto-tuning.<br/><br/>默认值为 0 ，这意味着将使用&quot;流连接窗口控制&quot;的值，并且不会自动调整。', '更大的窗口大小将使用更多的内存。', '0 or a number between 64K and 512M', '');

$_tipsdb['quicMaxPLPMTU'] = new DAttrHelp("PLPMTU的最大值", 'PLPMTU(无报头的最大数据包,以字节为单位)的上限. 此设置用于限制 在DPLPMTUD search space中&quot;最大数据包大小&quot;.  设置为0将会允许QUIC设置大小. (默认情况下LSQUIC暂定MTU为1,500 字节 (以太网)).<br/><br/>这个设置应该比&quot;PLPMTU 默认值&quot;的值高.<br/>Default value: 0', '', '0或1200至65527之间的整数', '');

$_tipsdb['quicMaxSfcw'] = new DAttrHelp("最大流量窗口值设置", '指定由于自动调整而允许流控制窗口达到的最大大小。<br/><br/>默认值为0，这意味着将使用&quot;连接流量窗口值&quot;的值，并且不会自动调整', '更大的窗口大小将使用更多的内存。', '0 or a number between 64K and 128M', '');

$_tipsdb['quicMaxStreams'] = new DAttrHelp("每个连接的最大并发数", '每个QUIC连接的最大并发数。 默认值为100。', '', 'Integer number between 10 and 1000', '');

$_tipsdb['quicSfcw'] = new DAttrHelp("连接流量窗口值", 'QUIC愿意为每个流接收的初始数据量。 默认值为1M。', '更大的窗口大小将使用更多的内存。', 'Number between 64K and 128M', '');

$_tipsdb['quicShmDir'] = new DAttrHelp("QUIC SHM 目录", '指定用于将QUIC数据保存到共享内存的目录。<br/><br/>默认情况下，将使用服务器的默认SHM目录/dev/shm.', '建议使用基于RAM的分区(内存盘),例如 /dev/shm.', 'Path', '');

$_tipsdb['quicVersions'] = new DAttrHelp("HTTP3/QUIC版本", '启用的HTTP3/QUIC版本的列表。 此设置仅应用于将HTTP3/QUIC支持限制为列出的版本，最好留空。', 'It is recommended to leave this setting blank to have the the best configuration applied automatically.', 'Comma-separated list', 'h3-29, h3-Q050');

$_tipsdb['railsDefaults'] = new DAttrHelp("Rack/Rails默认设置", 'Rack/Rails的默认配置. 这些设置可以被context级别覆盖.', '', '', '');

$_tipsdb['rcvBufSize'] = new DAttrHelp("接收缓冲区大小 (bytes)", '每个TCP套接字的接收缓冲区大小。 512K是允许的最大缓冲区大小。', '[性能建议] 建议将此值保留为“未设置”或设置为0以使用操作系统的默认缓冲区大小。<br/>[性能建议] 处理大载荷入站请求，如文件上传时，大的接收缓冲区会提高性能。<br/>[性能建议] 将此值设置为较低的值将减少吞吐量和每个套接字的内存使用率，从而在内存成为瓶颈时允许服务器拥有更多并发套接字。', '整数', '');

$_tipsdb['realm'] = new DAttrHelp("域", '指定这个context下的realm授权。 当指定时，必须提供有效的用户和用户名来访问这个context。 &quot;Realms授权&quot;需要在&quot;虚拟主机安全&quot;部分进行设置。 此设置使用每个realm的&quot;域名称&quot;。', '', '从列表中选择', '');

$_tipsdb['realmName'] = new DAttrHelp("域名称", '指定授权域的唯一名称。', '', '', '');

$_tipsdb['realms'] = new DAttrHelp("Realms授权", '列出这个虚拟主机的所有Realm。 Realm授权可以阻止未授权用户访问受保护的网页。 Realm是一个用户名录，其中包含了用户名、密码、分组（可选）。授权是在context级别执行的。不同的context可以共享相同的Realm（用户数据库），所以Realm是与调用它的context分开定义的。你可以通过context配置中的名称识别Realm。', '', '', '');

$_tipsdb['realtimerpt'] = new DAttrHelp("Real-Time Statistics", 'The Real-Time Statistics link leads to a page with a real-time server status report. This is a convenient tool to monitor the system.    The report shows a snapshot of your server statistics. The refresh rate for this snapshot  is controlled by the Refresh Interval drop-down list in the upper righthand corner.   The report contains the following sections: <ul><li>Server Health shows the basic server statistics, uptime, load, and anti-DDoS blocked IPs.</li>   <li>Server lists current traffic throughput, connections, and requests statistics.</li>  <li>Virtual Host shows request processing statuses and external application statuses for each virtual host.</li>  <li>External Application lists the external applications currently running and their usage statistics.   The CGI daemon process lscgid is always running as an external application.</li> </ul>   Many of the rows in the Real-Time Statistics feature a graph icon.  Clicking on this icon will open a graph of that row&#039;s statistics updated in real-time.   In the Server section, next to Requests, there is a link labeled (Details).  This link takes you to the Requests Snapshot, where you can view detailed information  on which clients are making certain kinds of requests or which aspects of your site  are bottlenecking. The fields in the blue area allow you to filter the snapshot to isolate  certain parts of your server or look for clients that are performing certain actions.', '', '', '');

$_tipsdb['recaptchaAllowedRobotHits'] = new DAttrHelp("允许的机器人点击", '设置每10秒允许“好机器人”通过的点击次数。 当服务器处于高负载状态时，僵尸程序仍会受到限制。<br/><br/>默认值是3.', '', '整数', '');

$_tipsdb['recaptchaBotWhiteList'] = new DAttrHelp("Bot白名单", '自定义允许访问的用户代理列表。 将受到“好机器人”的限制，包括allowedRobotHits。', '', '用户代理列表，每行一个。 支持正则表达式。', '');

$_tipsdb['recaptchaMaxTries'] = new DAttrHelp("最大尝试次数", '“最大尝试次数”指定在拒绝访客之前允许的最大reCAPTCHA次尝试次数。<br/>默认值是 3.', '', '整数', '');

$_tipsdb['recaptchaRegConnLimit'] = new DAttrHelp("连接限制", '激活reCAPTCHA所需的并发连接数（SSL和非SSL）。 在并发连接数高于该数字之前，将使用reCAPTCHA。<br/><br/>默认值是15000.', '', '整数', '');

$_tipsdb['recaptchaSecretKey'] = new DAttrHelp("密匙", '密匙是Google通过其reCAPTCHA服务提供的私钥。 如未设置将使用默认的密匙', '', '', '');

$_tipsdb['recaptchaSiteKey'] = new DAttrHelp("网站密匙", '站点密钥是Google通过其reCAPTCHA服务提供的公共密钥。 如果未设置，将使用默认的站点密钥。', '', '', '');

$_tipsdb['recaptchaSslConnLimit'] = new DAttrHelp("SSL连接限制", '激活reCAPTCHA所需的并发SSL连接数。在并发连接数高于该数字之前，将使用reCAPTCHA。<br/><br/>默认值是 10000.', '', '整数', '');

$_tipsdb['recaptchaType'] = new DAttrHelp("reCAPTCHA类型", '指定要与密钥对一起使用的reCAPTCHA类型。 如果未提供密钥对，并且此设置设置为 未设置，将使用隐形类型的默认密钥对。<br/>复选框将显示一个复选框reCAPTCHA，以供访问者验证。<br/>隐形将尝试自动验证reCAPTCHA，如果成功，将重定向到所需的页面。<br/><br/>默认值为隐形。', '', '从列表中选择', '');

$_tipsdb['recaptchaVhReqLimit'] = new DAttrHelp("并发请求限制", '激活reCAPTCHA所需的并发请求数。 当并发请求数超过该值时将启用reCAPTCHA， 默认值为15000.', '', '整数', '');

$_tipsdb['redirectContext'] = new DAttrHelp("重定向 Context", 'A Redirect Context can be used to forward one URI or a group of URIs to another location. The destination URI can be either on the same web site (an internal redirect) or an absolute URI pointing to another web site (an external redirect).', '', '', '');

$_tipsdb['renegProtection'] = new DAttrHelp("SSL密钥重新协商保护", '指定是否启用SSL密钥重新协商保护以 防御基于SSL握手的攻击。 默认值为“是”。', '可以在侦听器和虚拟主机级别启用此设置。', '从单选框选择', '');

$_tipsdb['required'] = new DAttrHelp("Require（授权的用户/组）", '指定哪些用户/用户组可以访问此context。 这里允许你使用一个用户/组数据库(在 &quot;域&quot;中指定)访问多个context， 但只允许该数据库下特定的用户/组访问这个context。', '', 'Syntax is compatible with Apache&#039;s Require directive. For example: <ul> <li>user username [username ...]<br/>Only listed users can access this context.</li> <li> group groupid [groupid ...]<br/>Only users belonging to the listed groups can access this context.</li> </ul> If this setting is not specified, all valid users will be allowed to access this resource.', '');

$_tipsdb['requiredPermissionMask'] = new DAttrHelp("必需的权限掩码", '为静态文件指定必需的权限掩码。 例如，如果只允许所有人都可读的文件可以被输出，将该值设置为0004。 用man 2 stat命令了解所有可选值。', '', 'octal numbers', '');

$_tipsdb['respBuffer'] = new DAttrHelp("响应缓冲", '指定是否要从外部应用程序缓冲收到的响应。 如果nph-(Non-Parsed-Header)脚本被检测到，则关闭具有完整HTTP头的缓冲响应。', '', '从列表中选择', '');

$_tipsdb['restart'] = new DAttrHelp("Apply Changes/Graceful Restart", 'By clicking Graceful Restart, a new server process will be started.  For Graceful Restart, the old server process will only exit after all requests  to it have been finished (or the &quot;平滑重启超时时长(secs)&quot; limit has been reached).   Configuration changes are applied at the next restart.  Graceful Restart will apply these changes without any server downtime.', 'Graceful restart takes less than 2 seconds to generate a new server process.', '', '');

$_tipsdb['restrained'] = new DAttrHelp("访问管制", '指定虚拟机根($VH_ROOT)以外的文件是否可以通过这个网站访问。 如果设置是Yes，只可以访问$VH_ROOT下的文件， 访问指向$VH_ROOT以外文件或目录的符号链接或context指向都将被阻止。 尽管如此，这里不会限制CGI脚本的访问。 这个选项在共享主机下非常有用。 &quot;跟随符号链接&quot;可以设置成Yes来允许用户使用在$VH_ROOT下的符号链接， $VH_ROOT以外的则不可以。', '[安全建议] 在共享主机环境下打开该功能。', '从单选框选择', '');

$_tipsdb['restrictedDirPermissionMask'] = new DAttrHelp("脚本目录限制权限掩码", 'Specifies the restricted permission mask of parent directories of script files that the server will not serve. For example, to prohibit serving PHP scripts in a directory that is group and world writable, set the mask to 022. Default value is 000. This option can be used to prevent serving scripts under a directory of  uploaded files.<br/><br/>See man 2 stat for all values.', '', 'octal numbers', '');

$_tipsdb['restrictedPermissionMask'] = new DAttrHelp("限制权限掩码", '为不能输出的静态文件指定限制权限掩码。 例如，要禁止服务可执行文件，将掩码设置为0111。<br/><br/>用man 2 stat命令了解所有可选值。', '', 'octal numbers', '');

$_tipsdb['restrictedScriptPermissionMask'] = new DAttrHelp("脚本限制权限掩码", '为不能服务的脚本文件指定限制权限掩码。 例如，要禁止服务属组可写和全局可写的PHP脚本， 设置掩码为022。默认值是000。<br/><br/>用man 2 stat命令了解所有可选值。', '', 'octal numbers', '');

$_tipsdb['retryTimeout'] = new DAttrHelp("重试超时时间 (secs)", '指定服务器在重试之前，等待出现通信问题的外部应用程序的时间。', '', '整数', '');

$_tipsdb['reusePort'] = new DAttrHelp("启用REUSEPORT", '使用SO_REUSEPORT套接字选项将传入流量分配给多个工作进程。 此设置仅对multi-worker可证有效。 启用后，所有工作程序都将自动绑定到此侦听器，并且将忽略“绑定”配置。<br/><br/>Default value: On', '', '从单选框选择', '');

$_tipsdb['rewriteBase'] = new DAttrHelp("重写基准", '指定重写规则的基准URL。', '', 'URL', '');

$_tipsdb['rewriteInherit'] = new DAttrHelp("重写继承", '指定是否从父级context继承重写规则。 如果启用重写但不继承，将启用本context的重写基准及重写规则。', '', '从单选框选择', '');

$_tipsdb['rewriteLogLevel'] = new DAttrHelp("日志级别", '指定重写调试输出的详细程度。 此值的范围是0-9。 设置为0将禁用日志记录。 设置为9将产生 最详细的日志。 服务器和虚拟主机的错误日志&quot;日志级别&quot; 至少设置为INFO才能使此选项生效。 这对测试重写规则很有帮助。', '', '整数', '');

$_tipsdb['rewriteMapLocation'] = new DAttrHelp("Location", 'Specifies the location of the rewrite map using the syntax MapType:MapSource.<br/>LiteSpeed&#039;s rewrite engine supports three types of rewrite maps: <ul> 	<li><b>Standard Plain Text</b> <blockquote> 		<b>MapType:</b> txt; <br/>		<b>MapSource:</b> file path to a valid plain ASCII file.  </blockquote> 		Each line of this file should contain two elements separated  		by blank spaces. The first element is the key and the second 		element is the value. Comments can be added with a leading &quot;#&quot; 		sign.  	</li> 	<li><b>Randomized Plain Text</b> <blockquote> 		<b>MapType:</b> rnd;<br/>		<b>MapSource:</b> file path of a valid plain ASCII file. </blockquote> 		File format is similar to the Standard Plain Text file, except that the 		second element can contain multiple choices separated by a &quot;|&quot; 		sign and chosen randomly by the rewrite engine. 	</li> 	<li><b>Internal Function</b> <blockquote> 	    <b>MapType:</b> int;<br/>		<b>MapSource:</b> Internal string function  </blockquote> 		4 functions are available: 		<ul> 			<li><b>toupper:</b> converts lookup key to upper cases.</li> 			<li><b>tolower:</b> converts lookup key to lower cases.</li> 			<li><b>escape:</b> perform URL encoding on lookup key.</li> 			<li><b>unescape:</b> perform URL decoding on lookup key.</li> 		</ul> 	</li> 	The following map types available in Apache 	have not been implemented in LiteSpeed:<br/>Hash File and External Rewriting Program. </ul> The implementation of LiteSpeed&#039;s rewrite engine follows the specifications of Apache&#039;s rewrite engine. For more details about rewrite map, please refer to <a href="   http://httpd.apache.org/docs/current/mod/mod_rewrite.html " target="_blank" rel="noopener noreferrer">   Apache&#039;s mod_rewrite document </a>.', '', 'String', '');

$_tipsdb['rewriteMapName'] = new DAttrHelp("名称", 'Specifies a unique name for the rewrite map at the virtual host  level. This name will be used by a mapping-reference in rewrite rules. When referencing this name, one of the following syntaxes should be used: <blockquote><code> $\{MapName:LookupKey\}<br/>$\{MapName:LookupKey|DefaultValue\} </code></blockquote><br/>The implementation of LiteSpeed&#039;s rewrite engine follows the specifications of Apache&#039;s rewrite engine. For more details about rewrite maps, please refer to <a href="   http://httpd.apache.org/docs/current/mod/mod_rewrite.html " target="_blank" rel="noopener noreferrer">   Apache&#039;s mod_rewrite document </a>.', '', 'string', '');

$_tipsdb['rewriteRules'] = new DAttrHelp("重写规则", '指定虚拟主机级别的重写规则。<br/><br/>请勿在此处添加任何目录级重写规则。 如果您在.htaccess有任何目录级的重写规则，则应该使用uri&quot;/&quot;创建一个静态context， 并在那里添加重写规则。 <br/><br/>重写规则由一个RewriteRule组成，并可可以在多个RewriteCond之后。 <ul>   <li>每行仅能有一条规则</li>   <li>     RewriteCond 和 RewriteRule 遵循Apache的rewrite语法。 只需从Apache配置文件中复制并粘贴重写规则即可。   </li>   <li>     LiteSpeed和Apache mod_rewrite之间有细微差别：     <ul>       <li>         %\{LA-U:variable\} and %\{LA-F:variable\} 在Litespeed 重写中将被忽略       </li>       <li>         Litespeed中加入了两个新变量:         %\{CURRENT_URI\}表示正在处理的URL         %\{SCRIPT_NAME\}表示为相应的CGI环境变量。       </li>       <li>         Litespeed在遇到[L]后为了避免循环将停止处理此及此后的重写规则         而Apache mod_rewrite将仅停止处理该条重写规则。此行为类似于apachemod_rewrite中的[END]标志。       </li>     </ul>   </li> </ul><br/>LiteSpeed的重写规则遵循Apache的重写规范。  有关重写规则的更多详细信息，请参阅 <a href="   http://httpd.apache.org/docs/current/mod/mod_rewrite.html " target="_blank" rel="noopener noreferrer">   Apache&#039;s mod_rewrite document(英文文档) </a> 和 <a href="   http://httpd.apache.org/docs/current/rewrite/ " target="_blank" rel="noopener noreferrer">   Apache&#039;s URL rewriting guide(英文文档) </a>.', '', 'string', '');

$_tipsdb['rubyBin'] = new DAttrHelp("Ruby路径", 'Ruby可执行文件的路径。 通常是 /usr/bin/ruby 或 /usr/local/bin/ruby (取决于Ruby的安装文章)', '', '绝对路径', '');

$_tipsdb['runOnStartUp'] = new DAttrHelp("开机自启", '指定是否在服务器启动时启动外部应用程序.  只适用于可以管理自己子进程的外部应用程序，并且&quot;实例数&quot;值设置为&quot;1&quot;。<br/><br/>如果启用，将在服务器启动时而不是运行时创建外部进程.<br/><br/>When selecting &quot;Yes (Detached mode)&quot;, all detached process can be restarted at the Server level or Virtual Host level by touching the &#039;.lsphp_restart.txt&#039; file under the $SERVER_ROOT/admin/tmp/ or $VH_ROOT/ directory respectively.<br/><br/>Default value: Yes (Detached mode)', '[性能建议] 如果配置的外部进程有很大的启动开销，比如Rails，那么应该启用这个选项来减少首页响应时间。', '从单选框选择', '');

$_tipsdb['runningAs'] = new DAttrHelp("运行方式", '指定服务器进程运行的用户/组。 在安装之前运行configure命令时，使用参数“--with-user”和“ --with-group”进行设置。 要重置这些值，必须重新运行configure命令并重新安装。', '[安全] 服务器不应该以特权用户列如&quot;root&quot;来运行. 将服务器配置为以非特权用户/组运行非常重要 没有登入和Shell的权限 nobody用户(组)通常是个不错的选择.', '', '');

$_tipsdb['servAction'] = new DAttrHelp("Actions", 'Six actions are available from this menu: Graceful Restart, Toggle Debug Logging, Server Log Viewer, Real-Time Statistics,  Version Manager, and Compile PHP.  <ul><li>&quot;Apply Changes/Graceful Restart&quot; restarts server process gracefully without interrupting requests in process.</li> 	<li>&quot;Toggle Debug Logging&quot; turns debug logging on or off.</li> 	<li>&quot;Server Log Viewer&quot; allows you to view the server log through the log viewer.</li> 	<li>&quot;Real-Time Statistics&quot; allows you to view real-time server status.</li> 	<li>&quot;Version Management&quot; allows you to download new versions of LSWS and switch between different versions. 	<li>Compile PHP allows you to compile PHP for LiteSpeed Web Server. </ul>', 'The shell utility $SERVER_ROOT/bin/lswsctrl can be used to control the server processes as well,  but requires a login shell.', '', '');

$_tipsdb['servModules'] = new DAttrHelp("Server Modules", 'The Server module configuration globally defines the module configuration data.  Once defined, the Listeners and Virtual Hosts have access to the modules and module configurations. <br/><br/>All modules that are to be processed must be registered in the Server configuration. The Server configuration also  defines the default values for module parameter data.  These values can be inherited  or overridden by the Listener and Virtual Host configuration data.<br/><br/>Module priority is only defined at server level and is inherited by the Listener and Virtual Host configurations.', '', '', '');

$_tipsdb['serverName'] = new DAttrHelp("服务器名称", '该服务器的唯一名称。您可以在此填写 $HOSTNAME 。', '', '', '');

$_tipsdb['serverPriority'] = new DAttrHelp("优先级", '指定服务进程的优先级。数值范围从 -20 到 20。数值越小，优先级越高。', '[性能] 通常，较高的优先级会导致繁忙的服务器上的Web性能稍有提高。 不要将优先级设置为高于数据库进程的优先级。', 'Integer number', '');

$_tipsdb['servletContext'] = new DAttrHelp("Servlet Context", 'Servlets can be imported individually through Servlet Contexts. A Servlet Context just specifies the URI for the servlet and the name of the servlet engine. You only need to use this when you do not want to import the whole web application or you want to protect different servlets with different authorization realms. This URI has the same requirements as for a &quot;Java Web App Context&quot;.', '', '', '');

$_tipsdb['servletEngine'] = new DAttrHelp("Servlet Engine", '指定为该Web应用程序提供服务的Servlet Engine的名称。 Servlet引擎必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义。', '', '从列表中选择', '');

$_tipsdb['setUidMode'] = new DAttrHelp("外部应用程序设置UID模式", '定如何为外部程序进程设置用户ID。可以选择下面三种方式： <ul><li>Server UID: 为外部应用程序设置与服务器用户/组ID相同的用户/组ID。</li>     <li>CGI File UID: 为外部应用CGI程序设置基于可执行文件的用户/组ID。该选项仅适用于CGI，不适用于FastCGI或LSPHP。</li>     <li>Doc Root UID: 为外部应用程序设置基于当前虚拟机根目录的用户/组ID。</li> </ul><br/><br/>Default value: Server UID', '[安全建议] 在共享主机环境中，建议使用CGI File UID  或 Doc Root UID模式来防止一个虚拟主机下的文件被另一个虚拟主机的外部应用程序访问。', '从列表中选择', '');

$_tipsdb['shHandlerName'] = new DAttrHelp("处理器名称", '当处理器类型为FastCGI，Web服务器，LSAPI，负载均衡器或Servlet引擎时， 指定处理脚本文件的外部程序名称。', '', '从列表中选择', '');

$_tipsdb['shType'] = new DAttrHelp("类型", '指定处理这些脚本文件的外部程序类型。 可用类型有：CGI, FastCGI, Web服务器, LSAPI应用程序, 负载均衡器, 或 Servlet引擎。 对于FastCGI, Web服务器和Servlet引擎，需要指定&quot;处理器名称&quot;。 这是在&quot;外部应用&quot;部分预设定的外部程序名称。', '', '从列表中选择', '');

$_tipsdb['shmDefaultDir'] = new DAttrHelp("默认SHM目录", '将共享内存的默认目录更改为指定的路径。 如果该目录不存在，则将创建该目录。除非另有说明，否则所有SHM数据都将存储在此目录中。', '', 'path', '');

$_tipsdb['showVersionNumber'] = new DAttrHelp("服务器签名", '指定是否在响应头的Server参数中显示服务器签名和版本号。 有三个选项: 当设置为Hide Version时、只显示LiteSpeed。当设置为 Show Version，显示LiteSpeed和版本号。  设置为Hide Full Header时，整个Server头都不会显示在响应报头中。', '[安全建议] 如果你不想暴露服务器的版本号，设置为Hide Version。', '从列表中选择', '');

$_tipsdb['sname'] = new DAttrHelp("Name - Server", 'The unique name that identifies this server. This is the  &quot;服务器名称&quot; specified in the general configuration.', '', '', '');

$_tipsdb['sndBufSize'] = new DAttrHelp("发送缓冲区大小", '每个TCP套接字的发送缓冲区的大小。设定值为0使用 操作系统默认的缓冲区大小。65535是允许的最大缓冲区大小。', '[性能建议] 建议将此值保留为“未设置”或设置为0以使用操作系统的默认缓冲区大小。<br/>[性能建议] 如果您的网站服务大量的静态文件，增加发送缓冲区 大小来提高性能。<br/>[性能建议] 将此值设置为较低的值将减少吞吐量和每个套接字的内存使用率，从而在内存成为瓶颈时允许服务器拥有更多并发套接字。', '整数', '');

$_tipsdb['softLimit'] = new DAttrHelp("连接软限制", '指定来自单个IP的并发连接的软限制。 并发连接数低于&quot;连接硬限制&quot;时，此软限制可以在&quot;宽限期（秒）&quot;期间临时超过， 但Keep-Alive连接将被尽快断开，直到连接数低于软限制。 如果&quot;宽限期（秒）&quot;之后，连接数仍然超过软限制，相应的IP将被封锁 &quot;禁止期（秒）&quot;所设置的时长。<br/>例如，如果页面包含许多小图像，浏览器可能会尝试同时建立许多连接，尤其是HTTP/1.0客户端。你应当在短时间内允许这些连接。<br/><br/>HTTP/1.1客户端还可能建立多个连接，以加快下载，另外SSL需要为非SSL连接建立单独的连接。确保限制设置正确， 以免影响正常服务。建议限制在5与10之间。', '安全建议] 一个较低的数字将使得服务器可以服务更多独立的客户。<br/>[安全建议] 受信任的IP或子网不受影响。<br/>[性能建议] 使用大量并发客户端进行性能评测时，请设置一个较高的值。', '整数', '');

$_tipsdb['sslCert'] = new DAttrHelp("SSL私钥和证书", '每个SSL侦听器都需要成对的SSL私钥和SSL证书。 多个SSL侦听器可以共享相同的密钥和证书。<br/>您可以使用SSL软件包自行生成SSL私钥， 例如OpenSSL。 SSL证书也可以从授权证书颁发机构（如VeriSign或Thawte）购买。 您也可以自己签署证书。 自签名证书将不受Web浏览器的信任，并且不应在公共网站上使用。 但是，自签名证书足以供内部使用，例如 用于加密到LiteSpeed Web服务器的WebAdmin控制台的流量。', '', '', '');

$_tipsdb['sslDefaultCiphers'] = new DAttrHelp("默认密码套件", 'SSL证书的默认密码套件。<br/><br/>Default value: Server Internal Default (Based on current best practices)', '', 'Colon-separated string of cipher specifications.', '');

$_tipsdb['sslEnableMultiCerts'] = new DAttrHelp("启用多个SSL证书", '允许侦听器/虚拟主机设置多个SSL证书。 如果启用了多个证书，则证书/密钥应遵循命名方案。 如果证书名为server.crt，则其他可能的证书名称为server.crt.rsa， server.crt.dsa，server.crt.ecc。 如果为“未设置”，则默认为“否”。', '', '从单选框选择', '');

$_tipsdb['sslOCSP'] = new DAttrHelp("OCSP装订", '在线证书状态协议（OCSP）是更加有效的检查数字证书是否有效的方式。 它通过与另一台服务器（OCSP响应服务器）通信，以获取证书有效的验证，而不是通过证书吊销列表（CRL）进行检查。<br/><br/>OCSP装订是对该协议的进一步改进，允许服务器以固定的时间间隔而不是每次请求证书时与OCSP响应程序进行检查。 有关更多详细信息，请参见<a href="   https://zh.wikipedia.org/wiki/%E5%9C%A8%E7%BA%BF%E8%AF%81%E4%B9%A6%E7%8A%B6%E6%80%81%E5%8D%8F%E8%AE%AE " target="_blank" rel="noopener noreferrer">   OCSP Wikipedia页面 </a>。', '', '', '');

$_tipsdb['sslOcspProxy'] = new DAttrHelp("OCSP Proxy", 'Socket address used as the proxy server address for OCSP verification. Leave this setting unset If not using a proxy.<br/><br/>Default value: not set', '', 'Socket Address', '');

$_tipsdb['sslProtocolSetting'] = new DAttrHelp("SSL协议", '自定义侦听器接受的SSL协议。', '', '', '');

$_tipsdb['sslSessionCache'] = new DAttrHelp("启用SSL会话缓存", '使用OpenSSL的默认设置启用会话ID缓存。 服务器级别设置必须设置为“是”才能使虚拟主机设置生效。<br/>默认值:<br/><b>服务器级别:</b> Yes<br/><b>虚拟主机级别:</b> Yes', '', '从单选框选择', '');

$_tipsdb['sslSessionCacheSize'] = new DAttrHelp("会话缓存大小 (bytes)", '设置要存储在缓存中的SSL会话ID的最大数量。 默认值为1,000,000。', '', 'Integer number', '');

$_tipsdb['sslSessionCacheTimeout'] = new DAttrHelp("会话缓存过期时间 (secs)", '此值确定需要重新握手之前，会话ID在缓存中有效的时间。 默认值为 3,600.', '', 'Integer number', '');

$_tipsdb['sslSessionTicketKeyFile'] = new DAttrHelp("SSL会话记录单密钥文件", 'Allows the SSL Ticket Key to be created/maintained by an administrator. The file must be 48 bytes long. If this option is left empty, the load balancer will generate and rotate its own set of keys.<br/><br/>IMPORTANT: To maintain forward secrecy, it is strongly recommended to change the key every <b>SSL Session Ticket Lifetime</b> seconds. If this cannot be done, it is recommended to leave this field empty.', '', 'Path', '');

$_tipsdb['sslSessionTicketLifetime'] = new DAttrHelp("SSL会话记录单生存时间(secs)", '此值确定需要重新握手之前会话记录单有效的时间。 默认值为3600。', '', 'Integer number', '');

$_tipsdb['sslSessionTickets'] = new DAttrHelp("启用会话记录单", '使用OpenSSL的默认会话票证设置启用会话记录单。 服务器级别设置必须设置为“是”才能使虚拟主机设置生效。<br/>默认值:<br/><b>服务器级别:</b> Yes<br/><b>虚拟主机级别:</b> Yes', '', '从单选框选择', '');

$_tipsdb['sslStrongDhKey'] = new DAttrHelp("SSL强DH密钥", '指定是使用2048位还是1024位DH密钥进行SSL握手。 如果设置为“是”，则2048位DH密钥将用于2048位SSL密钥和证书。 在其他情况下，仍将使用1024位DH密钥。 默认值为“是”。 Java的早期版本不支持大于1024位的DH密钥大小。 如果需要Java客户端兼容性，则应将其设置为“否”。', '', 'radio', '');

$_tipsdb['statDir'] = new DAttrHelp("统计输出目录", '实时统计报告文件将写入的目录。 默认目录是 <b>/tmp/lshttpd/</b> .', '在服务器操作期间，.rtreport文件将每秒写入一次。 为避免不必要的磁盘写入，请将其设置为RAM磁盘。<br/>.rtreport文件可以与第三方监控软件一起使用，以跟踪服务器的运行状况。', '绝对路径', '');

$_tipsdb['staticReqPerSec'] = new DAttrHelp("静态请求/秒", '指定每秒可处理的来自单个IP的静态内容请求数量（无论与该IP之间建立了多少个连接）。<br/><br/>当达到此限制时，所有后来的请求将被延滞到下一秒。 对于动态内容请求的限制与本限制无关。 每个客户端的请求限制可以在服务器或虚拟主机级别设置。 虚拟主机级别的设置将覆盖服务器级别的设置。', '[安全] 受信任的IP或子网不受影响。', '整数', '');

$_tipsdb['statuscode'] = new DAttrHelp("状态码", '指定外部重定向响应状态码。 如果状态码在300和399之间，可以指定&quot;目标URI&quot;。', '', '从列表中选择', '');

$_tipsdb['suexecGroup'] = new DAttrHelp("suEXEC组", '在当前context 级别，以该组身份运行。 必须将虚拟主机级别的<b>suEXEC用户</b>或外部应用程序级别的<b>用户运行身份</b>设置为<b>suEXEC Group</b>才能生效。<br/><br/>可以使用<b>运行方式组</b>设置在外部应用程序级别覆盖此配置。<br/>Default value: <b>suExec User</b> setting value', '', 'Valid group name or uid', '');

$_tipsdb['suexecUser'] = new DAttrHelp("suEXEC 用户", '在当前context 级别，以该用户身份运行。 如果设置了该项，则该值将覆盖虚拟主机级别<b>外部应用程序设置UID模式</b>的设置。<br/><br/>此配置可被外部应用程序级别的<b>以用户身份运行</b>设置覆盖。<br/><br/>Default value: Not Set', '', 'Valid user name or uid.', '');

$_tipsdb['suffix'] = new DAttrHelp("后缀", '指定将由此脚本处理程序处理的脚本文件后缀。 后缀必须是唯一的。', '服务器将为列表中的第一个后缀自动添加特殊的MIME类型 (&quot;application/x-httpd-[suffix]&quot;) 。 例如，将为后缀“ php53”添加MIME类型“ application/x-httpd-php53”。 首先需要在&quot;MIME设置&quot;设置中设置后缀。<br/>尽管我们在此字段中列出了后缀，但是脚本处理程序使用MIME类型而非后缀来确定要处理的脚本。<br/>[性能和安全建议] 仅指定您真正需要的后缀。', 'Comma delimited list with period &quot;.&quot; character prohibited.', '');

$_tipsdb['swappingDir'] = new DAttrHelp("交换目录", '指定交换文件的存放目录。 服务器在chroot模式启动时，该路径相对于新的根目录， 否则，它相对于真正的根目录。<br/><br/>Litespeed使用自己的虚拟内存 以降低系统的内存使用量。虚拟内存和磁盘交换会用来存储大的请求内容和 动态响应。交换目录应设置在有足够剩余空间的磁盘上。<br/><br/>默认值: /tmp/lshttpd/swap', '[性能建议] 将交换目录设置在一个单独的磁盘上，或者增加最大读写缓冲区大小以避免交换。', '绝对路径', '');

$_tipsdb['templateFile'] = new DAttrHelp("模板文件", '指定此模板配置文件的路径。 该文件必须位于$SERVER_ROOT/conf/templates/中，且文件名带有“ .conf”。 如果您指定的文件不存在，则在尝试保存模板后，将出现错误，为“CLICK TO CREATE”。  点击该链接将生成一个新的空模板文件。 当您删除模板时，该条目将从您的配置中删除，但实际的模板配置文件不会被删除。', '', 'path', '');

$_tipsdb['templateFileRef'] = new DAttrHelp("模板中的文件名", 'Specifies a path for the file to be used for member virtual hosts.   Variable $VH_NAME or $VH_ROOT must appear in the path so  each member virtual host will have its own file.', '', 'string', '');

$_tipsdb['templateName'] = new DAttrHelp("模板名称", '模板的唯一名称。', '', '', '');

$_tipsdb['templateVHAliases'] = new DAttrHelp("别名", '指定虚拟主机的备用名称。 所有可能的主机名和IP地址都应添加到此列表中。 名称中允许使用通配符 * 和 ?。 为不在端口80上的网站附加:<port>。<br/><br/>别名将在以下情况下使用： <ol>   <li>在处理请求时匹配主机标头中的主机名。</li>   <li>作为诸如FrontPage或AWstats之类的域名配置.</li>  <li>基于虚拟主机模板配置侦听器到虚拟主机的映射。</li> </ol>', '', 'Comma-separated list of domain names.', '');

$_tipsdb['templateVHConfigFile'] = new DAttrHelp("Instantiated VHost Config file", 'Specifies the location of the config file generated when you instantiate a member virtual host.  Variable $VH_NAME must appear in the path so each virtual host will have its own file. Must be located under  $SERVER_ROOT/conf/vhosts/. This config file will be created only after you move a member vhost out of the template  through instantiation.', '$VH_NAME/vhconf.conf is recommended for easy management.', 'String with $VH_NAME variable and .conf suffix', '');

$_tipsdb['templateVHDocRoot'] = new DAttrHelp("文件根目标", 'Specifies the unique path for each member virtual host&#039;s document root.   Variable $VH_NAME or $VH_ROOT must appear in the path so  each member virtual host will have its own document root.', '', 'path with $VH_NAME or $VH_ROOT variable', '$VH_ROOT/public_html/ or $SERVER_ROOT/$VH_NAME/public_html.');

$_tipsdb['templateVHDomain'] = new DAttrHelp("域名", '指定该成员虚拟主机的主域名。 如果留白，将使用成员虚拟主机名, 这应该是符合标准的域名,但是您也可以使用IP地址。 对于不在端口80上的网站，建议附加:<port> 对于包含域名的配置，可以使用变量$VH_DOMAIN来引用此域。<br/><br/>该域名将在以下情况下使用： <ol>   <li>在处理请求时匹配主机标头中的主机名。</li>   <li>作为诸如FrontPage或AWstats之类的域名配置.</li>  <li>基于虚拟主机模板配置侦听器到虚拟主机的映射。</li> </ol>', '', 'domain name', '');

$_tipsdb['templateVHName'] = new DAttrHelp("虚拟主机名", '此虚拟主机的唯一名称。 该名称在所有的虚拟主机模板和虚拟主机中不得重复。 在目录路径配置中，此名称可以由变量$VH_NAME引用。<br/><br/>如果还配置了具有相同名称的虚拟主机，则虚拟主机的配置将被忽略。', '', '', '');

$_tipsdb['templateVHRoot'] = new DAttrHelp("默认虚拟主机根", '指定使用此模板的成员虚拟主机的默认根目录。.  Variable $VH_NAME must appear in the path. This will allow each member template  to be automatically assigned a separate root directory based on its name.', '', 'path', '');

$_tipsdb['toggleDebugLog'] = new DAttrHelp("Toggle Debug Logging", 'Toggle Debug Logging toggles the value of &quot;调试级别&quot; between NONE and HIGH.  As debug logging has an impact on performance and can fill up the hard drive quickly, so &quot;调试级别&quot; should usually be set to NONE on a production server.  This feature can be used instead to turn debug logging on and off quickly  in order to debug a problem on a production server. Debug logging turned on or  off in this way will not change anything shown in your server configurations.', '&quot;Toggle Debug Logging&quot; will only work if &quot;日志级别&quot;  is set to DEBUG.   Important! Debug logging includes detailed information for each  request and response. Active debug logging will severely degrade service performance and potentially saturate disk space in a very short time. This feature should only be  used for a short period of time when trying to diagnose server issues.', '', '');

$_tipsdb['totalInMemCacheSize'] = new DAttrHelp("小文件缓存总大小 (bytes)", '指定分配用于缓存/服务小静态文件的总内存。', '', '整数', '');

$_tipsdb['totalMMapCacheSize'] = new DAttrHelp("总MMAP缓存大小 (bytes)", '指定分配用于缓存/服务中等大小静态文件的总内存。', '', '整数', '');

$_tipsdb['umask'] = new DAttrHelp("umask", '设置CGI进程默认的umask。 通过 man 2 umask命令了解详细信息。这也可作为外部应用程序&quot;umask&quot;的默认值。', '', '数值有效范围为[000] - [777]', '');

$_tipsdb['uploadPassByPath'] = new DAttrHelp("通过文件路径传递上传数据", '指定是否按文件路径传递上传数据。 如果启用，则上传时文件路径以及其他一些信息将发送到后端处理程序，而不是文件本身。 这样可以节省CPU资源和文件传输时间，但需要一些更新才能实现。 如果禁用，则文件内容将传输到后端处理程序，请求主体仍解析为文件。', '[性能] 如果向下兼容不是问题，启用此功能可加快文件上传处理速度。', '从单选框选择', '');

$_tipsdb['uploadTmpDir'] = new DAttrHelp("临时文件路径", '在扫描请求正文工作时，上传到服务器的文件将存放在临时目录中  默认值为/tmp/lshttpd/.', '', 'Absolute path or path starting with $SERVER_ROOT (for Server and VHost levels) or $VH_ROOT (for VHost levels).', '');

$_tipsdb['uploadTmpFilePermission'] = new DAttrHelp("临时文件权限", '设置<b>临时文件路径</b>目录中的文件权限。 服务器级别是全局设置，可以被虚拟主机级别的设置覆盖。', '', '3 digits octet number. Default value is 666.', '');

$_tipsdb['uri'] = new DAttrHelp("URI", '指定此context下的URI。这个URI应该以&quot;/&quot;开始。 如果一个URI以&quot;/&quot;结束，那么该context将包含这个URI下的所有下级URI。', '', 'URI', '');

$_tipsdb['useIpInProxyHeader'] = new DAttrHelp("使用报头中的客户端IP", '指定是否将在HTTP请求报头中的X-Forwarded-For参数列出的IP地址，用于 所有的IP地址相关的功能，包括 连接/带宽限制、访问控制和IP地理定位。<br/><br/>如果你的Web服务器放置在负载均衡器或代理服务器之后，此功能非常有用。 如果您选择了“仅限受信任的IP”，只有在请求来自受信任IP时，X-Forwarded-For 中的IP才会被使用。受信任IP可在服务器级别的&quot;允许列表&quot;中定义。<br/><br/>来自受信任IP的保留标头与仅受信任IP 相同，但是用于后端的X-Forwarded-For标头不会被修改为包括连接对等地址。<br/><br/>使用最后一个IP (对与 AWS ELB)将使用 &quot;X-Forwarded-For &quot;列表中的最后一个 IP 地址。如果您正在使用 AWS Elastic Load Balancer 或期望将真实 IP 附加到 &quot;X-Forwarded-For &quot;列表的末尾，请选择此选项。', '', '从列表中选择', '');

$_tipsdb['useSendfile'] = new DAttrHelp("使用sendfile()", '指定是否使用sendfile()系统调用来服务静态文件。静态文件 可以用四种不同的方式服务：内存缓存、内存映射缓存、直接读写和 sendfile()。 尺寸小于&quot;最大小文件缓存(bytes)&quot;的文件将使用内存缓存服务。尺寸大于该限制、但小于 &quot;最大MMAP文件大小(bytes)&quot;的文件，将使用内存映射缓存服务。 尺寸大于&quot;最大MMAP文件大小(bytes)&quot;的文件将通过直接读写或sendfile() 服务。Sendfile()是一个“零拷贝”系统调用，可在服务非常大的 文件时大大减少CPU的使用率。Sendfile()需要一个优化的网卡内核驱动， 因此可能不适合某些小厂商的网络适配器。', '', '从单选框选择', '');

$_tipsdb['userDBCacheTimeout'] = new DAttrHelp("用户数据库缓存超时 (secs)", '指定多久检查一次后端用户数据库变更。 在缓存中每个条目都有一个时间戳。 当缓存日期超过指定的超时时间时，将检查后端数据库是否有变化。 如果没有，时间戳将被重置为当前时间，否则会将新的数据载入。 服务器重载和平滑重启会立即清除缓存。', '[性能建议] 如果后端数据库不经常发生变更，设置较长的缓存时间来获得更好的性能。', '整数', '');

$_tipsdb['userDBLocation'] = new DAttrHelp("用户数据库地址", '指定用户数据库的位置。 建议将数据库存储在$ SERVER_ROOT / conf / vhosts / $ VH_NAME /目录下。<br/><br/>对于类型为Password File的数据库，应设置为包含用户名/密码的展平文件的路径。 您可以在WebAdmin控制台中点击文件名来进行修改。<br/>用户文件的每一行都包含一个用户名，后跟一个冒号，然后是一个crypt()加密的密码，后面还可以跟一个冒号和用户所属的组。多个组名用逗号隔开。<br/><br/>Example:<blockquote><code>john:HZ.U8kgjnMOHo:admin,user</code></blockquote>', '', 'Path to user DB file.', '');

$_tipsdb['userDBMaxCacheSize'] = new DAttrHelp("用户数据库最大缓存大小", '指定用户数据库的最大缓存大小。 最近访问的用户认证信息会被缓存在内存中以提供最佳性能。', '[性能建议] 由于更大的缓存会消耗更多的内存，更高的值可能会也可能不会提供更好的性能。 请根据您的用户数据库大小和网站使用情况来设定一个合适的大小。', '整数', '');

$_tipsdb['vaction'] = new DAttrHelp("Actions - Virtual Host", 'This field shows buttons to disable, enable, or restart the virtual host.   Actions taken on one virtual host do not affect the rest of the web server.', 'It is good idea to disable a virtual host temporarily when updating its content.', '', '');

$_tipsdb['vdisable'] = new DAttrHelp("Disable", 'The Disable action stops a running virtual host. New requests will not be accepted, but requests being processed will finish as usual.', '', '', '');

$_tipsdb['venable'] = new DAttrHelp("Enable", 'The Enable action starts up a stopped virtual host.   This allows new requests to be accepted.', '', '', '');

$_tipsdb['verifyDepth'] = new DAttrHelp("验证深度", ' Specifies how deeply a certificate should be verified before determining that the client does not have a valid certificate. The default is &quot;1&quot;.', '', '从列表中选择', '');

$_tipsdb['vhEnableBr'] = new DAttrHelp("Brotli 压缩", '指定是否为此虚拟主机启用Brotli压缩。 这个设置仅在服务器级&quot;Brotli 压缩等级 (静态文件)&quot;设置为一个非零值时有效.', '', '从单选框选择', '');

$_tipsdb['vhEnableGzip'] = new DAttrHelp("启用 GZIP 压缩", '指定是否为此虚拟主机启用GZIP压缩。 这个设置仅在服务器级&quot;启用GZIP压缩&quot; 设置为 Yes时有效.', '', '从单选框选择', '');

$_tipsdb['vhEnableQuic'] = new DAttrHelp("Enable HTTP3/QUIC", 'Enables the HTTP3/QUIC network protocol for this virtual host. For this setting to take effect, both &quot;启用HTTP3/QUIC&quot; and &quot;打开HTTP3/QUIC (UDP) 端口&quot; must also be set to Yes at the server and listener levels respectively. Default value is Yes.', 'When this setting is set to No, the HTTP3/QUIC advertisement will no longer be sent. If a browser still contains cached HTTP3/QUIC information and HTTP3/QUIC is still enabled at the server and listener levels, an HTTP3/QUIC connection will continue to be used until this information is no longer cached or an HTTP3/QUIC protocol error is encountered.', '从单选框选择', '');

$_tipsdb['vhMaxKeepAliveReq'] = new DAttrHelp("最大Keep-Alive请求数", '指定通过keep-alive(永久)连接服务的最大请求数量。当该限制值达到时连接将被断开。你可以为不同虚拟主机设置不同的数值。这个数值不能超过服务器级别的&quot;最大持续连接请求数&quot;限制值。', '[性能建议] 设置为一个合理的高数值。设置为1或比1更小的值将禁用keep-alive连接。', '整数', '');

$_tipsdb['vhModuleUrlFilters'] = new DAttrHelp("Virtual Host Module Context", 'It&#039;s a centralized place to customize module settings for virtual host contexts. Settings for a context URI will override the virtual host or the server level settings.', '', '', '');

$_tipsdb['vhModules'] = new DAttrHelp("Virtual Host Modules", 'Virtual Host module configuration data is, by default inherited from the Server module configuration.   The Virtual Host Modules are limited to the HTTP level hooks.', '', '', '');

$_tipsdb['vhName'] = new DAttrHelp("虚拟主机名", '为虚拟主机的唯一名称。建议使用虚拟主机的域名作为虚拟主机名。 虚拟主机名参数可以使用$VH_NAME变量来引用。', '', '', '');

$_tipsdb['vhRoot'] = new DAttrHelp("虚拟主机根", '指定虚拟主机的根目录。 注：这<b>不是</b>目录根。 建议将所有与该虚拟主机相关的文件 (像日志文件，html文件，CGI脚本等)都放置在这个目录下。 虚拟主机根参数可以使用$VH_ROOT变量来引用。.', '[性能建议] 将不同的虚拟主机放在不同的硬盘驱动器上。', '绝对路径或相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['vhaccessLog_fileName'] = new DAttrHelp("文件名", '访问日志的文件名', '[性能建议] 将访问日志文件放在单独的磁盘上。', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT,$VH_ROOT的相对路径。', '');

$_tipsdb['vhadminEmails'] = new DAttrHelp("管理员邮箱", '指定这个虚拟主机管理员的电子邮箱地址。', '', 'Comma separated list of email addresses', '');

$_tipsdb['vhlog_fileName'] = new DAttrHelp("文件路径", '指定日志文件的路径。', '[性能建议] 将日志文件放在单独的磁盘上。', '文件名可以是绝对路径,也可以是相对于$SERVER_ROOT,$VH_ROOT的相对路径。', '');

$_tipsdb['vhlog_logLevel'] = new DAttrHelp("日志级别", '指定日志记录级别。可用级别（从高到低）为ERROR,  WARNING, NOTICE, INFO, 和 DEBUG。 只有当消息等级高于或与当前设置相同时才被记录。 如果您希望将此设置为DEBUG，您也需要设置服务器日志级别为DEBUG。 调试的级别只能在服务器级别通过&quot;调试级别&quot;控制。', '[性能建议] 除非&quot;调试级别&quot;设置为NONE以外的日志级别, 否则DEBUG级别不会对性能产生任何影响，推荐采用。', '从列表中选择', '');

$_tipsdb['viewlog'] = new DAttrHelp("Server Log Viewer", 'The Server Log Viewer is a convenient tool for browsing the  current server log to check for errors or problems.  The log viewer  searches the server log file in blocks for the specified log level.    The default block size is 20KB. You can use the Begin, End, Next, and Prev buttons to navigate a large log file.', 'The size of a dynamically generated page is limited by &quot;动态回应主内容最大大小(bytes)&quot;.   So if the block is too big, the page might be truncated.', '', '');

$_tipsdb['virtualHostMapping'] = new DAttrHelp("虚拟主机映射", '指定侦听器和虚拟主机之间的关系。 侦听器和虚拟主机通过域名关联。 HTTP请求将被路由到具有匹配域名的虚拟主机。 一个侦听器可以映射到多个虚拟主机以获取不同的域名。 也可以从不同的侦听器映射一个虚拟主机。 一个侦听器可以允许一个具有域名值“*”的虚拟主机。 如果侦听器的映射中没有明确匹配的域名， 侦听器会将请求转发到含有&quot;*&quot;域名的虚拟主机。', '[性能] 仅添加必要的映射。 如果侦听器仅映射到一个虚拟主机，则仅设置一个通配符映射“*”。', '', '');

$_tipsdb['virtualHostName'] = new DAttrHelp("虚拟主机", '指定虚拟主机的名称。', '', '从列表中选择', '');

$_tipsdb['vname'] = new DAttrHelp("Name - Virtual Host", 'The unique name that identifies this virtual host. This is the &quot;虚拟主机名&quot;  you specified when setting up this virtual host.', '', '', '');

$_tipsdb['vreload'] = new DAttrHelp("Restart - Virtual Host", 'The Restart action causes the web server to load the newest configuration  for this virtual host. Requests being processed will finish with the old configuration.  The new configuration will only apply for new requests. All changes to a virtual host  can be applied on the fly this way.', '', '', '');

$_tipsdb['vstatus'] = new DAttrHelp("Status - Virtual Host", 'The current status of a virtual host.   The status can be: Running, Stopped, Restart Required,   or Running - Removed from Configuration.  <ul>     <li>Running means the virtual host is loaded and in service.</li>     <li>Stopped means the virtual host is loaded but not in service (disabled). </li>     <li> Restart Required means this is a newly added virtual host and          the server has not yet loaded its configuration. </li>     <li>Running - Removed from Configuration means the virtual host has been deleted      from the server&#039;s configuration but it is still in service. </li> </ul>', '', '', '');

$_tipsdb['wsaddr'] = new DAttrHelp("地址", 'WebSocket 后端使用的唯一网络套接字地址。 支持 IPv4 套接字、IPv6 套接字和 Unix 域套接字 (UDS)。 IPv4 和 IPv6 套接字可用于网络上的通信。 只有当 WebSocket 后端与服务器在同一台机器上时，才能使用 UDS。', ' If the WebSocket backend runs on the same machine,  UDS is preferred. If you have to use an IPv4 or IPv6 socket,  set the IP address to localhost or 127.0.0.1, so the WebSocket backend  is inaccessible from other machines.<br/> Unix Domain Sockets generally provide higher performance than IPv4 or IPv6 sockets.', 'IPv4/IPv6 address:port, UDS://path, or unix:path', '127.0.0.1:5434 <br/>UDS://tmp/lshttpd/php.sock<br/>unix:/tmp/lshttpd/php.sock');

$_tipsdb['wsgiBin'] = new DAttrHelp("WSGI路径", 'LiteSpeed Python Web服务器的可执行文件（lswsgi）的路径。<br/><br/>This 可执行文件是通过使用LiteSpeed的WSGI LSAPI模块编译Python生成的。', '', '绝对路径', '');

$_tipsdb['wsgiDefaults'] = new DAttrHelp("Python WSGI Default Settings", 'Default configurations for Python WSGI applications. These settings can be overriden at the context level.', '', '', '');

$_tipsdb['wsuri'] = new DAttrHelp("URI", '指定将使用此WebSocket后端的URI。 仅当该URI包含WebSocke协议升级请求时，才会将其流量转发到WebSocket后端。<br/><br/>没有议升级请求的流量将自动转发到此URI所属的Context。 如果此URI不存在任何Context，则LSWS会将该流量视为访问位置为$DOC_ROOT/URI的静态Context。', '', '普通URI（以“/”开头）。 如果URI以“/”结尾，则此WebSocket后端将包括此URI下的所有子URI。', '将WebSocket代理与Context结合使用，可以使您以不同的方式在同一页面上提供不同类型的流量，从而优化性能。 您可以将WebSocket流量发送到WebSocket后端，同时设置静态Context以使LSWS为页面的静态内容服务，或者可以设置LSAPI Context以使LSWS可以为PHP内容服务（这两个LSWS都比WebSocket后端更高效）。');


$_tipsdb['EDTP:GroupDBLocation'] = array('建议将数据库存储在 $SERVER_ROOT/conf/vhosts/$VH_NAME/目录。');

$_tipsdb['EDTP:UDBgroup'] = array('如果在此处添加了组信息，则此信息将用于资源授权，并且涉及该用户的任何组数据库设置都将被忽略。','您可以使用逗号分隔多个组。 空格字符将被视为组名的一部分。');

$_tipsdb['EDTP:accessControl_allow'] = array('You can set up access control at server, virtual host and context levels. If there is access control at server level, the virtual host rules will be applied after the server rules are satisfied.','Input format can be an IP like 192.168.0.2, a sub-network like 192.168.*, or a subnet/netmask like 192.168.128.5/255.255.128.0.','If you have trusted IP or sub-network, then you must specify them in allowed list by adding a trailing &quot;T&quot; such as 192.168.1.*T. Trusted IP or sub-network is not limited by connection/throttling limit.');

$_tipsdb['EDTP:accessControl_deny'] = array('To deny access from certain address, put &quot;ALL&quot; in allowed list, and put subnet or IP in denied list. To allow only certain IP or subnet to access the site, put &quot;ALL&quot; in denied list and specify the address in the allowed list.');

$_tipsdb['EDTP:accessDenyDir'] = array('如果要拒绝对特定目录的访问，请输入完整路径。 输入后跟*的路径将禁用所有子目录。','Path can be either absolute or relative to $SERVER_ROOT, 使用逗号分隔.','如果同时启用了<b>跟随符号链接</b>和<b>检查符号链接</b>，则会根据拒绝访问的目录检查符号链接。');

$_tipsdb['EDTP:accessLog_fileName'] = array('日志文件路径可以是绝对路径，也可以是相对于$SERVER_ROOT的相对路径。');

$_tipsdb['EDTP:aclogUseServer'] = array('如果需要，您可以禁用此虚拟主机的访问日志记录以降低磁盘I/O的占用。');

$_tipsdb['EDTP:adminEmails'] = array('您可以输入多个管理员电子邮件：使用逗号分隔。');

$_tipsdb['EDTP:adminOldPass'] = array('出于安全原因，如果忘记了管理员密码，将无法从WebAdmin Console进行更改。 请改用以下shell命令： <br><br> /usr/local/lsws/admin/misc/admpass.sh.  <br><br> 该脚本将删除所有输入的管理员用户ID，并用一个管理员用户覆盖它们。');

$_tipsdb['EDTP:allowBrowse'] = array('Static context can be used to map a URI to a directory either outside document root or within it. The directory can be absolute path or relative to document root(default), $VH_ROOT or $SERVER_ROOT.','Check &quot;Accessible&quot; will allow browsing static files in this context. You may want to disable it to prevent viewing static files, for e.g. when you update the content.');

$_tipsdb['EDTP:appType'] = array('','');

$_tipsdb['EDTP:as_location'] = array('The App Server context is for easy configuration of running Rack/Rails, WSGI, or Node.js applications. You only need to specify the root location of your application in the &quot;Location&quot; field.');

$_tipsdb['EDTP:as_startupfile'] = array('','');

$_tipsdb['EDTP:autoFix503'] = array('启用<b>自动修复503错误</b>后，监视进程将自动启动新的服务器进程，并且如果检测到崩溃，服务将立即恢复。');

$_tipsdb['EDTP:backlog'] = array('Local applications can be started by the web server. In this case, you need to specify the path, backlog and number of instances.');

$_tipsdb['EDTP:binPath'] = array('','');

$_tipsdb['EDTP:bubbleWrap'] = array('');

$_tipsdb['EDTP:bubbleWrapCmd'] = array('');

$_tipsdb['EDTP:cgi_path'] = array('CGI context可用于指定仅包含CGI脚本的目录。 路径可以是绝对路径，也可以是相对于$SERVER_ROOT, $VH_ROOT的相对路径或$DOC_ROOT（默认）。  对于cgi-bin目录，路径和URI必须以“ /”结尾。','If only a specific script is needed in that directory, it is recommended to create a CGI context for that script only. In this case, path and URI need not be a directory. For e.g., path can be ~/myapp/myscript.pl, URI can be /myapp/myscript.pl. All other files will not be served as CGI.');

$_tipsdb['EDTP:checkSymbolLink'] = array('检查符号链接仅在跟随符号链接启用后才会生效。 这将控制是否根据禁止访问的目录检查符号链接。');

$_tipsdb['EDTP:compressibleTypes'] = array('Compressible Types is a list of MIME types that are compressible, separated by commas. You can use wildcard &quot;*&quot; for MIME types, like */*, text/*. You can put &quot;!&quot; in front to exclude certain types. The order of the list is important if you use &quot;!&quot;. For e.g., a list like &quot;text/*, !text/css, !text/js&quot; will compress all text file except for css and js.');

$_tipsdb['EDTP:ctxType'] = array('<b>Static</b> context can be used to map a URI to a directory either outside document root or within it.','<b>Java Web App</b> context is used to automatically import a predefined Java Application in an AJPv13 compilant Java servlet engine.','<b>Servlet</b> context is used to import a specific servlet under a web application.','<b>Fast CGI</b> context is a mount point of Fast CGI application.','<b>LiteSpeed SAPI</b> context can be used to associate a URI with an LSAPI application.','<b>Proxy</b> context enables this virtual host to serve as a transparant reverse proxy server to an external web server or application server.','<b>CGI</b> context can be used to specify a directory only contains CGI scripts.','<b>Load Balancer</b> context can be used to assign a different cluster for that context.','<b>Redirect</b> context can set up an internal or external redirect URI.','<b>App Server</b> context is specifically used for Rack/Rails, WSGI, and Node.js applications.<br>','<b>Module handler</b> context is a mount point of hander type modules.');

$_tipsdb['EDTP:docRoot'] = array('如果文档根目录尚不存在，服务器不会自动创建它。  请确保该目录存在并且由正确的用户拥有。','在此处设置文档根目录，它可以是绝对路径，也可以相对于$SERV_ROOT或$VH_ROOT','在此虚拟主机中，文档根目录称为$DOC_ROOT，可在其他路径配置中使用。');

$_tipsdb['EDTP:domainName'] = array('输入您希望此侦听器响应的所有域。 使用逗号“”分隔域。','您只能选择一个虚拟主机来处理所有未指定的域，并在域中添加“ *”。');

$_tipsdb['EDTP:enableDynGzipCompress'] = array('动态压缩仅在启用压缩后才有效。');

$_tipsdb['EDTP:enableExpires'] = array('可以在服务器/虚拟主机/Context 级别设置过期。较低级别的设置将覆盖较高级别的设置。在覆盖优先级方面： <br><br> Context Level > 虚拟主机级别 > 服务器级别 <br><br>');

$_tipsdb['EDTP:enableRecaptcha'] = array('When this setting is set to <b>Yes</b> at the Server level, reCAPTCHA Protection can still be disabled at the Virtual Host level.');

$_tipsdb['EDTP:errURL'] = array('您可以为不同的错误代码设置自定义的错误页面。');

$_tipsdb['EDTP:expiresByType'] = array('按类型过期将覆盖默认设置。 每个条目的格式均为“MIME-type=A|Mseconds”， 两者之间没有空格。 您可以输入多个以逗号分隔的条目。');

$_tipsdb['EDTP:expiresDefault'] = array('过期语法，“A|Mseconds”表示在基准时间（A或M）加上指定的时间（以秒为单位）之后，文件将 到期。 “ A”表示客户端访问时间，“ M”表示文件修改时间。 您可以使用其他MIME类型覆盖此默认设置:A86400表示文件将根据客户端访问时间在1天后过期。','以下是一些常见的数字：1小时= 3600秒，1天= 86400秒，1周= 604800秒， 1个月= 2592000秒，1年= 31536000秒。');

$_tipsdb['EDTP:extAppAddress'] = array('地址可以是IPv4套接字地址 &quot;IP:PORT&quot;, 像192.168.1.3:7777 和localhost:7777 或者 Unix域套接字 地址 &quot;UDS://path&quot; 例如 UDS://tmp/lshttpd/myfcgi.sock.','UDS是在chroot环境下进行的。','对于本地应用程序，由于安全性和更好的性能，Unix域套接字是首选。 如果你必须使用IPv4套接字，将IP部分设置为localhost或127.0.0.1， 这样其他机器就无法访问应用程序。');

$_tipsdb['EDTP:extAppName'] = array('Give a name that easy to remember, other places will refer to this app by its name.');

$_tipsdb['EDTP:extAppType'] = array('You can set up external Fast CGI application and AJPv13 (Apache JServ Protocol v1.3) compatible servlet engine.');

$_tipsdb['EDTP:extWorkers'] = array('Load balancing workers must be previously defined.','Available ExtApp Types are fcgi(Fast CGI App), lsapi(LSAPI App), servlet(Servlet/JSP Engine), proxy(Web Server).','Different types of external applications can be mixed in one load balancing cluster.');

$_tipsdb['EDTP:externalredirect'] = array('在此处设置重定向URI。 如果是外部重定向，则可以指定状态代码。 内部重定向必须以&quot;/&quot;开始, 外部重定向可以以 &quot;/&quot;或&quot;http(s)://&quot;开始.');

$_tipsdb['EDTP:extraHeaders'] = array('The Header Operations setting is backwards compatible with the old &#039;header_name: value1,value2,...&#039; syntax, which is equivalent to setting the header + values using the &#039;Header&#039; directive.');

$_tipsdb['EDTP:fcgiapp'] = array('Fast CGI context是Fast CGI应用的一个挂载点，Fast CGI应用程序必须在服务器级或虚拟主机级预先定义。');

$_tipsdb['EDTP:followSymbolLink'] = array('如果启用了跟随符号链接，您仍然可以在虚拟主机级别禁用它。');

$_tipsdb['EDTP:gdb_groupname'] = array('组名应仅包含字母和数字。');

$_tipsdb['EDTP:gzipCompressLevel'] = array('压缩动态态内容的级别,范围从{val}1{/} (最低)到{val}9{/} (最高)');

$_tipsdb['EDTP:hardLimit'] = array('设置来自一个合理的的并发连接限制有助于抵御DoS攻击。');

$_tipsdb['EDTP:indexUseServer'] = array('您可以将默认服务器级别设置用于索引文件，也可以使用自己的服务器级别设置。','除了服务器级别的设置之外，您还可以使用其他设置。','您可以通过选择不使用服务器级别设置并将虚拟主机级别设置留为空白来禁用索引文件。','您可以在context级别启用/禁用“自动索引”。');

$_tipsdb['EDTP:javaServletEngine'] = array('如果Servlet Engine在其他计算机上运行，建议将webapps目录复制到本地。 否则，您必须将文件放在可访问的公用网络驱动器中，这可能会影响性能。');

$_tipsdb['EDTP:javaWebApp_location'] = array('Java web app context is used to automatically import a predefined Java Application in an AJPv13 compilant Java servlet engine, the servlet engine should be set up in external app section (either server or virtual host level).','Location is the directory that contains web application files, which includes WEB-INF/ sub directory.','The web server will automatically import configuration file of web application, which usually is WEB-INF/web.xml under the driectory specified by &quot;location&quot;.');

$_tipsdb['EDTP:listenerIP'] = array('从列表中选择一个IP地址，如果您未指定特定地址，则系统将绑定到该计算机上的所有可用IP地址。');

$_tipsdb['EDTP:listenerName'] = array('Give listener a name that is easy to understand and remember.');

$_tipsdb['EDTP:listenerPort'] = array('在此IP上为此侦听器输入一个唯一的端口号。 只有超级用户（root）可以使用低于1024的端口。端口80是默认的HTTP端口;端口443是默认的HTTPS端口。');

$_tipsdb['EDTP:listenerSecure'] = array('为<b>安全</b>选择“是”将使此侦听器使用https。 然后，您必须进一步进行配置SSL settings.');

$_tipsdb['EDTP:logUseServer'] = array('如果<b>使用服务器日志</b>被设置为&quot;是&quot;,日志将被写入服务器级别设置的服务器文件。');

$_tipsdb['EDTP:log_enableStderrLog'] = array('Stderr日志与服务器日志位于同一目录中。 如果启用，则所有输出到stderr的外部程序都将记录在此文件中。');

$_tipsdb['EDTP:log_fileName'] = array('日志文件路径可以是绝对路径，也可以是相对于$SERVER_ROOT的相对路径。');

$_tipsdb['EDTP:log_rollingSize'] = array('如果当前日志文件超过回滚大小，将创建一个新的日志文件。 文件大小以字节为单位，可以有多种输入格式：10240、10K或1M。');

$_tipsdb['EDTP:maxCGIInstances'] = array('限制CGI程序可以使用的资源,有助于抵御DoS攻击。','“最大CGI实例数”控制Web服务器可以启动多少个CGI进程。');

$_tipsdb['EDTP:maxReqHeaderSize'] = array('Numbers can be represented as 10240 or 10K.');

$_tipsdb['EDTP:mime'] = array('可以从上一页编辑MIME设置。 您可以指定mime配置文件的位置 可以是绝对路径，也可以是相对于$SERVER_ROOT的路径。');

$_tipsdb['EDTP:nodeBin'] = array('');

$_tipsdb['EDTP:phpIniOverride'] = array('');

$_tipsdb['EDTP:procSoftLimit'] = array('Process soft/hard limit controls how many processes are allowed for one user. This includes all the processes spawned by CGI application. OS level limit is used if not set.','Set to 0 or empty will use operation system default value for all soft/hard limits.','The soft limit is the value that the kernel enforces for the corresponding resource. The hard limit acts as a ceiling for the soft limit');

$_tipsdb['EDTP:proxyWebServer'] = array('代理context使此虚拟主机可用作外部Web服务器或应用程序服务器的透明反向代理服务器。','External web server must be pre-defined under External App at server or virtual host level.');

$_tipsdb['EDTP:realm'] = array('A Context can be protected with a predefined realm, which is set up in the virtual host security section. Optionally, an alternative name and additional requirements can be specified.');

$_tipsdb['EDTP:realmName'] = array('在此处定义您的HT访问域，可将其用于contexts。');

$_tipsdb['EDTP:recaptchaAllowedRobotHits'] = array('');

$_tipsdb['EDTP:recaptchaBotWhiteList'] = array('');

$_tipsdb['EDTP:recaptchaMaxTries'] = array('');

$_tipsdb['EDTP:recaptchaRegConnLimit'] = array('');

$_tipsdb['EDTP:recaptchaSecretKey'] = array('');

$_tipsdb['EDTP:recaptchaSiteKey'] = array('如果服务器管理多个域，则必须取消勾选相应密匙的&quot;验证 reCAPTCHA 解决方案的来源&quot;选项框。  否则，reCAPTCHA验证将无法正常进行。');

$_tipsdb['EDTP:recaptchaSslConnLimit'] = array('');

$_tipsdb['EDTP:recaptchaType'] = array('');

$_tipsdb['EDTP:recaptchaVhReqLimit'] = array('');

$_tipsdb['EDTP:restrained'] = array('Turn on Restrained in a shared hosting enviroment.');

$_tipsdb['EDTP:reusePort'] = array('');

$_tipsdb['EDTP:rewriteMapLocation'] = array('Enter URI for location. URI must start with &quot;/&quot;.');

$_tipsdb['EDTP:rewriteRules'] = array('这里应使用虚拟主机级别的重写规则，例如在Apache虚拟主机配置文件中找到的规则。 请勿在此处添加任何目录级重写规则。 如果您在.htaccess有任何目录级的重写规则，则应该使用uri&quot;/&quot;创建一个静态context， 并在那里添加重写规则。');

$_tipsdb['EDTP:rubyBin'] = array('<b>Ruby路径</b>是ruby可执行文件的绝对路径. 例如, /usr/local/bin/ruby.');

$_tipsdb['EDTP:serverName'] = array('服务器进程的用户和组设置无法修改。 这是在安装过程中设置的。 您必须重新安装才能更改此选项。');

$_tipsdb['EDTP:servletEngine'] = array('如果Servlet Engine在其他计算机上运行，建议将webapps目录复制到本地。 否则，您必须将文件放在可访问的公用网络驱动器中，这可能会影响性能。');

$_tipsdb['EDTP:shHandlerName'] = array('除CGI之外，还需要在“外部应用程序”中预定义其他处理程序。');

$_tipsdb['EDTP:shType'] = array('脚本处理程序可以是CGI，FCGI应用程序，模块处理程序，Servlet引擎或Web代理服务器。');

$_tipsdb['EDTP:sndBufSize'] = array('数值可以为10240、10K或1M。','如果发送/接收缓冲区大小设置为0，则将使用操作系统默认的TCP缓冲区大小。');

$_tipsdb['EDTP:softLimit'] = array('在此处设置IP级别的速率限制。 数值将四舍五入至4K单位。 设置为“ 0”以禁用宽带限制。','只要没有超过硬限制,连接数就可以在宽限期内暂时超过软限制,超过宽限时间后,如果连接数仍然超过软限制，相应的IP将被封锁屏蔽时长设置的时间');

$_tipsdb['EDTP:sslSessionCache'] = array('会话缓存使客户端可以在设置的时间内恢复会话，而不必重新执行SSL握手。 您可以使用<b>启用会话缓存</ b>为客户端分配会话ID，或者通过创建和使用会话记录单来做到这一点。');

$_tipsdb['EDTP:sslSessionTicketKeyFile'] = array('Session tickets will be rotated automatically if the tickets are being generated by the server. If using the <b>SSL Session Ticket Key File</b> option to create and manage your own session tickets, you must be rotate the tickets yourself using a cron job.');

$_tipsdb['EDTP:swappingDir'] = array('建议将交换目录放置在本地磁盘上，例如/tmp。 应不惜一切代价避免网络驱动器。 交换将在配置的内存I/O缓冲区耗尽时进行。');

$_tipsdb['EDTP:userDBLocation'] = array('建议将数据库存储在 $SERVER_ROOT/conf/vhosts/$VH_NAME/目录.');

$_tipsdb['EDTP:vhRoot'] = array('所有目录必须预先存在。这个设置不会为你创建目录。如果你要创建一个新的虚拟主机， 你可以创建一个空的根目录，并从头开始设置； 也可以将软件包中附带的DEFAULT虚拟根目录复制到这个虚拟主机根目录中， 并修改为对应用户所有。','虚拟主机根目录($VH_ROOT)可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径.');

$_tipsdb['EDTP:vhaccessLog_fileName'] = array('日志文件路径可以是绝对路径,也可以是相对于$SERVER_ROOT, $VH_ROOT的相对路径.');

$_tipsdb['EDTP:vhadminEmails'] = array('您可以输入多个管理员电子邮件，以逗号分隔。');

$_tipsdb['EDTP:vhlog_fileName'] = array('日志文件路径可以是绝对路径,也可以是相对于$SERVER_ROOT, $VH_ROOT的相对路径.','如果要将日志级别设置为DEBUG，则还必须将服务器日志级别也设置为DEBUG。 调试级别由服务器调试级别控制。 请仅在必要时才使用DEBUG级别，因为它会对服务器性能产生很大影响，并且可以快速填满磁盘空间。');

$_tipsdb['EDTP:virtualHostName'] = array('选择要映射到此侦听器的虚拟主机。','如果尚未设置要映射的虚拟主机，则可以跳过此步骤，稍后再进行');

$_tipsdb['EDTP:wsgiBin'] = array('');
