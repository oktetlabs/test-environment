#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([include], [1.0], [Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([te_defs.h])

AC_CONFIG_AUX_DIR([../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST([TE_CFLAGS])
AC_ARG_VAR([TE_CFLAGS])

AC_CONFIG_HEADERS([te_config_internal.h])
AH_TOP([#ifndef __TE_CONFIG_INTERNAL_H__
#error te_config_internal.h cannot be included directly
#else
])
AH_BOTTOM([
#endif])

AC_HEADER_STDC
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([\
aio.h \
arpa/inet.h \
assert.h \
ctype.h \
dirent.h \
errno.h \
expect.h \
fcntl.h \
inttypes.h \
limits.h \
net-snmp/definitions.h \
net-snmp/session_api.h \
net/ethernet.h \
net/if.h \
net/if_arp.h \
net/if_dl.h \
net/route.h \
netdb.h \
netinet/ether.h \
netinet/in.h \
netinet/in_systm.h \
netinet/ip.h \
netinet/tcp.h \
obstack.h \
poll.h \
popt.h \
pthread.h \
pwd.h \
regex.h \
rpc/pmap_clnt.h \
semaphore.h \
signal.h \
stdarg.h \
stdint.h \
stdio.h \
stdlib.h \
string.h \
strings.h \
sys/errno.h \
sys/ioctl.h \
sys/poll.h \
sys/queue.h \
sys/resource.h \
sys/select.h \
sys/sendfile.h \
sys/socket.h \
sys/stat.h \
sys/time.h \
sys/un.h \
sys/wait.h \
tcl.h \
tcl8.4/expect.h \
termio.h \
time.h \
ucd-snmp/snmp_impl.h \
unistd.h \
], [], [],
[#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/socket.h>
#endif
])

AC_C_INLINE

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
