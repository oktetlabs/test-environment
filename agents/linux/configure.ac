dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ta-linux], [0.4], [Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([linux.c])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CC
AC_C_INLINE

AC_PROG_INSTALL

AC_CHECK_TOOL(NM, nm)


AC_SUBST([host])
AC_SUBST([with_name])
AC_SUBST([TE_CPPFLAGS])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CPPFLAGS])
AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}


AC_CONFIG_HEADERS([config.h])

AC_CHECK_HEADERS([\
sys/types.h \
sys/socket.h \
time.h \
sys/time.h \
sys/resource.h \
semaphore.h \
aio.h \
net/ethernet.h \
net/if_dl.h \
netinet/in_systm.h \
sys/un.h \
])


AC_CHECK_HEADERS([net/if_arp.h net/route.h], [], [],
                 [#include <sys/types.h>
                  #include <sys/socket.h>])

dnl
dnl Try to find IP_MTU socket option definition.
dnl
AH_TEMPLATE([MY_IP_MTU],
    [Cope with strange location of IP_MTU socket option definition])

AC_CHECK_HEADER([sys/socket.h],
    [AC_MSG_CHECKING([for IP_MTU in sys/socket.h])
     AC_COMPILE_IFELSE([
#include <sys/socket.h>
int a = IP_MTU;],
         [AC_MSG_RESULT(found)],
         [AC_MSG_RESULT([not found])
          AC_CHECK_HEADER([linux/in.h],
              [AC_MSG_CHECKING([for IP_MTU in linux/in.h])
               AC_COMPILE_IFELSE([
#include <sys/socket.h>
#include <linux/in.h>
int a = IP_MTU;],
                   [AC_MSG_RESULT([found])
                    my_ip_mtu=`grep "#[ \t]*define[ \t][ \t]*IP_MTU\>" \
                               /usr/include/linux/in.h | sed -s \
    "s/#[ \t]*define[ \t][ \t]*IP_MTU[ \t][ \t]*\([0-9]*\)[ \t]*$/\1/"`
                    AC_DEFINE_UNQUOTED([MY_IP_MTU], [$my_ip_mtu])],
                   [AC_MSG_RESULT([not found])])
              ], [], [#include <sys/socket.h>])])])

if test -n "$with_cfg_linux_daemons" ; then
  DAEMONS_CFLAGS=-DCFG_LINUX_DAEMONS
  for i in $with_cfg_linux_daemons ; do
    case $i in 
        tftp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TFTP_SERVER"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
        ftp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_FTP_SERVER" ;;
        dns)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DNS_SERVER" ;;
        echo)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_ECHO_SERVER"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
        todudp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TODUDP_SERVER"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
        dhcp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DHCP_SERVER" ;
            DAEMONS_LDADD="-ldhcpctl -lomapi -ldst" ;;
        smtp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_SMTP" ;;
        sshd) ;;
        Xvfb) ;;
        *)  ;; 
    esac
  done
fi
AC_SUBST([DAEMONS_CFLAGS])
AC_SUBST([DAEMONS_LDADD])


AM_CONDITIONAL(CFG_LINUX_DAEMONS, test -n "$with_cfg_linux_daemons")
AM_CONDITIONAL(CFG_RCF_RPC, test -n "$with_rcf_rpc")

# #
# # Check for NET-SNMP library for possible usage by TAD SNMP; 
# # explore correct link flags if it is found installed. 
# #
# AC_PATH_PROG(NET_SNMP_CONFIG, net-snmp-config, no) 
# if test "$NET_SNMP_CONFIG" != "no" ; then
#     NET_SNMP_LIBS=`$NET_SNMP_CONFIG --libs`
# fi
# AC_SUBST([NET_SNMP_LIBS])
# 
# AM_CONDITIONAL(HAS_NET_SNMP, test "$NET_SNMP_CONFIG" != "no")

# # Check for expect library
# AC_CHECK_LIB(expect, exp_expectv)

dnl #######################################################################
dnl Newer BSD systems don't have a compatible rtentry - use ortentry
dnl #######################################################################
AC_CHECK_TYPES([struct rtentry, struct ortentry], [], [],
              [#include <sys/types.h>
               #include <sys/socket.h>
               #include <net/route.h>])

dnl
dnl Check for 'sa_len'-like members in 'struct sockaddr'-like structures.
dnl
AC_CHECK_MEMBERS([struct sockaddr.sa_len], [], [],
                 [#include <sys/types.h>
                  #include <sys/socket.h>])

dnl
dnl Check for 'sa_restorer' member in 'struct sigaction'.
dnl
AC_CHECK_MEMBERS([struct sigaction.sa_restorer], [], [],
                 [#include <signal.h>])

dnl
dnl POSIX threads are provided by:
dnl - -lpthread in Linux;
dnl - -lc_r in FreeBSD;
dnl - -lpthread in NetBSD ('pth' package).
dnl
AC_SEARCH_LIBS([pthread_create], [pthread c_r], [],
               AC_MSG_ERROR([No POSIX threads]))

dnl
dnl POSIX semaphores are provided by:
dnl - -lpthread in Linux;
dnl - -lc_r in FreeBSD;
dnl - -lsemaphore in NetNSD ('pthread-sem' package).
dnl
AC_SEARCH_LIBS([sem_init], [pthread c_r semaphore], [],
               AC_MSG_ERROR([No POSIX semaphores]))

if test -n "$with_rcf_rpc" -o -n "$with_cfg_linux_daemons" ; then
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
fi

if test -n "$with_rcf_rpc" ; then
    dnl
    dnl FreeBSD 4.x has no svc_exit().
    dnl NetBSD has no svcunix_create().
    dnl
    AC_CHECK_FUNCS([svcunix_create svc_exit])

    dnl
    dnl Asynchronous I/O:
    dnl - FreeBSD has in -lc;
    dnl - Linux has in -lnsl -lrt;
    dnl - NetBSD does not have.
    dnl
    AC_CHECK_FUNC([aio_read], [],
        AC_CHECK_LIB([nsl], [aio_read], [LIBS="-lnsl -lrt ${LIBS}"],
            AC_MSG_WARN([Asynchronous I/O not supported]), [-lrt]))

dnl    AC_CHECK_FUNC([xdr_uint8_t], []
dnl        AC_CHECK_FUNC([xdr_u_int8_t],
dnl            AC_DEFINE([xdr_uint8_t], xdr_u_int8_t),
dnl            AC_ERROR([There is no xdr_uint8_t or xdr_u_int8_t!])))

    AC_CHECK_FUNC([xdr_uint8_t], [],
        [AC_CHECK_FUNC([xdr_u_int8_t],
            [AC_DEFINE([xdr_uint8_t], [xdr_u_int8_t])],
            [AC_ERROR([There is no xdr_uint8_t or xdr_u_int8_t!])])])
fi


AC_CONFIG_FILES([Makefile])

AC_OUTPUT
