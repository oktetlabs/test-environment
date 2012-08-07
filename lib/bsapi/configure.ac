#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([bsapi], [0.01], [Elena.Vengerova@oktetlabs.ru])

AC_CONFIG_SRCDIR([bsapi.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(OBJDUMP, objdump)
AC_CHECK_TOOL(OBJCOPY, objcopy)

TE_LIB_SET

AC_CONFIG_HEADER(package.h)

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
