# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])

AC_INIT([tad],[0.8],[Andrew.Rybchenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([tad_ch.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

tad_libs_rem()
{
# It is used to delete possible duplicate to avoid possible craziness of
# pkg-config used in agent/unix/configure
    STR_ESCAPED=`echo $1 | sed "s/\//@/g"`
    LIBS_ESCAPED=`echo $TA_TAD_LIBS | sed "s/\//@/g"`
    if test -n "$STR_ESCAPED" ; then
        TA_TAD_LIBS=`echo $LIBS_ESCAPED | sed "s/$STR_ESCAPED//g"`
    fi
    TA_TAD_LIBS=`echo $TA_TAD_LIBS | sed "s/@/\//g"`
}

# FIXME: make it AC macro
find_lib()
{
LIB_NAME=$1
LIB_PATHNAME=
FILES=
START=`pwd`
DIRS="`${CC} ${TE_CPPFLAGS} ${TE_CFLAGS} --print-search-dirs | \
      grep libraries | sed s/libraries// | sed s/=//g | sed s/:/\ /g`"
DIRS="${TE_EXT_LIBS_PATH} ${DIRS} `echo $LD_LIBRARY_PATH | sed s/:/\ /g`"
for k in $DIRS ; do
    if cd $k 2>/dev/null ; then
        DIR=`pwd`

# FIXME: It is VERY dirty hack for solaris_64 platform
# The reason is '${CC} --print-search-dirs' gives wrong dirs (32-bit ones)
        if test "$TE_PLATFORM" = solaris_64; then
            if test -d amd64; then
                cd amd64
                DIR=`pwd`
            else
                continue
            fi
        fi

        tmp=`find -L $DIR -maxdepth 1 -name lib${LIB_NAME}\*.a 2>/dev/null`
        for j in $tmp ; do
            name=`basename $j`;
            #
            # The following lines are portable way of getting the result
            # of command: "suffix=${name/lib${LIB_NAME}/};"
            # but this command does not work in DASH, but only in BASH.
            # 
            cmd="echo $name | sed -e 's/^\\(lib${LIB_NAME}\\)\\(.*\\)/\\2/'"
            suffix=`eval $cmd`

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

for k in $FILES ; do
    LIB_PATHNAME=$k ;
    return ;
done
}

##
# Find and add a specified library in the variable @b TA_TAD_LIBS
#
# @param lib Specified library
#
add_lib_in_TA_TAD_LIBS() {
    local lib=$1

    # In case dynamic libc linking we add simple '-l${lib}', in case static
    # linking we find absolute path to 'lib${lib}.a' by function 'find_lib'.
    # Then 'TA_TAD_LIBS' libraries will be added to the variable 'LIBS'
    # with options '-Wl,-Bstatic' and '-Wl,-Bdynamic'.
    # There is example in the file 'agents/unix/configure.ac'.
    if test "${with_static_libc}" = "no" ; then
        LIB_PATHNAME="-l${lib}"
    else
        find_lib "${lib}"
        if test -z "${LIB_PATHNAME}" ; then
            AC_MSG_ERROR([Cannot find static $lib library])
        fi
    fi
    tad_libs_rem "$LIB_PATHNAME"
    TA_TAD_LIBS="$TA_TAD_LIBS ${LIB_PATHNAME}";
}

# Check for C compiler features
AC_C_CONST
AC_C_INLINE

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
# It is a good idea to check fork, but it's not used and generates a
# warning at configure time.
#AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset select socket strdup strtol])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT

AC_ARG_WITH([name],,)

AC_SUBST([with_name])

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



AC_ARG_WITH([atm],, 
[if test "${withval}" = yes; then
   w_atm=true;
   proto_specific atm;
   AC_CONFIG_FILES([atm/Makefile])
 fi
], [w_atm=false])

AC_CHECK_LIB([pcap], [pcap_open_live], [PCAP_LIBS=-lpcap])

AC_ARG_WITH([eth],, 
[if test "${withval}" = yes; then
   w_eth=true;
   proto_specific eth;
   tad_libs_rem "$PCAP_LIBS"
   TA_TAD_LIBS="$TA_TAD_LIBS ${PCAP_LIBS}"
   AC_CONFIG_FILES([eth/Makefile])
 fi
], [w_eth=false])
    
AC_ARG_WITH([bridge],, 
[if test "${withval}" = yes; then
   proto_specific bridge;
   AC_CONFIG_FILES([bridge/Makefile])
 fi
], [w_bridge=false])

AC_ARG_WITH([arp],, 
[if test "${withval}" = yes; then
   w_arp=true;
   proto_specific arp;
   AC_CONFIG_FILES([arp/Makefile])
 fi
], [w_arp=false])
    
AC_ARG_WITH([frame],, 
[if test "${withval}" = yes; then
   w_frame=true;
   proto_specific frame;
   AC_CONFIG_FILES([frame/Makefile])
 fi
], [w_frame=false])

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

AC_ARG_WITH([igmp],, 
[if test "${withval}" = yes; then
   proto_specific igmp;
   AC_CONFIG_FILES([igmp/Makefile])
 fi
], [w_igmp=false])

AC_ARG_WITH([ppp],, 
[if test "${withval}" = yes; then
   proto_specific ppp;
   AC_CONFIG_FILES([ppp/Makefile])
 fi
], [w_ppp=false])


AC_ARG_WITH([snmp],, 
[if test "${withval}" = yes; then
    w_snmp=true
    proto_specific snmp

    TE_BINUTIL([net-snmp-config], [NET_SNMP_CONFIG_FULL_PATH])

    for i in `$NET_SNMP_CONFIG_FULL_PATH --libs`; do
        # Please note that the following code is a replace for
        # "${i:0:2}", but this expression is BASH specific
        # and won't work for DASH, which is why we do this
        # in not elegant, but portable way.
        #
        i_prefix=`echo $i | sed -e 's/\(..\).*/\1/'`

        if test "x$i_prefix" = "x-l"; then
            # The following code is a replace for "${i:2}"
            lib_name=`echo $i | sed -e 's/\(..\)\(.*\)/\2/'`

            find_lib ${lib_name}
            if test -z "${LIB_PATHNAME}" ; then
                AC_MSG_WARN([Cannot find ${lib_name} library])
            fi
            tad_libs_rem "$LIB_PATHNAME"
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
    LIB_PATHNAME=\
            $NET_SNMP_CONFIG_FULL_PATH --libs | grep crypto >/dev/null || \
            find_lib crypto

    if test -n "${LIB_PATHNAME}" ; then
        tad_libs_rem "$LIB_PATHNAME"
        TA_TAD_LIBS="$TA_TAD_LIBS ${LIB_PATHNAME}"

        if test -n "${LIB_PATHNAME}" ; then
            CRYPTO_LIB=${LIB_PATHNAME}
            find_lib z

            #
            # libcrypto may require libz to be added at linking state,
            # which is why we need to check if it is necessary or not.
            # Here we use trick:
            # - We know that crypto library has COMP_zlib() function
            #   that depends on some symbols from 'libz', which is why
            #   we try to check for this function compilation;
            # - AC_CHECK_LIB() macro uses library name as the first
            #   argument and it will prepend it with "-l" on doing trial
            #   linking test. In our case we use full path to .a files,
            #   so we can't use 'crypto' as the first agrument;
            # - We use 'c' name for ('libc' library) as fake library
            #   which is linked with any application by default anyway;
            # - We first try to mount with ${CRYPTO_LIB} specifying it
            #   in the list of extra libraries (there we are free to
            #   specify anything - it is transparently passed to the linker
            #   command line);
            # - If that linking test passes it means we do not need libz
            #   and as the result do nothing, but otherwise we re-run
            #   that linking test again, but already with two libraries
            #   dpecified in extra libraries list: ${CRYPTO_LIB} and
            #   ${LIB_PATHNAME}. Here ${LIB_PATHNAME} is the path to just
            #   found 'z' library for which we called "find_lib z";
            # - This time we use 'm' library name ('libm' library) in
            #   AC_CHECK_LIB() macro because autoconf caches previous
            #   checks and if we used 'c' name again it would got
            #   the result from cache and the verdict would be "no".
            #   It does not matter for autoconf that the list of extra
            #   libraries has changed - it put attention to only 'library'
            #   and 'function' arguments.
            #
            AC_CHECK_LIB([c], [COMP_zlib], [], [
                AC_CHECK_LIB([m], [COMP_zlib], [
                      tad_libs_rem "$LIB_PATHNAME"
                      TA_TAD_LIBS="$TA_TAD_LIBS ${LIB_PATHNAME}"],
                    [AC_MSG_WARN([crypto library does not have COMP_zlib \
                                  or we can not link with it!])],
                    [${CRYPTO_LIB} ${LIB_PATHNAME}])],
                         [${CRYPTO_LIB}])
        fi
    fi

    AC_CONFIG_FILES([snmp/Makefile])
 fi
], [w_snmp=false])

AC_ARG_WITH([static_libc],
            AS_HELP_STRING([--with-static-libc],
                           [enable static libc (default is YES)]))

AC_ARG_WITH([cli],, 
[if test "${withval}" = yes; then
    w_cli=true;
    proto_specific cli;
    find_lib expect
    if test -z "${LIB_PATHNAME}" ; then
        AC_MSG_ERROR([Cannot find static expect library])
    fi
    tad_libs_rem "$LIB_PATHNAME"
    TA_TAD_LIBS="$TA_TAD_LIBS ${LIB_PATHNAME}";

    add_lib_in_TA_TAD_LIBS util

# tcl and m libraries are needed for expect library
# and they don't link automatically by some linkers
    find_lib tcl
    if test -z "${LIB_PATHNAME}" ; then
        AC_MSG_ERROR([Cannot find static tcl library])
    fi
    tad_libs_rem "$LIB_PATHNAME"
    TA_TAD_LIBS="$TA_TAD_LIBS ${LIB_PATHNAME}";

    add_lib_in_TA_TAD_LIBS m

    AC_CONFIG_FILES([cli/Makefile])
 fi
], [w_cli=false])

AC_ARG_WITH([pcap],, 
[if test "${withval}" = yes; then
    w_pcap=true;
    proto_specific pcap;
    tad_libs_rem "$PCAP_LIBS"
    TA_TAD_LIBS="$TA_TAD_LIBS ${PCAP_LIBS}";
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

AC_ARG_WITH([socket],, 
[if test "${withval}" = yes; then
   proto_specific socket;
   AC_CONFIG_FILES([socket/Makefile])
 fi
], [w_socket=false])

if test $w_eth = true -o $w_atm = true ; then
   proto_specific llc;
   AC_CONFIG_FILES([llc/Makefile])
fi

AC_ARG_WITH([forw],,
[if test "${withval}" = yes; then
   w_forw=true;
   proto_specific forw;
   AC_CONFIG_FILES([forw/Makefile])
 fi
], [w_forw=false])

TA_TAD_LIBS=`${TE_BASE}/engine/builder/static_libs_check.pl \
             "${TA_TAD_LIBS}"`

AC_SUBST([TAD_LIBLIST])
AC_SUBST([TAD_SUBDIRS])
AC_SUBST([TAD_CH_DEFS])
AC_SUBST([TA_TAD_LIBS])

AM_CONDITIONAL(TAD_DUMMY, test -z "$TAD_LIBLIST")

if test x$w_snmp = xtrue; then 
    AC_MSG_CHECKING(for net-snmp library)
    AC_CHECK_HEADERS([net-snmp/session_api.h], , ,
    [#include <net-snmp/net-snmp-config.h>]
    )

    if test -z "$NET_SNMP_CONFIG_FULL_PATH"; then
        AC_MSG_ERROR([Impossible happens, we should have found \
net-snmp-config earlier])
    fi

    NET_SNMP_CFLAGS=`$NET_SNMP_CONFIG_FULL_PATH --cflags`
fi

AC_SUBST([NET_SNMP_CFLAGS])
AC_SUBST([NET_SNMP_LIBS])

AC_CHECK_HEADERS([sys/bufmod.h])

TE_LIB_RESTORE

AC_OUTPUT
