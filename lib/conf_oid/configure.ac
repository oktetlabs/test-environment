#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT(conf_oid,  0.01, Renata.Sayakhova@oktetlabs.ru)

AC_CONFIG_SRCDIR([conf_oid.h])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(OBJDUMP, objdump)
AC_CHECK_TOOL(OBJCOPY, objcopy)

CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])

AC_CONFIG_HEADERS(config.h)

AC_CHECK_HEADERS(string.h strings.h)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
