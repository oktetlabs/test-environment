
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(tapi_dhcp, 0.01, konst@oktetlabs.ru, oleg@oktetlabs.ru)

AC_CONFIG_SRCDIR([tapi_eth.c])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(OBJDUMP, objdump)
AC_CHECK_TOOL(OBJCOPY, objcopy)

CFLAGS="${CFLAGS} ${TE_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${TE_CFLAGS}"

AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)
AC_SUBST(TE_LDADD)
AC_SUBST(TE_DEPENDENCIES)

AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)
AC_ARG_VAR(TE_LDADD)
AC_ARG_VAR(TE_DEPENDENCIES)

AM_CONFIG_HEADER(config.h)

AC_CHECK_HEADERS(net/ethernet.h)
AC_CHECK_HEADERS(netinet/ether.h)
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h stdint.h stdlib.h string.h strings.h sys/socket.h unistd.h])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
