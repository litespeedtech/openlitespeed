#!/bin/sh
##############################################################################
#    Open LiteSpeed is an open source HTTP server.                           #
#    Copyright (C) 2013 - 2024 LiteSpeed Technologies, Inc.                  #
#                                                                            #
#    This program is free software: you can redistribute it and/or modify    #
#    it under the terms of the GNU General Public License as published by    #
#    the Free Software Foundation, either version 3 of the License, or       #
#    (at your option) any later version.                                     #
#                                                                            #
#    This program is distributed in the hope that it will be useful,         #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of          #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            #
#    GNU General Public License for more details.                            #
#                                                                            #
#    You should have received a copy of the GNU General Public License       #
#    along with this program. If not, see http://www.gnu.org/licenses/.      #
##############################################################################

###    Author: dxu@litespeedtech.com (David Shue)

VERSION=1.0.2
OS=`uname`
if [ "${OS}" = "Linux" ] ; then
    ISLINUX=yes
    ARCH=`arch`
else
    ISLINUX=no
    ARCH=
fi
if [ -f /etc/alpine-release ] ; then
    OSTYPE=ALPINE
else
    OSTYPE=unknown
fi
VERSIONNUMBER=
DOSUDO=
MOD_PAGESPEED="ON"
MOD_SECURITY="ON"
MOD_LUA="ON"

if [ "${OS}" = "FreeBSD" ] ; then
    APP_MGRS="pkg"
elif [ "${OS}" = "Linux" ] ; then
    APP_MGRS="apt apt-get dnf yum zypper apk"
elif [ "${OS}" = "Darwin" ] ; then
    APP_MGRS="port brew"
else
    echo 'Operating System not Linux, Mac and FreeBSD, quit.'
    exit 1
fi

APP_MGR_CMD=
for APP_MGR in ${APP_MGRS}; do
  APP_MGR_CHECK=`which ${APP_MGR} &>/dev/null`
  if [ $? -eq 0 ] ; then
    APP_MGR_CMD="${APP_MGR}"
    FULL_APP_MGR_CMD=$APP_MGR_CMD
    break
  fi
done

echo OS is ${OS}, APP_MGR_CMD is ${APP_MGR_CMD}.
if [ "x${APP_MGR_CMD}" = "x" ] ; then 
    echo 'Can not find package installation command, quit.'
    exit 1
fi


# getVersionNumber0()
# {
#     STRING=$1
#     VERSIONNUMBER=1000000
#     IFS='.';
#     parts=( ${STRING} )
#     unset IFS;
#     if [ x${parts[2]} = 'x' ] ; then
#         VERSIONNUMBER=$(( 1000000 * ${parts[0]} + 1000 * ${parts[1]} ))
#     else
#     #When x${parts[2]} is not empty, add 1 more to make sure 1.5 and 1.5.0 is not same
#         VERSIONNUMBER=$(( 1000000 * ${parts[0]} + 1000 * ${parts[1]} + ${parts[2]} + 1 ))
#     fi
# }


getVersionNumber()
{
    STRING=$1
    VER1=`echo $STRING | awk -F. '{ printf("%d", $1); }'`
    VER2=`echo $STRING | awk -F. '{ printf("%d", $2); }'`
    VER3=`echo $STRING | awk -F. '{ printf("%d", $3); }'`
    VERSIONNUMBER=$(( 1000000 * ${VER1} + 1000 * ${VER2} + ${VER3} + 1 ))
}


usage()
{
    echo "The build script supports the following options:"
    echo "   -s : Run package installation functions with 'sudo' so that they work."
    echo "        This should always be done for a first time build,"
    echo "        but requires manually entering the root password"
    echo "   -d : Debug build"
    echo "   -p ON|OFF : Whether you want pagespeed compiled or not.  Defaults ON only for x64 Linux"
    echo "   -o ON|OFF : Whether you want mod_security compiled or not.  Defaults ON for everywhere but Mac"
    echo "   -l ON|OFF : Whether you want lua compiled or not.  Defaults ON only for x64 Linux"
    exit 1
}


validateOptArg()
{
    OPTARG=`echo $1 | tr '[:lower:]' '[:upper:]'`
    if [ "$OPTARG" != "OFF" ] && [ "$OPTARG" != "ON" ]; then
        echo "You must specify OFF or ON for a compilation option - you specified $OPTARG"
        exit 1
    fi
}


