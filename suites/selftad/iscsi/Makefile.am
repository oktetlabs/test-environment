
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml 


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = simple establish double

LDADD = -ltapi_rpc -ltapi_iscsi -ltapi_ipstack -ltapi_arp -ltapi_eth \
	-ltapi_tad -lndn -lasn -ltapi -lrcfrpc -lrcfapi -lrpcxdr \
	-lconfapi -lconf_oid -llogger_ten -lipc -ltools -lpthread

