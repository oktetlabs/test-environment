
ACLOCAL_AMFLAGS = -I @TE_M4@

AM_CPPFLAGS = @TE_CPPFLAGS@ -I$(DESTDIR)/$(includedir)
AM_CFLAGS = @TE_CFLAGS@ @NET_SNMP_CFLAGS@


include_HEADERS=tapi_snmp.h tapi_snmp_iftable.h tapi_snmp_macro.h \
                tapi_snmp_defs.h

lib_LTLIBRARIES=libtapi_snmp.la 

libtapi_snmp_la_SOURCES=tapi_snmp.c  


extra_DIST=tests/mib_table1.c

# What is this?
#LDADD= @NET_SNMP_LIBS@ -ltapi_snmp -lasn -lrcfapi -lipc \
#       -lndn_snmp -lnetsnmp -lpthread
#LDFLAGS=-static


install-exec-hook:
	touch -c -r .libs/libtapi_snmp.la $(DESTDIR)/$(libdir)/libtapi_snmp.*

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(includedir)/$$i ; \
	done

all: install
