#
# automake file for Report Generator tests directory
# Process it with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

LOCAL_TESTS = test1 test2 test3 test4 test5 \
              test6 test7 test8 test9 test10 \
              test11 test12 test13 test14 \
              test15 test16 test17 test18 test19 test20 \
              test30 \
              test40 test41 test42

check_SCRIPTS=$(LOCAL_TESTS)
TESTS = $(LOCAL_TESTS)

EXTRA_DIST = $(LOCAL_TESTS) $(LOCAL_TESTS:%=%.xml) test_gen_inc.tcl
BUILT_SOURCES = $(LOCAL_TESTS:%=%.tcl) test_gen.tcl
CLEANFILES = $(LOCAL_TESTS:%=%.tcl) test_gen.tcl

test_gen.tcl: $(srcdir)/test_gen_inc.tcl
	cp $(srcdir)/test_gen_inc.tcl test_gen.tcl

%.tcl: %.xml test_gen.tcl $(top_builddir)/rgt-cfg/rgt-filter-xml2tcl
	$(top_builddir)/rgt-cfg/rgt-filter-xml2tcl $< > $@

