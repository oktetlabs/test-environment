# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT([tad_ch], 0.1, [konst@oktetlabs.ru])

AC_CONFIG_SRCDIR([tad_ch.c])
AC_CONFIG_AUX_DIR([../../aux])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL


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


AC_ARG_WITH(snmp,, 
[case "${withval}" in
  yes) w_snmp=true;;
  no)  w_snmp=false;;
  esac], [w_snmp=false])

AM_CONDITIONAL(USE_SNMP, test x$w_snmp = xtrue )

AC_ARG_WITH(file,, 
[case "${withval}" in
  yes) w_file=true;;
  no)  w_file=false;;
  esac], [w_file=false])

AM_CONDITIONAL(USE_FILE, test x$w_file = xtrue )

    
AC_ARG_WITH(eth,, [case "${withval}" in
  yes) w_eth=true;;
  no)  w_eth=false;;
  esac], [w_eth=false])

AM_CONDITIONAL(USE_ETH, test x$w_eth = xtrue )


AC_ARG_WITH(ipstack,, [case "${withval}" in
  yes) w_ipstack=true;;
  no)  w_ipstack=false;;
  esac], [w_ipstack=false])

AM_CONDITIONAL(USE_IPSTACK, test x$w_ipstack = xtrue )


AC_ARG_WITH(dhcp,, 
[case "${withval}" in
  yes) w_dhcp=true;;
  no)  w_dhcp=false;;
  esac], [w_dhcp=false])

AM_CONDITIONAL(USE_DHCP, test x$w_dhcp = xtrue )


AC_ARG_WITH(cli,, 
[case "${withval}" in
  yes) w_cli=true;;
  no)  w_cli=false;;
  esac], [w_cli=false])

AM_CONDITIONAL(USE_CLI, test x$w_cli = xtrue )

    
AC_ARG_WITH(bridge,, 
[case "${withval}" in
  yes) bridge=true;;
  no)  bridge=false;;
  esac], [bridge=false])

AM_CONDITIONAL(USE_BRIDGE, test x$bridge = xtrue )


AM_CONFIG_HEADER([config.h])
AC_C_INLINE
AC_CHECK_HEADERS([\
time.h \
sys/time.h \
semaphore.h \
])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

