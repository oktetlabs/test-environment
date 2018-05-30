dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ta-unix],[0.9],[Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([main.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])
TE_SETUP_AR

AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_C_INLINE

AC_PROG_INSTALL
AC_PROG_RANLIB

AC_CANONICAL_HOST

TE_APP_SET

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS_VAR}
CFLAGS=${CFLAGS}" "${TE_CFLAGS_VAR}
DYNAMIC_LIBS=

AC_ARG_WITH([name], , , [with_name=linux])

AC_SUBST([with_name])

AC_ARG_WITH([tad], , , [with_tad=tad])


if test -z "$with_tad" ; then
    with_tad=tad
fi
AC_SUBST([TAD], [$with_tad])

AC_SUBST([TAD_LIBS])

AC_ARG_WITH([rcf_rpc], , , )

AM_CONDITIONAL(CFG_RCF_RPC, test -n "$with_rcf_rpc")

if test "$with_rcf_rpc" ; then
    TE_CPPFLAGS="$TE_CPPFLAGS -DRCF_RPC"
fi    

AH_TEMPLATE([TE_AGT_PLATFORM],[TA platform])
AC_DEFINE_UNQUOTED([TE_AGT_PLATFORM], ["${TE_PLATFORM}"], [])

dnl #######################################################################
dnl Newer BSD systems don't have a compatible rtentry - use ortentry
dnl #######################################################################
AC_CHECK_TYPES([struct rtentry, struct ortentry], [], [],
               [#include <sys/types.h>
                #include <sys/socket.h>
                #include <net/route.h>])

dnl
dnl Check for 'struct lifreq'
dnl
AC_CHECK_TYPES([struct lifreq], [], [], [#include <net/if.h>])

dnl
dnl Check for 'ifr_mtu' member in 'struct ifreq'.
dnl
AC_CHECK_MEMBERS([struct ifreq.ifr_mtu], [], [], [
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
])

dnl
dnl Check for 'ifr_hwaddr' member in 'struct ifreq'.
dnl
AC_CHECK_MEMBERS([struct ifreq.ifr_hwaddr], [], [], [
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
])

dnl
dnl Check for 'ifr_enaddr' member in 'struct ifreq'.
dnl
AC_CHECK_MEMBERS([struct ifreq.ifr_enaddr], [], [], [
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
])

dnl
dnl Check for 'lifr_mtu' member in 'struct lifreq'.
dnl
AC_CHECK_MEMBERS([struct lifreq.lifr_mtu], [], [], [
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
])

dnl
dnl Check for 'arp_dev' member in 'struct arpreq'.
dnl
AC_CHECK_MEMBERS([struct arpreq.arp_dev], [], [],
                 [#include <net/if_arp.h>])

dnl
dnl Berkley Socket API is provided by:
dnl - -lc on Linux, FreeBSD (5, 6), NetNSD (1.6, 2.0);
dnl - -lsocket on Solaris2 (SunOS 5.11).
dnl
AC_SEARCH_LIBS([socket], [socket], [],
    AC_MSG_ERROR([No library with Berkley Socket API found]))

dnl
dnl sendfile() is provided by:
dnl - -lc on Linux;
dnl - -lsendfile on Solaris2 (SunOS 5.11).
dnl
AC_SEARCH_LIBS([sendfile], [sendfile])

dnl    
dnl libdlpi is provided on Solaris
dnl
AC_ARG_ENABLE([libdlpi], [Build Test Agent with libdlpi, if available],
AC_CHECK_LIB([dlpi], [dlpi_open]))
AC_CHECK_HEADERS([libdlpi.h])

if test -n "$with_rcf_rpc" -o -n "$with_cfg_unix_daemons" ; then
    dnl
    dnl Dynamic linker functions are provided by:
    dnl - -ldl in Linux;
    dnl - -lc in FreeBSD;
    dnl - N/A for NetBSD, since automatically included in every dynamically
    dnl   linked program.
    dnl
    AC_CHECK_FUNC([dlopen], [],
                  AC_CHECK_LIB([dl], [dlopen], [],
                               AC_MSG_ERROR([No dynamic linker API found])))
fi

dnl
dnl Check for mkdtemp() function.
dnl
AC_CHECK_FUNCS([mkdtemp])

dnl
dnl POSIX threads are provided by:
dnl - -lpthread in Linux;
dnl - -lc_r (-lkse) in FreeBSD;
dnl - -lpthread in NetBSD ('pth' package).
dnl - -lc in QNX (usually included automatically)
dnl
AC_SEARCH_LIBS([pthread_create], [pthread kse c_r], [],
               AC_MSG_ERROR([No POSIX threads]))

dnl
dnl POSIX semaphores are provided by:
dnl - -lpthread on Linux and NetBSD 3.1;
dnl - -lc_r on FreeBSD (5, 6);
dnl - -lsemaphore on NetBSD 2.0 ('pthread-sem' package);
dnl - -lrt on Solaris2 (SunOS 5.11).
dnl
AC_SEARCH_LIBS([sem_init], [pthread c_r semaphore rt], [],
               AC_MSG_ERROR([No POSIX semaphores]))

dnl
dnl Pluggable Authentication Modules are provided by:
dnl - -lpam on Linux, NetBSD, Solaris2 (SunOS 5.11);
dnl - ? on FreeBSD.
dnl
AC_ARG_WITH([pam],,
[
AC_SEARCH_LIBS([pam_start], [pam],
               AC_DEFINE([HAVE_LIBPAM], [1],
                         [PAM (Pluggable Authentication Modules) support]),
               AC_MSG_WARN(
               [No PAM (Pluggable Authentication Modules) support]))

AC_CHECK_HEADERS([security/pam_appl.h])
],
[AC_DEFINE([HAVE_LIBPAM], [0],
           [No PAM (Pluggable Authentication Modules) support])])

AC_CHECK_HEADERS([sys/klog.h])
AC_CHECK_HEADERS([linux/if_vlan.h])
AC_CHECK_HEADERS([search.h])

dnl
dnl Check for 'struct if_nameindex'
dnl
AC_CHECK_TYPES([struct if_nameindex], [], [],
               [#include <net/if.h>])

dnl
dnl Asynchronous I/O:
dnl - FreeBSD has in -lc;
dnl - Linux has in -lnsl -lrt;
dnl - Solaris2 (SunOS 5.11) in -lrt;
dnl - NetBSD does not have.
dnl
AC_CHECK_FUNC([aio_read], [],
    AC_SEARCH_LIBS([aio_read], [rt], [],
        AC_CHECK_LIB([nsl], [aio_read], [LIBS="-lnsl -lrt ${LIBS}"],
             AC_MSG_WARN([Asynchronous I/O not supported]), [-lrt])))

export PKG_CONFIG_PATH="$DESTDIR/$prefix/../$TE_PLATFORM/lib/pkgconfig/"
PKG_CHECK_MODULES(TAD, te-$with_tad)
TAD_LIBS=`${TE_BASE}/engine/builder/static_libs_check.pl "${TAD_LIBS}"`

dnl util library is necessary for static linking of expect
AC_SEARCH_LIBS([openpty], [util])

dnl "conf" staff

EXTRA_CPPFLAGS=
EXTRA_LDADD=

AC_PROG_LEX
AC_PROG_YACC

AC_ARG_ENABLE([pci],
              AS_HELP_STRING([--enable-pci],
                             [enable PCI support (default is NO)]),
              [ac_cv_enable_pci=$enableval], [ac_cv_enable_pci=no])
AC_CACHE_CHECK([whether to enable PCI],
               [ac_cv_enable_pci], [ac_cv_enable_pci=no])
if test x"$ac_cv_enable_pci" != xno
then
    AC_DEFINE([ENABLE_PCI_SUPPORT], [], [Whether to enable PCI support])
fi
AM_CONDITIONAL([CFG_PCI], test x"$ac_cv_enable_pci" != xno)


AC_ARG_ENABLE([wifi],
              AS_HELP_STRING([--enable-wifi],
                             [enable WiFi support (default is NO)]),
              [ac_cv_enable_wifi=$enableval], [ac_cv_enable_wifi=no])
AC_CACHE_CHECK([whether to enable WiFi],
               [ac_cv_enable_wifi], [ac_cv_enable_wifi=no])

if test x"$ac_cv_enable_wifi" != xno
then
dnl
dnl Check that we have iwlib installed in our system
dnl
    AC_CHECK_HEADERS([iwlib.h], [],
                     AC_MSG_ERROR([Cannot find iwlib.h header]))
    AC_CHECK_LIB([iw], [iw_sockets_open], [EXTRA_LDADD="$EXTRA_LDADD -liw"],
                 AC_MSG_ERROR([Cannot find iw library]), [-lm])

    AC_DEFINE([ENABLE_WIFI_SUPPORT], [], [Whether to enable WiFi support])
    LIBS="$LIBS -lm"
fi

AM_CONDITIONAL([CFG_UNIX_WIFI], test x"$ac_cv_enable_wifi" != xno)

AC_ARG_ENABLE([8021x],
              AS_HELP_STRING([--enable-8021x],
                             [enable IEEE802.1x support (default is NO)]),
              [ac_cv_enable_8021x=$enableval], [ac_cv_enable_8021x=no])
AC_CACHE_CHECK([whether to enable 802.1x],
               [ac_cv_enable_8021x], [ac_cv_enable_8021x=no])

AM_CONDITIONAL([CFG_UNIX_8021X], test x"$ac_cv_enable_8021x" != xno)

if test x"$ac_cv_enable_8021x" != xno
then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DENABLE_8021X"
fi

DS_SOURCES=
if test -n "$with_cfg_unix_daemons" ; then
  DAEMONS_CFLAGS=-DCFG_UNIX_DAEMONS
  for i in $with_cfg_unix_daemons ; do
    case $i in 
        tftp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TFTP_SERVER"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
            
        ftp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_FTP_SERVER" 
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;            
            
        dns)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DNS_SERVER" 
            ;;
            
        echo)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_ECHO_SERVER"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
            
        todudp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TODUDP_SERVER"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
            
        telnet)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TELNET"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
            
        rsh)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_RSH"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
            
        radvd)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_RADVD" ;
            ;;
        dhcp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DHCP_SERVER" ;
            ;;

        openvpn)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_OPENVPN" ;
            ;;

        pppoe)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_PPPOE_SERVER"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_PPPOE_CLIENT"
            ;;

        radius)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_RADIUS_SERVER" ;
            ;;
            
        smtp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_SMTP" ;;
            
        sshd) ;;
        
        Xvfb) ;;
        
        vncserver)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_VNCSERVER" ;;
        
        vtund)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_VTUND" ;;

        l2tp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_L2TP" ;;

        socks)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_SOCKS" ;;

        *)  ;; 
    esac
  done
