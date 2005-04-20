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
AC_PROG_LEX
AC_PROG_YACC

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

AC_ARG_ENABLE([wifi],
              AS_HELP_STRING([--enable-wifi],
                             [enable WiFi support (default is NO)]),
              [ac_cv_enable_wifi=$enableval], [ac_cv_enable_wifi=no])
AC_CACHE_CHECK([whether to enable WiFi],
               [ac_cv_enable_wifi], [ac_cv_enable_wifi=no])

if test x"$ac_cv_enable_wifi" != xno
then
dnl
dnl Check that we have iwlib installed in our system
dnl
    AC_CHECK_HEADER([iwlib.h], [],
                    AC_MSG_ERROR([Cannot find iwlib.h header]))
    AC_CHECK_LIB([iw], [iw_sockets_open], [],
                 AC_MSG_ERROR([Cannot find iw library]))

    AC_DEFINE([ENABLE_WIFI_SUPPORT], [], [Wheter to enable WiFi support])
    LDADD="$LDADD -liw"
fi

if test -z "$with_tad" ; then
    with_tad=tad
fi
AC_SUBST([TAD], [$with_tad])

AC_SUBST([TAD_LIBS])

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}


AC_CONFIG_HEADERS([config.h])

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

DS_SOURCES=
if test -n "$with_cfg_linux_daemons" ; then
  DAEMONS_CFLAGS=-DCFG_LINUX_DAEMONS
  for i in $with_cfg_linux_daemons ; do
    case $i in 
        tftp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TFTP_SERVER"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
            
        ftp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_FTP_SERVER" 
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;            
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
            
        telnet)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_TELNET"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
            
        rsh)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_RSH"
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_XINETD" 
            ;;
            
        dhcp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_DHCP_SERVER" ;
            ;;
            
        smtp)
            DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_SMTP" ;;
            
        sshd) ;;
        
        Xvfb) ;;
        
        vncserver) DAEMONS_CFLAGS="$DAEMONS_CFLAGS -DWITH_VNCSERVER" ;;
        
        *)  ;; 
    esac
  done
fi
AC_SUBST([DAEMONS_CFLAGS])
AC_SUBST([DAEMONS_LDADD])


AM_CONDITIONAL(CFG_LINUX_DAEMONS, test -n "$with_cfg_linux_daemons")
AM_CONDITIONAL(CFG_LINUX_DAEMONS_NAMED, 
    [expr "$with_cfg_linux_daemons" : ".*dns" >/dev/null])
AM_CONDITIONAL(CFG_LINUX_DAEMONS_ISC_DHCP_SERVER,
    [expr "$with_cfg_linux_daemons" : ".*dhcp" >/dev/null])
AM_CONDITIONAL(CFG_RCF_RPC, test -n "$with_rcf_rpc")
AM_CONDITIONAL(CFG_NETLINK_USE, test -n "$with_netlink")


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

    AH_TEMPLATE([xdr_uint8_t], [RPC XDR for unsigned 8bit width type])
    AC_CHECK_FUNC([xdr_uint8_t], [],
        [AC_CHECK_FUNC([xdr_u_int8_t],
            [AC_DEFINE([xdr_uint8_t], [xdr_u_int8_t])],
            [AC_CHECK_FUNC([xdr_u_char],
                [AC_DEFINE([xdr_uint8_t], [xdr_u_char])],
                [AC_ERROR([There is no xdr_uint8_t or xdr_u_int8_t!])])])])

    AH_TEMPLATE([xdr_uint16_t], [RPC XDR for unsigned 8bit width type])
    AC_CHECK_FUNC([xdr_uint16_t], [],
        [AC_CHECK_FUNC([xdr_u_int16_t],
            [AC_DEFINE([xdr_uint16_t], [xdr_u_int16_t])],
            [AC_ERROR([There is no xdr_uint16_t or xdr_u_int16_t!])])])

    AH_TEMPLATE([xdr_uint32_t], [RPC XDR for unsigned 32bit width type])
    AC_CHECK_FUNC([xdr_uint32_t], [],
        [AC_CHECK_FUNC([xdr_u_int32_t],
            [AC_DEFINE([xdr_uint32_t], [xdr_u_int32_t])],
            [AC_ERROR([There is no xdr_uint32_t or xdr_u_int32_t!])])])

    AH_TEMPLATE([xdr_uint64_t], [RPC XDR for unsigned 64bit width type])
    AC_CHECK_FUNC([xdr_uint64_t], [],
        [AC_CHECK_FUNC([xdr_u_int64_t],
            [AC_DEFINE([xdr_uint64_t], [xdr_u_int64_t])],
            [AC_ERROR([There is no xdr_uint64_t or xdr_u_int64_t!])])])
fi

if test -n "$with_netlink" ; then
    if test -n "$with_ifconfig" ; then
        AC_ERROR([Test Environment cannot 
        use both ioctl calls and netlink calls
        to configure interfaces]) ;
    fi        
    CPPFLAGS="$CPPFLAGS -DUSE_NETLINK"
    AC_CHECK_LIB([netlink], [rtnl_open], [],
            AC_MSG_ERROR([Cannot find libnetlink!]))

else
    if test -n "$with_ifconfig" ; then
        CPPFLAGS="$CPPFLAGS -DUSE_IFCONFIG"
    fi    
    CPPFLAGS="$CPPFLAGS -DUSE_IOCTL"
fi

export PKG_CONFIG_PATH=$DESTDIR/$prefix/../$host/lib/pkgconfig/

PKG_CHECK_MODULES(TAD, te-$with_tad)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
