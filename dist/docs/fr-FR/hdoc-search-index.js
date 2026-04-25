window.HDOC_SEARCH_INDEX = [
    {
        "title": "Accueil",
        "url": "index.html",
        "text": "Accueil OpenLiteSpeed Web Server 1.9 Manuel utilisateur — Rev. 3 Table des matieres Licence Introduction Installation/desinstallation Administration Securite Configuration Pour plus d'informations, consultez notre site de documentation OpenLiteSpeed"
    },
    {
        "title": "Licence",
        "url": "license.html",
        "text": "Licence GNU GENERAL PUBLIC LICENSE v3 GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 Copyright (C) 2007 Free Software Foundation, Inc. <http://fsf.org/> Everyone is permitted to copy and distribute verbatim copies of this license document, but changing it is not allowed. Preamble The GNU General Public License is a free, copyleft license for software and other kinds of works. The licenses for most software and ot..."
    },
    {
        "title": "Introduction",
        "url": "intro.html",
        "text": "Introduction Introduction OpenLiteSpeed (OLS) est un serveur web open source a hautes performances. Il constitue une alternative gratuite a LiteSpeed Web Server Enterprise et offre une compatibilite avec les regles de reecriture Apache. OLS utilise une architecture evenementielle et inclut un moteur de cache integre (LSCache). Vue d'ensemble OpenLiteSpeed prend en charge les protocoles modernes, notamment HTTP/2 et H..."
    },
    {
        "title": "Installation",
        "url": "install.html",
        "text": "Installation Installation/Desinstallation Configuration systeme minimale Systemes d'exploitation pris en charge OpenLiteSpeed prend en charge les versions actuelles et non EOL des distributions Linux suivantes : CentOS* 8, 9, 10 Debian 11, 12, 13 Ubuntu 20 (EOL le 31 mai 2025), 22, 24 * Inclut RedHat Enterprise Linux et ses derives, AlmaLinux, CloudLinux, Oracle Linux, RockyLinux, VzLinux, etc. CPU Intel : x86-64 ARM..."
    },
    {
        "title": "Administration",
        "url": "admin.html",
        "text": "Administration Administration OpenLiteSpeed peut etre controle de trois manieres : via la console WebAdmin, via la ligne de commande ou en envoyant des signaux. Via la console WebAdmin La console WebAdmin est un panneau de controle centralise permettant de controler et de configurer tous les parametres d'OpenLiteSpeed. Connectez-vous a la console WebAdmin (par defaut https://[adresse de votre site]:7080/). Vous y tro..."
    },
    {
        "title": "Sécurité",
        "url": "security.html",
        "text": "Sécurité Securite LiteSpeed Web Server est concu avec la securite comme consideration principale. LSWS prend en charge SSL, dispose d'un controle d'acces aux niveaux serveur et virtual host, et d'une protection realm specifique au contexte. Outre ces fonctions standard, LSWS possede egalement les fonctions de securite speciales suivantes: Limites au niveau connexion: La limitation au niveau IP limite la bande passa..."
    },
    {
        "title": "Configuration",
        "url": "config.html",
        "text": "Configuration Configuration Concepts Voici quelques concepts de base a connaitre avant d'entrer dans le detail de la configuration. Virtual hosts LiteSpeed Web Server peut heberger plusieurs sites web (virtual hosts) avec une seule instance serveur. Traditionnellement, les virtual hosts sont classes en deux types: les virtual hosts bases sur IP et les virtual hosts bases sur le nom. Les virtual hosts bases sur IP son..."
    },
    {
        "title": "Console web",
        "url": "webconsole.html",
        "text": "Console web Console web La section Console web controle les reglages de la console WebAdmin. Certains de ces reglages incluent: Expiration de session Reglages des journaux Controle d'acces Creer/supprimer des utilisateurs admin Reinitialiser les mots de passe admin Listeners WebAdmin et reglages SSL"
    },
    {
        "title": "Compiler PHP",
        "url": "CompilePHP_Help.html",
        "text": "Compiler PHP Variables d'environnement PATH supplémentaires Valeurs PATH supplémentaires qui seront ajoutées aux variables d'environnement PATH actuelles pour les scripts de compilation. Préfixe du chemin d'installation Définit la valeur de l'option de configuration \"--prefix\". L'emplacement d'installation par défaut se trouve sous le répertoire d'installation de LiteSpeed Web Server. LiteSpeed Web Server peut..."
    },
    {
        "title": "Serveur - General",
        "url": "ServGeneral_Help.html",
        "text": "Serveur - General Reglages generaux pour l'ensemble du serveur. Lorsqu'un reglage requiert des informations de chemin, celui-ci peut etre absolu ou relatif a $SERVER_ROOT. $SERVER_ROOT est l'emplacement ou le serveur web LiteSpeed a ete installe (par exemple your_home_dir/lsws ou /opt/lsws). L'executable du serveur se trouve sous $SERVER_ROOT/bin. Nom du serveur Nom unique pour ce serveur. S’il est vide, le hostnam..."
    },
    {
        "title": "Journal du serveur",
        "url": "ServLog_Help.html",
        "text": "Journal du serveur Nom du fichier Spécifie le chemin du fichier journal. Placez le fichier journal sur un disque séparé. Niveau de journalisation Spécifie le niveau de journalisation à inclure dans le fichier journal. Les niveaux disponibles (du plus élevé au plus bas) sont: ERROR{/}, WARNING{/}, NOTICE{/}, INFO{/} et DEBUG{/}. Seuls les messages dont le niveau est supérieur ou égal au paramètre actuel sero..."
    },
    {
        "title": "Serveur - Réglages",
        "url": "ServTuning_Help.html",
        "text": "Serveur - Réglages Répertoire SHM par défaut Change le répertoire par défaut de la mémoire partagée vers le chemin spécifié. Si le répertoire n’existe pas, il sera créé. Toutes les données SHM seront stockées dans ce répertoire sauf indication contraire. Protocole PROXY Liste d’IP/sous-réseaux pour les proxies front-end qui communiquent avec ce serveur à l’aide du protocole PROXY. Une fois déf..."
    },
    {
        "title": "Serveur - Sécurité",
        "url": "ServSecurity_Help.html",
        "text": "Serveur - Sécurité Suivre les liens symboliques Spécifie le réglage par défaut au niveau serveur pour suivre les liens symboliques lors du service de fichiers statiques. Les choix sont Yes{/}, If Owner Match{/} et No{/}. Yes{/} configure le serveur pour toujours suivre les liens symboliques. If Owner Match{/} configure le serveur pour suivre un lien symbolique uniquement si le propriétaire du lien et celui de l..."
    },
    {
        "title": "Applications externes",
        "url": "ExtApp_Help.html",
        "text": "Applications externes Le serveur web LiteSpeed peut transmettre des requetes a des applications externes pour traiter et generer du contenu dynamique. Depuis la version 2.0, LiteSpeed Web Server prend en charge sept types d'applications externes: CGI, FastCGI, serveur web, moteur servlet, application LiteSpeed SAPI, equilibreur de charge et logger par tube. CGI signifie Common Gateway Interface. Le standard actuel es..."
    },
    {
        "title": "Gestionnaire de scripts",
        "url": "ScriptHandler_Help.html",
        "text": "Gestionnaire de scripts Le gestionnaire de scripts spécifie comment le serveur traite les scripts ayant des suffixes donnés. Chaque définition associe un suffixe de script à un type de gestionnaire et, le cas échéant, à une application externe précédemment définie. Suffixes Spécifie les suffixes des fichiers de script qui seront gérés par ce gestionnaire de scripts. Les suffixes doivent être uniques. Le..."
    },
    {
        "title": "Serveur d'applications",
        "url": "App_Server_Help.html",
        "text": "Serveur d'applications Paramètres par défaut Rack/Rails Configurations par defaut pour les applications Rack/Rails. Ces reglages peuvent etre remplaces au niveau du contexte. Chemin Ruby Chemin vers l'exécutable Ruby. Généralement, il s'agit de /usr/bin/ruby ou /usr/local/bin/ruby selon l'emplacement d'installation de Ruby. Mode d'exécution Indique le mode d'exécution de l'application: \"Development\", \"Producti..."
    },
    {
        "title": "Configuration des modules",
        "url": "Module_Help.html",
        "text": "Configuration des modules La prise en charge des modules est disponible dans OpenLiteSpeed 1.3 et LSWS Enterprise 5.0 ou version superieure. Tous les modules requis doivent etre enregistres sous l'onglet Server Modules Configuration. Les fichiers de module doivent se trouver dans le dossier server root/modules pour pouvoir etre enregistres. Au demarrage, le serveur charge tous les modules enregistres. Le serveur doit..."
    },
    {
        "title": "Listeners - Général",
        "url": "Listeners_General_Help.html",
        "text": "Listeners - Général Nom du listener Un nom unique pour ce listener. Adresse IP Indique l'adresse IP de ce listener. Toutes les adresses IP disponibles sont listees. Les adresses IPv6 sont placees entre \"[ ]\". Pour ecouter sur toutes les adresses IPv4, selectionnez ANY{/}. Pour ecouter sur toutes les adresses IPv4 et IPv6, selectionnez [ANY]{/}. Pour servir a la fois les clients IPv4 et IPv6, une adresse IPv6 mappee..."
    },
    {
        "title": "Listeners - SSL",
        "url": "Listeners_SSL_Help.html",
        "text": "Listeners - SSL Cle privee SSL et certificat Chaque listener SSL requiert une cle privee SSL et un certificat SSL associes. Plusieurs listeners SSL peuvent partager la meme cle et le meme certificat. Vous pouvez generer vous-meme des cles privees SSL avec un paquet logiciel SSL, comme OpenSSL. Les certificats SSL peuvent aussi etre achetes aupres d'une autorite de certification autorisee comme VeriSign ou Thawte. Vou..."
    },
    {
        "title": "Modeles de virtual hosts",
        "url": "Templates_Help.html",
        "text": "Modeles de virtual hosts Les modeles de virtual hosts facilitent la creation de nombreux nouveaux virtual hosts avec des configurations similaires. Chaque modele contient un fichier de configuration de modele, une liste de listeners mappes et une liste de virtual hosts membres. Pour ajouter un virtual host base sur un modele, l'administrateur doit seulement ajouter un membre avec un nom de virtual host unique et un n..."
    },
    {
        "title": "Enregistrement du VHost",
        "url": "VirtualHosts_Help.html",
        "text": "Enregistrement du VHost Cette page liste tous les virtual hosts definis. Depuis cette page, vous pouvez ajouter/supprimer un virtual host ou modifier un virtual host existant. Avant d'ajouter un virtual host, assurez-vous que le repertoire racine du virtual host existe. Nom de l'hote virtuel Nom unique d'un hote virtuel. Il est recommande d'utiliser le nom de domaine de l'hote virtuel comme nom d'hote virtuel. Le nom..."
    },
    {
        "title": "Hôte virtuel - Général",
        "url": "VHGeneral_Help.html",
        "text": "Hôte virtuel - Général Racine des documents Indique la racine des documents de cet hôte virtuel. $VH_ROOT/html{/} est recommandé. Dans les contextes, ce répertoire est référencé comme $DOC_ROOT. E-mail de l'administrateur Indique les adresses e-mail des administrateurs de cet hôte virtuel. Activer la compression Indique s’il faut activer la compression GZIP pour cet hôte virtuel. Ce réglage ne prend eff..."
    },
    {
        "title": "Hôte virtuel - Sécurité",
        "url": "VHSecurity_Help.html",
        "text": "Hôte virtuel - Sécurité Protection CAPTCHA CAPTCHA Protection est un service fourni pour aider a attenuer une forte charge serveur. CAPTCHA Protection s'activera apres l'une des situations ci-dessous. Une fois actif, toutes les requetes des clients NON TRUSTED(tels que configures) seront redirigees vers une page de validation CAPTCHA. Apres validation, le client sera redirige vers la page souhaitee. Les situations..."
    },
    {
        "title": "Hôte virtuel - SSL",
        "url": "VHSSL_Help.html",
        "text": "Hôte virtuel - SSL Cle privee SSL et certificat Chaque listener SSL requiert une cle privee SSL et un certificat SSL associes. Plusieurs listeners SSL peuvent partager la meme cle et le meme certificat. Vous pouvez generer vous-meme des cles privees SSL avec un paquet logiciel SSL, comme OpenSSL. Les certificats SSL peuvent aussi etre achetes aupres d'une autorite de certification autorisee comme VeriSign ou Thawte...."
    },
    {
        "title": "Rewrite",
        "url": "Rewrite_Help.html",
        "text": "Rewrite Activer rewrite Indique s’il faut activer le moteur de réécriture d’URL de LiteSpeed. Cette option peut être personnalisée au niveau hôte virtuel ou contexte, et elle est héritée le long de l’arborescence de répertoires jusqu’à être explicitement remplacée. Chargement automatique depuis .htaccess Charge automatiquement les regles rewrite contenues dans le fichier .htaccess d’un repertoire..."
    },
    {
        "title": "Contexte",
        "url": "Context_Help.html",
        "text": "Contexte Dans la terminologie de LiteSpeed Web Server, un \"context\" est un emplacement virtuel, une URL parente commune, qui identifie un groupe de ressources. Les contextes peuvent etre vus comme differents repertoires dans l'arborescence de votre site web. Par exemple, \"/\" est le contexte racine mappe au document root d'un site web. \"/cgi-bin/\" est un contexte situe plus haut dans l'arborescence, dedie aux applicat..."
    },
    {
        "title": "Proxy WebSocket",
        "url": "VHWebSocket_Help.html",
        "text": "Proxy WebSocket WebSocket est un protocole qui peut etre utilise a la place de HTTP pour fournir une communication bidirectionnelle en temps reel sur Internet. A partir de la version 1.1.1, OpenLiteSpeed prend en charge les backends WebSocket au moyen de proxies WebSocket. Ces proxies envoient la communication WebSocket au backend approprie indique dans le champ Adresse. URI Indique les URI qui utiliseront ce backend..."
    },
    {
        "title": "Console WebAdmin - Gestionnaire de services",
        "url": "ServerStat_Help.html",
        "text": "Console WebAdmin - Gestionnaire de services Le Gestionnaire de services sert de salle de contrôle pour surveiller le serveur et contrôler certaines fonctions de niveau supérieur. Il fournit les fonctions suivantes: (Le Gestionnaire de services est accessible depuis le menu Actions ou depuis la page d'accueil.) Surveiller l'état actuel du serveur, des listeners et des hôtes virtuels. Appliquer les modifications d..."
    },
    {
        "title": "Statistiques en temps réel",
        "url": "Real_Time_Stats_Help.html",
        "text": "Statistiques en temps réel IP bloquée par Anti-DDoS Liste d'adresses IP séparées par des virgules et bloquées par la protection Anti-DDoS. Chaque adresse se termine par un point-virgule et un code de motif indiquant pourquoi l'adresse IP a été bloquée. Codes de motif possibles: A{/}: BOT_UNKNOWN B{/}: BOT_OVER_SOFT C{/}: BOT_OVER_HARD D{/}: BOT_TOO_MANY_BAD_REQ E{/}: BOT_CAPTCHA F{/}: BOT_FLOOD G{/}: BOT_REWR..."
    },
    {
        "title": "Application LSAPI externe",
        "url": "External_LSAPI.html",
        "text": "Application LSAPI externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Adresse Adresse de socket unique utilisee par l'application externe. Les sockets IPv4/IPv6 et Unix Domain Sockets (UDS) sont pris en charge. Les sockets IPv4/IPv6 peuvent etre utilises pour la communication sur le reseau. UDS ne peut etre utilise que lo..."
    },
    {
        "title": "Serveur web externe",
        "url": "External_WS.html",
        "text": "Serveur web externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Adresse Adresse HTTP, HTTPS ou Unix Domain Sockets (UDS) utilisee par le serveur web externe. Si vous faites proxy vers un autre serveur web execute sur la meme machine avec une adresse IPv4/IPv6, definissez l'adresse IP sur localhost{/} ou 127.0.0.1{/}, afin..."
    },
    {
        "title": "Application FastCGI externe",
        "url": "External_FCGI.html",
        "text": "Application FastCGI externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Adresse Adresse de socket unique utilisee par l'application externe. Les sockets IPv4/IPv6 et Unix Domain Sockets (UDS) sont pris en charge. Les sockets IPv4/IPv6 peuvent etre utilises pour la communication sur le reseau. UDS ne peut etre utilise que ..."
    },
    {
        "title": "Authentificateur FastCGI externe",
        "url": "External_FCGI_Auth.html",
        "text": "Authentificateur FastCGI externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Adresse Adresse de socket unique utilisee par l'application externe. Les sockets IPv4/IPv6 et Unix Domain Sockets (UDS) sont pris en charge. Les sockets IPv4/IPv6 peuvent etre utilises pour la communication sur le reseau. UDS ne peut etre utilise..."
    },
    {
        "title": "Application SCGI externe",
        "url": "External_SCGI.html",
        "text": "Application SCGI externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Adresse Adresse de socket unique utilisee par l'application externe. Les sockets IPv4/IPv6 et Unix Domain Sockets (UDS) sont pris en charge. Les sockets IPv4/IPv6 peuvent etre utilises pour la communication sur le reseau. UDS ne peut etre utilise que lor..."
    },
    {
        "title": "Application Servlet externe",
        "url": "External_Servlet.html",
        "text": "Application Servlet externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Adresse Adresse de socket unique utilisee par l'application externe. Les sockets IPv4/IPv6 et Unix Domain Sockets (UDS) sont pris en charge. Les sockets IPv4/IPv6 peuvent etre utilises pour la communication sur le reseau. UDS ne peut etre utilise que ..."
    },
    {
        "title": "Application proxy externe",
        "url": "External_PL.html",
        "text": "Application proxy externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Adresse du journaliseur distant (optionnel) Indique l'adresse de socket facultative pour ce journaliseur par tube. Definissez cette valeur lorsque le journaliseur est accessible par un socket reseau ou Unix Domain Socket. Laissez ce champ vide pour un j..."
    },
    {
        "title": "Application d'équilibrage externe",
        "url": "External_LB.html",
        "text": "Application d'équilibrage externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Workers Liste des groupes de workers deja definis dans l'equilibreur de charge externe."
    },
    {
        "title": "Application uWSGI externe",
        "url": "External_UWSGI.html",
        "text": "Application uWSGI externe Nom Un nom unique pour cette application externe. Vous utiliserez ce nom pour y faire reference dans les autres parties de la configuration. Adresse Adresse de socket unique utilisee par l'application externe. Les sockets IPv4/IPv6 et Unix Domain Sockets (UDS) sont pris en charge. Les sockets IPv4/IPv6 peuvent etre utilises pour la communication sur le reseau. UDS ne peut etre utilise que lo..."
    },
    {
        "title": "Contexte statique",
        "url": "Static_Context.html",
        "text": "Contexte statique Contexte général Les parametres de contexte servent a definir des parametres speciaux pour les fichiers situes a un emplacement donne. Ils peuvent integrer des fichiers en dehors de la racine des documents (comme les directives Alias ou AliasMatch d'Apache), proteger un repertoire au moyen de realms d'autorisation, ou bloquer ou restreindre l'acces a un repertoire dans la racine des documents. URI..."
    },
    {
        "title": "Contexte d'application web Java",
        "url": "Java_Web_App_Context.html",
        "text": "Contexte d'application web Java Contexte d'application web Java Beaucoup de personnes executant des applications Java utilisent egalement le moteur de servlets pour servir du contenu statique. Toutefois, aucun moteur de servlets n'est aussi efficace que LiteSpeed Web Server pour ces operations. Pour ameliorer les performances globales, LiteSpeed Web Server peut etre configure comme serveur passerelle: il sert le cont..."
    },
    {
        "title": "Contexte Servlet",
        "url": "Servlet_Context.html",
        "text": "Contexte Servlet Contexte Servlet Les servlets peuvent etre importes individuellement au moyen de contextes de servlet. Un contexte de servlet indique simplement l'URI du servlet et le nom du moteur de servlets. Utilisez-le uniquement si vous ne voulez pas importer toute l'application web ou si vous voulez proteger differents servlets avec differents realms d'autorisation. Cette URI a les memes exigences que pour un ..."
    },
    {
        "title": "Contexte FastCGI",
        "url": "FCGI_Context.html",
        "text": "Contexte FastCGI Contexte FastCGI Les applications FastCGI ne peuvent pas etre utilisees directement. Une application FastCGI doit etre configuree comme gestionnaire de scripts ou associee a une URL au moyen d'un contexte FastCGI. Un contexte FastCGI associe une URI a une application FastCGI. URI Indique l’URI de ce contexte. Application FastCGI Indique le nom de l’application FastCGI. Cette application doit etre..."
    },
    {
        "title": "Contexte SCGI",
        "url": "SCGI_Context.html",
        "text": "Contexte SCGI Contexte SCGI Les applications SCGI ne peuvent pas etre utilisees directement. Une application SCGI doit etre configuree comme gestionnaire de scripts ou associee a une URL au moyen d'un contexte SCGI. Un contexte SCGI associe une URI a une application SCGI. URI Indique l’URI de ce contexte. Application SCGI Indique le nom de l’application SCGI. Cette application doit etre definie dans la section au..."
    },
    {
        "title": "Contexte LSAPI",
        "url": "LSAPI_Context.html",
        "text": "Contexte LSAPI Contexte LSAPI Les applications externes ne peuvent pas etre utilisees directement. Elles doivent etre configurees comme gestionnaire de scripts ou associees a une URL au moyen d'un contexte. Un contexte LiteSpeed SAPI associe une URI a une application LSAPI (LiteSpeed Server Application Programming Interface). Des modules LSAPI existent actuellement pour PHP, Ruby et Python. Comme LSAPI est developpe ..."
    },
    {
        "title": "Contexte proxy",
        "url": "Proxy_Context.html",
        "text": "Contexte proxy Contexte proxy Un contexte proxy permet a cet hote virtuel d'agir comme serveur proxy inverse transparent. Ce serveur proxy peut etre place devant tout serveur web ou serveur d'applications prenant en charge le protocole HTTP. Le serveur web externe pour lequel cet hote virtuel fait office de proxy doit etre defini dans avant la configuration d'un contexte proxy. URI Indique l’URI de ce contexte. Ser..."
    },
    {
        "title": "Contexte CGI",
        "url": "CGI_Context.html",
        "text": "Contexte CGI Contexte CGI Un contexte CGI definit les scripts d'un repertoire donne comme scripts CGI. Ce repertoire peut se trouver dans la racine des documents ou en dehors. Lorsqu'un fichier de ce repertoire est demande, le serveur tente toujours de l'executer comme script CGI, qu'il soit executable ou non. Ainsi, le contenu des fichiers sous un contexte CGI reste protege et ne peut pas etre lu comme contenu stati..."
    },
    {
        "title": "Contexte d'équilibrage",
        "url": "LB_Context.html",
        "text": "Contexte d'équilibrage Contexte d'équilibrage Comme les autres applications externes, les applications worker de l'equilibreur de charge ne peuvent pas etre utilisees directement. Elles doivent etre associees a une URL au moyen d'un contexte. Un contexte d'equilibreur de charge associe une URI qui sera repartie par les workers de l'equilibreur de charge. URI Indique l’URI de ce contexte. Load Balancer Indique le ..."
    },
    {
        "title": "Contexte de redirection",
        "url": "Redirect_Context.html",
        "text": "Contexte de redirection Contexte de redirection Un contexte de redirection peut servir a transferer une URI ou un groupe d'URI vers un autre emplacement. L'URI de destination peut se trouver sur le meme site web (redirection interne) ou etre une URI absolue pointant vers un autre site web (redirection externe). URI Indique l’URI de ce contexte. Redirection externe Indique si cette redirection est externe. Pour une ..."
    },
    {
        "title": "Contexte de serveur d'applications",
        "url": "App_Server_Context.html",
        "text": "Contexte de serveur d'applications Contexte de serveur d'applications Un contexte de serveur d'applications fournit un moyen simple de configurer une application Ruby Rack/Rails, WSGI ou Node.js. Pour ajouter une application via un contexte de serveur d'applications, il suffit de monter l'URL et d'indiquer le repertoire racine de l'application. Il n'est pas necessaire de definir une application externe, d'ajouter un ..."
    },
    {
        "title": "Contexte uWSGI",
        "url": "UWSGI_Context.html",
        "text": "Contexte uWSGI Contexte uWSGI Les applications uWSGI ne peuvent pas etre utilisees directement. Une application uWSGI doit etre configuree comme gestionnaire de scripts ou associee a une URL au moyen d'un contexte uWSGI. Un contexte uWSGI associe une URI a une application uWSGI. URI Indique l’URI de ce contexte. Application uWSGI Indique le nom de l’application uWSGI. Cette application doit etre definie dans la s..."
    },
    {
        "title": "Contexte de module",
        "url": "Module_Context.html",
        "text": "Contexte de module Contexte de module Un contexte de gestionnaire de module associe une URI a un module enregistre. Les modules doivent etre enregistres dans l'onglet de configuration des modules du serveur. URI Indique l’URI de ce contexte. Module Nom du module. Le module doit etre enregistre sous l’onglet Server Module Configuration. Une fois enregistre, le nom du module sera disponible dans la liste deroulante..."
    },
    {
        "title": "Console WebAdmin - Général",
        "url": "AdminGeneral_Help.html",
        "text": "Console WebAdmin - Général Admin Server est un virtual host special dedie a la console WebAdmin. Il est tres important de s'assurer qu'Admin Server est protege de maniere sure, soit en autorisant l'acces uniquement depuis les machines des administrateurs, soit en utilisant une connexion SSL chiffree. Activer le core dump Indique s'il faut activer le core dump lorsque le serveur est demarre par l'utilisateur \"root\"...."
    },
    {
        "title": "Console WebAdmin - Sécurité",
        "url": "AdminSecurity_Help.html",
        "text": "Console WebAdmin - Sécurité Controle d'acces Indique quels sous-reseaux et/ou adresses IP peuvent acceder au serveur. Au niveau serveur, ce reglage affectera tous les virtual hosts. Vous pouvez aussi configurer un controle d'acces propre a chaque virtual host au niveau virtual host. Les reglages de niveau virtual host NE remplaceront PAS les reglages de niveau serveur. Le blocage/l'autorisation d'une IP est determi..."
    },
    {
        "title": "Listeners d'administration - Général",
        "url": "AdminListeners_General_Help.html",
        "text": "Listeners d'administration - Général Les listeners d'administration sont dédiés au serveur d'administration. Les listeners sécurisés (SSL) sont recommandés pour le serveur d'administration. Nom du listener Un nom unique pour ce listener. Adresse IP Indique l'adresse IP de ce listener. Toutes les adresses IP disponibles sont listees. Les adresses IPv6 sont placees entre \"[ ]\". Pour ecouter sur toutes les adress..."
    },
    {
        "title": "Listeners d'administration - SSL",
        "url": "AdminListeners_SSL_Help.html",
        "text": "Listeners d'administration - SSL Les listeners d'administration sont dédiés au serveur d'administration. Les listeners sécurisés (SSL) sont recommandés pour le serveur d'administration. Cle privee SSL et certificat Chaque listener SSL requiert une cle privee SSL et un certificat SSL associes. Plusieurs listeners SSL peuvent partager la meme cle et le meme certificat. Vous pouvez generer vous-meme des cles privee..."
    }
];
