
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = \
	caught_any \
	simple_recv_send \
	vlan_recv_send \
	eth_serial1 \
	eth_serial2

LDADD= -ltapi -ltapi_tad -lndn -lasn -ltapi_env -lrcfrpc \
       -lrpc_types -lconfapi -lrcfapi -lrpcxdr -lconf_oid -llogger_ten \
       -lipc -ltools -lpthread
