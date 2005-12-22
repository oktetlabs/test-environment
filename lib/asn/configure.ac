
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([asn], [0.90], [konst@oktetlabs.ru])

AC_CONFIG_SRCDIR([asn_val.c])

AC_CONFIG_AUX_DIR([../../auxdir])
AC_CONFIG_MACRO_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(OBJDUMP, objdump)
AC_CHECK_TOOL(OBJCOPY, objcopy)

# We use APP here as we need to build tests
TE_APP_SET

AC_CONFIG_HEADER(package.h)

AC_CONFIG_FILES([Makefile tests/Makefile])

TE_APP_RESTORE

AC_OUTPUT
