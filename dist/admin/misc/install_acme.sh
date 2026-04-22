#!/bin/bash
CUR_DIR=`dirname "$0"`
cd $CUR_DIR/../..

function checkprereq() 
{
    if [ ! -e "/usr/bin/git" ]; then
        if [ -f /etc/redhat-release ] ; then
            yum -y install git
        elif [ -f /etc/debian_version ] ; then
            apt -y install git
        fi
    fi
}

function checkenv() 
{
    if [ `uname` != "Linux" ]; then
        echo "The acme feature is only available for Linux at this time"
        exit 1
    fi
    WHOAMI=$(whoami)
    if [ "${WHOAMI}" != "root" ] ; then
        echo "You must be root to install the acme feature"
        exit 1
    fi

}

function usage() 
{
    echo "Install_acme.sh: Installs acme support for LiteSpeed"
    echo "$1"
    echo "Usage: $0: [-l LSWS_HOME] [-e CERT_EMAIL] [-s CERT_SERVER_URL]"
}

function extract_email()
{
    if [ "$CERT_EMAIL" != "" ]; then
        return 0
    fi
    EMAILS_LINE=$(grep adminEmails "${LSWS_HOME}/conf/httpd_config.conf")
    if [ "$EMAILS_LINE" = "" ]; then
        return 0
    fi
    CERT_EMAIL=$(echo $EMAILS_LINE | awk '{print $2}')
    if [ "${CERT_EMAIL}" = "root@localhost" ]; then
        CERT_EMAIL=""
        return 0
    fi
    echo "Using email from config file: ${CERT_EMAIL}"
    return 0
}

function checkopts()
{
    CERT_EMAIL=""
    LSWS_HOME=`pwd`
    SERVER="https://acme-v02.api.letsencrypt.org/directory"
    while getopts "l:e:s:" OPT; do
        case $OPT in
            l)
                LSWS_HOME="$OPTARG"
                ;;
            e)
                CERT_EMAIL="$OPTARG"
                ;;
            s)
                SERVER="$OPTARG"
                ;;
            \?)
                usage "Invalid option: $OPT"
                exit 1
                ;;
            :)
                usage "Option -$OPT requires an argument"
                exit 1
                ;;
        esac
    done
    if [ "$CERT_EMAIL" = "" ]; then
        extract_email
    fi
    if [ "${SERVER}" != "" ]; then
        curl "$SERVER" -f > /dev/null 2>&1
        if [ "$?" -ne 0 ]; then
            usage "You must specify a valid cert URL: see https://github.com/acmesh-official/acme.sh/wiki/Server"
            exit 1
        fi
    fi
}

function checkacme()
{
    if [ ! -d "${LSWS_HOME}/acme" ]; then
        pushd "${LSWS_HOME}"
        git clone https://github.com/acmesh-official/acme.sh.git
        cd acme.sh
        if [ ! -d "${LSWS_HOME}/conf/cert/acme" ]; then
            mkdir "${LSWS_HOME}/conf/cert/acme"
        fi
        if [ "${CERT_EMAIL}" != "" ]; then
            CERT_EMAIL_TITLE="--accountemail"
        fi
        echo "${SERVER}" > "${LSWS_HOME}/conf/cert/acme/server.conf"
        export LE_CONFIG_HOME="${LSWS_HOME}/conf/cert/acme/data"
        export LE_WORKING_DIR="${LSWS_HOME}/acme" 
        export ACME_DIRECTORY="${SERVER}"
        ./acme.sh --install \
            --home "${LSWS_HOME}/acme" \
            --config-home "${LSWS_HOME}/conf/cert/acme/data" \
            --cert-home "${LSWS_HOME}/conf/cert/acme/certs" \
            --accountkey "${LSWS_HOME}/conf/cert/acme/account.key" \
            --accountconf "${LSWS_HOME}/acme/account.conf" \
            --useragent "LiteSpeed User Agent" \
            $CERT_EMAIL_TITLE $CERT_EMAIL \
            --nocron
        cd ..
        rm -rf acme.sh
        cd acme
        if [ "${SERVER}" != "" ]; then
            ./acme.sh  --set-default-ca --server $SERVER
            if [ "$?" -ne 0 ]; then
                echo "Error setting default ca to $SERVER"
                exit 1
            fi
        fi
        REGISTER=$(./acme.sh --register-account --server $SERVER)
        echo "Register returned $?: $REGISTER"
        THUMBPRINT="$(echo $REGISTER|sed -n 's/.*ACCOUNT_THUMBPRINT='\''\(.*\)'\''/\1/p')"
        echo "Thumbprint: $THUMBPRINT"
        chown -R lsadm:lsadm "${LSWS_HOME}/conf/cert/acme"
        chown -R lsadm:lsadm "${LSWS_HOME}/acme"
        if [ "${THUMBPRINT}" = "" ]; then
            echo "Internal error, thumbprint not obtained"
            exit 1
        fi
        grep mod_acme ../conf/httpd_config.conf -q
        if [ "$?" -ne 0 ]; then
            cat << EOF >> ../conf/httpd_config.conf

module mod_acme {
acmeEnable 1
acmeThumbPrint $THUMBPRINT
}
EOF
        else 
            echo "Unexpected mod_acme entry in the configuration file"
        fi
        popd
    fi
}

function restartlsws
{
    LSLINE=$(ps -ef|grep '[l]shttpd - main')
    if [ "$?" -eq 0 ]; then
        echo "Restarting LiteSpeed"
        PID=$(echo "${LSLINE}" | awk '{print $2}')
        kill -USR1 "${PID}"
    fi
}


# Begin mainline
checkenv
checkopts "$@"
checkprereq
checkacme
restartlsws
