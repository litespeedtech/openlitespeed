#! /bin/sh

LSUPVERSION=v2.83-9/29/2020
LOCKFILE=/tmp/olsupdatingflag

PIDFILE=/tmp/lshttpd/lshttpd.pid

CURDIR=`dirname "$0"`
cd $CURDIR
CURDIR=`pwd`
LSWSHOME=`dirname $CURDIR`
LSWSHOME=`dirname ${LSWSHOME}`

#When it is new installation, use default DIR
if [ ! -f ${LSWSHOME}/bin/openlitespeed ] ; then
    LSWSHOME=/usr/local/lsws
fi
LSWSCTRL=${LSWSHOME}/bin/lswsctrl

#the lsws_env may change the PID file location
if [ -f "${LSWSHOME}"/lsws_env ] ; then
    . "${LSWSHOME}"/lsws_env
fi

SYSCTRL=no
which systemctl >/dev/null 2>&1
if [ $? = 0 ] ; then
    SYSCTRL=yes
fi

RUNNING=0
test_running()
{
    RUNNING=0
    if [ -f $PIDFILE ] ; then
        FPID=`cat $PIDFILE`
        if [ "x$FPID" != "x" ]; then
            kill -0 $FPID 2>/dev/null
            if [ $? -eq 0 ] ; then
                RUNNING=1
                PID=$FPID
            fi
        fi
    fi
}


startService()
{
    if [ "$SYSCTRL" = "yes" ] ; then
        systemctl start lsws
    fi
    test_running
    if [ $RUNNING -eq 0 ] ; then
        ${LSWSCTRL} start
    fi
}

stopService()
{
    if [ "$SYSCTRL" = "yes" ] ; then
        systemctl stop lsws
    fi
    test_running
    if [ $RUNNING -eq 1 ] ; then
        ${LSWSCTRL} stop
    fi
    
    FPID=`cat $PIDFILE`
    if [ "x$FPID" != "x" ]; then
        kill -9 $FPID 2>/dev/null
    fi    
    
    #when FPID not exist, try again
    FPID=`ps -ef | grep openlitespeed | grep -v grep | awk '{print $2}'`
    if [ "x$FPID" != "x" ]; then
        kill -9 $FPID 2>/dev/null
    fi    
    
}


CURLONGVERSION=
CURVERSION=
PREVERSION=
ORGVERSION=
NEWVERSION=
DLCMD=
ONLYBIN=no

OSNAME=`uname -s`


URLDIR=
URLMODE=

ISLINUX=yes
if [ "x$OSNAME" != "xLinux" ] ; then
    ISLINUX=no
fi

