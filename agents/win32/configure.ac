#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ta-win32], [0.3], [Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([win32.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_CHECK_TOOL(NM, nm)

if test -z "$host" ; then
AC_CANONICAL_HOST
fi 
_host_=`echo $host | sed s/-/_/g`
AC_SUBST(_host_)

TE_APP_SET

AC_SUBST([host])
AC_SUBST([with_name])

if test -z "$with_tad" ; then
    with_tad=tad
fi 
AC_SUBST([TAD], [$with_tad])

AC_SUBST([TAD_LIBS])

AC_CONFIG_HEADERS([config.h])

AM_CONDITIONAL(CFG_RCF_RPC, test -n "$with_rcf_rpc")
AM_CONDITIONAL(CFG_STANDALONE_RPCSERVER, 
               test -n "$with_standalone_rpcserver")

export PKG_CONFIG_PATH=$DESTDIR/$prefix/../$host/lib/pkgconfig/

PKG_CHECK_MODULES(TAD, te-$with_tad)

AC_CONFIG_FILES([Makefile])

TE_APP_RESTORE

AC_OUTPUT
