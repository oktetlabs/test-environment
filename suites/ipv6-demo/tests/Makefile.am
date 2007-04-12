SUBDIRS = lib . sockets

EXTRA_DIST = Doxyfile mainpage.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TS_CPPFLAGS@
AM_CFLAGS = @TS_CFLAGS@
AM_LDFLAGS = @TS_LDFLAGS@

tetest_PROGRAMS = prologue

MYLDADD = @TS_TE_LIBS@
MYDEPS = @TS_TE_DEPS@

prologue_SOURCES = prologue.c
prologue_LDADD = $(MYLDADD)
prologue_DEPENDENCIES = $(MYDEPS)
