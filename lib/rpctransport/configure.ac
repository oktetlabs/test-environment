#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rpctransport],[0.01],[Artem.Andreev@oktetlabs.ru])

AC_CONFIG_SRCDIR([rpc_transport.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.11.6 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_INSTALL
AC_FIXED_LIBTOOL_RANLIB

TE_LIB_SET

AC_ARG_ENABLE([local-transport],
              AS_HELP_STRING([--disable-local-transport],
                             [disable local transport (Unix domain sockets or Windows pipes, enabled by default)]),
              [ac_cv_enable_local_transport=$enableval],
              [ac_cv_enable_local_transport=yes])
AC_CACHE_CHECK([whether to enable local transport],
               [ac_cv_enable_local_transport], [ac_cv_enable_local_transport=yes])
if test x"$ac_cv_enable_local_transport" != xno
then
    AC_DEFINE([ENABLE_LOCAL_TRANSPORT], [], [Enable local RPC transport])
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DENABLE_LOCAL_TRANSPORT="
fi

AC_ARG_ENABLE([tcp-transport],
              AS_HELP_STRING([--enable-tcp-transport],
                             [enable TCP transport (disabled by default)]),
              [ac_cv_enable_tcp_transport=$enableval],
              [ac_cv_enable_tcp_transport=no])
AC_CACHE_CHECK([whether to enable TCP transport],
               [ac_cv_enable_tcp_transport], [ac_cv_enable_tcp_transport=no])
if test x"$ac_cv_enable_tcp_transport" != xno
then
    AC_DEFINE([ENABLE_TCP_TRANSPORT], [], [Enable TCP RPC transport])
    EXTRA_CPPFLAGS="$EXTRA_CPPFLAGS -DENABLE_TCP_TRANSPORT="
fi

# EXTRA_CPPFLAGS are used so that a test for enabled transport could
# be done prior to including any headers
AC_SUBST([EXTRA_CPPFLAGS])

AC_CONFIG_HEADER([package.h])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
