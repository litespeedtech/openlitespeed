#!/bin/sh

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
WS_GROUP=$3
ADMIN_USER=$4
PASS_ONE=$5
ADMIN_EMAIL=$6

VERSION=open
HTTP_PORT=8088
ADMIN_PORT=7080
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


INSTALL_TYPE="reinstall"
if [ -f "$LSWS_HOME/conf/httpd_config.xml" ] ; then
    INSTALL_TYPE="upgrade"
    #Now check if the user and group match with the conf file
    OLD_USER_CONF=`grep "<user>" "$LSWS_HOME/conf/httpd_config.xml"`
    OLD_GROUP_CONF=`grep "<group>" "$LSWS_HOME/conf/httpd_config.xml"`
    OLD_USER=`expr "$OLD_USER_CONF" : '.*<user>\(.*\)</user>.*'`
    OLD_GROUP=`expr "$OLD_GROUP_CONF" : '.*<group>\(.*\)</group>.*'`
    
    if [ "$WS_USER" = "$DEFAULT_USER" ] && [ "$WS_GROUP" = "$DEFAULT_GROUP" ] ; then
        WS_USER=$OLD_USER
        WS_GROUP=$OLD_GROUP
    fi
    
    if [ "$OLD_USER" != "$WS_USER" ] || [ "$OLD_GROUP" != "$WS_GROUP" ]; then
        echo -e "\033[38;5;148m$LSWS_HOME/conf/httpd_config.xml exists, but the user/group do not match, installing abort!\033[39m"
        echo -e "\033[38;5;148mYou may change the user/group or remove the direcoty $LSWS_HOME and re-install.\033[39m"
        exit 1
    fi
fi

DIR_OWN=$WS_USER:$WS_GROUP
CONF_OWN=$WS_USER:$WS_GROUP
configRuby


#Comment out the below two lines
echo "Target_Dir:$LSWS_HOME User:$WS_USER Group:$WS_GROUP Admin:$ADMIN_USER Password:$PASS_ONE LSINSTALL_DIR:$LSINSTALL_DIR AdminSSL:$7 END"

echo
echo -e "\033[38;5;148mInstalling, please wait...\033[39m"
echo



if [ "x$7" = "xyes" ] ; then
    echo "Admin SSL enabled!"
    gen_selfsigned_cert ../adminssl.conf
    cp $LSINSTALL_DIR/${SSL_HOSTNAME}.crt $LSINSTALL_DIR/admin/conf/${SSL_HOSTNAME}.crt
    cp $LSINSTALL_DIR/${SSL_HOSTNAME}.key $LSINSTALL_DIR/admin/conf/${SSL_HOSTNAME}.key
else
    echo "Admin SSL disabled!"
fi

buildConfigFiles
installation

rm $LSWS_HOME/bin/lshttpd
ln -sf ./openlitespeed $LSWS_HOME/bin/lshttpd


# detect download method
OS=`uname -s`
OSTYPE=`uname -m`

DLCMD=
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

HASADMINPHP=n

if [ -f "$LSWS_HOME/admin/fcgi-bin/admin_php" ] ; then
#    echo -e "\033[38;5;148mphp already exists, needn't to re-build\033[39m"
    HASADMINPHP=y
else
    if [ "x$OS" = "xLinux" ] && [ "x$DLCMD" != "x" ]  ; then
        if [ "x$OSTYPE" != "xx86_64" ] ; then
            $DLCMD $LSWS_HOME/admin/fcgi-bin/admin_php http://www.litespeedtech.com/packages/lsphp5_bin/i386/lsphp5
        else
            $DLCMD $LSWS_HOME/admin/fcgi-bin/admin_php http://www.litespeedtech.com/packages/lsphp5_bin/x86_64/lsphp5
        fi
        
        if [ $? = 0 ] ; then 
            HASADMINPHP=y
        fi
        
#        if [ -f  "$LSWS_HOME/admin/fcgi-bin/admin_php" ] ; then
#            HASADMINPHP=y
#        fi
    fi
fi

if [ "x$HASADMINPHP" = "xn" ] ; then
    echo -e "\033[38;5;148mStart to build php, this may take several minutes, please waiting ...\033[39m"
    $LSWS_HOME/admin/misc/build_admin_php.sh
else
    chmod "$EXEC_MOD" "$LSWS_HOME/admin/fcgi-bin/admin_php"
fi

ENCRYPT_PASS=`"$LSWS_HOME/admin/fcgi-bin/admin_php" -q "$LSWS_HOME/admin/misc/htpasswd.php" $PASS_ONE`
echo "$ADMIN_USER:$ENCRYPT_PASS" > "$LSWS_HOME/admin/conf/htpasswd"



if [ -f "$LSWS_HOME/fcgi-bin/lsphp" ]; then
    mv -f "$LSWS_HOME/fcgi-bin/lsphp" "$LSWS_HOME/fcgi-bin/lsphp.old"
    echo "Your current PHP engine $LSWS_HOME/fcgi-bin/lsphp is renamed to lsphp.old"
fi

cp -f "$LSWS_HOME/admin/fcgi-bin/admin_php" "$LSWS_HOME/fcgi-bin/lsphp"
chown "$SDIR_OWN" "$LSWS_HOME/fcgi-bin/lsphp"
chmod "$EXEC_MOD" "$LSWS_HOME/fcgi-bin/lsphp"
if [ ! -f "$LSWS_HOME/fcgi-bin/lsphp5" ]; then
    ln -sf "./lsphp" "$LSWS_HOME/fcgi-bin/lsphp5"
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


