
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([tapi_dhcp], [0.01], [konst@oktetlabs.ru], [oleg@oktetlabs.ru])

AC_CONFIG_SRCDIR([tapi_stp.c])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL


CFLAGS="${CFLAGS} ${TE_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${TE_CFLAGS}"

AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

AC_CONFIG_HEADERS([package.h])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
