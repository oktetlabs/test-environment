#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([rpc], [1.0], [Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([rpc.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_PROG_CC
AC_PROG_RANLIB

AC_CONFIG_FILES([Makefile])

AM_CONDITIONAL(WINDOWS, test x$host != x${host%windows})

AC_SUBST(host)

AC_OUTPUT
