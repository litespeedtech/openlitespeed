#
# This script is to download PSOL and extract it to right location
#
#



cd `dirname "$0"`

if [ ! -f psol/include/out/Release/obj/gen/net/instaweb/public/version.h ] ; then

    DL=`which curl`
    DLCMD="$DL -O "
    TARGET=1.9.32.6.tar.gz
	$DLCMD https://dl.google.com/dl/page-speed/psol/$TARGET  
	tar -xzvf $TARGET # expands to psol/
	rm $TARGET

	#make symbolic links for code browsing
	cd `dirname "$0"`
	ln -sf psol/include/net/ net
	ln -sf psol/include/out/ out
	ln -sf psol/include/pagespeed/ pagespeed
	ln -sf psol/include/third_party/ third_party
  	ln -sf psol/include/third_party/chromium/src/base/ base
	ln -sf psol/include/third_party/chromium/src/build/ build
	ln -sf psol/include/third_party/css_parser/src/strings strings

fi









