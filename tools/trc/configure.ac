#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([trc], [0.1], [Andrew.Rybchenko@oktetlabs.ru])
AC_CONFIG_SRCDIR([te_trc.sh])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL


# Checks for libraries
LIBXML_CONFIG_PREFIX=""
AC_ARG_WITH([libxml-prefix],
    [--with-libxml-prefix=[PFX] \
     Specify location of libxml config (not including bin directory)],
    LIBXML_CONFIG_PREFIX=$withval)
if test "x$LIBXML_CONFIG_PREFIX" != "x" ; then
    XML_CONFIG=${LIBXML_CONFIG_PREFIX}/bin/xml2-config
else
    XML_CONFIG=xml2-config
fi

AC_MSG_CHECKING(for libxml2 library)
if ${XML_CONFIG} --libs print >/dev/null 2>&1
then
    CFLAGS=${CFLAGS}" "`$XML_CONFIG --cflags`
    LIBS=${LIBS}" "`$XML_CONFIG --libs`
    AC_MSG_RESULT(found)
else
    AC_MSG_ERROR([Could not find libxml2, check ftp://xmlsoft.org/.])
fi


AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

CFLAGS=${CFLAGS}" "${TE_CFLAGS}


AC_CONFIG_HEADERS([config.h])

AC_HEADER_STDC
AC_CHECK_HEADERS([\
unistd.h \
errno.h \
strings.h \
sys/types.h \
sys/queue.h \
popt.h \
])

AC_CHECK_TYPES([xmlError], [], [], [#include <libxml/xmlerror.h>])


AC_CONFIG_FILES([Makefile])

AC_OUTPUT
