# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT([selftad], [0.1], [Konstantin.Abramenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([configure.ac])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CANONICAL_HOST
AC_SUBST(host)

# Checks for programs.
AC_PROG_CC
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_RANLIB
AC_PROG_INSTALL

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_INSTALL])


AC_CONFIG_HEADER([config.h])

# It's assumed that stdio.h always presents.
# Check for stdlib.h, stdarg.h, string.h and float.h.
AC_HEADER_STDC
AC_CHECK_HEADERS([\
unistd.h \
pthread.h \
semaphore.h \
signal.h \
assert.h \
time.h \
sys/time.h \
sys/types.h \
sys/socket.h \
sys/select.h \
sys/poll.h \
sys/ioctl.h \
sys/sendfile.h \
netinet/in.h \
netinet/ip.h \
netinet/tcp.h \
net/if.h \
arpa/inet.h \
netdb.h \
])

AC_CHECK_HEADERS(ucd-snmp/snmp_impl.h, , ,
#include<sys/types.h>
#include<ucd-snmp/asn1.h>
)

AC_CHECK_HEADERS(net-snmp/definitions.h, , ,
#include<net-snmp/net-snmp-config.h>
)

AC_PATH_PROG(NET_SNMP_CONFIG, net-snmp-config, no)
if test "$NET_SNMP_CONFIG" = "no" ; then
    AC_MSG_ERROR([net-snmp-config is required but not found])
else
    NET_SNMP_CFLAGS=`$NET_SNMP_CONFIG --cflags`
    NET_SNMP_LIBS=`$NET_SNMP_CONFIG --libs`
fi
AC_SUBST([NET_SNMP_CFLAGS])
AC_SUBST([NET_SNMP_LIBS])

AC_DEFINE_UNQUOTED(SUITE_SRCDIR, "$srcdir", [The suite source directory])


AC_CONFIG_FILES([Makefile cli/Makefile \
		snmp/Makefile eth/Makefile ipstack/Makefile])

AC_OUTPUT
