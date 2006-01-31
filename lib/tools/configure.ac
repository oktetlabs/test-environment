#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([te-tools], [0.1], [Andrew.Rybchenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([te_tools.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AM_CONDITIONAL(WINDOWS, test x$host != x${host%windows})

AC_SUBST(host)

TE_LIB_SET

AC_CONFIG_HEADERS([package.h])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
