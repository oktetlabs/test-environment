
AUTOMAKE_OPTIONS = foreign 

AM_CFLAGS=@TE_CFLAGS@ @NET_SNMP_CFLAGS@ -I$(DESTDIR)/$(prefix)/include \
          -I$(DESTDIR)/$(prefix)/../include

AM_LDFLAGS=@TE_LDFLAGS@ -L$(DESTDIR)/$(prefix)/lib 


lib_LIBRARIES=libtapi_snmp.a 

include_HEADERS=tapi_snmp.h tapi_snmp_iftable.h tapi_snmp_macro.h
libtapi_snmp_a_SOURCES=tapi_snmp.c  

# Tests (incorrect)
#VPATH = @srcdir@:@srcdir@/tests
#
#check_PROGRAMS=mib_table1

extra_DIST=tests/mib_table1.c

LDADD= @NET_SNMP_LIBS@ -ltapi_snmp -lasn -lrcfapi -lipc \
       -lndn_snmp -lnetsnmp -lpthread

LDFLAGS=-static

install-exec-hook:
	touch -c -r libtapi_snmp.a $(DESTDIR)/$(prefix)/lib/libtapi_snmp.a

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

