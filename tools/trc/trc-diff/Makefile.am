
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

noinst_HEADERS = trc_diff.h

bin_PROGRAMS = te-trc-diff

te_trc_diff_SOURCES = main.c diff_tags.c diff.c report_html.c

te_trc_diff_LDADD = -ltrc -llogic_expr -llogger_file -ltools \
                    -lpopt @XML2_LIBS@ @TE_LDADD@

te_trc_diff_DEPENDENCIES = @TE_DEPENDENCIES@ \
	$(DESTDIR)/$(libdir)/libtrc.a \
	$(DESTDIR)/$(libdir)/liblogic_expr.a \
	$(DESTDIR)/$(libdir)/liblogger_file.a \
	$(DESTDIR)/$(libdir)/libtools.a
