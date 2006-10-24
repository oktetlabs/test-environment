dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.53)
AC_INIT([tad-ts], 0.1, Andrew.Rybchenko@oktetlabs.ru)

AC_CONFIG_SRCDIR([Doxyfile])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 -Wall foreign])

AC_CANONICAL_HOST
AC_SUBST([host])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

TS_CPPFLAGS="${TE_CPPFLAGS}"
TS_CFLAGS="${TE_CFLAGS}"
TS_LDFLAGS="${TE_LDFLAGS}"

AC_SUBST([TS_CPPFLAGS])
AC_SUBST([TS_CFLAGS])
AC_SUBST([TS_LDFLAGS])
AC_SUBST([TE_PATH])
AC_SUBST([TE_PLATFORM])

AC_ARG_VAR([TS_CPPFLAGS])
AC_ARG_VAR([TS_CFLAGS])
AC_ARG_VAR([TS_LDFLAGS])
AC_ARG_VAR([TE_CPPFLAGS])
AC_ARG_VAR([TE_CFLAGS])
AC_ARG_VAR([TE_LDFLAGS])
AC_ARG_VAR([TE_INSTALL])
AC_ARG_VAR([TE_PATH])
AC_ARG_VAR([TE_PLATFORM])

TS_LIBS="-ltapi_tad -ltapi_env -ltapi_rpc -lrcfrpc -ltapi -lconfapi \
-lconf_oid -lrcfapi -lndn -lasn -lrpcxdr -lrpc_types -llogger_ten \
-lipc -llog_bufs -ltools -lpthread -lm"

TS_DEPS="${TE_INSTALL}/${TE_PLATFORM}/lib/libtools.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/liblog_bufs.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libipc.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/liblogger_ten.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/librpc_types.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/librpcxdr.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libasn.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libndn.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/librcfapi.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libconf_oid.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libconfapi.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libtapi.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/librcfrpc.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libtapi_rpc.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libtapi_env.a \
${TE_INSTALL}/${TE_PLATFORM}/lib/libtapi_tad.a"

AC_SUBST([TS_LIBS])
AC_SUBST([TS_DEPS])

AC_CONFIG_HEADERS([package.h])

AC_CONFIG_FILES([\
Makefile \
common/Makefile \
atm/Makefile \
eth/Makefile \
ipstack/Makefile \
])

AC_OUTPUT
