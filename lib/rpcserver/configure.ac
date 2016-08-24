#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([rpcserver],[0.01],[Artem.Andreev@oktetlabs.ru])

AC_CONFIG_SRCDIR([rpc_server.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_INSTALL
AC_FIXED_LIBTOOL_RANLIB

TE_LIB_SET

AC_ARG_ENABLE([typesafe-rpc],
              AS_HELP_STRING([--disable-typesafe-rpc],
                             [disable type-safe RPC implementation (default is ENABLE)]),
              [ac_cv_enable_typesafe_rpc=$enableval], [ac_cv_enable_typesafe_rpc=yes])
AC_CACHE_CHECK([whether to enable type-safe RPCs],
               [ac_cv_enable_typesafe_rpc], [ac_cv_enable_typesafe_rpc=yes])

if test x"$ac_cv_enable_typesafe_rpc" != xno
then
dnl
dnl Check that the compiler supports GCC __typeof extension
dnl
    AC_COMPILE_IFELSE([AC_LANG_SOURCE(
                       [[int testfunc(void *ptr);
                         int testfunc2(void *ptr);
                         __typeof(testfunc) *funcptr = testfunc2;
                     ]])],
                     [AC_DEFINE([ENABLE_TYPESAFE_RPC], [], [Whether to enable type-safe RPCs])],
                     [AC_MSG_ERROR([__typeof is not supported])])
fi

AC_ARG_ENABLE([static-rpc-lookup],
              AS_HELP_STRING([--enable-static-rpc-lookup],
                             [use compile-time symbol resolution by default, not run-time (default is run-time)]),
              [ac_cv_enable_static_rpc_lookup=$enableval],
              [ac_cv_enable_static_rpc_lookup=no])
AC_CACHE_CHECK([whether to use compile-time RPC symbol lookup by default],
               [ac_cv_enable_static_rpc_lookup], [ac_cv_enable_static_rpc_lookup=no])
if test x"$ac_cv_enable_static_rpc_lookup" != xno
then
    AC_DEFINE([DEFAULT_STATIC_RPC_LOOKUP], [], [Whether to use static RPC symbol lookup by default])
fi

AC_ARG_ENABLE([plugins],
              AS_HELP_STRING([--disable-plugins],
                             [disable support for idle plugins (enabled by default)]),
              [ac_cv_enable_plugins=$enableval],
              [ac_cv_enable_plugins=yes])
AC_CACHE_CHECK([whether to use idle plugins],
               [ac_cv_enable_plugins], [ac_cv_enable_plugins=yes])
if test x"$ac_cv_enable_plugins" != xno
then
    AC_DEFINE([ENABLE_RPC_PLUGINS], [], [Whether to enable idle plugins])
fi

AC_ARG_ENABLE([mem],
              AS_HELP_STRING([--disable-mem],
                             [disable support for smart memory management (enabled by default)]),
              [ac_cv_enable_mem=$enableval],
              [ac_cv_enable_mem=yes])
AC_CACHE_CHECK([whether to use smart memory management],
               [ac_cv_enable_mem], [ac_cv_enable_mem=yes])
if test x"$ac_cv_enable_mem" != xno
then
    AC_DEFINE([ENABLE_RPC_MEM], [], [Whether to enable smart memory management])
fi


dnl
dnl Check for 'struct ip_mreqn'
dnl
AC_CHECK_TYPES([struct ip_mreqn], [], [],
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
dnl Check for 'tcpi_rcv_rtt' member in 'struct tcp_info'.
dnl
AC_CHECK_MEMBERS([struct tcp_info.tcpi_rcv_rtt], [], [],
                 [#include <netinet/in.h>])


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
dnl Check for 'd_type' member in 'struct dirent'.
dnl
AC_CHECK_MEMBERS([struct dirent.d_type], [], [],
                 [#include <dirent.h>])
dnl
dnl Check for 'd_off' member in 'struct dirent'.
dnl
AC_CHECK_MEMBERS([struct dirent.d_off], [], [],
                 [#include <dirent.h>])
dnl
dnl Check for 'd_offset' member in 'struct dirent'.
dnl
AC_CHECK_MEMBERS([struct dirent.d_offset], [], [],
                 [#include <dirent.h>])
dnl
dnl Check for 'd_namelen' member in 'struct dirent'.
dnl
AC_CHECK_MEMBERS([struct dirent.d_namelen], [], [],
                 [#include <dirent.h>])


dnl
dnl Check for 'pw_gecos' member in 'struct passwd'.
dnl
AC_CHECK_MEMBERS([struct passwd.pw_gecos], [], [],
                 [#include <pwd.h>])

dnl
dnl Check for 'arp_dev' member in 'struct arpreq'.
dnl
AC_CHECK_MEMBERS([struct arpreq.arp_dev], [], [],
                 [#include <net/if_arp.h>])

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

dnl
dnl Check if RTLD_NODELETE constant is defined
dnl (it could be defined either as a macro or
dnl as an element of enumeration).
dnl
AH_TEMPLATE([HAVE_RTLD_NODELETE], [check RTLD_NODELETE constant])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <dlfcn.h>]],
                                    [[dlopen("", RTLD_NODELETE); ]])],
                   [AC_DEFINE(HAVE_RTLD_NODELETE)],[])

AC_CHECK_HEADERS([sys/utsname.h])
AC_CHECK_HEADERS([sys/systeminfo.h])
AC_CHECK_HEADERS([sys/klog.h])
AC_CHECK_HEADERS([linux/if_vlan.h])
AC_CHECK_HEADERS([dirent.h])
AC_CHECK_HEADERS([sys/statvfs.h])
AC_CHECK_HEADERS([sys/queue.h])
AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([search.h])

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
dnl Check for 'struct if_nameindex'
dnl
AC_CHECK_TYPES([struct if_nameindex], [], [],
               [#include <net/if.h>])


dnl
dnl Check for 'struct sg_io_hdr'
dnl
AC_CHECK_HEADERS([scsi/sg.h])
AC_CHECK_HEADERS([cam/scsi/scsi_sg.h])

AC_CHECK_TYPES([struct sg_io_hdr], [], [], [
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SCSI_SG_H
#include <scsi/sg.h>
#endif
#if HAVE_CAM_SCSI_SCSI_SG_H
#include <cam/scsi/scsi_sg.h>
#endif
])

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


dnl
dnl FreeBSD 4.x has no svc_exit().
dnl NetBSD has no svcunix_create().
dnl
AC_CHECK_FUNCS([svcunix_create svc_exit])

dnl
dnl Find library with SunRPC XDR functions.
dnl -lnsl on Solaris2 (SunOS 5.11);
dnl
AC_SEARCH_LIBS([xdr_bytes], [rpc nsl], [],
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

AC_ARG_WITH([tr069],
            AS_HELP_STRING([--with-tr069],
                           [enable TR-069 support (default is NO)]),
            [],
            [with_tr069=no])

AM_CONDITIONAL(CFG_TR069, test x"$with_tr069" != xno)

AS_IF([test x"$with_tr069" != xno],
      [AC_DEFINE([WITH_TR069_SUPPORT], [], \
      [Whether to enable TR-069 support])])

AC_CONFIG_HEADER([rpc_server_config.h])

AC_SUBST([EXTRA_CPPFLAGS])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
