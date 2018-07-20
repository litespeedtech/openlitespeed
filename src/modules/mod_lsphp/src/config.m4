dnl
dnl To rebuild, for Bob with his project in /home/user/proj/openlitespeed
dnl    cd /home/user/proj/openlitespeed/src/modules/mod_lsphp/php-7.0 (yes, there are links in there)
dnl    ./buildconf --force
dnl    ./configure --disable-all --with-mod-lsphp=/home/user/proj/openlitespeed/
dnl To rebuild for full functionality, use the script ../configure_mod_lsphp.sh
dnl    make
dnl Good luck!
dnl Note that once you've setup the above, you can build in KDevelop.
dnl

PHP_ARG_WITH(mod-lsphp,,
[  --with-mod-lsphp[=FILE]   Build shared LiteSpeed Handler module. FILE is the optional
                          pathname to the litespeed module develop tool [lsr]], no, no)

AC_MSG_CHECKING([for Litespeed handler-module support via DSO through LSR])

if test "$PHP_MOD_LSPHP" != "no"; then
  if test "$PHP_MOD_LSPHP" = "yes"; then
    LSR_PATH=/usr/local/lsws
  else
    PHP_EXPAND_PATH($PHP_MOD_LSPHP, LSR_PATH)
  fi
  LITESPEED_CFLAGS="-I$LSR_PATH/include -fPIC -shared -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1"
dnl  LITESPEED_CFLAGS="-I$LSR_PATH/include -fPIC -shared"
  PHP_SELECT_SAPI(mod_lsphp, shared, mod_lsphp.c, $LITESPEED_CFLAGS) 
  #INSTALL_IT="$INSTALL_IT $SAPI_LIBTOOL"

  PHP_BUILD_THREAD_SAFE
dnl 
dnl You must use the 'visibility' macro in your code to get symbols exported.  See mod_lsphp
dnl The code below does NOT work (but it does add stuff to your links which is why it's here):
dnl  PHP_LDFLAGS="$PHP_LDFLAGS -fPIC -shared -export-dynamic"
dnl
  AC_MSG_RESULT(yes)
  PHP_SUBST(APXS)
else
  AC_MSG_RESULT(no)
fi

dnl ## Local Variables:
dnl ## tab-width: 4
dnl ## End:
