
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = \
	send_recv \
	caught_any \
	simple_recv_send \
	vlan_recv_send \
	eth_serial1 \
	eth_serial2

LDADD= -ltapi -ltapi_eth -ltapi_tad -lndn -lasn -ltapi_env -lrcfrpc \
       -lrpc_types -lconfapi -lrcfapi -lrpcxdr -lconf_oid -llogger_ten \
       -lipc -ltools -lpthread

MYDEPS = @SELFTAD_TE_DEPS@ \
	@TE_INSTALL@/$(TE_PLATFORM)/lib/libtapi_eth.a \
	@TE_INSTALL@/$(TE_PLATFORM)/lib/libtapi_env.a

send_recv_DEPENDENCIES = $(MYDEPS)
caught_any_DEPENDENCIES = $(MYDEPS)
simple_recv_send_DEPENDENCIES = $(MYDEPS)
vlan_recv_send_DEPENDENCIES = $(MYDEPS)
eth_serial1_DEPENDENCIES = $(MYDEPS)
eth_serial2_DEPENDENCIES = $(MYDEPS)
