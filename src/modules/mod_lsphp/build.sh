#!/bin/sh
# Setup some variables to use below
set -v
set -x
WDIR=`dirname $0`
cd $WDIR
PHPMAJ="php-7.2"
PHPVER="$PHPMAJ.1"
LSDIR=`cd ../../../../; pwd`
PHPGZIP=$LSDIR/$PHPVER.tar.gz
PHP_SRC_DIR=$LSDIR/php-src-$PHPVER
PHP_MAJ_SYMLINK=$LSDIR/$PHPMAJ
PHP_DBG_INST_DIR=$LSDIR/phpdbg/$PHPMAJ
MOD_LSPHP_DIR=`pwd`
OLS_PATH=`pwd | sed 's;src/modules/.*;;'`
#PHP_INSTALL_DIR=$HOME/lsphp72

BISON=`which bison`
if [ "x$BISON" = "x" ]
then
    echo "Error: bison required, please install bison and try again"
    exit 1
fi

usage() { echo "Usage: $0 [ -f ] [ -b | -s ] [-i install_dir ]\n\t-f: force rebuild\n\t-b: big php package (takes longer to build but includes required production PHP features SQL and the like, default)\n\t-s: small php package (minimum PHP features)\n\t-i: install directory (default ${PHP_DBG_INST_DIR})" 1>&2; exit 1; }

# Decide whether to to run 'configure' for PHP or not (really slows things down, but necessary indeed) if we need to build.
NEED_CONFIGURE=0
bigpkg=0
instdir=$PHP_DBG_INST_DIR
while getopts ":fbs" o; do
    case "${o}" in
        f)
            NEED_CONFIGURE=1
            ;;
        b)
            bigpkg=1
            ;;
        s)
            bigpkg=0
            ;;
        i)
            instdir=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

prefix=$instdir

if [ $bigpkg -eq 1 ]
then
CONFIG_PARAMS=" \
--disable-cgi --enable-cli --enable-phpdbg=no \
--build=x86_64-linux-gnu --host=x86_64-linux-gnu \
--program-suffix=7.21 --disable-all --disable-debug --disable-rpath --disable-static --with-pic --with-layout=GNU \
--without-pear --enable-bcmath --enable-calendar --enable-ctype --enable-dom --with-enchant --enable-exif \
--with-gettext --with-gmp=/usr/include/x86_64-linux-gnu --enable-fileinfo --enable-filter --enable-ftp --enable-hash --with-iconv --enable-mbregex \
--enable-mbregex-backtrack --enable-mbstring --enable-phar --enable-posix --with-mcrypt --enable-mysqlnd-compression-support \
--with-zlib --with-openssl=yes --with-libedit --enable-libxml --enable-session --enable-simplexml --enable-soap \
--enable-sockets --enable-tokenizer --enable-xml --enable-xmlreader --enable-xmlwriter --with-xsl --with-mhash=yes \
--enable-sysvmsg --enable-sysvsem --enable-sysvshm --enable-zip --with-system-tzdata --with-gd --enable-gd-native-ttf \
--with-jpeg-dir --with-xpm-dir --with-png-dir --with-freetype-dir --with-vpx-dir --disable-dtrace --enable-pdo \
--enable-mysqlnd --enable-pcntl --with-sqlite3=shared,/usr --with-pdo-sqlite=shared,/usr --with-pdo-dblib=shared \
--enable-shmop --with-ldap=shared,/usr --with-ldap-sasl=/usr --enable-intl=shared --with-snmp=shared,/usr --enable-json=shared \
--with-pgsql=shared,/usr --with-pdo-pgsql=shared,/usr --enable-opcache --enable-opcache-file --enable-huge-code-pages \
--with-imap=shared,/usr --with-kerberos --with-imap-ssl=yes --with-mysqli --with-pdo-mysql \
--with-tidy=shared,/usr --with-pspell=shared --with-curl=shared build_alias=x86_64-linux-gnu host_alias=x86_64-linux-gnu"
CONFIG_PATHS="--prefix=${prefix} --with-mod-lsphp=$OLS_PATH \
--with-config-file-path=${prefix}/etc/php/7.2/litespeed/ \
--with-config-file-scan-dir=${prefix}/etc/php/7.21/mods-available/ \
--libdir=${prefix}/lib/php --libexecdir=${prefix}/lib/php \
--datadir=${prefix}/share/php/7.21"
else
CONFIG_PARAMS=" \
--with-zlib --with-gd --enable-shmop --enable-sockets \
--enable-sysvsem --enable-sysvshm --enable-mbstring \
--with-iconv --with-mysqli --with-pdo-mysql --enable-ftp \
--enable-zip --with-curl --enable-soap --enable-xml \
--enable-json --with-openssl --enable-bcmath --enable-hash \
--enable-opcache --enable-opcache-file --with-tsrm-pthreads \
--enable-maintainer-zts --enable-exif --enable-intl \
--disable-phar"
CONFIG_PATHS="--prefix=${prefix} --with-mod-lsphp=$OLS_PATH"
fi

