#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(tester, 0.01, Elena.Vengerova@oktetlabs.ru)
AC_CONFIG_SRCDIR([te_tester])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)
AC_SUBST(TE_LDADD)
AC_SUBST(TE_DEPENDENCIES)

AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)
AC_ARG_VAR(TE_LDADD)
AC_ARG_VAR(TE_DEPENDENCIES)

CFLAGS=${CFLAGS}" "${TE_CFLAGS}
CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
