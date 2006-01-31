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

TE_PLAIN_SET

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

AC_CHECK_HEADERS([sys/types.h sys/queue.h])

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
netinet/in.h \
netinet/in_systm.h \
netinet/ip.h \
netinet/ip_icmp.h \
netinet/tcp.h \
obstack.h \
poll.h \
popt.h \
pthread.h \
pwd.h \
regex.h \
rpc/pmap_clnt.h \
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
sys/errno.h \
sys/ioctl.h \
sys/poll.h \
sys/resource.h \
sys/select.h \
sys/sendfile.h \
sys/stat.h \
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
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
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

TE_PLAIN_RESTORE

AC_OUTPUT
