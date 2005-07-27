# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([tad],[0.1],[konst@oktetlabs.ru])
    
AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_SRCDIR([tad_ch.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

# FIXME: make it AC macro
find_lib()
{
LIB_PATHNAME=
START=`pwd`
DIRS="`${CC} --print-search-dirs | grep libraries | \
      sed s/libraries// | sed s/=//g | sed s/:/\ /g`"
DIRS="$DIRS `echo $LD_LIBRARY_PATH | sed s/:/\ /g`"
for i in $DIRS ; do
    if cd $i 2>/dev/null ; then
        DIR=`pwd`

        for j in `find $DIR -name lib$1\*.a` ; do
            name=`basename $j`;
            suffix=${name/lib$1/};
            if test "$suffix" = ".a" ||
               test -z `echo $suffix | sed -s "s,^[[0-9._-]]*.a$,,"`; then
                FILES=$j;
                break 2;
            fi
        done
    fi
    cd $START
done
cd $START

if test -z "$FILES" ; then
    return ;
fi    

for i in $FILES ; do
    LIB_PATHNAME=$i ;
    return ;
done
}


# Check for C compiler features
AC_C_CONST
AC_C_INLINE

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset select socket strdup strtol])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([\
stdio.h \
stdlib.h \
string.h \
strings.h \
unistd.h\
limits.h \
errno.h \
fcntl.h \
sys/time.h \
sys/socket.h \
sys/ioctl.h \
arpa/inet.h \
netinet/in.h \
pthread.h \
semaphore.h \
])


CFLAGS="${CFLAGS} ${TE_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${TE_CFLAGS}"

AC_SUBST([with_name])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_DEPENDENCIES])


TAD_LIBLIST=
TAD_SUBDIRS=
TAD_CH_DEFS=
TA_TAD_LIBS=

proto_specific()
{
    VAR=`echo $1 | awk '{ print toupper($1);}'`
    TAD_LIBLIST="$TAD_LIBLIST $1/libtad_$1.a"
    TAD_SUBDIRS="$TAD_SUBDIRS $1"
    TAD_CH_DEFS="$TAD_CH_DEFS -DWITH_$VAR" 
}



AC_ARG_WITH([eth],, 
[if test "${withval}" = yes; then
   w_eth=true;
   proto_specific eth;
   AC_CONFIG_FILES([eth/Makefile])
 fi
], [w_eth=false])
    
AC_ARG_WITH([bridge],, 
[if test "${withval}" = yes; then
   proto_specific bridge;
   AC_CONFIG_FILES([bridge/Makefile])
 fi
], [w_bridge=false])

AC_ARG_WITH([ipstack],, 
[if test "${withval}" = yes; then
   w_ipstack=true;
   proto_specific ipstack;
   AC_CONFIG_FILES([ipstack/Makefile])
 fi
], [w_ipstack=false])

AC_ARG_WITH([dhcp],, 
[if test "${withval}" = yes; then
   proto_specific dhcp;
   AC_CONFIG_FILES([dhcp/Makefile])
 fi
], [w_dhcp=false])

AC_ARG_WITH([snmp],, 
[if test "${withval}" = yes; then
    w_snmp=true
    proto_specific snmp

    for i in `net-snmp-config --libs`; do
        if test ${i:0:2} = "-l"; then
            find_lib ${i:2}
            if test -z "${LIB_PATHNAME}" ; then
                AC_MSG_WARN([Cannot find ${i:2} library])
            fi
            TA_TAD_LIBS="$TA_TAD_LIBS ${LIB_PATHNAME}"
        fi
    done
#
# On some systems net-snmp-config --libs does not include
# libcrypto library thinking that this is something 
# that should be dynamically linked and there is no need to
# specify it explicitly.
# In our case we want it to be linked statically, so that
# we should take care about this.
#
    LIB_PATHNAME=
    net-snmp-config --libs | grep crypto >/dev/null || find_lib crypto
    if test -n "${LIB_PATHNAME}" ; then
        TA_TAD_LIBS="$TA_TAD_LIBS ${LIB_PATHNAME}"
    fi

    AC_CONFIG_FILES([snmp/Makefile])
 fi
], [w_snmp=false])

AC_ARG_WITH([file],, 
[if test "${withval}" = yes; then
    proto_specific file;
    AC_CONFIG_FILES([file/Makefile])
 fi
], [w_file=false])

AC_ARG_WITH([cli],, 
[if test "${withval}" = yes; then
    w_cli=true;
    proto_specific cli;
    find_lib expect
    if test -z "${LIB_PATHNAME}" ; then
        AC_MSG_ERROR([Cannot find static expect library])
    fi
    TA_TAD_LIBS="$TA_TAD_LIBS ${LIB_PATHNAME}";
    AC_CONFIG_FILES([cli/Makefile])
 fi
], [w_cli=false])

AC_ARG_WITH([pcap],, 
[if test "${withval}" = yes; then
    w_pcap=true;
    proto_specific pcap;
    TA_TAD_LIBS="$TA_TAD_LIBS -lpcap";
    AC_CONFIG_FILES([pcap/Makefile])
 fi
], [w_pcap=false])

AC_ARG_WITH([iscsi],, 
[if test "${withval}" = yes; then
    w_iscsi=true;
    proto_specific iscsi;
    AC_CONFIG_FILES([iscsi/Makefile])
 fi
], [w_iscsi=false])


AC_SUBST([TAD_LIBLIST])
AC_SUBST([TAD_SUBDIRS])
AC_SUBST([TAD_CH_DEFS])
AC_SUBST([TA_TAD_LIBS])

AM_CONDITIONAL(DUMMY_TAD, test -z "$TAD_LIBLIST")

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

AC_SUBST([NET_SNMP_CFLAGS])
AC_SUBST([NET_SNMP_LIBS])


AC_OUTPUT
