#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([confapi], [0.01], [Renata.Sayakhova@oktetlabs.ru])

AC_CONFIG_SRCDIR([conf_api.h])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])

AC_CONFIG_HEADERS([config.h])

AC_CHECK_HEADERS([\
pthread.h \
stdarg.h \
string.h \
strings.h \
sys/socket.h \
netinet/in.h \
arpa/inet.h \
assert.h \
])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
