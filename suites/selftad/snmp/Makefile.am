
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = @TE_CPPFLAGS@ -I. -I$(DESTDIR)/$(includedir) \
              -I$(top_srcdir)/lib
AM_CFLAGS = @TE_CFLAGS@ @NET_SNMP_CFLAGS@
AM_LDFLAGS = @TE_LDFLAGS@ -L. -L$(DESTDIR)/$(libdir)


tetest_PROGRAMS = snmp_gets snmp_get_if_table walk trap_any


MYLDADD = -ltapi_snmp -lndn -lasn -ltapi -lrcfapi -lrpcxdr -lconfapi \
          -lconf_oid -llogger_ten -lipc -ltools -lpthread

LDADD=$(MYLDADD) @NET_SNMP_LIBS@
