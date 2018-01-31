# To run this script, cd to the php-x.xx directory and run it as your user: ../configure_mod_lsphp.sh
#'./configure' '--with-mysqli' '--with-zlib' '--#!/bin/bash
# To run this script, cd to the php-x.xx directory and run it as your user: ../configure_mod_lsphp.sh
#‘./configure’ ‘--with-mysqli’ ‘--with-zlib’ ‘--with-gd’ ‘--enable-shmop’ ‘--enable-sockets’ ‘--enable-sysvsem’ ‘--enable-sysvshm’ ‘--enable-mbstring’ ‘--with-iconv’ ‘--with-mysql’ ‘--with-mcrypt’ ‘--with-pdo’ ‘--with-pdo-mysql’ ‘--enable-ftp’ ‘--enable-zip’ ‘--with-curl’ ‘--enable-soap’ ‘--enable-xml’ ‘--enable-json’ ‘--with-openssl’ ‘--enable-bcmath’ ‘--enable-hash’ ‘--enable-opcache’ ‘--with-tsrm-pthreads’ ‘--enable-maintainer-zts’ ‘--with-mod-lsphp=/home/user/proj/openlitespeed/’ ‘--enable-debug’  
if [ ! -d sapi ]
then
    echo 'You have to be in the PHP directory'
    exit 1
fi
if [ ! -L sapi/mod_lsphp ]
then
    echo 'Creating mod_lsphp symbolic link in sapi'
    cd sapi
    ln -s ../../src mod_lsphp
    cd ..
fi
./buildconf --force
#export CC="/usr/bin/clang -fsanitize=thread -O1 -g -fno-omit-frame-pointer -fsanitize-blacklist=blacklist.txt"
export CC="/usr/bin/clang"
#export CXX="/usr/bin/clang++ -fsanitize=thread -O1 -g -fno-omit-frame-pointer -fsanitize-blacklist=blacklist.txt -I/usr/include/c++/4.8 -I/usr/include/c++/4.8/x86_64-suse-linux"
#export CXX="/usr/bin/clang++ -I/usr/include/c++/4.8 -I/usr/include/c++/4.8/x86_64-suse-linux"
export CXX="/usr/bin/clang++"
#export CC="/usr/bin/clang"
#export CXX="/usr/bin/clang++"
#export CFLAGS="-fsanitize=thread"
#export CXXFLAGS="-fsanitize=thread"
#export LDFLAGS='-L/usr/lib64/gcc/x86_64-suse-linux/4.8'
#export ZEND_EXTRA_LIBS="-L/usr/lib64/gcc/x86_64-suse-linux/4.8"
# Remove  --with-pcre-regex=/home/user/proj/pcre-8.41 --with-mysqli
# Add --enable-debug for debugging
./configure --with-zlib --with-gd --enable-shmop --enable-sockets --enable-sysvsem --enable-sysvshm --enable-mbstring --with-iconv --with-mysqli --with-pdo-mysql --enable-ftp --enable-zip --with-curl --enable-soap --enable-xml --enable-json --with-openssl --enable-bcmath --enable-hash --enable-opcache=no --with-tsrm-pthreads --enable-maintainer-zts --enable-exif --enable-intl --with-mod-lsphp=$(pwd | sed 's;src/modules/.*;;') --disable-phar
