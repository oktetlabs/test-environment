#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([ta-isosctl], [0.01], [Elena.Vengerova@oktetlabs.ru])
AC_CONFIG_SRCDIR([isosctl.c])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC

AC_PROG_INSTALL

AC_CHECK_TOOL(NM, nm)

AC_CANONICAL_HOST

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

if test -z "$with_tad" ; then
    with_tad=tad
fi
AC_SUBST([TAD], [$with_tad])

AC_SUBST([TAD_LIBS])

AC_CONFIG_HEADERS([config.h])

AC_CHECK_HEADERS([\
time.h \
sys/time.h \
])
AC_C_INLINE

export PKG_CONFIG_PATH=$DESTDIR/$prefix/../$host/lib/pkgconfig/

PKG_CHECK_MODULES(TAD, te-$with_tad)
                            
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
