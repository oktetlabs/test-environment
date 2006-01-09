
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = simple_send

LDADD = -ltapi_rpc -ltapi_env -ltapi_tad -ltapi_ipstack -ltapi_atm \
	-ltapi_arp -ltapi_eth -lndn -lasn -ltapi -lrcfrpc -lrcfapi -lrpcxdr \
	-lconfapi -lconf_oid -llogger_ten -lipc -ltools -lpthread -lfl
