
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT([tapi_ipstack], [0.02], [konst@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_ip.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

TE_LIB_SET

AC_CONFIG_HEADERS([package.h])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
