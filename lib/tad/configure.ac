# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT([tad_ch], 0.1, [konst@oktetlabs.ru])

AC_CONFIG_SRCDIR([tad_ch.c])
AC_CONFIG_AUX_DIR([../../aux])

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

TAD_LIBADD=
TAD_LIBLIST=
TAD_SUBDIRS=
TAD_CH_DEFS=

proto_specific()
{
    VAR=`echo $1 | awk '{ print toupper($1);}'`
    TAD_LIBADD="$TAD_LIBADD ADDLIB $1/libtad_$1.a\n"
    TAD_LIBLIST="$TAD_LIBLIST $1/libtad_$1.a"
    TAD_SUBDIRS="$TAD_SUBDIRS $1"
    TAD_CH_DEFS="$TAD_CH_DEFS -DWITH_$VAR" 
}


AC_ARG_WITH(eth,, 
[if test "${withval}" = yes; then
   w_eth=true
   proto_specific eth;
 fi
], [w_eth=false])
    
AC_ARG_WITH(bridge,, 
[if test "${withval}" = yes; then
   proto_specific bridge;
 fi
], [w_bridge=false])

AC_ARG_WITH(ipstack,, 
[if test "${withval}" = yes; then
   w_ipstack=true
   proto_specific ipstack;
 fi
], [w_ipstack=false])


AC_ARG_WITH(dhcp,, 
[if test "${withval}" = yes; then
   proto_specific dhcp;
 fi
], [w_dhcp=false])

AC_ARG_WITH(snmp,, 
[if test "${withval}" = yes; then
    w_snmp=true
    proto_specific snmp;
 fi
], [w_snmp=false])


AC_ARG_WITH(file,, 
[if test "${withval}" = yes; then
   proto_specific file;
 fi
], [w_file=false])

AC_ARG_WITH(cli,, 
[if test "${withval}" = yes; then
   w_cli=true
   proto_specific cli;
 fi
], [w_cli=false])

if test x$w_snmp = xtrue; then 
    AC_MSG_CHECKING(for net-snmp library)
    AC_CHECK_HEADERS([net-snmp/session_api.h], , ,
    [#include <net-snmp/net-snmp-config.h>]
    )
    AC_PATH_PROG(NET_SNMP_CONFIG, net-snmp-config, no)
    if test "$NET_SNMP_CONFIG" = "no" ; then
        AC_MSG_ERROR([net-snmp-config is required but not found])
    else
        NET_SNMP_CFLAGS=`$NET_SNMP_CONFIG --cflags`
    fi
fi

if test x$w_cli = xtrue; then 
    AC_CHECK_HEADERS([expect.h tcl8.4/expect.h])
fi

if test x$w_eth = xtrue; then 
    AC_CHECK_HEADERS([netinet/ether.h])
fi

AC_SUBST(NET_SNMP_CFLAGS)
AC_SUBST(NET_SNMP_LIBS)
   
AC_C_INLINE

AC_STDC_HEADERS

AM_CONFIG_HEADER([config.h])

AC_SUBST(TAD_LIBADD)
AC_SUBST(TAD_LIBLIST)
AC_SUBST(TAD_SUBDIRS)
AC_SUBST(TAD_CH_DEFS)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h \
                  string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h\
                  semaphore.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset select socket strdup strtol])

AC_CONFIG_FILES([Makefile
                 bridge/Makefile
                 cli/Makefile
                 dhcp/Makefile
                 eth/Makefile
                 file/Makefile
                 ipstack/Makefile
                 snmp/Makefile])


AC_OUTPUT

