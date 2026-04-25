window.HDOC_SEARCH_INDEX = [
    {
        "title": "Inicio",
        "url": "index.html",
        "text": "Inicio OpenLiteSpeed Web Server 1.9 Manual de usuario — Rev. 3 Indice Licencia Introduccion Instalacion/desinstalacion Administracion Seguridad Configuracion Para obtener mas informacion, visite nuestra sitio de documentacion de OpenLiteSpeed"
    },
    {
        "title": "Licencia",
        "url": "license.html",
        "text": "Licencia GNU GENERAL PUBLIC LICENSE v3 GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/> Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed. Preamble The GNU General Public License is a free, copyleft license for software and other kinds of works. The licenses for most software and o..."
    },
    {
        "title": "Introducción",
        "url": "intro.html",
        "text": "Introducción Introduccion OpenLiteSpeed (OLS) es un servidor web open source de alto rendimiento. Sirve como alternativa gratuita a LiteSpeed Web Server Enterprise y ofrece compatibilidad con las reglas de reescritura de Apache. OLS utiliza una arquitectura orientada a eventos e incluye un motor de cache integrado (LSCache). Resumen OpenLiteSpeed admite protocolos modernos, incluidos HTTP/2 y HTTP/3 (QUIC). Es adecu..."
    },
    {
        "title": "Instalación",
        "url": "install.html",
        "text": "Instalación Instalacion/desinstalacion Requisitos minimos del sistema Sistemas operativos compatibles OpenLiteSpeed admite versiones actuales y no EOL de las siguientes distribuciones Linux: CentOS* 8, 9, 10 Debian 11, 12, 13 Ubuntu 20 (EOL el 31 de mayo de 2025), 22, 24 * Incluye RedHat Enterprise Linux y sus derivados, AlmaLinux, CloudLinux, Oracle Linux, RockyLinux, VzLinux, etc. CPU Intel: x86-64 ARM: aarch64 (s..."
    },
    {
        "title": "Administración",
        "url": "admin.html",
        "text": "Administración Administracion OpenLiteSpeed se puede controlar de tres formas: mediante la consola WebAdmin, desde la linea de comandos o enviando senales. Mediante la consola WebAdmin La consola WebAdmin es un panel de control centralizado para controlar y configurar todos los ajustes de OpenLiteSpeed. Inicie sesion en la consola WebAdmin (de forma predeterminada https://[direccion de su sitio]:7080/). Alli encontr..."
    },
    {
        "title": "Seguridad",
        "url": "security.html",
        "text": "Seguridad Seguridad LiteSpeed Web Server esta diseñado con la seguridad como una prioridad principal. LSWS admite SSL, tiene control de acceso a nivel de servidor y de virtual host, y proteccion de realm especifica por contexto. Ademas de estas funciones estandar, LSWS tambien cuenta con las siguientes funciones especiales de seguridad: Limites a nivel de conexion: La limitacion a nivel de IP restringe el ancho de b..."
    },
    {
        "title": "Configuración",
        "url": "config.html",
        "text": "Configuración Configuracion Conceptos Estos son algunos conceptos basicos que debe conocer antes de entrar en los detalles de la configuracion. Virtual hosts LiteSpeed Web Server puede alojar varios sitios web (virtual hosts) con una sola instancia de servidor. Tradicionalmente, los virtual hosts se clasifican en dos tipos: virtual hosts basados en IP y virtual hosts basados en nombre. Los virtual hosts basados en I..."
    },
    {
        "title": "Consola web",
        "url": "webconsole.html",
        "text": "Consola web Consola web La seccion Consola web controla los ajustes de la consola WebAdmin. Algunos de estos ajustes incluyen: Tiempo de espera de sesion Ajustes de registro Control de acceso Crear/eliminar usuarios administradores Restablecer contraseñas de administrador Listeners WebAdmin y ajustes SSL"
    },
    {
        "title": "Compilar PHP",
        "url": "CompilePHP_Help.html",
        "text": "Compilar PHP Variables de entorno PATH adicionales Valores PATH adicionales que se anexarán a las variables de entorno PATH actuales para los scripts de compilación. Prefijo de ruta de instalación Establece el valor de la opción de configuración \"--prefix\". La ubicación de instalación predeterminada está bajo el directorio de instalación de LiteSpeed Web Server. LiteSpeed Web Server puede usar varias version..."
    },
    {
        "title": "General del servidor",
        "url": "ServGeneral_Help.html",
        "text": "General del servidor Ajustes generales para todo el servidor. Cuando un ajuste requiere informacion de ruta, puede ser absoluta o relativa a $SERVER_ROOT. $SERVER_ROOT es la ubicacion donde se instalo el servidor web LiteSpeed (por ejemplo, your_home_dir/lsws o /opt/lsws). El ejecutable del servidor esta en $SERVER_ROOT/bin. Nombre del servidor Un nombre unico para este servidor. Si esta vacio, se usara de forma pred..."
    },
    {
        "title": "Registro del servidor",
        "url": "ServLog_Help.html",
        "text": "Registro del servidor Nombre de archivo Especifica la ruta del archivo de registro. Coloque el archivo de registro en un disco separado. Nivel de registro Especifica el nivel de registro que se incluirá en el archivo de registro. Los niveles disponibles (de mayor a menor) son: ERROR{/}, WARNING{/}, NOTICE{/}, INFO{/} y DEBUG{/}. Solo se registrarán los mensajes con nivel mayor o igual que la configuración actual. ..."
    },
    {
        "title": "Servidor - Ajustes",
        "url": "ServTuning_Help.html",
        "text": "Servidor - Ajustes Directorio SHM predeterminado Cambia el directorio predeterminado de memoria compartida a la ruta especificada. Si el directorio no existe, se creará. Todos los datos SHM se almacenarán en este directorio salvo que se especifique otra cosa. Protocolo PROXY Lista de IP/subredes para proxies front-end que se comunican con este servidor usando el protocolo PROXY. Una vez establecido, el servidor usa..."
    },
    {
        "title": "Servidor - Seguridad",
        "url": "ServSecurity_Help.html",
        "text": "Servidor - Seguridad Seguir enlaces simbólicos Especifica el ajuste predeterminado a nivel de servidor para seguir enlaces simbólicos al servir archivos estáticos. Las opciones son Yes{/}, If Owner Match{/} y No{/}. Yes{/} hace que el servidor siempre siga los enlaces simbólicos. If Owner Match{/} hace que el servidor siga un enlace simbólico solo si el propietario del enlace y el del destino son el mismo. No{/}..."
    },
    {
        "title": "Aplicaciones externas",
        "url": "ExtApp_Help.html",
        "text": "Aplicaciones externas El servidor web LiteSpeed puede reenviar solicitudes a aplicaciones externas para procesar y generar contenido dinamico. Desde la version 2.0, LiteSpeed Web Server admite siete tipos de aplicaciones externas: CGI, FastCGI, servidor web, motor servlet, aplicacion LiteSpeed SAPI, balanceador de carga y logger por tuberia. CGI significa Common Gateway Interface. El estandar actual es CGI/1.1. Las a..."
    },
    {
        "title": "Manejador de scripts",
        "url": "ScriptHandler_Help.html",
        "text": "Manejador de scripts El manejador de scripts especifica cómo procesa el servidor los scripts con sufijos concretos. Cada definición asigna un sufijo de script a un tipo de manejador y, cuando corresponde, a una aplicación externa definida previamente. Sufijos Especifica los sufijos de archivo de script que gestionará este manejador de scripts. Los sufijos deben ser únicos. El servidor agregará automáticamente ..."
    },
    {
        "title": "Servidor de aplicaciones",
        "url": "App_Server_Help.html",
        "text": "Servidor de aplicaciones Configuración predeterminada de Rack/Rails Configuraciones predeterminadas para aplicaciones Rack/Rails. Estos ajustes pueden sobrescribirse a nivel de contexto. Ruta de Ruby Ruta al ejecutable de Ruby. Normalmente es /usr/bin/ruby o /usr/local/bin/ruby, según dónde se haya instalado Ruby. Modo de ejecución Especifica el modo en que se ejecutará la aplicación: \"Development\", \"Production..."
    },
    {
        "title": "Configuracion de modulos",
        "url": "Module_Help.html",
        "text": "Configuracion de modulos El soporte de modulos esta disponible en OpenLiteSpeed 1.3 y LSWS Enterprise 5.0 o posterior. Todos los modulos requeridos deben registrarse en la pestana Server Modules Configuration. Los archivos de modulo deben estar ubicados en la carpeta server root/modules para poder registrarse. Al iniciar, el servidor carga todos los modulos registrados. El servidor debe reiniciarse despues de registr..."
    },
    {
        "title": "Listeners - General",
        "url": "Listeners_General_Help.html",
        "text": "Listeners - General Nombre del listener Un nombre unico para este listener. Direccion IP Especifica la IP de este listener. Se muestran todas las direcciones IP disponibles. Las direcciones IPv6 se encierran entre \"[ ]\". Para escuchar en todas las direcciones IPv4, seleccione ANY{/}. Para escuchar en todas las direcciones IPv4 e IPv6, seleccione [ANY]{/}. Para atender clientes IPv4 e IPv6, debe usarse una direccion I..."
    },
    {
        "title": "Listeners - SSL",
        "url": "Listeners_SSL_Help.html",
        "text": "Listeners - SSL Clave privada y certificado SSL Cada listener SSL requiere una clave privada SSL y un certificado SSL emparejados. Varios listeners SSL pueden compartir la misma clave y certificado. Puede generar claves privadas SSL usted mismo usando un paquete de software SSL, como OpenSSL. Los certificados SSL tambien pueden comprarse a una entidad emisora de certificados autorizada como VeriSign o Thawte. Tambien..."
    },
    {
        "title": "Plantillas de virtual hosts",
        "url": "Templates_Help.html",
        "text": "Plantillas de virtual hosts Las plantillas de virtual hosts facilitan la creacion de muchos virtual hosts nuevos con configuraciones similares. Cada plantilla contiene un archivo de configuracion de plantilla, una lista de listeners mapeados y una lista de virtual hosts miembros. Para agregar un virtual host basado en plantilla, el administrador solo debe agregar un miembro con un nombre de virtual host unico y un no..."
    },
    {
        "title": "Registro de virtual host",
        "url": "VirtualHosts_Help.html",
        "text": "Registro de virtual host Esta pagina enumera todos los virtual hosts definidos. Desde aqui puede agregar/eliminar un virtual host o hacer cambios en uno existente. Antes de agregar un virtual host, asegurese de que exista el directorio raiz del virtual host. Nombre del virtual host Un nombre unico para un virtual host. Se recomienda usar el nombre de dominio del virtual host como Nombre del virtual host. El Nombre de..."
    },
    {
        "title": "Host virtual - General",
        "url": "VHGeneral_Help.html",
        "text": "Host virtual - General Raíz de documentos Especifica la raíz de documentos de este virtual host. Se recomienda $VH_ROOT/html{/}. En los contextos, este directorio se referencia como $DOC_ROOT. Correo del administrador Especifica las direcciones de correo de los administradores de este virtual host. Habilitar compresión Especifica si se habilita la compresión GZIP para este virtual host. Este ajuste solo es efecti..."
    },
    {
        "title": "Host virtual - Seguridad",
        "url": "VHSecurity_Help.html",
        "text": "Host virtual - Seguridad Proteccion CAPTCHA CAPTCHA Protection es un servicio proporcionado como forma de mitigar una carga pesada del servidor. CAPTCHA Protection se activara despues de que ocurra una de las siguientes situaciones. Una vez activo, todas las solicitudes de clientes NON TRUSTED(segun configuracion) seran redirigidas a una pagina de validacion CAPTCHA. Despues de la validacion, el cliente sera redirigi..."
    },
    {
        "title": "Host virtual - SSL",
        "url": "VHSSL_Help.html",
        "text": "Host virtual - SSL Clave privada y certificado SSL Cada listener SSL requiere una clave privada SSL y un certificado SSL emparejados. Varios listeners SSL pueden compartir la misma clave y certificado. Puede generar claves privadas SSL usted mismo usando un paquete de software SSL, como OpenSSL. Los certificados SSL tambien pueden comprarse a una entidad emisora de certificados autorizada como VeriSign o Thawte. Tamb..."
    },
    {
        "title": "Rewrite",
        "url": "Rewrite_Help.html",
        "text": "Rewrite Habilitar rewrite Especifica si se habilita el motor de reescritura de URL de LiteSpeed. Esta opción puede personalizarse a nivel de virtual host o contexto, y se hereda a lo largo del árbol de directorios hasta que se sobrescribe explícitamente. Carga automatica desde .htaccess Carga automaticamente reglas rewrite contenidas en el archivo .htaccess de un directorio al acceder por primera vez a ese directo..."
    },
    {
        "title": "Contexto",
        "url": "Context_Help.html",
        "text": "Contexto En la terminologia de LiteSpeed Web Server, un \"context\" es una ubicacion virtual, una URL padre comun, que identifica un grupo de recursos. Los contextos pueden considerarse como distintos directorios en el arbol de directorios de su sitio web. Por ejemplo, \"/\" es el contexto raiz mapeado al document root de un sitio web. \"/cgi-bin/\" es un contexto mas arriba en el arbol, dedicado a las aplicaciones CGI de ..."
    },
    {
        "title": "Proxy WebSocket",
        "url": "VHWebSocket_Help.html",
        "text": "Proxy WebSocket WebSocket es un protocolo que puede usarse en lugar de HTTP para ofrecer comunicacion bidireccional en tiempo real por Internet. A partir de la version 1.1.1, OpenLiteSpeed admite backends WebSocket mediante proxies WebSocket. Estos proxies envian la comunicacion WebSocket al backend apropiado indicado en el campo Dirección. URI Especifica las URI que usarán este backend WebSocket. El tráfico hacia..."
    },
    {
        "title": "Consola WebAdmin - Administrador de servicios",
        "url": "ServerStat_Help.html",
        "text": "Consola WebAdmin - Administrador de servicios El Administrador de servicios actúa como sala de control para supervisar el servidor y controlar ciertas funciones de nivel superior. Proporciona las siguientes funciones: (Se puede acceder al Administrador de servicios desde el menú Actions o desde la página de inicio.) Supervisar el estado actual del servidor, los listeners y los hosts virtuales. Aplicar cambios de c..."
    },
    {
        "title": "Estadísticas en tiempo real",
        "url": "Real_Time_Stats_Help.html",
        "text": "Estadísticas en tiempo real IP bloqueada por Anti-DDoS Lista de direcciones IP separadas por comas y bloqueadas por la protección Anti-DDoS. Cada dirección termina con un punto y coma y un código de motivo que indica por qué se bloqueó la dirección IP. Códigos de motivo posibles: A{/}: BOT_UNKNOWN B{/}: BOT_OVER_SOFT C{/}: BOT_OVER_HARD D{/}: BOT_TOO_MANY_BAD_REQ E{/}: BOT_CAPTCHA F{/}: BOT_FLOOD G{/}: BOT_RE..."
    },
    {
        "title": "Aplicación LSAPI externa",
        "url": "External_LSAPI.html",
        "text": "Aplicación LSAPI externa Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Direccion Direccion de socket unica usada por la aplicacion externa. Se admiten sockets IPv4/IPv6 y Unix Domain Sockets (UDS). Los sockets IPv4/IPv6 pueden usarse para comunicacion por red. UDS solo puede usarse cuando la aplicacion externa reside en la ..."
    },
    {
        "title": "Servidor web externo",
        "url": "External_WS.html",
        "text": "Servidor web externo Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Direccion Direccion HTTP, HTTPS o Unix Domain Sockets (UDS) usada por el servidor web externo. Si hace proxy a otro servidor web que se ejecuta en la misma maquina mediante una direccion IPv4/IPv6, establezca la direccion IP en localhost{/} o 127.0.0.1{/}, pa..."
    },
    {
        "title": "Aplicación FastCGI externa",
        "url": "External_FCGI.html",
        "text": "Aplicación FastCGI externa Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Direccion Direccion de socket unica usada por la aplicacion externa. Se admiten sockets IPv4/IPv6 y Unix Domain Sockets (UDS). Los sockets IPv4/IPv6 pueden usarse para comunicacion por red. UDS solo puede usarse cuando la aplicacion externa reside en l..."
    },
    {
        "title": "Autenticador FastCGI externo",
        "url": "External_FCGI_Auth.html",
        "text": "Autenticador FastCGI externo Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Direccion Direccion de socket unica usada por la aplicacion externa. Se admiten sockets IPv4/IPv6 y Unix Domain Sockets (UDS). Los sockets IPv4/IPv6 pueden usarse para comunicacion por red. UDS solo puede usarse cuando la aplicacion externa reside en ..."
    },
    {
        "title": "Aplicación SCGI externa",
        "url": "External_SCGI.html",
        "text": "Aplicación SCGI externa Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Direccion Direccion de socket unica usada por la aplicacion externa. Se admiten sockets IPv4/IPv6 y Unix Domain Sockets (UDS). Los sockets IPv4/IPv6 pueden usarse para comunicacion por red. UDS solo puede usarse cuando la aplicacion externa reside en la m..."
    },
    {
        "title": "Aplicación Servlet externa",
        "url": "External_Servlet.html",
        "text": "Aplicación Servlet externa Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Direccion Direccion de socket unica usada por la aplicacion externa. Se admiten sockets IPv4/IPv6 y Unix Domain Sockets (UDS). Los sockets IPv4/IPv6 pueden usarse para comunicacion por red. UDS solo puede usarse cuando la aplicacion externa reside en l..."
    },
    {
        "title": "Aplicación proxy externa",
        "url": "External_PL.html",
        "text": "Aplicación proxy externa Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Direccion del registrador remoto (Opcional) Especifica la direccion de socket opcional para este registrador canalizado. Configure este valor cuando el registrador se alcance mediante un socket de red o Unix Domain Socket. Dejelo vacio para un registrado..."
    },
    {
        "title": "Aplicación de balanceo externa",
        "url": "External_LB.html",
        "text": "Aplicación de balanceo externa Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Workers Lista de grupos de workers definidos previamente en el balanceador de carga externo."
    },
    {
        "title": "Aplicación uWSGI externa",
        "url": "External_UWSGI.html",
        "text": "Aplicación uWSGI externa Nombre Un nombre unico para esta aplicacion externa. Se usara este nombre para hacer referencia a ella en otras partes de la configuracion. Direccion Direccion de socket unica usada por la aplicacion externa. Se admiten sockets IPv4/IPv6 y Unix Domain Sockets (UDS). Los sockets IPv4/IPv6 pueden usarse para comunicacion por red. UDS solo puede usarse cuando la aplicacion externa reside en la ..."
    },
    {
        "title": "Contexto estático",
        "url": "Static_Context.html",
        "text": "Contexto estático Contexto general La configuracion de contexto se usa para definir ajustes especiales para archivos en una ubicacion determinada. Estos ajustes pueden incorporar archivos fuera de la raiz de documentos (como las directivas Alias o AliasMatch de Apache), proteger un directorio mediante realms de autorizacion, o bloquear o restringir el acceso a un directorio dentro de la raiz de documentos. URI Espec..."
    },
    {
        "title": "Contexto de aplicación web Java",
        "url": "Java_Web_App_Context.html",
        "text": "Contexto de aplicación web Java Contexto de aplicación web Java Muchas instalaciones Java usan el motor de servlets para servir tambien contenido estatico. Sin embargo, ningun motor de servlets es tan eficiente como LiteSpeed Web Server para estos procesos. Para mejorar el rendimiento general, LiteSpeed Web Server puede configurarse como servidor de puerta de enlace, sirviendo contenido estatico y reenviando las so..."
    },
    {
        "title": "Contexto Servlet",
        "url": "Servlet_Context.html",
        "text": "Contexto Servlet Contexto Servlet Los servlets pueden importarse individualmente mediante contextos de servlet. Un contexto de servlet solo especifica la URI del servlet y el nombre del motor de servlets. Use esto solo cuando no quiera importar toda la aplicacion web o cuando quiera proteger servlets distintos con realms de autorizacion distintos. Esta URI tiene los mismos requisitos que para un . URI Especifica la U..."
    },
    {
        "title": "Contexto FastCGI",
        "url": "FCGI_Context.html",
        "text": "Contexto FastCGI Contexto FastCGI Las aplicaciones FastCGI no pueden usarse directamente. Una aplicacion FastCGI debe configurarse como manejador de scripts o asignarse a una URL mediante un contexto FastCGI. Un contexto FastCGI asocia una URI con una aplicacion FastCGI. URI Especifica la URI para este contexto. Aplicacion FastCGI Especifica el nombre de la aplicacion FastCGI. Esta aplicacion debe definirse en la sec..."
    },
    {
        "title": "Contexto SCGI",
        "url": "SCGI_Context.html",
        "text": "Contexto SCGI Contexto SCGI Las aplicaciones SCGI no pueden usarse directamente. Una aplicacion SCGI debe configurarse como manejador de scripts o asignarse a una URL mediante un contexto SCGI. Un contexto SCGI asocia una URI con una aplicacion SCGI. URI Especifica la URI para este contexto. Aplicacion SCGI Especifica el nombre de la aplicacion SCGI. Esta aplicacion debe definirse en la seccion a nivel de servidor o ..."
    },
    {
        "title": "Contexto LSAPI",
        "url": "LSAPI_Context.html",
        "text": "Contexto LSAPI Contexto LSAPI Las aplicaciones externas no pueden usarse directamente. Deben configurarse como manejador de scripts o asignarse a una URL mediante un contexto. Un contexto LiteSpeed SAPI asocia una URI con una aplicacion LSAPI (LiteSpeed Server Application Programming Interface). Actualmente PHP, Ruby y Python tienen modulos LSAPI. LSAPI, al estar desarrollado especificamente para LiteSpeed web server..."
    },
    {
        "title": "Contexto proxy",
        "url": "Proxy_Context.html",
        "text": "Contexto proxy Contexto proxy Un contexto proxy habilita este host virtual como servidor proxy inverso transparente. Este servidor proxy puede ejecutarse delante de cualquier servidor web o servidor de aplicaciones que admita el protocolo HTTP. El servidor web externo para el que este host virtual actua como proxy debe definirse en antes de configurar un contexto proxy. URI Especifica la URI para este contexto. Servi..."
    },
    {
        "title": "Contexto CGI",
        "url": "CGI_Context.html",
        "text": "Contexto CGI Contexto CGI Un contexto CGI define los scripts de un directorio determinado como scripts CGI. Este directorio puede estar dentro o fuera de la raiz de documentos. Cuando se solicita un archivo de este directorio, el servidor siempre intentara ejecutarlo como script CGI, sea ejecutable o no. De esta forma, el contenido de los archivos bajo un contexto CGI siempre queda protegido y no puede leerse como co..."
    },
    {
        "title": "Contexto de balanceo",
        "url": "LB_Context.html",
        "text": "Contexto de balanceo Contexto de balanceo Como otras aplicaciones externas, las aplicaciones worker del balanceador de carga no pueden usarse directamente. Deben asignarse a una URL mediante un contexto. Un contexto de balanceador de carga asocia una URI para que sea balanceada por los workers del balanceador de carga. URI Especifica la URI para este contexto. Load Balancer Especifica el nombre del load balancer que ..."
    },
    {
        "title": "Contexto de redirección",
        "url": "Redirect_Context.html",
        "text": "Contexto de redirección Contexto de redirección Un contexto de redireccion puede usarse para reenviar una URI o un grupo de URIs a otra ubicacion. La URI de destino puede estar en el mismo sitio web (una redireccion interna) o ser una URI absoluta que apunte a otro sitio web (una redireccion externa). URI Especifica la URI para este contexto. Redireccion externa Especifica si esta redireccion es externa. Para redir..."
    },
    {
        "title": "Contexto de servidor de aplicaciones",
        "url": "App_Server_Context.html",
        "text": "Contexto de servidor de aplicaciones Contexto de servidor de aplicaciones Un contexto de servidor de aplicaciones ofrece una forma sencilla de configurar una aplicacion Ruby Rack/Rails, WSGI o Node.js. Para agregar una aplicacion mediante un contexto de servidor de aplicaciones, solo se requiere montar la URL y el directorio raiz de la aplicacion. No hace falta definir una aplicacion externa, agregar un manejador 404..."
    },
    {
        "title": "Contexto uWSGI",
        "url": "UWSGI_Context.html",
        "text": "Contexto uWSGI Contexto uWSGI Las aplicaciones uWSGI no pueden usarse directamente. Una aplicacion uWSGI debe configurarse como manejador de scripts o asignarse a una URL mediante un contexto uWSGI. Un contexto uWSGI asocia una URI con una aplicacion uWSGI. URI Especifica la URI para este contexto. Aplicacion uWSGI Especifica el nombre de la aplicacion uWSGI. Esta aplicacion debe definirse en la seccion a nivel de se..."
    },
    {
        "title": "Contexto de módulo",
        "url": "Module_Context.html",
        "text": "Contexto de módulo Contexto de módulo Un contexto de manejador de modulo asocia una URI con un modulo registrado. Los modulos deben registrarse en la pestana Configuracion de modulos del servidor. URI Especifica la URI para este contexto. Modulo Nombre del modulo. El modulo debe registrarse bajo la pestana Server Module Configuration. Una vez registrado, el nombre del modulo estara disponible en el menu desplegable..."
    },
    {
        "title": "Consola WebAdmin - General",
        "url": "AdminGeneral_Help.html",
        "text": "Consola WebAdmin - General Admin Server es un virtual host especial dedicado a la consola WebAdmin. Es muy importante asegurarse de que Admin Server este protegido de forma segura, ya sea permitiendo acceso solo desde los equipos de los administradores o usando una conexion SSL cifrada. Habilitar volcado core Especifica si se habilita el volcado core cuando el servidor se inicia con el usuario \"root\". En la mayoria d..."
    },
    {
        "title": "Consola WebAdmin - Seguridad",
        "url": "AdminSecurity_Help.html",
        "text": "Consola WebAdmin - Seguridad Control de acceso Especifica que subredes y/o direcciones IP pueden acceder al servidor. A nivel de servidor, este ajuste afectara a todos los virtual hosts. Tambien puede configurar control de acceso unico para cada virtual host a nivel de virtual host. Los ajustes de nivel de virtual host NO sobrescribiran los ajustes de nivel de servidor. Bloquear/permitir una IP se determina por la co..."
    },
    {
        "title": "Listeners de administración - General",
        "url": "AdminListeners_General_Help.html",
        "text": "Listeners de administración - General Los listeners de administración están dedicados al servidor de administración. Se recomiendan listeners seguros (SSL) para el servidor de administración. Nombre del listener Un nombre unico para este listener. Direccion IP Especifica la IP de este listener. Se muestran todas las direcciones IP disponibles. Las direcciones IPv6 se encierran entre \"[ ]\". Para escuchar en todas..."
    },
    {
        "title": "Listeners de administración - SSL",
        "url": "AdminListeners_SSL_Help.html",
        "text": "Listeners de administración - SSL Los listeners de administración están dedicados al servidor de administración. Se recomiendan listeners seguros (SSL) para el servidor de administración. Clave privada y certificado SSL Cada listener SSL requiere una clave privada SSL y un certificado SSL emparejados. Varios listeners SSL pueden compartir la misma clave y certificado. Puede generar claves privadas SSL usted mism..."
    }
];
