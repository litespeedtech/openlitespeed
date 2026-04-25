window.HDOC_SEARCH_INDEX = [
    {
        "title": "Startseite",
        "url": "index.html",
        "text": "Startseite OpenLiteSpeed Web Server 1.9 Benutzerhandbuch — Rev. 3 Inhaltsverzeichnis Lizenz Einleitung Installation/Deinstallation Administration Sicherheit Konfiguration Weitere Informationen finden Sie in unserer OpenLiteSpeed-Dokumentation"
    },
    {
        "title": "Lizenz",
        "url": "license.html",
        "text": "Lizenz GNU GENERAL PUBLIC LICENSE v3 GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/> Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed. Preamble The GNU General Public License is a free, copyleft license for software and other kinds of works. The licenses for most software and oth..."
    },
    {
        "title": "Einführung",
        "url": "intro.html",
        "text": "Einführung Einleitung OpenLiteSpeed (OLS) ist ein leistungsstarker Open-Source-Webserver. Er dient als kostenlose Alternative zu LiteSpeed Web Server Enterprise und bietet Kompatibilitaet mit Apache-Rewrite-Regeln. OLS verwendet eine ereignisgesteuerte Architektur und enthaelt eine integrierte Caching-Engine (LSCache). Ueberblick OpenLiteSpeed unterstuetzt moderne Protokolle einschliesslich HTTP/2 und HTTP/3 (QUIC)...."
    },
    {
        "title": "Installation",
        "url": "install.html",
        "text": "Installation Installation/Deinstallation Minimale Systemanforderungen Unterstutzte Betriebssysteme OpenLiteSpeed unterstutzt aktuelle und nicht abgekundigte Versionen der folgenden Linux-Distributionen: CentOS* 8, 9, 10 Debian 11, 12, 13 Ubuntu 20 (EOL am 31. Mai 2025), 22, 24 * Einschliesslich RedHat Enterprise Linux und Derivaten wie AlmaLinux, CloudLinux, Oracle Linux, RockyLinux, VzLinux usw. CPU Intel: x86-64 AR..."
    },
    {
        "title": "Administration",
        "url": "admin.html",
        "text": "Administration Verwaltung OpenLiteSpeed kann auf drei Arten gesteuert werden: uber die WebAdmin-Konsole, uber die Befehlszeile oder durch das Senden von Signalen. Uber die WebAdmin-Konsole Die WebAdmin-Konsole ist ein zentrales Bedienfeld zum Steuern und Konfigurieren aller OpenLiteSpeed-Einstellungen. Melden Sie sich an der WebAdmin-Konsole an (standardmassig unter https://[Adresse Ihrer Website]:7080/). Dort finden..."
    },
    {
        "title": "Sicherheit",
        "url": "security.html",
        "text": "Sicherheit Sicherheit LiteSpeed Web Server wurde mit Sicherheit als zentralem Anliegen entwickelt. LSWS unterstuetzt SSL, bietet Zugriffskontrolle auf Server- und Virtual-Host-Ebene und kontextspezifischen Realm-Schutz. Neben diesen Standardfunktionen bietet LSWS auch die folgenden besonderen Sicherheitsfunktionen: Limits auf Verbindungsebene: Drosselung auf IP-Ebene begrenzt Netzwerkbandbreite zu und von einer einze..."
    },
    {
        "title": "Konfiguration",
        "url": "config.html",
        "text": "Konfiguration Konfiguration Konzepte Hier sind einige grundlegende Konzepte, die Sie kennen sollten, bevor Sie in die Details der Konfiguration einsteigen. Virtuelle Hosts LiteSpeed Web Server kann mehrere Websites (virtuelle Hosts) mit einer Serverinstanz hosten. Traditionell werden virtuelle Hosts in zwei Typen eingeteilt: IP-basierte virtuelle Hosts und namensbasierte virtuelle Hosts. IP-basierte virtuelle Hosts s..."
    },
    {
        "title": "Web-Konsole",
        "url": "webconsole.html",
        "text": "Web-Konsole Webkonsole Der Bereich Webkonsole steuert die Einstellungen fuer die WebAdmin-Konsole. Einige dieser Einstellungen sind: Sitzungs-Timeout Protokolleinstellungen Zugriffskontrolle Admin-Benutzer erstellen/loeschen Admin-Kennwoerter zuruecksetzen WebAdmin-Listener und SSL-Einstellungen"
    },
    {
        "title": "PHP kompilieren",
        "url": "CompilePHP_Help.html",
        "text": "PHP kompilieren Zusätzliche PATH-Umgebungsvariablen Zusätzliche PATH-Werte, die für Build-Skripte an die aktuellen PATH-Umgebungsvariablen angehängt werden. Installationspfad-Präfix Legt den Wert für die Konfigurationsoption \"--prefix\" fest. Der standardmäßige Installationsort liegt unter dem Installationsverzeichnis von LiteSpeed Web Server. LiteSpeed Web Server kann mehrere PHP-Versionen gleichzeitig verwen..."
    },
    {
        "title": "Server - Allgemein",
        "url": "ServGeneral_Help.html",
        "text": "Server - Allgemein Allgemeine Einstellungen fuer den gesamten Server. Wenn eine Einstellung Pfadinformationen erfordert, kann der Pfad absolut oder relativ zu $SERVER_ROOT sein. $SERVER_ROOT ist der Ort, an dem der LiteSpeed-Webserver installiert wurde (zum Beispiel your_home_dir/lsws oder /opt/lsws). Die Serverprogrammdatei befindet sich unter $SERVER_ROOT/bin. Servername Ein eindeutiger Name fuer diesen Server. Wen..."
    },
    {
        "title": "Serverprotokoll",
        "url": "ServLog_Help.html",
        "text": "Serverprotokoll Dateiname Gibt den Pfad für die Protokolldatei an. Legen Sie die Protokolldatei auf einem separaten Datenträger ab. Protokollstufe Gibt die Protokollstufe an, die in die Protokolldatei aufgenommen wird. Verfügbare Stufen (von hoch bis niedrig) sind: ERROR{/}, WARNING{/}, NOTICE{/}, INFO{/} und DEBUG{/}. Nur Meldungen mit einer Stufe, die höher oder gleich der aktuellen Einstellung ist, werden prot..."
    },
    {
        "title": "Server - Tuning",
        "url": "ServTuning_Help.html",
        "text": "Server - Tuning Standard-SHM-Verzeichnis Ändert das Standardverzeichnis für gemeinsam genutzten Speicher auf den angegebenen Pfad. Wenn das Verzeichnis nicht existiert, wird es erstellt. Alle SHM-Daten werden in diesem Verzeichnis gespeichert, sofern nichts anderes angegeben ist. PROXY-Protokoll Liste von IPs/Subnetzen für Frontend-Proxys, die mit diesem Server über das PROXY-Protokoll kommunizieren. Sobald geset..."
    },
    {
        "title": "Server - Sicherheit",
        "url": "ServSecurity_Help.html",
        "text": "Server - Sicherheit Symbolischen Links folgen Gibt die Standardeinstellung auf Serverebene für das Folgen symbolischer Links beim Bereitstellen statischer Dateien an. Die Auswahlmöglichkeiten sind Yes{/}, If Owner Match{/} und No{/}. Yes{/} lässt den Server symbolischen Links immer folgen. If Owner Match{/} lässt den Server einem symbolischen Link nur folgen, wenn der Eigentümer des Links und des Ziels identisch..."
    },
    {
        "title": "Externe Anwendungen",
        "url": "ExtApp_Help.html",
        "text": "Externe Anwendungen Der LiteSpeed-Webserver kann Anfragen an externe Anwendungen weiterleiten, um dynamische Inhalte zu verarbeiten und zu erzeugen. Seit Version 2.0 unterstuetzt LiteSpeed Web Server sieben Typen externer Anwendungen: CGI, FastCGI, Webserver, Servlet-Engine, LiteSpeed SAPI-Anwendung, Load Balancer und Piped Logger. CGI steht fuer Common Gateway Interface. Der aktuelle Standard ist CGI/1.1. CGI-Anwend..."
    },
    {
        "title": "Skript-Handler",
        "url": "ScriptHandler_Help.html",
        "text": "Skript-Handler Der Skript-Handler legt fest, wie der Server Skripte mit bestimmten Suffixen verarbeitet. Jede Definition ordnet ein Skriptsuffix einem Handler-Typ und, falls erforderlich, einer zuvor definierten externen Anwendung zu. Suffixe Gibt die Suffixe der Skriptdateien an, die von diesem Skript-Handler verarbeitet werden. Suffixe müssen eindeutig sein. Der Server fügt automatisch einen speziellen MIME-Typ (..."
    },
    {
        "title": "Anwendungsserver",
        "url": "App_Server_Help.html",
        "text": "Anwendungsserver Rack/Rails-Standardeinstellungen Standardkonfigurationen fuer Rack/Rails-Anwendungen. Diese Einstellungen koennen auf Kontextebene ueberschrieben werden. Ruby-Pfad Pfad zur Ruby-Ausführungsdatei. In der Regel ist dies /usr/bin/ruby oder /usr/local/bin/ruby, je nachdem, wo Ruby installiert wurde. Laufzeitmodus Gibt an, in welchem Modus die Anwendung ausgeführt wird: \"Development\", \"Production\" oder ..."
    },
    {
        "title": "Modulkonfiguration",
        "url": "Module_Help.html",
        "text": "Modulkonfiguration Modulunterstuetzung ist in OpenLiteSpeed 1.3 und LSWS Enterprise 5.0 oder hoeher verfuegbar. Alle erforderlichen Module muessen auf der Registerkarte Server Modules Configuration registriert werden. Moduldateien muessen im Ordner server root/modules liegen, damit sie registriert werden koennen. Beim Start laedt der Server alle registrierten Module. Der Server muss nach dem Registrieren neuer Module..."
    },
    {
        "title": "Listener - Allgemein",
        "url": "Listeners_General_Help.html",
        "text": "Listener - Allgemein Listener-Name Ein eindeutiger Name fuer diesen Listener. IP-Adresse Gibt die IP-Adresse dieses Listeners an. Alle verfuegbaren IP-Adressen werden aufgefuehrt. IPv6-Adressen stehen in \"[ ]\". Um auf allen IPv4-Adressen zu lauschen, waehlen Sie ANY{/}. Um auf allen IPv4- und IPv6-Adressen zu lauschen, waehlen Sie [ANY]{/}. Um sowohl IPv4- als auch IPv6-Clients zu bedienen, sollte statt einer einfach..."
    },
    {
        "title": "Listener - SSL",
        "url": "Listeners_SSL_Help.html",
        "text": "Listener - SSL Privater SSL-Schluessel und Zertifikat Jeder SSL-Listener benoetigt einen passenden privaten SSL-Schluessel und ein SSL-Zertifikat. Mehrere SSL-Listener koennen denselben Schluessel und dasselbe Zertifikat gemeinsam verwenden. Sie koennen private SSL-Schluessel selbst mit einem SSL-Softwarepaket wie OpenSSL erzeugen. SSL-Zertifikate koennen auch bei einer autorisierten Zertifizierungsstelle wie VeriSig..."
    },
    {
        "title": "Vorlagen fuer virtuelle Hosts",
        "url": "Templates_Help.html",
        "text": "Vorlagen fuer virtuelle Hosts Vorlagen fuer virtuelle Hosts erleichtern das Erstellen vieler neuer virtueller Hosts mit aehnlichen Konfigurationen. Jede Vorlage enthaelt eine Vorlagen-Konfigurationsdatei, eine Liste zugeordneter Listener und eine Liste von Mitglieds-virtual-hosts. Um einen vorlagenbasierten virtuellen Host hinzuzufuegen, muss der Administrator nur ein Mitglied mit einem eindeutigen Namen fuer den vir..."
    },
    {
        "title": "Virtual-Host-Registrierung",
        "url": "VirtualHosts_Help.html",
        "text": "Virtual-Host-Registrierung Diese Seite listet alle definierten virtuellen Hosts auf. Von hier aus koennen Sie einen virtuellen Host hinzufuegen/loeschen oder Aenderungen an einem vorhandenen vornehmen. Bevor Sie einen virtuellen Host hinzufuegen, stellen Sie sicher, dass das Stammverzeichnis des virtuellen Hosts existiert. Name des Virtual Hosts Ein eindeutiger Name fuer einen Virtual Host. Es wird empfohlen, den Dom..."
    },
    {
        "title": "Virtueller Host - Allgemein",
        "url": "VHGeneral_Help.html",
        "text": "Virtueller Host - Allgemein Dokumentstamm Gibt den Dokumentstamm für diesen virtuellen Host an. $VH_ROOT/html{/} wird empfohlen. In Kontexten wird dieses Verzeichnis als $DOC_ROOT referenziert. Administrator-E-Mail Gibt die E-Mail-Adresse(n) der Administratoren dieses virtuellen Hosts an. Komprimierung aktivieren Gibt an, ob die GZIP-Komprimierung für diesen virtuellen Host aktiviert wird. Diese Einstellung ist nur..."
    },
    {
        "title": "Virtueller Host - Sicherheit",
        "url": "VHSecurity_Help.html",
        "text": "Virtueller Host - Sicherheit CAPTCHA-Schutz CAPTCHA Protection ist ein Dienst zur Minderung hoher Serverlast. CAPTCHA Protection wird aktiviert, sobald eine der folgenden Situationen eintritt. Sobald aktiv, werden alle Anfragen von NON TRUSTED(wie konfiguriert) Clients auf eine CAPTCHA-Validierungsseite umgeleitet. Nach der Validierung wird der Client auf die gewuenschte Seite weitergeleitet. Die folgenden Situatione..."
    },
    {
        "title": "Virtueller Host - SSL",
        "url": "VHSSL_Help.html",
        "text": "Virtueller Host - SSL Privater SSL-Schluessel und Zertifikat Jeder SSL-Listener benoetigt einen passenden privaten SSL-Schluessel und ein SSL-Zertifikat. Mehrere SSL-Listener koennen denselben Schluessel und dasselbe Zertifikat gemeinsam verwenden. Sie koennen private SSL-Schluessel selbst mit einem SSL-Softwarepaket wie OpenSSL erzeugen. SSL-Zertifikate koennen auch bei einer autorisierten Zertifizierungsstelle wie ..."
    },
    {
        "title": "Rewrite",
        "url": "Rewrite_Help.html",
        "text": "Rewrite Rewrite aktivieren Gibt an, ob LiteSpeeds URL-Rewrite-Engine aktiviert wird. Diese Option kann auf Virtual-Host- oder Kontextebene angepasst werden und wird entlang des Verzeichnisbaums vererbt, bis sie ausdrücklich überschrieben wird. Automatisch aus .htaccess laden Laedt Rewrite-Regeln aus der .htaccess-Datei eines Verzeichnisses automatisch beim ersten Zugriff auf dieses Verzeichnis, wenn fuer dieses Ver..."
    },
    {
        "title": "Kontext",
        "url": "Context_Help.html",
        "text": "Kontext In der Terminologie von LiteSpeed Web Server ist ein \"context\" ein virtueller Ort, eine gemeinsame uebergeordnete URL, die eine Gruppe von Ressourcen identifiziert. Kontexte kann man sich als verschiedene Verzeichnisse im Verzeichnisbaum Ihrer Website vorstellen. Zum Beispiel ist \"/\" der Root-Kontext, der dem document root einer Website zugeordnet ist. \"/cgi-bin/\" ist ein weiter oben im Baum liegender Kontext..."
    },
    {
        "title": "WebSocket-Proxy",
        "url": "VHWebSocket_Help.html",
        "text": "WebSocket-Proxy WebSocket ist ein Protokoll, das statt HTTP verwendet werden kann, um bidirektionale Echtzeit- kommunikation ueber das Internet bereitzustellen. Ab Version 1.1.1 unterstuetzt OpenLiteSpeed WebSocket-Backends mithilfe von WebSocket-Proxys. Diese Proxys senden die WebSocket- Kommunikation an das passende Backend, das im Feld Adresse angegeben ist. URI Gibt die URI(s) an, die dieses WebSocket-Backend ver..."
    },
    {
        "title": "WebAdmin-Konsole - Dienstmanager",
        "url": "ServerStat_Help.html",
        "text": "WebAdmin-Konsole - Dienstmanager Der Dienstmanager dient als Kontrollraum zum Überwachen des Servers und zum Steuern bestimmter übergeordneter Funktionen. Er bietet die folgenden Funktionen: (Der Dienstmanager ist über das Menü Actions oder über die Startseite erreichbar.) Aktuellen Status von Server, Listenern und virtuellen Hosts überwachen. Konfigurationsänderungen mit einem graceful restart anwenden. Einen..."
    },
    {
        "title": "Echtzeitstatistiken",
        "url": "Real_Time_Stats_Help.html",
        "text": "Echtzeitstatistiken Von Anti-DDoS blockierte IP Eine durch Kommas getrennte Liste von IP-Adressen, die durch den Anti-DDoS-Schutz blockiert wurden. Jede Adresse endet mit einem Semikolon und einem Ursachencode, der angibt, warum die IP-Adresse blockiert wurde. Mögliche Ursachencodes: A{/}: BOT_UNKNOWN B{/}: BOT_OVER_SOFT C{/}: BOT_OVER_HARD D{/}: BOT_TOO_MANY_BAD_REQ E{/}: BOT_CAPTCHA F{/}: BOT_FLOOD G{/}: BOT_REWRI..."
    },
    {
        "title": "Externe LSAPI-Anwendung",
        "url": "External_LSAPI.html",
        "text": "Externe LSAPI-Anwendung Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Adresse Eine eindeutige Socket-Adresse, die von der externen Anwendung verwendet wird. IPv4/IPv6-Sockets und Unix Domain Sockets (UDS) werden unterstuetzt. IPv4/IPv6-Sockets koennen fuer Kommunikation ueber das Netzwerk verwendet werden. UDS kann nur verwendet wer..."
    },
    {
        "title": "Externer Webserver",
        "url": "External_WS.html",
        "text": "Externer Webserver Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Adresse HTTP-, HTTPS- oder Unix Domain Sockets (UDS)-Adresse, die vom externen Webserver verwendet wird. Wenn Sie per Proxy zu einem anderen Webserver auf derselben Maschine ueber eine IPv4/IPv6-Adresse verbinden, setzen Sie die IP-Adresse auf localhost{/} oder 127.0.0..."
    },
    {
        "title": "Externe FastCGI-Anwendung",
        "url": "External_FCGI.html",
        "text": "Externe FastCGI-Anwendung Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Adresse Eine eindeutige Socket-Adresse, die von der externen Anwendung verwendet wird. IPv4/IPv6-Sockets und Unix Domain Sockets (UDS) werden unterstuetzt. IPv4/IPv6-Sockets koennen fuer Kommunikation ueber das Netzwerk verwendet werden. UDS kann nur verwendet w..."
    },
    {
        "title": "Externer FastCGI-Authenticator",
        "url": "External_FCGI_Auth.html",
        "text": "Externer FastCGI-Authenticator Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Adresse Eine eindeutige Socket-Adresse, die von der externen Anwendung verwendet wird. IPv4/IPv6-Sockets und Unix Domain Sockets (UDS) werden unterstuetzt. IPv4/IPv6-Sockets koennen fuer Kommunikation ueber das Netzwerk verwendet werden. UDS kann nur verwen..."
    },
    {
        "title": "Externe SCGI-Anwendung",
        "url": "External_SCGI.html",
        "text": "Externe SCGI-Anwendung Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Adresse Eine eindeutige Socket-Adresse, die von der externen Anwendung verwendet wird. IPv4/IPv6-Sockets und Unix Domain Sockets (UDS) werden unterstuetzt. IPv4/IPv6-Sockets koennen fuer Kommunikation ueber das Netzwerk verwendet werden. UDS kann nur verwendet werd..."
    },
    {
        "title": "Externe Servlet-Anwendung",
        "url": "External_Servlet.html",
        "text": "Externe Servlet-Anwendung Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Adresse Eine eindeutige Socket-Adresse, die von der externen Anwendung verwendet wird. IPv4/IPv6-Sockets und Unix Domain Sockets (UDS) werden unterstuetzt. IPv4/IPv6-Sockets koennen fuer Kommunikation ueber das Netzwerk verwendet werden. UDS kann nur verwendet w..."
    },
    {
        "title": "Externe Proxy-Anwendung",
        "url": "External_PL.html",
        "text": "Externe Proxy-Anwendung Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Remote-Logger-Adresse (optional) Gibt die optionale Socket-Adresse fuer diesen verrohrten Logger an. Setzen Sie diesen Wert, wenn der Logger ueber einen Netzwerk-Socket oder Unix Domain Socket erreichbar ist. Lassen Sie das Feld leer fuer einen lokalen verrohrten ..."
    },
    {
        "title": "Externe Load-Balancer-Anwendung",
        "url": "External_LB.html",
        "text": "Externe Load-Balancer-Anwendung Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Worker Liste der Worker-Gruppen, die zuvor im externen Load Balancer definiert wurden."
    },
    {
        "title": "Externe uWSGI-Anwendung",
        "url": "External_UWSGI.html",
        "text": "Externe uWSGI-Anwendung Name Ein eindeutiger Name fuer diese externe Anwendung. Sie verweisen mit diesem Namen in anderen Teilen der Konfiguration auf sie. Adresse Eine eindeutige Socket-Adresse, die von der externen Anwendung verwendet wird. IPv4/IPv6-Sockets und Unix Domain Sockets (UDS) werden unterstuetzt. IPv4/IPv6-Sockets koennen fuer Kommunikation ueber das Netzwerk verwendet werden. UDS kann nur verwendet wer..."
    },
    {
        "title": "Statischer Kontext",
        "url": "Static_Context.html",
        "text": "Statischer Kontext Allgemeiner Kontext Kontexteinstellungen werden verwendet, um besondere Einstellungen fuer Dateien an einem bestimmten Speicherort festzulegen. Diese Einstellungen koennen Dateien ausserhalb des Dokumentstammverzeichnisses einbinden (wie die Apache-Direktiven Alias oder AliasMatch), ein bestimmtes Verzeichnis mit Autorisierungs-Realms schuetzen oder den Zugriff auf ein bestimmtes Verzeichnis innerh..."
    },
    {
        "title": "Java-Web-App-Kontext",
        "url": "Java_Web_App_Context.html",
        "text": "Java-Web-App-Kontext Java-Web-App-Kontext Viele Betreiber von Java-Anwendungen verwenden die Servlet-Engine auch zum Bereitstellen statischer Inhalte. Keine Servlet-Engine ist fuer diese Aufgaben jedoch annaehend so effizient wie LiteSpeed Web Server. Um die Gesamtleistung zu verbessern, kann LiteSpeed Web Server als Gateway-Server konfiguriert werden, der statische Inhalte ausliefert und dynamische Java-Seitenanfrag..."
    },
    {
        "title": "Servlet-Kontext",
        "url": "Servlet_Context.html",
        "text": "Servlet-Kontext Servlet-Kontext Servlets koennen einzeln ueber Servlet-Kontexte importiert werden. Ein Servlet-Kontext gibt lediglich die URI fuer das Servlet und den Namen der Servlet-Engine an. Verwenden Sie dies nur, wenn Sie nicht die gesamte Webanwendung importieren moechten oder wenn verschiedene Servlets mit unterschiedlichen Autorisierungs-Realms geschuetzt werden sollen. Diese URI hat dieselben Anforderungen..."
    },
    {
        "title": "FastCGI-Kontext",
        "url": "FCGI_Context.html",
        "text": "FastCGI-Kontext FastCGI-Kontext FastCGI-Anwendungen koennen nicht direkt verwendet werden. Eine FastCGI-Anwendung muss entweder als Skript-Handler konfiguriert oder ueber einen FastCGI-Kontext einer URL zugeordnet werden. Ein FastCGI-Kontext verknuepft eine URI mit einer FastCGI-Anwendung. URI Gibt die URI fuer diesen Kontext an. FastCGI-App Gibt den Namen der FastCGI-Anwendung an. Diese Anwendung muss im Abschnitt a..."
    },
    {
        "title": "SCGI-Kontext",
        "url": "SCGI_Context.html",
        "text": "SCGI-Kontext SCGI-Kontext SCGI-Anwendungen koennen nicht direkt verwendet werden. Eine SCGI-Anwendung muss entweder als Skript-Handler konfiguriert oder ueber einen SCGI-Kontext einer URL zugeordnet werden. Ein SCGI-Kontext verknuepft eine URI mit einer SCGI-Anwendung. URI Gibt die URI fuer diesen Kontext an. SCGI-App Gibt den Namen der SCGI-Anwendung an. Diese Anwendung muss im Abschnitt auf Server- oder Virtual-Hos..."
    },
    {
        "title": "LSAPI-Kontext",
        "url": "LSAPI_Context.html",
        "text": "LSAPI-Kontext LSAPI-Kontext Externe Anwendungen koennen nicht direkt verwendet werden. Sie muessen entweder als Skript-Handler konfiguriert oder ueber einen Kontext einer URL zugeordnet werden. Ein LiteSpeed-SAPI-Kontext verknuepft eine URI mit einer LSAPI-Anwendung (LiteSpeed Server Application Programming Interface). Derzeit gibt es LSAPI-Module fuer PHP, Ruby und Python. Da LSAPI speziell fuer LiteSpeed web server..."
    },
    {
        "title": "Proxy-Kontext",
        "url": "Proxy_Context.html",
        "text": "Proxy-Kontext Proxy-Kontext Ein Proxy-Kontext aktiviert diesen virtuellen Host als transparenten Reverse-Proxy-Server. Dieser Proxy-Server kann vor beliebigen Webservern oder Anwendungsservern betrieben werden, die das HTTP-Protokoll unterstuetzen. Der externe Webserver, fuer den dieser virtuelle Host als Proxy arbeitet, muss in definiert sein, bevor ein Proxy-Kontext eingerichtet werden kann. URI Gibt die URI fuer d..."
    },
    {
        "title": "CGI-Kontext",
        "url": "CGI_Context.html",
        "text": "CGI-Kontext CGI-Kontext Ein CGI-Kontext definiert Skripte in einem bestimmten Verzeichnis als CGI-Skripte. Dieses Verzeichnis kann innerhalb oder ausserhalb des Dokumentstammverzeichnisses liegen. Wenn eine Datei in diesem Verzeichnis angefordert wird, versucht der Server immer, sie als CGI-Skript auszufuehren, unabhaengig davon, ob sie ausfuehrbar ist. Dadurch ist Dateiinhalte unter einem CGI-Kontext stets geschuetz..."
    },
    {
        "title": "Load-Balancer-Kontext",
        "url": "LB_Context.html",
        "text": "Load-Balancer-Kontext Load-Balancer-Kontext Wie andere externe Anwendungen koennen Load-Balancer-Worker-Anwendungen nicht direkt verwendet werden. Sie muessen ueber einen Kontext einer URL zugeordnet werden. Ein Load-Balancer-Kontext verknuepft eine URI, die von den Load-Balancer-Workern lastverteilt werden soll. URI Gibt die URI fuer diesen Kontext an. Load Balancer Gibt den Namen des Load Balancers an, der diesem K..."
    },
    {
        "title": "Umleitungskontext",
        "url": "Redirect_Context.html",
        "text": "Umleitungskontext Umleitungskontext Ein Weiterleitungskontext kann verwendet werden, um eine URI oder eine Gruppe von URIs an einen anderen Ort weiterzuleiten. Die Ziel-URI kann sich auf derselben Website befinden (interne Weiterleitung) oder eine absolute URI sein, die auf eine andere Website verweist (externe Weiterleitung). URI Gibt die URI fuer diesen Kontext an. Externe Weiterleitung Gibt an, ob diese Weiterleit..."
    },
    {
        "title": "Anwendungsserver-Kontext",
        "url": "App_Server_Context.html",
        "text": "Anwendungsserver-Kontext Anwendungsserver-Kontext Ein App-Server-Kontext bietet eine einfache Moeglichkeit, eine Ruby-Rack/Rails-, WSGI- oder Node.js-Anwendung zu konfigurieren. Um eine Anwendung ueber einen App-Server-Kontext hinzuzufuegen, muessen nur die URL eingebunden und das Stammverzeichnis der Anwendung angegeben werden. Es ist nicht erforderlich, eine externe Anwendung zu definieren, einen 404-Handler hinzuz..."
    },
    {
        "title": "uWSGI-Kontext",
        "url": "UWSGI_Context.html",
        "text": "uWSGI-Kontext uWSGI-Kontext uWSGI-Anwendungen koennen nicht direkt verwendet werden. Eine uWSGI-Anwendung muss entweder als Skript-Handler konfiguriert oder ueber einen uWSGI-Kontext einer URL zugeordnet werden. Ein uWSGI-Kontext verknuepft eine URI mit einer uWSGI-Anwendung. URI Gibt die URI fuer diesen Kontext an. uWSGI-App Gibt den Namen der uWSGI-Anwendung an. Diese Anwendung muss im Abschnitt auf Server- oder Vi..."
    },
    {
        "title": "Modul-Kontext",
        "url": "Module_Context.html",
        "text": "Modul-Kontext Modul-Kontext Ein Modul-Handler-Kontext verknuepft eine URI mit einem registrierten Modul. Module muessen auf der Registerkarte Servermodul-Konfiguration registriert werden. URI Gibt die URI fuer diesen Kontext an. Modul Name des Moduls. Das Modul muss im Tab Server Module Configuration registriert sein. Nach der Registrierung ist der Modulname in der Dropdown-Liste fuer Listener- und Virtual-Host-Konfi..."
    },
    {
        "title": "WebAdmin-Konsole - Allgemein",
        "url": "AdminGeneral_Help.html",
        "text": "WebAdmin-Konsole - Allgemein Admin Server ist ein spezieller virtual host fuer die WebAdmin-Konsole. Es ist sehr wichtig, Admin Server sicher zu schuetzen, entweder indem Zugriff nur von den Rechnern der Administratoren erlaubt wird oder indem eine verschluesselte SSL-Verbindung verwendet wird. Core-Dump aktivieren Gibt an, ob Core-Dumps aktiviert werden, wenn der Server vom Benutzer \"root\" gestartet wird. Auf den me..."
    },
    {
        "title": "WebAdmin-Konsole - Sicherheit",
        "url": "AdminSecurity_Help.html",
        "text": "WebAdmin-Konsole - Sicherheit Zugriffskontrolle Gibt an, welche Subnetze und/oder IP-Adressen auf den Server zugreifen koennen. Auf Serverebene wirkt sich diese Einstellung auf alle virtuellen Hosts aus. Sie koennen auch eine eigene Zugriffskontrolle fuer jeden virtuellen Host auf Virtual-Host-Ebene einrichten. Einstellungen auf Virtual-Host-Ebene ueberschreiben Server-Level-Einstellungen NICHT. Das Blockieren/Erlaub..."
    },
    {
        "title": "Admin-Listener Allgemein",
        "url": "AdminListeners_General_Help.html",
        "text": "Admin-Listener Allgemein Admin-Listener sind dem Admin Server zugeordnet. Sichere (SSL-)Listener werden für den Admin Server empfohlen. Listener-Name Ein eindeutiger Name fuer diesen Listener. IP-Adresse Gibt die IP-Adresse dieses Listeners an. Alle verfuegbaren IP-Adressen werden aufgefuehrt. IPv6-Adressen stehen in \"[ ]\". Um auf allen IPv4-Adressen zu lauschen, waehlen Sie ANY{/}. Um auf allen IPv4- und IPv6-Adres..."
    },
    {
        "title": "Admin-Listener SSL",
        "url": "AdminListeners_SSL_Help.html",
        "text": "Admin-Listener SSL Admin-Listener sind dem Admin Server zugeordnet. Sichere (SSL-)Listener werden für den Admin Server empfohlen. Privater SSL-Schluessel und Zertifikat Jeder SSL-Listener benoetigt einen passenden privaten SSL-Schluessel und ein SSL-Zertifikat. Mehrere SSL-Listener koennen denselben Schluessel und dasselbe Zertifikat gemeinsam verwenden. Sie koennen private SSL-Schluessel selbst mit einem SSL-Softwa..."
    }
];
