#!/bin/sh

inst_admin_php()
{
    # detect download method
    OS=`uname -s`
    OSTYPE=`uname -m`

    DLCMD="wget -nv -O"
    if [ "x$OS" = "xFreeBSD" ] ; then
        DL=`which fetch`
        DLCMD="$DL -o"
    fi
    if [ "x$DLCMD" = "x" ] ; then
        DL=`which wget`
        DLCMD="$DL -nv -O"
    fi
    if [ "x$DLCMD" = "x" ] ; then
        DL=`which curl`
        DLCMD="$DL -L -o"
    fi

    echo "DLCMD is $DLCMD"
    echo

    HASADMINPHP=n
    if [ -f "$LSWS_HOME/admin/fcgi-bin/admin_php" ] ; then
        mv "$LSWS_HOME/admin/fcgi-bin/admin_php" "$LSWS_HOME/admin/fcgi-bin/admin_php.bak"
        echo "admin_php found and mv to admin_php.bak"
    fi

    if [ ! -d "$LSWS_HOME/admin/fcgi-bin/" ] ; then
        mkdir -p "$LSWS_HOME/admin/fcgi-bin/"
        echo "Mkdir $LSWS_HOME/admin/fcgi-bin/ for installing admni_php"
    fi
        
    if [ "x$OS" = "xLinux" ] ; then
        if [ "x$OSTYPE" != "xx86_64" ] ; then
            $DLCMD $LSWS_HOME/admin/fcgi-bin/admin_php http://www.litespeedtech.com/packages/lsphp5_bin/i386/lsphp5
        else
            $DLCMD $LSWS_HOME/admin/fcgi-bin/admin_php http://www.litespeedtech.com/packages/lsphp5_bin/x86_64/lsphp5
        fi
        
        if [ $? = 0 ] ; then 
            HASADMINPHP=y
            echo "admin_php downloaded."
        fi
        
#        if [ -f  "$LSWS_HOME/admin/fcgi-bin/admin_php" ] ; then
#            HASADMINPHP=y
#        fi

    elif [ "x$OS" = "xFreeBSD" ] ; then
        if [ "x$OSTYPE" != "xamd64" ] ; then
           $DLCMD $LSWS_HOME/admin/fcgi-bin/admin_php http://www.litespeedtech.com/packages/lsphp5_bin/i386-freebsd/lsphp5
        else
           $DLCMD $LSWS_HOME/admin/fcgi-bin/admin_php http://www.litespeedtech.com/packages/lsphp5_bin/x86_64-freebsd/lsphp5
        fi
       
        if [ $? = 0 ] ; then 
           HASADMINPHP=y
           echo "admin_php downloaded."
        fi
    fi

    if [ "x$HASADMINPHP" = "xn" ] ; then
        echo -e "\033[38;5;148mStart to build php, this may take several minutes, please waiting ...\033[39m"
        $LSWS_HOME/admin/misc/build_admin_php.sh
    else
        chmod "$EXEC_MOD" "$LSWS_HOME/admin/fcgi-bin/admin_php"
    fi

    #final checking of existence of admin_php
    if [ ! -f "$LSWS_HOME/admin/fcgi-bin/admin_php" ] ; then
        echo -e "\033[38;5;148mFinal checking found admin_php not exists, installation abort.\033[39m"
        exit 1
    fi
}



#script start here
cd `dirname "$0"`
source ./functions.sh 2>/dev/null
if [ $? != 0 ]; then
    . ./functions.sh
    if [ $? != 0 ]; then
        echo [ERROR] Can not include 'functions.sh'.
        exit 1
    fi
fi

#If install.sh in admin/misc, need to change directory
LSINSTALL_DIR=`dirname "$0"`
#cd $LSINSTALL_DIR/


init
LSWS_HOME=$1

WS_USER=$2
if [ "x$WS_USER" = "xyes" ] ; then 
    WS_USER=nobody
fi

WS_GROUP=$3
if [ "x$WS_GROUP" = "xyes" ] ; then 
    WS_GROUP=nobody
fi

