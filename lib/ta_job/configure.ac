AC_PREREQ([2.69])
AC_INIT([ta_job], [0.1], [Andrey.Izrailev@oktetlabs.ru])

AC_CONFIG_SRCDIR([ta_job.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.11.1 foreign -Wall])
TE_SETUP_AR

# Checks for programs.
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_FIXED_LIBTOOL_RANLIB
AC_PROG_INSTALL

TE_LIB_SET

dnl Check for libpcre
PCRE_LIBS=

AC_CHECK_HEADER([pcre.h], [],
[AC_MSG_ERROR([Agent job control library requires pcre.h])],
[#include <pcre.h>])
AC_CHECK_LIB([pcre], [pcre_exec], [],
[AC_MSG_ERROR([Agent job control library requires PCRE library])])

AC_SUBST([PCRE_LIBS])

dnl Used here because tarpc.h includes rpc/rpc.h which requires extra
dnl -I option in the case of libtirpc. There is no easy way to inherit
dnl it from rpcxdr findings so just repeat the check here
PKG_CHECK_MODULES([TIRPC], [libtirpc], [],
                  [AC_MSG_NOTICE([libtirpc not found])])
AC_SUBST([TIRPC_CFLAGS])

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
