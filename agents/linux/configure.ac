#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(ta-linux, 0.4, Elena.Vengerova@oktetlabs.ru)
AC_CONFIG_SRCDIR([linux.c])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign])

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_CHECK_TOOL(NM, nm)


AC_SUBST(host)
AC_SUBST(with_name)
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

AC_CHECK_HEADERS(time.h sys/time.h sys/resource.h semaphore.h aio.h \
                 net/ethernet.h net/if_dl.h netinet/in_systm.h sys/un.h)

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


#
# Check for 'sa_len'-like members in 'struct sockaddr'-like structures.
#
AC_CHECK_MEMBERS([struct sockaddr.sa_len], [], [],
                 [#include <sys/types.h>
                  #include <sys/socket.h>])

#
# POSIX threads are provided by:
# - -lpthread in Linux;
# - -lc_r in FreeBSD;
# - -lpthread in NetBSD ('pth' package).
#
AC_SEARCH_LIBS([pthread_create], [pthread c_r], [],
               AC_MSG_ERROR([No POSIX threads]))

#
# POSIX semaphores are provided by:
# - -lpthread in Linux;
# - -lc_r in FreeBSD;
# - -lsemaphore in NetNSD ('pthread-sem' package).
#
AC_SEARCH_LIBS([sem_init], [pthread c_r semaphore], [],
               AC_MSG_ERROR([No POSIX semaphores]))


if test -n "$with_rcf_rpc" ; then
    #
    # FreeBSD 4.x has no svc_exit().
    # NetBSD has no svcunix_create().
    #
    AC_CHECK_FUNCS([svcunix_create svc_exit])

    #
    # Asynchronous I/O:
    # - FreeBSD has in -lc;
    # - Linux has in -lnsl -lrt;
    # - NetBSD does not have.
    #
    AC_CHECK_FUNC([aio_read], [],
                  AC_CHECK_LIB([nsl], [aio_read], [LIBS="-lnsl -lrt ${LIBS}"],
                               AC_MSG_WARN([Asynchronous I/O not supported]),
                               [-lrt]))
fi


AC_CONFIG_FILES(Makefile)

AC_OUTPUT
