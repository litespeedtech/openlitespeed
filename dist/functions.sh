
#!/bin/sh

detectdlcmd()
{
    DLCMD=
    DL=`which wget`
    if [ $? -eq 0 ] ; then
        DLCMD="wget -nv -O "
    else
        DL=`which curl`
        if [ $? -eq 0 ] ; then
            DLCMD="curl -L -o "
        else
            DL=`which fetch`
            if [ $? -eq 0 ] ; then
                DLCMD="fetch -o "
            fi
        fi
    fi

    if [ "x$DLCMD" = "x" ] ; then
        echo "ERROR: cannot find proper download method curl/wget/fetch."
    else
        echo "download comamnd is $DLCMD"
    fi
}

init()
{
    LSINSTALL_DIR=`pwd`
    VERSION=`cat VERSION`

    export LSINSTALL_DIR

    DIR_MOD=755
    SDIR_MOD=700
    EXEC_MOD=555
    CONF_MOD=600
    DOC_MOD=644

    INST_USER=`id`
    INST_USER=`expr "$INST_USER" : 'uid=.*(\(.*\)) gid=.*'`

    ARCH=`arch`
    SYS_NAME=`uname -s`
    if [ "x$SYS_NAME" = "xFreeBSD" ] || [ "x$SYS_NAME" = "xNetBSD" ] || [ "x$SYS_NAME" = "xDarwin" ] ; then
        PS_CMD="ps -ax"
        ID_GROUPS="id"
        TEST_BIN="/bin/test"
        ROOTGROUP="wheel"
    else
        PS_CMD="ps -ef"
        ID_GROUPS="id -a"
        TEST_BIN="/usr/bin/test"
        ROOTGROUP="root"
    fi
    SETUP_PHP=0
    SET_LOGIN=0
    ADMIN_PORT=7080
    INSTALL_TYPE="upgrade"
    SERVER_NAME=`uname -n`
    ADMIN_EMAIL="root@localhost"
    AP_PORT_OFFSET=2000
    PHP_SUEXEC=2
    HOST_PANEL=""

    WS_USER=nobody
    WS_GROUP=nobody

    DIR_OWN=$WS_USER:$WS_GROUP
    CONF_OWN=$WS_USER:$WS_GROUP
    
}

license()
{
    more ./LICENSE
    cat <<EOF

IMPORTANT: In order to continue installation you must agree with above 
           license terms by typing "Yes" with capital "Y"! 

EOF
    SUCC=0
    TRY=1
    while [ $SUCC -eq "0" ]; do
        printf "%s" "Do you agree with above license? "
        read YES_NO
        if [ "x$YES_NO" != "xYes" ]; then
            if [ $TRY -lt 3 ]; then
                echo "Sorry, wrong answer! Type 'Yes' with capital 'Y', try again!"
                TRY=`expr $TRY + 1`
            else
                echo "Abort installation!"
                exit 0
            fi

        else
            SUCC=1
        fi
    done
    clear
}

readCurrentConfig()
{
    OLD_USER_CONF=`grep "user" "$LSWS_HOME/conf/httpd_config.conf"`
    OLD_GROUP_CONF=`grep "group" "$LSWS_HOME/conf/httpd_config.conf"`
    OLD_USER=`expr "$OLD_USER_CONF" : '\s*user\s*\(\S*\)'`
    OLD_GROUP=`expr "$OLD_GROUP_CONF" : '\s*group\s*\(\S*\)'`
    if [ "x$OLD_USER" != "x" ]; then
        WS_USER=$OLD_USER
    fi
    if [ "x$OLD_GROUP" != "x" ]; then
        WS_GROUP=$OLD_GROUP
    else
        D_GROUP=`$ID_GROUPS $WS_USER`
        WS_GROUP=`expr "$D_GROUP" : '.*gid=[0-9]*(\(.*\)) groups=.*'`
    fi
    DIR_OWN=$WS_USER:$WS_GROUP
    CONF_OWN=$WS_USER:$WS_GROUP

}




# Get destination directory
install_dir()
{

    SUCC=0
    INSTALL_TYPE="reinstall"
    SET_LOGIN=1

    if [ $INST_USER = "root" ]; then
        DEST_RECOM="/usr/local/lsws"
        if [ -f "/opt/lsws/conf/httpd_config.conf" ]; then
            DEST_RECOM="/opt/lsws"
        fi
        WS_USER="nobody"
    else
        cat <<EOF

As you are not the 'root' user, you may not be able to install the
web server into a system directory or enable chroot, the web server 
process will run on behalf of current user - '$INST_USER'.

EOF
        WS_USER=$INST_USER
        DEST_RECOM="~/lsws"
    fi



    while [ $SUCC -eq "0" ];  do
        cat <<EOF

Please specify the destination directory. You must have permissions to 
create and manage the directory. It is recommended to install the web server 
at /opt/lsws, /usr/local/lsws or in your home directory like '~/lsws'.

ATTENTION: The user '$WS_USER' must be able to access the destination
           directory.

EOF
        printf "%s" "Destination [$DEST_RECOM]: "
        read TMP_DEST
        echo ""
        if [ "x$TMP_DEST" = "x" ]; then
            TMP_DEST=$DEST_RECOM
        fi
        if [ `expr "$TMP_DEST" : '~'` -gt 0 ]; then
            LSWS_HOME="$HOME`echo $TMP_DEST | sed 's/^~//' `"
        else
            LSWS_HOME=$TMP_DEST
        fi
        if [ `expr "$LSWS_HOME" : '\/'` -eq 0 ]; then
            echo "[ERROR] Must be absolute path!"
        else
            SUCC=1
        fi
        if [ ! -d "$LSWS_HOME" ]; then
            mkdir "$LSWS_HOME"
            if [ ! $? -eq 0 ]; then
                SUCC=0
            fi
        fi
        if [ -f "$LSWS_HOME/conf/httpd_config.conf" ]; then
            cat <<EOF

Found old configuration file under destination directory $LSWS_HOME. 

To upgrade, press 'Enter', current configuration will not be changed.
To reinstall, press 'R' or 'r'.
To change directory, press 'C' or 'c'. 

EOF

            printf "%s" "Would you like to Upgrade, Reinstall or Change directory [U/r/c]? "
            read TMP_URC
            echo ""
            if [ "x$TMP_URC" = "x" ]; then
                INSTALL_TYPE="upgrade"
                SET_LOGIN=0
            else
                if [ `expr "$TMP_URC" : '[Uu]'` -gt 0 ]; then
                    INSTALL_TYPE="upgrade"
                    SET_LOGIN=0
                else
                    if [ `expr "$TMP_URC" : '[Rr]'` -gt 0 ]; then
                        INSTALL_TYPE="reinstall"
                        SET_LOGIN=1
                    else
                    #if [ `expr "$TMP_URC" : '[Cc]'` -gt 0 ]; then
                        SUCC=0
                    fi
                fi
            fi

        fi

    done

    export LSWS_HOME

    if [ -f "$LSWS_HOME/conf/httpd_config.conf" ]; then
        readCurrentConfig
    else
        INSTALL_TYPE="reinstall"
    fi


    DIR_OWN=$WS_USER:$WS_GROUP
    CONF_OWN=$WS_USER:$WS_GROUP

    chmod $DIR_MOD "$LSWS_HOME"
}


