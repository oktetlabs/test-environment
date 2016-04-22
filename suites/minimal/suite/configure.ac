dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.53])
AC_INIT([minimal],[0.1],[Oleg.Kravtsov@oktetlabs.ru])

AC_CONFIG_SRCDIR([configure.ac])

AC_CONFIG_AUX_DIR([auxdir])

AM_INIT_AUTOMAKE([1.8.5 -Wall foreign])
TE_SETUP_AR

AC_CANONICAL_HOST
AC_SUBST([host])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL

TS_CPPFLAGS="-I\$(srcdir)/lib -I\$(top_srcdir)/lib ${TE_CPPFLAGS}"
TS_CFLAGS="${TE_CFLAGS}"
TS_LDFLAGS="-L. -L./lib -L\$(top_builddir)/lib ${TE_LDFLAGS}"

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

TS_TE_LIBS="-ltapi_env -lrcfrpc -ltapi -ltapi_rpc \
-lconfapi -lconf_oid -lrcfapi -llogger_ten -lrpcxdr -lrpc_types -lipc \
-llog_bufs -lpthread -ltools"

TS_TE_DEPS="\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/libipc.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/librcfapi.a \
\$(DESTDIR)/\$(prefix)/../${TE_PLATFORM}/lib/libconfapi.a \
\$(DESTDIR)/\$(prefix)/..//${TE_PLATFORM}/lib/libconf_oid.a \
\$(DESTDIR)/\$(prefix)/..//${TE_PLATFORM}/lib/liblogger_ten.a \
\$(DESTDIR)/\$(prefix)/..//${TE_PLATFORM}/lib/liblog_bufs.a \
\$(DESTDIR)/\$(prefix)/..//${TE_PLATFORM}/lib/libtapi.a \
\$(DESTDIR)/\$(prefix)/..//${TE_PLATFORM}/lib/libtapi_env.a"

AC_SUBST([TS_TE_LIBS])
AC_SUBST([TS_TE_DEPS])

AC_CONFIG_HEADERS([package.h])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
