
# Process this file with autoconf to produce a configure script.

#AC_PREREQ(2.53)
AC_INIT(ndn, 0.90, konst@oktetlabs.ru)

AC_CONFIG_SRCDIR([ndn_generic.c])

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


AC_ARG_ENABLE(def-context,,
[case "${enableval}" in
  yes) defcontext=true;;
  no)  defcontext=false;;
 esac], 
  [defcontext=true])

AM_CONDITIONAL(DEF_CONTEXT, test x$defcontext = xtrue )



AC_ARG_WITH(file,, 
[case "${withval}" in
  yes) file=true;;
  no)  file=false;;
  esac], [file=false])

AM_CONDITIONAL(USE_FILE, test x$file = xtrue )



AC_ARG_WITH(cli,, 
[case "${withval}" in
  yes) cli=true;;
  no)  cli=false;;
  esac], [cli=false])

AM_CONDITIONAL(USE_CLI, test x$cli = xtrue )


AC_ARG_WITH(eth,, 
[case "${withval}" in
  yes) eth=true;;
  no)  eth=false;;
  esac], [eth=false])

AM_CONDITIONAL(USE_ETH, test x$eth = xtrue )


AC_ARG_WITH(ipstack,, 
[case "${withval}" in
  yes) ipstack=true;;
  no)  ipstack=false;;
  esac], [ipstack=false])

AM_CONDITIONAL(USE_IPSTACK, test x$ipstack = xtrue )



AC_ARG_WITH(dhcp,, 
[case "${withval}" in
  yes) dhcp=true;;
  no)  dhcp=false;;
  esac], [dhcp=false])

AM_CONDITIONAL(USE_DHCP, test x$dhcp = xtrue )



AC_ARG_WITH(snmp,, 
[case "${withval}" in
  yes) snmp=true;;
  no)  snmp=false;;
  esac], [snmp=false])

AM_CONDITIONAL(USE_SNMP, test x$snmp = xtrue )

AC_ARG_WITH(bridge,, 
[case "${withval}" in
  yes) bridge=true;;
  no)  bridge=false;;
  esac], [bridge=false])

AM_CONDITIONAL(USE_BRIDGE, test x$bridge = xtrue )


AC_CHECK_HEADERS(ucd-snmp/snmp_impl.h, ,,  
#include<sys/types.h> 
#include<ucd-snmp/asn1.h> 
) 

AC_CHECK_HEADERS(net-snmp/definitions.h, ,,  
#include<net-snmp/net-snmp-config.h>
) 



AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
