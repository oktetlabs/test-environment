#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([upnp_cp],[0.01],[Ivan.Melnikov@oktetlabs.ru])

AC_CONFIG_SRCDIR([tarpc_upnp_cp.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.9.6 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

AC_ARG_WITH([name],,,[with_name=$PACKAGE_NAME])
AC_SUBST([with_name])

UPNP_STATIC_LIBS=
UPNP_DYNAMIC_LIBS=

PKG_CHECK_MODULES([JANSSON], [jansson],
    [UPNP_STATIC_LIBS="${UPNP_STATIC_LIBS} -ljansson"],
    [AC_MSG_ERROR([jansson not found])]
)
PKG_CHECK_MODULES([GUPNPAV], [gupnp-av-1.0],
    [UPNP_STATIC_LIBS="${UPNP_STATIC_LIBS} -lgupnp-av-1.0"],
    [AC_MSG_ERROR([gupnp-av-1.0 not found])]
)
PKG_CHECK_MODULES([GUPNP], [gupnp-1.2],
    [UPNP_STATIC_LIBS="${UPNP_STATIC_LIBS} -lgupnp-1.2"],
    [
        PKG_CHECK_MODULES([GUPNP], [gupnp-1.0],
        [UPNP_STATIC_LIBS="${UPNP_STATIC_LIBS} -lgupnp-1.0"],
        [AC_MSG_ERROR([Neither gupnp-1.2 nor gupnp-1.0 found])])
    ]
)
PKG_CHECK_MODULES([GSSDP], [gssdp-1.2],
    [UPNP_STATIC_LIBS="${UPNP_STATIC_LIBS} -lgssdp-1.2"],
    [
        PKG_CHECK_MODULES([GSSDP], [gssdp-1.0],
        [UPNP_STATIC_LIBS="${UPNP_STATIC_LIBS} -lgssdp-1.0"],
        [AC_MSG_ERROR([Neither gssdp-1.2 nor gssdp-1.0 found])])
    ]
)
PKG_CHECK_MODULES([LIBSOUP], [libsoup-2.4],
    [UPNP_DYNAMIC_LIBS="${UPNP_DYNAMIC_LIBS} -lsoup-2.4"],
    [AC_MSG_ERROR([libsoup-2.4 not found])]
)

PKG_CHECK_MODULES([GLIB], [glib-2.0],
    [UPNP_DYNAMIC_LIBS="${UPNP_DYNAMIC_LIBS} -lglib-2.0"],
    [AC_MSG_ERROR([glib-2.0 not found])]
)
PKG_CHECK_MODULES([GIO], [gio-2.0],
    [UPNP_DYNAMIC_LIBS="${UPNP_DYNAMIC_LIBS} -lgio-2.0"],
    [AC_MSG_ERROR([gio-2.0 not found])]
)
PKG_CHECK_MODULES([GOBJECT], [gobject-2.0],
    [UPNP_DYNAMIC_LIBS="${UPNP_DYNAMIC_LIBS} -lgobject-2.0"],
    [AC_MSG_ERROR([gobject-2.0 not found])]
)
PKG_CHECK_MODULES([GMODULE], [gmodule-2.0],
    [UPNP_DYNAMIC_LIBS="${UPNP_DYNAMIC_LIBS} -lgmodule-2.0"],
    [AC_MSG_ERROR([gmodule-2.0 not found])]
)
PKG_CHECK_MODULES([UUID], [uuid],
    [UPNP_DYNAMIC_LIBS="${UPNP_DYNAMIC_LIBS} -luuid"],
    [AC_MSG_ERROR([uuid not found])]
)
PKG_CHECK_MODULES([LIBXML], [libxml-2.0],
    [UPNP_DYNAMIC_LIBS="${UPNP_DYNAMIC_LIBS} -lxml2"],
    [AC_MSG_ERROR([libxml-2.0 not found])]
)

TA_UPNP_CP_LIBS="-Wl,-Bstatic ${UPNP_STATIC_LIBS} -Wl,-Bdynamic ${UPNP_DYNAMIC_LIBS}"
AC_SUBST([TA_UPNP_CP_LIBS])

LIBS="${UPNP_STATIC_LIBS} -Wl,-Bdynamic ${UPNP_DYNAMIC_LIBS}"

dnl Used here tarpc.h includes rpc/rpc.h which requires extra -I option
dnl in the case of libtirpc. There is no easy way to inherit it from
dnl rpcxdr findings so just repeat the check here
PKG_CHECK_MODULES([TIRPC], [libtirpc], [],
    [AC_MSG_NOTICE([libtirpc not found])])
AC_SUBST([TIRPC_CFLAGS])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
