
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@

noinst_HEADERS = trc_tags.h trc_html.h trc_report.h trc_diff.h

noinst_LIBRARIES = libtrc_report.a libtrc_diff.a

libtrc_report_a_SOURCES = tags.c report.c log_parse.c report_html.c html.c

libtrc_diff_a_SOURCES = tags.c diff_tags.c diff.c diff_html.c html.c
