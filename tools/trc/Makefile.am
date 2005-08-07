
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/lib 

noinst_HEADERS = trc_log.h trc_db.h trc_xml.h trc_tag.h


bin_PROGRAMS = te_trc te_trc_diff


te_trc_SOURCES = main.c trc_tag.c db_io.c report_html.c log_parse.c

te_trc_LDADD = @TE_LDADD@ -lpopt

te_trc_DEPENDENCIES = @TE_DEPENDENCIES@


te_trc_diff_SOURCES = trc_diff.c trc_tag.c db_io.c diff_report.c

te_trc_diff_LDADD = @TE_LDADD@ -lpopt

te_trc_diff_DEPENDENCIES = @TE_DEPENDENCIES@


dist_bin_SCRIPTS = te_trc.sh
