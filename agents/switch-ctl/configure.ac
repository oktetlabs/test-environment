#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT(ta-switch-ctl, 0.1, Andrew.Rybchenko@oktetlabs.ru)
AC_CONFIG_SRCDIR([main.c])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([-Wall foreign])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL


AC_CHECK_TOOL(NM, nm)


AC_CANONICAL_HOST

AC_SUBST(host)
AC_SUBST(TE_CPPFLAGS)
AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)
AC_SUBST(TE_LDADD)
AC_SUBST(TE_DEPENDENCIES)

AC_ARG_VAR(TE_CPPFLAGS)
AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)
AC_ARG_VAR(TE_LDADD)
AC_ARG_VAR(TE_DEPENDENCIES)

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AM_CONFIG_HEADER(config.h)

AC_HEADER_STDC
AC_CHECK_HEADERS(time.h sys/time.h netinet/in.h assert.h semaphore.h)
AC_C_INLINE

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
