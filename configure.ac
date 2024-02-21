m4_include(ax_check_zlib.m4)
m4_include(ax_check_openssl.m4)
m4_include(ax_path_lib_pcre.m4)
m4_include(ax_lib_expat.m4)
m4_include(ax_check_liblua.m4)
m4_include(ax_check_libudns.m4)
m4_include(ax_check_ip2location.m4)

dnl Process this file with autoconf to produce a configure script.
AC_INIT([openlitespeed],[1.8.0],[info@litespeedtech.com],[openlitespeed],[http://www.litespeedtech.com/])
AM_INIT_AUTOMAKE([1.0 foreign no-define subdir-objects])

AC_CONFIG_HEADERS(src/config.h:src/config.h.in)

dnl NOW change the default installation directory!
AC_PREFIX_DEFAULT('/usr/local/lsws')

# General "with" options
# ----------------------------------------------------------------------------
dnl Checks for programs.

AC_PROG_MAKE_SET
AC_PROG_INSTALL

# Checks for programs.
AC_PROG_AWK
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
LT_INIT

OSTYPE=`uname -m`
OSNAME=`uname -s`
TOPDIR=`pwd`
echo Current dir is $TOPDIR

# Checks for header files.
AC_CHECK_HEADER([pthread.h],
    [AC_CHECK_LIB([pthread],[pthread_join],[
        AC_DEFINE([HAVE_LIBPTHREAD],[],[Define if pthread (-lpthread)])
        AC_DEFINE([HAVE_PTHREAD_H],[],[Define if <pthread.h>])
       LIBS="-lpthread $LIBS"
    ])])
    
AC_CHECK_HEADERS(arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h)

AC_HEADER_ASSERT

## ##Set option --with-user
AC_ARG_WITH([user],
            AS_HELP_STRING([--with-user],[Set user name to run openlitespeed/lscpd [default: nobody]]),
            [OPENLSWS_USER="$withval"], [OPENLSWS_USER=nobody])
AC_DEFINE_UNQUOTED([OPENLSWS_USER], ["$OPENLSWS_USER"], [Defined the user name])
AC_SUBST([OPENLSWS_USER])

## ##Set option --with-group
AC_ARG_WITH([group],
            AS_HELP_STRING([--with-group],[Set group of the user [default: nobody]]),
            [OPENLSWS_GROUP="$withval"], [OPENLSWS_GROUP=nobody])
AC_DEFINE_UNQUOTED([OPENLSWS_GROUP], ["$OPENLSWS_GROUP"], [Defined the group of the user])
AC_SUBST([OPENLSWS_GROUP])

## ##Set option --with-admin
AC_ARG_WITH([admin],
            AS_HELP_STRING([--with-admin],[Set administrator user name [default: admin]]),
            [OPENLSWS_ADMIN="$withval"], [OPENLSWS_ADMIN=admin])
AC_SUBST([OPENLSWS_ADMIN])

## ##Set option --with-adminport
AC_ARG_WITH([adminport],
            AS_HELP_STRING([--with-adminport],[Set administrator listener port [default: 7080]]),
            [OPENLSWS_ADMINPORT="$withval"], [OPENLSWS_ADMINPORT=7080])
AC_SUBST([OPENLSWS_ADMINPORT])


## ##Set option --with-password
AC_ARG_WITH([password],
            AS_HELP_STRING([--with-password],[Set password of administrator [default: 123456]]),
            [OPENLSWS_PASSWORD="$withval"], [OPENLSWS_PASSWORD=123456])
AC_SUBST([OPENLSWS_PASSWORD])

## ##Set option --with-email
AC_ARG_WITH([email],
            AS_HELP_STRING([--with-email],[Set email of administrator [default: root@localhost]]),
            [OPENLSWS_EMAIL="$withval"], [OPENLSWS_EMAIL=root@localhost])
AC_SUBST([OPENLSWS_EMAIL])


AC_ARG_ENABLE([adminssl],
              [AS_HELP_STRING([--enable-adminssl=@<:@yes/no@:>@],
                              [Enable HTTPS for admin console (modify adminssl.conf before installation) @<:@default=yes@:>@])],
              [OPENLSWS_ADMINSSL="$enableval"], [OPENLSWS_ADMINSSL=yes])
AC_SUBST([OPENLSWS_ADMINSSL])

AC_ARG_WITH([exampleport],
            AS_HELP_STRING([--with-exampleport],[Set example listener port [default: 8088]]),
            [OPENLSWS_EXAMPLEPORT="$withval"], [OPENLSWS_EXAMPLEPORT=8088])
AC_SUBST([OPENLSWS_EXAMPLEPORT])




AC_ARG_WITH([lsphp7],
            AS_HELP_STRING([--with-lsphp7],[Set lsphp7 as default php script handler[default: yes]]),
            [USE_LSPHP7="$withval"], [USE_LSPHP7=yes])

AC_ARG_WITH([tempdir],
            AS_HELP_STRING([--with-tempdir],[Set a customized temprary directory[default: /tmp/lshttpd]]),
            [DEFAULT_TMP_DIR="$withval"], [DEFAULT_TMP_DIR=/tmp/lshttpd])

AC_ARG_WITH([pidfile],
            AS_HELP_STRING([--with-pidfile],[Set a customized pid file path name[default: $DEFAULT_TMP_DIR/lshttpd.pid]]),
            [PID_FILE="$withval"], [PID_FILE=$DEFAULT_TMP_DIR/lshttpd.pid])

AC_ARG_WITH([lscpd],
            AS_HELP_STRING([--with-lscpd],[Set to build lscpd instead of openlitespeed[default: yes]]),
            [IS_LSCPD="$withval"], [IS_LSCPD=no])
echo IS_LSCPD=$IS_LSCPD
if test "$IS_LSCPD" = "yes" ; then
    DEFAULT_TMP_DIR=/tmp/lscpd
    PID_FILE=$DEFAULT_TMP_DIR/lscpd.pid
    USE_LSPHP7=yes
    OPENLSWS_ADMINSSL=no
    AC_DEFINE_UNQUOTED([IS_LSCPD], [1], [Defined to build lscpd])
    AC_SUBST([IS_LSCPD])
fi

AM_CONDITIONAL([BUILD_LSCPD], [test x$IS_LSCPD = xyes])
AC_DEFINE_UNQUOTED([DEFAULT_TMP_DIR], ["$DEFAULT_TMP_DIR"], [Defined the temprary directory])
AC_SUBST([DEFAULT_TMP_DIR])
AC_DEFINE_UNQUOTED([PID_FILE], ["$PID_FILE"], [Defined the pid file path])
AC_SUBST([PID_FILE])
AC_SUBST([USE_LSPHP7])
AC_SUBST([OPENLSWS_ADMINSSL])
echo "OPENLSWS_ADMINSSL=$OPENLSWS_ADMINSSL"
echo USE_LSPHP7=$USE_LSPHP7            
echo DEFAULT_TMP_DIR=$DEFAULT_TMP_DIR
echo PID_FILE=$PID_FILE


OPENLSWS_HTTP2="?"
AC_ARG_ENABLE([spdy],
              [AS_HELP_STRING([--enable-spdy=@<:@yes/no@:>@],
                              [Enable SPDY and http2 over HTTPS @<:@default=yes@:>@])],
               [OPENLSWS_HTTP2="$enableval"], [])

AC_ARG_ENABLE([http2],
              [AS_HELP_STRING([--enable-http2=@<:@yes/no@:>@],
                              [Enable SPDY and http2 over HTTPS @<:@default=yes@:>@])],
               [OPENLSWS_HTTP2="$enableval"], [])
if test "$OPENLSWS_HTTP2" = "no" ; then 
    echo "SPDY/http2 disabled!!!"
else
    AC_DEFINE([LS_ENABLE_SPDY], [1], [Define if need spdy/http2 feature]) 
    echo "SPDY/http2 enabled!!!"
fi


#AC_ARG_ENABLE([cacheredis],
#              [AS_HELP_STRING([--enable-redis],
#                              [Enable redis for cache module (Using redis is disabled by default)])],
#               [ AC_DEFINE([LS_ENABLE_REDIS], [1], [Define if cache module needs redis feature]) 
#                 echo "Cache redis enabled!!!" ], [])

AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [Enable debugging symbols (Debug is disabled by default)])],
              [OPENLSWS_DEBUG="$enableval"
                if test "$OPENLSWS_DEBUG" = "yes" ; then 
                    CFLAGS="$CFLAGS -g3 -O0"
                    CXXFLAGS="$CXXFLAGS -g3 -O0"
                    AC_DEFINE([LS_ENABLE_DEBUG], [1], [Define if set to debug mode]) 
                else
                    CFLAGS="$CFLAGS -g -O2"
                    CXXFLAGS="$CXXFLAGS -g -O2"
                fi
                echo "OPENLSWS_DEBUG='$OPENLSWS_DEBUG'"
                ], [])
                