admin_login()
{
    if [ $INSTALL_TYPE = "upgrade" ]; then
        printf "%s" "Would you like to reset the login password for Administration Web Interface [y/N]? "
        read TMP_URC
        echo ""
        if [ "x$TMP_URC" != "x" ]; then
            if [ `expr "$TMP_URC" : '[Yy]'` -gt 0 ]; then
                SET_LOGIN=1
            fi
        fi
    fi

    if [ $SET_LOGIN -eq 1 ]; then

# get admin user name and password

        SUCC=0
        cat <<EOF

Please specify the user name of the administrator.
This is the user name required to log into the administration web interface.

EOF

        printf "%s" "User name [admin]: "
        read ADMIN_USER
        if [ "x$ADMIN_USER" = "x" ]; then
            ADMIN_USER=admin
        fi

        cat <<EOF

Please specify the administrator's password.
This is the password required to log into the administration web interface.

EOF

        while [ $SUCC -eq "0" ];  do
            printf "%s" "Password: "
            stty -echo
            read PASS_ONE
            stty echo
            echo ""
            if [ `expr "$PASS_ONE" : '.*'` -ge 6 ]; then
                printf "%s" "Retype password: "
                stty -echo
                read PASS_TWO
                stty echo
                echo ""
                if [ "x$PASS_ONE" = "x$PASS_TWO" ]; then
                    SUCC=1
                else
                    echo ""
                    echo "[ERROR] Sorry, passwords does not match. Try again!"
                    echo ""
                fi
            else
                echo ""
                echo "[ERROR] Sorry, password must be at least 6 characters!"
                echo ""
            fi
        done


# generate password file
        ENCRYPT_PASS=`"$LSINSTALL_DIR/admin/fcgi-bin/admin_php" -q "$LSINSTALL_DIR/admin/misc/htpasswd.php" $PASS_ONE`
        echo "$ADMIN_USER:$ENCRYPT_PASS" > "$LSINSTALL_DIR/admin/conf/htpasswd"

    fi

}


getUserGroup()
{

    if [ $INST_USER = "root" ]; then
        cat <<EOF

As you are the root user, you must choose the user and group
whom the web server will be running as. For security reason, you should choose
a non-system user who does not have login shell and home directory such as
'nobody'.

EOF
# get user name 
        SUCC=0
        while [ $SUCC -eq "0" ]; do
            printf "%s" "User [$WS_USER]: "
            read TMP_USER
            if [ "x$TMP_USER" = "x" ]; then
                TMP_USER=$WS_USER
            fi
            USER_INFO=`id $TMP_USER 2>/dev/null`
            TST_USER=`expr "$USER_INFO" : 'uid=.*(\(.*\)) gid=.*'`
            if [ "x$TST_USER" = "x$TMP_USER" ]; then
                USER_ID=`expr "$USER_INFO" : 'uid=\(.*\)(.*) gid=.*'`
                if [ $USER_ID -gt 10 ]; then
                    WS_USER=$TMP_USER
                    SUCC=1
                else
                    cat <<EOF

[ERROR] It is not allowed to run LiteSpeed web server on behalf of a 
privileged user, user id must be greater than 10. The user id of user 
'$TMP_USER' is '$USER_ID'.

EOF
                fi
            else
                cat <<EOF

[ERROR] '$TMP_USER' is not valid user name in your system, please choose 
another user or create user '$TMP_USER' first.

EOF

            fi
        done
    fi

# get group name 
    SUCC=0
    TMP_GROUPS=`groups $WS_USER`
    TST_GROUPS=`expr "$TMP_GROUPS" : '.*:\(.*\)'`
    if [ "x$TST_GROUPS" = "x" ]; then
        TST_GROUPS=$TMP_GROUPS
    fi

    D_GROUP=`$ID_GROUPS $WS_USER`
    D_GROUP=`expr "$D_GROUP" : '.*gid=[0-9]*(\(.*\)) groups=.*'`
    echo "Please choose the group that the web server running as."
    echo
    while [ $SUCC -eq "0" ];  do
        echo "User '$WS_USER' is the member of following group(s): $TST_GROUPS"
        printf "%s" "Group [$D_GROUP]: "
        read TMP_GROUP
        if [ "x$TMP_GROUP" = "x" ]; then
            TMP_GROUP=$D_GROUP
        fi
        GRP_RET=`echo $TST_GROUPS | grep -w "$TMP_GROUP"`
        if [ "x$GRP_RET" != "x" ]; then
            WS_GROUP=$TMP_GROUP
            SUCC=1
        else
            cat <<EOF

[ERROR] '$TMP_GROUP' is not valid group for user '$WS_USER', please choose
another group in the list or add user '$WS_USER' to group '$TMP_GROUP' 
first.

EOF
        fi
    done

    DIR_OWN=$WS_USER:$WS_GROUP
    CONF_OWN=$WS_USER:$WS_GROUP

    if [ $INST_USER = "root" ]; then
        if [ $SUCC -eq "1" ]; then
            chown -R "$DIR_OWN" "$LSWS_HOME"
        fi
    fi
}

stopLshttpd()
{
    RUNNING_PROCESS=`$PS_CMD | grep lshttpd | grep -v grep`
    if [ "x$RUNNING_PROCESS" != "x" ]; then
        cat <<EOF
LiteSpeed web server is running, in order to continue installation, the server 
must be stopped.

EOF
        printf "Would you like to stop it now? [Y/n]"
        read TMP_YN
        echo ""
        if [ "x$TMP_YN" = "x" ] || [ `expr "$TMP_YN" : '[Yy]'` -gt 0 ]; then
            $LSINSTALL_DIR/bin/lswsctrl stop
            sleep 1
            RUNNING_PROCESS=`$PS_CMD | grep lshttpd | grep -v grep`
            if [ "x$RUNNING_PROCESS" != "x" ]; then
                echo "Failed to stop server, abort installation!"
                exit 1
            fi
        else
            echo "Abort installation!"
            exit 1
        fi
    fi

}


