<?php 

global $_tipsdb;

$_tipsdb['AIOBlockSize'] = new DAttrHelp("AIO块大小", '指定AIO的发送块大小。 此块大小乘以正在处理的文件总数应小于服务器物理内存，否则AIO不会带来帮助。如果服务器内存充足，可以选择更大的大小。<br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>128K</span>', '', '从列表中选择', '');

$_tipsdb['CACertFile'] = new DAttrHelp("CA证书文件", '指定包含链式证书所需证书颁发机构（CA）证书的文件。 此文件只是按优先顺序串联的PEM编码证书文件。它可以替代&quot;CA证书路径&quot;使用，也可以与其配合使用。这些证书用于客户端证书身份验证，并用于构建服务器证书链。服务器证书链会随服务器证书一起发送给浏览器。', '', '文件路径', '');

$_tipsdb['CACertPath'] = new DAttrHelp("CA证书路径", '指定保存证书颁发机构（CA）证书的目录。这些证书用于客户端证书身份验证，并用于构建服务器证书链。服务器证书链会随服务器证书一起发送给浏览器。', '', '路径', '');

$_tipsdb['CGIPriority'] = new DAttrHelp("CGI优先级", '指定外部应用程序进程的优先级。数值范围从<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-20</span>到<span class=&quot;lst-inline-token lst-inline-token--value&quot;>20</span>。数值越小，优先级越高。<br/>CGI进程不能拥有比Web服务器更高的优先级。如果这个优先级数值被设置为低于 服务器的优先级数值，则将使用服务器优先级作为替代。', '', '整数', '');

$_tipsdb['CPUHardLimit'] = new DAttrHelp("CPU硬限制(秒)", '以秒为单位，指定CGI进程的CPU占用时间限制。 如果进程持续占用CPU时间，达到硬限制，则进程将被强制杀死。如果没有设置该限制，或者限制设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>， 操作系统的默认设置将被使用。', '', '非负整数', '');

$_tipsdb['CPUSoftLimit'] = new DAttrHelp("CPU软限制(秒)", '以秒为单位，指定CGI进程的CPU占用时间限制。当进程达到 软限制时，将收到通知信号。如果没有设置该限制，或者限制设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>， 将使用操作系统的默认设置。', '', '非负整数', '');

$_tipsdb['DHParam'] = new DAttrHelp("DH参数", '指定DH密钥交换所需的Diffie-Hellman参数文件的位置。', '', '文件路径', '');

$_tipsdb['GroupDBLocation'] = new DAttrHelp("组数据库位置", '指定组数据库的位置。<br/>组信息可以在用户数据库或在这个独立的组数据库中设置。 用于用户验证时，将首先检查用户数据库。 如果用户数据库同样包含组信息，组数据库将不被检查。<br/>对于类型为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Password File</span>的数据库， 组数据库地址应当是到达包含有组定义的平面文件的路径。 你可以在WebAmin控制台中点击文件名来修改这个设置。<br/>每一行组文件应当包含一个组名， 组名后面跟一个冒号，并在冒号后面使用空格来分割组中的用户名。 例如: <blockquote><code>testgroup: user1 user2 user3</code></blockquote>', '', '文件路径', '');