AC_ARG_ENABLE([profiling],
              [AS_HELP_STRING([--enable-profiling],
                              [Enable cpu profiling (profiling is disabled by default)])],
              [OPENLSWS_PROF="$enableval"
                if test "$OPENLSWS_PROF" = "yes" ; then 
                    CFLAGS="$CFLAGS -pg"
                    CXXFLAGS="$CXXFLAGS -pg"
                fi
                echo "OPENLSWS_PROF='$OPENLSWS_PROF'"
                ], [])                
                
dnl liblua check
AC_MSG_CHECKING(whether to compile mod_lua module)


AC_ARG_WITH(lua,
[  --with-lua[[=DIR]]        use liblua (located in directory DIR, if supplied) for compiling mod_lua module.  [[default=no]]],
[
    if test $withval = no
    then
        need_lua=no
    elif test $withval = yes
    then
        need_lua=yes
    else
        need_lua=yes
        lua_dir=$withval
    fi
],[
    need_lua=no
    lua_dir=
])
if test "x$need_lua" = "xno" ; then
    AC_MSG_RESULT(no)
else
    AC_MSG_RESULT(yes)
    AC_LIBLUA_CHECK
    if test "x$need_lua" = "xno" ; then
        AC_MSG_RESULT(liblua header files not found - disabling compile mod_lua module)
    fi
