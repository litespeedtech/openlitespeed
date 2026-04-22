window.HDOC_SEARCH_INDEX = [
    {
        "title": "主页",
        "url": "index.html",
        "text": "主页 OpenLiteSpeed Web Server 1.9 用户手册 — Rev. 3 目录 许可协议 简介 安装/卸载 管理 安全 配置 有关更多信息，请访问我们的 OpenLiteSpeed知识库"
    },
    {
        "title": "许可协议",
        "url": "license.html",
        "text": "许可协议 GNU GENERAL PUBLIC LICENSE v3 GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 Copyright (C) 2007 Free Software Foundation, Inc. Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed. Preamble The GNU General Public License is a free, copyleft license for software and other kinds of works. The licenses for most software and other practical..."
    },
    {
        "title": "简介",
        "url": "intro.html",
        "text": "简介 简介 OpenLiteSpeed（OLS）是一款高性能、开源的Web服务器。它是LiteSpeed Web Server Enterprise的免费替代方案，并兼容Apache重写规则。 OLS采用事件驱动架构，并内置缓存引擎（LSCache）。 概述 OpenLiteSpeed支持包括HTTP/2和HTTP/3（QUIC）在内的现代协议。它适用于独立服务器、开发环境和生产环境，尤其适合单站点部署或偏好�..."
    },
    {
        "title": "安装",
        "url": "install.html",
        "text": "安装 安装/卸载 最低系统要求 操作系统: Linux(i386): kernel 2.4或更高, glibc-2.2 或更高 CentOS: 5 或更高 Ubuntu: 8.04 或更高 Debian: 4 或更高 FreeBSD(i386): 4.5 and up MacOSX: 10.3 and up CPU: Intel: 80486 或更高 PowerPC: PowerPC G4 内存: 32MB 或更高 硬盘: 安装时: 200MB 运行时: 300MB或更高, 交换空间取决于使用情况。 安装 安装必须在命令行终端中执..."
    },
    {
        "title": "管理",
        "url": "admin.html",
        "text": "管理 管理 可以通过三种方式控制LiteSpeed Web服务器：通过WebAdmin控制台，从命令行或通过发送信号进行控制. 通过WebAdmin控制台: WebAdmin控制台是一个集中式控制面板，用于控制和配置所有LiteSpeed Web服务器设置. 登录到WebAdmin控制台 (默认: http://[您的服务器IP]:7080/). 选择“服务管理”,您将找到一个包含执行常见任务的控�..."
    },
    {
        "title": "安全",
        "url": "security.html",
        "text": "安全 安全 LiteSpeed Web Server在设计安全性时是首要考虑因素。 LSWS支持SSL，具有服务器和虚拟主机级别的访问控制， 以及特定于context的领域保护。 除了这些标准功能， LSWS还具有以下特殊安全功能: 连接级别限制: IP级限制限制了与单个IP地址之间的网络带宽，而与连接数无关。 IP级别的连接限制了与单个IP地址的并发连..."
    },
    {
        "title": "配置",
        "url": "config.html",
        "text": "配置 配置 概念 在配置之前，您需要了解以下一些基本概念. 虚拟主机 LiteSpeed Web服务器可以通过一个服务器托管多个网站（虚拟主机）. 传统上，虚拟主机分为两种：基于IP的虚拟主机和 基于域的虚拟主机。 基于IP的虚拟主机是具有自己唯一IP地址的网站. 基于域的虚拟主机是通过域名区分的， 因此可以与其他网站共�..."
    },
    {
        "title": "Web控制台",
        "url": "webconsole.html",
        "text": "Web控制台 Web控制台 Web控制台可以控制一些Web控制台的设置. 其中包括: 会话超时 日志设定 访问控制 创建/删除管理员用户 重置管理员密码 WebAdmin侦听器和SSL设置"
    },
    {
        "title": "编译PHP",
        "url": "CompilePHP_Help.html",
        "text": "编译PHP 额外的PATH环境变量 将其他用于构建脚本的路径附加到当前PATH环境变量中。 安装路径前缀 设置“--prefix”配置选项的值。默认安装位置在LiteSpeed Web Server的安装目录下。 LiteSpeed Web Server可以同时使用多个PHP版本。如果要安装多个版本， 应为它们设置不同的前缀。 编译器标志 添加其他编译器标志，例如优化的�..."
    },
    {
        "title": "服务器常规",
        "url": "ServGeneral_Help.html",
        "text": "服务器常规 整个服务器的常规设置。 当设置中需要路径信息时，它可以是绝对路径，也可以相对于$SERVER_ROOT。 $SERVER_ROOT是LiteSpeed Web服务器的安装位置（例如，your_home_dir/lsws或/opt/lsws）。 服务器可执行文件位于$SERVER_ROOT/bin下。 服务器名称 此服务器的唯一名称。如果留空，默认使用服务器主机名。 Worker数量 指定httpd..."
    },
    {
        "title": "服务器日志",
        "url": "ServLog_Help.html",
        "text": "服务器日志 文件名 指定日志文件的路径。 [性能建议] 将日志文件放置在一个单独的磁盘上。 日志级别 指定日志文件中记录的日志级别。 可用级别（由高到低）为: ERROR{/}、 WARNING{/}、 NOTICE{/}、 INFO{/}和 DEBUG{/}。 只有级别与当前设置相同或更高的消息将被记录（级别越低记录越详细）。 [性能] 使用 DEBUG{/}日志级别本�..."
    },
    {
        "title": "服务器调优",
        "url": "ServTuning_Help.html",
        "text": "服务器调优 默认SHM目录 将共享内存的默认目录更改为指定的路径。 如果该目录不存在，则将创建该目录。除非另有说明，否则所有SHM数据都将存储在此目录中。 PROXY协议 使用PROXY协议与此服务器通信的前端代理IP/子网列表。设置后，服务器会对来自所列IP/子网的传入连接使用PROXY协议； 如果PROXY协议不可用，则回退�..."
    },
    {
        "title": "服务器安全",
        "url": "ServSecurity_Help.html",
        "text": "服务器安全 跟随符号链接 指定服务静态文件时跟踪符号链接的服务器级别默认设置。 选项有 Yes{/}、 If Owner Match{/}和 No{/}。 Yes{/}设置服务器始终跟踪符号链接。 If Owner Match{/}设置服务器只有在链接和目标属主一致时才跟踪符号链接。 No{/}表示服务器永远不会跟踪符号链接。 该设置可以在虚拟主机配置中覆盖，但不能..."
    },
    {
        "title": "外部应用",
        "url": "ExtApp_Help.html",
        "text": "外部应用 LiteSpeed网络服务器可以转发外部应用程序的请求，以处理和生成动态内容。 从2.0版本起, LiteSpeed Web Server已经支持了7种外部应用: CGI, FastCGI, web server, servlet engine, LiteSpeed SAPI application, load balancer, and piped logger. CGI是Common Gateway Interface的缩写. 目前的标准是 CGI/1.1. CGI应用程序在独立的进程中运行. 一个CGI进程将为..."
    },
    {
        "title": "脚本处理程序",
        "url": "ScriptHandler_Help.html",
        "text": "脚本处理程序 LiteSpeed Web Server支持所有脚本语言，包括Perl、PHP、Ruby、Python、Java等。使用这些不同语言编写的脚本必须交给相应的外部应用程序处理。 LiteSpeed Web Server使用脚本处理程序来决定脚本应交给哪个外部应用程序。这些脚本处理程序通过文件后缀将文件映射到外部应用程序。（实际上，后缀用于确定MIME 类型�..."
    },
    {
        "title": "应用服务器设置",
        "url": "App_Server_Help.html",
        "text": "应用服务器设置 Rack/Rails 默认设置 Rack/Rails应用程序的默认配置。这些设置可在Context级别覆盖。 Ruby路径 Ruby可执行文件的路径。通常为/usr/bin/ruby 或/usr/local/bin/ruby，具体取决于Ruby的安装位置。 运行模式 指定应用程序的运行模式：\"Development\"（开发）、 \"Production\"（生产）或\"Staging\"（预发布）。默认值为\"Production\"（生产..."
    },
    {
        "title": "模块配置",
        "url": "Module_Help.html",
        "text": "模块配置 OpenLiteSpeed 1.3及LSWS Enterprise 5.0或更高版本提供模块支持。 所有所需模块都必须在“服务器模块配置”标签下注册。模块文件必须位于服务器root/modules目录中，才能用于注册。启动时，服务器会加载所有已注册的模块。注册新模块后必须重新启动服务器。 模块可在“服务器模块配置”标签下进行全局配置。随..."
    },
    {
        "title": "侦听器常规",
        "url": "Listeners_General_Help.html",
        "text": "侦听器常规 侦听器名称 此侦听器的唯一名称。 IP地址 指定此侦听器的IP地址。所有可用IP地址都会列出。IPv6地址会包含在\"[ ]\"中。 要侦听所有IPv4地址，请选择 ANY{/}。要侦听所有IPv4和IPv6地址，请选择 [ANY]{/}. 为了同时为IPv4和IPv6客户端提供服务，应使用IPv4映射的IPv6地址代替纯IPv4地址。 IPv4映射的IPv6地址应写为[::FFFF:..."
    },
    {
        "title": "侦听器SSL",
        "url": "Listeners_SSL_Help.html",
        "text": "侦听器SSL SSL私钥和证书 每个SSL侦听器都需要成对的SSL私钥和SSL证书。 多个SSL侦听器可以共享相同的密钥和证书。 您可以使用SSL软件包自行生成SSL私钥， 例如OpenSSL。SSL证书也可以从授权证书颁发机构（如VeriSign或Thawte）购买。您也可以自己签署证书。 自签名证书将不受Web浏览器信任，并且不应在公共网站上使用。但..."
    },
    {
        "title": "虚拟主机模板",
        "url": "Templates_Help.html",
        "text": "虚拟主机模板 虚拟主机模板可以轻松创建许多配置相似的新虚拟主机。 每个模板都包含一个模板配置文件、一个映射侦听器列表以及一个成员虚拟主机列表。 要添加基于模板的虚拟主机，管理员只需向模板成员列表添加一个具有唯一虚拟主机名和合格域名的成员。 模板映射侦听器列表中的所有侦听器会自动添加侦�..."
    },
    {
        "title": "虚拟主机注册",
        "url": "VirtualHosts_Help.html",
        "text": "虚拟主机注册 此页面列出所有已定义的虚拟主机。你可以在这里添加/删除虚拟主机，或修改现有虚拟主机。 添加虚拟主机前，请确保虚拟主机根目录已经存在。 虚拟主机名 为虚拟主机的唯一名称。建议使用虚拟主机的域名作为虚拟主机名。 虚拟主机名参数可以使用$VH_NAME变量来引用。 虚拟主机根 指定虚拟主机的�..."
    },
    {
        "title": "虚拟主机常规",
        "url": "VHGeneral_Help.html",
        "text": "虚拟主机常规 文档根目录 指定此虚拟主机的文档根目录。 推荐使用 $VH_ROOT/html{/}。在Context中，此目录可以用$DOC_ROOT来引用。 管理员邮箱 指定这个虚拟主机管理员的电子邮箱地址。 启用压缩 指定是否为此虚拟主机启用GZIP压缩。 这个设置仅在服务器级别的 设置为 Yes{/}时有效。 Brotli压缩 指定是否为此虚拟主机启用Br..."
    },
    {
        "title": "虚拟主机安全",
        "url": "VHSecurity_Help.html",
        "text": "虚拟主机安全 CAPTCHA 保护 CAPTCHA保护是一种用于减轻服务器负载的服务。当下列情况之一发生后，CAPTCHA保护将被激活。激活后，所有非信任客户端（按配置）发出的请求都会被重定向到CAPTCHA验证页面。验证完成后，客户端会被重定向到目标页面。 下列情况将启用CAPTCHA保护： 1. 服务器或虚拟主机并发请求计数超过连�..."
    },
    {
        "title": "虚拟主机SSL",
        "url": "VHSSL_Help.html",
        "text": "虚拟主机SSL SSL私钥和证书 每个SSL侦听器都需要成对的SSL私钥和SSL证书。 多个SSL侦听器可以共享相同的密钥和证书。 您可以使用SSL软件包自行生成SSL私钥， 例如OpenSSL。SSL证书也可以从授权证书颁发机构（如VeriSign或Thawte）购买。您也可以自己签署证书。 自签名证书将不受Web浏览器信任，并且不应在公共网站上使用。..."
    },
    {
        "title": "重写",
        "url": "Rewrite_Help.html",
        "text": "重写 启用重写 指定是否启用LiteSpeed的URL重写引擎。 此选项可在虚拟主机或Context级别自定义， 并会沿目录树继承，直到被显式覆盖。 自动加载.htaccess 如果使用rewritefile指令的目录还没有对应的HttpContext，则在首次访问该目录时自动加载.htaccess文件中包含的重写规则。首次加载后，必须执行平滑重启才能使对该.htaccess�..."
    },
    {
        "title": "上下文",
        "url": "Context_Help.html",
        "text": "上下文 在LiteSpeed Web Server术语中，\"上下文\"是一个虚拟位置，即用于标识一组资源的公共父URL。 上下文可以看作网站目录树中的不同目录。例如，\"/\"是映射到网站文档根目录的根上下文； \"/cgi-bin/\"是目录树上的另一个上下文，专用于此站点的CGI应用程序。 可以在LiteSpeed的WebAdmin控制台中显式定义上下文，用于以下目的..."
    },
    {
        "title": "WebSocket代理",
        "url": "VHWebSocket_Help.html",
        "text": "WebSocket代理 WebSocket是一种可替代HTTP、通过Internet提供实时双向通信的协议。 从1.1.1版本开始，OpenLiteSpeed通过WebSocket代理支持WebSocket后端。这些代理会将WebSocket通信发送到 字段中指定的相应后端。 URI 指定将使用此WebSocket后端的URI。仅当发往此URI的流量包含WebSocket升级请求时，才会转发到WebSocket后端。 不包含此升级请..."
    },
    {
        "title": "WebAdmin控制台-服务管理器",
        "url": "ServerStat_Help.html",
        "text": "WebAdmin控制台-服务管理器 服务管理器用于集中监视服务器并控制部分顶层功能。它提供以下功能： （可从“操作”菜单或首页访问服务管理器。） 监视服务器、监听器和虚拟主机的当前状态。 通过平滑重启应用配置更改。 启用或禁用指定的虚拟主机。 通过日志查看器查看服务器日志。 监视实时服务器统计信息。 �..."
    },
    {
        "title": "实时统计",
        "url": "Real_Time_Stats_Help.html",
        "text": "实时统计 Anti-DDoS屏蔽的IP 由Anti-DDoS保护屏蔽的IP地址列表，以逗号分隔。每个条目都以分号结尾， 并带有说明该IP地址被屏蔽原因的原因代码。 可能的原因代码： A{/}: BOT_UNKNOWN B{/}: BOT_OVER_SOFT C{/}: BOT_OVER_HARD D{/}: BOT_TOO_MANY_BAD_REQ E{/}: BOT_CAPTCHA F{/}: BOT_FLOOD G{/}: BOT_REWRITE_RULE H{/}: BOT_TOO_MANY_BAD_STATUS I{/}: BOT_BRUTE_FORCE 完整的�..."
    },
    {
        "title": "LSAPI应用",
        "url": "External_LSAPI.html",
        "text": "LSAPI应用 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 地址 外部应用程序使用的唯一套接字地址。 支持IPv4/IPv6套接字和Unix域套接字（UDS）。 IPv4/IPv6套接字可用于网络通信。 只有当外部应用程序与服务器在同一台机器上时，才能使用UDS。 [安全] 如果外部应用程序在..."
    },
    {
        "title": "Web服务器（代理）",
        "url": "External_WS.html",
        "text": "Web服务器（代理） 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 地址 外部网络服务器使用的HTTP,HTTPS或Unix域套接字(UDS)地址。 [安全建议] 如果代理到同一台机器上运行的另一台Web服务器，请将IP地址设置为 localhost{/}或 127.0.0.1{/}，这样从其他机器上就无法访问外部应�..."
    },
    {
        "title": "Fast CGI应用",
        "url": "External_FCGI.html",
        "text": "Fast CGI应用 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 地址 外部应用程序使用的唯一套接字地址。 支持IPv4/IPv6套接字和Unix域套接字（UDS）。 IPv4/IPv6套接字可用于网络通信。 只有当外部应用程序与服务器在同一台机器上时，才能使用UDS。 [安全] 如果外部应用程序..."
    },
    {
        "title": "Fast CGI授权器",
        "url": "External_FCGI_Auth.html",
        "text": "Fast CGI授权器 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 地址 外部应用程序使用的唯一套接字地址。 支持IPv4/IPv6套接字和Unix域套接字（UDS）。 IPv4/IPv6套接字可用于网络通信。 只有当外部应用程序与服务器在同一台机器上时，才能使用UDS。 [安全] 如果外部应用程..."
    },
    {
        "title": "Simple CGI应用",
        "url": "External_SCGI.html",
        "text": "Simple CGI应用 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 地址 外部应用程序使用的唯一套接字地址。 支持IPv4/IPv6套接字和Unix域套接字（UDS）。 IPv4/IPv6套接字可用于网络通信。 只有当外部应用程序与服务器在同一台机器上时，才能使用UDS。 [安全] 如果外部应用程�..."
    },
    {
        "title": "Servlet引擎",
        "url": "External_Servlet.html",
        "text": "Servlet引擎 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 地址 外部应用程序使用的唯一套接字地址。 支持IPv4/IPv6套接字和Unix域套接字（UDS）。 IPv4/IPv6套接字可用于网络通信。 只有当外部应用程序与服务器在同一台机器上时，才能使用UDS。 [安全] 如果外部应用程序�..."
    },
    {
        "title": "管道日志器",
        "url": "External_PL.html",
        "text": "管道日志器 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 远程日志程序地址（可选） 指定此管道日志程序使用的可选套接字地址。如果日志程序通过网络套接字或Unix域套接字访问，请设置此项。对于由已配置命令路径启动的本地管道日志程序，可留空。 最大连接�..."
    },
    {
        "title": "负载均衡器",
        "url": "External_LB.html",
        "text": "负载均衡器 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 Worker 之前在外部负载均衡器中定义的worker组列表。"
    },
    {
        "title": "uWSGI",
        "url": "External_UWSGI.html",
        "text": "uWSGI 名称 此外部应用程序的唯一名称。 在配置的其他部分中使用该名称时，将使用该名称进行引用。 地址 外部应用程序使用的唯一套接字地址。 支持IPv4/IPv6套接字和Unix域套接字（UDS）。 IPv4/IPv6套接字可用于网络通信。 只有当外部应用程序与服务器在同一台机器上时，才能使用UDS。 [安全] 如果外部应用程序在同一..."
    },
    {
        "title": "静态上下文",
        "url": "Static_Context.html",
        "text": "静态上下文 静态上下文 上下文设置用于为特定位置的文件指定特殊设置。 这些设置可用于引入文档根目录外的文件（类似Apache的Alias或AliasMatch指令）， 使用授权域保护特定目录，或阻止/限制对文档根目录中特定目录的访问。 URI 指定此上下文的URI。 位置 指定此上下文在文件系统中的对应位置。 默认值：$DOC_ROOT + �..."
    },
    {
        "title": "Java Web应用上下文",
        "url": "Java_Web_App_Context.html",
        "text": "Java Web应用上下文 Java Web应用上下文 许多运行Java应用程序的用户也使用Servlet引擎提供静态内容。 但在这些处理上，Servlet引擎远不如LiteSpeed Web Server高效。 为了提升整体性能，可以将LiteSpeed Web Server配置为网关服务器，由它提供静态内容并将动态Java页面请求转发到Servlet引擎。 LiteSpeed Web Server需要定义特定上下文才能�..."
    },
    {
        "title": "Servlet上下文",
        "url": "Servlet_Context.html",
        "text": "Servlet上下文 Servlet上下文 可以通过Servlet上下文单独导入Servlet。 Servlet上下文只需指定Servlet的URI和Servlet引擎名称。 仅当你不想导入整个Web应用程序，或希望用不同授权域保护不同Servlet时才需要使用此项。 此URI与 具有相同要求。 URI 指定此上下文的URI。 Servlet Engine 指定为该Web应用程序提供服务的Servlet Engine的名称。 S..."
    },
    {
        "title": "Fast CGI上下文",
        "url": "FCGI_Context.html",
        "text": "Fast CGI上下文 FastCGI上下文 FastCGI应用程序不能直接使用。必须将FastCGI应用程序配置为脚本处理程序，或通过FastCGI上下文映射到URL。 FastCGI上下文会将URI关联到FastCGI应用程序。 URI 指定此上下文的URI。 FastCGI 应用程序 指定FastCGI应用程序的名称。 必须在服务器或虚拟主机级别的 中定义此应用程序。 标头控制 指定要添�..."
    },
    {
        "title": "SCGI上下文",
        "url": "SCGI_Context.html",
        "text": "SCGI上下文 SCGI上下文 SCGI应用程序不能直接使用。必须将SCGI应用程序配置为脚本处理程序，或通过SCGI上下文映射到URL。 SCGI上下文会将URI关联到SCGI应用程序。 URI 指定此上下文的URI。 SCGI App 指定SCGI应用程序的名称。此应用程序必须在服务器或虚拟主机级别的 部分中定义。 标头控制 指定要添加的附加响应/请求头。可..."
    },
    {
        "title": "LSAPI上下文",
        "url": "LSAPI_Context.html",
        "text": "LSAPI上下文 LiteSpeed SAPI上下文 外部应用程序不能直接使用。必须将其配置为脚本处理程序，或通过上下文映射到URL。 LiteSpeed SAPI上下文会将URI关联到LSAPI（LiteSpeed Server Application Programming Interface）应用程序。 目前PHP、Ruby和Python都有LSAPI模块。LSAPI专为LiteSpeed Web Server开发，是与LiteSpeed Web Server通信的最高效方式。 URI 指�..."
    },
    {
        "title": "代理上下文",
        "url": "Proxy_Context.html",
        "text": "代理上下文 代理上下文 代理上下文可将此虚拟主机作为透明反向代理服务器。 该代理服务器可运行在任何支持HTTP协议的Web服务器或应用服务器前端。 设置代理上下文之前，必须先在 中定义此虚拟主机要代理的外部Web服务器。 URI 指定此上下文的URI。 Web服务器 指定外部Web服务器的名称。 此外部Web服务器必须在服务�..."
    },
    {
        "title": "CGI上下文",
        "url": "CGI_Context.html",
        "text": "CGI上下文 CGI上下文 CGI上下文会将特定目录中的脚本定义为CGI脚本。 该目录可以位于文档根目录内或外。 请求此目录下的文件时，无论文件是否可执行，服务器都会尝试将其作为CGI脚本执行。 这样，CGI上下文下的文件内容始终受到保护，不能作为静态内容读取。 建议将所有CGI脚本放在一个目录中，并设置CGI上下文来..."
    },
    {
        "title": "负载均衡上下文",
        "url": "LB_Context.html",
        "text": "负载均衡上下文 负载均衡上下文 与其他外部应用程序一样，负载均衡工作进程应用程序不能直接使用。 必须通过上下文将它们映射到URL。 负载均衡上下文会将URI关联到由负载均衡工作进程进行负载均衡的目标。 URI 指定此上下文的URI。 负载均衡 指定要与此上下文关联的负载均衡器。 此负载均衡是一个虚拟应用程序..."
    },
    {
        "title": "重定向上下文",
        "url": "Redirect_Context.html",
        "text": "重定向上下文 重定向上下文 重定向上下文可将一个URI或一组URI转发到其他位置。 目标URI可以位于同一网站（内部重定向），也可以是指向其他网站的绝对URI（外部重定向）。 URI 指定此上下文的URI。 外部重定向 指定此重定向是否为外部重定向。 对于外部重定向，可以指定 ，并且 可以以\"/\"或\"http(s)://\"开头。 对于内�..."
    },
    {
        "title": "应用服务器上下文",
        "url": "App_Server_Context.html",
        "text": "应用服务器上下文 App Server上下文 App Server上下文提供了一种简便方式来配置Ruby Rack/Rails、WSGI或Node.js应用程序。 通过App Server上下文添加应用程序时，只需要挂载URL和应用程序根目录。 无需额外定义外部应用程序、添加404处理程序或编写重写规则等。 URI 指定此上下文的URI。 位置 指定此上下文在文件系统中的对应位�..."
    },
    {
        "title": "uWSGI上下文",
        "url": "UWSGI_Context.html",
        "text": "uWSGI上下文 uWSGI上下文 uWSGI应用程序不能直接使用。必须将uWSGI应用程序配置为脚本处理程序，或通过uWSGI上下文映射到URL。 uWSGI上下文会将URI关联到uWSGI应用程序。 URI 指定此上下文的URI。 uWSGI App 指定uWSGI应用程序的名称。此应用程序必须在服务器或虚拟主机级别的 部分中定义。 标头控制 指定要添加的附加响应/请求..."
    },
    {
        "title": "模块处理程序上下文",
        "url": "Module_Context.html",
        "text": "模块处理程序上下文 模块处理程序上下文 模块处理程序上下文会将URI关联到已注册模块。 模块需要在服务器模块配置选项卡中注册。 URI 指定此上下文的URI。 模块 模块名称。 该模块必须在“服务器模块配置”选项卡下配置。 配置后，模块名称将在侦听器和虚拟主机配置的下拉框中显示。 标头控制 指定要添加的附�..."
    },
    {
        "title": "管理控制台常规",
        "url": "AdminGeneral_Help.html",
        "text": "管理控制台常规 管理服务器是专用于WebAdmin控制台的特殊虚拟主机。务必确保管理服务器受到安全保护：可以只允许管理员机器访问，或使用加密的SSL连接。 启用核心转储 指定当服务器由root用户启动时是否启用核心转储。 对于大多数现代Unix系统，出于安全考虑，更改用户ID或组ID的进程不允许生成core文件。不过，co..."
    },
    {
        "title": "管理控制台安全",
        "url": "AdminSecurity_Help.html",
        "text": "管理控制台安全 登入限制 指定哪些子网络和/或IP地址可以访问该服务器。 这是影响所有的虚拟主机的服务器级别设置。您还可以为每个虚拟主机设置登入限制。虚拟主机的设置不会覆盖服务器设置。 是否阻止/允许一个IP是由允许列表与阻止列表共同决定。 如果你想阻止某个特定IP或子网，请在 中写入 *{/} 或 ALL{/}，..."
    },
    {
        "title": "管理监听器常规",
        "url": "AdminListeners_General_Help.html",
        "text": "管理监听器常规 管理监听器专用于管理服务器。建议管理服务器使用安全（SSL）监听器。 侦听器名称 此侦听器的唯一名称。 IP地址 指定此侦听器的IP地址。所有可用IP地址都会列出。IPv6地址会包含在\"[ ]\"中。 要侦听所有IPv4地址，请选择 ANY{/}。要侦听所有IPv4和IPv6地址，请选择 [ANY]{/}. 为了同时为IPv4和IPv6客户端提供�..."
    },
    {
        "title": "管理监听器SSL",
        "url": "AdminListeners_SSL_Help.html",
        "text": "管理监听器SSL 管理监听器专用于管理服务器。建议管理服务器使用安全（SSL）监听器。 SSL私钥和证书 每个SSL侦听器都需要成对的SSL私钥和SSL证书。 多个SSL侦听器可以共享相同的密钥和证书。 您可以使用SSL软件包自行生成SSL私钥， 例如OpenSSL。SSL证书也可以从授权证书颁发机构（如VeriSign或Thawte）购买。您也可以自己..."
    }
];
