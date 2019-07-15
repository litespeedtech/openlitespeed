#! /bin/sh

VERSION=v2.0-7/5/2019
LOCKFILE=/tmp/olsupdatingflag

echoc()
{
	if [ $# -eq 0 ] ; then
		echo
	else
		echo -e "\033[38;5;148m$@\033[39m"
	fi
}

if [ "x$1" = "x-v" ] || [ "x$1" = "x--version" ] ; then
    echoc "Version $VERSION"
    exit 0
fi


OS=`uname`
if [ "x$OS" != "xLinux" ] ; then
    echoc "Error: this tool only works on Linux. Quit."
    exit 1
fi


if [ "x$1" = "x-h" ] || [ "x$1" = "x--help" ] ; then
    echoc "This tool is for pickup the binaries from the specified version of openlitespeed package"
    echoc "Usage: "
    echoc "1, $0 [-d] [Version] [SERVER_ROOT], will install the specified version binaries"
    echoc "         such as .\>$0 1.4.45"
    echoc "         The default version is the latest 1.4.x "
    echoc "         The default server root is /usr/local/lsws "
    echoc "         If -d is added, will use the DEBUG build instead of release build."
    echoc "2, $0 -r [SERVER_ROOT], will recover to your original installed version"
    echoc "3, $0 -h, display help"
    echoc "4, $0 -v, display version"
    echoc
    exit 0
fi


if [ -f $LOCKFILE ] ; then
    echoc "Openlitespeed is updating, quit."
    exit 0
fi
touch $LOCKFILE

ISDEBUG=no
if [ "x$1" = "x-d" ] ; then
    ISDEBUG=yes
    shift
fi

ISRECOVER=no
if [ "x$1" = "x-r" ] ; then
    #recover mode
    ISRECOVER=yes
else
    which wget
    if [ $? != 0 ] ; then
        echoc It seems you do not have wget installed, please install it first and try again.
        rm -rf $LOCKFILE
        exit 1
    fi

    if [ "x$1" = "x" ] ; then
        wget -nv -O release https://openlitespeed.org/preuse/release
        if [ $? != 0 ] ; then
            echoc Can not download the latest version file
            rm -rf $LOCKFILE
            exit 1
        else
            VERSION=`cat release`
        fi
    else
        VERSION=$1
    fi
VER=$VERSION
fi

SERVERROOT=/usr/local/lsws
if  [ "x$2" != "x" ] ; then
    SERVERROOT=$2
fi

if [ "x$ISRECOVER" = "xno" ] ; then
    echoc Version v$VER will going to be used, server root is $SERVERROOT.
else 
    echoc Will recover. The server root is $SERVERROOT.
fi

if [ ! -f $SERVERROOT/bin/openlitespeed ] ; then
    rm -rf $LOCKFILE
    echoc It seems you do not have openlitespeed installed in $SERVERROOT.
    echoc So can not install/recover by this tool.
    exit 1
fi


if [ "x$ISRECOVER" = "xyes" ] ; then
    
    if [ ! -f $SERVERROOT/bin/openlitespeed.old ] ; then 
        rm -rf $LOCKFILE
        echoc Can not find $SERVERROOT/bin/openlitespeed.old, can not recover
        echoc Quit.
        exit 1
    fi
    
    cd $SERVERROOT
    bin/lswsctrl stop
    
    cd bin/
    rm openlitespeed
    mv openlitespeed.old openlitespeed

    rm lswsctrl
    mv lswsctrl.old lswsctrl

    
    cd ../modules
    rm cache.so
    mv cache.so.old cache.so

    if [ -f modpagespeed.so ] ; then
        rm modpagespeed.so
        mv modpagespeed.so.old modpagespeed.so
    fi
    
    if [ -f mod_security.so ] ; then
        rm mod_security.so
        mv mod_security.so.old mod_security.so
    fi

    $SERVERROOT/bin/lswsctrl start
    echo All binaries are recoved and service is on.
    rm -rf $LOCKFILE
    exit 0
fi



cd /tmp
if [ -f ols.tgz ] ; then
    rm -f ols.tgz
fi

URL=https://openlitespeed.org/preuse/openlitespeed-$VER.tgz
if [ "$ISDEBUG" = "yes" ] ; then
    URL=https://openlitespeed.org/preuse/openlitespeed-$VER.d.tgz
fi

wget -nv -O ols.tgz $URL
if [ $? != 0 ] ; then
    echoc Failed to download $URL, quit.
    rm -rf $LOCKFILE
    exit 1
fi



tar xf ols.tgz
SRCDIR=/tmp/openlitespeed

cd $SERVERROOT
bin/lswsctrl stop

cd bin/
if [ -f openlitespeed.old ] ; then
    rm -f openlitespeed.old
fi
mv openlitespeed openlitespeed.old

cp -f $SRCDIR/bin/openlitespeed openlitespeed-$VER
ln -sf openlitespeed-$VER openlitespeed


if [ -f lswsctrl.old ] ; then
    rm -f lswsctrl.old
fi
mv lswsctrl lswsctrl.old

cp -f $SRCDIR/bin/lswsctrl lswsctrl-$VER
ln -sf lswsctrl-$VER lswsctrl

cd ../modules
if [ -f cache.so.old ] ; then
    rm -f cache.so.old
fi

mv cache.so  cache.so.old
cp -f $SRCDIR/modules/cache.so cache.so-$VER
ln -sf cache.so-$VER cache.so

if [ -f modpagespeed.so ] ; then

    if [ -f modpagespeed.so.old ] ; then
        rm -f modpagespeed.so.old
    fi
    mv modpagespeed.so  modpagespeed.so.old
    cp -f $SRCDIR/modules/modpagespeed.so modpagespeed.so-$VER
    ln -sf modpagespeed.so-$VER modpagespeed.so
fi

if [ -f mod_security.so ] ; then

    if [ -f mod_security.so.old ] ; then
        rm -f mod_security.so.old
    fi

    mv mod_security.so  mod_security.so.old
    cp -f $SRCDIR/modules/mod_security.so mod_security.so-$VER
    ln -sf mod_security.so-$VER mod_security.so
fi

$SERVERROOT/bin/lswsctrl start
echoc All binaries are updated and service is on.
rm -rf $LOCKFILE
echo 
echo



