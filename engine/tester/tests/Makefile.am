
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) 
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir) -static

TESTS = scen_op_1

dist_noinst_SCRIPTS = $(TESTS)

check_PROGRAMS = scen_op_test

scen_op_test_SOURCES = scen_op_test.c ../scenario.c

LDADD=-llogger_file -ltools