fi
AM_CONDITIONAL([HAVE_LIBLUA], [test x$need_lua = xyes])
echo "Lua inlcude = $LUA_INCLUDES, need_lua = $need_lua"    
                
CFLAGS="$CFLAGS -D_GLIBCXX_USE_CXX11_ABI=0 -fstack-protector $(getconf LFS_CFLAGS) "
CXXFLAGS="$CXXFLAGS -D_GLIBCXX_USE_CXX11_ABI=0 -fstack-protector $(getconf LFS_CFLAGS) "

LIBBROTLI=
OPENLSWS_BROTLI=no
AC_ARG_WITH(brotli,
            [  --with-brotli[[=DIR]]   Set to enable brotli compression [[default=no]]],
            [
                if test "x$withval" = "x"
                then
                    OPENLSWS_BROTLI=no
                elif test $withval = no
                then
                    OPENLSWS_BROTLI=no
                elif test $withval = yes
                then
                    echo "Brotli start to build ..."
                    BUILDBROTLI=`. $srcdir/dlbrotli.sh`
                    OPENLSWS_BROTLI="$TOPDIR/brotli-master"
                else
                    echo "Use defined brotli directory $withval."
                    OPENLSWS_BROTLI="$withval"
                fi
            ],[OPENLSWS_BROTLI=no])
            
if test "$OPENLSWS_BROTLI" = "no" ; then 
    echo "Brotli compression disabled!!!"
