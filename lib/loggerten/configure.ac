#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([logger_ten], [0.03], [Igor.Vasiliev@oktetlabs.ru])

AC_CONFIG_SRCDIR([logger_ten.h])

AC_CONFIG_AUX_DIR([../../auxdir])

AC_C_INLINE

AM_INIT_AUTOMAKE([1.8.5 foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL


CPPFLAGS=${CPPFLAGS}" "${TE_CPPFLAGS}
CFLAGS=${CFLAGS}" "${TE_CFLAGS}

AC_SUBST([TE_CFLAGS])
AC_SUBST([TE_LDFLAGS])

AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])

AM_CONFIG_HEADER([config.h])

AC_CHECK_HEADERS([\
stdlib.h \
stdarg.h \
string.h \
strings.h \
errno.h \
unistd.h \
sys/types.h \
time.h \
sys/time.h \
netinet/in.h \
sys/stat.h \
pthread.h \
])


AC_CONFIG_FILES([Makefile])

AC_OUTPUT
