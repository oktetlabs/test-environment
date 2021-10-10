# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rpcs_job], [0.1], [Igor.Romanov@oktetlabs.ru])

AC_CONFIG_SRCDIR([job.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.11.1 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_RANLIB
AC_PROG_INSTALL

RANLIB=touch

AC_SUBST([TE_BASE])
AC_ARG_VAR([TE_BASE])

TE_LIB_SET

dnl Used here tarpc.h includes rpc/rpc.h which requires extra -I option
dnl in the case of libtirpc. There is no easy way to inherit it from
dnl rpcxdr findings so just repeat the check here
PKG_CHECK_MODULES([TIRPC], [libtirpc], [],
    [AC_MSG_NOTICE([libtirpc not found])])
AC_SUBST([TIRPC_CFLAGS])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
