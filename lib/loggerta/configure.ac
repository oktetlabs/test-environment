#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([loggerta], [0.01], [Igor.Vasiliev@oktetlabs.ru])

AC_CONFIG_SRCDIR([logger_ta.h])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

CPPFLAGS=${CPPFLAGS}" "${TE_CFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])

AM_CONFIG_HEADER([config.h])

AC_C_INLINE

AC_CHECK_HEADERS([\
stdlib.h \
stdarg.h \
string.h \
time.h \
sys/time.h \
semaphore.h \
netinet/in.h \
])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
