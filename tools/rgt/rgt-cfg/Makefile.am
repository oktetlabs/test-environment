#
# automake file for Report Generator Configuration File Parser
# Process it with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

SUBDIRS = tmpls .

pkgdatadir = $(datadir)/$(subdir)

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              -I$(top_srcdir)/lib/tmpls @EXTRA_CPPFLAGS@ $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @GLIB_CFLAGS@ @XML_PARSER_CFLAGS@ $(TE_CFLAGS_VAR)

GEN_LDADD = -L. -L$(top_builddir)/lib/tmpls @GLIB_LIBS@ @TE_LDFLAGS@ \
            @XML_PARSER_LIBS@  -ltmpls -lpopt
GEN_DEP = $(top_builddir)/lib/tmpls/libtmpls.a


bin_PROGRAMS = rgt-filter-xml2tcl


rgt_filter_xml2tcl_SOURCES = main.c rgt-filter-xml2tcl.h
rgt_filter_xml2tcl_LDADD = $(GEN_LDADD)
rgt_filter_xml2tcl_DEPENDENCIES = $(GEN_DEP)

BUILT_SOURCES = rgt-filter-xml2tcl.h

CLEANFILES = rgt-filter-xml2tcl.h

dist-hook:
	rm -f $(distdir)/rgt-filter-xml2tcl.h

rgt-filter-xml2tcl.h: tmpls/Makefile Makefile
	for util in rgt-filter-xml2tcl; do \
	    echo -E "enum {" > $${util}.h; \
	    for i in `ls $(srcdir)/tmpls/*.tmpl`; do \
	        basename $${i} | sed 's,\.tmpl,,' | \
	            tr [:lower:] [:upper:] | \
	            awk '{ printf "%s,\n", $$1 }' >> $${util}.h; \
	    done; \
	    echo -E "};" >> $${util}.h; \
	    echo -E "const char *xml2fmt_files[] = {" >> $${util}.h; \
	    find $(srcdir)/tmpls/*.tmpl -printf "\"%f\"," >> $${util}.h; \
	    echo -E "};" >> $${util}.h; \
	    echo -E -n "rgt_tmpl_t xml2fmt_tmpls[" >> $${util}.h; \
	    echo -E -n "sizeof(xml2fmt_files) / " >> $${util}.h; \
	    echo -E "sizeof(xml2fmt_files[0])];" >> $${util}.h; \
	    echo -E -n "size_t     xml2fmt_tmpls_num = " >> $${util}.h; \
	    echo -E -n "sizeof(xml2fmt_files) / " >> $${util}.h; \
	    echo -E "sizeof(xml2fmt_files[0]);" >> $${util}.h; \
	done


