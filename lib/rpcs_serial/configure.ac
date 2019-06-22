# Process this file with autoconf to produce a configure script.
AC_INIT([rpcs_serial],[0.1],[Ivan.Danilov@oktetlabs.ru])

AC_CONFIG_SRCDIR([serial.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.9.6 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
