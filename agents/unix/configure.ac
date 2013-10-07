dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([ta-unix],[0.8],[Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([main.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CONFIG_HEADERS([config.h])

dnl Checks for programs.
AC_PROG_CC
AC_C_INLINE

AC_PROG_INSTALL
AC_PROG_RANLIB

AC_CANONICAL_HOST

AC_CHECK_TOOL(NM, nm)

TE_APP_SET

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

# Include telephony support by default
AC_ARG_WITH([telephony],
            [AS_HELP_STRING([--without-telephony],
              [Disable support DAHDI interface via RPC calls. \
Default - enabled.])],
            [if test "x${withval}" = "xyes"; then
                 with_telephony=yes;
             fi], [with_telephony=yes])

AM_CONDITIONAL(WITH_TELEPHONY, test "x$with_telephony" = "xyes")
if test "x$with_telephony" = "xyes"; then
    TE_CPPFLAGS="$TE_CPPFLAGS -DWITH_TELEPHONY"
fi

# Include power switch support by default
AC_ARG_WITH([power_sw],
            [AS_HELP_STRING([--without-power_sw],
              [Disable support for power switch via RPC calls. \
Default - enabled.])],
            [if test "x${withval}" = "xyes"; then
                 with_power_sw=yes;
             fi], [with_power_sw=yes])

AM_CONDITIONAL(WITH_POWER_SW, test "x$with_power_sw" = "xyes")
if test "x$with_power_sw" = "xyes"; then
    TE_CPPFLAGS="$TE_CPPFLAGS -DWITH_POWER_SW"
fi

dnl #######################################################################
dnl Newer BSD systems don't have a compatible rtentry - use ortentry
dnl #######################################################################
AC_CHECK_TYPES([struct rtentry, struct ortentry], [], [],
               [#include <sys/types.h>
                #include <sys/socket.h>
                #include <net/route.h>])

dnl
dnl Check for 'struct ip_mreqn'
dnl
AC_CHECK_TYPES([struct ip_mreqn], [], [],
               [#include <netinet/in.h>])
dnl
dnl Check for 'struct ip_opts'
dnl
AC_CHECK_TYPES([struct ip_opts], [], [],
               [#include <netinet/in.h>])

dnl
dnl Check for 'struct group_req'
dnl
AC_CHECK_TYPES([struct group_req], [], [],
               [#include <netinet/in.h>])

dnl
dnl Check for 'struct tcp_info'
dnl
AC_CHECK_TYPES([struct tcp_info], [], [],
               [#include <netinet/tcp.h>])

dnl
dnl Check for 'ipv6mr_interface' member in 'struct ipv6_mreq'.
dnl
AC_CHECK_MEMBERS([struct ipv6_mreq.ipv6mr_interface], [], [],
                 [#include <netinet/in.h>])
dnl
dnl Check for 'ipv6mr_ifindex' member in 'struct ipv6_mreq'.
dnl
AC_CHECK_MEMBERS([struct ipv6_mreq.ipv6mr_ifindex], [], [],
                 [#include <netinet/in.h>])

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
dnl Check for 'sa_restorer' member in 'struct sigaction'.
dnl
AC_CHECK_MEMBERS([struct sigaction.sa_restorer], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_trapno' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_trapno], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_addr_lsb' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_addr_lsb], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_utime' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_utime], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_stime' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_stime], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_int' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_int], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_ptr' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_ptr], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_overrun' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_overrun], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_band' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_band], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_timerid' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_timerid], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_fd' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_fd], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'si_ptr' member in 'siginfo_t'.
dnl
AC_CHECK_MEMBERS([siginfo_t.si_ptr], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'arp_dev' member in 'struct arpreq'.
dnl
AC_CHECK_MEMBERS([struct arpreq.arp_dev], [], [],
                 [#include <net/if_arp.h>])

dnl
dnl Check for 'sival_int' member in 'union sigval'.
dnl
AC_CHECK_MEMBERS([union sigval.sival_int], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'sigval_int' member in 'union sigval'.
dnl
AC_CHECK_MEMBERS([union sigval.sigval_int], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'sival_ptr' member in 'union sigval'.
dnl
AC_CHECK_MEMBERS([union sigval.sival_ptr], [], [],
                 [#include <signal.h>])

dnl
dnl Check for 'sigval_ptr' member in 'union sigval'.
dnl
AC_CHECK_MEMBERS([union sigval.sigval_ptr], [], [],
                 [#include <signal.h>])

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
dnl
AC_SEARCH_LIBS([pthread_create], [pthread kse c_r], [],
               AC_MSG_ERROR([No POSIX threads]))
AC_SEARCH_LIBS([pthread_atfork], [pthread kse c_r], [],
               AC_MSG_ERROR([pthread_atfork is not supported]))

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

AC_CHECK_HEADERS([sys/utsname.h])
AC_CHECK_HEADERS([sys/systeminfo.h])
AC_CHECK_HEADERS([sys/klog.h])
AC_CHECK_HEADERS([linux/if_vlan.h])

dnl
dnl Terminal devices
dnl
AC_CHECK_HEADERS([termios.h])

dnl
dnl Parport devices
dnl
AC_CHECK_HEADERS([linux/ppdev.h])

dnl
dnl We do not need epoll really, since we use it via RPC and dlopen,
dnl but we do need the header for struct epoll_event.
dnl
AC_CHECK_TYPES([struct epoll_event], [], [],
               [#include <sys/epoll.h>])


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

if test -n "$with_rcf_rpc" ; then
    dnl
    dnl FreeBSD 4.x has no svc_exit().
    dnl NetBSD has no svcunix_create().
    dnl
    AC_CHECK_FUNCS([svcunix_create svc_exit])

    dnl
    dnl Find library with SunRPC XDR functions.
    dnl -lnsl on Solaris2 (SunOS 5.11);
    dnl
    AC_SEARCH_LIBS([xdr_bytes], [rpc nsl tirpc], [],
                   AC_MSG_ERROR([No RPC library found]))

    AH_TEMPLATE([xdr_uint8_t], [RPC XDR for unsigned 8bit width type])
    AC_CHECK_FUNC([xdr_uint8_t], [],
        [AC_CHECK_FUNC([xdr_u_int8_t],
            [AC_DEFINE([xdr_uint8_t], [xdr_u_int8_t])],
            [AC_CHECK_FUNC([xdr_u_char],
                [AC_DEFINE([xdr_uint8_t], [xdr_u_char])],
                [AC_MSG_ERROR([There is no xdr_uint8_t or xdr_u_int8_t!])]
             )])])

    AH_TEMPLATE([xdr_uint16_t], [RPC XDR for unsigned 8bit width type])
    AC_CHECK_FUNC([xdr_uint16_t], [],
        [AC_CHECK_FUNC([xdr_u_int16_t],
            [AC_DEFINE([xdr_uint16_t], [xdr_u_int16_t])],
            [AC_MSG_ERROR([There is no xdr_uint16_t or xdr_u_int16_t!])])])

    AH_TEMPLATE([xdr_uint32_t], [RPC XDR for unsigned 32bit width type])
    AC_CHECK_FUNC([xdr_uint32_t], [],
        [AC_CHECK_FUNC([xdr_u_int32_t],
            [AC_DEFINE([xdr_uint32_t], [xdr_u_int32_t])],
            [AC_MSG_ERROR([There is no xdr_uint32_t or xdr_u_int32_t!])])])

    AH_TEMPLATE([xdr_uint64_t], [RPC XDR for unsigned 64bit width type])
    AC_CHECK_FUNC([xdr_uint64_t], [],
        [AC_CHECK_FUNC([xdr_u_int64_t],
            [AC_DEFINE([xdr_uint64_t], [xdr_u_int64_t])],
            [AC_MSG_ERROR([There is no xdr_uint64_t or xdr_u_int64_t!])])])
fi

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

        pppoe)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_PPPOE_SERVER" ;
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
AM_CONDITIONAL(CFG_UNIX_DAEMONS_PPPOE_SERVER,
    [expr "$with_cfg_unix_daemons" : ".*pppoe" >/dev/null])
AM_CONDITIONAL(CFG_UNIX_DAEMONS_VTUND, 
    [expr "$with_cfg_unix_daemons" : ".*vtund" >/dev/null])
AM_CONDITIONAL([CFG_UNIX_DAEMONS_RADIUS],
    [expr "$with_cfg_unix_daemons" : ".*radius" >/dev/null])

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
    # The rest should use IOCTL interface for now
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DUSE_IOCTL -DUSE_ROUTE_SOCKET"
fi

AC_ARG_ENABLE([vcm],
              AS_HELP_STRING([--enable-vcm],
                             [enable VCM mgmt support (default is NO)]),
              [ac_cv_enable_vcm=$enableval], [ac_cv_enable_vcm=no])

AM_CONDITIONAL(CFG_VCM, test -n "$ac_cv_enable_vcm")
if test -n "$ac_cv_enable_vcm"; then
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

AC_SUBST([EXTRA_CPPFLAGS])
AC_SUBST([EXTRA_LDADD])

DYNAMIC_LIBS="-ldl -lpthread -lnsl -lsendfile -lsocket"
if test x${PCAP_AVAIL} = "xyes" ; then
    DYNAMIC_LIBS="${DYNAMIC_LIBS} -lpcap"
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
       echo D_LIBS ${D_LIBS} 
       LIBS="${S_LIBS} -Wl,-Bdynamic ${D_LIBS}" ;;
esac

AC_ARG_WITH([aggr], , [with_aggr=yes], [])
AM_CONDITIONAL(CFG_UNIX_AGGR, test "x$with_aggr" != "x")
if test "x$with_aggr" != "x" ; then
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DWITH_AGGREGATION"
fi

#
# On PowerPC function descriptors are used which are
# placed in data section like variables, not in
# text one - so we cannot distinguish functions
# and variables with --format=bsd in nm output
#
# Please note that the following code is a replace for
# "${host:0:9}", but this expression is BASH specific
# and won't work for DASH, which is why we do this
# in not elegant, but portable way.
#
host_prefix=`echo $host | sed -e 's/\(.........\).*/\1/'`
if test "x$host_prefix" = "xpowerpc64" || \
   test "x$host_prefix" = "xpowerpc"; then
    NM_FORMAT=sysv
else
    NM_FORMAT=bsd
fi
AC_SUBST([NM_FORMAT])

dnl end of "conf" staff

AC_CONFIG_FILES([Makefile rpc/Makefile conf/Makefile])

TE_APP_RESTORE

AC_OUTPUT