fi
AC_SUBST([DAEMONS_CFLAGS])

AM_CONDITIONAL(CFG_UNIX_DAEMONS, test -n "$with_cfg_unix_daemons")
AM_CONDITIONAL(CFG_UNIX_DAEMONS_NAMED, 
    [expr "$with_cfg_unix_daemons" : ".*dns" >/dev/null])
AM_CONDITIONAL(CFG_UNIX_DAEMONS_RADVD,
    [expr "$with_cfg_unix_daemons" : ".*radvd" >/dev/null])
AM_CONDITIONAL(CFG_UNIX_DAEMONS_ISC_DHCP_SERVER,
    [expr "$with_cfg_unix_daemons" : ".*dhcp" >/dev/null])
AM_CONDITIONAL(CFG_UNIX_DAEMONS_OPENVPN,
    [expr "$with_cfg_unix_daemons" : ".*openvpn" >/dev/null])
AM_CONDITIONAL(CFG_UNIX_DAEMONS_PPPOE_SERVER,
    [expr "$with_cfg_unix_daemons" : ".*pppoe" >/dev/null])
AM_CONDITIONAL(CFG_UNIX_DAEMONS_PPPOE_CLIENT,
    [expr "$with_cfg_unix_daemons" : ".*pppoe" >/dev/null])
