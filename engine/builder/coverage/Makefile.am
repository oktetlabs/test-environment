
@SET_MAKE@

srcdir=@srcdir@
VPATH=@srcdir@

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS = 
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 

mybindir=$(prefix)/bin
dist_mybin_SCRIPTS=\
    tce_gcc \
    tce_gxx \
    tce_gcov \
    tce_merge \
    tce_count \
    tce_report \
    tce_summary \
    te_tce_report

#    tce_instrument_sources
#    tce_uninstrument_sources

nobase_dist_data_DATA = bbhook.c modules/Makefile-2.6 modules/bbinit.c

