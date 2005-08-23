
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml \
                   udp-data-csap.asn udp-filter.asn


AM_CPPFLAGS = -I$(DESTDIR)/$(prefix)/include 
AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = tcp_simple ip4_simple ip4_recv_send ip4_udp_dgm \
	ip4_fragments tcp_conn tcp_data

LDADD = -ltapi_rpc -ltapi_tad -ltapi_ipstack -ltapi_arp -ltapi_eth -lndn \
	-lasn -ltapi -lrcfrpc -lrcfapi -lrpcxdr -lconfapi -lconf_oid -lipc \
	-lpthread -llogger_ten  -ltools