# get normal TCP port
getServerPort()
{
    cat <<EOF

Please specify the port for normal HTTP service.
Port 80 is the standard HTTP port, only 'root' user is allowed to use 
port 80, if you have another web server running on port 80, you need to
specify another port or stop the other web server before starting LiteSpeed
Web Server.
You can access the normal web page at http://<YOUR_HOST>:<HTTP_PORT>/

EOF

    SUCC=0
    DEFAULT_PORT=8088
    while [ $SUCC -eq "0" ];  do
        printf "%s" "HTTP port [$DEFAULT_PORT]: "
        read TMP_PORT
        if [ "x$TMP_PORT" = "x" ]; then
            TMP_PORT=$DEFAULT_PORT
        fi
        SUCC=1
        if [ `expr "$TMP_PORT" : '.*[^0-9]'` -gt 0 ]; then
            echo "[ERROR] Only digits is allowed, try again!"
            SUCC=0
        fi
        if  [ $SUCC -eq 1 ]; then
            if [ $INST_USER != "root" ]; then
                if [ $TMP_PORT -le 1024 ]; then
                    echo "[ERROR] Only 'root' can use port below 1024, try again!"
                    SUCC=0
                fi
            fi
        fi
        if [ $SUCC -eq 1 ]; then
            if [ `netstat -an | grep -w $TMP_PORT | grep -w LISTEN | wc -l` -gt 0 ]; then
                echo "[ERROR] Port $TMP_PORT is in use now, stop the server using this port first,"
                echo "        or choose another port."
                SUCC=0
            fi
        fi
    done

    HTTP_PORT=$TMP_PORT
}


# get administration TCP port
getAdminPort()
{
    cat <<EOF

Please specify the HTTP port for the administration web interface,
which can be accessed through http://<YOUR_HOST>:<ADMIN_PORT>/

EOF

    SUCC=0
    DEFAULT_PORT=7080
    while [ $SUCC -eq "0" ];  do
        printf "%s" "Admin HTTP port [$DEFAULT_PORT]: "
        read TMP_PORT
        if [ "x$TMP_PORT" = "x" ]; then
            TMP_PORT=$DEFAULT_PORT
        fi
        SUCC=1
        if [ `expr "$TMP_PORT" : '.*[^0-9]'` -gt 0 ]; then
            echo "[ERROR] Only digits is allowed, try again!"
            SUCC=0
        fi
        if  [ $SUCC -eq 1 ]; then
            if [ $INST_USER != "root" ]; then
                if [ $TMP_PORT -le 1024 ]; then
                    echo "[ERROR] Only 'root' can use port below 1024, try again!"
                    SUCC=0
                fi
            fi
        fi
        if  [ $SUCC -eq 1 ]; then
            if [ $TMP_PORT -eq $HTTP_PORT ]; then
                echo "[ERROR] The admin HTTP port must be different from the normal HTTP port!"
                SUCC=0
            fi
        fi

        if [ $SUCC -eq 1 ]; then
            if [ `netstat -an | grep -w $TMP_PORT | grep -w LISTEN | wc -l` -gt 0 ]; then
                echo "[ERROR] Port $TMP_PORT is in use, stop the server that using this port first,"
                echo "        or choose another port."
                SUCC=0
            fi
        fi
    done

    ADMIN_PORT=$TMP_PORT
}

configAdminEmail()
{
        cat <<EOF

Please specify administrators' email addresses.
It is recommended to specify a real email address,
Multiple email addresses can be set by a comma 
delimited list of email addresses. Whenever something
abnormal happened, a notification will be sent to
emails listed here.

EOF

        printf "%s" "Email addresses [root@localhost]: "
        read ADMIN_EMAIL
        if [ "x$ADMIN_EMAIL" = "x" ]; then
            ADMIN_EMAIL=root@localhost
        fi

}

configRuby()
{

    if [ -x "/usr/local/bin/ruby" ]; then
        RUBY_PATH="\/usr\/local\/bin\/ruby"
    elif [ -x "/usr/bin/ruby" ]; then
        RUBY_PATH="\/usr\/bin\/ruby"
    else
        RUBY_PATH=""
        cat << EOF
Cannot find RUBY installation, remember to fix up the ruby path configuration
before you can use our easy RubyOnRails setup.

EOF
    fi
}

enablePHPHandler()
{
    cat <<EOF

You can setup a global script handler for PHP with the pre-built PHP engine
shipped with this package now. The PHP engine runs as Fast CGI which  
outperforms Apache's mod_php. 
You can always replace the pre-built PHP engine with your customized PHP 
engine.

EOF

    SUCC=0
    SETUP_PHP=1
    printf "%s" "Setup up PHP [Y/n]: "
    read TMP_YN
    if [ "x$TMP_YN" != "x" ]; then
        if [ `expr "$TMP_YN" : '[Nn]'` -gt 0 ]; then
            SETUP_PHP=0
        fi
    fi
    if [ $SETUP_PHP -eq 1 ]; then
        PHP_SUFFIX="php"
        printf "%s" "Suffix for PHP script(comma separated list) [$PHP_SUFFIX]: "
        read TMP_SUFFIX
        if [ "x$TMP_SUFFIX" != "x" ]; then
            PHP_SUFFIX=$TMP_SUFFIX
        fi
#        PHP_PORT=5101
#        SUCC=0
#        while [ $SUCC -eq "0" ];  do
#            if [ `netstat -an | grep -w $PHP_PORT | grep -w LISTEN | wc -l` -eq 0 ]; then
#                SUCC=1
#            fi
#            PHP_PORT=`expr $PHP_PORT + 1`
#        done
    fi
}





# generate configuration from template

