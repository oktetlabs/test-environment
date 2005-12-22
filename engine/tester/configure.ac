#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([te_tester], [0.6], [Andrew.Rybchenko@oktetlabs.ru])
AC_CONFIG_SRCDIR([tester.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL

TE_APP_SET

# Checks for libraries
TE_PATH_LIBXML2

AC_CONFIG_HEADERS([config.h])

CPPFLAGS=${XML2_CFLAGS}
AC_CHECK_TYPES([xmlError], [], [], [#include <libxml/xmlerror.h>])

AC_CHECK_LIB([curses], [start_color],
             [LIBS="-lcurses ${LIBS}" 
              AC_DEFINE(HAVE_COLOR, 1,
                        [ Define if your curses library supports color. ])],
             [AC_MSG_WARN([Color terminal is not supported])])


AC_CONFIG_FILES([Makefile])

TE_APP_RESTORE

AC_OUTPUT
