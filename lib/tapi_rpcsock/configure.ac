# Process this file with autoconf to produce a configure script.
AC_INIT(tapi_rpcsock, 0.01, Elena.Vangerova@oktetlabs.ru)

AC_CONFIG_SRCDIR([tapi_rpcsock.h])
AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_SUBST(TE_CFLAGS)
CFLAGS="${CFLAGS} ${TE_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${TE_CFLAGS}"
AC_SUBST(TE_LDFLAGS)
AC_SUBST(TE_LDADD)
AC_SUBST(TE_DEPENDENCIES)

AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)
AC_ARG_VAR(TE_LDADD)
AC_ARG_VAR(TE_DEPENDENCIES)

RPC_XFILE=${srcdir}/../../agents/${with_ta}/tarpc.x.m4

AC_SUBST(RPC_XFILE)

AM_CONFIG_HEADER(config.h)

AC_CHECK_HEADERS(sys/socket.h netinet/in.h arpa/inet.h net/if.h signal.h \
netinet/ip.h netinet/tcp.h sys/ioctl.h sys/poll.h netdb.h semaphore.h)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
