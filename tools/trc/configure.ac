#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([trc], [0.4], [Andrew.Rybchenko@oktetlabs.ru])
AC_CONFIG_SRCDIR([te_trc.sh])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

TE_APP_SET

# Checks for libraries
TE_PATH_LIBXML2

AC_CONFIG_HEADERS([config.h])

CPPFLAGS=${XML2_CFLAGS}
AC_CHECK_TYPES([xmlError], [], [], [#include <libxml/xmlerror.h>])

AC_CONFIG_FILES([Makefile])

TE_APP_RESTORE

AC_OUTPUT
