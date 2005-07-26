
AUTOMAKE_OPTIONS = foreign 

AM_CPPFLAGS=@TE_CFLAGS@ -I$(DESTDIR)/$(prefix)/include 
AM_CFLAGS =
AM_LDFLAGS=@TE_LDFLAGS@ 

if HAS_SNMP 
AM_CPPFLAGS+=-DHAS_SNMP -DCMU_COMPATIBLE -DUCD_COMPATIBLE
SNMP_S=ndn_snmp.c
SNMP_H=ndn_snmp.h
else
SNMP_S=
SNMP_H=
endif


lib_LIBRARIES=libndn.a

include_HEADERS=ndn.h ndn_internal.h $(SNMP_H) ndn_file.h ndn_forw.h \
                ndn_dhcp.h ndn_eth.h ndn_ipstack.h ndn_cli.h ndn_bridge.h \
                ndn_pcap.h ndn_iscsi.h

libndn_a_SOURCES=ndn_generic.c ndn_context.c $(SNMP_S) ndn_file.c \
                 ndn_dhcp.c ndn_eth.c ndn_ipstack.c ndn_cli.c \
                 ndn_bridge.c ndn_forw.c ndn_pcap.c ndn_iscsi.c


CFLAGS=@CFLAGS@ 

install-exec-hook:
	touch -c -r libndn.a $(DESTDIR)/$(prefix)/lib/libndn.a 

install-data-hook:
	for i in $(include_HEADERS) ; do \
	    touch -c -r $(srcdir)/$$i $(DESTDIR)/$(prefix)/include/$$i ; \
	done

all: install