AM_CONDITIONAL(CFG_UNIX_DAEMONS_VTUND, 
    [expr "$with_cfg_unix_daemons" : ".*vtund" >/dev/null])
AM_CONDITIONAL([CFG_UNIX_DAEMONS_RADIUS],
    [expr "$with_cfg_unix_daemons" : ".*radius" >/dev/null])
AM_CONDITIONAL([CFG_UNIX_DAEMONS_L2TP],
    [expr "$with_cfg_unix_daemons" : ".*l2tp" >/dev/null])
AM_CONDITIONAL([CFG_UNIX_DAEMONS_SOCKS],
    [expr "$with_cfg_unix_daemons" : ".*socks" >/dev/null])

AC_ARG_WITH([libnetconf],
            AS_HELP_STRING([--with-libnetconf],
                           [enable libnetconf support (defaults are:
                            YES for linux, NO for others)]),
            [],
            [with_libnetconf=check])

if test "x$with_libnetconf" = xcheck; then
    # Check if $host variable keeps 'linux' substring
    # and replace 'linux' with an empty string.
    # Then we compare if the original string is equal to
    # a string after the replacement.
    up_host=`echo $host | sed "s/linux//"`
    if test x"$host" != x"${up_host}" ; then
        # Linux platform - use libnetconf by default
        with_libnetconf=yes
    else
        with_libnetconf=no
    fi