else
    LIBBROTLI=" $OPENLSWS_BROTLI/out/libbrotlidec-static.a $OPENLSWS_BROTLI/out/libbrotlienc-static.a $OPENLSWS_BROTLI/out/libbrotlicommon-static.a "
    AC_CHECK_FILES([$OPENLSWS_BROTLI/out/libbrotlidec-static.a $OPENLSWS_BROTLI/c/include/brotli/decode.h],,[LIBBROTLI=])
    if test "x$LIBBROTLI" = x ; then
        echo "Brotli files check failed. If continue, brotli compression will be disabled."
    else
        echo "Brotli checked and enabled!!!"
        CPPFLAGS="$CPPFLAGS -I$OPENLSWS_BROTLI/c/include "
        LIBBROTLI=" $OPENLSWS_BROTLI/out/libbrotlidec-static.a $OPENLSWS_BROTLI/out/libbrotlienc-static.a $OPENLSWS_BROTLI/out/libbrotlicommon-static.a "
        AC_DEFINE_UNQUOTED([USE_BROTLI], [1], [Defined to compile with Brotli compression included])
    fi
fi
echo "LIBBROTLI=$LIBBROTLI"
AC_SUBST([LIBBROTLI])


LIBMMDB=
OPENLSWS_IPTOGEO2=no
AC_ARG_WITH(iptogeo2,
            [  --with-iptogeo2[[=DIR]]   Set to enable iptogeo2, if need to build maxminddb which need to have autotool installed [[default=no]]],
            [
                if test "x$withval" = "x"
                then
                    OPENLSWS_IPTOGEO2=no
                elif test $withval = no
                then
                    OPENLSWS_IPTOGEO2=no
                elif test $withval = yes
                then
                    echo "libmaxminddb start to build ..."
                    BUILDMAXMINDDB=`. $srcdir/dlmaxminddb.sh`
                    OPENLSWS_IPTOGEO2="$TOPDIR/libmaxminddb"
                else
                    echo "Use defined iptogeo2 directory $withval."
                    OPENLSWS_IPTOGEO2="$withval"
                fi
            ],[OPENLSWS_IPTOGEO2=no])

if test "$OPENLSWS_IPTOGEO2" = "no" ; then 
    echo "iptogeo2 disabled!!!"
else
    LIBMMDBEXIST=
    AC_CHECK_FILE([$OPENLSWS_IPTOGEO2/src/.libs/libmaxminddb.a],[LIBMMDBEXIST=yes],[LIBMMDBEXIST=no])
    if test "$LIBMMDBEXIST" = no ; then
        echo It seems libmaxminddb library not eixts, please check and ty again.
        echo Or you can continue to make and iptogeo2 will be disabled.
        echo
    else
        LIBMMDB=" $OPENLSWS_IPTOGEO2/src/.libs/libmaxminddb.a "
        CPPFLAGS="$CPPFLAGS -I$OPENLSWS_IPTOGEO2/include/ "
        AC_DEFINE_UNQUOTED([ENABLE_IPTOGEO2], [1], [Defined to compile with iptogeo2 enabled])
    fi
fi
echo "LIBMMDB=$LIBMMDB"
AC_SUBST([LIBMMDB])


## ##Set option --with-bssl
AC_ARG_WITH(bssl,
            [  --with-bssl[[=DIR]]       Set to use BoringSSL instead of build while configure [[default=boringssl]]],
            [ OPENLSWS_BSSL="$withval" ],[OPENLSWS_BSSL=boringssl])
AC_SUBST([OPENLSWS_BSSL])



CPPFLAGS=" -I$srcdir/ssl/include $CPPFLAGS -I../../src -I../../include -I/usr/local/include -I/usr/local -I/usr -I/usr/pkg -DUSE_BORINGSSL=1 "
BSSL=`. $srcdir/dlbssl.sh "$OPENLSWS_BSSL"`
echo "BoringSSL files copied to $srcdir/ssl/ from $OPENLSWS_BSSL."

AC_DEFINE_UNQUOTED([USE_BORINGSSL], [1], [Defined to use BoringSSL instead of OpenSSL])
AC_DEFINE_UNQUOTED([USE_DYN_OSSL], [0], [Defined to use openssl])
AC_DEFINE_UNQUOTED([USE_BSSL], [1], [Defined to use boringSSL])


