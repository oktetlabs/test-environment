
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

noinst_HEADERS = trc_log.h trc_db.h trc_xml.h trc_tag.h


bin_PROGRAMS = te_trc te_trc_diff


te_trc_SOURCES = main.c trc_tag.c db_io.c report_html.c log_parse.c

te_trc_LDADD = @TE_LDADD@ @XML2_LIBS@ -lpopt

te_trc_DEPENDENCIES = @TE_DEPENDENCIES@


te_trc_diff_SOURCES = trc_diff.c trc_tag.c db_io.c diff_report.c

te_trc_diff_LDADD = @TE_LDADD@ @XML2_LIBS@ -lpopt

te_trc_diff_DEPENDENCIES = @TE_DEPENDENCIES@


dist_bin_SCRIPTS = te_trc.sh
