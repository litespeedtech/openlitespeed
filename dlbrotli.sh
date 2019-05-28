#! /bin/sh

cd `dirname "$0"`
echo "Checking libbrotli ..."

DLCMD=
source dist/functions.sh 2>/dev/null
if [ $? != 0 ] ; then
    . dist/functions.sh
    if [ $? != 0 ] ; then
        echo [ERROR] Can not include 'functions.sh'.
        exit 1
    fi
fi



if [ ! -f brotli-master/out/libbrotlidec-static.a ] ; then

    detectdlcmd
    echo -e "\033[38;5;148mDownloading libbrotli latest version and building, it will take several minutes ...\033[39m"

    which cmake 
    if [ $? != 0 ] ; then
        echo -e "\033[38;5;148mError: can not find cmake, you need to install cmake to continue.\033[39m"
        exit 1
    fi

    $DLCMD br.zip https://codeload.github.com/google/brotli/zip/master
    unzip br.zip
    cd brotli-master
    
    mkdir out
    cd out
    ../configure-cmake
    make
    cd ../..
    
    if [ ! -f brotli-master/out/libbrotlidec-static.a ] ; then
        echo -e "\033[38;5;148mError: failed to make libbrotli libraries.\033[39m"
        exit 1
    else
        echo -e "\033[38;5;148mGood, libbrotli libraries made.\033[39m"
        exit 0
    fi
else
    echo -e "\033[38;5;148mLibbrotli libraries exist.\033[39m"
    exit 0
fi