AC_ARG_ENABLE([recaptcha],
              [AS_HELP_STRING([--enable-recaptcha=@<:@yes/no@:>@],
                              [Enable recaptcha @<:@default=yes@:>@])],
               [OPENLSWS_RECAPTCHA="$enableval"], [])
if test "$OPENLSWS_RECAPTCHA" = "no" ; then 
    echo "Recaptcha disabled!!!"
else
    echo "Recaptcha enabled, will compile it..."
    LSRECAPTCHA=`. $srcdir/src/modules/lsrecaptcha/build_lsrecaptcha.sh`
    echo $LSRECAPTCHA
fi

AC_ARG_WITH(ip2loc,
[  --with-ip2loc[[=DIR]]        enable ip2location (located in directory DIR, if supplied).  [[default=no]]],
[
    if test "x$withval" = "x" ; then
        need_ip2location=no
    elif test $withval = no ; then
        need_ip2location=no
    elif test $withval = yes ; then
        need_ip2location=yes
    else
        need_ip2location=yes
        ip2location_dir=$withval
    fi
],[
    need_ip2location=no
    ip2location_dir=
])
AC_SUBST([need_ip2location])
AC_SUBST([ip2location_dir])

AC_MSG_CHECKING(whether to check if ip2location library exists)
if test "x$need_ip2location" = "xno" ; then
    AC_MSG_RESULT(no)
else
    AC_MSG_RESULT(yes)
    AC_DEFINE_UNQUOTED([USE_IP2LOCATION], [1], [Defined to compile with IP2Location included])
    AC_IP2LOCATION_CHECK
    if test "x$need_ip2location" = "xno" ; then
        AC_MSG_RESULT(ip2location header files not found - disabling ip2location)
    elif test "x$need_ip2location" = "xcheck" ; then
        need_ip2location=yes
        AC_MSG_RESULT(ip2location header files found - enable ip2location)
    fi
fi
AM_CONDITIONAL([HAVE_IP2LOCATION], [test x$need_ip2location = xyes])
echo "IP2Location include = $IP2LOCATION_INCLUDES, need_ip2location = $need_ip2location"

AC_ARG_ENABLE([rpath],
              [AS_HELP_STRING([--disable-rpath], 
                              [Disable rpath (It is 'no' by default)])],
              [OPENLSWS_DISABLE_RPATH=yes              
               echo "OPENLSWS_DISABLE_RPATH=yes"], [OPENLSWS_DISABLE_RPATH=no])                
AC_SUBST([OPENLSWS_DISABLE_RPATH])
               
AC_ARG_WITH([libdir],
            AS_HELP_STRING([--with-libdir],[Set system lib directory. It is lib or lib64 and will be automatically checked by default]),
            [OPENLSWS_LIBDIR="$withval"], [

                OPENLSWS_LIBDIR=lib
                if test "$OSNAME" = Linux ; then
                    if test "$OSTYPE" = x86_64 ; then
                        OPENLSWS_LIBDIR=lib64
                    elif test "$OSTYPE" = i686 ; then
               
                        CFLAGS="$CFLAGS -march=i686"
                        CXXFLAGS="$CXXFLAGS -march=i686"
                        elif test "$OSTYPE" = armv7l ; then
                       # Raspberry Pi 2, Scaleway C1, Parallella
                        CFLAGS="$CFLAGS -march=armv7"
                        CXXFLAGS="$CXXFLAGS -march=armv7"
                    elif test "$OSTYPE" = armv6l ; then
                       # Raspberry Pi (not Pi 2)
                        CFLAGS="$CFLAGS -march=armv6 -mfloat-abi=hard -mfpu=vfp"
                        CXXFLAGS="$CXXFLAGS -march=armv6 -mfloat-abi=hard -mfpu=vfp"
                        
                    fi
                fi
             ])
echo "Final CFLAGS='$CFLAGS' CXXFLAGS='$CXXFLAGS' CPPFLAGS='$CPPFLAGS'"
             
