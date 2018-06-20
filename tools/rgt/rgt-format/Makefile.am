#
# automake file for Report Generator RGT format directory
# Process it with automake
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author: Oleg N. Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
#

RGT_SUBDIRS = xml2html xml2text xml2html-multi

SUBDIRS = $(RGT_SUBDIRS) .

pkgdatadir = $(datadir)/$(subdir)

AM_CPPFLAGS = -I$(top_srcdir) @TE_CPPFLAGS@ \
              -I$(top_srcdir)/lib/tmpls @EXTRA_CPPFLAGS@ $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @GLIB_CFLAGS@ @XML_PARSER_CFLAGS@ $(TE_CFLAGS_VAR)

GEN_LDADD = -L. -L$(top_builddir)/lib/tmpls -lxml2fmt_core @GLIB_LIBS@ \
            @TE_LDFLAGS@ @XML_PARSER_LIBS@  -ltmpls -lpopt
GEN_DEP = libxml2fmt_core.a $(top_builddir)/lib/tmpls/libtmpls.a

noinst_LIBRARIES = libxml2fmt_core.a libcapture.a
libxml2fmt_core_a_SOURCES = xml2fmt_core.c
libcapture_a_SOURCES = capture.c capture.h

noinst_HEADERS = xml2gen.h

bin_PROGRAMS = rgt-xml2html rgt-xml2text rgt-xml2html-multi

rgt_xml2text_SOURCES = xml2text/xml2text.c xml2text.h capture.h
rgt_xml2text_LDADD = $(GEN_LDADD) -lcapture
rgt_xml2text_DEPENDENCIES = $(GEN_DEP)

rgt_xml2html_SOURCES = xml2html/xml2html.c xml2html.h
rgt_xml2html_LDADD = $(GEN_LDADD) -lcapture
rgt_xml2html_DEPENDENCIES = $(GEN_DEP)

rgt_xml2html_multi_SOURCES = xml2html-multi/xml2html-multi.c \
                             xml2html-multi.h
rgt_xml2html_multi_LDADD = $(GEN_LDADD) -lcapture
rgt_xml2html_multi_DEPENDENCIES = $(GEN_DEP)

BUILT_SOURCES = xml2text.h xml2html.h xml2html-multi.h 

CLEANFILES=$(RGT_SUBDIRS:%=%.h)

dist-hook:
	for util in $(RGT_SUBDIRS); do \
	    rm -f $(distdir)/$${util}.h; \
	done

$(RGT_SUBDIRS:%=%.h): $(RGT_SUBDIRS:%=%/tmpls/Makefile) Makefile
	util=`echo $@ | sed 's,\.h$$,,'` ; \
	echo -E "enum {" > $@; \
	for i in `ls $(srcdir)/$${util}/tmpls/*.tmpl`; do \
		basename $${i} | sed 's,\.tmpl$$,,' | \
			tr [:lower:] [:upper:] | \
			awk '{ printf "%s", $$1 }' >> $@; \
		echo "," >> $@; \
	done; \
	echo -E "};" >> $@; \
	echo -E "const char *xml2fmt_files[] = {" >> $@; \
	find $(srcdir)/$${util}/tmpls/*.tmpl -printf "\"%f\"," >> $@; \
	echo -E "};" >> $@; \
	echo -E -n "rgt_tmpl_t xml2fmt_tmpls[" >> $@; \
	echo -E -n "sizeof(xml2fmt_files) / " >> $@; \
	echo -E "sizeof(xml2fmt_files[0])];" >> $@; \
	echo -E -n "size_t     xml2fmt_tmpls_num = " >> $@; \
	echo -E -n "sizeof(xml2fmt_files) / " >> $@; \
	echo -E "sizeof(xml2fmt_files[0]);" >> $@;