CONFIG="$CONFIG_PATHS $CONFIG_PARAMS"

OASAN=`cat $PHP_SRC_DIR/ASAN.sav`
OTSAN=`cat $PHP_SRC_DIR/TSAN.sav`
OCC=`cat $PHP_SRC_DIR/CC.sav`
OCXX=`cat $PHP_SRC_DIR/CXX.sav`


#defaults:
if [ "x$CC" = "x" ]
then
    echo "Using default C compiler (clang)"
    export CC=clang
fi

if [ "x$CXX" = "x" ]
then
    echo "Using default C++ compiler (clang)"
    export CXX=clang++
fi
if [ "x$OASAN" != "x$ASAN" ] || [ "x$OTSAN" != "x$TSAN" ] || [ "x$OCC" != "x$CC" ] || [ "x$OCXX" != "x$CXX" ]
then
    echo "Flags or compiler changed, reconfigure needed"
    NEED_CONFIGURE=1
fi

export CFLAGS='-g -O2 -fstack-protector-strong -Wformat -Werror=format-security -O2 -Wall -fsigned-char -fno-strict-aliasing -g'
export CXXFLAGS='-g -O2 -fstack-protector-strong -Wformat -Werror=format-security -O2 -Wall -fsigned-char -fno-strict-aliasing -g'

if [ "$TSAN" = "1" ]
then
    export CFLAGS="$CFLAGS -fsanitize=thread"
    export CXXFLAGS="$CXXFLAGS -fsanitize=thread"
    CONFIG="$CONFIG --enable-debug"
fi

if [ "$ASAN" = "1" ]
then
    export CFLAGS="$CFLAGS -fsanitize=address"
    export CXXFLAGS="$CXXFLAGS -fsanitize=address"
    CONFIG="$CONFIG --enable-debug"
fi

if [ ! -d $PHP_SRC_DIR ]; then
    if [ ! -f $PHPGZIP ]; then
        wget -O $PHPGZIP https://github.com/php/php-src/archive/$PHPVER.tar.gz
    fi
    NEED_CONFIGURE=1
    mkdir -p $PHP_SRC_DIR
    tar xvfz $PHPGZIP --directory $PHP_SRC_DIR --strip-components=1
    # alternatively:
    # tar xvfz $PHPGZIP --directory $LSDIR --strip-components=1
fi

rm -f $PHP_MAJ_SYMLINK
ln -sf $PHP_SRC_DIR $PHP_MAJ_SYMLINK
rm -f $PHP_MAJ_SYMLINK/sapi/mod_lsphp
rm -f $MOD_LSPHP_DIR/src/src
ln -sf $MOD_LSPHP_DIR/src $PHP_MAJ_SYMLINK/sapi/mod_lsphp
rm -f $MOD_LSPHP_DIR/$PHPMAJ
#ln -sf $PHP_MAJ_SYMLINK $MOD_LSPHP_DIR/$PHPMAJ 
cd $PHP_MAJ_SYMLINK

# this just doesn't work - if we enable the flags, the code crashes with invalid pointers
# as soon as opcache is enabled
# best we can do right now is enable debug
if [ "$TSAN" = "1" ]
then
    export CFLAGS="-fsanitize=thread"
    export CXXFLAGS="-fsanitize=thread"
    CONFIG="$CONFIG --enable-debug"
    patch -p1 < $MOD_LSPHP_DIR/php-7.2-opcache-patch.txt
fi

if [ "$ASAN" = "1" ]
then
    #export CFLAGS="-fsanitize=address"
    #export CXXFLAGS="-fsanitize=address"
    CONFIG="$CONFIG --enable-debug"
fi

if [ $NEED_CONFIGURE -eq 1 ]
then
    echo 'Reconfiguring...'
    #TODO - check if need buildconf --force here
    ./buildconf --force
    ./configure $CONFIG
    #TODO - check if need make clean here
    make clean
fi

export MAKEFLAGS="$MAKEFLAGS VERBOSE=1"
echo "Doing make..."
make
make install

echo $ASAN >  $PHP_SRC_DIR/ASAN.sav
echo $TSAN >  $PHP_SRC_DIR/TSAN.sav
echo $CC >  $PHP_SRC_DIR/CC.sav
echo $CXX >  $PHP_SRC_DIR/CXX.sav


cd $MOD_LSPHP_DIR
echo "Setting symlink..."
ln -sf $PHP_MAJ_SYMLINK/libs/libphp7.so mod_lsphp72.so
mkdir -p ${PHP_DBG_INST_DIR}/etc/php/7.2/litespeed/
echo MAKE SURE TO COPY / LINK YOUR php.ini to ${PHP_DBG_INST_DIR}/etc/php/7.2/litespeed/
