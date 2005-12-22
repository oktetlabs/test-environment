
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([tapi_pcap], [0.01], [konst@oktetlabs.ru], [kam@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_pcap.c])

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
