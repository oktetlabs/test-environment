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

dnl Check for libpcre

PCRE_LIBS=

AC_CHECK_HEADER([pcre.h], [],
[AC_MSG_ERROR([Job control RPC server requires pcre.h])],
[#include <pcre.h>])
AC_CHECK_LIB([pcre], [pcre_exec], [],
[AC_MSG_ERROR([Job control RPC server requires PCRE library])])

AC_SUBST([PCRE_LIBS])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
