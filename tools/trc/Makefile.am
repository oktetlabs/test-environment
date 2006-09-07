
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LDFLAGS = -L. @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

noinst_HEADERS = trc_db.h trc_xml.h trc_tag.h


dist_bin_SCRIPTS = te_trc.sh te-trc-log

bin_PROGRAMS = te-trc-report te-trc-diff


te_trc_report_SOURCES = main.c trc_tag.c db_io.c report_html.c log_parse.c

te_trc_report_LDADD = -llogic_expr -ltools -llogger_stderr \
                      -lpopt @XML2_LIBS@ @TE_LDADD@

te_trc_report_DEPENDENCIES = @TE_DEPENDENCIES@ \
	$(DESTDIR)/$(libdir)/liblogic_expr.a \
	$(DESTDIR)/$(libdir)/libtools.a \
	$(DESTDIR)/$(libdir)/liblogger_stderr.a


te_trc_diff_SOURCES = trc_diff.c trc_tag.c db_io.c diff_report.c

te_trc_diff_LDADD = -llogic_expr -ltools -llogger_stderr \
                    -lpopt @XML2_LIBS@ @TE_LDADD@

te_trc_diff_DEPENDENCIES = @TE_DEPENDENCIES@ \
	$(DESTDIR)/$(libdir)/liblogic_expr.a \
	$(DESTDIR)/$(libdir)/libtools.a \
	$(DESTDIR)/$(libdir)/liblogger_stderr.a
