#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([tapi_media],[0.1],[Ivan.Melnikov@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_media_file.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.9.6 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
