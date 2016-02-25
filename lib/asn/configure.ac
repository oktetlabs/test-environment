
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([asn],[0.90],[konst@oktetlabs.ru])

AC_CONFIG_SRCDIR([asn_val.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

# We use APP here as we need to build tests
TE_APP_SET

AC_CONFIG_HEADER(package.h)

AC_CONFIG_FILES([Makefile tests/Makefile])

TE_APP_RESTORE

AC_OUTPUT
