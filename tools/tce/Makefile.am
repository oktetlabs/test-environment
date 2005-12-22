
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS =@TE_CFLAGS@
AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(libdir) 

bin_PROGRAMS = tce_collector tce_dump

tce_collector_SOURCES = tce_collector.c
tce_collector_LDADD = -ltce @TE_LDADD@
tce_collector_DEPENDENCIES = @TE_DEPENDENCIES@ \
                             $(DESTDIR)/$(libdir)/libtce.a

tce_dump_SOURCES = tce_dump.c
tce_dump_LDADD = -ltce @TE_LDADD@
tce_dump_DEPENDENCIES = @TE_DEPENDENCIES@ \
                        $(DESTDIR)/$(libdir)/libtce.a

mybindir=$(bindir)
dist_mybin_SCRIPTS=\
    tce_kallsyms \
    tce_gcc \
    tce_gxx \
    tce_gcov \
    tce_merge \
    tce_count \
    tce_report \
    tce_summary \
	tce_summary_file \
	tce_merge_logs

mydatadir=$(datadir)/@PACKAGE_NAME@
nobase_dist_mydata_DATA = bbhook.c modules/Makefile-2.6 modules/tce_bbinit.c modules/tce_kmem.c
