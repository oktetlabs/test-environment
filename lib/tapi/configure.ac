#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(tapi, 0.1, Andrew.Rybchenko@oktetlabs.ru)

AC_CONFIG_SRCDIR([tapi_tad.h])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST(TE_CPPFLAGS)
AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)

AC_ARG_VAR(TE_CPPFLAGS)
AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)

AM_CONFIG_HEADER(config.h)

# It's assumed that stdio.h always presents.
# Check for stdlib.h, stdarg.h, string.h and float.h.
AC_HEADER_STDC
AC_CHECK_HEADERS(ctype.h sys/types.h sys/stat.h unistd.h assert.h \
                 time.h sys/time.h strings.h sys/queue.h sys/socket.h \
                 arpa/inet.h netinet/in.h net/if.h net/ethernet.h \
                 string.h strings.h)

AC_C_INLINE

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
