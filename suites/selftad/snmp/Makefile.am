
#EXTRA_DIST = package.dox

tetestdir=$(prefix)/@PACKAGE_NAME@/$(subdir)

dist_tetest_DATA = package.xml


AM_CPPFLAGS = -I. -I$(DESTDIR)/$(prefix)/include -I$(top_srcdir)/lib

AM_CFLAGS = @NET_SNMP_CFLAGS@

AM_LDFLAGS = @TE_LDFLAGS@ -L. -L$(DESTDIR)/$(prefix)/lib


tetest_PROGRAMS = snmp_gets snmp_get_if_table walk


MYLDADD = -ltapi_snmp -lndn -lasn -ltapi -lrcfapi -lconfapi -lconf_oid \
          -lipc -lpthread -llogger_ten

LDADD=$(MYLDADD) @NET_SNMP_LIBS@
