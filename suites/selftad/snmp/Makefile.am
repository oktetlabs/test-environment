
#EXTRA_DIST = package.dox

AM_CPPFLAGS = -I$(top_srcdir)/lib


AM_CFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
                      -I$(DESTDIR)/$(prefix)/../include -I.
AM_LDFLAGS=@TE_LDFLAGS@ -L. -L$(DESTDIR)/$(prefix)/lib

mybindir=$(bindir)/tests/$(subdir)
mydatadir=$(prefix)/data/$(subdir)

#noinst_HEADERS = checklib.h
#noinst_LIBRARIES = libcheck.a 
#libcheck_a_SOURCES = checklib.c

mybin_PROGRAMS = snmptest

MYLDADD = -ltapi_snmp -lndn -lasn -ltad_snmp -lasn -lrcfapi -lconfapi -lipc -lpthread -llogger_ten

PPFLAGS=@NET_SNMP_CFLAGS@
LDADD=$(MYLDADD) @NET_SNMP_LIBS@

