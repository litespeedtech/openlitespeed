#!/bin/sh

# convertxml.sh -- convert webadmin configuration between XML and plain conf.
#
# Ships in the OpenLiteSpeed-family webadmin package and is copied to
# $SERVER_ROOT/admin/misc/ at install time, next to convertxml.php.
#
# Usage:
#   ./convertxml.sh <SERVER_ROOT>          convert XML -> conf (default)
#   ./convertxml.sh <SERVER_ROOT> 2conf    convert XML -> conf
#   ./convertxml.sh <SERVER_ROOT> 2xml     convert conf -> XML

CUR_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

if [ "x$1" = "x" ] ; then
    echo "Need server_root as parameter, such as /usr/local/lsws"
    echo "Usage: $0 <SERVER_ROOT> [2conf|2xml]"
    exit 1
fi

SERVER_ROOT=$1
MODE=${2:-2conf}

case "${MODE}" in
    2xml)
        echo "Converting plain conf configuration files to XML format ..."
        ;;
    2conf)
        echo "Converting XML format configuration files to plain conf ..."
        ;;
    *)
        echo "Unknown mode '${MODE}'. Use 2conf (XML -> conf) or 2xml (conf -> XML)."
        exit 1
        ;;
esac

echo "Server_root is ${SERVER_ROOT}, converting ..."

# Prefer the packaged admin PHP CLI; fall back to a system php.
ADMIN_PHP="${SERVER_ROOT}/admin/fcgi-bin/admin_php"
if [ -x "${ADMIN_PHP}" ] ; then
    PHP_BIN="${ADMIN_PHP}"
elif command -v php >/dev/null 2>&1 ; then
    PHP_BIN=php
else
    echo "ERROR: no PHP CLI found (looked for ${ADMIN_PHP} and php on PATH)."
    exit 1
fi

if [ "${MODE}" = "2conf" ] ; then
    "${PHP_BIN}" "${CUR_DIR}/convertxml.php" "${SERVER_ROOT}" 2conf "${SERVER_ROOT}/backup/recover_xml.sh"
else
    "${PHP_BIN}" "${CUR_DIR}/convertxml.php" "${SERVER_ROOT}" 2xml
fi

STATUS=$?
if [ ${STATUS} -ne 0 ] ; then
    echo "Conversion failed (exit ${STATUS})."
    exit ${STATUS}
fi

echo "Converting finished."