$_tipsdb['HANDLER_RESTART'] = new DAttrHelp("Hook::HANDLER_RESTART 优先级", '设置此模块回调在HTTP Handler Restart Hook中的优先级。<br/><br/>当Web服务器需要丢弃当前响应并从头开始处理时会触发HTTP Handler Restart Hook，例如请求了内部重定向时。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['HTTP_AUTH'] = new DAttrHelp("Hook::HTTP_AUTH 优先级", '设置此模块回调在HTTP Authentication Hook中的优先级。<br/><br/>HTTP Authentication Hook会在资源映射之后、处理程序处理之前触发。它发生在HTTP内置身份验证之后，可用于执行额外的身份验证检查。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['HTTP_BEGIN'] = new DAttrHelp("Hook::HTTP_BEGIN 优先级", '设置此模块回调在HTTP Begin Hook中的优先级。<br/><br/>TCP/IP连接开始HTTP会话时会触发HTTP Begin Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['HTTP_END'] = new DAttrHelp("Hook::HTTP_END 优先级", '设置此模块回调在HTTP Session End Hook中的优先级。<br/><br/>HTTP连接结束时会触发HTTP Session End Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['L4_BEGINSESSION'] = new DAttrHelp("Hook::L4_BEGINSESSION 优先级", '设置此模块回调在L4 Begin Session Hook中的优先级。<br/><br/>TCP/IP连接开始时会触发L4 Begin Session Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['L4_ENDSESSION'] = new DAttrHelp("Hook::L4_ENDSESSION 优先级", '设置此模块回调在L4 End Session Hook中的优先级。<br/><br/>TCP/IP连接结束时会触发L4 End Session Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['L4_RECVING'] = new DAttrHelp("Hook::L4_RECVING 优先级", '设置此模块回调在L4 Receiving Hook中的优先级。<br/><br/>TCP/IP连接接收数据时会触发L4 Receiving Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['L4_SENDING'] = new DAttrHelp("Hook::L4_SENDING 优先级", '设置此模块回调在L4 Sending Hook中的优先级。<br/><br/>TCP/IP连接发送数据时会触发L4 Sending Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['MAIN_ATEXIT'] = new DAttrHelp("Hook::MAIN_ATEXIT 优先级", '设置此模块回调在Main At Exit Hook中的优先级。<br/><br/>Main At Exit Hook会在主（控制器）进程即将退出前触发。这是主进程调用的最后一个Hook点。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['MAIN_INITED'] = new DAttrHelp("Hook::MAIN_INITED 优先级", '设置此模块回调在Main Initialized Hook中的优先级。<br/><br/>Main Initialized Hook会在启动时触发一次，时间是在主（控制器）进程完成服务器配置和初始化之后、处理任何请求之前。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['MAIN_POSTFORK'] = new DAttrHelp("Hook::MAIN_POSTFORK 优先级", '设置此模块回调在Main Postfork Hook中的优先级。<br/><br/>Main Postfork Hook会在新的工作进程启动（fork）之后，由主（控制器）进程立即触发。每个工作进程都会调用此Hook，可能发生在系统启动期间或工作进程重启时。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['MAIN_PREFORK'] = new DAttrHelp("Hook::MAIN_PREFORK 优先级", '设置此模块回调在Main Prefork Hook中的优先级。<br/><br/>Main Prefork Hook会在新的工作进程启动（fork）之前，由主（控制器）进程立即触发。每个工作进程都会调用此Hook，可能发生在系统启动期间或工作进程重启时。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['RCVD_REQ_BODY'] = new DAttrHelp("Hook::RCVD_REQ_BODY 优先级", '设置此模块回调在HTTP Received Request Body Hook中的优先级。<br/><br/>Web服务器完成接收请求正文数据时会触发HTTP Received Request Body Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['RCVD_RESP_BODY'] = new DAttrHelp("Hook::RCVD_RESP_BODY 优先级", '设置此模块回调在HTTP Received Response Body Hook中的优先级。<br/><br/>Web服务器后端完成接收响应正文时会触发HTTP Received Response Body Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['RECV_REQ_BODY'] = new DAttrHelp("Hook::RECV_REQ_BODY 优先级", '设置此模块回调在HTTP Receive Request Body Hook中的优先级。<br/><br/>Web服务器接收请求正文数据时会触发HTTP Receive Request Body Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['RECV_REQ_HEADER'] = new DAttrHelp("Hook::RECV_REQ_HEADER 优先级", '设置此模块回调在HTTP Receive Request Header Hook中的优先级。<br/><br/>Web服务器收到请求头时会触发HTTP Receive Request Header Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['RECV_RESP_BODY'] = new DAttrHelp("Hook::RECV_RESP_BODY 优先级", '设置此模块回调在HTTP Receive Response Body Hook中的优先级。<br/><br/>Web服务器后端接收响应正文时会触发HTTP Receive Response Body Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['RECV_RESP_HEADER'] = new DAttrHelp("Hook::RECV_RESP_HEADER 优先级", '设置此模块回调在HTTP Receive Response Header Hook中的优先级。<br/><br/>Web服务器创建响应头时会触发HTTP Receive Response Header Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['SEND_RESP_BODY'] = new DAttrHelp("Hook::SEND_RESP_BODY 优先级", '设置此模块回调在HTTP Send Response Body Hook中的优先级。<br/><br/>Web服务器即将发送响应正文时会触发HTTP Send Response Body Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['SEND_RESP_HEADER'] = new DAttrHelp("Hook::SEND_RESP_HEADER 优先级", '设置此模块回调在HTTP Send Response Header Hook中的优先级。<br/><br/>Web服务器准备发送响应头时会触发HTTP Send Response Header Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['UDBgroup'] = new DAttrHelp("组", '该用户所属组的逗号分隔列表。用户只能访问属于这些组的资源。 如果在此处添加了组信息，则此信息将用于资源授权，并且涉及该用户的任何组数据库设置都将被忽略。', '', '', '');

$_tipsdb['UDBpass'] = new DAttrHelp("新密码", '密码可以是任意长度，可以包含任何字符。', '', '', '');

$_tipsdb['UDBpass1'] = new DAttrHelp("重新输入密码", '密码可以是任意长度，可以包含任何字符。', '', '', '');

$_tipsdb['UDBusername'] = new DAttrHelp("用户名", '仅包含字母和数字的用户名。（无特殊字符）', '', '', '');

$_tipsdb['URI_MAP'] = new DAttrHelp("Hook::URI_MAP 优先级", '设置此模块回调在HTTP URI Map Hook中的优先级。<br/><br/>Web服务器将URI请求映射到上下文时会触发HTTP URI Map Hook。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['VHlsrecaptcha'] = new DAttrHelp("CAPTCHA 保护", 'CAPTCHA保护是一种用于减轻服务器负载的服务。当下列情况之一发生后，CAPTCHA保护将被激活。激活后，所有非信任客户端（按配置）发出的请求都会被重定向到CAPTCHA验证页面。验证完成后，客户端会被重定向到目标页面。<br/><br/>下列情况将启用CAPTCHA保护： 1. 服务器或虚拟主机并发请求计数超过连接限制。<br/>2. 启用了Anti-DDoS，并且客户端以可疑的方式访问了URL。 客户端将首先重定向到CAPTCHA，而不是在触发时被拒绝。<br/>3. 提供了新的重写规则环境，可通过RewriteRules激活CAPTCHA。可以设置“verifycaptcha”将客户端重定向到CAPTCHA。特殊值“: deny”可在客户端失败次数过多时拒绝它。例如，[E=verifycaptcha]会一直重定向到CAPTCHA，直到验证通过。[E=verifycaptcha: deny]会一直重定向到CAPTCHA，直到达到最大尝试次数，之后客户端将被拒绝。', '', '', '');

$_tipsdb['WORKER_ATEXIT'] = new DAttrHelp("Hook::WORKER_ATEXIT 优先级", '设置此模块回调在Worker At Exit Hook中的优先级。<br/><br/>Worker At Exit Hook会在工作进程即将退出前触发。这是工作进程调用的最后一个Hook点。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['WORKER_POSTFORK'] = new DAttrHelp("Hook::WORKER_POSTFORK 优先级", '设置此模块回调在Worker Postfork Hook中的优先级。<br/><br/>Worker Postfork Hook会在工作进程由主（控制器）进程创建后，由工作进程触发。请注意，对应的Main Postfork Hook可能会在此回调之前或之后由主进程调用。<br/><br/>只有模块在此处有挂钩点时，此设置才会生效。如果未设置，优先级将使用模块中定义的默认值。', '', '-6000到6000之间的整数。值越小，优先级越高。', '');

$_tipsdb['accessAllowed'] = new DAttrHelp("允许访问列表", '指定允许访问此上下文下资源的IP地址或子网。综合 &quot;拒绝访问列表&quot;项的配置以及服务器/虚拟主机级别访问控制， 可访问性将以客户端IP所符合的最小范围来确定。', '', '逗号分隔的IP地址/子网列表。', '网络可以写成<span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.0/255.255.255.0</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1</span> 或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.*</span>。');

$_tipsdb['accessControl'] = new DAttrHelp("登入限制", '指定哪些子网络和/或IP地址可以访问该服务器。 这是影响所有的虚拟主机的服务器级别设置。您还可以为每个虚拟主机设置登入限制。虚拟主机的设置不会覆盖服务器设置。<br/>是否阻止/允许一个IP是由允许列表与阻止列表共同决定。 如果你想阻止某个特定IP或子网，请在&quot;允许列表&quot;中写入<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>ALL</span>，并在&quot;拒绝列表&quot;中写入需要阻止的IP或子网。 如果你想允许某个特定的IP或子网，请在&quot;拒绝列表&quot;中写入<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>ALL</span>，并在&quot;允许列表&quot;中写入需要允许的IP或子网。 单个IP地址是被允许访问还是禁止访问取决于该IP符合的最小限制范围。<br/>信任的IP或子网络可以在&quot;允许列表&quot;列表中添加后缀“T”来指定。受信任的IP或子网不受连接数/流量限制。 只有服务器级别的登入限制才可以设置受信任的IP或子网。', '[安全建议] 用此项设置适用于所有虚拟主机的常规限制。', '', '');

$_tipsdb['accessControl_allow'] = new DAttrHelp("允许列表", '指定允许的IP地址或子网的列表。 可以使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ALL</span>。', '[安全建议] 在服务器级别设置的受信任的IP或子网不受连接/节流限制。', '逗号分隔的IP地址或子网列表。 结尾加上“T”可以用来表示一个受信任的IP或子网，如<span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.*T</span>。', '子网: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.0/255.255.255.0</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.0/24</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.*</span>.<br/>IPv6 地址: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>::1</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>[::1]</span><br/>IPv6 子网: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>3ffe:302:11:2:20f:1fff:fe29:717c/64</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>[3ffe:302:11:2:20f:1fff:fe29:717c]/64</span>.');

$_tipsdb['accessControl_deny'] = new DAttrHelp("拒绝列表", '指定不允许的IP地址或子网的列表。', '', '逗号分隔的IP地址或子网列表。 可以使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ALL</span>。', '子网: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.0/255.255.255.0</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.0/24</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.*</span>.<br/>IPv6 地址: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>::1</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>[::1]</span><br/>IPv6 子网: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>3ffe:302:11:2:20f:1fff:fe29:717c/64</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>[3ffe:302:11:2:20f:1fff:fe29:717c]/64</span>.');

$_tipsdb['accessDenied'] = new DAttrHelp("拒绝访问列表", '指定哪些IP地址或子网不允许访问此上下文下的资源。 综合&quot;允许访问列表&quot;项的配置以及服务器/虚拟主机级别访问控制， 可访问性将以客户端IP所符合的最小范围来确定。', '', '逗号分隔的IP地址/子网列表。', '子网络可以写成<span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.0/255.255.255.0</span>, <span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1</span> 或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>192.168.1.*</span>。');

$_tipsdb['accessDenyDir'] = new DAttrHelp("拒绝访问的目录", '指定应该拒绝访问的目录。 将包含敏感数据的目录加入到这个列表，以防止向客户端意外泄露敏感文件。 在路径后加一个“*”，可包含所有子目录。 如果&quot;跟随符号链接&quot;和&quot;检查符号链接&quot;都被启用， 符号链接也将被检查是否在被拒绝访问目录中。', '[安全建议] 至关重要: 此设置只能防止服务这些目录中的静态文件。 这不能防止外部脚本如PHP、Ruby、CGI造成的泄露。', '以逗号分隔的目录列表。', '');

$_tipsdb['accessLog_bytesLog'] = new DAttrHelp("字节记录", '指定带宽字节日志文件的路径。设置后，将创建一份兼容cPanel面板的带宽日志。这将记录 一个请求传输的总字节数，包括请求内容和响应内容。', ' 将日志文件放在单独的磁盘上。', '文件名可以是绝对路径，也可以是相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['accessLog_fileName'] = new DAttrHelp("文件名", '指定访问日志文件的文件名。', '[性能] 将访问日志文件放置在单独的磁盘上。', '文件路径', '');

$_tipsdb['accessLog_logFormat'] = new DAttrHelp("日志格式", ' 指定访问日志的格式。 设置之后，它将覆盖&quot;日志头部&quot; 的设定。', '', '字符串。日志格式语法与Apache 2.0的自定义 <a href="http://httpd.apache.org/docs/current/mod/mod_log_config.html#formats" target="_blank" rel="noopener noreferrer">日志格式</a>兼容。', '<b>一般日志格式（CLF）</b><br/>    &quot;%h %l %u %t \&quot;%r\&quot; %>s %b&quot;<br/><br/><b>支持虚拟主机的一般日志格式</b><br/>    &quot;%v %h %l %u %t \&quot;%r\&quot; %>s %b&quot;<br/><br/><b>NCSA扩展/组合日志格式</b><br/>    &quot;%h %l %u %t \&quot;%r\&quot; %>s %b \&quot;%{Referer}i\&quot; \&quot;%{User-agent}i\&quot; <br/><br/><b>记录Foobar的cookie值</b><br/>    &quot;%{Foobar}C&quot;');

$_tipsdb['accessLog_logHeader'] = new DAttrHelp("日志头部", '指定是否记录HTTP请求头：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Referer</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>UserAgent</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Host</span>。', '[性能] 如果不需要在访问日志中记录这些头部信息，请关闭此功能。', '从复选框选择', '');

$_tipsdb['accessLog_pipedLogger'] = new DAttrHelp("管道日志记录", '指定一个外部应用程序，用来通过其STDIN流上的管道（文件句柄为0）接收LiteSpeed发送的访问日志数据。 指定此字段后，访问日志将仅发送到记录器应用程序，而不发送到上一个条目中指定的访问日志文件。<br/><br/>必须先在&quot;外部应用&quot;部分定义记录器应用程序。服务器级别的访问日志只能使用在服务器级别定义的外部记录器应用程序。虚拟主机级别的访问日志只能使用在虚拟主机级别定义的记录器应用程序。<br/><br/>记录器进程的启动方式与其他外部（CGI/FastCGI/LSAPI）进程相同。 这意味着它会以虚拟主机&quot;外部应用程序设置UID模式&quot;设置中指定的用户ID运行， 绝不会代表特权用户运行。<br/><br/>如果配置了记录器应用程序的多个实例，LiteSpeed Web服务器会在多个记录器应用程序之间执行简单的负载均衡。LiteSpeed服务器始终尽量将记录器应用程序数量保持在最低。只有当某个记录器应用程序无法及时处理访问日志条目时，服务器才会尝试启动该记录器应用程序的另一个实例。<br/><br/>如果记录器崩溃，Web服务器会启动另一个实例，但流缓冲区中的日志数据将会丢失。如果外部记录器无法跟上日志流的速度和数量，也可能丢失日志数据。', '', '从下拉列表选择', '');

$_tipsdb['aclogUseServer'] = new DAttrHelp("日志管理", '指定访问日志的写入位置。有三个选项： <ol> <li>写入服务器的访问日志</li> <li>为此虚拟主机创建访问日志</li> <li>禁用访问日志记录</li> </ol>', '', '从列表中选择', '');

$_tipsdb['acme'] = new DAttrHelp("ACME自动证书", '使用Automatic Certificate Management Environment(ACME)证书协议自动生成和续订SSL证书（配置后）。更多信息请参见：<br/><a href=" https://docs.openlitespeed.org/config/advanced/acme/ " target="_blank" rel="noopener noreferrer"> 自动SSL证书（ACME） </a><br/>在服务器级别将此项设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>会在整个服务器范围内禁用此设置。在其他所有情况下， 服务器级别设置都可以在虚拟主机级别覆盖。<br/>默认值：<br/><b>服务器级别：</b>Off<br/><b>虚拟主机级别：</b>继承服务器级别设置', '', '从列表中选择', '');

$_tipsdb['acme_dns_type'] = new DAttrHelp("ACME API dns_type", '（可选）执行通配符证书生成API调用时使用的dns类型。此值取决于所使用的DNS提供商， 生成非通配符证书时不需要。有关可用dns类型API值的更多信息，请参见：<br/><a href=" https://github.com/acmesh-official/acme.sh/wiki/dnsapi " target="_blank" rel="noopener noreferrer"> acme.sh-如何使用DNS API </a><br/><br/>与设置&quot;环境&quot;配合使用。', '', '', '对于DNS提供商CloudFlare，dns类型值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>dns_cf</span>。');

$_tipsdb['acme_env'] = new DAttrHelp("环境", '（可选）执行通配符证书生成API调用时要包含的环境值。所需的具体值取决于使用的DNS提供商， 生成非通配符证书时不需要。有关所需环境API值的更多信息，请参见：<br/><a href=" https://github.com/acmesh-official/acme.sh/wiki/dnsapi " target="_blank" rel="noopener noreferrer"> acme.sh-如何使用DNS API </a><br/><br/>与设置&quot;ACME API dns_type&quot;配合使用。', '', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Key=&quot;VALUE&quot;</span>键值对列表，每行一个，且每个VALUE都用双引号括起。', '对于DNS提供商CloudFlare，唯一必需的环境值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>CF_Token=&quot;YOUR_TOKEN&quot;</span>');

$_tipsdb['acme_vhenable'] = new DAttrHelp("启用", '使用Automatic Certificate Management Environment（ACME）证书协议自动生成和续订SSL证书（配置后）。更多信息请参见：<br/><a href=" https://docs.openlitespeed.org/config/advanced/acme/ " target="_blank" rel="noopener noreferrer"> 自动SSL证书（ACME） </a><br/><br/>在服务器级别将此项设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>会在整个服务器范围内禁用此设置。在其他所有情况下， 服务器级别设置都可以在虚拟主机级别覆盖。<br/><br/>默认值：<br/><b>服务器级别：</b>关闭<br/><b>虚拟主机级别：</b>继承服务器级别设置', '', '从列表中选择', '');

$_tipsdb['addDefaultCharset'] = new DAttrHelp("添加默认的字符集", '指定当内容类型为&quot;text/html&quot;或&quot;text/plain&quot;且没有参数时，是否向&quot;Content-Type&quot;响应报头添加字符集标记。设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Off</span>时，该功能禁用。设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>On</span>时，将添加&quot;自定义默认字符集&quot;中指定的字符集；如果未指定，则添加默认的&quot;iso-8859-1&quot;字符集。', '', '布尔值', '');

$_tipsdb['addMIMEType'] = new DAttrHelp("MIME类型", '为此上下文指定额外的MIME类型映射。新映射会覆盖此上下文及其子上下文下的现有映射。<br/>	   如果要将PHP脚本显示为文本文件而不是作为脚本执行，则只需覆盖.php映射到MIME类型&quot;text/plain&quot;', '', 'MIME-type1 extension extension ..., MIME-type2 extension ... 		使用逗号分隔MIME类型，使用空格分隔多个扩展名。', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>image/jpg jpeg jpg, image/gif gif</span>');

$_tipsdb['addonmodules'] = new DAttrHelp("附加模块", '选择要使用的附加模块。 如果要使用此处未列出的版本，可以手动更新源代码。（源代码位置会在PHP构建的此步骤中通过提示显示。）', '', '从复选框中选择', '');

$_tipsdb['adminEmails'] = new DAttrHelp("管理员电子邮箱", '指定服务器管理员的电子邮箱地址。 如果设置了电子邮箱地址，管理员将收到重要事件的电子邮件通知（例如， LiteSpeed服务因崩溃而自动重启或者授权即将过期）。', '电子邮件提醒功能只有在服务器有活动的MX服务器（如postfix、exim或sendmail）的情况下才会生效。', '以逗号分隔的电子邮件地址列表。', '');

$_tipsdb['adminNewPass'] = new DAttrHelp("新密码", '输入此WebAdmin用户的新密码。密码可以包含任意字符；创建或更新WebAdmin用户时必须填写。', '', '', '');

$_tipsdb['adminOldPass'] = new DAttrHelp("旧密码", '输入此WebAdmin用户的当前密码。保存用户名或密码更改前必须验证当前密码。', '', '', '');

$_tipsdb['adminRetypePass'] = new DAttrHelp("确认新密码", '再次输入新密码。它必须与新密码一致。', '', '', '');

$_tipsdb['adminUser'] = new DAttrHelp("网络管理员用户", '更改WebAdmin控制台的用户名和密码。 必须输入并验证旧密码后才能保存更改。', '', '', '');

$_tipsdb['adminUserName'] = new DAttrHelp("用户名", '指定WebAdmin控制台登录名。请使用1到25个字符：英文字母、数字、点、下划线或连字符。', '', '', '');

$_tipsdb['allowBrowse'] = new DAttrHelp("访问权限", '指定是否可以访问此上下文。设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>以拒绝访问。 您可以使用此功能来防止访问指定目录。 您可以在更新此上下文内容或此目录中有特殊数据时使用它。', '', '布尔值', '');

$_tipsdb['allowQuic'] = new DAttrHelp("打开HTTP/3/QUIC（UDP）端口", '允许映射到此侦听器的虚拟主机使用HTTP/3/QUIC网络协议。 要使此设置生效，服务器级别的&quot;启用HTTP3/QUIC&quot;也必须设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>。', '当此设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>时，仍可以通过虚拟主机级别的&quot;启用HTTP3/QUIC&quot;设置禁用HTTP/3/QUIC。', '', '');

$_tipsdb['allowSetUID'] = new DAttrHelp("允许设置UID", '指定CGI脚本是否允许set UID位。如果允许，并且CGI脚本启用了set UID位，那么无论CGI脚本代表哪个用户启动，CGI进程的用户ID都会切换为CGI脚本所有者的用户ID。<br/>默认值为&quot;Off&quot;。', '[安全建议] 尽可能不要允许CGI脚本设置UID，因为这本身存在安全风险。', '布尔值', '');

$_tipsdb['allowSymbolLink'] = new DAttrHelp("跟随符号链接", '指定在这个虚拟主机内是否要跟随符号链接。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>If Owner Match</span>选项启用后，只有在链接和目标属主一致时才跟踪符号链接。 此设置将覆盖默认的服务器级设置。', '[性能和安全性建议] 为了更好的安全性，请禁用此功能。为了获得更好的性能，启用它。', '从下拉列表选择', '');

$_tipsdb['antiddos_blocked_ip'] = new DAttrHelp("Anti-DDoS屏蔽的IP", '由Anti-DDoS保护屏蔽的IP地址列表，以逗号分隔。每个条目都以分号结尾， 并带有说明该IP地址被屏蔽原因的原因代码。<br/><br/>可能的原因代码： <ul>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>A</span>: BOT_UNKNOWN</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>B</span>: BOT_OVER_SOFT</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>C</span>: BOT_OVER_HARD</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>D</span>: BOT_TOO_MANY_BAD_REQ</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>E</span>: BOT_CAPTCHA</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>F</span>: BOT_FLOOD</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>G</span>: BOT_REWRITE_RULE</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>H</span>: BOT_TOO_MANY_BAD_STATUS</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>I</span>: BOT_BRUTE_FORCE</li> </ul><br/><br/>完整的已屏蔽IP列表也可在&quot;统计输出目录&quot;中设置的实时统计报告文件中查看。', '', '&lt;blocked_ip_address&gt;;&lt;reason_code&gt;', '1.0.100.50;E, 1.0.100.60;D, 1.0.100.70;F');

$_tipsdb['appServerContext'] = new DAttrHelp("App Server上下文", 'App Server上下文提供了一种简便方式来配置Ruby Rack/Rails、WSGI或Node.js应用程序。 通过App Server上下文添加应用程序时，只需要挂载URL和应用程序根目录。 无需额外定义外部应用程序、添加404处理程序或编写重写规则等。', '', '', '');

$_tipsdb['appType'] = new DAttrHelp("应用类型", '此上下文使用的应用程序类型。支持Rack/Rails、WSGI和Node.js。', '', '', '');

$_tipsdb['appserverEnv'] = new DAttrHelp("运行模式", '指定应用程序的运行模式：&quot;Development&quot;（开发）、 &quot;Production&quot;（生产）或&quot;Staging&quot;（预发布）。默认值为&quot;Production&quot;（生产）。', '', '从下拉列表选择', '');

$_tipsdb['as_location'] = new DAttrHelp("位置", '指定此上下文在文件系统中的对应位置。<br/><br/>默认值：$DOC_ROOT + &quot;URI&quot;', '', '可以是绝对路径，也可以是相对于$SERVER_ROOT、$VH_ROOT或$DOC_ROOT的路径。 $DOC_ROOT是默认相对路径，可以省略。<br/><br/>如果&quot;URI&quot;是正则表达式，匹配的子字符串可用于组成&quot;Root&quot;字符串。匹配的子字符串可通过&quot;$1&quot;到&quot;$9&quot;引用；&quot;$0&quot;和&quot;&&quot;可引用整个匹配字符串。此外，可以追加&quot;?&quot;和查询字符串来设置查询字符串。注意：查询字符串中的&quot;&&quot;应转义为&quot;\&&quot;。', '将&quot;位置&quot;设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/home/john/web_examples</span>时，类似<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/examples/</span>的普通URI会将请求&quot;/examples/foo/bar.html&quot;映射到文件&quot;/home/john/web_examples/foo/bar.html&quot;。<br/>要模拟Apache的mod_userdir，可将URI设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>exp: ^/~([A-Za-z0-9]+)(.*)</span>，并将&quot;位置&quot;设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/home/$1/public_html$2</span>。这样，URI /~john/foo/bar.html的请求会映射到文件/home/john/public_html/foo/bar.html。');

$_tipsdb['as_startupfile'] = new DAttrHelp("启动文件", '用于启动应用程序的文件,路径应相对于应用程序根目录。<br/><br/>默认的启动文件包括Rack/Rails的&#039;config.ru&#039;、WSGI的&#039;wsgi.py&#039;和&#039;passenger_wsgi.py&#039;，以及Node.js的&#039;app.js&#039;。', '', '相对于应用程序根目录的路径', '');

$_tipsdb['authName'] = new DAttrHelp("认证名称", '为当前上下文的授权域指定一个替代名称。 如果未指定，将使用原授权域名称。 认证名称会显示在浏览器登录弹窗中。', '', '文本', '');

$_tipsdb['autoFix503'] = new DAttrHelp("自动修复503错误", '指定是否尝试通过平滑重启服务器来修复“503 Service Unavailable”错误。 “503”错误通常由发生故障的外部应用程序引起，重启Web服务器通常可以临时修复该错误。 如果启用，当30秒内出现超过30个“503”错误时，服务器将自动重启。<br/><br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>', '', '从单选框中选择', '');

$_tipsdb['autoIndex'] = new DAttrHelp("自动索引", '在目录中，当&quot;索引文件&quot;中所列的索引文件不可用时，指定运行时是否即时生成目录索引。<br/>此选项可以在虚拟主机级别和Context级别中设置，并可以顺着目录树继承，直到被显式覆盖。 您可以自定义生成的索引页面。请查看在线Wiki操作指南。', '[安全] 建议尽可能关闭自动索引，以防止泄露机密数据。', '布尔值', '');

$_tipsdb['autoIndexURI'] = new DAttrHelp("自动索引URI", '在目录中，当&quot;索引文件&quot;中所列出的索引文件（index）不可用时，指定用来生成索引页面的URI。 LiteSpeed Web服务器使用一个外部脚本来生成索引页面，从而为定制提供最大的灵活性。 默认的脚本生成一个类似于Apache的索引页面。 如需自定义生成的索引页面，请查看在线Wiki操作指南。 待索引目录会通过环境变量&quot;LS_AI_PATH&quot;传递给脚本。', '', 'URI', '');

$_tipsdb['autoLoadHtaccess'] = new DAttrHelp("自动加载.htaccess", '如果使用<b>rewritefile</b>指令的目录还没有对应的HttpContext，则在首次访问该目录时自动加载.htaccess文件中包含的重写规则。首次加载后，必须执行平滑重启才能使对该.htaccess文件的后续修改生效。<br/><br/>虚拟主机级别设置将覆盖服务器级别设置。 默认值：<br/><br/><b>服务器级别：</b>No<br/><br/><b>虚拟主机级别：</b>继承服务器级别设置', '', '布尔值', '');

$_tipsdb['autoStart'] = new DAttrHelp("由服务器启动", '指定是否要由Web服务器自动启动应用程序。只有运行在同一台机器上的FastCGI和LSAPI应用程序才能自动启动。 &quot;地址&quot;中的IP必须是本地IP。通过LiteSpeed CGI守护进程而不是主服务器进程启动，有助于减少系统开销。<br/><br/>默认值：Yes (Through CGI Daemon)', '', '从下拉列表选择', '');

$_tipsdb['backlog'] = new DAttrHelp("待处理队列", '指定监听套接字的待处理队列长度。启用&quot;由服务器启动&quot;时需要设置。', '', '非负整数', '');

$_tipsdb['banPeriod'] = new DAttrHelp("禁止期（秒）", '指定在&quot;宽限期（秒）&quot;之后，如果连接数仍然高于 &quot;连接软限制&quot;，来自该IP的新连接将被拒绝多长时间。如果IP 经常被屏蔽，我们建议您延长禁止期以更强硬地惩罚滥用.', '', '非负整数', '');

$_tipsdb['binPath'] = new DAttrHelp("二进制文件路径", 'App Server应用程序二进制文件的位置。', '', '', '');

$_tipsdb['blockBadReq'] = new DAttrHelp("阻止异常请求", '阻止持续发送格式错误的HTTP请求的IP，阻止时长由&quot;禁止期（秒）&quot;指定。默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>。 这有助于阻止反复发送垃圾请求的僵尸网络攻击。', '', '从单选框中选择', '');

$_tipsdb['brStaticCompressLevel'] = new DAttrHelp("Brotli压缩等级（静态文件）", '指定Brotli压缩静态内容的级别。 范围从<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span>(最低)到<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span>(最高)。<br/>当设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>时，Brotli压缩将在全局禁用。<br/>默认值：5', '[性能建议] 压缩可以用来节省网络带宽。 基于文本的响应（例如html，css和javascript文件）效果最好，平均可以将其压缩为原始大小的一半。', '0到11之间的数字。', '');

$_tipsdb['bubbleWrap'] = new DAttrHelp("Bubblewrap容器", '如果希望在Bubblewrap沙箱中启动CGI进程（包括PHP程序），请设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Enabled</span>。有关使用Bubblewrap的详细信息，请参见 <a href="   https://wiki.archlinux.org/title/Bubblewrap " target="_blank" rel="noopener noreferrer">   https://wiki.archlinux.org/title/Bubblewrap </a> 。使用此设置之前，必须先在系统上安装Bubblewrap。<br/>如果服务器级别设置为“Disabled”，则无法在虚拟主机级别启用此设置。<br/>默认值：<br/><b>服务器级别：</b>Disabled<br/><b>虚拟主机级别：</b>继承服务器级别设置', '', '从列表中选择', '');

$_tipsdb['bubbleWrapCmd'] = new DAttrHelp("Bubblewrap Command", 'bubblewraps使用的完整的命令, 包括bubblewrap程序本身。 有关配置此命令的更多信息，请参见： <a href="   https://openlitespeed.org/kb/bubblewrap-in-openlitespeed/ " target="_blank" rel="noopener noreferrer">   https://openlitespeed.org/kb/bubblewrap-in-openlitespeed/ </a>. 如果未指定，将使用下面列出的默认命令。<br/>默认值: <span class=&quot;lst-inline-token lst-inline-token--command&quot;>/bin/bwrap --ro-bind /usr /usr --ro-bind /lib /lib --ro-bind-try /lib64 /lib64 --ro-bind /bin /bin --ro-bind /sbin /sbin --dir /var --dir /tmp --proc /proc --symlink../tmp var/tmp --dev /dev --ro-bind-try /etc/localtime /etc/localtime --ro-bind-try /etc/ld.so.cache /etc/ld.so.cache --ro-bind-try /etc/resolv.conf /etc/resolv.conf --ro-bind-try /etc/ssl /etc/ssl --ro-bind-try /etc/pki /etc/pki --ro-bind-try /etc/man_db.conf /etc/man_db.conf --ro-bind-try /home/$USER /home/$USER --bind-try /var/lib/mysql/mysql.sock /var/lib/mysql/mysql.sock --bind-try /home/mysql/mysql.sock /home/mysql/mysql.sock --bind-try /tmp/mysql.sock /tmp/mysql.sock  --unshare-all --share-net --die-with-parent --dir /run/user/$UID ‘$PASSWD 65534’ ‘$GROUP 65534’</span>', '', '文本', '');

$_tipsdb['certChain'] = new DAttrHelp("证书链", '指定证书是否为链式证书。存储证书链的文件必须为PEM格式， 并且证书必须按链式顺序排列，从最低级别（实际的客户端或服务器证书）到最高级别（根CA）。', '', '布尔值', '');

$_tipsdb['certFile'] = new DAttrHelp("证书文件", 'SSL证书文件的文件名。', '[安全建议] 证书文件应放在安全目录中，并且只允许服务器运行用户读取。', '文件路径', '');

$_tipsdb['cgiContext'] = new DAttrHelp("CGI上下文", 'CGI上下文会将特定目录中的脚本定义为CGI脚本。 该目录可以位于文档根目录内或外。 请求此目录下的文件时，无论文件是否可执行，服务器都会尝试将其作为CGI脚本执行。 这样，CGI上下文下的文件内容始终受到保护，不能作为静态内容读取。 建议将所有CGI脚本放在一个目录中，并设置CGI上下文来访问它们。', '', '', '');

$_tipsdb['cgiResource'] = new DAttrHelp("CGI Settings", 'The following settings control CGI processes. Memory and process limits also serve as the default for other external applications if limits have not been set explicitly for those applications.', '', '', '');

$_tipsdb['cgi_path'] = new DAttrHelp("路径", '指定CGI脚本的位置。', '', '路径可以是包含一组CGI脚本的目录，例如<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT/myapp/cgi-bin/</span>。 这种情况下，上下文&quot;URI&quot;必须以&quot;/&quot;结尾，例如<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/app1/cgi/</span>。 路径也可以只指定一个CGI脚本，例如<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT/myapp/myscript.pl</span>。 该脚本应有对应的&quot;URI&quot; <span class=&quot;lst-inline-token lst-inline-token--value&quot;>/myapp/myscript.pl</span>。', '');

$_tipsdb['cgidSock'] = new DAttrHelp("CGI守护进程套接字", '用于与CGI守护进程沟通的唯一套接字地址。为了 最佳性能和安全性，LiteSpeed服务器使用一个独立的CGI 守护进程来产生CGI脚本的子进程。 默认套接字是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>uds://$SERVER_ROOT/admin/lscgid/.cgid.sock</span>。 如果你需要放置在另一个位置，在这里指定一​​个Unix域套接字。', '', 'UDS://path', 'UDS://tmp/lshttpd/cgid.sock');

$_tipsdb['cgroups'] = new DAttrHelp("cgroups", '如果当前操作系统支持(目前支持RedHat/Centos Linux v7.5+和Ubuntu 18.04+)，则将cgroup设置应用于此CGI进程。 。 当前执行的用户将用于确定要应用的cgroup配置。<br/>在服务器级别将此设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>将在服务器范围内禁用此设置。 在其他情况下，可以在虚拟主机级别覆盖服务器级别的设置。<br/>默认值:<br/><b>服务器级别:</b> Off<br/><b>虚拟主机级别:</b> 继承服务器级别设置', '', '从下拉列表选择', '');

$_tipsdb['checkSymbolLink'] = new DAttrHelp("检查符号链接", '指定在启用了&quot;跟随符号链接&quot;时，是否检查符号链接在不在&quot;拒绝访问的目录&quot;中。 如果启用检查，将检查网址对应的真正的资源路径是否在配置的禁止访问目录中。 如果在禁止访问目录中，访问将被禁止。', '[性能和安全] 要获得最佳的安全性，启用该选项。要获得最佳性能，禁用该选项。', '布尔值', '');

$_tipsdb['ciphers'] = new DAttrHelp("密码套件", '指定SSL握手时要使用的密码套件。 LSWS支持在SSL v3.0，TLS v1.0，TLS v1.2和TLS v1.3中实现的密码套件。', '[安全建议] 建议将此字段留空，以使用遵循SSL密码最佳实践的默认密码套件。', '以冒号分隔的密码套件规范字符串。', 'ECDHE-RSA-AES128-SHA256:RC4:HIGH:!MD5:!aNULL:!EDH');

$_tipsdb['clientVerify'] = new DAttrHelp("客户端验证", ' 指定客户端证书身份验证的类型。 可用类型包括： <ul> <li><b>None:</b> 不需要客户端证书。</li> <li><b>Optional:</b> 客户端证书可选。</li> <li><b>Require:</b> 客户端必须提供有效证书。</li> <li><b>Optional_no_ca:</b> 与Optional相同。</li> </ul> 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>None</span>。', '建议使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>None</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Require</span>。', '从下拉列表选择', '');

$_tipsdb['compilerflags'] = new DAttrHelp("编译器标志", '添加其他编译器标志，例如优化的编译器选项。', '', '支持的标志有CFLAGS、CXXFLAGS、CPPFLAGS、LDFLAGS。使用空格分隔不同的标志。 标志值请使用单引号（不是双引号）。', 'CFLAGS=&#039;-O3 -msse2 -msse3 -msse4.1 -msse4.2 -msse4 -mavx&#039;');

$_tipsdb['compressibleTypes'] = new DAttrHelp("压缩类型", '指定允许压缩哪些MIME类型。 保留此设置不变，或输入<span class=&quot;lst-inline-token lst-inline-token--value&quot;>default</span>以使用服务器内置的默认列表，该列表已经涵盖了大多数mime类型。<br/>默认值： <span class=&quot;lst-inline-token lst-inline-token--value&quot;>text/*,application/x-javascript,application/javascript,application/xml,image/svg+xml,application/rss+xml, application/json,application/vnd.ms-fontobject,application/x-font,application/x-font-opentype, application/x-font-truetype,application/x-font-ttf,font/eot,font/opentype,font/otf,font/ttf,image/x-icon, image/vnd.microsoft.icon,application/xhtml+xml</span>', '[性能建议] 只允许特定类型进行GZIP压缩。 二进制文件如gif/png/jpeg图片文件及flash文件无法从压缩中获益。', '逗号分隔的MIME类型列表。允许使用通配符“*”和排除符号“!”，例如text/*, !text/js。', '如果要压缩text/*但不压缩text/css，可以使用如下规则： <span class=&quot;lst-inline-token lst-inline-token--value&quot;>text/*, !text/css</span>。“!”会排除该MIME类型。');

$_tipsdb['configFile'] = new DAttrHelp("配置文件", '指定虚拟主机的配置文件名称。 配置文件必须位于$SERVER_ROOT/conf/vhosts/目录下。', '推荐使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$SERVER_ROOT/conf/vhosts/$VH_NAME/vhconf.conf</span>。', '文件路径', '');

$_tipsdb['configureparams'] = new DAttrHelp("配置参数", '配置PHP构建参数。单击下一步时，Apache专用参数和“--prefix”值会自动移除，并会自动追加“--with-litespeed”。 （前缀可以在上方字段中设置。）这样，您可以直接从现有可用PHP构建的phpinfo()输出中复制并粘贴configure参数。', '', '以空格分隔的一组选项（可带或不带双引号）', '');

$_tipsdb['connTimeout'] = new DAttrHelp("连接超时时间(secs)", '指定一个请求允许的最大连接空闲时间。 如果在这段时间内连接一直处于空闲状态(即没有I/O活动），则它将被关闭。', '[安全建议] 将值设置得尽可能低，在可能的拒绝服务攻击中，这可以帮助释放无效连接所占用的连接数。', '非负整数', '');

$_tipsdb['consoleSessionTimeout'] = new DAttrHelp("会话超时时长（秒）", '自定义WebAdmin控制台会话超时时间。 默认值为60秒。', '[安全] 在生产环境中请设置合适的值，通常应低于300秒。', '整数', '');

$_tipsdb['cpuAffinity'] = new DAttrHelp("处理器亲和性", 'CPU关联将进程绑定到一个或多个CPU（核心）。 始终使用同一CPU对进程来说是有益的，因为这样进程可以利用CPU缓存中剩余的数据。 如果进程移至其他CPU，则不会使用CPU缓存，并且会产生不必要的开销。<br/><br/>CPU Affinity设置控制一个服务器进程将与多少个CPU（核心）相关联。 最小值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>，它将禁用此功能。 最大值是服务器具有的核心数。 通常，<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span>是最佳设置，因为它会最严格地使用CPU亲和力，从而最大程度地利用CPU缓存。<br/><br/>默认值：0', '', '0到64之间的整数。（0将禁用此功能）', '');

$_tipsdb['crlFile'] = new DAttrHelp("客户端吊销文件", ' 指定包含PEM编码CA CRL文件的文件，这些文件列出已吊销的客户端证书。 它可以替代&quot;客户端吊销路径&quot;使用，也可以与其配合使用。', '', '文件路径', '');

$_tipsdb['crlPath'] = new DAttrHelp("客户端吊销路径", ' 指定包含已吊销客户端证书的PEM编码CA CRL文件的目录。 此目录中的文件必须为PEM编码。 这些文件通过哈希文件名（hash-value.rN）访问。 有关如何创建哈希文件名，请参考OpenSSL或Apache mod_ssl文档。', '', '路径', '');

$_tipsdb['ctxType'] = new DAttrHelp("上下文类型", '创建的上下文类型决定其用途。 <br/><br/> <b>静态</b>上下文可将URI映射到文档根目录内外的目录。 <br/> <b>LiteSpeed SAPI</b>上下文可将URI关联到LSAPI应用程序。 <br/> <b>代理</b>上下文可让此虚拟主机作为外部Web服务器或应用服务器的透明反向代理服务器。 <br/> <b>Fast CGI</b>上下文是Fast CGI应用程序的挂载点。 <br/> <b>SCGI</b>上下文可将URI关联到SCGI应用程序。 <br/> <b>CGI</b>上下文可指定仅包含CGI脚本的目录。 <br/> <b>重定向</b>上下文可设置内部或外部重定向URI。 <br/> <b>负载均衡</b>上下文可为该上下文分配不同的集群。 <br/> <b>Java Web应用</b>上下文用于从兼容AJPv13的Java Servlet引擎自动导入预定义Java应用程序。 <br/> <b>Servlet</b>上下文用于导入Web应用程序下的特定Servlet。 <br/> <b>App Server</b>上下文专用于Rack/Rails、WSGI和Node.js应用程序。 <br/> <b>uWSGI</b>上下文可将URI关联到uWSGI应用程序。 <br/> <b>模块处理程序</b>上下文是处理程序类型模块的挂载点。', '', '', '');

$_tipsdb['defaultCharsetCustomized'] = new DAttrHelp("自定义默认字符集", '指定当&quot;添加默认的字符集&quot;为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>On</span>时使用的字符集。此项可选，默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>iso-8859-1</span>。当&quot;添加默认的字符集&quot;为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Off</span>时，本设置不生效。', '', '字符集名称。', 'utf-8');

$_tipsdb['defaultType'] = new DAttrHelp("默认MIME类型", '指定后，当无法通过文档后缀确定MIME类型映射或没有后缀时，将使用此类型。如果未指定，将使用默认值<span class=&quot;lst-inline-token lst-inline-token--value&quot;>application/octet-stream</span>。', '', 'MIME类型', '');

$_tipsdb['destinationuri'] = new DAttrHelp("目标URI", '指定重定向的目标位置。 如果被重定向的URI映射到另一个重定向URI时，将再次被重定向。', '', '此URI可以是同一网站上以&quot;/&quot;开头的相对URI，也可以是指向其他网站并以&quot;http(s)://&quot;开头的绝对URI。 如果&quot;URI&quot;包含正则表达式，目标地址可以引用匹配变量，例如$1或$2。', '');

$_tipsdb['disableInitLogRotation'] = new DAttrHelp("禁用初始日志轮换", '指定在启动时是否启用/禁用服务器错误日志文件的轮换。 使用值为“未设置”时，默认启用初始日志轮转。', '', '布尔值', '');

$_tipsdb['docRoot'] = new DAttrHelp("文档根目录", '指定此虚拟主机的文档根目录。 推荐使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT/html</span>。在Context中，此目录可以用$DOC_ROOT来引用。', '', '路径', '');

$_tipsdb['domainName'] = new DAttrHelp("域名", '指定映射域名。域名不区分大小写，并且会移除开头的&quot;www.&quot;。 允许使用通配符&quot;*&quot;和&quot;?&quot;。&quot;?&quot;只代表一个字符， &quot;*&quot;代表任意数量的字符。 不允许重复域名。', '[性能] 如果侦听器专用于一个虚拟主机， 请始终使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>作为域名，以避免不必要的检查。 除catchall域名外，应尽量避免使用带通配符的域名。', '逗号分隔列表。', 'www?.example.com<br/>&quot;*.mydomain.com&quot;将匹配mydomain.com的所有子域。<br/>&quot;*&quot;本身是catchall域名，会匹配任何未匹配的域名。');

$_tipsdb['dynReqPerSec'] = new DAttrHelp("动态请求/秒", '指定每秒可处理的来自单个IP的动态请求的数量（无论与该IP之间建立了多少个连接） 当达到此限制时，所有后来的请求将被延滞到下一秒。<br/>静态内容的请求限制与此限制无关。 可以在服务器或虚拟主机级别设置每个客户端请求的限制。 虚拟主机级别的设置将覆盖服务器级别的设置。', '[安全] 受信任的IP或子网不受影响。', '非负整数', '');

$_tipsdb['enableCoreDump'] = new DAttrHelp("启用核心转储", '指定当服务器由root用户启动时是否启用核心转储。 对于大多数现代Unix系统，出于安全考虑，更改用户ID或组ID的进程不允许生成core文件。不过，core dump文件有助于定位问题根因。 此选项仅适用于Linux内核2.4及更高版本。 Solaris用户应当使用<span class=&quot;lst-inline-token lst-inline-token--command&quot;>coreadm</span>命令来控制这个功能。', '[安全] 仅当你在服务器日志中看到<span class=&quot;lst-inline-token lst-inline-token--value&quot;>no core file created</span>时启用。生成core文件后请立即关闭。创建core dump后请提交bug报告。', '布尔值', '');

$_tipsdb['enableDHE'] = new DAttrHelp("启用DH密钥交换", '允许使用Diffie-Hellman密钥交换进行进一步的SSL加密。', '[安全建议] DH密钥交换比仅使用RSA密钥更安全。 ECDH和DH密钥安全性相同。<br/><br/>[性能] 启用DH密钥交换会增加CPU负载，并且比ECDH密钥交换和RSA都慢。如果可用，建议优先使用ECDH密钥交换。', '布尔值', '');

$_tipsdb['enableDynGzipCompress'] = new DAttrHelp("启用GZIP动态压缩", '控制动态HTTP回应的GZIP压缩。 &quot;启用压缩&quot;必须设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>来开启动态GZIP压缩。<br/>默认值：Yes', '[性能建议] 压缩动态回应将增加CPU和内存的使用，但可以节省网络带宽。', '布尔值', '');

$_tipsdb['enableECDHE'] = new DAttrHelp("启用ECDH密钥交换", '允许使用椭圆曲线Diffie-Hellman密钥交换进行进一步的SSL加密。', '[安全建议] ECDH密钥交换比仅使用RSA密钥更安全。 ECDH和DH密钥交换安全性相同。<br/><br/>[性能] 启用ECDH密钥交换会增加CPU负载，并且比仅使用RSA密钥要慢。', '布尔值', '');

$_tipsdb['enableExpires'] = new DAttrHelp("启用过期", '指定是否为静态文件生成Expires头。如果启用，将根据 &quot;默认过期&quot;和&quot;按类型过期&quot;生成Expires头。<br/><br/>这可以在服务器、虚拟主机和上下文级别设置。低级别的设置将 覆盖高级别的设置。例如，上下文级别的设置将覆盖虚拟主机级别的设置， 虚拟主机级别的设置将覆盖服务器级别的设置。', '', '布尔值', '');

$_tipsdb['enableGzipCompress'] = new DAttrHelp("启用压缩", '为静态和动态响应启用GZIP/Brotli压缩。<br/>默认值：Yes', '[性能建议] 启用后可节省网络带宽。基于文本的响应（如html、css和javascript文件）受益最大，平均可压缩到原始大小的一半。', '从单选框中选择', '');

$_tipsdb['enableIpGeo'] = new DAttrHelp("启用IP地理定位", ' 指定是否启用IP地理定位查找。 可以在服务器、虚拟主机或上下文级别设置。使用“Not Set”值时，默认禁用IP地理定位。', '', '布尔值', '');

$_tipsdb['enableLVE'] = new DAttrHelp("CloudLinux", '指定当CloudLinux存在时是否启用CloudLinux的轻量级虚拟 环境（LVE）。您可以搭配使用LiteSpeed与LVE实现更好的资源管理。 欲了解更多信息，请访问 <a href="http://www.cloudlinux.com" target="_blank" rel="noopener noreferrer">http://www.cloudlinux.com</a>。', '', '从下拉列表选择', '');

$_tipsdb['enableRecaptcha'] = new DAttrHelp("启用CAPTCHA", '在当前级别启用CAPTCHA保护功能。必须先在服务器级别将此设置设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>，才能使用CAPTCHA保护功能。<br/>默认值：<br/><b>服务器级别：</b><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span><br/><b>虚拟主机级别：</b>继承服务器级别设置', '', '布尔值', '');

$_tipsdb['enableRewrite'] = new DAttrHelp("启用重写", '指定是否启用LiteSpeed的URL重写引擎。 此选项可在虚拟主机或Context级别自定义， 并会沿目录树继承，直到被显式覆盖。', '', '布尔值', '');

$_tipsdb['enableScript'] = new DAttrHelp("启用脚本/外部应用程序", '指定在这个虚拟主机中是否允许运行脚本（非静态页面）。 如果禁用，CGI, FastCGI, LSAPI, Servlet引擎 和其他脚本语言都不允许在此虚拟主机中使用。 因此如果你希望使用一个脚本处理程序，你需要启用本项。', '', '布尔值', '');

$_tipsdb['enableSpdy'] = new DAttrHelp("ALPN", '应用层协议协商（ALPN）用于有选择地启用HTTP/3和HTTP/2网络协议。<br/><br/>如果要禁用HTTP/2和HTTP3，请勾选<span class=&quot;lst-inline-token lst-inline-token--value&quot;>None</span>，并取消勾选所有其他选项。<br/><br/>默认值：全部启用', '可在侦听器和虚拟主机级别设置。', '从复选框选择', '');

$_tipsdb['enableStapling'] = new DAttrHelp("启用 OCSP 装订", '确定是否启用OCSP装订，这是一种更有效的验证公钥证书的方式。', '', '布尔值', '');

$_tipsdb['enableh2c'] = new DAttrHelp("启用 明文TCP上的HTTP/2", '指定是否通过非加密TCP连接启用HTTP/2。 默认设置为禁用。', '', '布尔值', '');

$_tipsdb['env'] = new DAttrHelp("环境", '为外部应用程序指定额外的环境变量。', '', 'Key=value。多个变量可以用回车换行分隔。', '');

$_tipsdb['errCode'] = new DAttrHelp("错误代码", '指定错误页面的HTTP状态码。 只有特定的HTTP状态码才可以自定义错误页面。', '', '从下拉列表选择', '');

$_tipsdb['errPage'] = new DAttrHelp("自定义错误页面", '当服务器在处理请求时遇到问题， 服务器将向网络客户端返回错误代码和html页面作为错误消息。 错误代码在HTTP协议中定义（请参阅RFC 2616）。 LiteSpeed Web服务器为每个错误代码都有一个内置的默认错误页面，但是也可以为每个错误代码配置一个自定义页面。 这些错误页面还可以进一步自定义，使其对每个虚拟主机都保持唯一。', '', '', '');

$_tipsdb['errURL'] = new DAttrHelp("URL", '指定自定义错误页的URL。 当返回相应HTTP状态时服务器会将请求转发到该URL。 如果此URL指向不存在的资源，将使用内置错误页面。 该URL可以是静态文件、动态生成的页面，或其他网站的页面 （以&quot;http(s)://&quot;开头的URL）。 当引用其他网站上的页面时，客户端会收到重定向状态码， 而不是原始状态码。', '', 'URL', '');

$_tipsdb['expWSAddress'] = new DAttrHelp("地址", '外部网络服务器使用的HTTP,HTTPS或Unix域套接字(UDS)地址。', '[安全建议] 如果代理到同一台机器上运行的另一台Web服务器，请将IP地址设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>localhost</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>127.0.0.1</span>，这样从其他机器上就无法访问外部应用程序。', 'IPv4或IPv6地址（:端口）、UDS://path或unix:path。如果外部Web服务器使用HTTPS，则在前面加上“https://”。 如果外部Web服务器使用标准端口80或443，则端口是可选的。', '192.168.0.10<br/>127.0.0.1:5434<br/>https://10.0.8.9<br/>https://127.0.0.1:5438<br/>UDS://tmp/lshttpd/php.sock<br/>unix:/tmp/lshttpd/php.sock');

$_tipsdb['expiresByType'] = new DAttrHelp("按类型过期", '为各个MIME类型分别指定Expires头设置。', '', '逗号分隔的“MIME-类型=A|M秒数”的列表。 文件将在基准时间（A|M）加指定秒数的时间后失效。<br/><br/>“A”代表基准时间为客户端的访问时间，“M”代表文件的最后修改时间。 MIME-类型可使用通配符“*”，如image/*。', '');

$_tipsdb['expiresDefault'] = new DAttrHelp("默认过期", '指定生成Expires头的默认设置。该设置在&quot;启用过期&quot; 设为“启用”时有效。它可以被&quot;按类型过期&quot;覆盖。 除非必要，否则不要在服务器或虚拟主机级别设置该默认值。 因为它会为所有网页生成Expires头。大多数时候，应该是 为不常变动的某些目录在上下文级别设置。如果没有默认设置，&quot;按类型过期&quot;中未指定的类型不会生成Expires头。', '', 'A|Mseconds<br/>文件将在基准时间（A|M）加指定秒数的时间后失效。 “A”代表基准时间为客户端的访问时间，“M”代表文件的最后修改时间。', '');

$_tipsdb['expuri'] = new DAttrHelp("URI", '指定此上下文的URI。', '', 'URI可以是普通URI（以&quot;/&quot;开头），也可以是Perl兼容的正则表达式URI（以&quot;exp:&quot;开头）。 如果普通URI以&quot;/&quot;结尾，则此上下文将包含该URI下的所有子URI。 如果上下文映射到文件系统上的目录，则必须添加结尾的&quot;/&quot;。', '');

$_tipsdb['extAppAddress'] = new DAttrHelp("地址", '外部应用程序使用的唯一套接字地址。 支持IPv4/IPv6套接字和Unix域套接字（UDS）。 IPv4/IPv6套接字可用于网络通信。 只有当外部应用程序与服务器在同一台机器上时，才能使用UDS。', '[安全] 如果外部应用程序在同一台机器上运行，则首选UDS。如果必须使用IPv4/IPv6， 将IP地址设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>localhost</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>127.0.0.1</span>，这样外部应用就无法从其他机器上访问。 [性能建议] Unix域套接字一般比IPv4套接字拥有更高的性能。', 'IPv4或IPv6地址:端口，或者UDS://path', '127.0.0.1:5434<br/>UDS://tmp/lshttpd/php.sock.');

$_tipsdb['extAppName'] = new DAttrHelp("名称", '此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。', '', '文本', '');

$_tipsdb['extAppPath'] = new DAttrHelp("命令", '指定执行外部应用程序的完整命令行，包括参数。 启用&quot;由服务器启动&quot;时，此项为必填。如果参数中包含空格或制表符，应使用双引号或单引号括起该参数。', '', '可执行文件的完整路径，可带可选参数。', '');

$_tipsdb['extAppPriority'] = new DAttrHelp("优先级", '指定外部应用的优先级，值的范围是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-20</span>到<span class=&quot;lst-inline-token lst-inline-token--value&quot;>20</span>。 数值越小表示优先级越高。外部应用进程的优先级不能高于Web服务器。 如果该优先级设置一个比服务器的优先级小的数字，则将使用服务器的优先级。', '', '整数', '');

$_tipsdb['extAppType'] = new DAttrHelp("类型", '指定外部应用程序的类型。应用程序类型根据它们提供的服务或与服务器通信所使用的协议区分。可选类型包括： <ul>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>LiteSpeed SAPI App</span>: 使用LSAPI协议与Web服务器通信的应用程序。</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Web Server (Proxy)</span>: 支持HTTP协议的Web服务器或应用服务器。</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>FastCGI App</span>: 具有<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Responder</span>角色的FastCGI应用程序。</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>FastCGI Authorizer</span>: 具有<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Authorizer</span>角色的FastCGI应用程序。</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>SCGI App</span>: 使用SCGI协议与Web服务器通信的应用程序。</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Servlet Engine</span>: 带AJPv13连接器的Servlet引擎，例如Tomcat。</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Piped Logger</span>: 可处理从STDIN流接收的访问日志条目的应用程序。</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Load Balancer</span>: 可在worker应用程序之间进行负载均衡的虚拟应用程序。</li>   <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>uWSGI</span>: 使用uWSGI协议与Web服务器通信的应用程序。</li> </ul>', '大多数应用程序会使用LSAPI或FastCGI协议。LSAPI支持PHP、Ruby和Python。Perl可与FastCGI一起使用。（PHP、Ruby和Python也可以设置为使用FastCGI运行，但使用LSAPI运行更快。）Java使用Servlet引擎。', '从下拉列表选择', '');

$_tipsdb['extAuthorizer'] = new DAttrHelp("授权器", '指定可用于生成授权/未授权决策的外部应用程序。目前仅FastCGI授权器可用。有关FastCGI授权器角色的更多详细信息， 请访问<a href="https://fastcgi-archives.github.io/" target="_blank" rel="noopener noreferrer">https://fastcgi-archives.github.io/</a>。', '', '从下拉列表选择', '');

$_tipsdb['extGroup'] = new DAttrHelp("以特定组运行", '外部应用程序将以此处指定的组名运行。如果未设置，将继承虚拟主机级别的设置。<br/><br/>默认值：未设置', '', '有效的组名。', '');

$_tipsdb['extMaxIdleTime'] = new DAttrHelp("最大空闲时间", '指定服务器停止外部应用之前的最大空闲时间，以释放空闲资源。 当设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-1</span>时，服务器不会停止外部应用，除非它在ProcessGroup模式下运行； 在ProcessGroup模式下，空闲的外部应用会在30秒后停止。<br/><br/>默认值为: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>-1</span>', '[性能] 此功能在大规模托管环境中特别有用。为了避免一个虚拟主机拥有的文件被另一个虚拟主机的外部应用脚本访问，通常会同时以SetUID模式运行许多不同的应用。将该值设置得较低，可以避免这些外部应用不必要地长时间空闲。', '整数', '');

$_tipsdb['extUmask'] = new DAttrHelp("umask", '设置此外部应用程序进程的默认umask。详情请参阅<span class=&quot;lst-inline-token lst-inline-token--command&quot;> man 2 umask </span>。默认值取自服务器级别的&quot;umask&quot;设置。', '', '有效范围为[000]-[777]的值。', '');

$_tipsdb['extUser'] = new DAttrHelp("运行用户", '外部应用程序将以此处指定的用户名运行。如果未设置，将继承虚拟主机级别的设置。<br/><br/>默认值：未设置', '', '有效的用户名。', '');

$_tipsdb['extWorkers'] = new DAttrHelp("Worker", '之前在外部负载均衡器中定义的worker组列表。', '', 'ExternalAppType::ExternalAppName格式的逗号分隔列表。', 'fcgi::localPHP, proxy::backend1');

$_tipsdb['externalredirect'] = new DAttrHelp("外部重定向", '指定此重定向是否为外部重定向。 对于外部重定向，可以指定&quot;状态码&quot;，并且&quot;目标URI&quot;可以以&quot;/&quot;或&quot;http(s)://&quot;开头。 对于内部重定向，&quot;目标URI&quot;必须以&quot;/&quot;开头。', '', '', '');

$_tipsdb['extraHeaders'] = new DAttrHelp("标头控制", '指定要添加的附加响应/请求头。可添加多个头指令，每行一个。&quot;NONE&quot;可用于禁用父级头继承。如果未提供指令，则假定为&quot;Header&quot;。', '[信息] 语法和用法类似于 <a href="https://httpd.apache.org/docs/2.2/mod/mod_headers.html#header" target="_blank" rel="noopener noreferrer">Apache的mod_headers指令</a><br/><br/>[信息] &#039;Header&#039;指令是可选的。从其他位置复制规则时，可以移除或保留它，不会影响使用。', '[Header]|RequestHeader [condition] set|append|merge|add|unset header [value] [early|env=[!]variable]', 'set Cache-control no-cache<br/>append Cache-control no-store<br/>Header set My-header cust_header_val<br/>RequestHeader set My-req-header cust_req_header_val');

$_tipsdb['extrapathenv'] = new DAttrHelp("额外的PATH环境变量", '将其他用于构建脚本的路径附加到当前PATH环境变量中。', '', '多个路径以“:”分隔', '');

$_tipsdb['fcgiContext'] = new DAttrHelp("FastCGI上下文", 'FastCGI应用程序不能直接使用。必须将FastCGI应用程序配置为脚本处理程序，或通过FastCGI上下文映射到URL。 FastCGI上下文会将URI关联到FastCGI应用程序。', '', '', '');

$_tipsdb['fcgiapp'] = new DAttrHelp("FastCGI 应用程序", '指定FastCGI应用程序的名称。 必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义此应用程序。', '', '从下拉列表选择', '');

$_tipsdb['fileETag'] = new DAttrHelp("文件ETag", '指定是否使用一个文件的索引节点、最后修改时间和大小属性 生成静态文件的ETag HTTP响应头。 所有这三个属性是默认启用的。 如果您打算使用镜像服务器服务相同的文件，您应该不勾选索引节点。 否则，为同一个文件生成的ETag在不同的服务器上是不同的。', '', '从复选框选择', '');

$_tipsdb['fileUpload'] = new DAttrHelp("文件上传", '上传文件时，通过使用请求正文解析器将文件解析到服务器本地目录，可以提供附加的安全功能。第三方模块可以轻松扫描本地目录中的文件是否存在危害。当启用&quot;通过文件路径传递上传数据&quot;，或模块在LSI_HKPT_HTTP_BEGIN级别调用LSIAPI的set_parse_req_body时，将使用请求正文解析器。 源代码包中提供的API示例。', '', '', '');

$_tipsdb['followSymbolLink'] = new DAttrHelp("跟随符号链接", '指定服务静态文件时跟踪符号链接的服务器级别默认设置。<br/>选项有<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>If Owner Match</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>。<br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>设置服务器始终跟踪符号链接。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>If Owner Match</span>设置服务器只有在链接和目标属主一致时才跟踪符号链接。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>表示服务器永远不会跟踪符号链接。 该设置可以在虚拟主机配置中覆盖，但不能通过.htaccess文件覆盖。', '[性能和安全建议] 要获得最佳安全性，选择<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>If Owner Match</span>。 要获得最佳性能，选择<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>。', '从下拉列表选择', '');

$_tipsdb['forceGID'] = new DAttrHelp("强制GID", '指定一组ID，以用于所有在suEXEC模式下启动的外部应用程序。 当设置为非零值时，所有suEXEC的外部应用程序（CGI、FastCGI、 LSAPI）都将使用该组ID。这可以用来防止外部应用程序访问其他用 户拥有的文件。<br/>例如，在共享主机环境，LiteSpeed以“www-data”用户、“www-data”组 身份运行。每个文件根目录是由用户帐户所有，属组为“www-data”，权限 为0750。如果强制GID被设置为“nogroup”（或“www-data”之外的任何一 个组），所有suEXEC外部应用程序都将以特定用户身份运行，但属组为 “nogroup”。这些外部应用程序的进程依然能够访问属于相应用户的文件（ 因为他们的用户ID），但没有组权限访问其他人的文件。另一方面，服务器 仍然可以服务在任何用户文件根目录下的文件（因为它的组ID）。', '[安全建议] 设置足够高的值以排除所有系统用户所在的组。', '非负整数', '');

$_tipsdb['forceStrictOwnership'] = new DAttrHelp("强制严格属主检查", '指定是否执行严格的文件所有权检查。 如果启用，Web服务器将检查正在服务的文件的所有者与虚拟主机的所有者是否相同。 如果不同，将返回403拒绝访问错误。 该功能默认是关闭的。', '[安全建议] 对于共享主机，启用此检查以得到更好的安全性。', '布尔值', '');

$_tipsdb['forceType'] = new DAttrHelp("强制MIME类型", '指定后，无论文件后缀如何， 此上下文下的所有文件都将作为具有指定MIME类型的静态文件提供。 设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>时，将禁用强制MIME类型。', '', 'MIME类型或NONE。', '');

$_tipsdb['gdb_groupname'] = new DAttrHelp("组名", '组名，仅包含字母和数字（无特殊字符）。', '', 'string', '');

$_tipsdb['gdb_users'] = new DAttrHelp("用户", '属于该组的用户列表，以空格分隔。', '', '', '');

$_tipsdb['generalContext'] = new DAttrHelp("静态上下文", '上下文设置用于为特定位置的文件指定特殊设置。 这些设置可用于引入文档根目录外的文件（类似Apache的Alias或AliasMatch指令）， 使用授权域保护特定目录，或阻止/限制对文档根目录中特定目录的访问。', '', '', '');

$_tipsdb['geoipDBFile'] = new DAttrHelp("数据库文件路径", '指定MaxMind GeoIP数据库路径。', '', '路径', '/usr/local/share/GeoIP/GeoLite2-Country.mmdb');

$_tipsdb['geoipDBName'] = new DAttrHelp("数据库名称", 'MaxMind GeoIP数据库名称。从GeoIP2开始，此设置是必需的。<br/><br/>从GeoIP升级到GeoIP2时，将此设置设为&quot;COUNTRY_DB&quot;、&quot;CITY_DB&quot;或&quot;ASN_DB&quot;，会自动使用一些与GeoIP兼容的条目（在下面的数据库名称值中列出）填充PHP的$_SERVER变量，以简化迁移。<br/><b>CITY_DB:</b> &#039;GEOIP_COUNTRY_CODE&#039;, &#039;GEOIP_COUNTRY_NAME&#039;, &#039;GEOIP_CONTINENT_CODE&#039;, &#039;GEOIP_COUNTRY_CONTINENT&#039;, &#039;GEOIP_DMA_CODE&#039;, &#039;GEOIP_METRO_CODE&#039;, &#039;GEOIP_LATITUDE&#039;, &#039;GEOIP_LONGITUDE&#039;, &#039;GEOIP_POSTAL_CODE&#039;和&#039;GEOIP_CITY&#039;。<br/><b>COUNTRY_DB:</b> &#039;GEOIP_COUNTRY_CODE&#039;, &#039;GEOIP_COUNTRY_NAME&#039;, &#039;GEOIP_CONTINENT_CODE&#039;, 和&#039;GEOIP_COUNTRY_CONTINENT&#039;。<br/><b>ASN_DB:</b> &#039;GEOIP_ORGANIZATION&#039;和&#039;GEOIP_ISP&#039;。', '', '文本', 'COUNTRY_DB');

$_tipsdb['geolocationDB'] = new DAttrHelp("IP地理定位数据库", '多个MaxMind地理定位数据库可以在这里指定。MaxMind有以下数据库类型: 国家，地区，城市，组织，ISP和NETSPEED。如果混合配置“国家”，“地区”，和“城市”类型数据库，则最后一项将会生效。', '', '', '');

$_tipsdb['gracePeriod'] = new DAttrHelp("宽限期（秒）", '指定来自一个IP的连接数超过&quot;连接软限制&quot;之后， 多长时间之内可以继续接受新连接。在此期间，如果总连接数仍然 低于&quot;连接硬限制&quot;，将继续接受新连接。之后，如果连接数 仍然高于&quot;连接软限制&quot;，相应的IP将被封锁&quot;禁止期（秒）&quot;里设置的时长。', '[性能与安全建议] 设置为足够大的数量，以便下载完整网页， 但也要足够低以防范蓄意攻击。', '整数', '');

$_tipsdb['gracefulRestartTimeout'] = new DAttrHelp("平滑重启超时时长(secs)", '平滑重启时，即使新的服务器实例已经启动，旧的实例仍将继续 处理现有的请求。此项超时设置定义了旧实例等待多长时间后中止。 默认值是300秒。 -1表示永远等待。 0表示不等待，立即中止。', '', '整数', '');

$_tipsdb['groupDBCacheTimeout'] = new DAttrHelp("组数据库缓存超时（秒）", '指定多久检查一次后端组数据库变更。 更多详细信息请参阅&quot;用户数据库缓存超时（秒）&quot;。', '', '非负整数', '');

$_tipsdb['groupDBMaxCacheSize'] = new DAttrHelp("组数据库最大缓存大小", '指定组数据库的最大缓存大小。', '[性能建议] 由于更大的缓存会消耗更多的内存， 更高的值可能会也可能不会提供更好的性能。 请根据你的用户数据库大小和网站使用情况来设置合适的大小。', '非负整数', '');

$_tipsdb['gzipAutoUpdateStatic'] = new DAttrHelp("自动更新静态文件", '指定是否由LiteSpeed自动创建/更新可压缩静态文件的GZIP压缩版本。 如果设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>，当请求文件MIME属于&quot;压缩类型&quot;时， LiteSpeed会根据压缩的文件时间戳来创建/更新文件的压缩版本。 压缩的文档会创建在&quot;静态缓存目录&quot;目录下。 文件名称根据原文件的MD5散列创建。<br/>默认值: Yes', '', '布尔值', '');

$_tipsdb['gzipCacheDir'] = new DAttrHelp("静态缓存目录", '指定目录路径来存储静态内容的压缩文件。默认是&quot;交换目录&quot;。', '', '目录路径', '');

$_tipsdb['gzipCompressLevel'] = new DAttrHelp("GZIP压缩级别（动态内容）", '指定压缩动态内容的级别。 范围从<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span>(最低)到<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span>(最高)。<br/>该设置仅在&quot;启用压缩&quot;和&quot;启用GZIP动态压缩&quot;启用时生效。<br/>默认值: 6', '[性能建议] 更高的压缩级别将消耗更多的内存和CPU资源。 如果您的机器有额外的资源您可以设置更高的级别。 级别9与级别6没有太大的区别，但是级别9会占用多得多的CPU资源。', '1到9之间的数字。', '');

$_tipsdb['gzipMaxFileSize'] = new DAttrHelp("静态文件最大尺寸(bytes)", '指定LiteSpeed可以自动创建压缩文件的静态文件最大尺寸。<br/>默认值为 10M', '[性能建议] 不建议使用LiteSpeed创建/更新较大文件的压缩文件。 压缩操作会占用整个服务器进程并且在压缩结束前新请求都无法被处理。', '不小于1K的字节数。', '');

$_tipsdb['gzipMinFileSize'] = new DAttrHelp("静态文件最小尺寸 (bytes)", '指定LiteSpeed创建相应压缩文件的静态文件最小尺寸。<br/>默认值：200', '非常小的文件节省的带宽可以忽略，通常无需压缩。', '不小于200的字节数。', '');

$_tipsdb['gzipStaticCompressLevel'] = new DAttrHelp("GZIP压缩级别（静态内容）", '指定GZIP压缩静态内容的级别。 范围从<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span> (最低)到<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span> (最高)。。<br/>该选项仅在 &quot;启用压缩&quot;和&quot;自动更新静态文件&quot; 启用后才会生效<br/>默认值是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>6</span>', '', '1到9之间的数字。', '');

$_tipsdb['hardLimit'] = new DAttrHelp("连接硬限制", '指定来自单个IP的并发连接的硬限制。 此限制是永远执行的，客户端将永远无法超过这个限制。 HTTP/1.0客户端通常会尝试建立尽可能多的连接，因为它们需要同时下载嵌入的内容。此限制应设置得足够高，以使HTTP/1.0客户端仍然可以访问相应的网站。 使用&quot;连接软限制&quot;设置期望的连接限制。<br/>建议根据你的网页内容和流量负载，限制在<span class=&quot;lst-inline-token lst-inline-token--value&quot;>20</span>与<span class=&quot;lst-inline-token lst-inline-token--value&quot;>50</span>之间。', '[安全] 一个较低的数字将使得服务器可以服务更多独立的客户。<br/>[安全] 受信任的IP或子网不受影响。<br/>[性能] 使用大量并发客户端进行基准测试时，设置一个较高的值。', '整数', '');

$_tipsdb['httpdWorkers'] = new DAttrHelp("Worker数量", '指定httpd worker的数量。', '[性能] 设置适当的数量以满足您的需求。增加更多worker不一定意味着性能更好。', '1到16之间的整数。', '');

$_tipsdb['inBandwidth'] = new DAttrHelp("入口带宽 (bytes/sec)", '指定对单个IP地址允许的最大传入吞吐量（无论与该IP之间建立了多少个连接）。 为提高效率，真正的带宽可能最终会略高于设定值。 带宽是按1KB单位分配。设定值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>可禁用限制。 每个客户端的带宽限制（字节/秒）可以在服务器或虚拟主机级别设置。 虚拟主机级别的设置将覆盖服务器级别的设置。', '[安全] 受信任的IP或子网不受影响。', '非负整数', '');

$_tipsdb['inMemBufSize'] = new DAttrHelp("内存读写缓冲区大小", '指定用于存储请求内容和相应的动态响应的最大缓冲区大小。达到此限制时， 服务器将在&quot;交换目录&quot;中创建临时交换文件。', '[性能] 设置足够大的缓冲区，以容纳所有并发 请求/响应，避免内存和磁盘数据交换。如果交换目录（默认为/tmp/lshttpd/swap/）存在频繁的读写活动，说明缓冲区太小，LiteSpeed正在使用交换文件。', '非负整数', '');

$_tipsdb['indexFiles'] = new DAttrHelp("索引文件", '指定URL映射到目录时按顺序查找的索引文件名称。 您可以在服务器、虚拟主机和上下文级别对其进行自定义。', '[性能建议] 只设置你需要的索引文件。', '以逗号分隔的索引文件名列表。', '');

$_tipsdb['indexUseServer'] = new DAttrHelp("使用服务器索引文件", '指定是否使用服务器的索引文件。 如果设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>，那么只有服务器的设置将被使用。 如果设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>，那么服务器的设置将不会被使用。 如果设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Addition</span>，那么附加的索引文件可以被添加到此虚拟主机服务器的索引文件列表中。 如果想要禁用此虚拟主机的索引文件，您可以将该值设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>，并将索引文件栏留空。', '', '从下拉列表选择', '');

$_tipsdb['initTimeout'] = new DAttrHelp("初次请求超时时间 (secs)", '指定服务器等待外部应用响应新建立连接上的第一个请求的最大时间（秒）。 如果服务器在这个限制时间内没有收到外部应用的任何数据，它将把这个连接标记为坏(Bad)。这有助于识别与外部应用程序的通信问题。 这有助于尽快发现与外部应用的通信问题。如果某些请求的处理时间较长，则增加这个限制以避免503错误信息。', '', '秒数整数', '');

$_tipsdb['installpathprefix'] = new DAttrHelp("安装路径前缀", '设置“--prefix”配置选项的值。默认安装位置在LiteSpeed Web Server的安装目录下。', 'LiteSpeed Web Server可以同时使用多个PHP版本。如果要安装多个版本， 应为它们设置不同的前缀。', '路径', '/usr/local/lsws/lsphp5');

$_tipsdb['instances'] = new DAttrHelp("实例数", '指定服务器创建的外部应用程序最大实例数。启用&quot;由服务器启动&quot;时，此项为必填。 大多数FastCGI/LSAPI应用程序每个进程实例只能处理一个请求。对于这类应用，实例数应与&quot;最大连接数&quot;的值匹配。 有些FastCGI/LSAPI应用程序可以生成多个子进程来同时处理多个请求。对于这类应用，应将实例数设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span>，并使用环境变量控制应用可生成的子进程数量。', '', '非负整数', '');

$_tipsdb['internalmodule'] = new DAttrHelp("内部模块", '指定模块是否为内部模块（静态链接），而不是动态链接库。', '', '布尔值', '');

$_tipsdb['ip2locDBCache'] = new DAttrHelp("数据库缓存类型", '使用的缓存方法。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Memory</span>。', '', '从下拉列表选择', '');

$_tipsdb['ip2locDBFile'] = new DAttrHelp("IP2Location数据库文件路径", '有效数据库文件的位置。', '', '文件路径', '');

$_tipsdb['javaServletEngine'] = new DAttrHelp("Servlet Engine", '指定为该Web应用程序提供服务的Servlet Engine的名称。 Servlet引擎必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义。', '', '从下拉列表选择', '');

$_tipsdb['javaWebAppContext'] = new DAttrHelp("Java Web应用上下文", '许多运行Java应用程序的用户也使用Servlet引擎提供静态内容。 但在这些处理上，Servlet引擎远不如LiteSpeed Web Server高效。 为了提升整体性能，可以将LiteSpeed Web Server配置为网关服务器，由它提供静态内容并将动态Java页面请求转发到Servlet引擎。<br/><br/>LiteSpeed Web Server需要定义特定上下文才能运行Java应用程序。 Java Web应用上下文会根据Java Web应用程序的配置文件（WEB-INF/web.xml）自动创建所有必需的上下文。<br/><br/>设置Java Web应用上下文时需要注意：<br/><ul> <li>设置Java Web应用上下文之前，必须先在&quot;外部应用&quot;中设置<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Servlet Engine</span>外部应用程序。</li> <li>还应为.jsp文件定义&quot;脚本处理程序&quot;。</li> <li>如果Web应用程序打包为.war文件，必须先展开该.war文件。服务器无法访问压缩归档文件。</li> <li>访问同一资源时，无论通过LiteSpeed Web Server还是Servlet引擎内置HTTP服务器，都应使用相同URL。<br/>例如，Tomcat 4.1安装在/opt/tomcat下，&quot;examples&quot; Web应用程序文件位于/opt/tomcat/webapps/examples/。 通过Tomcat内置HTTP服务器访问该应用时使用类似&quot;/examples/***&quot;的URI。 因此对应的Java Web应用上下文应配置为： &quot;URI&quot; = <span class=&quot;lst-inline-token lst-inline-token--value&quot;>/examples/</span>, &quot;位置&quot; = <span class=&quot;lst-inline-token lst-inline-token--value&quot;>/opt/tomcat/webapps/examples/</span>。</li>   </ul>', '', '', '');

$_tipsdb['javaWebApp_location'] = new DAttrHelp("位置", '指定包含此Web应用程序文件的目录。该目录应包含&quot;WEB-INF/web.xml&quot;。<br/><br/>默认值：$DOC_ROOT + &quot;URI&quot;', '', '路径', '');

$_tipsdb['jsonReports'] = new DAttrHelp("输出JSON报告", '将带有.json扩展名的其他JSON格式报告文件输出到/tmp/lshttpd目录。<br/><br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>', '[信息] 这对应用程序开发者很有用，他们可能希望使用大多数编程语言内置的标准JSON 处理工具，将LiteSpeed状态和实时报告集成到自己的应用程序中。', '从单选框中选择', '设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>时，除常规的.status、.rtreport、.rtreport.2等报告文件外， 还会创建.status.json、.rtreport.json、.rtreport.2.json等报告文件。');

$_tipsdb['keepAliveTimeout'] = new DAttrHelp("持续连接超时时长", '指定持续连接请求的最长闲置时间。 如果在这段时间内没有接收到新的请求，该连接将被关闭。 这个设置只适用于HTTP/1.1连接。HTTP/2连接有较长的闲置时间，不受此设置影响。', '[安全和性能建议] 我们建议您将值设置得刚好足够处理单个页面 视图的所有请求。没有必要延长持续连接时间。较小的值可以减少闲置 连接，提高能力，以服务更多的用户，并防范拒绝服务攻击。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>2-5</span>秒 对大多数应用是合理范围。LiteSpeed在非持续连接环境是非常高效的。', '非负整数', '');

$_tipsdb['keyFile'] = new DAttrHelp("私钥文件", 'SSL私钥文件的文件名。密钥文件不应加密。', '[安全建议] 私钥文件应放在安全目录中，并且只允许服务器运行用户读取。', '文件路径', '');

$_tipsdb['lbContext'] = new DAttrHelp("负载均衡上下文", '与其他外部应用程序一样，负载均衡工作进程应用程序不能直接使用。 必须通过上下文将它们映射到URL。 负载均衡上下文会将URI关联到由负载均衡工作进程进行负载均衡的目标。', '', '', '');

$_tipsdb['lbapp'] = new DAttrHelp("负载均衡", '指定要与此上下文关联的负载均衡器。 此负载均衡是一个虚拟应用程序，必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义。', '', '从下拉列表选择', '');

$_tipsdb['listenerBinding'] = new DAttrHelp("绑定", '指定侦听器分配给哪个lshttpd子进程。 通过手动将侦听器与进程关联，可以使用不同的子进程处理发往不同侦听器的请求。默认情况下，侦听器会分配给所有子进程。', '', '从复选框选择', '');

$_tipsdb['listenerIP'] = new DAttrHelp("IP地址", '指定此侦听器的IP地址。所有可用IP地址都会列出。IPv6地址会包含在&quot;[ ]&quot;中。<br/><br/>要侦听所有IPv4地址，请选择<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ANY</span>。要侦听所有IPv4和IPv6地址，请选择 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>[ANY]</span>.<br/><br/>为了同时为IPv4和IPv6客户端提供服务，应使用IPv4映射的IPv6地址代替纯IPv4地址。  IPv4映射的IPv6地址应写为[::FFFF:x.x.x.x]。', '[安全建议] 如果您的计算机在不同的子网中具有多个IP, 您可以选择特定的IP以仅允许来自相应子网的流量。', '从下拉列表选择', '');

$_tipsdb['listenerModules'] = new DAttrHelp("侦听器模块", '侦听器模块配置数据默认从服务器模块配置继承。 侦听器模块仅限于TCP/IP第4层Hook。', '', '', '');

$_tipsdb['listenerName'] = new DAttrHelp("侦听器名称", '此侦听器的唯一名称。', '', '文本', '');

$_tipsdb['listenerPort'] = new DAttrHelp("端口", '指定侦听器的TCP端口。只有超级用户（root）可以使用低于<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1024</span>的端口。端口<span class=&quot;lst-inline-token lst-inline-token--value&quot;>80</span>是默认HTTP端口，端口<span class=&quot;lst-inline-token lst-inline-token--value&quot;>443</span>是默认HTTPS端口。', '', '整数', '');

$_tipsdb['listenerSecure'] = new DAttrHelp("安全", '指定这是否是安全（SSL）侦听器。 对于安全的侦听器，需要正确设置其他SSL设置。', '', '布尔值', '');

$_tipsdb['lmap'] = new DAttrHelp("虚拟主机映射", '显示特定侦听器当前建立到虚拟主机的映射。 虚拟主机名称显示在方括号中，后面是此侦听器匹配的域名。', '如果虚拟主机未成功加载（虚拟主机配置中存在致命错误）， 则不会显示到该虚拟主机的映射。', '', '');

$_tipsdb['lname'] = new DAttrHelp("名称 - 侦听器", '用于识别此侦听器的唯一名称。 这是设置侦听器时指定的&quot;侦听器名称&quot;。', '', '', '');

$_tipsdb['location'] = new DAttrHelp("位置", '指定此上下文在文件系统中的对应位置。<br/><br/>默认值：$DOC_ROOT + &quot;URI&quot;', '', '可以是绝对路径，也可以是相对于$SERVER_ROOT、$VH_ROOT或$DOC_ROOT的路径。 $DOC_ROOT是默认相对路径，可以省略。<br/><br/>如果&quot;URI&quot;是正则表达式，匹配的子字符串可用于组成&quot;Root&quot;字符串。匹配的子字符串可通过&quot;$1&quot;到&quot;$9&quot;引用；&quot;$0&quot;和&quot;&&quot;可引用整个匹配字符串。此外，可以追加&quot;?&quot;和查询字符串来设置查询字符串。注意：查询字符串中的&quot;&&quot;应转义为&quot;\&&quot;。', '将&quot;位置&quot;设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/home/john/web_examples</span>时，类似<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/examples/</span>的普通URI会将请求&quot;/examples/foo/bar.html&quot;映射到文件&quot;/home/john/web_examples/foo/bar.html&quot;。<br/>要模拟Apache的mod_userdir，可将URI设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>exp: ^/~([A-Za-z0-9]+)(.*)</span>，并将&quot;位置&quot;设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/home/$1/public_html$2</span>。这样，URI /~john/foo/bar.html的请求会映射到文件/home/john/public_html/foo/bar.html。');

$_tipsdb['logUseServer'] = new DAttrHelp("使用服务器日志", '指定是否将虚拟主机的日志信息放置到服务器日志文件中，而不是创建独自的日志文件。', '', '从单选框中选择', '');

$_tipsdb['log_compressArchive'] = new DAttrHelp("压缩存档", '指定是否压缩轮转日志文件以节省磁盘空间。', '日志文件压缩率很高，建议启用此功能以减少旧日志的磁盘占用。', '布尔值', '');

$_tipsdb['log_debugLevel'] = new DAttrHelp("调试级别", '指定调试日志级别。 要使用此功能，&quot;日志级别&quot;必须设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>。 当“调试级别”设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>时，即使&quot;日志级别&quot; 设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>，调试日志也会被禁用。 &quot;切换调试日志&quot;可用于在运行中的服务器上 控制调试级别而无需重启。', '[性能] 重要！如果你不需要详细的调试日志记录， 应始终将其设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>。启用调试日志记录将严重降低服务性能 ，且可能在很短时间内耗尽磁盘空间。 调试日志记录包括每个请求和响应的详细信息。<br/><br/>我们推荐将日志级别设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>，调试级别设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>。 这些设置意味着调试日志不会塞满磁盘， 但仍可使用&quot;切换调试日志&quot; 控制调试输出。此操作可以实时启用或关闭调试记录， 对于调试繁忙的生产服务器非常有用。', '从下拉列表选择', '');

$_tipsdb['log_enableStderrLog'] = new DAttrHelp("启用标准错误日志（stderr）", '指定接收到服务器启动的进程输出的stderr时，是否写入日志。 如果启用，stderr消息会记录到服务器日志所在目录中固定名为&quot;stderr.log&quot;的文件。如果禁用，所有stderr输出都会被丢弃。<br/><br/>PHP的error_log()等不会直接写入stderr（文件句柄2）的函数不受此设置影响。它们会写入PHP ini指令&#039;error_log&#039;设置的文件；如果未设置该指令，则写入服务器的&quot;error.log&quot;文件，并带有&quot;[STDERR]&quot;标签。', '如果您需要调试配置的外部应用程序，如PHP、Ruby、Java、Python、Perl，请开启该功能。', '布尔值', '');

$_tipsdb['log_fileName'] = new DAttrHelp("文件名", '指定日志文件的路径。', '[性能建议] 将日志文件放置在一个单独的磁盘上。', '文件路径', '');

$_tipsdb['log_keepDays'] = new DAttrHelp("保留天数", '指定访问日志文件将被保存在磁盘上多少天。 只有早于指定天数的轮转日志文件会被删除。 当前的日志文件不会被删除，无论它包含了多少天的数据。 如果你不想自动删除过时的、很旧的日志文件， 将该值设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>。', '', '整数', '');

$_tipsdb['log_logLevel'] = new DAttrHelp("日志级别", '指定日志文件中记录的日志级别。 可用级别（由高到低）为: <span class=&quot;lst-inline-token lst-inline-token--value&quot;>ERROR</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>WARNING</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NOTICE</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>INFO</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>。 只有级别与当前设置相同或更高的消息将被记录（级别越低记录越详细）。', '[性能] 使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>日志级别本身不会影响性能， 除非&quot;调试级别&quot;设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>以外的级别。建议将日志级别设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>， 并将调试级别设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>。这样不会让调试日志填满磁盘， 但仍可使用&quot;切换调试日志&quot;控制调试输出。此操作可以实时启用或关闭调试记录， 对于调试繁忙的生产服务器非常有用。', '从下拉列表选择', '');

$_tipsdb['log_rollingSize'] = new DAttrHelp("轮转大小（字节）", '指定当前日志文件何时需要轮转，也称为日志轮转。 当文件大小超过轮转限制后，活动日志文件会在同一目录中重命名 为log_name.mm_dd_yyyy(.sequence)，并创建新的活动日志文件。 轮转后的日志文件实际大小有时会略大于此限制。 设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>可禁用日志轮转。', '可在数字后附加&quot;K&quot;、&quot;M&quot;、&quot;G&quot;分别表示千字节、兆字节和千兆字节。', '整数', '');

$_tipsdb['loggerAddress'] = new DAttrHelp("远程日志程序地址（可选）", '指定此管道日志程序使用的可选套接字地址。如果日志程序通过网络套接字或Unix域套接字访问，请设置此项。对于由已配置命令路径启动的本地管道日志程序，可留空。', '', 'IPv4地址:端口、主机名:端口、[IPv6地址]:端口、UDS://path或unix:path', '127.0.0.1:1514<br/>logger.example.com:1514<br/>UDS://tmp/lshttpd/logger.sock');

$_tipsdb['loginHistoryRetention'] = new DAttrHelp("登录历史保留天数", '指定登录历史记录在被移除前保留的天数。未设置时默认值：90天。', '请保留足够的历史记录用于审计和故障排查，但避免保留超过实际需要的数据。', '整数', '');

$_tipsdb['loginThrottle'] = new DAttrHelp("登录限制", '配置WebAdmin控制台登录限制，以及相关登录记录和审计记录的保留策略。', '', '', '');

$_tipsdb['lsapiContext'] = new DAttrHelp("LiteSpeed SAPI上下文", '外部应用程序不能直接使用。必须将其配置为脚本处理程序，或通过上下文映射到URL。 LiteSpeed SAPI上下文会将URI关联到LSAPI（LiteSpeed Server Application Programming Interface）应用程序。 目前PHP、Ruby和Python都有LSAPI模块。LSAPI专为LiteSpeed Web Server开发，是与LiteSpeed Web Server通信的最高效方式。', '', '', '');

$_tipsdb['lsapiapp'] = new DAttrHelp("LiteSpeed SAPI App", '指定要连接到此上下文的LiteSpeed SAPI应用程序。 此应用程序必须在服务器或虚拟主机级别的 &quot;外部应用&quot;中定义。', '', '从下拉列表选择', '');

$_tipsdb['lsrecaptcha'] = new DAttrHelp("CAPTCHA保护", 'CAPTCHA保护是一种用于减轻服务器负载的服务。当下列情况之一发生后，CAPTCHA保护将被激活。激活后，所有非信任客户端（按配置）发出的请求都会被重定向到CAPTCHA验证页面。验证完成后，客户端会被重定向到目标页面。<br/>下列情况将启用CAPTCHA保护： 1. 服务器或虚拟主机并发请求计数超过连接限制。<br/>2. 启用了Anti-DDoS，并且客户端以可疑的方式访问了URL。 客户端将首先重定向到CAPTCHA，而不是在触发时被拒绝。<br/>3. 提供了新的重写规则环境，可通过RewriteRules激活CAPTCHA。可以设置“verifycaptcha”将客户端重定向到CAPTCHA。特殊值“: deny”可在客户端失败次数过多时拒绝它。例如，[E=verifycaptcha]会一直重定向到CAPTCHA，直到验证通过。[E=verifycaptcha: deny]会一直重定向到CAPTCHA，直到达到最大尝试次数，之后客户端将被拒绝。', '', '', '');

$_tipsdb['lstatus'] = new DAttrHelp("状态 - 侦听器", '此侦听器的当前状态。状态为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Error</span>。', '如果侦听器处于<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Error</span>状态，可以查看服务器日志了解原因。', '', '');

$_tipsdb['mappedListeners'] = new DAttrHelp("映射的侦听器", '指定此模板映射到的所有侦听器名称。 此模板成员虚拟主机的侦听器到虚拟主机映射会添加到此字段指定的侦听器。 该映射会根据成员虚拟主机各自配置中的域名和别名，将侦听器映射到虚拟主机。', '', '逗号分隔列表', '');

$_tipsdb['maxCGIInstances'] = new DAttrHelp("最大CGI实例数量", '指定服务器可以启动的CGI进程最大并发数量。 对于每个对CGI脚本的请求，服务器需要启动一个独立的CGI进程。 在Unix系统中，并发进程的数量是有限的。过多的并发进程会降 低整个系统的性能，也是一种进行拒绝服务攻击的方法。 LiteSpeed服务器将对CGI脚本的请求放入管道队列，限制并发 CGI进程数量，以确保最优性能和可靠性。 硬限制为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>2000</span>。', '[安全和性能建议] 更高的数量并不一定转化为更快的性能。 在大多数情况下，更低的数量提供更好的性能和安全性。更高的数量 只在CGI处理过程中读写延迟过高时有帮助。', '非负整数', '');

$_tipsdb['maxCachedFileSize'] = new DAttrHelp("最大小文件缓存(bytes)", '指定预分配内存缓冲区中缓存的静态文件最大尺寸。静态文件 可以用四种不同的方式服务：内存缓存、内存映射缓存、直接读写和 sendfile()。 尺寸小于&quot;最大小文件缓存(bytes)&quot;的文件将使用内存缓存服务。尺寸大于该限制、但小于 &quot;最大MMAP文件大小(bytes)&quot;的文件，将使用内存映射缓存服务。 尺寸大于&quot;最大MMAP文件大小(bytes)&quot;的文件将通过直接读写或sendfile() 服务。使用内存缓存服务小于4K的文件是最佳做法。', '', '非负整数', '');

$_tipsdb['maxConnections'] = new DAttrHelp("最大连接数", '指定服务器可以接受的最大并发连接数。这包括纯TCP连接和SSL连接。 一旦达到此限制，服务器将在完成活动请求时关闭“Keep-Alive”的连接。', '当服务器由&quot;root&quot;用户启动时，服务器会尝试自动调整每个进程的文件描述符限制，但是，如果自动调整失败，你可能需要手动增加此限制。', '非负整数', '');

$_tipsdb['maxConns'] = new DAttrHelp("最大连接数", '指定服务器与外部应用之间可建立的最大并发连接数。该设置控制外部应用程序可同时处理多少个请求。 然而，真正的限制还取决于外部应用本身。如果外部的速度不够快，或者无法处理大量的并发请求，那么将这个值设置得更高也无济于事。', '[性能建议] 设置一个高值并不能直接变成高性能。设置为一个不会使外部应用程序过载的值，将提供最佳的性能/吞吐量。', '非负整数', '');

$_tipsdb['maxDynRespHeaderSize'] = new DAttrHelp("动态回应报头最大大小(bytes)", '指定动态回应的最大报头大小。硬限制为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>131072</span>字节或128K.<br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>32768</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>32K</span>', '[可靠性和性能建议] 设置一个合理的低值以帮助识别外部应用程序产生的 坏的动态回应。', '非负整数', '');

$_tipsdb['maxDynRespSize'] = new DAttrHelp("最大动态响应主体大小(bytes)", '指定动态回应的最大主内容尺寸。硬限制是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>2047MB</span>。', '[可靠性和性能建议] 设置一个合理的低值以帮助识别坏的响应。恶意脚本经常包含 无限循环而导致大尺寸回应。', '非负整数', '');

$_tipsdb['maxKeepAliveReq'] = new DAttrHelp("最大持续连接请求数", '指定通过持续连接（持久）会话处理的请求的最大数量。一旦达 到此限制，连接将被关闭。您也可以为每个虚拟主机单独设置限制。', '[性能建议] 设置为合理的较高的值。值为“1”或“0”时将禁用持续连接。', '非负整数', '');

$_tipsdb['maxMMapFileSize'] = new DAttrHelp("最大MMAP文件大小(bytes)", '指定使用内存映射（MMAP）的最大静态文件大小。 静态文件可以用四种不同的方式服务：内存缓存、内存映射缓存、直接读写和 sendfile()。 尺寸小于&quot;最大小文件缓存(bytes)&quot;的文件将使用内存缓存服务。尺寸大于该限制、但小于 &quot;最大MMAP文件大小(bytes)&quot;的文件，将使用内存映射缓存服务。 尺寸大于&quot;最大MMAP文件大小(bytes)&quot;的文件将通过直接读写或sendfile() 服务。 由于服务器有一个32位的地址空间（2GB），不建议使用内存映射非常大的文件。', '', '非负整数', '');

$_tipsdb['maxMindDBEnv'] = new DAttrHelp("环境变量", '将数据库查找的结果分配给环境变量。', '', 'Variable_Name mapped_DB_data<br/><br/>每行一个条目。数据路径可以使用映射键或从0开始的数组索引，两者都用/分隔。', 'COUNTRY_CODE COUNTRY_DB/country/iso_code<br/>REGION_CODE  CITY_DB/subdivisions/0/iso_code');

$_tipsdb['maxReqBodySize'] = new DAttrHelp("最大请求主内容大小(bytes)", '指定HTTP请求主内容最大尺寸。对于32位操作系统， 硬限制为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>2GB</span>。对于64位操作系统，几乎是无限的。', '[安全建议] 为了防止拒绝服务攻击，尽量将限制值设定到实际需求的大小。 交换空间的剩余空间必须比这个限制值大。', '非负整数', '');

$_tipsdb['maxReqHeaderSize'] = new DAttrHelp("最大请求头大小(bytes)", '指定请求URL中包含的HTTP请求头最大值。 硬限制为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>131072</span> 字节或128K.<br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>32768</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>32K</span>', '[安全和性能建议] 设置合理的低值来减少内存的使用并帮助识别虚假请求和拒绝服务攻击。<br/>对于大多数网站来说4000-8000已经足够大。', '非负整数', '');

$_tipsdb['maxReqURLLen'] = new DAttrHelp("最大请求URL长度(bytes)", '指定请求URL的最大大小。 URL是一个纯文本的地址，包含查询字符串来请求服务器上的资源。 硬限制为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>65530</span>个字节。 大于此值的值，例如<span class=&quot;lst-inline-token lst-inline-token--value&quot;>64K</span>（大6个字节），将被视为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>65530</span>。<br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>8192</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>8K</span>。', '[安全和性能建议] 将其设置合理的低值来以减少内存使用 并帮助识别虚假请求和拒绝服务攻击。<br/>对大多数网站2000-3000已经足够大，除非使用HTTP GET而不是POST来提交大型的查询字符串。', '非负整数', '');

$_tipsdb['maxSSLConnections'] = new DAttrHelp("最大SSL连接数", '指定服务器将接受的最大并发SSL连接数。 由于并发SSL和非SSL的连接总数不能超过&quot;最大连接数&quot;的限制，因此允许的最大SSL连接的数量须低于此限制。', '', '非负整数', '');

$_tipsdb['memHardLimit'] = new DAttrHelp("内存硬限制 (bytes)", '与&quot;内存软限制 (bytes)&quot;非常相同，但是在一个用户进程中，软限制 可以被放宽到硬限制的数值。硬限制可以在服务器级别或独立的外部应用程序级别设 置。如果未在独立的外部应用程序级别设定限制，将使用服务器级别的限制。<br/>如果在两个级别都没有设置该限制，或者限制值设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>，将使用操 作系统的默认设置。', '[注意] 不要过度调整这个限制。如果您的应用程序需要更多的内存， 这可能会导致503错误。', '非负整数', '');

$_tipsdb['memSoftLimit'] = new DAttrHelp("内存软限制 (bytes)", '以字节为单位指定服务器启动的外部应用进程或程序的内存占用限制。<br/>此限制的目的主要是为了防范软件缺陷或蓄意攻击造成的过度内存使用， 而不是限制正常使用。确保留有足够的内存，否则您的应用程序可能故障并 返回503错误。限制可以在服务器级别或独立的外部应用程序级别设置。如 果未在独立的外部应用程序级别设定限制，将使用服务器级别的限制。<br/>如果在两个级别都没有设置该限制，或者限制值设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>，将使用操 作系统的默认设置。', '[注意] 不要过度调整这个限制。如果您的应用程序需要更多的内存， 这可能会导致503错误。', '非负整数', '');

$_tipsdb['memberVHRoot'] = new DAttrHelp("成员虚拟主机根目录", '指定此虚拟主机的根目录。如果留空，将使用此模板的默认虚拟主机根目录。<br/><br/>注意：这<b>不是</b>文档根。建议将与虚拟主机相关的所有文件（如虚拟主机配置、日志文件、HTML文件、CGI脚本等）放置在此目录下。 虚拟主机根目录可以通过变量$VH_ROOT引用。', '', '路径', '');

$_tipsdb['mime'] = new DAttrHelp("MIME设置", '为此服务器指定包含MIME设置的文件。 在chroot模式中提供了绝对路径时，该文件路径总是相对于真正的根。 点击文件名可查看/编辑详细的MIME项。', '点击文件名可编辑MIME设置。', '文件路径', '');

$_tipsdb['mimesuffix'] = new DAttrHelp("后缀", '你可以列出相同MIME类型的多个后缀，用逗号分隔。', '', '', '');

$_tipsdb['mimetype'] = new DAttrHelp("MIME类型", 'MIME类型由类型和子类型组成，格式为“类型/子类型”.', '', '', '');

$_tipsdb['minGID'] = new DAttrHelp("最小的GID", '指定外部应用程序的最小组ID。 如果组ID比这里指定的值更小，其外部脚本的执行将被拒绝。 如果的LiteSpeed Web服务器是由“Root”用户启动，它可以在“suEXEC” 模式运行外部应用程序，类似Apache（可以切换到与Web服务器不同的用户/组ID）。', '[安全] 设置足够高的值以排除所有系统用户所属的组。', '非负整数', '');

$_tipsdb['minUID'] = new DAttrHelp("最小的UID", '指定外部应用程序的最小用户ID。 如果用户ID比这里指定的值更低。其外部脚本的执行将被拒绝。 如果的LiteSpeed Web服务器由“Root”用户启动，它可以在“suEXEC” 模式运行外部应用程序，类似Apache（可以切换到与Web服务器不同的用户/组ID）。', '[安全] 设置足够高的值以排除所有系统/特权用户。', '非负整数', '');

$_tipsdb['modParams'] = new DAttrHelp("模块参数", '设置模块参数。模块参数由模块开发者定义。<br/><br/>服务器级别设置的值会全局分配为默认值。 用户可以在侦听器、虚拟主机或上下文级别覆盖此设置。如果在这些级别未设置该值，则会从上一级继承。', '', '由模块接口指定。', '');

$_tipsdb['moduleContext'] = new DAttrHelp("模块处理程序上下文", '模块处理程序上下文会将URI关联到已注册模块。 模块需要在服务器模块配置选项卡中注册。', '', '', '');

$_tipsdb['moduleEnabled'] = new DAttrHelp("启用模块", '全局启用模块Hook。此设置可在侦听器和虚拟主机级别覆盖。<br/><br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>', '', '布尔值', '');

$_tipsdb['moduleEnabled_lst'] = new DAttrHelp("启用模块", '在侦听器级别启用模块Hook。只有当模块具有TCP/IP级别Hook (L4_BEGSESSION, L4_ENDSESSION, L4_RECVING, L4_SENDING)时，此设置才会生效。<br/><br/>默认值：继承服务器级别设置', '', '布尔值', '');

$_tipsdb['moduleEnabled_vh'] = new DAttrHelp("启用模块", '在虚拟主机级别启用模块Hook。只有当模块具有HTTP级别Hook时，此设置才会生效。<br/><br/>默认值：继承服务器级别设置', '', '布尔值', '');

$_tipsdb['moduleNameSel'] = new DAttrHelp("模块", '模块名称。 该模块必须在“服务器模块配置”选项卡下配置。 配置后，模块名称将在侦听器和虚拟主机配置的下拉框中显示。', '', '从下拉列表选择', '');

$_tipsdb['modulename'] = new DAttrHelp("模块", '服务器启动时要加载的外部或内部模块的名称。<br/><br/><b>外部模块</b><br/>用于外部模块的值必须与 $SERVER_ROOT/modules/modulename.so 下的“.so”文件名匹配， 以便服务器应用程序加载该文件。 在注册新模块后需要重新启动服务器。<br/><br/><b>内部模块</b><br/>内部模块使用的值必须与构建模块时使用的名称匹配 例如，对于服务器附带的内部缓存模块，必须将其设置为“缓存”。', '', '字符串', '');

$_tipsdb['namespace'] = new DAttrHelp("Namespace容器", '如果希望在Namespace容器沙箱中启动CGI进程（包括PHP程序），请设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Enabled</span>。 仅当&quot;Bubblewrap容器&quot;设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>时使用。<br/>当服务器级别未设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span>时，此设置值可以在虚拟主机级别覆盖。<br/>默认值：<br/><b>服务器级别：</b><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disabled</span><br/><b>虚拟主机级别：</b>继承服务器级别设置', '', '从列表中选择', '');

$_tipsdb['namespaceConf'] = new DAttrHelp("Namespace模板文件", '指向现有配置文件的路径，该文件包含要挂载的目录列表以及挂载方法。 当&quot;Namespace容器&quot;设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Enabled</span>且此值未设置时，将使用以下安全默认配置：<br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;> $HOMEDIR/.lsns/tmp /tmp,tmp<br/>/usr,ro-bind<br/>/lib,ro-bind<br/>/lib64,ro-bind-try<br/>/bin,ro-bind<br/>/sbin,ro-bind<br/>/var,dir<br/>/var/www,ro-bind-try<br/>/proc,proc<br/>../tmp var/tmp,symlink<br/>/dev,dev<br/>/etc/localtime,ro-bind-try<br/>/etc/ld.so.cache,ro-bind-try<br/>/etc/resolv.conf,ro-bind-try<br/>/etc/ssl,ro-bind-try<br/>/etc/pki,ro-bind-try<br/>/etc/man_db.conf,ro-bind-try<br/>/usr/local/bin/msmtp /etc/alternatives/mta,ro-bind-try<br/>/usr/local/bin/msmtp /usr/sbin/exim,ro-bind-try<br/>$HOMEDIR,bind-try<br/>/var/lib/mysql/mysql.sock,bind-try<br/>/home/mysql/mysql.sock,bind-try<br/>/tmp/mysql.sock,bind-try<br/>/run/mysqld/mysqld.sock,bind-try<br/>/var/run/mysqld.sock,bind-try<br/>/run/user/$UID,bind-try<br/>$PASSWD<br/>$GROUP<br/>/etc/exim.jail/$USER.conf $HOMEDIR/.msmtprc,copy-try<br/>/etc/php.ini,ro-bind-try<br/>/etc/php-fpm.conf,ro-bind-try<br/>/etc/php-fpm.d,ro-bind-try<br/>/var/run,ro-bind-try<br/>/var/lib,ro-bind-try<br/>/etc/imunify360/user_config/,ro-bind-try<br/>/etc/sysconfig/imunify360,ro-bind-try<br/>/opt/plesk/php,ro-bind-try<br/>/opt/alt,bind-try<br/>/opt/cpanel,bind-try<br/>/opt/psa,bind-try<br/>/var/lib/php/sessions,bind-try </span>', '', '绝对路径或相对于$SERVER_ROOT的相对路径。', '');

$_tipsdb['namespaceConfVhAdd'] = new DAttrHelp("附加 Namespace 模板文件", '指向现有配置文件的路径，该文件包含要挂载的目录列表以及挂载方式。 如果服务器级别也设置了&quot;Namespace模板文件&quot;，则两个文件都会被使用。', '', '可以是绝对路径，也可以是相对于$SERVER_ROOT或$VH_ROOT的相对路径。', '');

$_tipsdb['nodeBin'] = new DAttrHelp("Node.js路径", 'Node.js可执行文件的路径。', '', '绝对路径', '');

$_tipsdb['nodeDefaults'] = new DAttrHelp("Node.js应用默认设置", 'Node.js应用程序的默认配置。这些设置可在Context级别覆盖。', '', '', '');

$_tipsdb['note'] = new DAttrHelp("备注", '为自己添加备注。', '', '', '');

$_tipsdb['ocspCACerts'] = new DAttrHelp("OCSP CA证书", '指定存储OCSP证书颁发机构（CA）证书的文件的位置。 这些证书用于检查OCSP响应服务器的响应（并确保这些响应不被欺骗或以其他方式被破坏）。 该文件应包含整个证书链。 如果该文件不包含根证书，则LSWS无需将根证书添加到文件中就应该能够在系统目录中找到该根证书， 但是，如果此验证失败，则应尝试将根证书添加到此文件中。<br/><br/>此设置是可选的。如果未设置，服务器会自动检查&quot;CA证书文件&quot;。', '', '文件路径', '');

$_tipsdb['ocspRespMaxAge'] = new DAttrHelp("OCSP响应最大有效时间（秒）", '此选项设置OCSP响应的最大有效时间。如果OCSP响应早于该最大有效时间，服务器将联系OCSP响应服务器以获取新的响应。默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>86400</span>。将此值设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-1</span>可以关闭最大有效时间限制。', '', '秒数整数', '');

$_tipsdb['ocspResponder'] = new DAttrHelp("OCSP响应服务器", '指定要使用的OCSP响应服务器的URL。 如果未设置，则服务器将尝试联系OCSP响应服务器 在证书颁发机构的颁发者证书中有详细说明。 某些颁发者证书可能未指定OCSP服务器URL。', '', '以<span class=&quot;lst-inline-token lst-inline-token--value&quot;>http://</span>开头的URL', '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>http://rapidssl-ocsp.geotrust.com</span>');

$_tipsdb['opsAuditRetainFiles'] = new DAttrHelp("活动日志保留文件数", '指定WebAdmin控制台要保留的操作审计文件最大数量。未设置时默认值：30个文件。', '如果需要更长的审计轨迹，可以增大此值，但请注意更多文件会占用更多磁盘空间。', '整数', '');

$_tipsdb['outBandwidth'] = new DAttrHelp("出口带宽 (bytes/sec)", '指定对单个IP地址允许的最大传出吞吐量（无论与该IP之间建立了多少个连接）。 为提高效率，真正的带宽可能最终会略高于设定值。 带宽按4KB为单位分配。设定值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>可禁用限制。 每个客户端的带宽限制（字节/秒）可以在服务器或虚拟主机级别设置。 虚拟主机级别的设置将覆盖服务器级别的设置。', '[性能建议] 按8KB单位设置带宽可获得更好的性能。<br/>[安全建议] 受信任的IP或子网不受影响。', '非负整数', '');

$_tipsdb['pcKeepAliveTimeout'] = new DAttrHelp("持久连接超时时间", '指定空闲持久连接保持打开的最大时间（秒）。<br/><br/>设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>-1</span>时，连接永不超时。设置为0或更大时，连接会在经过该秒数后关闭。', '', '秒数整数', '');

$_tipsdb['perClientConnLimit'] = new DAttrHelp("客户端流量限制", '这些是基于客户端IP的连接控制设置。 这些设置有助于缓解DoS（拒绝服务）和DDoS（分布式拒绝服务）攻击。', '', '', '');

$_tipsdb['persistConn'] = new DAttrHelp("持久连接", '指定处理完请求后是否保持连接。持久连接可以提高性能， 但某些FastCGI外部应用程序并不完全支持持久连接。默认值为“On”。', '', '布尔值', '');

$_tipsdb['phpIniOverride'] = new DAttrHelp("php.ini覆盖", '用于覆盖当前Context（虚拟主机级别或Context级别）中的php.ini设置。<br/><br/>支持的指令是：<br/>php_value<br/>php_flag<br/>php_admin_value<br/>php_admin_flag<br/><br/>所有其他行/指令将被忽略。', '', '覆盖语法类似于Apache：每行一个指令及其值，每个指令前根据需要加上php_value、php_flag、php_admin_value或php_admin_flag。', 'php_value include_path &quot;.:/usr/local/lib/php&quot;<br/>php_admin_flag engine on<br/>php_admin_value open_basedir &quot;/home&quot;');

$_tipsdb['pid'] = new DAttrHelp("PID", '当前服务器进程的PID（进程ID）。', '每次重启服务器后，PID都会改变。', '', '');

$_tipsdb['procHardLimit'] = new DAttrHelp("进程硬限制", '与&quot;进程软限制&quot;非常相同，但是，在用户进程中软限制 可以被放宽到硬限制的数值。硬限制可以在服务器级别或独立的外部应用程序级别设 置。如果未在独立的外部应用程序级别设定限制，将使用服务器级别的限制。 如果在两个级别都没有设置该限制，或者限制值设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>，将使用操 作系统的默认设置。', '', '非负整数', '');

$_tipsdb['procSoftLimit'] = new DAttrHelp("进程软限制", '限制用户可以创建的进程总数.所有现有的进程都将被计算在这个限制之内,而不仅仅是要启动的新进程。<br/><br/>该限制可以在服务器级别或单个外部应用级别进行设置.如果未在应用级别设置，则将使用服务器级别的限制. 如果该值为0或服务器级和应用级都没有设置,将使用操作系统的默认设置', 'PHP脚本可以派生进程。此限制的主要目的，是作为最后一道防线，防止PHP进程继续创建其他进程而造成fork bomb或其他攻击。<br/><br/>将此设置得太低可能会严重影响功能。因此，当设置低于某些级别时将被忽略。<br/><br/>当<b>开机自启</b>设置为“Yes (Daemon mode)”时，实际进程限制会高于此设置，以确保父进程不会被限制。', '非负整数', '');

$_tipsdb['proxyContext'] = new DAttrHelp("代理上下文", '代理上下文可将此虚拟主机作为透明反向代理服务器。 该代理服务器可运行在任何支持HTTP协议的Web服务器或应用服务器前端。 设置代理上下文之前，必须先在&quot;外部应用&quot;中定义此虚拟主机要代理的外部Web服务器。', '', '', '');

$_tipsdb['proxyProtocol'] = new DAttrHelp("PROXY协议", '使用PROXY协议与此服务器通信的前端代理IP/子网列表。设置后，服务器会对来自所列IP/子网的传入连接使用PROXY协议； 如果PROXY协议不可用，则回退到常规连接。<br/>适用于HTTP、HTTPS、HTTP2和websocket连接。', '', '以逗号分隔的IP地址或子网列表。', '');

$_tipsdb['proxyWebServer'] = new DAttrHelp("Web服务器", '指定外部Web服务器的名称。 此外部Web服务器必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义', '', '从下拉列表选择', '');

$_tipsdb['quicBasePLPMTU'] = new DAttrHelp("PLPMTU 默认值", 'QUIC默认使用的PLPMTU (无报头的最大数据包大小,以字节为单位)的默认值. 设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>将会允许QUIC设置大小.<br/>这个设置必须低于 &quot;PLPMTU的最大值&quot; 的值.<br/>默认值：0', '', '0或1200至65527之间的整数', '');

$_tipsdb['quicCfcw'] = new DAttrHelp("流连接窗口控制", '为QUIC连接分配的缓冲区的初始大小。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;> 1.5M </span>。', '更大的窗口大小将使用更多的内存。', '64K到512M之间的数字', '');

$_tipsdb['quicCongestionCtrl'] = new DAttrHelp("拥塞控制", '使用的拥塞控制算法。 可以手动设置，也可以选择“默认”选项，将其保留到的QUIC库中。<br/>默认值：Default', '', '从下拉列表选择', '');

$_tipsdb['quicEnable'] = new DAttrHelp("启用HTTP3/QUIC", '在整个服务器范围内启用HTTP3/QUIC网络协议。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>是</span>。', '当此设置设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>是</span>时，仍然可以通过&quot;打开HTTP/3/QUIC（UDP）端口&quot;设置在侦听器级别 或通过&quot;启用HTTP3/QUIC&quot;设置在虚拟主机级别禁用HTTP3/QUIC', '布尔值', '');

$_tipsdb['quicEnableDPLPMTUD'] = new DAttrHelp("启用DPLPMTUD", '启用Datagram Packetization Layer Path Maximum Transmission Unit Discovery(DPLPMTUD)。<br/><b><a href="https://tools.ietf.org/html/rfc8899" target="_blank" rel="noopener noreferrer">DPLPMTUD背景信息（RFC 8899）</a></b><br/>默认值：Yes', '', '布尔值', '');

$_tipsdb['quicHandshakeTimeout'] = new DAttrHelp("握手超时时间（秒）", '给出新的QUIC连接完成其握手的时间（以秒为单位），超过限制时间后连接将中止。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>10</span>。', '', '1到15之间的整数', '');

$_tipsdb['quicIdleTimeout'] = new DAttrHelp("空闲超时时间（秒）", '空闲的QUIC连接将被关闭的时间（以秒为单位）。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;> 30 </span>。', '', '10到30之间的整数', '');

$_tipsdb['quicMaxCfcw'] = new DAttrHelp("最大连接流量窗口值设置", '指定连接流控制窗口缓冲区通过自动调优可以达到的最大大小。<br/>默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;> 0 </span>，这意味着将使用&quot;流连接窗口控制&quot;的值，并且不会自动调整。', '更大的窗口大小将使用更多的内存。', '0或64K到512M之间的数字', '');

$_tipsdb['quicMaxPLPMTU'] = new DAttrHelp("PLPMTU的最大值", 'PLPMTU（不含报头的最大数据包大小，以字节为单位）的探测上限。此设置用于限制 DPLPMTUD搜索范围中的“最大数据包大小”。设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>将允许QUIC自行选择大小。 (默认情况下LSQUIC暂定MTU为1,500 字节 (以太网)).<br/>这个设置应该比&quot;PLPMTU 默认值&quot;的值高.<br/>默认值：0', '', '0或1200至65527之间的整数', '');

$_tipsdb['quicMaxSfcw'] = new DAttrHelp("最大流量窗口值设置", '指定由于自动调整而允许流控制窗口达到的最大大小。<br/>默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>，这意味着将使用&quot;连接流量窗口值&quot;的值，并且不会自动调整', '更大的窗口大小将使用更多的内存。', '0或64K到128M之间的数字', '');

$_tipsdb['quicMaxStreams'] = new DAttrHelp("每个连接的最大并发数", '每个QUIC连接的最大并发数。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>100</span>。', '', '10到1000之间的整数', '');

$_tipsdb['quicSfcw'] = new DAttrHelp("连接流量窗口值", 'QUIC愿意为每个流接收的初始数据量。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1M</span>。', '更大的窗口大小将使用更多的内存。', '64K到128M之间的数字', '');

$_tipsdb['quicShmDir'] = new DAttrHelp("QUIC SHM目录", '指定用于将QUIC数据保存到共享内存的目录。<br/>默认情况下，将使用服务器的默认SHM目录<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/dev/shm</span>。', '建议使用基于RAM的分区(内存盘)，例如<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/dev/shm</span>。', '路径', '');

$_tipsdb['quicVersions'] = new DAttrHelp("HTTP3/QUIC版本", '启用的HTTP3/QUIC版本的列表。 此设置仅应用于将HTTP3/QUIC支持限制为列出的版本，最好留空。', '建议将此设置留空，以便自动应用最佳配置。', '逗号分隔列表', 'h3-29, h3-Q050');

$_tipsdb['railsDefaults'] = new DAttrHelp("Rack/Rails 默认设置", 'Rack/Rails应用程序的默认配置。这些设置可在Context级别覆盖。', '', '', '');

$_tipsdb['rcvBufSize'] = new DAttrHelp("接收缓冲区大小 (bytes)", '每个TCP套接字的接收缓冲区大小。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>512K</span>是允许的最大缓冲区大小。', '[性能建议] 建议将此值保留为“未设置”或设置为0以使用操作系统的默认缓冲区大小。<br/>[性能建议] 处理大载荷入站请求，如文件上传时，大的接收缓冲区会提高性能。<br/>[性能建议] 将此值设置为较低的值将减少吞吐量和每个套接字的内存使用率，从而在内存成为瓶颈时允许服务器拥有更多并发套接字。', '非负整数', '');

$_tipsdb['realm'] = new DAttrHelp("域", '指定此上下文的授权域。 指定后，必须提供有效的用户名和密码才能访问此上下文。 &quot;授权Realm&quot;需要在&quot;虚拟主机安全&quot;部分进行设置。 此设置使用每个授权域的&quot;Realm名称&quot;。', '', '从下拉列表选择', '');

$_tipsdb['realmName'] = new DAttrHelp("Realm名称", '指定授权Realm的唯一名称。', '', '文本', '');

$_tipsdb['realms'] = new DAttrHelp("授权Realm", '列出此虚拟主机的所有授权Realm。 授权Realm用于阻止未授权用户访问受保护的网页。 Realm是一个用户目录，其中包含用户名、密码和可选的组分类。授权在上下文级别执行。 由于不同上下文可以共享同一个Realm（用户数据库），因此Realm会与使用它们的上下文分开定义。 您可以在上下文配置中通过这些名称引用Realm。', '', '', '');

$_tipsdb['realtimerpt'] = new DAttrHelp("实时统计", '实时统计链接会打开实时服务器状态报告页面。 这是监控系统的便捷工具。   该报告显示服务器统计信息的快照。此快照的刷新速率由右上角的刷新间隔下拉列表控制。   报告包含以下部分： <ul><li>服务器健康状态显示基本服务器统计、运行时间、负载以及被anti-DDoS阻止的IP。</li>   <li>服务器列出当前流量吞吐量、连接和请求统计。</li>  <li>虚拟主机显示每个虚拟主机的请求处理状态和外部应用程序状态。</li>  <li>外部应用程序列出当前运行的外部应用程序及其使用统计。  CGI守护进程lscgid始终作为外部应用程序运行。</li> </ul>   实时统计中的许多行带有图表图标。 点击此图标会打开该行统计数据的实时更新图表。   在“服务器”部分的“请求”旁边，有一个标记为（详细信息）的链接。 该链接会进入请求快照页面，你可以查看哪些客户端正在发出特定类型的请求， 或站点的哪些方面成为瓶颈。蓝色区域中的字段可用于筛选快照， 以隔离服务器的特定部分，或查找正在执行特定操作的客户端。', '', '', '');

$_tipsdb['recaptchaAllowedRobotHits'] = new DAttrHelp("允许的机器人点击", '设置每10秒允许“好机器人”通过的点击次数。 当服务器处于高负载状态时，僵尸程序仍会受到限制。<br/>默认值是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>3</span>.', '', '非负整数', '');

$_tipsdb['recaptchaBotWhiteList'] = new DAttrHelp("Bot白名单", '自定义允许访问的用户代理列表。 将受到“好机器人”的限制，包括allowedRobotHits。', '', '用户代理列表，每行一个。 支持正则表达式。', '');

$_tipsdb['recaptchaMaxTries'] = new DAttrHelp("最大尝试次数", '“最大尝试次数”指定在拒绝访客之前允许的最大CAPTCHA次尝试次数。<br/>默认值是 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>3</span>.', '', '非负整数', '');

$_tipsdb['recaptchaRegConnLimit'] = new DAttrHelp("连接限制", '激活CAPTCHA所需的并发连接数（SSL和非SSL）。 在并发连接数高于该数字之前，将使用CAPTCHA。<br/>默认值是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>15000</span>.', '', '非负整数', '');

$_tipsdb['recaptchaSecretKey'] = new DAttrHelp("密钥", 'reCAPTCHA或hCaptcha服务分配的私钥。<br/>使用reCAPTCHA时，如果未设置密钥，将使用默认Secret Key（不推荐）。', '', '', '');

$_tipsdb['recaptchaSiteKey'] = new DAttrHelp("网站密匙", 'reCAPTCHA或hCaptcha服务分配的公钥。<br/>使用reCAPTCHA时，如果未设置站点密钥，将使用默认Site Key（不推荐）。', '', '文本', '');

$_tipsdb['recaptchaSslConnLimit'] = new DAttrHelp("SSL连接限制", '激活CAPTCHA所需的并发SSL连接数。在并发连接数高于该数字之前，将使用CAPTCHA。<br/>默认值是 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>10000</span>.', '', '非负整数', '');

$_tipsdb['recaptchaType'] = new DAttrHelp("CAPTCHA类型", '指定与密钥对一起使用的CAPTCHA类型。<br/>如果未提供密钥对，且此设置设为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Not Set</span>，则会使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>reCAPTCHA Invisible</span>类型的默认CAPTCHA密钥对。<br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>reCAPTCHA Checkbox</span>会向访问者显示复选框CAPTCHA以进行验证。 （需要&quot;网站密匙&quot;、&quot;密钥&quot;）<br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>reCAPTCHA Invisible</span>会尝试自动验证CAPTCHA，验证成功后会重定向到目标页面。 （需要&quot;网站密匙&quot;、&quot;密钥&quot;）<br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>hCaptcha</span>可用于支持CAPTCHA提供商 <a href="https://www.hcaptcha.com" target="_blank" rel="noopener noreferrer">hCaptcha</a>。 （需要&quot;网站密匙&quot;、&quot;密钥&quot;）<br/>默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>reCAPTCHA Invisible</span>。', '', '从列表中选择', '');

$_tipsdb['recaptchaVhReqLimit'] = new DAttrHelp("并发请求限制", '激活CAPTCHA所需的并发请求数。CAPTCHA会持续使用，直到并发请求数低于此值。<br/><br/>默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>15000</span>。', '', '非负整数', '');

$_tipsdb['redirectContext'] = new DAttrHelp("重定向上下文", '重定向上下文可将一个URI或一组URI转发到其他位置。 目标URI可以位于同一网站（内部重定向），也可以是指向其他网站的绝对URI（外部重定向）。', '', '', '');

$_tipsdb['renegProtection'] = new DAttrHelp("SSL密钥重新协商保护", '指定是否启用SSL密钥重新协商保护，以 防御基于SSL握手的攻击。默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>。', '可以在侦听器和虚拟主机级别启用此设置。', '布尔值', '');

$_tipsdb['required'] = new DAttrHelp("Require（授权的用户/组）", '指定哪些用户/用户组可以访问此上下文。 这允许你在多个上下文中使用同一个用户/组数据库（由&quot;域&quot;指定）， 但只允许该数据库中的特定用户/组访问此上下文。', '', '语法与Apache的Require指令兼容。例如： <ul> <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>user username [username ...]</span><br/>只有列出的用户可以访问此上下文。</li> <li> <span class=&quot;lst-inline-token lst-inline-token--value&quot;>group groupid [groupid ...]</span><br/>只有属于列出组的用户可以访问此上下文。</li> </ul> 如果未指定此设置，所有有效用户都可以访问此资源。', '');

$_tipsdb['requiredPermissionMask'] = new DAttrHelp("必需的权限掩码", '为静态文件指定必需的权限掩码。 例如，如果只允许所有人都可读的文件可以被输出，将该值设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0004</span>。 用<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 stat</span>命令了解所有可选值。', '', '八进制数', '');

$_tipsdb['respBuffer'] = new DAttrHelp("响应缓冲", '指定是否缓冲从外部应用程序收到的响应。 如果检测到“nph-”（Non-Parsed-Header）脚本，则会关闭带完整HTTP头响应的缓冲。', '', '从下拉列表选择', '');

$_tipsdb['restart'] = new DAttrHelp("应用更改/平滑重启", '点击<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Graceful Restart</span>后，将启动新的服务器进程。 对于<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Graceful Restart</span>，旧服务器进程只会在所有请求处理完成后 （或达到&quot;平滑重启超时时长(secs)&quot;限制后）退出。   配置更改会在下一次重启时应用。 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Graceful Restart</span>会在不中断服务器服务的情况下应用这些更改。', '平滑重启生成新服务器进程通常少于2秒。', '', '');

$_tipsdb['restrained'] = new DAttrHelp("访问管制", '指定是否可以通过此网站访问虚拟主机根目录（$VH_ROOT）之外的文件。 如果设置是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>，只可以访问$VH_ROOT下的文件， 访问指向$VH_ROOT以外文件或目录的符号链接或Context都将被阻止。 尽管如此，这里不会限制CGI脚本的访问。 这个选项在共享主机下非常有用。 &quot;跟随符号链接&quot;可以设置成<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>来允许用户使用在$VH_ROOT下的符号链接， $VH_ROOT以外的则不可以。', '[安全建议] 在共享主机环境下打开该功能。', '布尔值', '');

$_tipsdb['restrictedDirPermissionMask'] = new DAttrHelp("脚本目录限制权限掩码", '指定服务器不会提供服务的脚本文件父目录的限制权限掩码。 例如，要禁止服务属组可写和全局可写的文件夹内的PHP脚本， 设置掩码为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>022</span>。默认值是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>000</span>。 此选项可用于防止执行文件上传目录内的脚本。<br/>用<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 stat</span>命令了解所有可选值。', '', '八进制数', '');

$_tipsdb['restrictedPermissionMask'] = new DAttrHelp("限制权限掩码", '为不能输出的静态文件指定限制权限掩码。 例如，要禁止服务可执行文件，将掩码设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0111</span>。<br/>用<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 stat</span>命令了解所有可选值。', '', '八进制数', '');

$_tipsdb['restrictedScriptPermissionMask'] = new DAttrHelp("脚本限制权限掩码", '为不能服务的脚本文件指定限制权限掩码。 例如，要禁止服务属组可写和全局可写的PHP脚本， 设置掩码为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>022</span>。默认值是<span class=&quot;lst-inline-token lst-inline-token--value&quot;>000</span>。<br/>用<span class=&quot;lst-inline-token lst-inline-token--command&quot;>man 2 stat</span>命令了解所有可选值。', '', '八进制数', '');

$_tipsdb['retryTimeout'] = new DAttrHelp("重试超时（秒）", '指定服务器在重试之前，等待出现通信问题的外部应用程序的时间。', '', '秒数整数', '');

$_tipsdb['reusePort'] = new DAttrHelp("启用REUSEPORT", '使用SO_REUSEPORT套接字选项将传入流量分配给多个工作进程。此设置仅对多工作进程许可证有效。启用后，所有工作进程都会自动绑定到此侦听器，并忽略“绑定”配置。<br/><br/>默认值：开启', '', '布尔值', '');

$_tipsdb['rewriteBase'] = new DAttrHelp("重写基准", '指定重写规则的基准URL。', '', 'URL', '');

$_tipsdb['rewriteInherit'] = new DAttrHelp("重写继承", '指定是否从父级上下文继承重写规则。 如果启用重写但不继承，将使用此上下文中定义的重写基准和重写规则。', '', '布尔值', '');

$_tipsdb['rewriteLogLevel'] = new DAttrHelp("日志级别", '指定重写引擎调试输出的详细程度。 此值范围为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>-<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span>。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>禁用日志记录，<span class=&quot;lst-inline-token lst-inline-token--value&quot;>9</span>生成最详细日志。 服务器和虚拟主机的错误日志&quot;日志级别&quot; 必须至少设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>INFO</span>，此选项才会生效。 此设置在测试重写规则时很有用。', '', '无符号整数', '');

$_tipsdb['rewriteMapLocation'] = new DAttrHelp("位置", '使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>MapType:MapSource</span>语法指定重写映射的位置。<br/>LiteSpeed重写引擎支持三种重写映射： <ul> 	<li><b>标准纯文本</b> <blockquote> 		<b>MapType:</b> txt; <br/>		<b>MapSource:</b> 指向有效纯ASCII文件的文件路径。 </blockquote> 		此文件的每一行都应包含两个以空白分隔的元素。第一个元素是键，第二个元素是值。可以使用前导&quot;<span class=&quot;lst-inline-token lst-inline-token--value&quot;>#</span>&quot;符号添加注释。 	</li> 	<li><b>随机纯文本</b> <blockquote> 		<b>MapType:</b> rnd;<br/>		<b>MapSource:</b> 有效纯ASCII文件的文件路径。 </blockquote> 		文件格式类似于标准纯文本文件，但第二个元素可以包含多个由&quot;<span class=&quot;lst-inline-token lst-inline-token--value&quot;>|</span>&quot;符号分隔的选项，并由重写引擎随机选择。 	</li> 	<li><b>内部函数</b> <blockquote> 	    <b>MapType:</b> int;<br/>		<b>MapSource:</b> 内部字符串函数 </blockquote> 		有4个可用函数： 		<ul> 			<li><b>toupper:</b> 将查找键转换为大写。</li> 			<li><b>tolower:</b> 将查找键转换为小写。</li> 			<li><b>escape:</b> 对查找键执行URL编码。</li> 			<li><b>unescape:</b> 对查找键执行URL解码。</li> 		</ul> 	</li> 	Apache中可用的以下映射类型未在LiteSpeed中实现：<br/>哈希文件和外部重写程序。 </ul> LiteSpeed重写引擎实现遵循Apache重写引擎规范。有关重写映射的更多详细信息，请参阅 <a href="   http://httpd.apache.org/docs/current/mod/mod_rewrite.html " target="_blank" rel="noopener noreferrer">   Apache mod_rewrite文档 </a>.', '', '字符串', '');

$_tipsdb['rewriteMapName'] = new DAttrHelp("名称", '指定虚拟主机级别重写映射的唯一名称。重写规则中的映射引用会使用此名称。 引用此名称时，应使用以下语法之一： <blockquote><code> $\{MapName:LookupKey\}<br/>$\{MapName:LookupKey|DefaultValue\} </code></blockquote><br/>LiteSpeed重写引擎实现遵循Apache重写引擎规范。有关重写映射的更多详细信息，请参阅 <a href="   http://httpd.apache.org/docs/current/mod/mod_rewrite.html " target="_blank" rel="noopener noreferrer">   Apache mod_rewrite文档 </a>.', '', '字符串', '');

$_tipsdb['rewriteRules'] = new DAttrHelp("重写规则", '指定虚拟主机级别的重写规则列表。<br/><br/>请勿在此处添加任何文档根级别的重写规则。 如果有来自.htaccess的文档根级别重写规则，请改为创建URI为&quot;/&quot;的静态Context， 并在那里添加这些重写规则。<br/><br/>重写规则由一个<span class=&quot;lst-inline-token lst-inline-token--value&quot;>RewriteRule</span>指令组成，并且前面可以选择性地放置多个<span class=&quot;lst-inline-token lst-inline-token--value&quot;>RewriteCond</span>指令。 <ul>   <li>每条指令应只占一行。</li>   <li>     <span class=&quot;lst-inline-token lst-inline-token--value&quot;>RewriteCond</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>RewriteRule</span>遵循Apache的重写指令语法。可以直接从Apache配置文件复制并粘贴重写指令。   </li>   <li>     LiteSpeed与Apache mod_rewrite的实现有细微差别：     <ul>       <li>         <span class=&quot;lst-inline-token lst-inline-token--value&quot;>%\{LA-U:variable\}</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>%\{LA-F:variable\}</span>会被LiteSpeed重写引擎忽略。       </li>       <li>         LiteSpeed重写引擎新增了两个服务器变量：         <span class=&quot;lst-inline-token lst-inline-token--value&quot;>%\{CURRENT_URI\}</span>表示重写引擎正在处理的当前URI，         <span class=&quot;lst-inline-token lst-inline-token--value&quot;>%\{SCRIPT_NAME\}</span>与对应CGI环境变量含义相同。       </li>       <li>         LiteSpeed重写引擎遇到<span class=&quot;lst-inline-token lst-inline-token--value&quot;>[L]</span>标志后会停止处理重写规则以避免循环；         Apache mod_rewrite只会停止当前迭代中的重写规则处理。此行为类似于Apache mod_rewrite中的<span class=&quot;lst-inline-token lst-inline-token--value&quot;>[END]</span>标志。       </li>     </ul>   </li> </ul><br/>LiteSpeed重写引擎实现遵循Apache重写引擎规范。 有关重写规则的更多详细信息，请参阅 <a href="   http://httpd.apache.org/docs/current/mod/mod_rewrite.html " target="_blank" rel="noopener noreferrer">   Apache mod_rewrite文档 </a> 和 <a href="   http://httpd.apache.org/docs/current/rewrite/ " target="_blank" rel="noopener noreferrer">   Apache URL重写指南 </a>.', '', '字符串', '');

$_tipsdb['rubyBin'] = new DAttrHelp("Ruby路径", 'Ruby可执行文件的路径。通常为/usr/bin/ruby 或/usr/local/bin/ruby，具体取决于Ruby的安装位置。', '', '绝对路径', '');

$_tipsdb['runOnStartUp'] = new DAttrHelp("开机自启", '指定是否在服务器启动时启动外部应用程序。 只适用于可以管理自己子进程的外部应用程序，并且&quot;实例数&quot;值设置为&quot;1&quot;。<br/><br/>如果启用，将在服务器启动时而不是运行时创建外部进程。<br/><br/>选择“Yes (Detached mode)”时，可以通过分别触碰$SERVER_ROOT/admin/tmp/或$VH_ROOT/目录下的 &#039;.lsphp_restart.txt&#039;文件，在服务器级别或虚拟主机级别重新启动所有分离模式进程。<br/><br/>默认值：Yes (Detached mode)', '[性能] 如果配置的外部进程有很大的启动开销（例如Rails），应启用此选项以减少首页响应时间。', '布尔值', '');

$_tipsdb['runningAs'] = new DAttrHelp("运行方式", '指定服务器进程运行所使用的用户/组。该值在安装前运行configure命令时，通过“--with-user”和“--with-group”参数设置。要重置这些值，必须重新运行configure命令并重新安装。', '[安全] 服务器不应以特权用户（如“root”）运行。 请务必将服务器配置为以没有登录/shell访问权限的非特权用户/组组合运行。通常，用户/组使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>nobody</span>是不错的选择。', '', '');

$_tipsdb['scgiContext'] = new DAttrHelp("SCGI上下文", 'SCGI应用程序不能直接使用。必须将SCGI应用程序配置为脚本处理程序，或通过SCGI上下文映射到URL。 SCGI上下文会将URI关联到SCGI应用程序。', '', '', '');

$_tipsdb['scgiapp'] = new DAttrHelp("SCGI App", '指定SCGI应用程序的名称。此应用程序必须在服务器或虚拟主机级别的&quot;外部应用&quot;部分中定义。', '', '从下拉列表选择', '');

$_tipsdb['servAction'] = new DAttrHelp("操作", '此菜单提供六个操作：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Graceful Restart</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Toggle Debug Logging</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Server Log Viewer</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Real-Time Statistics</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Version Manager</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Compile PHP</span>。 <ul><li>&quot;应用更改/平滑重启&quot;会平滑重启服务器进程，不中断正在处理的请求。</li> 	<li>&quot;切换调试日志&quot;可开启或关闭调试日志。</li> 	<li>&quot;服务器日志查看器&quot;可通过日志查看器查看服务器日志。</li> 	<li>&quot;实时统计&quot;可查看实时服务器状态。</li> 	<li>&quot;版本管理&quot;可下载LSWS的新版本，并在不同版本之间切换。</li> 	<li>Compile PHP可为LiteSpeed Web Server编译PHP。</li> </ul>', '也可以使用shell工具<span class=&quot;lst-inline-token lst-inline-token--command&quot;>$SERVER_ROOT/bin/lswsctrl</span>控制服务器进程， 但这需要登录shell。', '', '');

$_tipsdb['servModules'] = new DAttrHelp("服务器模块", '服务器模块配置会全局定义模块配置数据。 定义后，侦听器和虚拟主机即可访问这些模块及其配置。<br/><br/>所有要处理的模块都必须在服务器配置中注册。服务器配置还定义模块参数数据的默认值。这些值可由侦听器和虚拟主机配置数据继承或覆盖。<br/><br/>模块优先级只在服务器级别定义，并由侦听器和虚拟主机配置继承。', '', '', '');

$_tipsdb['serverName'] = new DAttrHelp("服务器名称", '此服务器的唯一名称。如果留空，默认使用服务器主机名。', '', '文本', '');

$_tipsdb['serverPriority'] = new DAttrHelp("优先级", '指定服务进程的优先级。数值范围从 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>-20</span> 到 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>20</span>。数值越小，优先级越高。', '[性能] 通常，较高的优先级会导致繁忙的服务器上的Web性能稍有提高。 不要将优先级设置为高于数据库进程的优先级。', '整数', '');

$_tipsdb['servletContext'] = new DAttrHelp("Servlet上下文", '可以通过Servlet上下文单独导入Servlet。 Servlet上下文只需指定Servlet的URI和Servlet引擎名称。 仅当你不想导入整个Web应用程序，或希望用不同授权域保护不同Servlet时才需要使用此项。 此URI与&quot;Java Web应用上下文&quot;具有相同要求。', '', '', '');

$_tipsdb['servletEngine'] = new DAttrHelp("Servlet Engine", '指定为该Web应用程序提供服务的Servlet Engine的名称。 Servlet引擎必须在服务器或虚拟主机级别的&quot;外部应用&quot;中定义。', '', '从下拉列表选择', '');

$_tipsdb['setUidMode'] = new DAttrHelp("外部应用程序设置UID模式", '指定如何为外部应用程序进程设置用户ID。可以选择下面三种方式： <ul><li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Server UID</span>: 为外部应用程序设置与服务器用户/组ID相同的用户/组ID。</li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>CGI File UID</span>: 为外部应用CGI程序设置基于可执行文件的用户/组ID。该选项仅适用于CGI，不适用于FastCGI或LSPHP。</li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Doc Root UID</span>: 为外部应用程序设置基于当前虚拟主机文档根目录的用户/组ID。</li> </ul><br/><br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Server UID</span>', '[安全] 在共享主机环境中，建议使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>CGI File UID</span> 或 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Doc Root UID</span>模式来防止一个虚拟主机下的文件被另一个虚拟主机的外部应用程序访问。', '从下拉列表选择', '');

$_tipsdb['shHandlerName'] = new DAttrHelp("处理器名称", '当处理器类型为LSAPI app、Web Server (Proxy)、Fast CGI、SCGI、Load balancer、 Servlet Engine或uWSGI时，指定处理脚本文件的外部应用程序名称。', '', '从列表中选择', '');

$_tipsdb['shType'] = new DAttrHelp("处理器类型", '指定处理这些脚本文件的外部应用程序类型。可用类型包括： <span class=&quot;lst-inline-token lst-inline-token--value&quot;>LSAPI app</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Web Server (Proxy)</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Fast CGI</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>SCGI</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>CGI</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Load balancer</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Servlet Engine</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>uWSGI</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Module Handler</span>。除<span class=&quot;lst-inline-token lst-inline-token--value&quot;>CGI</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Module Handler</span> 处理器类型外，&quot;处理器名称&quot;还必须设置为&quot;外部应用&quot;部分中已预先定义的外部应用程序。', '', '从列表中选择', '');

$_tipsdb['shmDefaultDir'] = new DAttrHelp("默认SHM目录", '将共享内存的默认目录更改为指定的路径。 如果该目录不存在，则将创建该目录。除非另有说明，否则所有SHM数据都将存储在此目录中。', '', 'path', '');

$_tipsdb['showVersionNumber'] = new DAttrHelp("服务器签名", '指定是否在响应头的Server参数中显示服务器签名和版本号。 有三个选项：当设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Hide Version</span>时，只显示<span class=&quot;lst-inline-token lst-inline-token--value&quot;>LiteSpeed</span>。当设置为 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Show Version</span>，显示LiteSpeed和版本号。  设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Hide Full Header</span>时，整个Server头都不会显示在响应报头中。', '[安全建议] 如果你不想暴露服务器的版本号，设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Hide Version</span>。', '从下拉列表选择', '');

$_tipsdb['sname'] = new DAttrHelp("名称 - 服务器", '用于识别此服务器的唯一名称。 这是在常规配置中指定的&quot;服务器名称&quot;。', '', '', '');

$_tipsdb['sndBufSize'] = new DAttrHelp("发送缓冲区大小", '每个TCP套接字的发送缓冲区的大小。设定值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>0</span>使用 操作系统默认的缓冲区大小。<span class=&quot;lst-inline-token lst-inline-token--value&quot;>65535</span>是允许的最大缓冲区大小。', '[性能建议] 建议将此值保留为“未设置”或设置为0以使用操作系统的默认缓冲区大小。<br/>[性能建议] 如果您的网站服务大量的静态文件，增加发送缓冲区 大小来提高性能。<br/>[性能建议] 将此值设置为较低的值将减少吞吐量和每个套接字的内存使用率，从而在内存成为瓶颈时允许服务器拥有更多并发套接字。', '非负整数', '');

$_tipsdb['softLimit'] = new DAttrHelp("连接软限制", '指定来自单个IP的并发连接的软限制。 并发连接数低于&quot;连接硬限制&quot;时，此软限制可以在&quot;宽限期（秒）&quot;期间临时超过， 但Keep-Alive连接将被尽快断开，直到连接数低于软限制。 如果&quot;宽限期（秒）&quot;之后，连接数仍然超过软限制，相应的IP将被封锁 &quot;禁止期（秒）&quot;所设置的时长。<br/>例如，如果页面包含许多小图像，浏览器可能会尝试同时建立许多连接，尤其是HTTP/1.0客户端。你应当在短时间内允许这些连接。<br/>HTTP/1.1客户端还可能建立多个连接，以加快下载，另外SSL需要为非SSL连接建立单独的连接。确保限制设置正确， 以免影响正常服务。建议限制在<span class=&quot;lst-inline-token lst-inline-token--value&quot;>5</span>与<span class=&quot;lst-inline-token lst-inline-token--value&quot;>10</span>之间。', '[安全] 较低的数值可让服务器服务更多不同的客户端。<br/>[安全建议] 受信任的IP或子网不受影响。<br/>[性能建议] 使用大量并发客户端进行性能评测时，请设置一个较高的值。', '非负整数', '');

$_tipsdb['sslCert'] = new DAttrHelp("SSL私钥和证书", '每个SSL侦听器都需要成对的SSL私钥和SSL证书。 多个SSL侦听器可以共享相同的密钥和证书。<br/>您可以使用SSL软件包自行生成SSL私钥， 例如OpenSSL。SSL证书也可以从授权证书颁发机构（如VeriSign或Thawte）购买。您也可以自己签署证书。 自签名证书将不受Web浏览器信任，并且不应在公共网站上使用。但是，自签名证书足以供内部使用，例如用于加密到LiteSpeed Web服务器WebAdmin控制台的流量。', '', '', '');

$_tipsdb['sslDefaultCiphers'] = new DAttrHelp("默认密码套件", 'SSL证书使用的默认密码套件。<br/>默认值：服务器内部默认值（基于当前最佳实践）', '', '以冒号分隔的密码套件规格字符串。', '');

$_tipsdb['sslEnableMultiCerts'] = new DAttrHelp("启用多个SSL证书", '允许侦听器/虚拟主机设置多个SSL证书。 如果启用了多个证书，则证书/密钥应遵循命名方案。 如果证书名为server.crt，则其他可能的证书名称为server.crt.rsa， server.crt.dsa，server.crt.ecc。 如果为“未设置”，则默认为“否”。', '', '布尔值', '');

$_tipsdb['sslOCSP'] = new DAttrHelp("OCSP装订", '在线证书状态协议（OCSP）是更加有效的检查数字证书是否有效的方式。 它通过与另一台服务器（OCSP响应服务器）通信，以获取证书有效的验证，而不是通过证书吊销列表（CRL）进行检查。<br/><br/>OCSP装订是对该协议的进一步改进，允许服务器以固定的时间间隔而不是每次请求证书时与OCSP响应程序进行检查。 有关更多详细信息，请参见<a href="   https://zh.wikipedia.org/wiki/%E5%9C%A8%E7%BA%BF%E8%AF%81%E4%B9%A6%E7%8A%B6%E6%80%81%E5%8D%8F%E8%AE%AE " target="_blank" rel="noopener noreferrer">   OCSP Wikipedia页面 </a>。', '', '', '');

$_tipsdb['sslOcspProxy'] = new DAttrHelp("OCSP代理", '用作OCSP验证代理服务器地址的套接字地址。不使用代理时请保持未设置。<br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>未设置</span>', '', '套接字地址', '');

$_tipsdb['sslProtocol'] = new DAttrHelp("协议版本", '侦听器接受的SSL协议选择。<br/><br/>选项包括：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>SSL v3.0</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>TLS v1.0</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>TLS v1.1</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>TLS v1.2</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>TLS v1.3</span>。', '', '从复选框中选择', '');

$_tipsdb['sslSessionCache'] = new DAttrHelp("启用SSL会话缓存", '使用OpenSSL的默认设置启用会话ID缓存。 服务器级别设置必须设置为“是”才能使虚拟主机设置生效。<br/>默认值:<br/><b>服务器级别:</b> Yes<br/><b>虚拟主机级别:</b> Yes', '', '布尔值', '');

$_tipsdb['sslSessionCacheSize'] = new DAttrHelp("会话缓存大小 (bytes)", '设置要存储在缓存中的SSL会话ID的最大数量。 默认值为1,000,000。', '', '整数', '');

$_tipsdb['sslSessionCacheTimeout'] = new DAttrHelp("会话缓存过期时间 (secs)", '此值确定需要重新握手之前，会话ID在缓存中有效的时间。 默认值为 3,600.', '', '整数', '');

$_tipsdb['sslSessionTicketKeyFile'] = new DAttrHelp("SSL会话记录单密钥文件", '允许管理员创建和维护SSL票证密钥。该文件长度必须为48字节。 如果此选项留空，负载均衡器将生成并轮换自己的密钥集。<br/>重要：为保持前向保密，强烈建议每隔<b>SSL会话票证生存时间</b>秒更换密钥。 如果无法做到，建议将此字段留空。', '', '路径', '');

$_tipsdb['sslSessionTicketLifetime'] = new DAttrHelp("SSL会话记录单生存时间(secs)", '此值确定需要重新握手之前会话记录单有效的时间。 默认值为3600。', '', '整数', '');

$_tipsdb['sslSessionTickets'] = new DAttrHelp("启用会话记录单", '使用OpenSSL的默认会话票证设置启用会话记录单。 服务器级别设置必须设置为“是”才能使虚拟主机设置生效。<br/>默认值:<br/><b>服务器级别:</b> Yes<br/><b>虚拟主机级别:</b> Yes', '', '布尔值', '');

$_tipsdb['sslStrictSni'] = new DAttrHelp("严格SNI证书", '指定是否严格要求专用的虚拟主机证书配置。启用后，没有专用证书配置的虚拟主机SSL连接将失败， 而不会使用默认的兜底证书。<br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>', '', '从单选框中选择', '');

$_tipsdb['sslStrongDhKey'] = new DAttrHelp("SSL强DH密钥", '指定是使用2048位还是1024位DH密钥进行SSL握手。 如果设置为“是”，则2048位DH密钥将用于2048位SSL密钥和证书。 在其他情况下，仍将使用1024位DH密钥。 默认值为“是”。 Java的早期版本不支持大于1024位的DH密钥大小。 如果需要Java客户端兼容性，则应将其设置为“否”。', '', '从单选按钮选择', '');

$_tipsdb['statDir'] = new DAttrHelp("统计输出目录", '实时统计报告文件将写入的目录。 默认目录是 <b>/tmp/lshttpd/</b> .', '在服务器操作期间，.rtreport文件将每秒写入一次。 为避免不必要的磁盘写入，请将其设置为RAM磁盘。<br/>.rtreport文件可以与第三方监控软件一起使用，以跟踪服务器的运行状况。', '路径', '');

$_tipsdb['staticReqPerSec'] = new DAttrHelp("静态请求/秒", '指定每秒可处理的来自单个IP的静态内容请求数量（无论与该IP之间建立了多少个连接）。<br/>当达到此限制时，所有后来的请求将被延滞到下一秒。 对于动态内容请求的限制与本限制无关。 每个客户端的请求限制可以在服务器或虚拟主机级别设置。 虚拟主机级别的设置将覆盖服务器级别的设置。', '[安全] 受信任的IP或子网不受影响。', '非负整数', '');

$_tipsdb['statuscode'] = new DAttrHelp("状态码", '指定外部重定向响应状态码。 如果状态码在300和399之间，可以指定&quot;目标URI&quot;。', '', '从下拉列表选择', '');

$_tipsdb['suexecGroup'] = new DAttrHelp("suEXEC组", '在当前Context级别，以该组身份运行。 必须设置虚拟主机级别的<b>suEXEC用户</b>或外部应用程序级别的<b>以用户身份运行</b>，<b>suEXEC组</b>才会生效。<br/><br/>可以使用<b>运行方式组</b>设置在外部应用程序级别覆盖此配置。<br/>默认值：<b>suEXEC用户</b>设置值', '', '有效组名或GID', '');

$_tipsdb['suexecUser'] = new DAttrHelp("suEXEC 用户", '在当前Context级别，以该用户身份运行。 如果设置了该项，则该值将覆盖虚拟主机级别<b>外部应用程序设置UID模式</b>的设置。<br/><br/>此配置可被外部应用程序级别的<b>以用户身份运行</b>设置覆盖。<br/><br/>默认值：未设置', '', '有效用户名或UID。', '');

$_tipsdb['suffix'] = new DAttrHelp("后缀", '指定将由此脚本处理程序处理的脚本文件后缀。后缀必须唯一。', '服务器会自动为列表中的第一个后缀添加特殊MIME类型（“application/x-httpd-[suffix]”）。 例如，会为后缀“php53”添加MIME类型“application/x-httpd-php53”。第一个之后的后缀需要在&quot;MIME设置&quot;设置中配置。<br/>尽管我们在此字段中列出了后缀，但是脚本处理程序使用MIME类型而非后缀来确定要处理的脚本。<br/> 仅指定确实需要的后缀。', '逗号分隔列表，禁止使用句点“.”字符。', '');

$_tipsdb['swappingDir'] = new DAttrHelp("交换目录", '指定交换文件的存放目录。 服务器在chroot模式启动时，该路径相对于新的根目录， 否则，它相对于真正的根目录。<br/><br/>LiteSpeed使用自己的虚拟内存 以降低系统的内存使用量。虚拟内存和磁盘交换会用来存储大的请求内容和 动态响应。交换目录应设置在有足够剩余空间的磁盘上。<br/><br/>默认值: /tmp/lshttpd/swap', '[性能建议] 将交换目录设置在一个单独的磁盘上，或者增加最大读写缓冲区大小以避免交换。', '路径', '');

$_tipsdb['templateFile'] = new DAttrHelp("模板文件", '指定此模板配置文件的路径。 该文件必须位于$SERVER_ROOT/conf/templates/下，且文件名扩展名为&quot;.conf&quot;。 如果指定的文件不存在，尝试保存模板后会显示包含&quot;点击创建&quot;链接的错误。 点击该链接将生成一个新的空模板文件。删除模板时，该条目会从配置中移除，但实际的模板配置文件不会被删除。', '', '路径', '');

$_tipsdb['templateName'] = new DAttrHelp("模板名称", '模板的唯一名称。', '', '文本', '');

$_tipsdb['templateVHAliases'] = new DAttrHelp("域名别名", '指定虚拟主机的备用名称。所有可能的主机名和IP地址都应添加到此列表中。名称中允许使用通配符<span class=&quot;lst-inline-token lst-inline-token--value&quot;>*</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>?</span>。对于不在端口80上的网站，请附加<span class=&quot;lst-inline-token lst-inline-token--value&quot;>:<port></span>。<br/><br/>别名将在以下情况下使用： <ol>   <li>处理请求时匹配Host标头中的主机名。</li>   <li>填充FrontPage或AWstats等附加组件的域名和别名配置。</li>  <li>基于虚拟主机模板配置侦听器到虚拟主机的映射。</li> </ol>', '', '以逗号分隔的域名列表。', '');

$_tipsdb['templateVHConfigFile'] = new DAttrHelp("实例化虚拟主机配置文件", '指定实例化成员虚拟主机时生成的配置文件位置。 路径中必须包含变量<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>，以便每个虚拟主机都有自己的文件。该文件必须位于 $SERVER_ROOT/conf/vhosts/下。只有在通过实例化将成员虚拟主机移出模板后，才会创建此配置文件。', '建议使用$VH_NAME/vhconf.conf，便于管理。', '包含$VH_NAME变量并以.conf为后缀的字符串', '');

$_tipsdb['templateVHDocRoot'] = new DAttrHelp("文档根目录", '指定每个成员虚拟主机的唯一文档根目录路径。 路径中必须包含变量<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT</span>，以便每个成员虚拟主机都有自己的文档根目录。', '', '包含$VH_NAME或$VH_ROOT变量的路径', '$VH_ROOT/public_html/或$SERVER_ROOT/$VH_NAME/public_html。');

$_tipsdb['templateVHDomain'] = new DAttrHelp("域名", '指定该成员虚拟主机的主域名。 如果留空，将使用虚拟主机名。该值应为完全限定域名，也可以使用IP地址。 对于不在端口80上的网站，建议附加<span class=&quot;lst-inline-token lst-inline-token--value&quot;>:<port></span>。 对于包含域名的配置，可以使用变量<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_DOMAIN</span>来引用此域。<br/><br/>该域名将在以下情况下使用： <ol>   <li>处理请求时匹配Host标头中的主机名。</li>   <li>填充FrontPage或AWstats等附加组件的域名配置。</li>  <li>基于虚拟主机模板配置侦听器到虚拟主机的映射。</li> </ol>', '', '域名', '');

$_tipsdb['templateVHName'] = new DAttrHelp("虚拟主机名", '此虚拟主机的唯一名称。该名称在所有模板成员虚拟主机和独立虚拟主机中不得重复。 在目录路径配置中，此名称可以由变量<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>引用。<br/><br/>如果还配置了同名的独立虚拟主机，则成员虚拟主机配置会被忽略。', '', '文本', '');

$_tipsdb['templateVHRoot'] = new DAttrHelp("默认虚拟主机根目录", '指定使用此模板的成员虚拟主机的默认根目录。 路径中必须包含变量<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>。这样每个成员模板都可以根据名称自动分配独立的根目录。', '', '路径', '');

$_tipsdb['throttleBlockWindow'] = new DAttrHelp("初始封锁时长（秒）", '指定客户端达到允许的最大登录失败次数后，初次被屏蔽的时长（秒）。 未设置时默认值：900秒。', '请设置足够长的时长来阻止重复攻击，同时避免让合法用户被锁定过久。', '整数', '');

$_tipsdb['throttleEnabled'] = new DAttrHelp("启用登录限制", '为WebAdmin控制台启用登录限制。启用后，重复的失败登录尝试会被跟踪并临时屏蔽， 以减少暴力破解密码攻击。如果只启用此选项，而其他登录限制设置保持未设置，则会使用内置默认值。', '[安全] 除非正在排查登录问题，否则请在生产环境中保持启用。 启用后如果未设置自定义值，默认值为失败5次、初始屏蔽900秒、最大屏蔽14400秒。', '从单选框中选择', '');

$_tipsdb['throttleMaxBackoff'] = new DAttrHelp("最大封锁时长（秒）", '指定登录失败持续发生且限制退避时间增加时的最大屏蔽时长（秒）。 未设置时默认值：14400秒。', '请设置足够强的上限来减缓自动化攻击，同时仍允许在合理时间内恢复访问。', '整数', '');

$_tipsdb['throttleMaxFailures'] = new DAttrHelp("最大登录失败次数", '指定客户端在被屏蔽前允许连续登录失败的次数。未设置时默认值：5。', '[安全] 较低的值会增强保护，但也可能更快屏蔽合法用户。', '整数', '');

$_tipsdb['toggleDebugLog'] = new DAttrHelp("切换调试日志", '切换调试日志会在<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>HIGH</span>之间切换 &quot;调试级别&quot;的值。 由于调试日志会影响性能并可能快速填满硬盘， 生产服务器上的&quot;调试级别&quot;通常应设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>。 此功能可用于快速开启或关闭调试日志，以便在生产服务器上排查问题。 通过此方式开启或关闭调试日志不会改变服务器配置中显示的任何内容。', '只有当&quot;日志级别&quot;设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>时， &quot;切换调试日志&quot;才会生效。<br/>[性能] 重要！调试日志包含每个请求和响应的详细信息。启用调试日志会严重降低服务性能， 并可能在很短时间内耗尽磁盘空间。此功能只应在诊断服务器问题时短时间使用。', '', '');

$_tipsdb['totalInMemCacheSize'] = new DAttrHelp("小文件缓存总大小 (bytes)", '指定分配用于缓存/服务小静态文件的总内存。', '', '非负整数', '');

$_tipsdb['totalMMapCacheSize'] = new DAttrHelp("总MMAP缓存大小 (bytes)", '指定分配用于缓存/服务中等大小静态文件的总内存。', '', '非负整数', '');

$_tipsdb['tp_vhFileName'] = new DAttrHelp("模板中使用的文件名", '指定基于模板的虚拟主机设置所使用的文件路径。 路径中必须包含变量<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_NAME</span>或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$VH_ROOT</span>，以便每个成员虚拟主机解析到自己的文件。 此设置用于日志文件等归模板所有的文件，而不是用于实例化虚拟主机配置文件。', '', '带有$VH_NAME或$VH_ROOT变量的路径', '');

$_tipsdb['umask'] = new DAttrHelp("umask", '设置CGI进程默认的umask。 通过<span class=&quot;lst-inline-token lst-inline-token--command&quot;> man 2 umask</span>命令了解详细信息。这也可作为外部应用程序&quot;umask&quot;的默认值。', '', '数值有效范围为[000] - [777]', '');

$_tipsdb['uploadPassByPath'] = new DAttrHelp("通过文件路径传递上传数据", '指定是否按文件路径传递上传数据。 如果启用，则上传时文件路径以及其他一些信息将发送到后端处理程序，而不是文件本身。 这样可以节省CPU资源和文件传输时间，但需要一些更新才能实现。 如果禁用，则文件内容将传输到后端处理程序，请求主体仍解析为文件。', '[性能] 如果向下兼容不是问题，启用此功能可加快文件上传处理速度。', '布尔值', '');

$_tipsdb['uploadTmpDir'] = new DAttrHelp("临时文件路径", '请求正文解析器工作期间，上传到服务器的文件会存储在此临时目录中。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>/tmp/lshttpd/</span>。', '', '绝对路径，或以$SERVER_ROOT开头的路径（服务器和虚拟主机级别），或以$VH_ROOT开头的路径（虚拟主机级别）。', '');

$_tipsdb['uploadTmpFilePermission'] = new DAttrHelp("临时文件权限", '确定临时目录中所存文件使用的文件权限。 服务器级别设置为全局设置，可在虚拟主机级别覆盖。', '', '3位八进制数。默认值为666。', '');

$_tipsdb['uri'] = new DAttrHelp("URI", '指定此上下文的URI。URI应以&quot;/&quot;开头。 如果URI以&quot;/&quot;结尾，则此上下文将包含该URI下的所有子URI。', '', 'URI', '');

$_tipsdb['useAIO'] = new DAttrHelp("使用异步 I/O (AIO)", '指定是否使用异步I/O服务静态文件，以及要使用的AIO实现。选项<span class=&quot;lst-inline-token lst-inline-token--value&quot;>LINUX AIO</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>io_uring</span>仅在Linux机器上可用。<br/>默认值：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>', '[性能建议] 对于I/O等待较高的服务器，AIO可帮助提升性能。<br/>[注意] 选择<span class=&quot;lst-inline-token lst-inline-token--value&quot;>io_uring</span>但当前机器不支持时，将改用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Linux AIO</span>。', '从列表中选择', '');

$_tipsdb['useIpInProxyHeader'] = new DAttrHelp("使用报头中的客户端IP", '指定是否将在HTTP请求报头中的X-Forwarded-For参数列出的IP地址，用于 所有的IP地址相关的功能，包括 连接/带宽限制、访问控制和IP地理定位。<br/><br/>如果你的Web服务器放置在负载均衡器或代理服务器之后，此功能非常有用。 如果您选择了“仅限受信任的IP”，只有在请求来自受信任IP时，X-Forwarded-For 中的IP才会被使用。受信任IP可在服务器级别的&quot;允许列表&quot;中定义。<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Keep Header from Trusted IP</span>与<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Trusted IP Only</span>相同，但是用于后端的X-Forwarded-For头不会被修改为包含连接对端地址。<br/><br/><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Use Last IP (for AWS ELB)</span>将使用“X-Forwarded-For”列表中的最后一个IP地址。如果您正在使用AWS Elastic Load Balancer，或期望真实IP被追加到“X-Forwarded-For”列表末尾，请选择此选项。', '', '从下拉列表选择', '');

$_tipsdb['useSendfile'] = new DAttrHelp("使用sendfile()", '指定是否使用sendfile()系统调用来服务静态文件。静态文件 可以用四种不同的方式服务：内存缓存、内存映射缓存、直接读写和 sendfile()。 尺寸小于&quot;最大小文件缓存(bytes)&quot;的文件将使用内存缓存服务。尺寸大于该限制、但小于 &quot;最大MMAP文件大小(bytes)&quot;的文件，将使用内存映射缓存服务。 尺寸大于&quot;最大MMAP文件大小(bytes)&quot;的文件将通过直接读写或sendfile() 服务。Sendfile()是一个“零拷贝”系统调用，可在服务非常大的 文件时大大减少CPU的使用率。Sendfile()需要一个优化的网卡内核驱动， 因此可能不适合某些小厂商的网络适配器。', '', '布尔值', '');

$_tipsdb['userDBCacheTimeout'] = new DAttrHelp("用户数据库缓存超时（秒）", '指定多久检查一次后端用户数据库变更。 在缓存中每个条目都有一个时间戳。 当缓存日期超过指定的超时时间时，将检查后端数据库是否有变化。 如果没有，时间戳将被重置为当前时间，否则会将新的数据载入。 服务器重载和平滑重启会立即清除缓存。', '[性能建议] 如果后端数据库不经常发生变更，设置较长的缓存时间来获得更好的性能。', '非负整数', '');

$_tipsdb['userDBLocation'] = new DAttrHelp("用户数据库地址", '指定用户数据库的位置。建议将数据库存储在$SERVER_ROOT/conf/vhosts/$VH_NAME/目录下。<br/><br/>对于类型为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Password File</span>的数据库，应设置为包含用户名/密码的展平文件的路径。 您可以在WebAdmin控制台中点击文件名来进行修改。<br/>用户文件的每一行包含一个用户名，后跟冒号和crypt()加密的密码，后面还可以跟一个冒号和用户所属的组名。多个组名用逗号分隔。<br/><br/>示例：<blockquote><code>john:HZ.U8kgjnMOHo:admin,user</code></blockquote>', '', '用户数据库文件路径。', '');

$_tipsdb['userDBMaxCacheSize'] = new DAttrHelp("用户数据库最大缓存大小", '指定用户数据库的最大缓存大小。 最近访问的用户认证信息会被缓存在内存中以提供最佳性能。', '[性能建议] 由于更大的缓存会消耗更多的内存，更高的值可能会也可能不会提供更好的性能。 请根据您的用户数据库大小和网站使用情况来设定一个合适的大小。', '非负整数', '');

$_tipsdb['uwsgiContext'] = new DAttrHelp("uWSGI上下文", 'uWSGI应用程序不能直接使用。必须将uWSGI应用程序配置为脚本处理程序，或通过uWSGI上下文映射到URL。 uWSGI上下文会将URI关联到uWSGI应用程序。', '', '', '');

$_tipsdb['uwsgiapp'] = new DAttrHelp("uWSGI App", '指定uWSGI应用程序的名称。此应用程序必须在服务器或虚拟主机级别的&quot;外部应用&quot;部分中定义。', '', '从下拉列表选择', '');

$_tipsdb['vaction'] = new DAttrHelp("操作 - 虚拟主机", '此字段显示用于禁用、启用或重启虚拟主机的按钮。 对一个虚拟主机执行的操作不会影响Web服务器的其他部分。', '更新虚拟主机内容时，临时禁用该虚拟主机是个好做法。', '', '');

$_tipsdb['vdisable'] = new DAttrHelp("禁用", '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Disable</span>操作会停止正在运行的虚拟主机。 新的请求将不再被接受，但正在处理的请求会照常完成。', '', '', '');

$_tipsdb['venable'] = new DAttrHelp("启用", '<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Enable</span>操作会启动已停止的虚拟主机。 这将允许其接受新请求。', '', '', '');

$_tipsdb['verifyDepth'] = new DAttrHelp("验证深度", ' 指定在判定客户端没有有效证书之前，应验证证书链的深度。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>1</span>。', '', '从下拉列表选择', '');

$_tipsdb['vhEnableBr'] = new DAttrHelp("Brotli压缩", '指定是否为此虚拟主机启用Brotli压缩。 这个设置仅在服务器级别的&quot;Brotli压缩等级（静态文件）&quot;设置为一个非零值时有效。', '', '布尔值', '');

$_tipsdb['vhEnableGzip'] = new DAttrHelp("启用压缩", '指定是否为此虚拟主机启用GZIP压缩。 这个设置仅在服务器级别的&quot;启用压缩&quot;设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>时有效。', '', '布尔值', '');

$_tipsdb['vhEnableQuic'] = new DAttrHelp("启用HTTP3/QUIC", '为此虚拟主机启用HTTP3/QUIC网络协议。 要使此设置生效，服务器级别的&quot;启用HTTP3/QUIC&quot;和监听器级别的&quot;打开HTTP/3/QUIC（UDP）端口&quot;也必须分别设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>。 默认值为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Yes</span>。', '当此设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>No</span>时，将不再发送HTTP3/QUIC公告。如果浏览器仍缓存HTTP3/QUIC信息，并且服务器级别和监听器级别仍启用HTTP3/QUIC，则在这些信息不再缓存或遇到HTTP3/QUIC协议错误之前，仍会继续使用HTTP3/QUIC连接。', '从单选框中选择', '');

$_tipsdb['vhMaxKeepAliveReq'] = new DAttrHelp("最大Keep-Alive请求数", '指定通过keep-alive(永久)连接服务的最大请求数量。当该限制值达到时连接将被断开。你可以为不同虚拟主机设置不同的数值。这个数值不能超过服务器级别的&quot;最大持续连接请求数&quot;限制值。', '[性能] 设置为一个合理的较高数值。设置为1或更小的值将禁用keep-alive连接。', '整数', '');

$_tipsdb['vhModuleUrlFilters'] = new DAttrHelp("虚拟主机模块上下文", '这是集中自定义虚拟主机上下文模块设置的位置。 上下文URI的设置会覆盖虚拟主机或服务器级别设置。', '', '', '');

$_tipsdb['vhModules'] = new DAttrHelp("虚拟主机模块", '虚拟主机模块配置数据默认从服务器模块配置继承。 虚拟主机模块仅限于HTTP级别Hook。', '', '', '');

$_tipsdb['vhName'] = new DAttrHelp("虚拟主机名", '为虚拟主机的唯一名称。建议使用虚拟主机的域名作为虚拟主机名。 虚拟主机名参数可以使用$VH_NAME变量来引用。', '', '文本', '');

$_tipsdb['vhRoot'] = new DAttrHelp("虚拟主机根", '指定虚拟主机的根目录。 注：这<b>不是</b>目录根。 建议将所有与该虚拟主机相关的文件 (像日志文件，html文件，CGI脚本等)都放置在这个目录下。 虚拟主机根目录可以使用变量$VH_ROOT来引用。', '[性能建议] 将不同的虚拟主机放在不同的硬盘驱动器上。', '路径', '');

$_tipsdb['vhaccessLog_fileName'] = new DAttrHelp("文件名", '访问日志文件名。', ' 将访问日志文件放在单独的磁盘上。', '文件名可以是绝对路径，也可以是相对于$SERVER_ROOT或$VH_ROOT的相对路径。', '');

$_tipsdb['vhadminEmails'] = new DAttrHelp("管理员邮箱", '指定这个虚拟主机管理员的电子邮箱地址。', '', '以逗号分隔的电子邮箱地址列表', '');

$_tipsdb['vhlog_fileName'] = new DAttrHelp("文件路径", '指定日志文件的路径。', ' 将日志文件放在单独的磁盘上。', '文件名可以是绝对路径，也可以是相对于$SERVER_ROOT或$VH_ROOT的相对路径。', '');

$_tipsdb['vhlog_logLevel'] = new DAttrHelp("日志级别", '指定日志记录级别。可用级别（从高到低）为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>ERROR</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>WARNING</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NOTICE</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>INFO</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>。 只有当消息等级高于或与当前设置相同时才被记录。 如果您希望将此设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>，您也需要设置服务器日志级别为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>。 调试的级别只能在服务器级别通过&quot;调试级别&quot;控制。', ' 除非&quot;调试级别&quot;设置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>NONE</span>以外的级别，否则<span class=&quot;lst-inline-token lst-inline-token--value&quot;>DEBUG</span>日志级别不会影响性能，建议使用。', '从列表中选择', '');

$_tipsdb['viewlog'] = new DAttrHelp("服务器日志查看器", '服务器日志查看器是浏览当前服务器日志以检查错误或问题的便捷工具。 日志查看器会按块搜索指定日志级别的服务器日志文件。   默认块大小为20KB。可以使用<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Begin</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>End</span>、 <span class=&quot;lst-inline-token lst-inline-token--value&quot;>Next</span>和<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Prev</span>按钮浏览较大的日志文件。', '动态生成页面的大小受&quot;最大动态响应主体大小(bytes)&quot;限制。 因此，如果块太大，页面可能会被截断。', '', '');

$_tipsdb['virtualHostMapping'] = new DAttrHelp("虚拟主机映射", '指定侦听器和虚拟主机之间的关系。 侦听器和虚拟主机通过域名关联。 HTTP请求会路由到具有匹配域名的虚拟主机。 一个侦听器可以映射到多个虚拟主机，以处理不同域名。 一个虚拟主机也可以从不同侦听器映射。 一个侦听器可以允许一个域名值为&quot;*&quot;的catchall虚拟主机。 如果侦听器的映射中没有明确匹配的域名， 侦听器会将请求转发到该catchall虚拟主机。', '[性能] 仅添加必要的映射。如果侦听器只映射到一个虚拟主机，则只需设置一个catchall映射&quot;*&quot;。', '', '');

$_tipsdb['virtualHostName'] = new DAttrHelp("虚拟主机", '指定虚拟主机的名称。', '', '从下拉列表选择', '');

$_tipsdb['vname'] = new DAttrHelp("名称 - 虚拟主机", '用于识别此虚拟主机的唯一名称。 这是设置此虚拟主机时指定的&quot;虚拟主机名&quot;。', '', '', '');

$_tipsdb['vreload'] = new DAttrHelp("重启 - 虚拟主机", 'Restart操作会使Web服务器加载此虚拟主机的最新配置。 正在处理的请求会使用旧配置完成。 新配置只会应用于新的请求。通过这种方式，可以即时应用虚拟主机的所有更改。', '', '', '');

$_tipsdb['vstatus'] = new DAttrHelp("状态 - 虚拟主机", '虚拟主机的当前状态。 状态可以是：<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Stopped</span>、<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Restart Required</span> 或<span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running - Removed from Configuration</span>。  <ul>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running</span>表示虚拟主机已加载并正在提供服务。</li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Stopped</span>表示虚拟主机已加载但未提供服务（已禁用）。</li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Restart Required</span>表示这是新添加的虚拟主机，服务器尚未加载其配置。</li>     <li><span class=&quot;lst-inline-token lst-inline-token--value&quot;>Running - Removed from Configuration</span>表示虚拟主机已从服务器配置中删除，但仍在提供服务。</li> </ul>', '', '', '');

$_tipsdb['wsaddr'] = new DAttrHelp("地址", 'WebSocket后端使用的唯一套接字地址。 支持IPv4套接字、IPv6套接字和Unix Domain Socket（UDS）。 IPv4和IPv6套接字可用于网络通信。 只有当WebSocket后端与服务器位于同一台机器上时，才能使用UDS。', ' 如果WebSocket后端在同一台机器上运行，建议优先使用UDS。如果必须使用IPv4或IPv6套接字， 请将IP地址设置为localhost或127.0.0.1，避免其他机器访问该WebSocket后端。<br/> Unix Domain Socket通常比IPv4或IPv6套接字提供更高性能。', 'IPv4/IPv6地址:端口、UDS://路径或unix:路径', '127.0.0.1:5434 <br/>UDS://tmp/lshttpd/php.sock<br/>unix:/tmp/lshttpd/php.sock');

$_tipsdb['wsgiBin'] = new DAttrHelp("WSGI路径", 'LiteSpeed Python Web服务器网关接口可执行文件（lswsgi）的路径。<br/><br/>该可执行文件通过使用LiteSpeed的WSGI LSAPI模块编译Python生成。', '', '绝对路径', '');

$_tipsdb['wsgiDefaults'] = new DAttrHelp("Python WSGI默认设置", 'Python WSGI应用程序的默认配置。这些设置可在Context级别覆盖。', '', '', '');

$_tipsdb['wsuri'] = new DAttrHelp("URI", '指定将使用此WebSocket后端的URI。仅当发往此URI的流量包含WebSocket升级请求时，才会转发到WebSocket后端。<br/><br/>不包含此升级请求的流量会自动转发到此URI所属的Context。如果此URI没有对应的Context，LSWS会将该流量视为访问位置为<span class=&quot;lst-inline-token lst-inline-token--value&quot;>$DOC_ROOT/URI</span>的静态Context。', '', '普通URI（以“/”开头）。如果URI以“/”结尾，则此WebSocket后端会包含该URI下的所有子URI。', '将WebSocket代理与Context结合使用，可以在同一页面上以不同方式处理不同类型的流量，从而优化性能。您可以将WebSocket流量发送到WebSocket后端，同时设置静态Context让LSWS提供页面的静态内容，或设置LSAPI Context让LSWS提供PHP内容（这两种内容由LSWS处理都比由WebSocket后端处理更高效）。');

$_tipsdb['EDTP:GroupDBLocation'] = array('建议将数据库存储在 $SERVER_ROOT/conf/vhosts/$VH_NAME/目录。');
$_tipsdb['EDTP:UDBgroup'] = array('如果在此处添加了组信息，则此信息将用于资源授权，并且涉及该用户的任何组数据库设置都将被忽略。','您可以使用逗号分隔多个组。空格字符将被视为组名的一部分。');
$_tipsdb['EDTP:accessControl_allow'] = array('可以在服务器、虚拟主机和上下文级别设置访问控制。如果服务器级别有访问控制，虚拟主机规则会在满足服务器规则后应用。','输入格式可以是IP（如192.168.0.2）、子网（如192.168.*）或子网/掩码（如192.168.128.5/255.255.128.0）。','如果有受信任IP或子网，必须在允许列表中通过追加“T”指定，例如192.168.1.*T。受信任IP或子网不受连接/限速限制。');
$_tipsdb['EDTP:accessControl_deny'] = array('要拒绝某个地址访问，请在允许列表中填写“ALL”，并在拒绝列表中填写子网或IP。要只允许特定IP或子网访问站点，请在拒绝列表中填写“ALL”，并在允许列表中指定地址。');
$_tipsdb['EDTP:accessDenyDir'] = array('如果要拒绝对特定目录的访问，请输入完整路径。 输入后跟*的路径将禁用所有子目录。','路径可以是绝对路径，也可以相对于$SERVER_ROOT，使用逗号分隔。','如果同时启用了<b>跟随符号链接</b>和<b>检查符号链接</b>，则会根据拒绝访问的目录检查符号链接。');
$_tipsdb['EDTP:accessLog_fileName'] = array('日志文件路径可以是绝对路径，也可以是相对于$SERVER_ROOT的相对路径。');
$_tipsdb['EDTP:aclogUseServer'] = array('如果需要，您可以禁用此虚拟主机的访问日志记录以降低磁盘I/O的占用。');
$_tipsdb['EDTP:adminEmails'] = array('您可以输入多个管理员电子邮件：使用逗号分隔。');
$_tipsdb['EDTP:adminOldPass'] = array('出于安全原因，如果忘记管理员密码，将无法从WebAdmin控制台进行更改。请改用以下shell命令： <br><br> /usr/local/lsws/admin/misc/admpass.sh. <br><br> 该脚本将删除所有输入的管理员用户ID，并用一个管理员用户覆盖它们。');
$_tipsdb['EDTP:allowBrowse'] = array('静态上下文可将URI映射到文档根目录内外的目录。目录可以是绝对路径，也可以相对于文档根目录（默认）、$VH_ROOT或$SERVER_ROOT。','勾选&quot;可访问&quot;将允许浏览此上下文中的静态文件。为防止查看静态文件（例如更新内容时），可以禁用此项。');
$_tipsdb['EDTP:appType'] = array('','');
$_tipsdb['EDTP:as_location'] = array('App Server上下文用于简化Rack/Rails、WSGI或Node.js应用程序的运行配置。 只需在&quot;位置&quot;字段中指定应用程序根位置。');
$_tipsdb['EDTP:as_startupfile'] = array('','');
$_tipsdb['EDTP:autoFix503'] = array('启用<b>自动修复503错误</b>后，如果检测到崩溃，监控进程将自动启动新的服务器进程， 服务会立即恢复。');
$_tipsdb['EDTP:backlog'] = array('本地应用可以由Web服务器启动。在这种情况下，需要指定路径、待处理队列长度和实例数。');
$_tipsdb['EDTP:binPath'] = array('','');
$_tipsdb['EDTP:bubbleWrap'] = array('');
$_tipsdb['EDTP:bubbleWrapCmd'] = array('');
$_tipsdb['EDTP:cgi_path'] = array('CGI上下文可用于指定仅包含CGI脚本的目录。路径可以是绝对路径，也可以相对于$SERVER_ROOT、$VH_ROOT或$DOC_ROOT（默认）。 对于cgi-bin目录，路径和URI必须以&quot;/&quot;结尾。','如果目录中只需要某个特定脚本，建议仅为该脚本创建CGI上下文。此时路径和URI不必是目录。 例如，路径可以是~/myapp/myscript.pl，URI可以是/myapp/myscript.pl。其他所有文件都不会作为CGI提供。');
$_tipsdb['EDTP:checkSymbolLink'] = array('检查符号链接仅在跟随符号链接启用后才会生效。 这将控制是否根据禁止访问的目录检查符号链接。');
$_tipsdb['EDTP:compressibleTypes'] = array('压缩类型是可压缩MIME类型的逗号分隔列表。MIME类型可以使用通配符“*”，例如*/*、text/*。 可以在前面加“!”来排除某些类型。使用“!”时列表顺序很重要。例如“text/*, !text/css, !text/js” 会压缩除css和js之外的所有文本文件。');
$_tipsdb['EDTP:ctxType'] = array('<b>静态</b>上下文可将URI映射到文档根目录内外的目录。','<b>LiteSpeed SAPI</b>上下文可将URI关联到LSAPI应用程序。','<b>代理</b>上下文可让此虚拟主机作为外部Web服务器或应用服务器的透明反向代理服务器。','<b>Fast CGI</b>上下文是Fast CGI应用程序的挂载点。','<b>SCGI</b>上下文可将URI关联到SCGI应用程序。','<b>CGI</b>上下文可指定仅包含CGI脚本的目录。','<b>重定向</b>上下文可设置内部或外部重定向URI。','<b>负载均衡</b>上下文可为该上下文分配不同的集群。','<b>Java Web应用</b>上下文用于从兼容AJPv13的Java Servlet引擎自动导入预定义Java应用程序。','<b>Servlet</b>上下文用于导入Web应用程序下的特定Servlet。','<b>App Server</b>上下文专用于Rack/Rails、WSGI和Node.js应用程序。','<b>uWSGI</b>上下文可将URI关联到uWSGI应用程序。','<b>模块处理程序</b>上下文是处理程序类型模块的挂载点。');
$_tipsdb['EDTP:docRoot'] = array('如果文档根目录尚不存在，服务器不会自动创建它。  请确保该目录存在并且由正确的用户拥有。','在此处设置文档根目录，它可以是绝对路径，也可以相对于$SERV_ROOT或$VH_ROOT','在此虚拟主机中，文档根目录称为$DOC_ROOT，可在其他路径配置中使用。');
$_tipsdb['EDTP:domainName'] = array('输入希望此侦听器响应的所有域名。使用逗号&quot;,&quot;分隔各个域名。','可以只选择一个虚拟主机来处理所有未指定的域名，并在域名中填入&quot;*&quot;。');
$_tipsdb['EDTP:enableDynGzipCompress'] = array('动态压缩仅在启用压缩后才有效。');
$_tipsdb['EDTP:enableExpires'] = array('可以在服务器/虚拟主机/上下文级别设置过期。较低级别的设置将覆盖较高级别的设置。在覆盖优先级方面： <br><br> 上下文级别 > 虚拟主机级别 > 服务器级别 <br><br>');
$_tipsdb['EDTP:enableRecaptcha'] = array('当服务器级别将此设置设为<b>Yes</b>时，仍可在虚拟主机级别禁用CAPTCHA保护。');
$_tipsdb['EDTP:errURL'] = array('您可以为不同的错误代码设置自定义错误页面。');
$_tipsdb['EDTP:expiresByType'] = array('按类型过期将覆盖默认设置。 每个条目的格式均为“MIME-type=A|Mseconds”， 两者之间没有空格。 您可以输入多个以逗号分隔的条目。');
$_tipsdb['EDTP:expiresDefault'] = array('过期语法，“A|Mseconds”表示在基准时间（A或M）加上指定的时间（以秒为单位）之后，文件将 到期。 “ A”表示客户端访问时间，“ M”表示文件修改时间。 您可以使用其他MIME类型覆盖此默认设置:A86400表示文件将根据客户端访问时间在1天后过期。','以下是一些常见的数字：1小时= 3600秒，1天= 86400秒，1周= 604800秒， 1个月= 2592000秒，1年= 31536000秒。');
$_tipsdb['EDTP:extAppAddress'] = array('地址可以是IPv4套接字地址 &quot;IP:PORT&quot;, 像192.168.1.3:7777 和localhost:7777 或者 Unix域套接字 地址 &quot;UDS://path&quot; 例如 UDS://tmp/lshttpd/myfcgi.sock.','UDS是在chroot环境下进行的。','对于本地应用程序，由于安全性和更好的性能，Unix域套接字是首选。 如果你必须使用IPv4套接字，将IP部分设置为localhost或127.0.0.1， 这样其他机器就无法访问应用程序。');
$_tipsdb['EDTP:extAppName'] = array('使用易记的名称，其他位置会通过此名称引用该应用。');
$_tipsdb['EDTP:extAppType'] = array('可以设置外部Fast CGI应用程序和兼容AJPv13（Apache JServ Protocol v1.3）的Servlet引擎。');
$_tipsdb['EDTP:extWorkers'] = array('必须先定义负载均衡worker。','可用的外部应用类型包括lsapi（LSAPI App）、proxy（Web Server (Proxy)）、fcgi（Fast CGI App）、scgi（SCGI App）、servlet（Servlet/JSP Engine）和uwsgi（uWSGI）。','一个负载均衡集群中可以混合不同类型的外部应用程序。');
$_tipsdb['EDTP:externalredirect'] = array('在此处设置重定向URI。 如果是外部重定向，则可以指定状态代码。 内部重定向必须以&quot;/&quot;开头，外部重定向可以以&quot;/&quot;或&quot;http(s)://&quot;开头。');
$_tipsdb['EDTP:extraHeaders'] = array('标头控制设置向后兼容旧的&#039;header_name: value1,value2,...&#039;语法，该语法等同于使用&#039;Header&#039;指令设置标头和值。');
$_tipsdb['EDTP:fcgiapp'] = array('Fast CGI上下文是Fast CGI应用程序的挂载点。Fast CGI应用程序必须在服务器级别或虚拟主机级别预先定义。');
$_tipsdb['EDTP:followSymbolLink'] = array('如果启用了跟随符号链接，您仍然可以在虚拟主机级别禁用它。');
$_tipsdb['EDTP:gdb_groupname'] = array('组名应仅包含字母和数字。');
$_tipsdb['EDTP:gzipCompressLevel'] = array('压缩动态内容的级别，范围从1(最低)到9(最高)。');
$_tipsdb['EDTP:hardLimit'] = array('设置来自一个合理的的并发连接限制有助于抵御DoS攻击。');
$_tipsdb['EDTP:indexUseServer'] = array('您可以将默认服务器级别设置用于索引文件，也可以使用自己的服务器级别设置。','除了服务器级别的设置之外，您还可以使用其他设置。','您可以通过选择不使用服务器级别设置并将虚拟主机级别设置留为空白来禁用索引文件。','您可以在Context级别启用/禁用“自动索引”。');
$_tipsdb['EDTP:javaServletEngine'] = array('如果Servlet Engine在其他计算机上运行，建议将webapps目录复制到本地。 否则，您必须将文件放在可访问的公用网络驱动器中，这可能会影响性能。');
$_tipsdb['EDTP:javaWebApp_location'] = array('Java Web应用上下文用于从兼容AJPv13的Java Servlet引擎自动导入预定义Java应用程序。 Servlet引擎应在服务器级别或虚拟主机级别的外部应用部分中设置。','位置是包含Web应用程序文件的目录，其中包括WEB-INF/子目录。','Web服务器会自动导入Web应用程序的配置文件，通常是&quot;位置&quot;指定目录下的WEB-INF/web.xml。');
$_tipsdb['EDTP:listenerIP'] = array('从列表中选择一个IP地址，如果您未指定特定地址，则系统将绑定到该计算机上的所有可用IP地址。');
$_tipsdb['EDTP:listenerName'] = array('为侦听器指定一个易于理解和记忆的名称。');
$_tipsdb['EDTP:listenerPort'] = array('在此IP上为该侦听器输入一个唯一的端口号。只有超级用户（root）可以使用低于1024的端口。端口80是默认HTTP端口；端口443是默认HTTPS端口。');
$_tipsdb['EDTP:listenerSecure'] = array('为<b>安全</b>选择“是”会使此侦听器使用HTTPS。之后，必须在SSL设置中继续配置。');
$_tipsdb['EDTP:logUseServer'] = array('如果<b>使用服务器日志</b>设置为“是”，日志将写入服务器级别设置的服务器日志文件。');
$_tipsdb['EDTP:log_enableStderrLog'] = array('stderr日志与服务器日志位于同一目录中。如果启用，所有外部应用程序输出到stderr的内容都会记录在此文件中。');
$_tipsdb['EDTP:log_fileName'] = array('日志文件路径可以是绝对路径，也可以是相对于$SERVER_ROOT的相对路径。');
$_tipsdb['EDTP:log_rollingSize'] = array('如果当前日志文件超过轮转大小，将创建一个新的日志文件。文件大小以字节为单位，可使用多种输入格式：10240、10K或1M。');
$_tipsdb['EDTP:maxCGIInstances'] = array('限制CGI程序可以使用的资源,有助于抵御DoS攻击。','“最大CGI实例数”控制Web服务器可以启动多少个CGI进程。');
$_tipsdb['EDTP:maxReqHeaderSize'] = array('数值可以写成10240或10K。');
$_tipsdb['EDTP:mime'] = array('可以从上一页编辑MIME设置。 您可以指定mime配置文件的位置 可以是绝对路径，也可以是相对于$SERVER_ROOT的路径。');
$_tipsdb['EDTP:namespace'] = array('');
$_tipsdb['EDTP:namespaceConf'] = array('');
$_tipsdb['EDTP:nodeBin'] = array('');
$_tipsdb['EDTP:phpIniOverride'] = array('');
$_tipsdb['EDTP:procSoftLimit'] = array('进程软/硬限制用于控制一个用户允许拥有多少个进程。这包括CGI应用生成的所有进程。如果未设置，将使用操作系统级别的限制。','设置为0或留空时，所有软/硬限制都将使用操作系统默认值。','软限制是内核对相应资源强制执行的值。硬限制是软限制可提升到的上限。');
$_tipsdb['EDTP:proxyWebServer'] = array('代理上下文使此虚拟主机可作为外部Web服务器或应用程序服务器的透明反向代理服务器。','外部Web服务器必须在服务器级别或虚拟主机级别的外部应用中预先定义。');
$_tipsdb['EDTP:realm'] = array('上下文可以通过虚拟主机安全部分中设置的预定义授权域保护。也可以指定替代名称和额外要求。');
$_tipsdb['EDTP:recaptchaAllowedRobotHits'] = array('');
$_tipsdb['EDTP:recaptchaBotWhiteList'] = array('');
$_tipsdb['EDTP:recaptchaMaxTries'] = array('');
$_tipsdb['EDTP:recaptchaRegConnLimit'] = array('');
$_tipsdb['EDTP:recaptchaSecretKey'] = array('');
$_tipsdb['EDTP:recaptchaSiteKey'] = array('如果服务器管理多个域，服务器级别的站点/密钥对必须配置为跳过域名检查。否则，CAPTCHA验证将无法正常工作。');
$_tipsdb['EDTP:recaptchaSslConnLimit'] = array('');
$_tipsdb['EDTP:recaptchaType'] = array('');
$_tipsdb['EDTP:recaptchaVhReqLimit'] = array('');
$_tipsdb['EDTP:restrained'] = array('在共享主机环境中启用访问管制。');
$_tipsdb['EDTP:reusePort'] = array('');
$_tipsdb['EDTP:rewriteMapLocation'] = array('输入位置URI。URI必须以&quot;/&quot;开头。');
$_tipsdb['EDTP:rewriteRules'] = array('这里应使用虚拟主机级别的重写规则，例如在Apache虚拟主机配置文件中找到的规则。 请勿在此处添加任何文档根级别的重写规则。 如果有来自.htaccess的文档根级别重写规则，请改为创建URI为&quot;/&quot;的静态Context， 并在那里添加这些重写规则。');
$_tipsdb['EDTP:rubyBin'] = array('<b>Ruby路径</b>是Ruby可执行文件的绝对路径。例如：/usr/local/bin/ruby。');
$_tipsdb['EDTP:scgiapp'] = array('SCGI上下文是SCGI应用程序的挂载点。SCGI应用程序必须在服务器级别或虚拟主机级别预先定义。');
$_tipsdb['EDTP:serverName'] = array('服务器进程的用户和组设置无法修改。 这是在安装过程中设置的。 您必须重新安装才能更改此选项。');
$_tipsdb['EDTP:servletEngine'] = array('如果Servlet Engine在其他计算机上运行，建议将webapps目录复制到本地。 否则，您必须将文件放在可访问的公用网络驱动器中，这可能会影响性能。');
$_tipsdb['EDTP:shHandlerName'] = array('除CGI和Module Handler外，其他处理器类型都需要先在“外部应用”部分中预定义。');
$_tipsdb['EDTP:sndBufSize'] = array('数值可以为10240、10K或1M。','如果发送/接收缓冲区大小设置为0，则将使用操作系统默认的TCP缓冲区大小。');
$_tipsdb['EDTP:softLimit'] = array('在此处设置IP级别的速率限制。 数值将四舍五入至4K单位。 设置为“ 0”以禁用宽带限制。','只要没有超过硬限制,连接数就可以在宽限期内暂时超过软限制,超过宽限时间后,如果连接数仍然超过软限制，相应的IP将被封锁屏蔽时长设置的时间');
$_tipsdb['EDTP:sslSessionCache'] = array('会话缓存使客户端可以在设置的时间内恢复会话，而不必重新执行SSL握手。您可以使用<b>启用会话缓存</b>为客户端分配会话ID，或者通过创建和使用会话票证来做到这一点。');
$_tipsdb['EDTP:sslSessionTicketKeyFile'] = array('如果会话票证由服务器生成，票证会自动轮换。如果使用 <b>SSL会话票证密钥文件</b>选项自行创建和管理会话票证，则必须通过cron任务自行轮换票证。');
$_tipsdb['EDTP:swappingDir'] = array('建议将交换目录放置在本地磁盘上，例如/tmp。 应不惜一切代价避免网络驱动器。 交换将在配置的内存I/O缓冲区耗尽时进行。');
$_tipsdb['EDTP:userDBLocation'] = array('建议将数据库存储在 $SERVER_ROOT/conf/vhosts/$VH_NAME/目录下。');
$_tipsdb['EDTP:uwsgiapp'] = array('uWSGI上下文是uWSGI应用程序的挂载点。uWSGI应用程序必须在服务器级别或虚拟主机级别预先定义。');
$_tipsdb['EDTP:vhRoot'] = array('所有目录必须预先存在。这个设置不会为你创建目录。如果你要创建一个新的虚拟主机， 你可以创建一个空的根目录，并从头开始设置； 也可以将软件包中附带的DEFAULT虚拟根目录复制到这个虚拟主机根目录中， 并修改为对应用户所有。','虚拟主机根目录($VH_ROOT)可以是绝对路径,也可以是相对于$SERVER_ROOT的相对路径.');
$_tipsdb['EDTP:vhaccessLog_fileName'] = array('日志文件路径可以是绝对路径，也可以是相对于$SERVER_ROOT或$VH_ROOT的相对路径。');
$_tipsdb['EDTP:vhadminEmails'] = array('您可以输入多个管理员电子邮件，以逗号分隔。');
$_tipsdb['EDTP:vhlog_fileName'] = array('日志文件路径可以是绝对路径，也可以是相对于$SERVER_ROOT或$VH_ROOT的相对路径。','如果要将日志级别设置为DEBUG，则还必须将服务器日志级别也设置为DEBUG。 调试级别由服务器调试级别控制。 请仅在必要时才使用DEBUG级别，因为它会对服务器性能产生很大影响，并且可以快速填满磁盘空间。');
$_tipsdb['EDTP:virtualHostName'] = array('选择要映射到此侦听器的虚拟主机。','如果尚未设置要映射的虚拟主机，可以跳过此步骤，稍后再回来完成。');
$_tipsdb['EDTP:wsgiBin'] = array('');
