#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([iscsi_unh_target], [0.01], [Artem.Andreev@oktetlabs.ru])

AC_CONFIG_SRCDIR([main.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

TE_APP_SET

AC_CONFIG_HEADER(package.h)

AC_CONFIG_FILES([\
Makefile \
common/Makefile \
security/Makefile \
target/Makefile \
])

TE_APP_RESTORE

AC_OUTPUT
