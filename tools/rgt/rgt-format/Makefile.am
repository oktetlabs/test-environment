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

RGT_SUBDIRS = xml2html xml2text xml2html-multi

SUBDIRS = $(RGT_SUBDIRS) .

pkgdatadir = $(datadir)/$(subdir)

GEN_CFLAGS = -I$(top_srcdir) @TE_CFLAGS@ -I$(top_srcdir)/lib/tmpls \
             @XML2_CFLAGS@ @GLIB_CFLAGS@
GEN_LDADD = -L. -L$(top_builddir)/lib/tmpls @TE_LDFLAGS@ \
            @XML2_LIBS@ @GLIB_LIBS@ -lxml2fmt_core -ltmpls -lpopt

noinst_LIBRARIES = libxml2fmt_core.a
libxml2fmt_core_a_SOURCES = xml2fmt_core.c
libxml2fmt_core_a_CFLAGS = $(GEN_CFLAGS)

bin_PROGRAMS = rgt-xml2html rgt-xml2text rgt-xml2html-multi

rgt_xml2text_SOURCES = xml2text/xml2text.c xml2text.h
rgt_xml2text_CFLAGS = $(GEN_CFLAGS)
rgt_xml2text_LDADD = $(GEN_LDADD)

rgt_xml2html_SOURCES = xml2html/xml2html.c xml2html.h
rgt_xml2html_CFLAGS = $(GEN_CFLAGS)
rgt_xml2html_LDADD = $(GEN_LDADD)

rgt_xml2html_multi_SOURCES = xml2html-multi/xml2html-multi.c \
                             xml2html-multi.h
rgt_xml2html_multi_CFLAGS = $(GEN_CFLAGS)
rgt_xml2html_multi_LDADD = $(GEN_LDADD)

BUILT_SOURCES = xml2text.h xml2html.h xml2html-multi.h 

CLEANFILES=$(RGT_SUBDIRS:%=%.h)

$(RGT_SUBDIRS:%=%.h):
	for util in $(RGT_SUBDIRS); do \
	    echo -E "enum {" > $${util}.h; \
	    for i in `ls $(srcdir)/$${util}/tmpls/*.tmpl`; do \
	        basename $${i} | sed 's,\.tmpl,,' | \
	            tr [:lower:] [:upper:] | \
	            awk '{ printf "%s", $$1 }' >> $${util}.h; \
	        echo "," >> $${util}.h; \
	    done; \
	    echo -E "};" >> $${util}.h; \
	    echo -E "const char *xml2fmt_files[] = {" >> $${util}.h; \
	    for i in `ls $(srcdir)/$${util}/tmpls/*.tmpl`; do \
	        basename $${i} | \
	            sed "s,\(.*\),\"$(pkgdatadir)/$${util}/tmpls/\1\"," | \
	                awk '{ printf "%s", $$1 }'>> $${util}.h; \
	        echo "," >> $${util}.h; \
	    done; \
	    echo -E "};" >> $${util}.h; \
	    echo -E -n "rgt_tmpl_t xml2fmt_tmpls[" >> $${util}.h; \
	    echo -E -n "sizeof(xml2fmt_files) / " >> $${util}.h; \
	    echo -E "sizeof(xml2fmt_files[0])];" >> $${util}.h; \
	    echo -E -n "size_t     xml2fmt_tmpls_num = " >> $${util}.h; \
	    echo -E -n "sizeof(xml2fmt_files) / " >> $${util}.h; \
	    echo -E "sizeof(xml2fmt_files[0]);" >> $${util}.h; \
	    echo "#define INSTALL_DIR \"${pkgdatadir}/$${util}/\"" >> \
	            $${util}.h; \
	done