echo "Default system lib directory = $OPENLSWS_LIBDIR"
AC_SUBST([OPENLSWS_LIBDIR])


AX_CHECK_ZLIB(, AC_MSG_ERROR(Can not find zlib. You must install it before continuing.))


if test "$OSNAME" = Darwin ; then
    AX_CHECK_OPENSSL(, AC_MSG_ERROR(Can not find openssl. You must install it before continuing.))
fi

AX_PATH_LIB_PCRE(, AC_MSG_ERROR(Can not find pcre. You must install it before continuing.))
AX_PATH_LIB_UDNS(, AC_MSG_ERROR(Can not find udns library. You must install it before continuing.))

AX_LIB_EXPAT(0.5)
AC_CHECK_LIB(crypt,crypt,LIBS="-lcrypt $LIBS")
      
      
# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_CHECK_TYPES(ptrdiff_t)

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
#AC_FUNC_MALLOC disable due to this replacing malloc() with non-existing rpl_malloc() issue on some systems
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS(malloc bzero dup2 ftruncate getcwd gethostbyaddr gethostbyname gettimeofday inet_ntoa localtime_r memchr memmove memset mkdir munmap select socket strcasecmp strchr strcspn strdup strerror strncasecmp strpbrk strrchr strspn strstr strtol)

RT_LIB_OPTION=" -lrt" 
DL_LIB_OPTION=" -ldl"
if test "$OSNAME" = Linux ; then
    LDFLAGS="$LDFLAGS -Wl,--export-dynamic"
    AC_DEFINE_UNQUOTED([IOURING], [1], [Defined to compile with IOURING])
    AC_DEFINE_UNQUOTED([LS_AIO_USE_LINUX_AIO], [1], [Defined to compile with LS_AIO_USE_LINUX_AIO flag])
    
fi

if test "$OSNAME" = FreeBSD ; then
    LDFLAGS="$LDFLAGS -Wl,--export-dynamic"
    DL_LIB_OPTION=
fi

#For Mac OS, add special falgs for LuaJit
if test "$OSNAME" = Darwin ; then
    LDFLAGS="$LDFLAGS -Wl,-export_dynamic -pagezero_size 10000 -image_base 100000000"
    RT_LIB_OPTION=
fi

if test "$OSTYPE" = aarch64 ; then
    EXTRA_LIBS="$EXTRA_LIBS -latomic"
fi

LDFLAGS="$LDFLAGS $PCRE_LDFLAGS $UDNS_LDFLAGS -D_GLIBCXX_USE_CXX11_ABI=0"
echo "Final LDFLAGS='$LDFLAGS'"
AC_SUBST([DL_LIB_OPTION])
AC_SUBST([RT_LIB_OPTION])

MODULE_VERSION_INFO=`. $srcdir/modverinfo.sh`
AC_DEFINE_UNQUOTED([LS_MODULE_VERSION_INFO], ["$MODULE_VERSION_INFO"], [The pre-defined modules version info])

AC_CONFIG_FILES(Makefile
                support/Makefile
                support/unmount_ns/Makefile
                src/Makefile
                src/edio/Makefile
                src/extensions/Makefile
                src/http/Makefile
                src/spdy/Makefile
                src/h2/Makefile
                src/log4cxx/Makefile
                src/main/Makefile
                src/socket/Makefile
                src/sslpp/Makefile
                src/ssi/Makefile
                src/lsiapi/Makefile
                src/modules/Makefile
                src/shm/Makefile
                src/adns/Makefile
                src/quic/Makefile
                src/liblsquic/Makefile
                src/modules/modinspector/Makefile
                src/modules/modreqparser/Makefile
                src/modules/uploadprogress/Makefile)
                
if test x$need_lua = xyes ; then
    AC_CONFIG_FILES(src/modules/lua/Makefile)
    echo "Will compile and build mod_lua module."
else
    echo "Will NOT compile and build mod_lua module."
fi

AC_OUTPUT

