
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@ $(TE_CFLAGS_VAR)

#pkgdatadir = $(datadir)/$(subdir)
#dist_pkgdata_DATA = jquery_flot.js
dist_bin_SCRIPTS = jquery_include.sh jquery_flot.js

noinst_HEADERS = \
	trc_tags.h \
	trc_html.h \
	trc_report.h \
	trc_update.h \
	trc_tools.h \
	re_subst.h \
	trc_diff.h \
	log_parse.h \
	gen_wilds.h

noinst_LIBRARIES = \
	libtrc_report.a \
	libtrc_diff.a

libtrc_report_a_SOURCES = \
	tags.c \
	tools.c \
	report.c \
	update.c \
	log_parse.c \
	report_html.c \
	re_subst.c \
	html.c \
	gen_wilds.c

libtrc_diff_a_SOURCES = \
	tags.c \
	tools.c \
	diff_tags.c \
	diff.c \
	diff_html.c \
	update.c \
	log_parse.c \
	report.c \
	re_subst.c \
	html.c \
	gen_wilds.c
