
AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/../include

AM_CFLAGS =

AM_LDFLAGS = -L$(DESTDIR)/$(prefix)/lib 


noinst_HEADERS = trc_log.h trc_db.h


bin_PROGRAMS = te_trc


te_trc_SOURCES = main.c db_io.c report_html.c

te_trc_LDADD = @TE_LDADD@ -lpopt

te_trc_DEPENDENCIES = @TE_DEPENDENCIES@


dist_bin_SCRIPTS = te_trc.sh
