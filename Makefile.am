
SUBDIRS = src
AUTOMAKE_OPTIONS = foreign

install-data-hook:
	( cd dist; ./install.sh "$(DESTDIR)$(prefix)" "$(OPENLSWS_USER)" "$(OPENLSWS_GROUP)" "$(OPENLSWS_ADMIN)" "$(OPENLSWS_PASSWORD)" "$(OPENLSWS_EMAIL)" "$(OPENLSWS_ADMINSSL)" "$(OPENLSWS_ADMINPORT)" ;)
	( rm -f $(DESTDIR)$(prefix)/modules/cache.la  $(DESTDIR)$(prefix)/modules/modinspector.la $(DESTDIR)$(prefix)/modules/uploadprogress.la  $(DESTDIR)$(prefix)/modules/mod_lua.la )
	
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
