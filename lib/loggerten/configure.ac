#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(logger_ten, 0.01, Igor.Vasiliev@oktetlabs.ru)

AC_CONFIG_SRCDIR([logger_ten.h])

AC_CONFIG_AUX_DIR(../../aux)

AC_C_INLINE

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)

AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)

AM_CONFIG_HEADER(config.h)

AC_CHECK_HEADERS(stdlib.h string.h strings.h sys/types.h time.h sys/time.h \
                 netinet/in.h sys/stat.h pthread.h)

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
