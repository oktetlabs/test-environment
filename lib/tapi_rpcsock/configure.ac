# Process this file with autoconf to produce a configure script.
AC_INIT([tapi_rpcsock], [0.01], [Elena.Vangerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_rpcsock.h])
AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_SUBST([TE_CFLAGS])
CFLAGS="${CFLAGS} ${TE_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${TE_CFLAGS}"
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

RPC_XFILE=${srcdir}/../../include/tarpc.x.m4

AC_SUBST([RPC_XFILE])

AM_CONFIG_HEADER([config.h])

AC_CHECK_HEADERS([\
signal.h \
sys/socket.h \
sys/ioctl.h \
sys/poll.h \
arpa/inet.h \
netinet/in.h \
netinet/ip.h \
netinet/tcp.h \
net/if.h \
net/if_arp.h \
netdb.h \
fcntl.h \
semaphore.h \
])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
