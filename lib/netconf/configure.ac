#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([netconf],[0.90],[Maxim.Alyutov@oktetlabs.ru])

AC_CONFIG_SRCDIR([netconf.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

AC_CONFIG_HEADER(package.h)

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
