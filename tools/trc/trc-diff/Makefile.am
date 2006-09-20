
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = -I$(top_srcdir)/lib @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @XML2_CFLAGS@
AM_LDFLAGS = -I$(top_builddir)/lib @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)

bin_PROGRAMS = te-trc-diff

te_trc_diff_SOURCES = main.c

te_trc_diff_LDADD = -ltrc_diff -ltrc -llogic_expr -llogger_file -ltools \
                    -lpopt @XML2_LIBS@ @TE_LDADD@

te_trc_diff_DEPENDENCIES = @TE_DEPENDENCIES@ \
	$(DESTDIR)/$(libdir)/libtrc.a \
	$(DESTDIR)/$(libdir)/liblogic_expr.a \
	$(DESTDIR)/$(libdir)/liblogger_file.a \
	$(DESTDIR)/$(libdir)/libtools.a
