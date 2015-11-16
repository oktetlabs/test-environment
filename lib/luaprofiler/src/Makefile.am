SUBDIRS = analyzer

AM_CPPFLAGS = @TE_CPPFLAGS@ @EXTRA_CPPFLAGS@ $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ -ansi $(TE_CFLAGS_VAR)

lib_luadir = $(libdir)/lua

lib_LTLIBRARIES = liblua-profiler.la

liblua_profiler_la_SOURCES = \
    clocks.c                    \
    core_profiler.c             \
    function_meter.c            \
    lua50_profiler.c            \
    stack.c

install-exec-hook:
	@INSTALL@ -d $(lib_luadir)
	@LN_S@ -f ../liblua-profiler.so.0.0.0 $(lib_luadir)/profiler.so
