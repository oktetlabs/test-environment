#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(ipclib, 0.2, Andrew.Rybchenko@oktetlabs.ru)

AC_CONFIG_SRCDIR([ipc_internal.h])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_SUBST(TE_CPPFLAGS)
AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)

AC_ARG_VAR(TE_CPPFLAGS)
AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)


AM_CONFIG_HEADER(config.h)

AC_CHECK_HEADERS([ \
stdlib.h        \
unistd.h        \
string.h        \
assert.h        \
errno.h         \
sys/types.h     \
sys/un.h        \
ctype.h         \
netdb.h         \
sys/socket.h    \
arpa/inet.h     \
netinet/in.h    \
rpc/pmap_clnt.h \
sys/queue.h     \
])

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
