#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT([bsapi],  0.01, Elena.Vengerova@oktetlabs.ru)

AC_CONFIG_SRCDIR([bsapi.c])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(OBJDUMP, objdump)
AC_CHECK_TOOL(OBJCOPY, objcopy)

AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)

AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)

CFLAGS=${CFLAGS}" "${TE_CFLAGS}
CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
