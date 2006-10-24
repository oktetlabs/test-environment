
EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

noinst_HEADERS=ipstack-ts.h

AM_CPPFLAGS = @TS_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TS_CFLAGS@
AM_LDFLAGS = @TS_LDFLAGS@ -L$(DESTDIR)/$(libdir)

tetest_PROGRAMS = check_sent_data

LDADD = -ltapi_rpc -lrpc_types -ltapi_env -ltapi_tad -ltapi_ipstack -ltapi_arp \
	-ltapi_eth -lndn -lasn -ltapi -lrcfrpc -lrcfapi -lrpcxdr -lconfapi \
	-lconf_oid -llogger_ten -lipc -llog_bufs -ltools -lpthread

