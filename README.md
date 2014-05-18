OpenLiteSpeed Web Server
========

Description
--------

OpenLiteSpeed is a high-performance, lightweight, open source HTTP server developed and copyrighted by 
LiteSpeed Technologies. Users are free to download, use, distribute, and modify OpenLiteSpeed and its 
source code in accordance with the precepts of the GPLv3 license.

This is the official repository for OpenLiteSpeed's source code. It is maintained byLiteSpeed 
Technologies.

Fork specific changes
--------
- It is now possible to change the directory that is used for cache data. This is done by using the module setting *storagePath* in the module parameters list. This only works on server level, any settings on virtual host level will be ignored.

Note: There is a possible bug with the cache module in the upstream. See more [here](https://groups.google.com/forum/#!topic/openlitespeed-development/8hCiIDd-0Ek).

Documentation
--------

Users can find all OpenLiteSpeed documentation on the [OpenLiteSpeed site](http://open.litespeedtech.com), 
but here are some quick links to important parts of the site:

[Installation](http://open.litespeedtech.com/mediawiki/index.php/Help:Installation)

[Configuration](http://open.litespeedtech.com/mediawiki/index.php/Help:Configuration)

[Road map](http://open.litespeedtech.com/mediawiki/index.php/Road_Map)

[Release log](http://open.litespeedtech.com/mediawiki/index.php/Release_Log/1.x)

Get in Touch
--------

OpenLiteSpeed has a [Google Group](https://groups.google.com/forum/#!forum/openlitespeed-development). If 
you find a bug, want to request new features, or just want to talk about OpenLiteSpeed, this is the place
to do it.
