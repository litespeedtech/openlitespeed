#! /bin/sh
#
# This script is to download boringSSL and build
# Or,
# Use your pre-built boringSSL


CUR_DIR=`pwd`
cd `dirname "$0"`

DLCMD=
source dist/functions.sh 2>/dev/null
if [ $? != 0 ] ; then
    . dist/functions.sh
    if [ $? != 0 ] ; then
        echo [ERROR] Can not include 'functions.sh'.
        cd $CUR_DIR
        exit 1
    fi
fi

BSSLDIR=boringssl
if [ "x$1" != "x" ] ; then 
    BSSLDIR=$1
fi

if [ -f ssl/libcrypto.a ] ; then
   echo File exists 
    
elif [ ! -f $BSSLDIR/build/ssl/libssl.a ] ; then
    BSSLDIR=boringssl
    git clone https://github.com/google/boringssl.git
    
    if [ -d "go" ]; then
        PATH=$PWD/go/bin:$PATH
        echo $PATH
    fi

    cd boringssl
    git reset --hard
    git checkout master
    git pull

    git checkout 32e59d2d3264e4e104b355ef73663b8b79ac4093

    rm -rf build

    detectdlcmd
    $DLCMD 1.patch https://raw.githubusercontent.com/litespeedtech/third-party/master/patches/boringssl/bssl_inttypes.patch
    $DLCMD 2.patch https://raw.githubusercontent.com/litespeedtech/third-party/master/patches/boringssl/bssl_max_early_data_sz.patch
    $DLCMD 3.patch https://raw.githubusercontent.com/litespeedtech/third-party/master/patches/boringssl/bssl_no_eoed.patch
    
    #patch -p1 < ../bssl_lstls.patch
    patch -p1 < 1.patch
    patch -p1 < 2.patch
    patch -p1 < 3.patch

    mkdir build
    cd build
    cmake ../ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-fPIC"
    make -j4
    mkdir ssl
    cd ssl
    make -j4
    cd ../decrepit
    make -j4
    cd ../../..
fi

if [ ! -f ssl/libcrypto.a ] ; then
    mkdir ssl
    cp $BSSLDIR/build/ssl/libssl.a           ssl/
    cp $BSSLDIR/build/crypto/libcrypto.a     ssl/
    cp $BSSLDIR/build/decrepit/libdecrepit.a ssl/
    cp -r $BSSLDIR/include                   ssl/
fi

    
if [ ! -f ssl/libcrypto.a ] ; then
    echo -e "\033[38;5;148mError: failed to make boringSSL libraries.\033[39m"
    cd $CUR_DIR
    exit 1
else
    echo -e "\033[38;5;148mOK, openssl libraries made.\033[39m"
fi

cd $CUR_DIR
exit 0
