
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include
AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/lib 

noinst_HEADERS = trc_log.h trc_db.h trc_xml.h


bin_PROGRAMS = te_trc


te_trc_SOURCES = main.c db_io.c report_html.c log_parse.c

te_trc_LDADD = @TE_LDADD@ -lpopt

te_trc_DEPENDENCIES = @TE_DEPENDENCIES@


dist_bin_SCRIPTS = te_trc.sh
