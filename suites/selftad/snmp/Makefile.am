
#EXTRA_DIST = package.dox

AM_CPPFLAGS = -I$(top_srcdir)/lib


AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include -I.
AM_LDFLAGS=@TE_LDFLAGS@ -L. -L$(DESTDIR)/$(prefix)/lib

mybindir=$(prefix)/@PACKAGE_NAME@/$(subdir)
mydatadir=$(prefix)/@PACKAGE_NAME@/$(subdir)

mybin_PROGRAMS = snmptest

MYLDADD = -ltapi_snmp -lndn -lasn -ltapi -lrcfapi -lconfapi -lconf_oid -lipc -lpthread -llogger_ten

PPFLAGS=@NET_SNMP_CFLAGS@
LDADD=$(MYLDADD) @NET_SNMP_LIBS@

