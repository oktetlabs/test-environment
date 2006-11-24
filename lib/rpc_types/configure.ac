#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([te-rpc_types], [0.1], [Andrew.Rybchenko@oktetlabs.ru])

AC_CONFIG_SRCDIR([te_rpc_types.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
if test x$host != x${host%windows} ; then
    RANLIB=touch
else    
    AC_PROG_RANLIB
fi    

AM_CONDITIONAL(WINDOWS, test x$host != x${host%windows})

TE_LIB_SET

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
              ], [], [#include <sys/socket.h>])])], [],
[
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])

dnl
dnl Check for Solaris-style sysinfo()
dnl
AC_CHECK_HEADERS([sys/systeminfo.h])

dnl
dnl Check for 'sa_restorer' member in 'struct sigaction'.
dnl
AC_CHECK_MEMBERS([struct sigaction.sa_restorer], [], [],
                 [#include <signal.h>])

dnl
dnl Check for Solaris-specific extension '__sin6_src_id' members in
dnl 'struct sockaddr_in6' structure.
dnl
AC_CHECK_MEMBERS([struct sockaddr_in6.__sin6_src_id], [], [], [
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
