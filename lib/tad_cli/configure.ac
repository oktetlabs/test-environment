# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(tad_cli, 0.01, kam@oktetlabs.ru)

AC_CONFIG_SRCDIR([tad_cli_csap.c])
AC_CONFIG_AUX_DIR(../../aux)

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

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

AM_CONFIG_HEADER(config.h)

AC_STDC_HEADERS
AC_CHECK_HEADERS([unistd.h fcntl.h sys/socket.c sys/types.h sys/time.h \
                  sys/wait.h signal.h sys/ioctl.h \
		  expect.h tcl8.4/expect.h])

# AC_CONFIG_FILES([Makefile tests/Makefile])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
