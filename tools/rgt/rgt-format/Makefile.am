#
# automake file for Report Generator RGT format directory
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

RGT_SUBDIRS = xml2html xml2text
RGT_FORMAT_UTILS = $(RGT_SUBDIRS:%=rgt-%)
RGT_FORMAT_UTILS_TMPLS = $(RGT_FORMAT_UTILS:%=%.tmpls)

SUBDIRS = $(RGT_SUBDIRS) .

pkgdatadir = $(datadir)/$(subdir)
pkgdata_DATA = $(RGT_FORMAT_UTILS_TMPLS)

bin_PROGRAMS = rgt-xml-translator rgt-xml-translator-mult
rgt_xml_translator_SOURCES = main.c
rgt_xml_translator_CFLAGS = -I$(top_srcdir) @TE_CFLAGS@ \
                            -I$(top_srcdir)/lib/tmpls \
                            @XML2_CFLAGS@
rgt_xml_translator_LDADD = @TE_LDFLAGS@ \
                           -L$(top_builddir)/lib/tmpls -ltmpls -lpopt \
                           @XML2_LIBS@

rgt_xml_translator_mult_SOURCES = main_mult.c
rgt_xml_translator_mult_CFLAGS = @GLIB_CFLAGS@ -I$(top_srcdir) @TE_CFLAGS@ \
                                 @XML2_CFLAGS@
rgt_xml_translator_mult_LDADD = @GLIB_LIBS@ @TE_LDFLAGS@ -lpopt @XML2_LIBS@

AM_CFLAGS = -W -Wall -ggdb -O0

bin_SCRIPTS = $(RGT_FORMAT_UTILS)

EXTRA_DIST=rgt-xml.in

CLEANFILES=$(RGT_FORMAT_UTILS) $(RGT_FORMAT_UTILS_TMPLS)

$(RGT_FORMAT_UTILS): $(top_srcdir)/rgt-format/rgt-xml.in
	for i in $(RGT_FORMAT_UTILS) ; do \
		cat $(top_srcdir)/rgt-format/rgt-xml.in | \
			sed "s,\@SHELL\@,${SHELL},g; s,\@PACKAGE\@,${PACKAGE},g; \
				 s/\@TE_COPYRIGHT\@/${TE_COPYRIGHT}/g; \
				 s,\@UTIL_FORMAT\@,$${i#rgt-},g; \
				 s,\@UTIL_DIR\@,${pkgdatadir}/$${i#rgt-},g; \
				 s,\@VERSION\@,${VERSION},g; s,\@bindir\@,${bindir},g; \
				 s,\@pkgdatadir\@,${pkgdatadir},g; \
				 s,@utility@,$${i},g" > $${i} ; \
	done

$(RGT_FORMAT_UTILS_TMPLS): $(top_srcdir)/lib/tmpls/tmpl_files.inc.in
	for i in $(RGT_FORMAT_UTILS) ; do \
		cat $(top_srcdir)/lib/tmpls/tmpl_files.inc.in | \
			sed "s,\@datadir\@,${datadir},g; s,@subdir@,${subdir},g; \
				 s,@utility@,$${i#rgt-},g" > $${i}.tmpls ; \
	done
