#
# automake file for Report Generator Configuration File Parser
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

SUBDIRS = tmpls . tests

pkgdatadir = $(datadir)/$(subdir)

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ \
              -I$(top_srcdir)/lib/tmpls @EXTRA_CPPFLAGS@
AM_CFLAGS = @TE_CFLAGS@ @GLIB_CFLAGS@ @XML2_CFLAGS@

GEN_LDADD = -L. -L$(top_builddir)/lib/tmpls @GLIB_LIBS@ @TE_LDFLAGS@ \
            @XML2_LIBS@  -ltmpls -lpopt
GEN_DEP = $(top_builddir)/lib/tmpls/libtmpls.a


bin_PROGRAMS = rgt-filter-xml2tcl


rgt_filter_xml2tcl_SOURCES = main.c rgt-filter-xml2tcl.h
rgt_filter_xml2tcl_LDADD = $(GEN_LDADD)
rgt_filter_xml2tcl_DEPENDENCIES = $(GEN_DEP)

BUILT_SOURCES = rgt-filter-xml2tcl.h

CLEANFILES = rgt-filter-xml2tcl.h

rgt-filter-xml2tcl.h: tmpls/Makefile
	for util in rgt-filter-xml2tcl; do \
	    echo -E "enum {" > $${util}.h; \
	    for i in `ls $(srcdir)/tmpls/*.tmpl`; do \
	        basename $${i} | sed 's,\.tmpl,,' | \
	            tr [:lower:] [:upper:] | \
	            awk '{ printf "%s", $$1 }' >> $${util}.h; \
	        echo "," >> $${util}.h; \
	    done; \
	    echo -E "};" >> $${util}.h; \
	    echo -E "const char *xml2fmt_files[] = {" >> $${util}.h; \
	    for i in `ls $(srcdir)/tmpls/*.tmpl`; do \
	        basename $${i} | \
	            sed "s,\(.*\),\"$(pkgdatadir)/tmpls/\1\"," | \
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
	    echo "#define INSTALL_DIR \"${pkgdatadir}/\"" >> \
	            $${util}.h; \
	done


