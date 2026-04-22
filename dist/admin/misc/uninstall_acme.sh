#!/bin/bash
CUR_DIR=`dirname "$0"`
cd $CUR_DIR/../..

function checkenv() 
{
    if [ `uname` != "Linux" ]; then
        echo "The acme feature is only available for Linux at this time"
        exit 1
    fi
    WHOAMI=$(whoami)
    if [ "${WHOAMI}" != "root" ]; then
        echo "You must be root to uninstall the acme feature"
        exit 1
    fi

}

function usage() 
{
    echo "uninstall_acme.sh: Uninstalls acme support for LiteSpeed"
    echo "$1"
    echo "Usage: $0: [-l LSWS_HOME]"
}

function checkopts()
{
    LSWS_HOME=`pwd`
    while getopts "l:" OPT; do
        case $OPT in
            l)
                LSWS_HOME="$OPTARG"
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
    if [ ! -d "${LSWS_HOME}" ]; then
        usage "LiteSpeed is not found in the specified location"
        exit 1
    fi
}

function uninstallacme()
{
    grep mod_acme "${LSWS_HOME}/conf/httpd_config.conf" -q
    if [ "$?" -eq 0 ]; then
        cp "${LSWS_HOME}/conf/httpd_config.conf" "${LSWS_HOME}/conf/httpd_config.conf0"
        cat "${LSWS_HOME}/conf/httpd_config.conf" | tr '\n' '\f' | sed 's/module\smod_acme\s{.*\sacmeThumbPrint\s\S*\s}\s//' | tr '\f' '\n' > step1
        cat step1 > "${LSWS_HOME}/conf/httpd_config.conf"
        rm step1
        echo "mod_acme reference removed from the configuration: $?"
    fi
    if [ -d "${LSWS_HOME}/acme" ]; then
        ${LSWS_HOME}/acme/acme.sh --uninstall
        rm -rf "${LSWS_HOME}/acme"
        echo "acme directory removed"
        rm -rf "${LSWS_HOME}/conf/cert/acme"
        echo "acme configuration files removed"
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
uninstallacme
restartlsws

