#! /bin/sh

LSUPVERSION=v2.1-11/22/2019
LOCKFILE=/tmp/olsupdatingflag

CURDIR=`dirname "$0"`
cd $CURDIR
CURDIR=`pwd`
LSWSHOME=`dirname $CURDIR`
LSWSHOME=`dirname ${LSWSHOME}`
LSWSCTRL=${LSWSHOME}/bin/lswsctrl

CURLONGVERSION=
CURVERSION=
PREVERSION=
ORGVERSION=
NEWVERSION=
DLCMD=

OSNAME=`uname -s`
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
        mv -f ${LSWSHOME}/admin/misc/lsup.sh ${LSWSHOME}/admin/misc/lsup.shold
        mv -f ${LSWSHOME}/admin/misc/lsup.shnew ${LSWSHOME}/admin/misc/lsup.sh
        chmod 777 ${LSWSHOME}/admin/misc/lsup.sh
        echoG "lsup.sh updated, please run again."
        exit 10
    else
        rm ${LSWSHOME}/admin/misc/lsup.shnew
    fi
fi

if [ -f $LSWSHOME/autoupdate/release ] ; then 
    NEWVERSION=`cat $LSWSHOME/autoupdate/release`
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
    ${LSWSCTRL} stop
    rm -rf /tmp/lshttpd/*
    if [ -e /dev/shm/ols ] ; then
        rm -rf /dev/shm/ols/*
    else 
        rm -rf /tmp/shm/ols/*
    fi
    rm -rf ${LSWSHOME}/cgid/cgid.sock*
    
    ${LSWSCTRL} start
    echoG Cleaned and service started.
    exit 0
}

testrunning()
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

    echoG lsup version ${LSUPVERSION}.  
    cat <<EOF
Usage: lsup.sh [-t] | [-c] | [[-d] [-r] | [-v VERSION]]
  
  -d
     Choose Debug version to upgrade or downgrade, will do clean like -c at the same time.
  
  -v VERSION
     If VERSION is given, this command will try to install specified VERSION. Otherwise, it will get the latest version from $LSWSHOME/autoupdate/release.

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
    elif [ "x$1" = "--help" ] ; then
        display_usage
    elif [ "x$1" = "x-v" ] ; then
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
        testrunning
    else 
        display_usage
    fi
done

if [ "x${VERSION}" = "x" ] ; then
    echoR "Can not get the right version for installation, quit."
    exit 6
fi


if [ -f ${LOCKFILE} ] ; then
    echoR "Openlitespeed is updating, quit."
    exit 0
fi

touch ${LOCKFILE}
cd /tmp
if [ -f ols.tgz ] ; then
    rm -f ols.tgz
fi




if [ "x${ISLINUX}" = "xyes" ] ; then
    URL=https://openlitespeed.org/packages/openlitespeed-${VERSION}.tgz
    if [ "$ISDEBUG" = "yes" ] ; then
        URL=https://openlitespeed.org/packages/openlitespeed-${VERSION}.d.tgz
    fi
else
    URL=https://openlitespeed.org/packages/openlitespeed-${VERSION}.src.tgz
fi
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
    echoR "Failed to download $URL, will try our under development version."
    
    if [ "x${ISLINUX}" = "xyes" ] ; then
        URL=https://openlitespeed.org/preuse/openlitespeed-${VERSION}.tgz
        if [ "$ISDEBUG" = "yes" ] ; then
            URL=https://openlitespeed.org/preuse/openlitespeed-${VERSION}.d.tgz
        fi
    else
        URL=https://openlitespeed.org/preuse/openlitespeed-${VERSION}.src.tgz
    fi
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
        echoR "Error again, failed to download $URL, quit."
        rm -rf ${LOCKFILE}
        exit 7
    fi
fi

tar xf ols.tgz
if [ "x${ISLINUX}" = "xyes" ] ; then
    SRCDIR=/tmp/openlitespeed
else
    SRCDIR=/tmp/openlitespeed-${VERSION}
fi

if [ -f ${LSWSHOME}/VERSION ] ; then 
    if [ ! -f ${LSWSHOME}/VERSION.org ] ; then 
        cp ${LSWSHOME}/VERSION ${LSWSHOME}/VERSION.org
    fi
    
    cp -f ${LSWSHOME}/VERSION ${LSWSHOME}/VERSION.old
fi

${LSWSCTRL} stop
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
        echoR "This version does not have ./build.sh, please run ./configure; make; make install to install it."
        echoG Usually ./configure need some parameters, this auto tool cannot continue. Exit. 
        rm -rf ${LOCKFILE}
        exit 8
    fi
fi


#Change to old server root when it is updating.
if [ -f ${LSWSHOME}/bin/openlitespeed ] ; then
    echo "SERVERROOT=${LSWSHOME}" > ols.conf
    echo  >> ols.conf
fi

./install.sh

rm -rf ${LOCKFILE}

${LSWSCTRL} start
echoG All binaries are updated and service is on.
echo 
echo