fi

if test x"$with_libnetconf" != xno; then
    AC_CHECK_HEADER([netconf.h],
                    [],
                    [AC_MSG_ERROR([netconf.h was not found, try to add \
netconf in TE_PLATFORM macro in your builder.conf])],
                    [])

    # On Linux netlink is default way to configure interfaces and IP
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DUSE_LIBNETCONF"
    EXTRA_LDADD="$EXTRA_LDADD -lnetconf"
else
    #
    # We do not use netconf interface, so the only thing we can do
    # is to use ioctl interface and routing sockets.
 
    # Check if we have routing sockets
    AC_CHECK_HEADERS([net/route.h])
    AC_CHECK_TYPES([struct rt_msghdr],
                   [EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DUSE_ROUTE_SOCKET"],
                   [], [
                   #if HAVE_UNISTD_H
                   #include <unistd.h>
                   #endif
                   #if HAVE_NET_ROUTE_H
                   #include <net/route.h>
                   #endif
    ])

    # The rest should use IOCTL interface for now
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DUSE_IOCTL"
fi

AC_ARG_ENABLE([vcm],
              AS_HELP_STRING([--enable-vcm],
                             [enable VCM mgmt support (default is NO)]),
              [ac_cv_enable_vcm=$enableval], [ac_cv_enable_vcm=no])

AM_CONDITIONAL(CFG_VCM, test "x$ac_cv_enable_vcm" = "xyes")
if test "x$ac_cv_enable_vcm" = "xyes" ; then
    AC_DEFINE([ENABLE_VCM_SUPPORT], [], [Whether to enable VCM support])
fi    

AC_ARG_WITH([tr069],
            AS_HELP_STRING([--with-tr069],
                           [enable TR-069 support (default is NO)]),
            [],
            [with_tr069=no])

AM_CONDITIONAL(CFG_TR069, test x"$with_tr069" != xno)
AS_IF([test x"$with_tr069" != xno],
      [AC_DEFINE([WITH_TR069_SUPPORT], [], \
      [Whether to enable TR-069 support]) \
      EXTRA_LDADD="$EXTRA_LDADD -lgsoapssl -lssl -lcrypto -lz"])

AM_CONDITIONAL(ISCSI_TARGET_USE, test -n "$with_iscsi")
if test -n "$with_iscsi"; then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_ISCSI"
    EXTRA_LDADD="$EXTRA_LDADD -liscsi_unh_target -liscsi_initiator_conf"
fi    

AC_ARG_WITH([symtbl-curl],
            AS_HELP_STRING([--with-symtbl-curl],
                           [enable curl symtbl (default is NO)]),
            [with_symtbl_curl=yes],
            [with_symtbl_curl=""])

test -n "$with_symtbl_curl" && {
    echo "It searches in ${TE_EXT_LIBS_PATH}"
    if test -f ${TE_EXT_LIBS_PATH}/libcurl.a ; then
        CURL_LIB=${TE_EXT_LIBS_PATH}/libcurl.a
    else
        CURL_LIB=`$CC $CFLAGS -print-file-name=libcurl.a`
    fi
#   Do not do this: it is used just for generating symbol table
#   CURL_LIB=`${TE_BASE}/engine/builder/static_libs_check.pl "${CURL_LIB}"`
    AC_SUBST(CURL_LIB)
}

AC_ARG_WITH([iptables],
            AS_HELP_STRING([--with-iptables],
                           [enable iptables (default is NO)]),
            [with_iptables=yes],
            [with_iptables=""])

AM_CONDITIONAL(CFG_IPTABLES_SUPPORT, test -n "$with_iptables")
if test -n "$with_iptables"; then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_IPTABLES"
fi

AC_CHECK_LIB([pcap], [pcap_open_live], [PCAP_AVAIL=yes])

AC_ARG_WITH([sniffers],
            AS_HELP_STRING([--with-sniffers=yes],
                           [enable sniffers (default is NO)]),
            [], [with_sniffers=""])

if test "x${PCAP_AVAIL}" != "xyes" ; then
    if test "x$with_sniffers" = "xyes" ; then
        with_sniffers=""
        AC_MSG_WARN([Failed to enable sniffers because
                     pcap library is not available])
    fi
fi

AM_CONDITIONAL([CFG_SNIFFERS_SUPPORT], [test "x$with_sniffers" = "xyes"])
AM_CONDITIONAL([CFG_SNIFFERS_DUMMY], [test "x$with_sniffers" != "xyes"])
if test "x$with_sniffers" = "xyes" ; then
        EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_SNIFFERS"
fi

AC_ARG_WITH([serialparse],
            AS_HELP_STRING([--with-serialparse],
                           [enable serial parsers (default is NO)]),
            [with_serialparse=yes], [with_serialparse=""])

AM_CONDITIONAL(CFG_SERIALPARSE_SUPPORT, test "$with_serialparse" = "yes")
if test "$with_serialparse" = "yes" ; then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_SERIALPARSE"
fi


dnl
dnl UPnP Control Point
dnl
AC_ARG_WITH([upnp_cp],
            [AS_HELP_STRING([--with-upnp_cp],
                [enable UPnP Control Point (disabled on default)])],
            [AS_CASE([$withval],
                [yes], [ac_cv_with_upnp_cp=true],
                [no] , [ac_cv_with_upnp_cp=false],
                [AC_MSG_ERROR([invalid value $withval for --with-upnp_cp])])
            ],
            [ac_cv_with_upnp_cp=false]
)
AM_CONDITIONAL([CFG_UPNP_CP_SUPPORT], [test x$ac_cv_with_upnp_cp = xtrue])
UPNP_CP_LIBS=
AS_IF([test x$ac_cv_with_upnp_cp = xtrue],
      [
        AC_DEFINE([WITH_UPNP_CP], [1],
                  ["Whether use or not UPnP Control Point."])

        PKG_CHECK_MODULES([LIBUPNP_CP], [te-upnp_cp],
            [UPNP_CP_LIBS=${LIBUPNP_CP_LIBS}],
            [AC_MSG_ERROR([te-upnp_cp not found])]
        )
      ]
)


AC_ARG_WITH([sfptpd],
            AS_HELP_STRING([--with-sfptpd],
                           [enable sfptpd (default is NO)]),
            [with_sfptpd=yes], [with_sfptpd=""])
AM_CONDITIONAL(CFG_SFPTPD_SUPPORT, test "$with_sfptpd" = "yes")
if test "$with_sfptpd" = "yes" ; then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_SFPTPD"
fi

AC_ARG_WITH([ntpd],
            AS_HELP_STRING([--with-ntpd],
                           [enable ntpd (default is NO)]),
            [with_ntpd=yes], [with_ntpd=""])
AM_CONDITIONAL(CFG_NTPD_SUPPORT, test "$with_ntpd" = "yes")
if test "$with_ntpd" = "yes" ; then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_NTPD"
fi

AC_ARG_WITH([tirpc], , , )
if test "x$with_tirpc" = "xyes" ; then
    # Modern Linux distros, such as Fedora 28, do not have RPC support
    # in glibc; libtirpc can be used instead.
    TIRPC=`${TE_BASE}/engine/builder/static_libs_check.pl "-ltirpc"`
    EXTRA_LDADD="$EXTRA_LDADD ${TIRPC}"
fi

AC_SUBST([EXTRA_CPPFLAGS])
AC_SUBST([EXTRA_LDADD])

DYNAMIC_LIBS="-ldl -lpthread -lnsl -lsendfile -lsocket -lm"
if test x${PCAP_AVAIL} = "xyes" ; then
    DYNAMIC_LIBS="${DYNAMIC_LIBS} -lpcap"
fi

dnl m library is necessary for telephony.c
AC_SEARCH_LIBS([cos], [m])

AC_ARG_WITH([static_libc],
            AS_HELP_STRING([--with-static-libc],
                           [enable static libc (default is YES)]))
if test "${with_static_libc}" = "no" ; then
    # If the library name is a part of the variable 'DYNAMIC_LIBS',
    # then dynamic version of this library will be used for linking.
    DYNAMIC_LIBS="${DYNAMIC_LIBS} -lutil -lrt"
fi

#if ! test "$cross_compiling" = "yes" ; then
case "$host" in
   sparc*linux*) TE_CPPFLAGS="$TE_CPPFLAGS -DNO_DL" ;;
   *uml*) LIBS="$LIBS -ldl";;
   *)
       S_LIBS=
       D_LIBS=
       for i in $LIBS $TAD_LIBS ; do
           echo Scan $i
           found=
           for k in ${DYNAMIC_LIBS} ; do
               if test "${k}" = "${i}" ; then
                   echo found
                   found=yes
                   break;
               fi
           done
           if test -z "$found" ; then
               S_LIBS="${S_LIBS} $i"
           else
               D_LIBS="${D_LIBS} $i"
           fi
       done
       echo S_LIBS ${S_LIBS}
       echo D_LIBS ${D_LIBS}
       LIBS="${S_LIBS} -Wl,-Bdynamic ${D_LIBS} ${UPNP_CP_LIBS}" ;;