getOptions()
{
    MOD_PAGESPEED="OFF"
    MOD_LUA="OFF"
    if [ "${ISLINUX}" = "yes" ] && [ "${ARCH}" = "x86_64" ]; then
        if [ ! "${OSTYPE}" = "ALPINE" ] ; then
            MOD_PAGESPEED="ON"
            MOD_LUA="ON"
            ALPINE="ON"
        fi
    fi
    if [ "${OS}" = "Darwin" ] ; then
        MOD_SECURITY="OFF"
    else
        MOD_SECURITY="ON"
    fi
    while getopts ":sdp:o:l:q:i:u:" opt; do
        case $opt in
            s)
                DOSUDO="sudo"
                FULL_APP_MGR_CMD="$DOSUDO $FULL_APP_MGR_CMD"
                echo "Running sudo with package installation/check"
                ;;
            d)
                DEBUG="ON"
                echo "Debug mode"
                ;;
            p)
                validateOptArg $OPTARG
                MOD_PAGESPEED=$OPTARG
                echo "MOD_PAGESPEED=$MOD_PAGESPEED"
                ;;
            o)
                validateOptArg $OPTARG
                MOD_SECURITY=$OPTARG
                echo "MOD_SECURITY=$MOD_SECURITY"
                ;;
            l)
                validateOptArg $OPTARG
                MOD_LUA=$OPTARG
                echo "MOD_LUA=$MOD_LUA"
                ;;
            *)
                echo "Unsupported argument: $arg"
                usage
                ;;
        esac
    done
    shift $((OPTIND-1))
    if [ $# -gt 0 ]; then
        if [ "x$1" = "xDebug" ]; then
            DEBUG="ON"
            echo "Debugging mode triggered by traditional command line"
        fi
    fi
}


installCmake()
{
    if [ "${APP_MGR_CMD}" = "apk" ] ; then
        ${FULL_APP_MGR_CMD} add --update git cmake
    else
        ${FULL_APP_MGR_CMD} install -y git cmake
    fi
    
    CMAKEVER=`cmake --version | grep version | awk  '{print $3}'`
    getVersionNumber $CMAKEVER
        
    if [ $VERSIONNUMBER -gt 3000000 ] ; then
        echo cmake installed.
        return
    fi
    
    version=3.14
    build=5
    mkdir cmaketemp
    CURDIR=`pwd`
    cd ./cmaketemp
    wget https://cmake.org/files/v${version}/cmake-${version}.${build}.tar.gz
    tar -xzvf cmake-${version}.${build}.tar.gz
    cd cmake-${version}.${build}/
    
    ./bootstrap
    jobs=$(nproc)
    make -j${jobs}
    make install
    cmake --version
    cd ${CURDIR}
}

# lsrecaptcha requirement
installgo()
{
    if [ "${APP_MGR_CMD}" = "apk" ] ; then
        ${FULL_APP_MGR_CMD} add --update go
    else
        ${FULL_APP_MGR_CMD} -y install golang-go
    fi

    which go
    
    if [ $? = 0 ] ; then
        echo go installed.
    else
        wget https://golang.org/dl/go1.16.3.linux-amd64.tar.gz
        tar -xvf go1.16.3.linux-amd64.tar.gz
        mv -f go /usr/local
        export PATH=/usr/local/go/bin:${PATH}
    fi
}


preparelibquic()
{
    if [ -e lsquic ] ; then
        ls src/ | grep liblsquic
        if [ $? -eq 0 ] ; then
            echo Need to git download the submodule ...
            rm -rf lsquic
            git clone https://github.com/litespeedtech/lsquic.git
            cd lsquic
            
            LIBQUICVER=`cat ../LSQUICCOMMIT`
            echo "LIBQUICVER is ${LIBQUICVER}"
            git checkout ${LIBQUICVER}
            git submodule update --init --recursive
            cd ..
            
        fi
    fi
}

