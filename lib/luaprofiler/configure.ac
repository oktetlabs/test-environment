#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(luaprofiler, 2.0.2)
AC_CONFIG_SRCDIR([src/core_profiler.h])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S

TE_LIB_SET

EXTRA_CPPFLAGS=-DTESTS
AC_SUBST([EXTRA_CPPFLAGS])

AC_CONFIG_FILES([Makefile src/Makefile src/analyzer/Makefile])
TE_LIB_RESTORE
AC_OUTPUT
