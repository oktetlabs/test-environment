#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([te_ipc],[0.2],[Andrew.Rybchenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([ipc_internal.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

AC_CONFIG_HEADER(package.h)

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
