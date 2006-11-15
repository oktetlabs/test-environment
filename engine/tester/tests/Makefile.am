
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) 
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir) -static

TESTS = scen_op_1 \
	scen_op_01 \
	scen_op_02 \
	scen_op_03 \
	scen_op_04 \
	scen_op_05 \
	scen_op_06 \
	scen_op_07 \
	scen_op_08 \
	scen_op_09 \
	scen_op_10 \
	scen_op_11 \
	scen_op_12 \
	scen_op_sub_01 \
	scen_op_sub_02 \
	scen_op_sub_03 \
	scen_op_sub_04 \
	scen_op_sub_05 \
	scen_op_sub_06 \
	scen_op_sub_07 \
	scen_op_sub_08 \
	scen_op_sub_09 \
	scen_op_sub_10 \
	scen_op_sub_11 \
	scen_op_sub_12 \
	scen_op_sub_13 \
	scen_op_sub_14

XFAIL_TESTS = \
	scen_op_11 \
	scen_op_sub_05

dist_noinst_SCRIPTS = $(TESTS)

check_PROGRAMS = scen_op_test

scen_op_test_SOURCES = scen_op_test.c ../scenario.c

LDADD=-llogger_file -ltools
