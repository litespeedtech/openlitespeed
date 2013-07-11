
SUBDIRS = src
AUTOMAKE_OPTIONS = foreign

install-data-hook:
	( cd dist; ./install.sh "$(prefix)" "$(OPENLSWS_USER)" "$(OPENLSWS_GROUP)" "$(OPENLSWS_ADMIN)" "$(OPENLSWS_PASSWORD)" "$(OPENLSWS_EMAIL)" "$(OPENLSWS_ADMINSSL)";)

ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