ADMIN_USER=$4
if [ "x$ADMIN_USER" = "xyes" ] ; then 
    ADMIN_USER=admin
fi

PASS_ONE=$5
if [ "x$PASS_ONE" = "xyes" ] ; then 
    PASS_ONE=123456
fi

ADMIN_EMAIL=$6
if [ "x$ADMIN_EMAIL" = "xyes" ] ; then 
    ADMIN_EMAIL=root@localhost
fi

ADMIN_SSL=$7
ADMIN_PORT=$8
if [ "x$ADMIN_PORT" = "xyes" ] ; then 
    ADMIN_PORT=7080
fi


VERSION=open
HTTP_PORT=8088
SETUP_PHP=1
PHP_SUFFIX="php"
SSL_HOSTNAME=""

DEFAULT_USER="nobody"
DEFAULT_GROUP="nobody"
grep -q nobody: "/etc/group"
if  [ $? != 0 ] ; then
    DEFAULT_GROUP="nogroup"
fi

if [ "$WS_GROUP" = "nobody" ] ; then
    WS_GROUP=$DEFAULT_GROUP
fi


PHP_INSTALLED=n
INSTALL_TYPE="reinstall"
if [ -f "$LSWS_HOME/conf/httpd_config.xml" ] ; then

    printf '\033[31;42m\e[5mWarning:\e[25m\033[0m\033[31m This version uses a plain text configuration file which can also be modified by hand.\n\033[0m '
    printf '\033[31m \tThe XML configuration file for your current version (1.3.x or below) will be converted\n\tby the installation program to this format and a copy will be made of your current XML\n\033[0m '
    printf '\033[31m \tfile named <filename>.xml.bak. If you have any installed modules, they will need to be\n\trecompiled to comply with the upgraded API.\n\tAre you sure you want to upgrade to this version? [Yes/No]\033[0m '
    read Overwrite_Old
    echo

    if [ "x$Overwrite_Old" = "x" ]; then
        Overwrite_Old=No
    fi

    if [ "x$Overwrite_Old" != "xYes" ] && [ "x$Overwrite_Old" != "xY" ] ; then
        echo "Abort installation!" 
        exit 0
    fi
    echo
    
    echo -e "\033[38;5;148m$LSWS_HOME/conf/httpd_config.xml exists, will be converted to $LSWS_HOME/conf/httpd_config.conf!\033[39m"
    inst_admin_php
    PHP_INSTALLED=y
    
    if [ -e "$LSWS_HOME/conf/httpd_config.conf" ] ; then
        mv "$LSWS_HOME/conf/httpd_config.conf" "$LSWS_HOME/conf/httpd_config.conf.old"
    fi
    
    if [ -e "$LSWS_HOME/DEFAULT/conf/vhconf.conf" ] ; then
        mv "$LSWS_HOME/DEFAULT/conf/vhconf.conf" "$LSWS_HOME/DEFAULT/conf/vhconf.conf.old"
    fi

    if [ ! -d "$LSWS_HOME/backup" ] ; then
        mkdir "$LSWS_HOME/backup"
    fi

    $LSINSTALL_DIR/admin/misc/convertxml.sh $LSWS_HOME
    if [ -e "$LSWS_HOME/conf/httpd_config.xml" ] ; then
        rm "$LSWS_HOME/conf/httpd_config.xml"
    fi
fi

if [ -f "$LSWS_HOME/conf/httpd_config.conf" ] ; then
    INSTALL_TYPE="upgrade"
    #Now check if the user and group match with the conf file
    OLD_USER_CONF=`grep "user" "$LSWS_HOME/conf/httpd_config.conf"`
    OLD_GROUP_CONF=`grep "group" "$LSWS_HOME/conf/httpd_config.conf"`
    OLD_USER=`expr "$OLD_USER_CONF" : '\s*user\s*\(\S*\)'`
    OLD_GROUP=`expr "$OLD_GROUP_CONF" : '\s*group\s*\(\S*\)'`
    
    if [ "$WS_USER" = "$DEFAULT_USER" ] && [ "$WS_GROUP" = "$DEFAULT_GROUP" ] ; then
        WS_USER=$OLD_USER
        WS_GROUP=$OLD_GROUP
    fi
    
    if [ "$OLD_USER" != "$WS_USER" ] || [ "$OLD_GROUP" != "$WS_GROUP" ]; then
        echo -e "\033[38;5;148m$LSWS_HOME/conf/httpd_config.conf exists, but the user/group do not match, installing abort!\033[39m"
        echo -e "\033[38;5;148mYou may change the user/group or remove the direcoty $LSWS_HOME and re-install.\033[39m"
        exit 1
    fi
