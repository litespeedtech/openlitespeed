#!/bin/ksh

ADMINS=jim.tilander@gmail.com
PROJECTNAME=UnitTest++
SUBJECT=
REPORT=/tmp/`basename $0`.report.$$
LOCK=/tmp/`basename $0`.$PROJECTNAME.lock
ERROR=/tmp/`basename $0`.$PROJECTNAME.error
PATH=/bin:/sbin:/usr/bin:/usr/sbin:$HOME/local/bin

exec > $REPORT 2>&1

if [ -e $LOCK ]; then
	exit 1
fi

touch $LOCK

cd `dirname $0`/..

svn update

cd UnitTest++
make clean all

if [ $? != "0" ]; then
	SUBJECT="$PROJECTNAME Failed building"
	if [ -e $ERROR ]; then
		SUBJECT=
	fi
	touch $ERROR
elif [ -e $ERROR ]; then
	rm $ERROR
	SUBJECT="$PROJECTNAME build fixed"
fi

if [ "$SUBJECT" ]; then
	cat $REPORT | mailx -E -s "$SUBJECT" $ADMINS
fi



rm $REPORT
rm $LOCK
