#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT(builder, 0.3, Elena.Vengerova@oktetlabs.ru)
AC_CONFIG_SRCDIR([builder.m4])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

CFLAGS=${CFLAGS}" "${TE_CFLAGS}
CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}

AC_CONFIG_HEADERS([config.h])

# It's assumed that stdio.h always presents.
# Check for stdlib.h, stdarg.h, string.h and float.h.
AC_HEADER_STDC
AC_CHECK_HEADERS([\
unistd.h \
errno.h \
sys/types.h \
sys/time.h \
sys/socket.h \
sys/select.h \
sys/wait.h \
sys/stat.h \
signal.h \
fcntl.h \
logger_api.h \
])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
