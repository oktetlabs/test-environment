
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml \
                   udp-data-csap.asn udp-filter.asn


AM_CFLAGS = -I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = udp1 tcp_simple ip4_simple

LDADD = -ltapi_rpcsock -ltapi_ipstack -lndn -lasn -ltapi -lrcfrpc -lrcfapi \
	-lconfapi -lconf_oid -lipc -lpthread -llogger_ten

