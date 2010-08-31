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
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_RANLIB
AC_PROG_INSTALL

TE_APP_SET

# Checks for libraries
TE_PATH_LIBXML2

CPPFLAGS=${XML2_CFLAGS}

# Check 'with' options
AC_ARG_WITH([spoilers],, 
[if test "${withval}" = yes; then
    with_spoilers=yes;
 fi
], [with_spoilers=no])

AC_ARG_WITH([popups],, 
[if test "${withval}" = yes; then
    with_popups=yes;
 fi
], [with_popups=no])

AC_ARG_WITH([log_urls],, 
[if test "${withval}" = yes; then
    with_log_urls=yes;
 fi
], [with_log_urls=no])

AM_CONDITIONAL(WITH_SPOILERS,     test "$with_spoilers" = "yes")
AM_CONDITIONAL(WITH_POPUPS,       test "$with_popups" = "yes")
AM_CONDITIONAL(WITH_LOG_URLS,     test "$with_log_urls" = "yes")

if test "$with_spoilers" = "yes"; then
  TE_CFLAGS="$TE_CFLAGS -DWITH_SPOILERS"
fi

if test "$with_popups" = "yes"; then
  TE_CFLAGS="$TE_CFLAGS -DWITH_POPUPS"
fi

if test "$with_log_urls" = "yes"; then
  TE_CFLAGS="$TE_CFLAGS -DWITH_LOG_URLS"
fi

AC_SUBST([TE_CFLAGS])

AC_ARG_VAR([TE_CFLAGS])

AC_CONFIG_HEADERS([config.h])

AC_CHECK_TYPES([xmlError], [], [], [#include <libxml/xmlerror.h>])
AC_CHECK_MEMBER([struct _xmlSAXHandler._private], [], [], 
                [#include <libxml/parser.h>])
AC_CHECK_MEMBER([struct _xmlSAXHandler.startElementNs], [], [], 
                [#include <libxml/parser.h>])
AC_CHECK_MEMBER([struct _xmlSAXHandler.endElementNs], [], [], 
                [#include <libxml/parser.h>])
AC_CHECK_MEMBER([struct _xmlSAXHandler.serror], [], [], 
                [#include <libxml/parser.h>])

AC_CONFIG_FILES([
Makefile
lib/Makefile
trc-log/Makefile
trc-report/Makefile
trc-diff/Makefile
trc-key/Makefile
])

TE_APP_RESTORE

AC_OUTPUT
