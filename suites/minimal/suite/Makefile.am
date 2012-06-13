tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TS_CPPFLAGS@
AM_CFLAGS = @TS_CFLAGS@
AM_LDFLAGS = @TS_LDFLAGS@

tetest_PROGRAMS = sample1

MYLDADD = @TS_TE_LIBS@ -lndn -lasn -lpthread
MYDEPS = @TS_TE_DEPS@

sample1_SOURCES = sample1.c
sample1_LDADD = $(MYLDADD)
sample1_DEPENDENCIES = $(MYDEPS)
