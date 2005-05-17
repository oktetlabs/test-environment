#
# automake file for Report Generator tests directory
# Process it with automake
#
# Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
# root directory of the distribution).
#
# Test Environment is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# Test Environment is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA  02111-1307  USA
#
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

LOCAL_TESTS = test1 test2 test3 test4 test5 \
              test6 test7 test8 test9 test10 \
              test11 test12

check_SCRIPTS=$(LOCAL_TESTS)
TESTS = $(LOCAL_TESTS)

EXTRA_DIST = $(LOCAL_TESTS) $(LOCAL_TESTS:%=%.xml) test_gen_inc.tcl
BUILT_SOURCES = $(LOCAL_TESTS:%=%.tcl) test_gen.tcl
CLEANFILES = $(LOCAL_TESTS:%=%.tcl) test_gen.tcl

test_gen.tcl: $(srcdir)/test_gen_inc.tcl
	cp $(srcdir)/test_gen_inc.tcl test_gen.tcl

%.tcl: %.xml
	@XSLT_PROC@ $(top_srcdir)/rgt-core/rgt-filter-xml2tcl.xsl $< > $@