buildConfigFiles()
{

#sed -e "s/%ADMIN_PORT%/$ADMIN_PORT/" -e "s/%PHP_FCGI_PORT%/$ADMIN_PHP_PORT/" "$LSINSTALL_DIR/admin/conf/admin_config.conf.in" > "$LSINSTALL_DIR/admin/conf/admin_config.conf"
    
    if [ "x${SSL_HOSTNAME}" != "x" ] ; then
        echo "SSL host is [${SSL_HOSTNAME}], use adminSSL"
        sed -e "s/%ADMIN_PORT%/$ADMIN_PORT/" -e "s/%SSL_HOSTNAME%/$SSL_HOSTNAME/" "$LSINSTALL_DIR/admin/conf/admin_config_ssl.conf.in" > "$LSINSTALL_DIR/admin/conf/admin_config.conf"
    else
        echo "SSL host is [${SSL_HOSTNAME}], No adminssl"
        sed -e "s/%ADMIN_PORT%/$ADMIN_PORT/" "$LSINSTALL_DIR/admin/conf/admin_config.conf.in" > "$LSINSTALL_DIR/admin/conf/admin_config.conf"
    fi
    
#    sed -e "s/%USER%/$WS_USER/" -e "s#%DEFAULT_TMP_DIR%#$DEFAULT_TMP_DIR#" -e "s/%GROUP%/$WS_GROUP/" -e "s/%ADMIN_EMAIL%/$ADMIN_EMAIL/" -e "s/%HTTP_PORT%/$HTTP_PORT/" -e  "s/%RUBY_BIN%/$RUBY_PATH/" -e "s/%SERVER_NAME%/$SERVER_NAME/" "$LSINSTALL_DIR/conf/httpd_config.conf.in" > "$LSINSTALL_DIR/conf/httpd_config.conf.tmp"
#    if [ $SETUP_PHP -eq 1 ]; then
#        sed -e "s/%PHP_BEGIN%//" -e "s/%PHP_END%//" -e "s/%PHP_SUFFIX%/$PHP_SUFFIX/" -e "s/%PHP_PORT%/$PHP_PORT/" "$LSINSTALL_DIR/conf/httpd_config.conf.tmp" > "$LSINSTALL_DIR/conf/httpd_config.conf"
#    else
#        sed -e "s/%PHP_BEGIN%/<!--/" -e "s/%PHP_END%/-->/" -e "s/%PHP_SUFFIX%/php/" -e "s/%PHP_PORT%/5201/" "$LSINSTALL_DIR/conf/httpd_config.conf.tmp" > "$LSINSTALL_DIR/conf/httpd_config.conf"
#    fi

    sed -e "s/%USER%/$WS_USER/" -e "s/%GROUP%/$WS_GROUP/" -e "s#%DEFAULT_TMP_DIR%#$DEFAULT_TMP_DIR#"  -e "s/%ADMIN_EMAIL%/$ADMIN_EMAIL/" -e "s/%HTTP_PORT%/$HTTP_PORT/" -e "s/%RUBY_BIN%/$RUBY_PATH/"  "$LSINSTALL_DIR/conf/httpd_config.conf.in" > "$LSINSTALL_DIR/conf/httpd_config.conf"

}

util_mkdir()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    for arg
      do
      if [ ! -d "$LSWS_HOME/$arg" ]; then
          mkdir "$LSWS_HOME/$arg"
      fi
      chown "$OWNER" "$LSWS_HOME/$arg"
      chmod $PERM  "$LSWS_HOME/$arg"
    done

}


util_cpfile()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    for arg
      do
      if [ -f "$LSINSTALL_DIR/$arg" ]; then
        cp -f "$LSINSTALL_DIR/$arg" "$LSWS_HOME/$arg"
        chown "$OWNER" "$LSWS_HOME/$arg"
        chmod $PERM  "$LSWS_HOME/$arg"
      fi
    done

}

util_ccpfile()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    for arg
      do
      if [ ! -f "$LSWS_HOME/$arg" ] && [ -f "$LSINSTALL_DIR/$arg" ]; then
        cp "$LSINSTALL_DIR/$arg" "$LSWS_HOME/$arg"
      fi
      if [ -f "$LSWS_HOME/$arg" ]; then
        chown "$OWNER" "$LSWS_HOME/$arg"
        chmod $PERM  "$LSWS_HOME/$arg"
      fi
    done
}


util_cpdir()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    for arg
      do
      cp -R "$LSINSTALL_DIR/$arg/"* "$LSWS_HOME/$arg/"
      chown -R "$OWNER" "$LSWS_HOME/$arg/"*
      #chmod -R $PERM  $LSWS_HOME/$arg/*
    done
}

util_cp_htaccess()
{
    OWNER=$1
    PERM=$2
    arg=$3
    cp -R "$LSINSTALL_DIR/$arg/".htaccess "$LSWS_HOME/$arg/"
    chown -R "$OWNER" "$LSWS_HOME/$arg/".htaccess
}



util_cpdirv()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    VERSION=$1
    shift
    for arg
      do
      cp -R "$LSINSTALL_DIR/$arg/"* "$LSWS_HOME/$arg.$VERSION/"
      chown -R "$OWNER" "$LSWS_HOME/$arg.$VERSION"
      $TEST_BIN -L "$LSWS_HOME/$arg"
      if [ $? -eq 0 ]; then
          rm -f "$LSWS_HOME/$arg"
      fi
      FILENAME=`basename $arg`
      ln -sf "./$FILENAME.$VERSION/" "$LSWS_HOME/$arg"
              #chmod -R $PERM  $LSWS_HOME/$arg/*
    done
}

util_cpfilev()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    VERSION=$1
    shift
    for arg
      do
      cp -f "$LSINSTALL_DIR/$arg" "$LSWS_HOME/$arg.$VERSION"
      chown "$OWNER" "$LSWS_HOME/$arg.$VERSION"
      chmod $PERM  "$LSWS_HOME/$arg.$VERSION"
      $TEST_BIN -L "$LSWS_HOME/$arg"
      if [ $? -eq 0 ]; then
          rm -f "$LSWS_HOME/$arg"
      fi
      FILENAME=`basename $arg`
      ln -sf "./$FILENAME.$VERSION" "$LSWS_HOME/$arg"
    done
}


installation1()
{
    umask 022
    if [ $INST_USER = "root" ]; then
        SDIR_OWN="root:$ROOTGROUP"
        chown $SDIR_OWN $LSWS_HOME
    else
        SDIR_OWN=$DIR_OWN
    fi
    sed "s:%LSWS_CTRL%:$LSWS_HOME/bin/lswsctrl:" "$LSINSTALL_DIR/admin/misc/lsws.rc.in" > "$LSINSTALL_DIR/admin/misc/lsws.rc"

    OWNER=$1
    PERM=$2
    shift
    shift
    for arg
      do
      if [ ! -f "$LSWS_HOME/$arg" ]; then
          cp "$LSINSTALL_DIR/$arg" "$LSWS_HOME/$arg"
      fi
      chown "$OWNER" "$LSWS_HOME/$arg"
      chmod $PERM  "$LSWS_HOME/$arg"
    done
}