esac

AC_ARG_WITH([aggr], , [with_aggr=yes], [])
AM_CONDITIONAL(CFG_UNIX_AGGR, test "x$with_aggr" != "x")
if test "x$with_aggr" != "x" ; then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_AGGREGATION"
fi

AC_ARG_WITH([dpdk],
            AS_HELP_STRING([--with-dpdk=RTE_SDK_BIN],
                           [specify location of the DPDK RTE SDK bulid]),
            [ac_cv_with_dpdk="$with_dpdk"],
            [ac_cv_with_dpdk=no])

if test x"$ac_cv_with_dpdk" != xno; then
    test x"$ac_cv_with_dpdk" = xyes &&
        ac_cv_with_dpdk="${TE_INSTALL}/${TE_PLATFORM}"
    # Add DPDK library just before system libraries
    DPDK_LIBS="$(find "${ac_cv_with_dpdk}/lib" \( -name librte_*.a -or -name libethdev.a \) -printf '%p ')"
    LIBS="-L${ac_cv_with_dpdk}/lib -Wl,-whole-archive ${DPDK_LIBS} -Wl,-no-whole-archive ${LIBS}"
fi

AC_SUBST([DPDK_LIBS])

AC_ARG_ENABLE([generate-symtbl],
              AS_HELP_STRING([--disable-generate-symtbl],
                             [disable symbol table generation (default is YES)]),
              [ac_cv_enable_generate_symtbl=$enableval], [ac_cv_enable_generate_symtbl=yes])
