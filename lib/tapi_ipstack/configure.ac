
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT(tapi_ipstack, 0.01, konst@oktetlabs.ru)

AC_CONFIG_SRCDIR([tapi_ipstack.c])

AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE([foreign -Wall])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

CFLAGS="${CFLAGS} ${TE_CFLAGS}"
CPPFLAGS="${CPPFLAGS} ${TE_CFLAGS}"

AC_SUBST(TE_CFLAGS)
AC_SUBST(TE_LDFLAGS)
AC_SUBST(TE_LDADD)
AC_SUBST(TE_DEPENDENCIES)

AC_ARG_VAR(TE_CFLAGS)
AC_ARG_VAR(TE_LDFLAGS)
AC_ARG_VAR(TE_LDADD)
AC_ARG_VAR(TE_DEPENDENCIES)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([\
stdlib.h \
string.h \
strings.h \
sys/socket.h \
unistd.h \
arpa/inet.h \
netinet/in.h \
])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([inet_ntoa memset])


AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
