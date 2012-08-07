dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([tapi], [0.1], [Andrew.Rybchenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_tad.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

dnl Checks for programs.
AC_PROG_CC
AC_C_INLINE

AC_PROG_LIBTOOL
AC_PROG_INSTALL

TE_LIB_SET

dnl List of all layers
layers="arp atm bridge cli dhcp eth ipstack iscsi\
        pcap snmp igmp ppp flow forw"
dnl Whether any --with-<layer> option specified
layers_specified=
for layer in $layers ; do
    if test "$(eval echo \$with_$layer)" = "yes" ; then
        layers_specified=yes
        break
    fi
done
if test -z "$layers_specified" ; then
    dnl No --with-<layer> options are specified, everything by default
    dnl except --without-<layer>
    for layer in $layers ; do
        test -z "$(eval echo \$with_$layer)" && eval with_$layer="yes"
    done
fi

dnl 
dnl Dependencies processing
dnl
if test -n "$with_iscsi" ; then
    test -z "$with_ipstack" && with_ipstack="yes"
fi
if test -n "$with_ipstack" ; then
    test -z "$with_arp" && with_arp="yes"
    test -z "$with_eth" && with_eth="yes"
fi
if test -n "$with_arp" -o -n "$with_bridge" ; then
    test -z "$with_eth" && with_eth="yes"
fi

AM_CONDITIONAL(WITH_ARP,     test "$with_arp" = "yes")
AM_CONDITIONAL(WITH_ATM,     test "$with_atm" = "yes")
AM_CONDITIONAL(WITH_BRIDGE,  test "$with_bridge" = "yes")
AM_CONDITIONAL(WITH_CLI,     test "$with_cli" = "yes")
AM_CONDITIONAL(WITH_DHCP,    test "$with_dhcp" = "yes")
AM_CONDITIONAL(WITH_ETH,     test "$with_eth" = "yes")
AM_CONDITIONAL(WITH_PPP,     test "$with_ppp" = "yes")
AM_CONDITIONAL(WITH_IPSTACK, test "$with_ipstack" = "yes")
AM_CONDITIONAL(WITH_IGMP,    test "$with_igmp" = "yes")
AM_CONDITIONAL(WITH_ISCSI,   test "$with_iscsi" = "yes")
AM_CONDITIONAL(WITH_PCAP,    test "$with_pcap" = "yes")
AM_CONDITIONAL(WITH_SNMP,    test "$with_snmp" = "yes")
AM_CONDITIONAL(WITH_FLOW,    test "$with_flow" = "yes")
AM_CONDITIONAL(WITH_FORW,    test "$with_forw" = "yes")

if test "$with_snmp" = "yes" ; then
    AC_MSG_CHECKING(for net-snmp library)

    TE_BINUTIL([net-snmp-config], [NET_SNMP_CONFIG_FULL_PATH])

    NET_SNMP_CFLAGS=`$NET_SNMP_CONFIG_FULL_PATH --cflags`
    NET_SNMP_LIBS=`$NET_SNMP_CONFIG_FULL_PATH --libs`

    AC_SUBST([NET_SNMP_CFLAGS])
    AC_SUBST([NET_SNMP_LIBS])
     
    AC_CHECK_HEADERS([net-snmp/definitions.h], , ,  
                     [#include<net-snmp/net-snmp-config.h>]) 
fi

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