AC_CACHE_CHECK([whether to enable symbol table generation],
               [ac_cv_enable_generate_symtbl], [ac_cv_enable_generate_symtbl=yes])
if test x"$ac_cv_enable_generate_symtbl" != xno
then
    AC_DEFINE([ENABLE_GENERATED_SYMTBL], [], [Whether to use generated symbol tables])
    AC_CHECK_TARGET_TOOL(NM, [nm --format=sysv],
                         [AC_MSG_ERROR([No SysV-compatible 'nm' tool found])])
    
fi
AM_CONDITIONAL([GENERATE_SYMTBL], test x"$ac_cv_enable_generate_symtbl" != xno)

AC_ARG_ENABLE([tce],
              AS_HELP_STRING([--enable-tce],
                             [enable TCE support (default is NO)]),
              [ac_cv_enable_tce=$enableval], [ac_cv_enable_tce=no])
AC_CACHE_CHECK([whether to enable TCE],
               [ac_cv_enable_tce], [ac_cv_enable_tce=no])
if test x"$ac_cv_enable_tce" != xno
then
    TE_CFLAGS="$TE_CFLAGS --coverage"
fi

dnl
dnl Check for setns() function.
dnl
AC_CHECK_FUNCS([setns])

AC_SUBST([TE_BASE])
AC_SUBST([TE_BUILD])

dnl end of "conf" staff

AC_CONFIG_FILES([Makefile conf/Makefile])

TE_APP_RESTORE

AC_OUTPUT