fi

echo "INSTALL_TYPE is $INSTALL_TYPE"

DIR_OWN=$WS_USER:$WS_GROUP
CONF_OWN=$WS_USER:$WS_GROUP
configRuby


#Comment out the below two lines
echo "Target_Dir:$LSWS_HOME User:$WS_USER Group:$WS_GROUP Admin:$ADMIN_USER Password:$PASS_ONE LSINSTALL_DIR:$LSINSTALL_DIR AdminSSL:$ADMIN_SSL ADMIN_PORT:$ADMIN_PORT END"

echo
echo -e "\033[38;5;148mInstalling, please wait...\033[39m"
echo



if [ "x$ADMIN_SSL" = "xyes" ] ; then
    echo "Admin SSL enabled!"
    gen_selfsigned_cert ../adminssl.conf
    cp $LSINSTALL_DIR/${SSL_HOSTNAME}.crt $LSINSTALL_DIR/admin/conf/${SSL_HOSTNAME}.crt
    cp $LSINSTALL_DIR/${SSL_HOSTNAME}.key $LSINSTALL_DIR/admin/conf/${SSL_HOSTNAME}.key
else
    echo "Admin SSL disabled!"
fi

buildConfigFiles
installation

if [ "x$PHP_INSTALLED" = "xn" ] ; then
    inst_admin_php
fi

rm $LSWS_HOME/bin/lshttpd
ln -sf ./openlitespeed $LSWS_HOME/bin/lshttpd


if [ ! -f "$LSWS_HOME/admin/conf/htpasswd" ] ; then
    ENCRYPT_PASS=`"$LSWS_HOME/admin/fcgi-bin/admin_php" -q "$LSWS_HOME/admin/misc/htpasswd.php" $PASS_ONE`
    echo "$ADMIN_USER:$ENCRYPT_PASS" > "$LSWS_HOME/admin/conf/htpasswd"
fi


if [ ! -f "$LSWS_HOME/fcgi-bin/lsphp" ]; then
    cp -f "$LSWS_HOME/admin/fcgi-bin/admin_php" "$LSWS_HOME/fcgi-bin/lsphp"
    chown "$SDIR_OWN" "$LSWS_HOME/fcgi-bin/lsphp"
    chmod "$EXEC_MOD" "$LSWS_HOME/fcgi-bin/lsphp"
    if [ ! -f "$LSWS_HOME/fcgi-bin/lsphp5" ]; then
        ln -sf "./lsphp" "$LSWS_HOME/fcgi-bin/lsphp5"
    fi
fi


#compress_admin_file
if [ ! -f "$LSWS_HOME/admin/conf/jcryption_keypair" ]; then
    $LSWS_HOME/admin/misc/create_admin_keypair.sh
fi

chown "$CONF_OWN" "$LSWS_HOME/admin/conf/jcryption_keypair"
chmod 0600 "$LSWS_HOME/admin/conf/jcryption_keypair"

chown "$CONF_OWN" "$LSWS_HOME/admin/conf/htpasswd"
chmod 0600 "$LSWS_HOME/admin/conf/htpasswd"


#for root user, we'll try to start it automatically
INST_USER=`id`
INST_USER=`expr "$INST_USER" : 'uid=.*(\(.*\)) gid=.*'`
if [ $INST_USER = "root" ]; then
    $LSWS_HOME/admin/misc/rc-inst.sh
fi



echo
echo -e "\033[38;5;148mInstallation finished, Enjoy!\033[39m"
echo


