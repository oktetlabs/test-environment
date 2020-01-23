# Process this file with autoconf to produce a configure script.
AC_INIT([rpcs_serial],[0.1],[Ivan.Danilov@oktetlabs.ru])

AC_CONFIG_SRCDIR([serial.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.9.6 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

# Used here tarpc.h includes rpc/rpc.h which requires extra -I option
# in the case of libtirpc. There is no easy way to inherit it from
# rpcxdr findings so just repeat the check here
PKG_CHECK_MODULES([TIRPC], [libtirpc], [],
    [AC_MSG_NOTICE([libtirpc not found])])
AC_SUBST([TIRPC_CFLAGS])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
