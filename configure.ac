m4_include(ax_check_zlib.m4)
m4_include(ax_check_openssl.m4)
m4_include(ax_path_lib_pcre.m4)
m4_include(ax_lib_expat.m4)

dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([openlitespeed],[1.2.11],[info@litespeedtech.com],[openlitespeed],[http://www.litespeedtech.com/])
AM_INIT_AUTOMAKE([1.0 foreign no-define ])

AM_CONFIG_HEADER(src/config.h:src/config.h.in)

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
            AS_HELP_STRING([--with-user],[Set user name to run openlitespeed [default: nobody]]),
            [OPENLSWS_USER="$withval"], [OPENLSWS_USER=nobody])
AC_SUBST([OPENLSWS_USER])

## ##Set option --with-group
AC_ARG_WITH([group],
            AS_HELP_STRING([--with-group],[Set group of the user [default: nobody]]),
            [OPENLSWS_GROUP="$withval"], [OPENLSWS_GROUP=nobody])
AC_SUBST([OPENLSWS_GROUP])

## ##Set option --with-admin
AC_ARG_WITH([admin],
            AS_HELP_STRING([--with-admin],[Set administrator user name [default: admin]]),
            [OPENLSWS_ADMIN="$withval"], [OPENLSWS_ADMIN=admin])
AC_SUBST([OPENLSWS_ADMIN])

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
              [AC_HELP_STRING([--enable-adminssl=@<:@yes/no@:>@],
                              [Enable HTTPS for admin console (modify adminssl.conf before installation) @<:@default=yes@:>@])],
              [OPENLSWS_ADMINSSL="$enableval"], [OPENLSWS_ADMINSSL=yes])
AC_SUBST([OPENLSWS_ADMINSSL])

AC_ARG_ENABLE([spdy],
              [AC_HELP_STRING([--enable-spdy],
                              [Enable SPDY over HTTPS (Spdy is disabled by default)])],
               [ AC_DEFINE([LS_ENABLE_SPDY], [1], [Define if need spdy feature]) 
                 echo "SPDY enabled!!!" ], [])

AC_ARG_ENABLE([debug],
              [AC_HELP_STRING([--enable-debug],
                              [Enable debugging symbols (Debug is disabled by default)])],
              [OPENLSWS_DEBUG="$enableval"
                if test "$OPENLSWS_DEBUG" = "yes" ; then 
                    CFLAGS="-g3 -O0"
                    CXXFLAGS="-g3 -O0"
                else
                    CFLAGS="-g -O3"
                    CXXFLAGS="-g -O3"
                fi
                echo "OPENLSWS_DEBUG='$OPENLSWS_DEBUG'"
                ], [])

                
AC_ARG_ENABLE([profiling],
              [AC_HELP_STRING([--enable-profiling],
                              [Enable cpu profiling (profiling is disabled by default)])],
              [OPENLSWS_PROF="$enableval"
                if test "$OPENLSWS_PROF" = "yes" ; then 
                    CFLAGS="$CFLAGS -pg"
                    CXXFLAGS="$CXXFLAGS -pg"
                fi
                echo "OPENLSWS_PROF='$OPENLSWS_PROF'"
                ], [])                
                
CFLAGS="$CFLAGS -fstack-protector `getconf LFS_CFLAGS`"
CXXFLAGS="$CXXFLAGS -fstack-protector `getconf LFS_CFLAGS`"
echo "Final CFLAGS='$CFLAGS' CXXFLAGS='$CXXFLAGS'"


AC_ARG_ENABLE([rpath],
              [AC_HELP_STRING([--disable-rpath], 
                              [Disable rpath (It is 'no' by default)])],
              [OPENLSWS_DISABLE_RPATH=yes              
               echo "OPENLSWS_DISABLE_RPATH=yes"], [OPENLSWS_DISABLE_RPATH=no])                
AC_SUBST([OPENLSWS_DISABLE_RPATH])
               
AC_ARG_WITH([libdir],
            AS_HELP_STRING([--with-libdir],[Set system lib directory. It is lib or lib64 and will be automatically checked by default]),
            [OPENLSWS_LIBDIR="$withval"], [
                OSTYPE=`uname -m`
                OSNAME=`uname -s`
                if test "$OSNAME" != Linux ; then
                    OPENLSWS_LIBDIR=lib
                else
                    if test "$OSTYPE" != x86_64 ; then
                        OPENLSWS_LIBDIR=lib
                    else
                        OPENLSWS_LIBDIR=lib64
                    fi
                fi
             ])
echo "Default system lib directory = $OPENLSWS_LIBDIR"
AC_SUBST([OPENLSWS_LIBDIR])


AX_CHECK_ZLIB(, AC_MSG_ERROR(Can not find zlib. You must install it before continuing.))
AX_CHECK_OPENSSL(, AC_MSG_ERROR(Can not find openssl. You must install it before continuing.))
AX_PATH_LIB_PCRE(, AC_MSG_ERROR(Can not find pcre. You must install it before continuing.))
AX_LIB_EXPAT(0.5)
AC_CHECK_LIB([GeoIP], [GeoIP_id_by_addr])
AC_CHECK_LIB(crypt,crypt,LIBS="-lcrypt $LIBS")
      
      
# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
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

AC_CONFIG_FILES(Makefile
                src/Makefile
                src/edio/Makefile
                src/extensions/Makefile
                src/http/Makefile
                src/spdy/Makefile
                src/log4cxx/Makefile
                src/main/Makefile
                src/socket/Makefile
                src/sslpp/Makefile
                src/ssi/Makefile
                src/util/Makefile)

AC_OUTPUT

