
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = tapi_pcap_test


LDADD = -ltapi -ltapi_pcap -lndn -lasn -lrcfapi -lrpcxdr -lconfapi \
        -lconf_oid -llogger_ten -lipc -ltools -lpthread
 
