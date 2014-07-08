#!/bin/sh

echo =====================================================================================

if [ $# -eq 0 ] ; then
  echo Need a c file name, such as $0 mymodule.c
  echo
  exit 1
fi

echo "Your command is $0 $1"
echo

if [ ! -f $1 ] ; then
  echo File $1 does not exist
  echo
  exit 1
fi

if [ "x$1" = "x./loopbuff.c" ] ; then
  echo "As I know loopbuff.c cannot be compiled to a module. Quit."
  echo
  exit 1
fi


TARGET=`basename $1 .c`
echo Target=$TARGET
echo




SYS_NAME=`uname -s`
if [ "x$SYS_NAME" = "xDarwin" ] ; then
	UNDEFINED_FLAG="-undefined dynamic_lookup"	
else
	UNDEFINED_FLAG=""
fi



gcc -g -Wall -fPIC -c -D_REENTRANT $(getconf LFS_CFLAGS)   $TARGET.c
if [ -f loopbuff.c ]; then
    gcc -g -Wall -fPIC -c -D_REENTRANT $(getconf LFS_CFLAGS) loopbuff.c
    gcc -g -Wall -fPIC $UNDEFINED_FLAG  $(getconf LFS_CFLAGS)  -o $TARGET.so $TARGET.o  loopbuff.o -shared
    rm loopbuff.o
else
    gcc -g -Wall -fPIC $UNDEFINED_FLAG $(getconf LFS_CFLAGS)  -o $TARGET.so $TARGET.o -shared
fi

if [ -f $(pwd)/$TARGET.so ] ; then
	echo -e "\033[38;5;71m$TARGET.so created.\033[39m"
else
    echo -e "\033[38;5;203mError, $TARGET.so does not exist.\033[39m"
fi

if [ -f $TARGET.o ] ; then 
  rm $TARGET.o
fi


echo Done!
echo