echoR()
{
	if [ $# -eq 0 ] ; then
		echo
	else
        if [ "${ISLINUX}" = "yes" ] ; then
            echo "$(tput setaf 1)$@$(tput sgr0)"
        else
            echo -e "\e[31m$@\e[39m"
        fi
	fi
}

echoG()
{
	if [ $# -eq 0 ] ; then
		echo
	else
        if [ "${ISLINUX}" = "yes" ] ; then
            echo "$(tput setaf 2)$@$(tput sgr0)"
        else
            echo -e "\e[32m$@\e[39m"
        fi
	fi
}

echoG "lsup.sh Version: ${LSUPVERSION}."

#Test which downloader
which wget >/dev/null 2>&1
if [ $? != 0 ] ; then
    which curl >/dev/null 2>&1
    if [ $? != 0 ] ; then
        if [ "x$OSNAME" = "xFreeBSD" ] ; then
            which fetch >/dev/null 2>&1
            if [ $? = 0 ] ; then
                DLCMD="fetch -o"
            else
                echoR "It seems you do not have 'wget', 'curl' and 'fetch' installed, please install one first and try again."
                exit 1
            fi
        else
            echoR "It seems you do not have 'wget' and 'curl' installed, please install one first and try again."
            exit 1
        fi
    else
        DLCMD="curl -L -k -o"
    fi
else
    DLCMD="wget -nv -O"
fi

#Update lsup.sh itself
$DLCMD ${LSWSHOME}/admin/misc/lsup.shnew https://raw.githubusercontent.com/litespeedtech/openlitespeed/master/dist/admin/misc/lsup.sh >/dev/null 2>&1
if [ $? = 0 ] ; then
    diff ${LSWSHOME}/admin/misc/lsup.shnew ${LSWSHOME}/admin/misc/lsup.sh >/dev/null 2>&1
    if [ $? != 0 ] ; then
        mv -f ${LSWSHOME}/admin/misc/lsup.sh ${LSWSHOME}/admin/misc/lsup.shold >/dev/null 2>&1
        mv -f ${LSWSHOME}/admin/misc/lsup.shnew ${LSWSHOME}/admin/misc/lsup.sh >/dev/null 2>&1
        chmod 777 ${LSWSHOME}/admin/misc/lsup.sh >/dev/null 2>&1
        echoG "lsup.sh (Version ${LSUPVERSION}) updated, now start new one."
        exec ${LSWSHOME}/admin/misc/lsup.sh "$@"
        exit 10
    else
        rm ${LSWSHOME}/admin/misc/lsup.shnew
    fi
fi

if [ -f ${LSWSHOME}/autoupdate/release ] ; then 
    NEWVERSION=`cat ${LSWSHOME}/autoupdate/release`
else
    if [ "x${NEWVERSION}" = "x" ] ; then
        $DLCMD /tmp/tmprelease https://openlitespeed.org/packages/release >/dev/null 2>&1
        NEWVERSION=`cat /tmp/tmprelease`
        rm /tmp/tmprelease
    fi
fi
if [ "x${NEWVERSION}" = "x" ] ; then
    echoR "Error: cannot get the latest stable version info."
fi

if [ -f ${LSWSHOME}/VERSION ] ; then 
    CURVERSION=`cat ${LSWSHOME}/VERSION`
fi
if [ -f ${LSWSHOME}/VERSION.org ] ; then 
    ORGVERSION=`cat ${LSWSHOME}/VERSION.org`
fi
if [ -f ${LSWSHOME}/VERSION.old ] ; then 
    PREVERSION=`cat ${LSWSHOME}/VERSION.old`
fi

toggle()
{
    FPID=
    PIDFILE=/tmp/lshttpd/lshttpd.pid
    if [ -f $PIDFILE ] ; then
        FPID=`cat $PIDFILE`
    fi
    
    if [ "x$FPID" = "x" ] ; then
        FPID=`ps  -ef > /tmp/testpid ; cat /tmp/testpid | grep 'lshttpd - main' | awk '{printf "%d ", $2}'`
        rm /tmp/testpid
        if [ "x$FPID" = "x" ] ; then
            echoR Can not find pid file [$PIDFILE] or running openlitespeed, toggle DEBUG failed.
            exit 2
        fi
    fi
    
    echoG Main process pid is $FPID
    kill -0 $FPID 2>/dev/null
    if [ $? = 0 ] ; then
        #PIDLIST=`pgrep -P $FPID -a | grep lshttpd | awk '{printf "%d ", $1}'`
        PIDLIST=`ps  -ef > /tmp/testpid ; cat /tmp/testpid | grep 'lshttpd - #' | awk '{printf "%d ", $2}'`
        rm /tmp/testpid
        echoG Debug log toggled to all children processes [ $PIDLIST].
        kill -USR2 $PIDLIST
    else
        echoR It seems pid $FPID can not be accessed, toggle DEBUG failed.
    fi
    exit 0
}


status()
{
    if [ "x${ORGVERSION}" != "x" ] ; then 
        echoG "You original installed version is ${ORGVERSION}."
    fi
    if [ "x${PREVERSION}" != "x" ] ; then 
        echoG "Your previous installed version is ${PREVERSION}."
    fi
    
    echoG "The latest stable version is ${NEWVERSION}. "
    
    if [ ! -f ${LSWSHOME}/bin/openlitespeed ] ; then
        echoR It seems you do not have openlitespeed installed in ${LSWSHOME}.
        exit 3
    
    else
        BINPATH=${LSWSHOME}/bin/openlitespeed
        DEBUGSTR=`${BINPATH} -v | grep DEBUG` >/dev/null 2>&1
        CURLONGVERSION=`${BINPATH} -v | grep Open` >/dev/null 2>&1

        if [ "x${DEBUGSTR}" != "x" ] ; then
            echo ${CURLONGVERSION} | grep DEBUG >/dev/null 2>&1
            if [ $? != 0 ] ; then
                CURLONGVERSION="${CURLONGVERSION}${DEBUGSTR}"
            fi
        fi
        echoG "Your current installed version is ${CURVERSION}, build ${CURLONGVERSION}."
    fi
}

clean()
{
    rm -rf ${LOCKFILE}
    status
    stopService
    rm -rf /tmp/lshttpd/*
    if [ -e /dev/shm/ols ] ; then
        rm -rf /dev/shm/ols/*
    else 
        rm -rf /tmp/shm/ols/*
    fi
    rm -rf ${LSWSHOME}/cgid/cgid.sock*
    rm -rf ${LSWSHOME}/autoupdate/*
    
    startService
    echoG Cleaned and service started.
    exit 0
}

changeAdminPasswd()
{
    SHFILE=${LSWSHOME}/admin/misc/admpass.sh
    if [ -f ${SHFILE} ] ; then
        ${SHFILE}
    else
        echoR ${SHFILE} not exist.
    fi
    exit 0
}

testCurrentStatus()
{
    status
    if [ -f ${LOCKFILE} ] ; then
        echoG "Openlitespeed is updating ...."
    fi
    
    echoG Checking error log ...
    cat ${LSWSHOME}/logs/error.log | grep ERROR 
    if [ $? = 0 ] ; then
        echoR "There is ERROR(s) in your error log."
    else
        echoG "There isn't any ERROR in your error log."
    fi
    
    echoG Checking server core file ...
    ls -l /tmp/lshttpd/core*  >/dev/null 2>&1
    if [ $? = 0 ] ; then
        #Move the core file the bak_core and check together
        if [ ! -e /tmp/lshttpd/bak_core/ ] ; then
            mkdir /tmp/lshttpd/bak_core/ 
        fi
        mv /tmp/lshttpd/core* /tmp/lshttpd/bak_core/
    fi
    
    
    if [ -d /tmp/lshttpd/bak_core/ ] ; then
    
        ls -l /tmp/lshttpd/bak_core/core* | grep core >/dev/null 2>&1
        if [ $? != 0 ] ; then
            echoG "Good. There isn't any core file."
        else
            echoR "There is core file(s) in /tmp/lshttpd/bak_core/"
            
            echo ${CURLONGVERSION} | grep DEBUG >/dev/null 2>&1
            if [ $? != 0 ] ; then
                echoG "You are not running a DEBUG version. You can use the below command to switch to DEBUG version"
                echoG "./lsup.sh -d -v ${NEWVERSION}"
                echoG "And run ./lsup.sh -t again when you get a core file again or after two days running with the DEBUG version."
                exit 4
            fi
            
            which gdb  >/dev/null 2>&1
            if [ $? != 0 ] ; then
                echoG "You do not have 'gdb' installed,  pleaese install it first and run this command again."
                exit 5
            fi   
            
            #output
            echo "Platform `uname -s -m`, installed ${CURLONGVERSION}" > ${LSWSHOME}/corefile.txt
            
            gdb --batch --command=${CURDIR}/gdb-bt ${LSWSHOME}/bin/openlitespeed /tmp/lshttpd/bak_core/core* >> ${LSWSHOME}/corefile.txt
            echoR "Please send file ${LSWSHOME}/corefile.txt to bug@litespeedtech.com to help us to figure the issue soon, thanks."
        fi
    else
        echoG "Good. There isn't any core file."
    fi
    exit 0
}

display_usage()
{
    cat <<EOF
Usage: lsup.sh [-t] | [-c] | [[-d] [-r] | [-v|-e VERSION]]
  
  -d
     Choose Debug version to upgrade or downgrade, will do clean like -c at the same time.

  -s
     Choose Asan version to upgrade or downgrade, will do clean like -c at the same time.
     
  -b
     Choose under development version instead of released version. <special option, be careful>
  
  -v VERSION
     If VERSION is given, this command will try to install specified VERSION. Otherwise, it will get the latest version from ${LSWSHOME}/autoupdate/release.

  -e VERSION
     If VERSION is given, this command will try to install the binaries of the specified VERSION. Otherwise, it will get the latest version from ${LSWSHOME}/autoupdate/release.

  -r 
     Recover to the original installed version which is in file VERSION.
     
  -p
     Recover to the previous installed version which was renamed to .old files.
      
  -t
     To test openlitespeed running status.
     
  -g
     Toggle DEBUG log of running openlitespeed.
     
  -c
     Do some clean and restart openlitespeed service.
     
  -a
     Change the webAdmin password.

  -h | --help     
     Display this help and exit.

EOF
    exit 0
}


VERSION=${NEWVERSION}
while [ "x$1" != "x" ] 
do
    if [ "x$1" = "x-d" ] ; then
        ISDEBUG=yes
        shift
    elif [ "x$1" = "x-s" ] ; then
        ISASAN=yes
        shift    
    elif [ "x$1" = "x-b" ] ; then
        ISBETA=yes
        shift
    elif [ "x$1" = "--help" ] ; then
        display_usage
    elif [ "x$1" = "x-v" ] ; then
        shift
        VERSION=$1
        shift
        if [ "x$VERSION" = "x" ] ; then
            display_usage
        fi
    elif [ "x$1" = "x-e" ] ; then
        ONLYBIN=yes
        shift
        VERSION=$1
        shift
        if [ "x$VERSION" = "x" ] ; then
            display_usage
        fi    
    elif [ "x$1" = "x-r" ] ; then
        VERSION=${ORGVERSION}
        echoG "You choose to install the original installed version."
        shift
    elif [ "x$1" = "x-p" ] ; then
        VERSION=${PREVERSION}
        echoG "You choose to install the previous version."
        shift
    elif [ "x$1" = "x-g" ] ; then
        toggle
    elif [ "x$1" = "x-c" ] ; then
        clean
    elif [ "x$1" = "x-t" ] ; then
        testCurrentStatus
    elif [ "x$1" = "x-a" ] ; then
        changeAdminPasswd        
    else 
        display_usage
    fi
done

if [ "x${VERSION}" = "x" ] ; then
    echoR "Can not get the right version for installation, quit."
    exit 6
fi


if [ -f ${LOCKFILE} ] ; then
    FILETIME=`stat -c %Y  ${LOCKFILE}`
    SYSTEMTIME=`date -u +%s`
    COMSYSTEMTIME=$(($SYSTEMTIME-600))
    #echoY ${LOCKFILE} exists, timestamp is $FILETIME, current time is $SYSTEMTIME( $COMSYSTEMTIME + 600 seconds)
    if [ $COMSYSTEMTIME -gt $FILETIME ] ; then
        echoG "${LOCKFILE} exists, timestamp is $FILETIME, current time is $SYSTEMTIME, removed it."
        rm -rf ${LOCKFILE}
    else
        echoR "Openlitespeed is updating, quit. (You may run -c to remove the lock file and try again.)"
        exit 0
    fi
fi

touch ${LOCKFILE}

TEMPPATH=${LSWSHOME}/autoupdate
if [ ! -e ${TEMPPATH} ] ; then
    TEMPPATH=/usr/src
fi
cd ${TEMPPATH}
if [ -f ols.tgz ] ; then
    rm -f ols.tgz
fi


if [ "x${ISBETA}" = "xyes" ]; then
    URLDIR=preuse
else
    URLDIR=packages
fi

if [ "x${ISLINUX}" = "xyes" ] ; then
    
    if [ "$ISASAN" = "yes" ] ; then
        URLMODE=a.
    elif [ "$ISDEBUG" = "yes" ] ; then
        URLMODE=d.
    else
        URLMODE=
    fi
else
    URLMODE=src.
fi

URL=https://openlitespeed.org/${URLDIR}/openlitespeed-${VERSION}.${URLMODE}tgz
echoG "download URL is ${URL}"

testsz=1000000  
RET=1
$DLCMD ols.tgz $URL
RET=$?
if [ $RET = 0 ] ; then
    tz=$(stat -c%s ols.tgz)
    if [ $tz -lt $testsz ] ; then
        RET=1
    fi
fi
    
if [ $RET != 0 ] ; then
    if [ "x${URLDIR}" = "xpreuse" ] ; then
        echoR "Error, failed to download $URL, quit."
        rm -rf ${LOCKFILE}
        exit 7
    else
        echoR "Failed to download $URL, will try our under development version."
        URLDIR=preuse
        URL=https://openlitespeed.org/${URLDIR}/openlitespeed-${VERSION}.${URLMODE}tgz
        echoG "download URL is ${URL}"
        
        RET=1
        $DLCMD ols.tgz $URL
        RET=$?
        if [ $RET = 0 ] ; then
            tz=$(stat -c%s ols.tgz)
            if [ $tz -lt $testsz ] ; then
                RET=1
            fi
        fi
        
        if [ $RET != 0 ] ; then
            echoR "Error, failed to download $URL, quit."
            rm -rf ${LOCKFILE}
            exit 7
        fi
    fi
fi

tar xf ols.tgz
if [ "x${ISLINUX}" = "xyes" ] ; then
    SRCDIR=${TEMPPATH}/openlitespeed
else
    SRCDIR=${TEMPPATH}/openlitespeed-${VERSION}
fi

if [ -f ${LSWSHOME}/VERSION ] ; then 
    if [ ! -f ${LSWSHOME}/VERSION.org ] ; then 
        cp ${LSWSHOME}/VERSION ${LSWSHOME}/VERSION.org
    fi
    
    cp -f ${LSWSHOME}/VERSION ${LSWSHOME}/VERSION.old
fi

stopService
if [ -e /tmp/lshttpd/bak_core ] ; then
    mv /tmp/lshttpd/bak_core /tmp/lshttpdcore
fi
rm -rf /tmp/lshttpd/*
if [ -e /dev/shm/ols ] ; then
    rm -rf /dev/shm/ols/*
else 
    rm -rf /tmp/shm/ols/*
fi

cd $SRCDIR/
if [ "x${ISLINUX}" = "xno" ] ; then
    
    echoR "Your platform is $OSNAME, we do not have pre-built package ready for it."
    echoG "Need to compile from source code, please wait for 10~20 minutes."
    if [ -f ./build.sh ] ; then
        ./build.sh
    else
        echoR "This version does not have ./build.sh, please run './configure; make; make install' to install it."
        echoG Usually ./configure need some parameters, this auto tool cannot continue. Exit. 
        rm -rf ${LOCKFILE}
        exit 8
    fi
fi


#Change to old server root when it is updating.
if [ -f ${LSWSHOME}/bin/openlitespeed ] ; then
    echo "SERVERROOT=${LSWSHOME}" > ols.conf
    echo "###" >> ols.conf
fi

if [ "$ONLYBIN" = "no" ] ; then 
    ./install.sh
else
    stopService
    mv -f ${LSWSHOME}/bin/openlitespeed ${LSWSHOME}/bin/openlitespeed.old
    cp bin/* ${LSWSHOME}/bin/
    cp modules/* ${LSWSHOME}/modules/
fi

rm -rf $SRCDIR
rm -rf ${LSWSHOME}/autoupdate/*

#Sign it and keep old sign
if [ ! -e ${LSWSHOME}/PLAT ] ; then 
    echo lsup > ${LSWSHOME}/PLAT
else
    ORGPLAT=`cat ${LSWSHOME}/PLAT`
    echo $ORGPLAT | grep lsup >/dev/null 2>&1
    if [ $? != 0 ] ; then
        echo "lsup-$ORGPLAT" > ${LSWSHOME}/PLAT
    fi
fi

rm -rf ${LOCKFILE}

startService
test_running
if [ $RUNNING -eq 1 ] ; then
    RUNSTATE="started"
else
    RUNSTATE="stopped"
fi

if [ "$ONLYBIN" = "no" ] ; then 
    PACKNAME="files"
else
    PACKNAME="binaries"
fi
echoG "All ${PACKNAME} are updated and service is ${RUNSTATE}."
echo 
echo



