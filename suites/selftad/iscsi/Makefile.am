
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml 

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = simple double


LDADD = -ltapi_rpc -ltapi_iscsi -ltapi_ipstack -ltapi_arp -ltapi_eth \
	-ltapi_tad -lndn -lasn -ltapi -lrcfrpc -lrcfapi -lrpcxdr \
	-lconfapi -lconf_oid -llogger_ten -lipc -ltools -lpthread

