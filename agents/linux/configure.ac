#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(ta-linux, 0.3, Elena.Vengerova@oktetlabs.ru)
AC_CONFIG_SRCDIR([linux.c])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_CHECK_TOOL(NM, nm)

AC_CANONICAL_HOST

AC_SUBST(host)
AC_SUBST(TE_CPPFLAGS)
AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)
AC_SUBST(TE_LDADD)
AC_SUBST(TE_DEPENDENCIES)

AC_ARG_VAR(TE_CPPFLAGS)
AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)
AC_ARG_VAR(TE_LDADD)
AC_ARG_VAR(TE_DEPENDENCIES)

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AM_CONFIG_HEADER(config.h)

AC_CHECK_HEADERS(time.h sys/time.h sys/resource.h semaphore.h \
                 net/if_dl.h netinet/in_systm.h)
AC_C_INLINE

if test -n "$with_cfg_linux_daemons" ; then
    DAEMONS_CFLAGS=-DCFG_LINUX_DAEMONS
    for i in $with_cfg_linux_daemons ; do
        case $i in 
            tftp)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TFTP_SERVER" ;;
            ftp)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_FTP_SERVER" ;;
            dns)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DNS_SERVER" ;;
            echo)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_ECHO_SERVER" ;;
            todudp)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TODUDP_SERVER" ;;
            dhcp)  DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DHCP_SERVER" ;
                   DAEMONS_LDADD="-ldhcpctl -lomapi -ldst" ;;
            *)  ;; 
        esac
    done
fi
AC_SUBST(DAEMONS_CFLAGS)
AC_SUBST(DAEMONS_LDADD)

AM_CONDITIONAL(CFG_LINUX_DAEMONS, test -n "$with_cfg_linux_daemons")
AM_CONDITIONAL(CFG_RCF_RPC, test -n "$with_rcf_rpc")

AC_LIBLTDL_INSTALLABLE
# Substitute INCLTDL and LIBLTDL in the Makefiles
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)
# Check for dlopen support
AC_LIBTOOL_DLOPEN
# Configure libtool
AC_PROG_LIBTOOL
# Configure libltdl
enable_ltdl_install=yes
AC_CONFIG_SUBDIRS(libltdl)

#
# Check for 'sa_len'-like members in 'struct sockaddr'-like structures.
#
AC_CHECK_MEMBERS([struct sockaddr.sa_len], [], [],
                 [#include <sys/types.h>
                  #include <sys/socket.h>])

#
# POSIX threads are provided by:
# - -lpthread in Linux
# - -lc_r in FreeBSD
#
AC_CHECK_LIB([pthread], [pthread_create], [PTHREAD_LDADD=-lpthread],
             AC_CHECK_LIB([c_r], [pthread_create], [PTHREAD_LDADD=-lc_r]))
AC_SUBST(PTHREAD_LDADD)

#
# Asynchronous I/O:
# - FreeBSD has in -lc
# - Linux has in -lnsl -lrt
#
AC_CHECK_FUNC([aio_read], [],
              AC_CHECK_LIB([nsl], [aio_read], [AIO_LDADD="-lnsl -lrt"],
                           AC_MSG_ERROR([Asynchronous I/O not supported]),
                           [-lrt]))
AC_SUBST(AIO_LDADD)

#
# svc_exit() is provided by -lc in Linux and by -lc_r in FreeBSD
#
AC_CHECK_LIB([c_r], [svc_exit])
AC_CHECK_FUNCS([svc_exit])


AC_ARG_WITH(bsd, [], [TATYPE=freebsd], [TATYPE=linux])
AM_CONDITIONAL(TALINUX, test x$TATYPE = xlinux)


AC_CONFIG_FILES(Makefile)

AC_OUTPUT