util_cpdir()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    for arg
      do
      cp -R "$LSINSTALL_DIR/$arg/"* "$LSWS_HOME/$arg/"
      chown -R "$OWNER" "$LSWS_HOME/$arg/"*
      #chmod -R $PERM  $LSWS_HOME/$arg/*
    done
}



util_cpdirv()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    VERSION=$1
    shift
    for arg
      do
      if [ -d "$LSINSTALL_DIR/$arg" ]; then
        cp -R "$LSINSTALL_DIR/$arg/"* "$LSWS_HOME/$arg.$VERSION/"
        chown -R "$OWNER" "$LSWS_HOME/$arg.$VERSION"
        $TEST_BIN -L "$LSWS_HOME/$arg"
        if [ $? -eq 0 ]; then
          rm -f "$LSWS_HOME/$arg"
        fi
        FILENAME=`basename $arg`
        ln -sf "./$FILENAME.$VERSION/" "$LSWS_HOME/$arg"
              #chmod -R $PERM  $LSWS_HOME/$arg/*
      fi
    done
}

util_cpfilev()
{
    OWNER=$1
    PERM=$2
    shift
    shift
    VERSION=$1
    shift
    for arg
      do
      if [ -f "$LSINSTALL_DIR/$arg" ]; then

        cp -f "$LSINSTALL_DIR/$arg" "$LSWS_HOME/$arg.$VERSION"
        chown "$OWNER" "$LSWS_HOME/$arg.$VERSION"
        chmod $PERM  "$LSWS_HOME/$arg.$VERSION"
        $TEST_BIN -L "$LSWS_HOME/$arg"
        if [ $? -eq 0 ]; then
            rm -f "$LSWS_HOME/$arg"
        fi
        FILENAME=`basename $arg`
        ln -sf "./$FILENAME.$VERSION" "$LSWS_HOME/$arg"
      fi
    done
}

compress_admin_file()
{
    TMP_DIR=`pwd`
    cd $LSWS_HOME/admin/html
    find . | grep -e '\.js$'  | xargs -n 1 ../misc/gzipStatic.sh 9
    find . | grep -e '\.css$' | xargs -n 1 ../misc/gzipStatic.sh 9
    cd $TMP_DIR
}


create_lsadm_freebsd()
{
    pw group add lsadm
    lsadm_gid=`grep "^lsadm:" /etc/group | awk -F : '{ print $3; }'`
    pw user add -g $lsadm_gid -d / -s /usr/sbin/nologin -n lsadm 
    pw usermod lsadm -G $WS_GROUP
}

create_lsadm()
{
    groupadd lsadm 
    #1>/dev/null 2>&1
    lsadm_gid=`grep "^lsadm:" /etc/group | awk -F : '{ print $3; }'`
    useradd -g $lsadm_gid -d / -r -s /sbin/nologin lsadm 
    usermod -a -G $WS_GROUP lsadm
    #1>/dev/null 2>&1
    
}

create_lsadm_solaris()
{
    groupadd lsadm 
    #1>/dev/null 2>&1
    lsadm_gid=`grep "^lsadm:" /etc/group | awk -F: '{ print $3; }'`
    useradd -g $lsadm_gid -d / -s /bin/false lsadm 
    usermod -G $WS_GROUP lsadm

    #1>/dev/null 2>&1
    
}


installation_lscpd()
{   
    umask 022
    if [ $INST_USER = "root" ]; then
        export PATH=/sbin:/usr/sbin:$PATH
        if [ "x$SYS_NAME" = "xLinux" ]; then
            create_lsadm
        elif [ "x$SYS_NAME" = "xFreeBSD" ] || [ "x$SYS_NAME" = "xNetBSD" ]; then
            create_lsadm_freebsd
        elif [ "x$SYS_NAME" = "xSunOS" ]; then
            create_lsadm_solaris
        fi 
        grep "^lsadm:" /etc/passwd 1>/dev/null 2>&1
        if [ $? -eq 0 ]; then
            CONF_OWN="lsadm:lsadm"
        fi
        SDIR_OWN="root:$ROOTGROUP"
        LOGDIR_OWN="root:$WS_GROUP"
        chown $SDIR_OWN $LSWS_HOME
    else
        SDIR_OWN=$DIR_OWN
        LOGDIR_OWN=$DIR_OWN
    fi
    
    util_mkdir "$SDIR_OWN" $DIR_MOD  bin conf fcgi-bin php lib modules  share share/autoindex share/autoindex/icons
    util_mkdir "$LOGDIR_OWN" "0750" logs
    util_mkdir "$CONF_OWN" $SDIR_MOD   phpbuild
    util_mkdir "$DIR_OWN" $SDIR_MOD tmp tmp/ocspcache autoupdate cyberpanel
    
    find "$DEFAULT_TMP_DIR" -type s -atime +1 -delete 2>/dev/null
    if [ $? -ne 0 ]; then
        find "$DEFAULT_TMP_DIR" -type s -atime +1 2>/dev/null | xargs rm -f
    fi 

    util_cpfile "$CONF_OWN" $CONF_MOD conf/mime.properties
    util_cpdir "$CONF_OWN" $DOC_MOD share/autoindex
    util_ccpfile "$SDIR_OWN" $EXEC_MOD fcgi-bin/lsperld.fpl 
    util_cpfile "$SDIR_OWN" $DOC_MOD VERSION GPL.txt
}


install_missing_packages()
{
    if [ -f /etc/redhat-release ] ; then
        output=$(cat /etc/redhat-release)
        if echo $output | grep " 9."; then
            yum -y install libxcrypt-compat libnsl
        fi
        if [ "${ARCH}" = "aarch64" ]; then
            yum -y install libatomic
        fi
    elif [ -f /etc/debian_version ] ; then
        if [ "${ARCH}" = "aarch64" ]; then
            apt -y install libatomic1
        fi
    fi
}


