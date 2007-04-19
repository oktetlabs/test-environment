
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS =@TE_CFLAGS@
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(libdir) 

mybindir=$(bindir)
dist_mybin_SCRIPTS=\
    tce_gcc \
    tce_gxx \
    tce_gcov \
    tce_merge \
	tce_merge_sum tce_merge_max tce_merge_min tce_merge_diff \
    tce_report \
    tce_summary


mydatadir=$(datadir)/@PACKAGE_NAME@
nobase_dist_mydata_DATA = bbhook.c modules/Makefile-2.6 modules/tce_bbinit.c
