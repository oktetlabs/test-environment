# Process this file with autoconf to produce a configure script.
AC_INIT([tapi_rpcsock], [0.01], [Elena.Vangerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_rpc.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

TE_LIB_SET

RPC_XFILE=${srcdir}/../../include/tarpc.x.m4

AC_ARG_WITH([tr069],
            AS_HELP_STRING([--with-tr069],
                           [enable TR-069 support (default is NO)]),
            [],
            [with_tr069=no])

AM_CONDITIONAL(CFG_TR069, test x"$with_tr069" != xno)

AS_IF([test x"$with_tr069" != xno],
      [AC_DEFINE([WITH_TR069_SUPPORT], [], \
      [Whether to enable TR-069 support]) ])

AC_SUBST([RPC_XFILE])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
