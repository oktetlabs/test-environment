#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT(comm_net_engine, 0.2, Andrew.Rybchenko@oktetlabs.ru)

AC_CONFIG_SRCDIR([comm_net_engine.c])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_SUBST([TE_CPPFLAGS])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])

AC_ARG_VAR([TE_CPPFLAGS])
AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

AM_CONFIG_HEADER([config.h])

AC_CHECK_HEADERS([config.h              \
                  stdlib.h              \
                  stdio.h               \
                  unistd.h              \
                  netdb.h               \
                  ctype.h               \
                  sys/types.h           \
                  sys/socket.h          \
                  sys/errno.h           \
                  arpa/inet.h           \
                  netinet/in.h          \
                  netinet/tcp.h         \
                ])


AC_CONFIG_FILES([Makefile])

AC_OUTPUT
