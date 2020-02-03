#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([tapi_upnp],[0.1],[Ivan.Melnikov@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_upnp.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.9.6 foreign -Wall])

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

dnl Check for libglib
PKG_CHECK_MODULES([GLIB], [glib-2.0],
    [],
    [AC_MSG_ERROR([glib-2.0 not found])]
)
dnl Check for libxml
PKG_CHECK_MODULES([LIBXML], [libxml-2.0],
    [],
    [AC_MSG_ERROR([libxml-2.0 not found])]
)
dnl Check for libsoup
PKG_CHECK_MODULES([LIBSOUP], [libsoup-2.4],
    [],
    [AC_MSG_ERROR([libsoup-2.4 not found])]
)
dnl Check for libgssdp
PKG_CHECK_MODULES([GSSDP], [gssdp-1.0],
    [],
    [AC_MSG_ERROR([gssdp-1.0 not found])]
)
dnl Check for libgupnp
PKG_CHECK_MODULES([GUPNP], [gupnp-1.0],
    [],
    [AC_MSG_ERROR([gupnp-1.0 not found])]
)
dnl Check for libgupnp-av
PKG_CHECK_MODULES([GUPNPAV], [gupnp-av-1.0],
    [],
    [AC_MSG_ERROR([gupnp-av-1.0 not found])]
)
dnl Check for libjansson
PKG_CHECK_MODULES([JANSSON], [jansson],
    [],
    [AC_MSG_ERROR([jansson not found])]
)

# Used here tarpc.h includes rpc/rpc.h which requires extra -I option
# in the case of libtirpc. There is no easy way to inherit it from
# rpcxdr findings so just repeat the check here
PKG_CHECK_MODULES([TIRPC], [libtirpc], [],
    [AC_MSG_NOTICE([libtirpc not found])])
AC_SUBST([TIRPC_CFLAGS])
AC_SUBST([TIRPC_LIBS])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
