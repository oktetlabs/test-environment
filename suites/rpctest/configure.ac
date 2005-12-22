# Process this file with autoconf to produce a configure script.
AC_INIT([rpctest], [0.01], [Elena.Vangerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([rpctest.c])
AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CANONICAL_HOST
AC_SUBST([host])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_SUBST([TE_CPPFLAGS])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

RPC_XFILE=${TE_INSTALL}/agents/etc/linux/tarpc.x

AC_SUBST([RPC_XFILE])

AC_CHECK_HEADERS([\
stdarg.h \
])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