prepareLinux()
{
    OSTYPE=unknownlinux
    if [ -f /etc/redhat-release ] ; then
        OSTYPE=CENTOS
        ${FULL_APP_MGR_CMD} update -y
        ${FULL_APP_MGR_CMD} install -y epel-release 
        output=$(cat /etc/redhat-release)
        if echo $output | grep " 7."; then
            OSTYPE=CENTOS7
        elif echo $output | grep " 8."; then
            OSTYPE=CENTOS8
        elif echo $output | grep " release 8"; then
            OSTYPE=CENTOS8
        elif echo $output | grep " 9."; then
            OSTYPE=CENTOS9
        elif echo $output | grep " release 9"; then
            OSTYPE=CENTOS9
        fi
        
        if [ "${OSTYPE}" = "CENTOS7" ] ; then
            if [ ! -f ./installing ] ; then    
                ${FULL_APP_MGR_CMD} -y install centos-release-scl
                which yum-config-manager
                if [ $? = 0 ] ; then
                    $DOSUDO yum-config-manager --enable rhel-server-rhscl-7-rpms
                fi

                ${FULL_APP_MGR_CMD} -y install devtoolset-7
                touch ./installing
                scl enable devtoolset-7 "$0"
                rm ./installing
                exit 0
            fi
            
        elif [ "${OSTYPE}" = "CENTOS8" ] || [ "${OSTYPE}" = "CENTOS9" ] ; then
            $DOSUDO dnf -y groupinstall "Development Tools"

        else
            echo This script only works on 7/8/9 for centos family._Static_assert
            exit 1
        fi
        
        ${FULL_APP_MGR_CMD} -y install git cmake
        installCmake
        
        ${FULL_APP_MGR_CMD} -y install libtool autoconf automake wget go clang patch expat-devel libcap-devel libaio-devel
        if [ "${ARCH}" = "aarch64" ]; then
            ${FULL_APP_MGR_CMD} -y install libatomic
        fi
        
    #now for debian and Ubuntu    
    elif [ -f /etc/debian_version ] ; then
        if [ -f /etc/lsb-release ] ; then
            output=$(cat /etc/*release)
            if echo $output | grep "18.04"; then
                OSTYPE=UBUNTU18
            elif echo $output | grep "20.04"; then
                OSTYPE=UBUNTU20
            elif echo $output | grep "22.04"; then
                OSTYPE=UBUNTU22
            fi
        elif [ -f /etc/debian_version ] ; then
            output=$(cat /etc/*release)
            if echo $output | grep "Debian GNU/Linux 9"; then
                OSTYPE=DEBIAN9
            elif echo $output | grep "Debian GNU/Linux 10"; then
                OSTYPE=DEBIAN10
            elif echo $output | grep "Debian GNU/Linux 11"; then
                OSTYPE=DEBIAN11
            fi
        fi
        
        #other debian OS, we still can 
        if [ "${OSTYPE}" = "unknownlinux" ] ; then
            echo It seems you are not using ubuntu 18/20/22 and Debian 9/10/11.
            echo But we still can try to go further.
        fi
        
        ${FULL_APP_MGR_CMD} -y update
        ${FULL_APP_MGR_CMD} -f -y install
        ${FULL_APP_MGR_CMD} -y install gcc g++ wget curl make clang patch libexpat-dev libcap-dev
        
        installCmake
        ${FULL_APP_MGR_CMD} -y install git libtool ca-certificates autotools-dev autoconf automake libaio-dev
        installgo

        if [ "${ARCH}" = "aarch64" ]; then
            ${FULL_APP_MGR_CMD} -y install libatomic1
        fi

    elif [ -f /etc/alpine-release ] ; then
        OSTYPE=ALPINE
        ${FULL_APP_MGR_CMD} add make
        ${FULL_APP_MGR_CMD} add gcc g++
        ${FULL_APP_MGR_CMD} add patch
        installCmake
        ${FULL_APP_MGR_CMD} add git libtool linux-headers bsd-compat-headers curl
        ${FULL_APP_MGR_CMD} add automake autoconf
        ${FULL_APP_MGR_CMD} add build-base expat-dev zlib-dev libcap-dev libaio-dev
        installgo
        sed -i -e "s/u_int32_t/uint32_t/g" $(grep -rl u_int32_t src/)
        sed -i -e "s/u_int64_t/uint64_t/g" $(grep -rl u_int64_t src/)
        sed -i -e "s/u_int8_t/uint8_t/g" $(grep -rl u_int8_t src/)
        sed -i -e "s@<sys/sysctl.h>@<linux/sysctl.h>@g" $(grep -rl "<sys/sysctl.h>" src/)
        sed -i -e "s/PTHREAD_MUTEX_ADAPTIVE_NP/PTHREAD_MUTEX_NORMAL/g" src/lsr/ls_lock.c    
        
    else 
        echo May not support your platform, but we can do a try to install some tools.
        ${FULL_APP_MGR_CMD} -y update
        ${FULL_APP_MGR_CMD} -y install make
        ${FULL_APP_MGR_CMD} -y install clang 
        ${FULL_APP_MGR_CMD} -y install patch 
        installCmake
        ${FULL_APP_MGR_CMD} -y install git libtool ca-certificates
        ${FULL_APP_MGR_CMD} -y install autotools-dev libcap-dev
        ${FULL_APP_MGR_CMD} -y install autoconf
        ${FULL_APP_MGR_CMD} -y install autoheader 
        ${FULL_APP_MGR_CMD} -y install automake 
        installgo
        
    fi
}


prepareBsd()
{
    echo 'OS Type is `freebsd-version -k`'
    pkg install -f -y wget
    pkg install -f -y curl
    pkg install -f -y lang/gcc
    pkg install -f -y cmake
    pkg install -f -y git
    pkg install -f -y libtool
    pkg install -f -y autoconf 
    pkg install -f -y automake 
    pkg install -f -y go 
    pkg install -f -y patch 
    pkg install -f -y python
    pkg install -f -y gmake
    
    #something can not install through pkg will use ports
    #portsnap fetch
    #portsnap extract
    #cd /usr/ports/textproc/libxml2
    #make install clean BATCH=yes
}

prepareMac()
{
    echo OS Type is `sw_vers`
    if [ "${APP_MGR_CMD}" = "port" ] ; then 
        ${FULL_APP_MGR_CMD} selfupdate
        ${FULL_APP_MGR_CMD} -f -N install wget
        ${FULL_APP_MGR_CMD} -f -N install curl                             
        ${FULL_APP_MGR_CMD} -f -N install cmake                          
        ${FULL_APP_MGR_CMD} -f -N install git                            
        ${FULL_APP_MGR_CMD} -f -N install libtool                        
        ${FULL_APP_MGR_CMD} -f -N install autoconf                       
        ${FULL_APP_MGR_CMD} -f -N install automake                       
        ${FULL_APP_MGR_CMD} -f -N install go                             
        ${FULL_APP_MGR_CMD} -f -N install patch                          
        ${FULL_APP_MGR_CMD} -f -N install python                         
        ${FULL_APP_MGR_CMD} -f -N install gmake      
    else
        echo You need to install these tools "wget cmake git libtool autoconf automake go patch python gmake"
        ${FULL_APP_MGR_CMD} install -f wget
        ${FULL_APP_MGR_CMD} install -f curl                             
        ${FULL_APP_MGR_CMD} install -f cmake                          
        ${FULL_APP_MGR_CMD} install -f git                            
        ${FULL_APP_MGR_CMD} install -f libtool                        
        ${FULL_APP_MGR_CMD} install -f autoconf                       
        ${FULL_APP_MGR_CMD} install -f automake                       
        ${FULL_APP_MGR_CMD} install -f go                             
        ${FULL_APP_MGR_CMD} install -f patch                          
        ${FULL_APP_MGR_CMD} install -f python                         
        ${FULL_APP_MGR_CMD} install -f gmake      
    
    fi
    
}

commentout()
{
    sed -i -e "s/$1/#$1/g" $2
}



updateSrcCMakelistfile()
{   
    OS=`uname`
    commentout 'add_definitions(-DRUN_TEST)'                 CMakeLists.txt 
    commentout 'add_definitions(-DPOOL_TESTING)'             CMakeLists.txt
    commentout 'add_definitions(-DTEST_OUTPUT_PLAIN_CONF)'   CMakeLists.txt
    commentout 'add_definitions(-DDEBUG_POOL)'   CMakeLists.txt
    
    commentout 'set(libUnitTest'  CMakeLists.txt
    
    commentout 'find_package(ZLIB'  CMakeLists.txt
    commentout 'find_package(PCRE'  CMakeLists.txt
   # commentout 'find_package(EXPAT REQUIRED)'
    commentout 'add_subdirectory(test)'   CMakeLists.txt
    
    
    commentout 'SET (CMAKE_C_COMPILER'  CMakeLists.txt
    commentout 'SET (CMAKE_CXX_COMPILER'   CMakeLists.txt
    
    sed -i -e "s/\${unittest_STAT_SRCS}//g"  src/CMakeLists.txt
    sed -i -e "s/libstdc++.a//g"  src/CMakeLists.txt
    sed -i -e "s/-nodefaultlibs //g"  src/CMakeLists.txt
    sed -i -e "s/-nodefaultlibs libstdc++.a//g"  src/modules/modsecurity-ls/CMakeLists.txt
    sed -i -e "s/-nodefaultlibs libstdc++.a//g"  src/modules/lua/CMakeLists.txt

    commentout  ls_llmq.c  src/lsr/CMakeLists.txt
    commentout  ls_llxq.c  src/lsr/CMakeLists.txt
    
    if [ "${OS}" = "Darwin" ] ; then
        sed -i -e "s/ rt//g" src/CMakeLists.txt
        sed -i -e "s/ crypt//g"  src/CMakeLists.txt
        sed -i -e "s/gcc_eh//g"  src/CMakeLists.txt
        sed -i -e "s/c_nonshared//g"  src/CMakeLists.txt
        sed -i -e "s/gcc//g"  src/CMakeLists.txt
        sed -i -e "s/-Wl,--whole-archive//g"  src/CMakeLists.txt
        sed -i -e "s/-Wl,--no-whole-archive//g"  src/CMakeLists.txt
    fi
    
    if [ "${OSTYPE}" = "ALPINE" ] ; then
        sed -i -e "s/c_nonshared//g"  src/CMakeLists.txt
        sed -i -e "s/c_nonshared//g"  src/modules/modsecurity-ls/CMakeLists.txt
    fi
}


cpModuleSoFiles()
{
    if [ ! -d dist/modules/ ] ; then
        mkdir dist/modules/
    fi
    
    for module in build/src/modules/*; do
        cp -f ${module}/*.so dist/modules/
    done
    
    if [ -e build/src/modules/modsecurity-ls/mod_security.so ] ; then
        cp -f build/src/modules/modsecurity-ls/mod_security.so dist/modules/
    fi

    if [ -e build/src/modules/lua/mod_lua.so ] ; then
        cp -f build/src/modules/lua/mod_lua.so dist/modules/
    fi

    if [ -e build/src/modules/pagespeed/modpagespeed.so ] ; then
        cp -f build/src/modules/pagespeed/modpagespeed.so dist/modules/
    fi
}


fixshmdir()
{
    if [ ! -d /dev/shm ] ; then
        mkdir /tmp/shm
        chmod 777  /tmp/shm
        sed -i -e "s/\/dev\/shm/\/tmp\/shm/g" dist/conf/httpd_config.conf.in
        if [ "${OS}" = "FreeBSD" ] ; then
            echo 'link /tmp/shm shm' >> /etc/devfs.conf
        fi
    fi
}


freebsdFix()
{   
    if [ "${OS}" = "FreeBSD" ] ; then
        NEEDREBOOT=yes
        if [ -e /etc/sysctl.conf ] ; then 
            cat /etc/sysctl.conf | grep  "security.bsd.conservative_signals=0" >/dev/null
            if [ $? -eq 0 ] ; then
                NEEDREBOOT=no
            fi
        fi
        
        if [ "${NEEDREBOOT}" = "yes" ] ; then 
            echo "security.bsd.conservative_signals=0" >> /etc/sysctl.conf
            echo You must reboot the server to ensure the settings change take effect. 
            touch ./needreboot.txt
        fi
    fi
}


fixPagespeed()
{
PSOLVERSION=1.11.33.4
cat << EOF > ../third-party/psol-$PSOLVERSION/include/pagespeed/kernel/base/scoped_ptr.h
/**
* Due the compiling issue, this file was updated from the original file.
*/
#ifndef PAGESPEED_KERNEL_BASE_SCOPED_PTR_H_
#define PAGESPEED_KERNEL_BASE_SCOPED_PTR_H_
#include "base/memory/scoped_ptr.h"

namespace net_instaweb {
template<typename T> class scoped_array : public scoped_ptr<T[]> {
public:
    scoped_array() : scoped_ptr<T[]>() {}
    explicit scoped_array(T* t) : scoped_ptr<T[]>(t) {}
};
}
#endif

EOF

}

cd `dirname "$0"`
CURDIR=`pwd`

getOptions "$@"

if [ "${OS}" = "FreeBSD" ] ; then
    prepareBsd
elif [ "${OS}" = "Linux" ] ; then
    ISLINUX=yes
    prepareLinux
elif [ "${OS}" = "Darwin" ] ; then
    prepareMac
fi



cd ..
if [ ! -d third-party ]; then

    git clone https://github.com/litespeedtech/third-party.git
    mkdir third-party/lib64
    cd third-party/script/

    #Remove  unittest-cpp and add bcrypt
    sed -i -e "s/unittest-cpp/bcrypt/g" ./build_ols.sh
    if [ "$MOD_SECURITY" != "ON" ]; then
        sed -i -e "s/libmodsec/ /g" ./build_ols.sh
    fi
    if [ "$MOD_PAGESPEED" != "ON" ]; then
        sed -i -e "s/psol/ /g"  ./build_ols.sh
    fi
    if [ "$MOD_LUA" != "ON" ]; then
        sed -i -e "s/luajit/ /g" ./build_ols.sh
    fi

    ./build_ols.sh

fi

cd ${CURDIR}

updateSrcCMakelistfile
preparelibquic


if [ "$MOD_PAGESPEED" = "ON" ]; then
    fixPagespeed
fi

if [ "$MOD_SECURITY" = "ON" ]; then
    #special case modsecurity
    cd src/modules/modsecurity-ls
    ln -sf ../../../../third-party/src/ModSecurity .
    cd ../../../
    #Done of modsecurity
fi

cd src/modules/lsrecaptcha
export GOPATH=$CUR_PATH/src/modules/lsrecaptcha
export GO111MODULE=off
go build lsrecaptcha
cp lsrecaptcha ../../../dist/lsrecaptcha/_recaptcha
cd ../../../

fixshmdir

if [ "$DEBUG" = "ON" ]; then
    BUILD=Debug
else
    BUILD=RelWithDebInfo
fi

if [ ! -d build ]; then
    mkdir build
fi

rm -f CMakeCache.txt
rm -f build/CMakeCache.txt

cd build
if [ "$OSTYPE" = "ALPINE" ]; then
    CMAKE_ALPINE="-DALPINE=ON"
else
    CMAKE_ALPINE=
fi

cmake -DCMAKE_BUILD_TYPE=$BUILD $CMAKE_ALPINE \
    -DMOD_PAGESPEED=$MOD_PAGESPEED \
    -DMOD_SECURITY=$MOD_SECURITY \
    -DMOD_LUA=$MOD_LUA \
    ..
jobs=$(nproc)
make -j${jobs}
cd ..

cp build/src/openlitespeed  dist/bin/
if [ -e build/support/unmount_ns/unmount_ns ] ; then
    cp build/support/unmount_ns/unmount_ns  dist/lsns/bin/
fi

if [ -e build/support/cmd_ns/cmd_ns ] ; then
    cp build/support/cmd_ns/cmd_ns  dist/lsns/bin/
fi

if [ -e build/lshostexec/lshostexec ] ; then
    cp build/lshostexec/lshostexec  dist/lsns/bin/
fi


cpModuleSoFiles

#Version >= 1.6.0 which has QUIC need to fix for freebsd
if [ -e src/liblsquic ] ; then 
    freebsdFix
fi

cat > ./ols.conf <<END 
#If you want to change the default values, please update this file.
#

SERVERROOT=/usr/local/lsws
OPENLSWS_USER=nobody
OPENLSWS_GROUP=nobody
OPENLSWS_ADMIN=admin
OPENLSWS_EMAIL=root@localhost
OPENLSWS_ADMINSSL=yes
OPENLSWS_ADMINPORT=7080
USE_LSPHP7=yes
DEFAULT_TMP_DIR=/tmp/lshttpd
PID_FILE=/tmp/lshttpd/lshttpd.pid
OPENLSWS_EXAMPLEPORT=8088

#You can set password here
#OPENLSWS_PASSWORD=

END


echo Start to pack files.
mv dist/install.sh  dist/_in.sh

cat > ./install.sh <<END 
#!/bin/sh

SERVERROOT=/usr/local/lsws
OPENLSWS_USER=nobody
OPENLSWS_GROUP=nobody
OPENLSWS_ADMIN=admin
OPENLSWS_PASSWORD=
OPENLSWS_EMAIL=root@localhost
OPENLSWS_ADMINSSL=yes
OPENLSWS_ADMINPORT=7080
USE_LSPHP7=yes
DEFAULT_TMP_DIR=/tmp/lshttpd
PID_FILE=/tmp/lshttpd/lshttpd.pid
OPENLSWS_EXAMPLEPORT=8088
CONFFILE=./ols.conf
    
#script start here
cd `dirname "\$0"`

if [ -f \${CONFFILE} ] ; then
    . \${CONFFILE}
fi

cd dist

mkdir -p \${SERVERROOT} >/dev/null 2>&1


PASSWDFILEEXIST=no
if [ -f \${SERVERROOT}/admin/conf/htpasswd ] ; then
    PASSWDFILEEXIST=yes
else
    PASSWDFILEEXIST=no
    #Generate the random PASSWORD if not set in ols.conf
    if [ "x\$OPENLSWS_PASSWORD" = "x" ] ; then
        OPENLSWS_PASSWORD=\`openssl rand -base64 6\`
        if [ "x\$OPENLSWS_PASSWORD" = "x" ] ; then
            TEMPRANDSTR=\`ls -l ..\`
            TEMPRANDSTR=\`echo "\${TEMPRANDSTR}" |  md5sum | base64 | head -c 8\`
            if [ \$? = 0 ] ; then
                OPENLSWS_PASSWORD=\${TEMPRANDSTR}
            else
                OPENLSWS_PASSWORD=123456
            fi
        fi
        echo OPENLSWS_PASSWORD=\${OPENLSWS_PASSWORD} >> ./ols.conf
    fi

    echo "WebAdmin user/password is admin/\${OPENLSWS_PASSWORD}" > \$SERVERROOT/adminpasswd
    chmod 600 \$SERVERROOT/adminpasswd
fi


#Change to nogroup for debain/ubuntu
if [ -f /etc/debian_version ] ; then
    if [ "\${OPENLSWS_GROUP}" = "nobody" ] ; then
        OPENLSWS_GROUP=nogroup
    fi
fi 


ISRUNNING=no

if [ -f \${SERVERROOT}/bin/openlitespeed ] ; then 
    echo Openlitespeed web server exists, will upgrade.
    
    \${SERVERROOT}/bin/lswsctrl status | grep ERROR
    if [ \$? != 0 ]; then
        ISRUNNING=yes
    fi
fi

./_in.sh "\${SERVERROOT}" "\${OPENLSWS_USER}" "\${OPENLSWS_GROUP}" "\${OPENLSWS_ADMIN}" "\${OPENLSWS_PASSWORD}" "\${OPENLSWS_EMAIL}" "\${OPENLSWS_ADMINSSL}" "\${OPENLSWS_ADMINPORT}" "\${USE_LSPHP7}" "\${DEFAULT_TMP_DIR}" "\${PID_FILE}" "\${OPENLSWS_EXAMPLEPORT}" no

cp -f modules/*.so \${SERVERROOT}/modules/
cp -f bin/openlitespeed \${SERVERROOT}/bin/

if [ "\${PASSWDFILEEXIST}" = "no" ] ; then
    echo -e "\e[31mYour webAdmin password is \${OPENLSWS_PASSWORD}, written to file \$SERVERROOT/adminpasswd.\e[39m"
else
    echo -e "\e[31mYour webAdmin password not changed.\e[39m"
fi
echo

    
if [ -f ../needreboot.txt ] ; then
    rm ../needreboot.txt
    echo -e "\e[31mYou must reboot the server to ensure the settings change take effect!\e[39m"
    echo
    exit 0
fi 

if [ "\${ISRUNNING}" = "yes" ] ; then
    \${SERVERROOT}/bin/lswsctrl start
fi

END

chmod 777 ./install.sh

echo -e "\033[38;5;71mBuilding finished, please run ./install.sh for installation.\033[39m"
echo -e "\033[38;5;71mYou may want to update the ols.conf to change the settings before installation.\033[39m"
echo -e "\033[38;5;71mEnjoy.\033[39m"

