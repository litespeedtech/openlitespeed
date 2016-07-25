#
# This script is to download openssl latest stable and make the static library ready
#
#
#! /bin/sh

VERSION=OpenSSL_1_0_2-stable

#a bug fix of openssl 1.0.2
fixbug()
{
    sed -i -e "s/], s->s3->wpend_tot/], len - tot/" ssl/s3_pkt.c
}


cd `dirname "$0"`

if [ ! -f openssl/libcrypto.a ] ; then
    echo -e "\033[38;5;148mDownload openssl $VERSION and building, it will take several minutes ...\033[39m"

    DL=`which curl`
    DLCMD="$DL -k -L -o ossl.tar.gz"
    $DLCMD https://github.com/openssl/openssl/archive/$VERSION.tar.gz
    tar xf ossl.tar.gz
    mv openssl-$VERSION openssl
    rm ossl.tar.gz
    cd openssl
    fixbug
    ./config
    make depend
    make
    
    if [ ! -f libcrypto.a ] ; then
        echo -e "\033[38;5;148mError: failed to make openssl libraries.\033[39m"
        exit 1
    else
        echo -e "\033[38;5;148mOK, openssl libraries made.\033[39m"
    fi
    cd ..
else
    echo "openssl libraries exists."
    exit 0
fi

echo -e "\033[38;5;148mOK, openssl libraries exists.\033[39m"
exit 0
