AC_PREREQ([2.69])
AC_INIT([ta_bpf], [0.1], [Dmitry.Izbitsky@oktetlabs.ru])

AC_CONFIG_SRCDIR([te_bpf_helpers.h])

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

AC_CONFIG_FILES([Makefile])

TE_LIB_RESTORE

AC_OUTPUT
