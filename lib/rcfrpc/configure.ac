#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(rcfrpc, 0.02, Elena.Vengerova@oktetlabs.ru)

AC_CONFIG_SRCDIR([rcf_rpc.h])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)


CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST(TE_CPPFLAGS)
AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)

AC_ARG_VAR(TE_CPPFLAGS)
AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)

RPC_XFILE=${srcdir}/../../include/tarpc.x.m4
AC_SUBST(RPC_XFILE)

AM_CONFIG_HEADER(config.h)

AC_CHECK_HEADERS(pthread.h semaphore.h fcntl.h sys/socket.h semaphore.h \
                 sys/wait.h)

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
