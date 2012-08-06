#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([logger_ten], [0.03], [Igor.Vasiliev@oktetlabs.ru])

AC_CONFIG_SRCDIR([logger_ten.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AC_C_INLINE

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

TE_LIB_SET

AC_CONFIG_HEADER(package.h)

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
