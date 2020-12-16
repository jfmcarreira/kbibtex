#!/usr/bin/env bash

categories=(Data Networking IO Config)

if [[ $# != 1 || $1 == "-h" || $1 == "--help" ]] ; then
	echo "Generate logging category headers and implementations."
	echo
	echo "One parameter is expected:"
	echo " 1. The absolute path to the the 'src/' directory inside"
	echo "    the build directory. If the directory does not yet"
	echo "    exist, it will be created."
	echo
	echo "Example:"
	echo "  ./generate-logging-categories.sh ~/git/build-bibsearch-SailfishOS_3_0_3_9_i486_in_Sailfish_OS_Build_Engine-Debug/src/"
	echo
	echo "The following logging category headers and implementations will be generated:"
	for category in "${categories[@]}" ; do echo "  ${category}" ; done
	exit 1
fi

destdir="$1"
test -d "${destdir}" || { echo "Creating directory: '${destdir}'" >&2 ; mkdir -p "${destdir}" ; }

for category in "${categories[@]}" ; do
	categoryUC="$(tr 'a-z' 'A-Z' <<<"${category}")"
	categoryLC="$(tr 'A-Z' 'a-z' <<<"${category}")"

	stem="${destdir}/${categoryLC}/logging_${categoryLC}"
	mkdir -p "$(dirname "${stem}")"
	cat <<EOF >"${stem}".h
// This file was generated by generate-logging-categories.sh: DO NOT EDIT!
#ifndef QLOGGINGCATEGORY_LOG_KBIBTEX_${categoryUC}_LOGGING_${categoryUC}_H
#define QLOGGINGCATEGORY_LOG_KBIBTEX_${categoryUC}_LOGGING_${categoryUC}_H
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(LOG_KBIBTEX_${categoryUC})
#endif // QLOGGINGCATEGORY_LOG_KBIBTEX_${categoryUC}_LOGGING_${categoryUC}_H
EOF
	cat <<EOF >"${stem}".cpp
// This file was generated by generate-logging-categories.sh: DO NOT EDIT!
#include "logging_${categoryLC}.h"
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
Q_LOGGING_CATEGORY(LOG_KBIBTEX_${categoryUC}, "kbibtex.${categoryLC}", QtInfoMsg)
#else
Q_LOGGING_CATEGORY(LOG_KBIBTEX_${categoryUC}, "kbibtex.${categoryLC}")
#endif
EOF
done
