#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ta-win32], [0.3], [Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([win32.c])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_CHECK_TOOL(NM, nm)

AC_CANONICAL_HOST

AC_SUBST([host])
AC_SUBST([TE_CPPFLAGS])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CPPFLAGS])
AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AM_CONFIG_HEADER([config.h])

AC_CHECK_HEADERS([\
time.h \
sys/time.h \
sys/resource.h \
semaphore.h \
])
AC_C_INLINE

AM_CONDITIONAL(CFG_RCF_RPC, test -n "$with_rcf_rpc")

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
