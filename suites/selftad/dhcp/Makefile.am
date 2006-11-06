
tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @NET_SNMP_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = wait_discover

LDADD= -ltapi -ltapi_tad -lndn -lasn -ltapi_rpc -lrpc_types -lrcfrpc \
       -lconfapi -lrcfapi -lrpcxdr -lconf_oid -llogger_ten -lipc \
       -llog_bufs -ltools -lpthread @NET_SNMP_LIBS@
