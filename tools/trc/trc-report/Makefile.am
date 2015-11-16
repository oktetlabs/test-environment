
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(top_srcdir)/lib @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir) \
              $(TE_CPPFLAGS_VAR)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@ $(TE_CFLAGS_VAR)
AM_LDFLAGS = -L$(top_builddir)/lib @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

bin_PROGRAMS = te-trc-report

te_trc_report_SOURCES = main.c

te_trc_report_LDADD = -ltrc_report -ltrc -llogic_expr -llogger_file \
                      -lpthread -ltools -lpopt -lcrypto @XML2_LIBS@ \
                      @TE_LDADD@

te_trc_report_DEPENDENCIES = @TE_DEPENDENCIES@ \
	$(top_builddir)/lib/libtrc_report.a \
	$(DESTDIR)/$(libdir)/libtrc.a \
	$(DESTDIR)/$(libdir)/liblogic_expr.a \
	$(DESTDIR)/$(libdir)/liblogger_file.a \
	$(DESTDIR)/$(libdir)/libtools.a
