
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([tapi_snmp], [0.01], [konst@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_snmp.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

TE_LIB_SET

AC_MSG_CHECKING(for net-snmp library)

TE_BINUTIL([net-snmp-config], [NET_SNMP_CONFIG_FULL_PATH])

NET_SNMP_CFLAGS=`$NET_SNMP_CONFIG_FULL_PATH --cflags`
NET_SNMP_LIBS=`$NET_SNMP_CONFIG_FULL_PATH --libs`

AC_SUBST([NET_SNMP_CFLAGS])
AC_SUBST([NET_SNMP_LIBS])
 
AC_CHECK_HEADERS(net-snmp/definitions.h, ,,  
#include<net-snmp/net-snmp-config.h>
) 

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
