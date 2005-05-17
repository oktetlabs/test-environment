
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = \
    tapi_pcap_test

LDADD = -ltapi -ltapi_pcap -lndn -lasn -lrcfapi -lrpcxdr -lconfapi \
        -lconf_oid -llogger_ten -lipc -lpthread
 
