#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ta-vxworks], [0.01], [Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([vxworks.c])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

AC_PROG_INSTALL

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(NM, nm)
AC_CHECK_TOOL(OBJDUMP, objdump)
AC_CHECK_TOOL(OBJCOPY, objcopy)

AC_SUBST([host])
AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])
AC_SUBST([TE_LDADD])
AC_SUBST([TE_DEPENDENCIES])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_LDADD])
AC_ARG_VAR([TE_DEPENDENCIES])

CFLAGS=${CFLAGS}" "${TE_CFLAGS}
CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}

case $host in

#powerpc-wrs-vxworks | i686-pc-linux-gnu)
arm-wrs-vxworks )

AC_CONFIG_HEADERS([config.h])
AC_C_INLINE
just_dist=no ;;

*) just_dist=yes ;;

esac

AM_CONDITIONAL(JUST_DIST, test "$just_dist" = yes)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
