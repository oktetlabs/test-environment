
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CFLAGS = -I$(DESTDIR)/$(prefix)/include \
            -I$(DESTDIR)/$(prefix)/../include

AM_LDFLAGS = @TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


tetest_PROGRAMS = udp1

LDADD = -ltapi_ipstack -lndn -lasn -ltapi -lrcfapi -lconfapi -lconf_oid \
          -lipc -lpthread -llogger_ten

tetest_DATA=${srcdir}/udp-filter.asn
