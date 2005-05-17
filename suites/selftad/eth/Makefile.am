
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = @TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS = 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = caught_any simple_recv_send vlan_recv_send \
		    eth_serial1 eth_serial2

LDADD= -ltapi -ltapi_eth -ltapi_tad -lndn -lasn -lconfapi -lrcfapi \
       -lrpcxdr -lconf_oid -llogger_ten -lipc -lpthread

MYDEPS = @SELFTAD_TE_DEPS@ @TE_INSTALL@/@host@/lib/libtapi_eth.a 

get_any_DEPENDENCIES = $(MYDEPS)

eth1_DEPENDENCIES = $(MYDEPS)

eth2_DEPENDENCIES = $(MYDEPS)

