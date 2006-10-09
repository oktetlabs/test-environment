#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([include], [1.0], [Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([te_defs.h])

AC_CONFIG_AUX_DIR([../auxdir])
AC_CONFIG_MACRO_DIR([../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

TE_LIB_SET

AC_CONFIG_HEADERS([te_config_internal.h])
AH_TOP([#ifndef __TE_CONFIG_INTERNAL_H__
#error te_config_internal.h cannot be included directly
#else
])
AH_BOTTOM([
#endif])

AC_HEADER_STDC

if test x$host != x${host%windows} ; then

# Native Windows compiler

# In fact, it's simple to check this dynamically, but it is VERY long

if test $host != ${host#x86_64} ; then

SIZEOF_SIZE_T=8
SIZEOF_VOID_P=8

else

SIZEOF_SIZE_T=4
SIZEOF_VOID_P=4

fi

AC_CONFIG_FILES([te_win.h])

AC_SUBST(SIZEOF_SIZE_T)
AC_SUBST(SIZEOF_VOID_P)

else

# Standard platforms

dnl
dnl At first check headers used by other headers to be checked
dnl 

AC_CHECK_HEADERS([sys/types.h sys/queue.h sys/param.h])

AC_CHECK_HEADERS([sys/socket.h], [], [], [
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])

AC_CHECK_HEADERS([net/if.h], [], [], [
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
])

AC_CHECK_HEADERS([netinet/in.h], [], [], [
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

AC_CHECK_HEADERS([\
aio.h \
arpa/inet.h \
assert.h \
ctype.h \
dirent.h \
dlfcn.h \
errno.h \
expect.h \
fcntl.h \
glob.h \
inet/mib2.h \
inet/nd.h \
inttypes.h \
limits.h \
math.h \
net/ethernet.h \
net/if_arp.h \
net/if_dl.h \
net/if_ether.h \
net/route.h \
netdb.h \
netinet/ether.h \
netinet/if_ether.h \
netinet/in_systm.h \
netinet/ip.h \
netinet/ip_icmp.h \
netinet/udp.h \
netinet/tcp.h \
netpacket/packet.h \
obstack.h \
pcap.h \
poll.h \
popt.h \
pthread.h \
pwd.h \
regex.h \
rpc/types.h \
rpc/xdr.h \
rpc/auth.h \
rpc/clnt.h \
rpc/pmap_clnt.h \
scsi/sg.h \
semaphore.h \
setjmp.h \
signal.h \
stdarg.h \
stddef.h \
stdint.h \
stdio.h \
stdlib.h \
string.h \
strings.h \
stropts.h \
sys/dlpi.h \        
sys/errno.h \
sys/ethernet.h \
sys/filio.h \
sys/ioctl.h \
sys/poll.h \
sys/resource.h \
sys/select.h \
sys/sendfile.h \
sys/sockio.h \
sys/stat.h \
sys/sysctl.h \
sys/tihdr.h \
sys/time.h \
sys/uio.h \
sys/un.h \
sys/wait.h \
tcl.h \
tcl8.4/expect.h \
termio.h \
time.h \
ucd-snmp/snmp_impl.h \
unistd.h \
], [], [], [
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#if HAVE_SYS_PARAM_H
/* NetBSD: sys/sysctl.h */
#include <sys/param.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NETINET_IN_H
/* NetBSD: netinet/ip.h, netinet/ip_icmp.h  */
#include <netinet/in.h>
#endif
#if HAVE_NETINET_IN_SYSTM_H
/* NetBSD: netinet/ip.h, netinet/ip_icmp.h */
#include <netinet/in_systm.h>
#endif
#if HAVE_NETINET_IP_H
/* NetBSD: netinet/ip_icmp.h */
#include <netinet/ip.h>
#endif
#if HAVE_RPC_TYPES_H
/* NetBSD: rpc/pmap_clnt.h */
#include <rpc/types.h>
#endif
#if HAVE_RPC_XDR_H
/* NetBSD: rpc/pmap_clnt.h */
#include <rpc/xdr.h>
#endif
#if HAVE_RPC_AUTH_H
/* NetBSD: rpc/clnt.h */
#include <rpc/auth.h>
#endif
#if HAVE_RPC_CLNT_H
/* NetBSD: rpc/pmap_clnt.h */
#include <rpc/clnt.h>
#endif
])

dnl
dnl Check for Linux kernel interfaces
dnl
AC_CHECK_HEADERS([linux/ethtool.h], [], [], [
/* linux/ethtool.h */
#include <sys/types.h>
typedef unsigned long long u64;
typedef __uint32_t u32;
typedef __uint16_t u16;
typedef __uint8_t  u8;
])

dnl
dnl Check for 'sa_len'-like members in 'struct sockaddr'-like structures.
dnl
AC_CHECK_MEMBERS([struct sockaddr.sa_len], [], [], [
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

AC_CHECK_HEADERS([net-snmp/net-snmp-config.h])
AC_CHECK_HEADERS([\
net-snmp/definitions.h \
net-snmp/session_api.h \
], [], [], [
#if HAVE_NET_SNMP_NET_SNMP_CONFIG_H
#include <net-snmp/net-snmp-config.h>
#endif
])

AC_CHECK_SIZEOF([void *])
AC_CHECK_SIZEOF([char])
AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([size_t], [], [
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])
AC_CHECK_SIZEOF([socklen_t], [], [
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

CPPFLAGS="-I${srcdir} ${CPPFLAGS}"

AC_CHECK_SIZEOF([te_log_nfl],     [], [#include "te_raw_log.h"])
AC_CHECK_SIZEOF([te_log_version], [], [#include "te_raw_log.h"])
AC_CHECK_SIZEOF([te_log_ts_sec],  [], [#include "te_raw_log.h"])
AC_CHECK_SIZEOF([te_log_ts_usec], [], [#include "te_raw_log.h"])
AC_CHECK_SIZEOF([te_log_level],   [], [#include "te_raw_log.h"])
AC_CHECK_SIZEOF([te_log_id],      [], [#include "te_raw_log.h"])
AC_CHECK_SIZEOF([te_log_seqno],   [], [#include "te_raw_log.h"])

AC_C_INLINE

fi

AM_CONDITIONAL(WINDOWS, test x$host != x${host%windows})

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
