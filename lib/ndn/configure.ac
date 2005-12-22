
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([ndn], [0.90], [konst@oktetlabs.ru])

AC_CONFIG_SRCDIR([ndn_generic.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(OBJDUMP, objdump)
AC_CHECK_TOOL(OBJCOPY, objcopy)


TE_LIB_SET


AC_CHECK_HEADERS(ucd-snmp/snmp_impl.h, has_snmp=true,has_snmp=false,  
#include<sys/types.h> 
#include<ucd-snmp/asn1.h> 
) 

AC_CHECK_HEADERS(net-snmp/definitions.h, has_snmp=true,has_snmp=false,  
#include<net-snmp/net-snmp-config.h>
) 

AM_CONDITIONAL(HAS_SNMP, test x$has_snmp = xtrue )


AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
