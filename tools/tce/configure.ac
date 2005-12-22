#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([tce], [1.0], [Artem.Andreev@oktetlabs.ru])
AC_CONFIG_SRCDIR([tce_gcc])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

TE_APP_SET

AC_CONFIG_HEADERS([config.h])

AC_HEADER_STDC
AC_CHECK_HEADERS([\
assert.h \
unistd.h \
errno.h \
strings.h \
sys/types.h \
])


AC_CONFIG_FILES([Makefile])

TE_APP_RESTORE

AC_OUTPUT
