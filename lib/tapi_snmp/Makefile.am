
AM_CPPFLAGS=-I$(DESTDIR)/$(prefix)/include

AM_CFLAGS=@NET_SNMP_CFLAGS@

AM_LDFLAGS=@TE_LDFLAGS@ 


include_HEADERS=tapi_snmp.h tapi_snmp_iftable.h tapi_snmp_macro.h

lib_LTLIBRARIES=libtapi_snmp.la 

libtapi_snmp_la_SOURCES=tapi_snmp.c  


# Tests (incorrect)
#VPATH = @srcdir@:@srcdir@/tests
#
#check_PROGRAMS=mib_table1

extra_DIST=tests/mib_table1.c

LDADD= @NET_SNMP_LIBS@ -ltapi_snmp -lasn -lrcfapi -lipc \
       -lndn_snmp -lnetsnmp -lpthread

LDFLAGS=-static


install-exec-hook:
	touch -c -r .libs/libtapi_snmp.la $(DESTDIR)/$(prefix)/lib/libtapi_snmp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install
