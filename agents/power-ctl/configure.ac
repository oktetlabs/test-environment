dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([tapower-ctl],[0.01],[Boris.Misenov@oktetlabs.ru])
AC_CONFIG_SRCDIR([main.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

TE_APP_SET

AC_SUBST([with_name])

AC_ARG_WITH([sniffers],
            AS_HELP_STRING([--with-sniffers=yes],
                           [enable sniffers (default is NO)]),
            [with_sniffers=sniffers_dummy], [with_sniffers=""])
AM_CONDITIONAL(CFG_SNIFFERS_DUMMY, test -n "$with_sniffers")

if test -z "$with_tad" ; then
    with_tad=tad
fi
AC_SUBST([TAD], [$with_tad])
AC_SUBST([TAD_LIBS])

AC_CONFIG_HEADERS([config.h])

AC_SEARCH_LIBS([pthread_create], [pthread kse c_r], [],
               AC_MSG_ERROR([No POSIX threads]))

export PKG_CONFIG_PATH="$DESTDIR/$prefix/../$TE_PLATFORM/lib/pkgconfig/"

PKG_CHECK_MODULES(TAD, te-$with_tad)

AC_MSG_CHECKING(for net-snmp library)
AC_CHECK_HEADERS([net-snmp/session_api.h], , ,
                 [#include <net-snmp/net-snmp-config.h>
                  #include <net-snmp/library/snmp_transport.h>]
                )
AC_PATH_PROG(NET_SNMP_CONFIG, net-snmp-config, no)
if test "$NET_SNMP_CONFIG" = "no" ; then
    AC_MSG_ERROR([net-snmp-config is required but not found])
else
    LIBS="`$NET_SNMP_CONFIG --libs` -lcrypto $LIBS"
    CFLAGS="`$NET_SNMP_CONFIG --cflags` $CFLAGS"
fi

AC_CHECK_FUNCS([snmp_pdu_type], [], [])

AC_CONFIG_FILES([Makefile])

TE_APP_RESTORE

AC_OUTPUT
