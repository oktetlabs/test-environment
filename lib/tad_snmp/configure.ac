# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(tad_snmp, 0.01, konst@oktetlabs.ru)

AC_CONFIG_SRCDIR([tad_snmp_csap.c])
AC_CONFIG_AUX_DIR(../../aux)

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

AC_MSG_CHECKING(for net-snmp library)
AC_PATH_PROG(NET_SNMP_CONFIG, net-snmp-config, no)
if test "$NET_SNMP_CONFIG" = "no" ; then
    AC_MSG_ERROR([net-snmp-config is required but not found])
else
    NET_SNMP_CFLAGS=`$NET_SNMP_CONFIG --cflags`
    NET_SNMP_LIBS=`$NET_SNMP_CONFIG --libs`
fi
AC_SUBST(NET_SNMP_CFLAGS)
AC_SUBST(NET_SNMP_LIBS)

AC_CHECK_HEADERS(unistd.h fcntl.h sys/types.h sys/time.h unistd.h semaphore.h)
#AC_CHECK_HEADERS(ucd-snmp/snmp_client.h, ,,  
##include<stdio.h> 
##include<sys/types.h> 
##include<sys/time.h> 
##include<ucd-snmp/asn1.h> 
##include<ucd-snmp/snmp.h> 
##include<ucd-snmp/snmp_impl.h> 
##include<ucd-snmp/snmp_api.h> 
#) 

AC_CHECK_HEADERS(net-snmp/session_api.h, ,,  
#include<net-snmp/net-snmp-config.h>
)

AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