installation()
{   
    umask 022
    if [ $INST_USER = "root" ]; then
        export PATH=/sbin:/usr/sbin:$PATH
        if [ "x$SYS_NAME" = "xLinux" ]; then
            create_lsadm
            install_missing_packages
        elif [ "x$SYS_NAME" = "xFreeBSD" ] || [ "x$SYS_NAME" = "xNetBSD" ]; then
            create_lsadm_freebsd
        elif [ "x$SYS_NAME" = "xSunOS" ]; then
            create_lsadm_solaris
        fi 
        grep "^lsadm:" /etc/passwd 1>/dev/null 2>&1
        if [ $? -eq 0 ]; then
            CONF_OWN="lsadm:lsadm"
        fi
        SDIR_OWN="root:$ROOTGROUP"
        LOGDIR_OWN="root:$WS_GROUP"
        chown $SDIR_OWN $LSWS_HOME
    else
        SDIR_OWN=$DIR_OWN
        LOGDIR_OWN=$DIR_OWN
    fi
    sed "s:%LSWS_CTRL%:$LSWS_HOME/bin/lswsctrl:" "$LSINSTALL_DIR/admin/misc/lsws.rc.in" > "$LSINSTALL_DIR/admin/misc/lsws.rc"
    sed "s:%LSWS_CTRL%:$LSWS_HOME/bin/lswsctrl:" "$LSINSTALL_DIR/admin/misc/lsws.rc.gentoo.in" > "$LSINSTALL_DIR/admin/misc/lsws.rc.gentoo"
    sed "s:%LSWS_CTRL%:$LSWS_HOME/bin/lswsctrl:" "$LSINSTALL_DIR/admin/misc/lshttpd.service.in" > "$LSINSTALL_DIR/admin/misc/lshttpd.service"
    if [ -d "$LSWS_HOME/admin/html.$VERSION" ]; then
        rm -rf "$LSWS_HOME/admin/html.$VERSION"
    fi

    util_mkdir "$SDIR_OWN" $DIR_MOD admin bin docs fcgi-bin lsrecaptcha php lib modules backup autoupdate tmp cachedata gdata docs/css docs/img docs/ja-JP docs/zh-CN add-ons share share/autoindex share/autoindex/icons admin/fcgi-bin admin/html.$VERSION admin/misc lsns lsns/bin lsns/conf
    util_mkdir "$LOGDIR_OWN" "0750" logs admin/logs lsns/logs
    util_mkdir "$CONF_OWN" $SDIR_MOD conf conf/cert conf/templates conf/vhosts conf/vhosts/Example admin/conf admin/tmp phpbuild
    util_mkdir "$SDIR_OWN" $SDIR_MOD cgid admin/cgid admin/cgid/secret
    util_mkdir "$DIR_OWN" $SDIR_MOD  tmp/ocspcache
    chgrp  $WS_GROUP $LSWS_HOME/admin/tmp $LSWS_HOME/admin/cgid $LSWS_HOME/cgid
    chmod  g+x $LSWS_HOME/admin/tmp $LSWS_HOME/admin/cgid $LSWS_HOME/cgid
    chown  $CONF_OWN $LSWS_HOME/admin/tmp/sess_* 1>/dev/null 2>&1
    chown  $DIR_OWN $LSWS_HOME/cachedata
    chown  $DIR_OWN $LSWS_HOME/autoupdate
    chown  $DIR_OWN $LSWS_HOME/tmp
    util_mkdir "$SDIR_OWN" $DIR_MOD Example 

    find "$LSWS_HOME/admin/tmp" -type s -atime +1 -delete 2>/dev/null
    if [ $? -ne 0 ]; then
        find "$LSWS_HOME/admin/tmp" -type s -atime +1 2>/dev/null | xargs rm -f
    fi 

    find "$DEFAULT_TMP_DIR" -type s -atime +1 -delete 2>/dev/null
    if [ $? -ne 0 ]; then
        find "$DEFAULT_TMP_DIR" -type s -atime +1 2>/dev/null | xargs rm -f
    fi 

    WHM_CGIDIR="/usr/local/cpanel/whostmgr/docroot/cgi"
    if [ -d "$WHM_CGIDIR" ] && [ $INST_USER = "root" ]  ; then
        HOST_PANEL="cpanel"
    fi
    util_cpdir "$SDIR_OWN" $DOC_MOD add-ons
    util_cpdir "$CONF_OWN" $DOC_MOD share/autoindex
    
    util_ccpfile "$SDIR_OWN" $EXEC_MOD fcgi-bin/lsperld.fpl fcgi-bin/RackRunner.rb fcgi-bin/lsnode.js
    util_cpfile "$SDIR_OWN" $EXEC_MOD  fcgi-bin/RailsRunner.rb  fcgi-bin/RailsRunner.rb.2.3
    util_cpfile "$SDIR_OWN" $EXEC_MOD lsns/bin/common.py lsns/bin/lshostexec lsns/bin/lscgctl lsns/bin/lscgstats lsns/bin/lspkgctl lsns/bin/lsnsctl lsns/bin/unmount_ns lsns/bin/cmd_ns lsns/bin/lssetup

    pkill _recaptcha
    util_cpfile "$SDIR_OWN" $EXEC_MOD  lsrecaptcha/_recaptcha lsrecaptcha/_recaptcha.shtml
    util_cpfile "$SDIR_OWN" $EXEC_MOD admin/misc/rc-inst.sh admin/misc/admpass.sh admin/misc/rc-uninst.sh admin/misc/uninstall.sh admin/misc/lsws.rc admin/misc/lsws.rc.gentoo admin/misc/enable_phpa.sh admin/misc/mgr_ver.sh admin/misc/gzipStatic.sh admin/misc/fp_install.sh admin/misc/create_admin_keypair.sh admin/misc/awstats_install.sh admin/misc/update.sh admin/misc/cleancache.sh admin/misc/lsup.sh admin/misc/testbeta.sh
    util_cpfile "$SDIR_OWN" $EXEC_MOD admin/misc/ap_lsws.sh.in admin/misc/build_ap_wrapper.sh admin/misc/cpanel_restart_httpd.in admin/misc/build_admin_php.sh admin/misc/convertxml.sh admin/misc/lscmctl
    util_cpfile "$SDIR_OWN" $DOC_MOD admin/misc/gdb-bt admin/misc/htpasswd.php admin/misc/php.ini admin/misc/genjCryptionKeyPair.php admin/misc/purge_cache_byurl.php
    util_cpfile "$SDIR_OWN" $DOC_MOD admin/misc/convertxml.php  admin/misc/lshttpd.service 
    
    
    if [ $SET_LOGIN -eq 1 ]; then
        util_cpfile "$CONF_OWN" $CONF_MOD admin/conf/htpasswd
    else
        util_ccpfile "$CONF_OWN" $CONF_MOD admin/conf/htpasswd
    fi
    if [ $INSTALL_TYPE = "upgrade" ]; then
        util_ccpfile "$CONF_OWN" $CONF_MOD admin/conf/admin_config.conf
        util_cpfile "$CONF_OWN" $CONF_MOD admin/conf/php.ini admin/conf/${SSL_HOSTNAME}.key admin/conf/${SSL_HOSTNAME}.crt
        util_ccpfile "$CONF_OWN" $CONF_MOD conf/httpd_config.conf conf/mime.properties conf/templates/ccl.conf conf/templates/phpsuexec.conf conf/templates/rails.conf
        $TEST_BIN ! -L "$LSWS_HOME/bin/lshttpd"
        if [ $? -eq 0 ]; then
            mv -f "$LSWS_HOME/bin/lshttpd" "$LSWS_HOME/bin/lshttpd.old"
        fi
        $TEST_BIN ! -L "$LSWS_HOME/bin/lswsctrl"
        if [ $? -eq 0 ]; then
            mv -f "$LSWS_HOME/bin/lswsctrl" "$LSWS_HOME/bin/lswsctrl.old"
        fi
        $TEST_BIN ! -L "$LSWS_HOME/admin/html"
        if [ $? -eq 0 ]; then
            mv -f "$LSWS_HOME/admin/html" "$LSWS_HOME/admin/html.old"
        fi

        if [ ! -f "$LSWS_HOME/conf/vhosts/Example/vhconf.conf" ]; then
            util_cpdir "$CONF_OWN" $CONF_MOD conf/vhosts/Example
        fi

        #test if contains Example/html Example/cgi-bin and copy when installation
        if [ ! -d "$LSWS_HOME/Example/html" ]; then
            util_mkdir "$SDIR_OWN" $DIR_MOD Example/html
            util_cpdir "$SDIR_OWN" $DOC_MOD Example/html
            util_cp_htaccess "$SDIR_OWN" $DOC_MOD Example/html
        fi
        if [ ! -d "$LSWS_HOME/Example/cgi-bin" ]; then
            util_mkdir "$SDIR_OWN" $DIR_MOD Example/cgi-bin
            util_cpdir "$SDIR_OWN" $DOC_MOD Example/cgi-bin
        fi

    else
        util_cpfile "$CONF_OWN" $CONF_MOD admin/conf/admin_config.conf
        util_cpfile "$CONF_OWN" $CONF_MOD conf/templates/ccl.conf conf/templates/phpsuexec.conf conf/templates/rails.conf
        util_cpfile "$CONF_OWN" $CONF_MOD admin/conf/php.ini admin/conf/${SSL_HOSTNAME}.key admin/conf/${SSL_HOSTNAME}.crt
        util_cpfile "$CONF_OWN" $CONF_MOD conf/httpd_config.conf conf/mime.properties conf/httpd_config.conf
        util_cpdir "$CONF_OWN" $CONF_MOD conf/vhosts/Example
        util_mkdir "$SDIR_OWN" $DIR_MOD Example/html Example/cgi-bin
        util_cpdir "$SDIR_OWN" $DOC_MOD Example/html Example/cgi-bin
        util_cp_htaccess "$SDIR_OWN" $DOC_MOD Example/html
    fi

    #change conf own as lsadm:nobody permission 750
    chown -R lsadm:$WS_GROUP "$LSWS_HOME/conf/"
    chmod -R 0750 "$LSWS_HOME/conf/"
    
    chmod 0600 "$LSWS_HOME/conf/httpd_config.conf"
    chmod 0600 "$LSWS_HOME/conf/vhosts/Example/vhconf.conf"

    util_cpfile "$CONF_OWN" $DOC_MOD conf/${SSL_HOSTNAME}.crt
    util_cpfile "$CONF_OWN" $DOC_MOD conf/${SSL_HOSTNAME}.key
    
    util_mkdir "$DIR_OWN" $DIR_MOD Example/logs Example/fcgi-bin
    util_cpdir "$SDIR_OWN" $DOC_MOD admin/html.$VERSION
    rm -rf $LSWS_HOME/admin/html
    ln -sf ./html.$VERSION $LSWS_HOME/admin/html


    util_cpfile "$SDIR_OWN" $EXEC_MOD bin/updateagent
    util_cpfile "$SDIR_OWN" $EXEC_MOD bin/wswatch.sh
    util_cpfile "$SDIR_OWN" $EXEC_MOD bin/unmount_ns
    util_cpfilev "$SDIR_OWN" $EXEC_MOD $VERSION bin/lswsctrl bin/lshttpd

    #if [ -e "$LSINSTALL_DIR/bin/lshttpd.dbg" ]; then
    #    if [ -f "$LSINSTALL_DIR/bin/lshttpd.dbg.$VERSION" ]; then
    #        rm "$LSINSTALL_DIR/bin/lshttpd.dbg.$VERSION"
    #    fi
    #    util_cpfilev "$SDIR_OWN" $EXEC_MOD $VERSION bin/lshttpd.dbg
    #    
    #    #enable debug build for beta release
    #    ln -sf ./lshttpd.dbg.$VERSION $LSWS_HOME/bin/lshttpd
    #fi

    ln -sf ./lshttpd.$VERSION $LSWS_HOME/bin/lshttpd
    ln -sf lshttpd $LSWS_HOME/bin/litespeed

    util_cpfile "$SDIR_OWN" $DOC_MOD docs/* docs/css/* docs/img/* docs/ja-JP/* docs/zh-CN/*
    util_cpfile "$SDIR_OWN" $DOC_MOD VERSION GPL.txt

    if [ -f $LSWS_HOME/autoupdate/download ]; then
        rm $LSWS_HOME/autoupdate/download
    fi


}


setupPHPAccelerator()
{
    cat <<EOF

PHP Opcode Cache Setup

In order to maximize the performance of PHP, a pre-built PHP opcode cache
can be installed automatically. The opcode cache increases performance of
PHP scripts by caching them in compiled state, the overhead of compiling
PHP is avoided.

Note: If an opcode cache has been installed already, you do not need to
      change it. If you need to built PHP binary by yourself, you need to 
      built PHP opcode cache from source as well, unless the version of your
      PHP binary is same as that the pre-built PHP opcode cache built for.

EOF

    printf "%s" "Would you like to change PHP opcode cache setting [y/N]? "

    read PHPACC
    echo

    if [ "x$PHPACC" = "x" ]; then
        PHPACC=n
    fi
    if [ `expr "$PHPACC" : '[Yy]'` -gt 0 ]; then
        $LSWS_HOME/admin/misc/enable_phpa.sh
    fi
}


gen_selfsigned_cert_new()
{   
    COMMNAME=`hostname`
    SSL_COUNTRY=US
    SSL_STATE="New Jersey"
    csr="${SSL_HOSTNAME}.csr"
    key="${SSL_HOSTNAME}.key"
    cert="${SSL_HOSTNAME}.crt"
    
#     openssl req -subj "/CN=${COMMNAME}/O=webadmin/C=US/extendedKeyUsage=1.3.6.1.5.5.7.3.1/subjectAltName=DNS.1=${MYIP}/" -new -newkey rsa:2048 -sha256 -days 730 -nodes -x509 -keyout ${key} -out ${cert}
# 
    
    cat << EOF > $csr
[req]
prompt=no
distinguished_name=openlitespeed
[openlitespeed]
commonName = ${COMMNAME}
countryName = ${SSL_COUNTRY}
localityName = Virtual
organizationName = LiteSpeedCommunity
organizationalUnitName = Testing
stateOrProvinceName = NJ
emailAddress = mail@${COMMNAME}
name = openlitespeed
initials = CP
dnQualifier = openlitespeed
[server_exts]
extendedKeyUsage=1.3.6.1.5.5.7.3.1
EOF
    openssl req -x509 -config $csr -extensions 'server_exts' -nodes -days 820 -newkey rsa:2048 -keyout ${key} -out ${cert}
    rm -f $csr
}

gen_selfsigned_cert()
{
    # source outside config file
    if [ $# -eq 1 ] ; then
        . ${1}
    fi

   # set default value
    if [ "${SSL_COUNTRY}" = "" ] ; then
    SSL_COUNTRY=US
    fi

    if [ "${SSL_STATE}" = "" ] ; then
    SSL_STATE="New Jersey"
    fi

    if [ "${SSL_LOCALITY}" = "" ] ; then
    SSL_LOCALITY=Virtual
    fi

    if [ "${SSL_ORG}" = "" ] ; then
    SSL_ORG=LiteSpeedCommunity
    fi
    
    if [ "${SSL_ORGUNIT}" = "" ] ; then
    SSL_ORGUNIT=Testing
    fi

    if [ "${SSL_HOSTNAME}" = "" ] ; then
    SSL_HOSTNAME=webadmin
    fi

    if [ "${SSL_EMAIL}" = "" ] ; then
    SSL_EMAIL=.
    fi

    csr="${SSL_HOSTNAME}.csr"
    key="${SSL_HOSTNAME}.key"
    cert="${SSL_HOSTNAME}.crt"

# Create the certificate signing request
    openssl req -new -passin pass:password -passout pass:password -out $csr <<EOF
${SSL_COUNTRY}
${SSL_STATE}
${SSL_LOCALITY}
${SSL_ORG}
${SSL_ORGUNIT}
${SSL_HOSTNAME}
${SSL_EMAIL}
.
.
EOF
    echo ""

    [ -f ${csr} ] && openssl req -text -noout -in ${csr}
    echo ""

# Create the Key
    openssl rsa -in privkey.pem -passin pass:password -passout pass:password -out ${key}

# Create the Certificate

    openssl x509 -in ${csr} -out ${cert} -req -signkey ${key} -days 1000
}



finish()
{
    cat <<EOF
Congratulations! The LiteSpeed Web Server has been successfully installed.
Command line script - "$LSWS_HOME/bin/lswsctrl"
can be used to start or stop the server.

It is recommended to limit access to the web administration interface. 
Right now the interface can be accessed from anywhere where this
machine can be reached over the network. 

Three options are available:
  1. If the interface needs to be accessed only from this machine, just 
     change the listener for the interface to only listen on the loopback 
     interface - localhost(127.0.0.1). 
  2. If the interface needs to be accessible from limited IP addresses or sub 
     networks, then set up access control rules for the interface accordingly.
  3. If the interface has to be accessible via internet, SSL (Secure Sockets 
     Layer) should be used. Please read respective HOW-TOs on SSL configuration.

To change configurations of the interface, login and click 
"Interface Configuration" button on the main page.
The administration interface is located at http://localhost:<ADMIN_PORT>/
or http://<ip_or_Hostname_of_this_machine>:<ADMIN_PORT>/ 

EOF

    if [ $INST_USER = "root" ]; then
        if [ $INSTALL_TYPE != "upgrade" ]; then
            printf "%s\n%s" "Would you like to have LiteSpeed Web Server started automatically" "when the server restarts [Y/n]? "
            read START_SERVER
            echo

            if [ "x$START_SERVER" = "x" ]; then
                START_SERVER=y
            fi
            if [ `expr "$START_SERVER" : '[Yy]'` -gt 0 ]; then
                $LSWS_HOME/admin/misc/rc-inst.sh
            else
                cat <<EOF
If you want to start the web server automatically later, just run 
    "$LSWS_HOME//rc-inst.sh"
to install the service control script.

EOF
            fi
        fi
        if [ "x$HOST_PANEL" != "x" ]; then
            cat << EOF 

The default configuration file contain support for both PHP4 and PHP5,
A prebuilt PHP4 binary comes with this package, however, we recommend 
you to build your own PHP4 and PHP5 binaries though our web console with
the same configuration parameters as your current PHP installation. You 
can check your current PHP configuration via a phpinfo() page.

Press [ENTER] to continue

EOF

            read TMP_VAL

            cat << EOF 

When you replace Apache with LiteSpeed, remember to stop Apache completely.
On most Linux servers, you should do:

    service httpd stop
    chkconfig httpd off
or
    service apache stop
    chkconfig apache off

If "Port Offset" has been set to "0", you should do it now.

Press [ENTER] to continue

EOF
            read TMP_VAL


        fi
    fi



    if [ $INSTALL_TYPE != "upgrade" ]; then
        printf "%s" "Would you like to start it right now [Y/n]? "
    else
        printf "%s" "Would you like to restart it right now [Y/n]? "
    fi
    read START_SERVER
    echo

    if [ "x$START_SERVER" = "x" ]; then
        START_SERVER=y
    fi

    if [ `expr "$START_SERVER" : '[Yy]'` -gt 0 ]; then
        if [ $INSTALL_TYPE != "upgrade" ]; then
            "$LSWS_HOME/bin/lswsctrl" start
        else
            "$LSWS_HOME/bin/lswsctrl" restart
        fi
    else
        exit 0
    fi

    sleep 1
    RUNNING_PROCESS=`$PS_CMD | grep lshttpd | grep -v grep`

    if [ "x$RUNNING_PROCESS" != "x" ]; then

        cat <<EOF

LiteSpeed Web Server started successfully! Have fun!

EOF
        exit 0
    else

        cat <<EOF

[ERROR] Failed to start the web server. For trouble shooting information,
        please refer to documents in "$LSWS_HOME/docs/".

EOF
    fi

}

