#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([te-lib-trc],[0.1],[Andrew.Rybchenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([te_trc.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

# Checks for libraries
TE_PATH_LIBXML2

AC_CONFIG_HEADERS([package.h])

CPPFLAGS=${CPPFLAGS}" "${XML2_CFLAGS}
AC_CHECK_TYPES([xmlError], [], [], [#include <libxml/xmlerror.h>])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
