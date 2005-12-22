dnl Process this file with autoconf to produce a configure script.

AC_INIT([example], [0.01], [Artem.Andreev@oktetlabs.ru])

AC_CONFIG_SRCDIR([example.c])
AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CANONICAL_HOST
AC_SUBST([host])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_SUBST([TE_CPPFLAGS])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])
AC_SUBST([TE_PATH])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])
AC_ARG_VAR([TE_PATH])

AC_CONFIG_HEADERS([config.h])

dnl It's assumed that stdio.h always presents.
dnl Check for stdlib.h, stdarg.h, string.h and float.h.
AC_HEADER_STDC
AC_CHECK_HEADERS([\
sys/types.h \
errno.h \
assert.h \
pthread.h \
])

AC_CONFIG_FILES([\
Makefile \
])

AC_OUTPUT
