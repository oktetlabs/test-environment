#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ta-win32],[0.3],[Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([win32.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_CHECK_TOOL(NM, nm)
AC_CANONICAL_HOST

TE_APP_SET

RPCSERVER_SYMTBL_ADD=
RPCSERVER_LIBS_ADD=
with_rpc_libs="${with_rpc_libs//,/ }"
for i in $with_rpc_libs ; do 
    RPCSERVER_SYMTBL_ADD="$RPCSERVER_SYMTBL_ADD \
        \$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/lib${i}.a"
    RPCSERVER_LIBS_ADD="$RPCSERVER_LIBS_ADD lib${i}.a"
done
AC_SUBST(RPCSERVER_SYMTBL_ADD)
AC_SUBST(RPCSERVER_LIBS_ADD)
    

AC_SUBST([with_name])

if test -z "$with_tad" ; then
    with_tad=tad
fi 
AC_SUBST([TAD], [$with_tad])

AC_SUBST([TAD_LIBS])

AC_CONFIG_HEADERS([config.h])

AC_MSG_CHECKING([unsetenv() return type])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]],
                                   [[ int tmp = unsetenv(NULL); ]])],
                  [unsetenv_returns=int
                   AC_MSG_RESULT([int])],
                  [unsetenv_returns=void
                   AC_MSG_RESULT([void])])
AC_DEFINE_UNQUOTED(UNSETENV_RETURN_TYPE, $unsetenv_returns, 
                   [Defined if unsetenv() returns int, not void])
if test "$unsetenv_returns" = "int"; then
   AC_DEFINE(UNSETENV_RETURNS_INT, 1, [Defined when unsetenv() returns int])
fi                     

AM_CONDITIONAL(CFG_RCF_RPC, test "$with_rcf_rpc" = yes)
AM_CONDITIONAL(CFG_ISCSI, test "$with_iscsi" = yes)
AM_CONDITIONAL(CFG_STANDALONE_RPCSERVER, 
               test "$with_standalone_rpcserver" = yes)

export PKG_CONFIG_PATH=$DESTDIR/$prefix/../$TE_PLATFORM/lib/pkgconfig/

AC_CONFIG_FILES([Makefile])

TE_APP_RESTORE

AC_OUTPUT
