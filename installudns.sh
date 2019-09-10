#!/bin/sh
#Download udns source code and install it

DLCMD=
source dist/functions.sh 2>/dev/null
if [ $? != 0 ] ; then
    . dist/functions.sh
    if [ $? = 0 ] ; then
        detectdlcmd
    else
        DLCMD="curl -o"
    fi
fi


echo Will download stable version of the udns library 0.4 and install it
$DLCMD ./udns.tar.gz  http://www.corpit.ru/mjt/udns/udns-0.4.tar.gz
tar xf udns.tar.gz
cd udns-0.4/
./configure
make
cd ../
