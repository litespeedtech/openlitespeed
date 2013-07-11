#! /bin/bash

svn update
rm UnitTest++.zip
find UnitTest++/ | grep -v .svn | zip UnitTest++.zip -9 -@
