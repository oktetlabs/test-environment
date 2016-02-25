dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.53])
AC_INIT([telephony],[0.1],[Evegeny.Omelchenko@oktetlabs.ru])

AC_CONFIG_AUX_DIR([../../auxdir])

AM_INIT_AUTOMAKE([1.8.5 -Wall foreign])
TE_SETUP_AR

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

TS_LIBS=" -ltapi_rpc -lrcfrpc -ltapi -lconfapi \
-lconf_oid -lrcfapi -lrpcxdr -lrpc_types -llogger_ten \
-lipc -llog_bufs -ltools -lpthread -lm"

TS_DEPS="\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/libtools.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/liblog_bufs.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/libipc.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/liblogger_ten.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/librpc_types.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/librpcxdr.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/librcfapi.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/libconf_oid.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/libconfapi.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/libtapi.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/librcfrpc.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/libtapi_rpc.a"

AC_SUBST([TS_LIBS])
AC_SUBST([TS_DEPS])

AC_CONFIG_HEADERS([package.h])

AC_CONFIG_FILES([\
Makefile \
])

AC_OUTPUT
